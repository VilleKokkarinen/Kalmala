#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaMinimapWidget.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaCatalogueIconLibrary.h"
#include "KalmalaStatusIconLibrary.h"
#include "KalmalaIconWidget.h"
#include "KalmalaExposureResponse.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStatusHotbarTest, "Kalmala.UI.StatusHotbar.SnapshotAndLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaStatusHotbarTest::RunTest(const FString&)
{
    FKalmalaSurvivalStatusSnapshot S;
    TestEqual(TEXT("No pawn draws no chrome"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 0);
    S.bHasCharacter = true;
    TestEqual(TEXT("Empty is empty"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 0);
    S.Statuses.Add({UKalmalaPlayerStatusComponent::SteadyMealStatusId, 74});
    S.Statuses.Add({UKalmalaPlayerStatusComponent::WetStatusId, 31.2f});
    S.Statuses.Add({UKalmalaPlayerStatusComponent::WetStatusId, 80});
    S.Exposure.HeatIntensity = .6f; S.Exposure.ColdIntensity = .5f; S.Exposure.Warmth = 20;
    S.ActiveSupportEffect = EKalmalaSupportEffect::HearthShield; S.ActiveSupportEffectExpiry = 80; S.ServerTimeSeconds = 45;
    S.bHasWeatherState = true; S.Weather.ServerStartTimeSeconds = 15;
    S.Weather.PrecipitationIntensity = .9f; S.Weather.WindStrength = .9f; S.Weather.RefreshActivityLevel();
    const auto Entries = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestEqual(TEXT("Simultaneous effects deduplicate"), Entries.Num(), 6);
    if (Entries.Num() != 6) return false;
    TestEqual(TEXT("Stable order"), Entries[0].Id, UKalmalaPlayerStatusComponent::WetStatusId);
    TestEqual(TEXT("Accessible name remains in the entry model"), Entries[0].Name, FString(TEXT("Wet")));
    TestEqual(TEXT("Replicated finite status uses rounded m:ss"), Entries[0].TimerText, FString(TEXT("00:32")));
    TestEqual(TEXT("Finite meal timer uses m:ss"), Entries[1].TimerText, FString(TEXT("01:14")));
    TestTrue(TEXT("Untimed heat has no duration label"), Entries[2].TimerText.IsEmpty());
    TestTrue(TEXT("Untimed cold has no duration label"), Entries[3].TimerText.IsEmpty());
    TestEqual(TEXT("Wet raster identity"), Entries[0].StatusIconId, FName(TEXT("Wet")));
    TestEqual(TEXT("Meal raster identity"), Entries[1].StatusIconId, FName(TEXT("SteadyMeal")));
    TestEqual(TEXT("Hot raster identity"), Entries[2].StatusIconId, FName(TEXT("Heat")));
    TestEqual(TEXT("Cold raster identity"), Entries[3].StatusIconId, FName(TEXT("Cold")));
    TestEqual(TEXT("Shield maps to its canonical hearth image"), Entries[4].StatusIconId, FName(TEXT("HearthShield")));
    TestEqual(TEXT("Storm distinct"), Entries[5].Icon, EKalmalaIcon::Storm);
    TestEqual(TEXT("Weather maps to the canonical storm image"), Entries[5].StatusIconId, FName(TEXT("Storm")));
    TestTrue(TEXT("Storm has no countdown"), Entries[5].TimerText.IsEmpty());
    TestEqual(TEXT("Finite support effect uses rounded m:ss"), Entries[4].TimerText, FString(TEXT("00:35")));
    S.Statuses.Reset(); S.ActiveSupportEffectExpiry = 45;
    TestEqual(TEXT("Expired statuses removed together"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 3);
    auto HasEntry = [](const TArray<FKalmalaStatusHotbarEntry>& Values, FName Id)
    {
        return Values.ContainsByPredicate([Id](const FKalmalaStatusHotbarEntry& Entry) { return Entry.Id == Id; });
    };
    S.Exposure.Warmth = FKalmalaExposureResponse::ColdStaminaRecoveryWarmthThreshold;
    auto QualifiedExposure = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestTrue(TEXT("Hot remains while heat exposure qualifies"), HasEntry(QualifiedExposure, FName(TEXT("Heat"))));
    TestFalse(TEXT("Cold disappears at its recovery threshold"), HasEntry(QualifiedExposure, FName(TEXT("Cold"))));

    S.Weather.FogIntensity = .85f; S.Weather.PrecipitationIntensity = .1f; S.Weather.WindStrength = .2f;
    S.Weather.RefreshActivityLevel();
    const auto FogOnly = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestFalse(TEXT("Highly active fog without a storm adds no weather entry"), HasEntry(FogOnly, FName(TEXT("Weather"))));
    S.Weather.FogIntensity = 0; S.Weather.PrecipitationIntensity = .9f; S.Weather.WindStrength = .7f;
    S.Weather.RefreshActivityLevel();
    const auto BelowStormThreshold = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestFalse(TEXT("Active weather below the shared storm threshold stays hidden"), HasEntry(BelowStormThreshold, FName(TEXT("Weather"))));
    S.Weather.PrecipitationIntensity = 1.0f; S.Weather.WindStrength = FKalmalaWeatherState::HighlyActiveStormThreshold;
    S.Weather.RefreshActivityLevel();
    const auto QualifiedStorm = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestTrue(TEXT("Storm appears at the shared qualification threshold"), HasEntry(QualifiedStorm, FName(TEXT("Weather"))));
    S.ServerTimeSeconds = S.Weather.ServerStartTimeSeconds + S.Weather.DurationSeconds;
    const auto ExpiredStorm = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestFalse(TEXT("Expired weather interval removes its storm immediately"), HasEntry(ExpiredStorm, FName(TEXT("Weather"))));
    S.ServerTimeSeconds = S.Weather.ServerStartTimeSeconds - 1.0f;
    TestFalse(TEXT("Future weather interval is not treated as current"),
        HasEntry(UKalmalaStatusHotbarWidget::BuildEntries(S), FName(TEXT("Weather"))));

    S.ServerTimeSeconds = 45; S.Weather.ServerStartTimeSeconds = 15;
    S.Exposure.HeatIntensity = 0; S.Exposure.ColdIntensity = 0;
    S.Statuses.Add({UKalmalaPlayerStatusComponent::WetStatusId, 0});
    S.Statuses.Add({UKalmalaPlayerStatusComponent::SteadyMealStatusId, -1});
    S.Statuses.Add({FName(TEXT("State.Unknown")), 20});
    S.ActiveSupportEffectExpiry = 45;
    S.Weather.PrecipitationIntensity = 0; S.Weather.WindStrength = 0; S.Weather.RefreshActivityLevel();
    const auto EmptySnapshot = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestEqual(TEXT("Cleared, expired, unsupported and ordinary weather leave no end icons"), EmptySnapshot.Num(), 0);
    const FVector2D TestViewport(1280, 720);
    const FVector2D TestGroupPosition = UKalmalaMinimapWidget::GetDefaultStatusGroupViewportPosition();
    const float TestGroupRight = TestViewport.X + TestGroupPosition.X;
    const FVector2D EmptySize = UKalmalaStatusHotbarWidget::CalculateSize(0, 100, TestViewport, TestGroupRight);
    TestEqual(TEXT("Empty state has no retained width"), EmptySize.X, 0.0);
    TestEqual(TEXT("Empty state has no retained height"), EmptySize.Y, 0.0);
    const FVector2D OneSize = UKalmalaStatusHotbarWidget::CalculateSize(1, 100, TestViewport, TestGroupRight);
    TestEqual(TEXT("One state uses one compact icon cell"), OneSize.X,
        static_cast<double>(UKalmalaStatusHotbarWidget::StatusCellWidth + UKalmalaStatusHotbarWidget::StatusCellGap));
    TestEqual(TEXT("One state reserves one icon/timer row"), OneSize.Y,
        static_cast<double>(UKalmalaStatusHotbarWidget::StatusCellContentHeight + UKalmalaStatusHotbarWidget::StatusCellGap));
    S.bHasWeatherState = false;
    TestEqual(TEXT("Missing weather state also stays empty"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 0);
    UKalmalaMinimapWidget* Minimap = NewObject<UKalmalaMinimapWidget>();
    Minimap->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot MinimapSlot = UGameViewportSubsystem::Get()->GetWidgetSlot(Minimap);
    const FVector2D GroupPosition = Minimap->GetStatusGroupViewportPosition();
    TestEqual(TEXT("Status group shares the minimap's top inset"), GroupPosition.Y,
        static_cast<double>(MinimapSlot.Offsets.Top));
    TestEqual(TEXT("Status group right edge uses the minimap's actual left edge and 12-unit gap"),
        GroupPosition.X, static_cast<double>(MinimapSlot.Offsets.Left - MinimapSlot.Offsets.Right - UKalmalaMinimapWidget::StatusGroupGap));

    UKalmalaStatusHotbarWidget* Hotbar = NewObject<UKalmalaStatusHotbarWidget>();
    const FVector2D SlotSize = UKalmalaStatusHotbarWidget::CalculateSize(6, 100, TestViewport, TestGroupRight);
    Hotbar->ConfigureViewportPlacement(SlotSize, GroupPosition);
    const FGameViewportWidgetSlot HotbarSlot = UGameViewportSubsystem::Get()->GetWidgetSlot(Hotbar);
    TestTrue(TEXT("Status group uses the same top-right anchor and right alignment as the minimap"),
        HotbarSlot.Anchors == MinimapSlot.Anchors && HotbarSlot.Alignment == MinimapSlot.Alignment);
    TestEqual(TEXT("Actual status slot shares the map's top offset"), HotbarSlot.Offsets.Top, MinimapSlot.Offsets.Top);
    TestEqual(TEXT("Actual status slot ends 12 UI units left of the map"), HotbarSlot.Offsets.Left,
        MinimapSlot.Offsets.Left - MinimapSlot.Offsets.Right - UKalmalaMinimapWidget::StatusGroupGap);

    for (const FVector2D View : {FVector2D(1024,768), FVector2D(1280,720), FVector2D(2560,1080)})
        for (const float DpiScale : {0.75f, 1.0f, 1.25f})
            for (const int32 TextScale : {100,125,150})
        {
            const FVector2D LogicalViewport = View / DpiScale;
            const FVector2D ScaledGroupPosition = UKalmalaMinimapWidget::GetDefaultStatusGroupViewportPosition();
            const float ScaledGroupRight = LogicalViewport.X + ScaledGroupPosition.X;
            const FVector2D ScaledEmptySize = UKalmalaStatusHotbarWidget::CalculateSize(0, TextScale, LogicalViewport, ScaledGroupRight);
            const FVector2D ScaledOneSize = UKalmalaStatusHotbarWidget::CalculateSize(1, TextScale, LogicalViewport, ScaledGroupRight);
            const FVector2D ThreeSize = UKalmalaStatusHotbarWidget::CalculateSize(3, TextScale, LogicalViewport, ScaledGroupRight);
            const FVector2D ManySize = UKalmalaStatusHotbarWidget::CalculateSize(6, TextScale, LogicalViewport, ScaledGroupRight);
            // A narrowed HUD lane must wrap rather than covering another cue.
            const float ReservedLeft = ScaledGroupRight - (2.0f * (72.0f * TextScale / 100.0f + 4.0f) + 1.0f);
            const FVector2D ReservedSize = UKalmalaStatusHotbarWidget::CalculateSize(6, TextScale,
                LogicalViewport, ScaledGroupRight, ReservedLeft);
            TestTrue(TEXT("Status entries stay to the right of a reserved support cue"),
                ScaledGroupRight - ReservedSize.X >= ReservedLeft);
            TestTrue(TEXT("Reserved support space wraps the six entries down"), ReservedSize.Y > ManySize.Y);
            const float Scale = TextScale / 100.0f;
            const float CellWidth = UKalmalaStatusHotbarWidget::StatusCellWidth * Scale;
            const float RowHeight = UKalmalaStatusHotbarWidget::StatusCellContentHeight * Scale
                + UKalmalaStatusHotbarWidget::StatusCellGap;
            const float SafeRowWidth = FMath::Min(UKalmalaStatusHotbarWidget::MaximumStatusRowWidth,
                ScaledGroupRight - UKalmalaMinimapWidget::ViewportInset);
            const int32 ManyColumns = FMath::Max(1, FMath::FloorToInt(SafeRowWidth /
                (CellWidth + UKalmalaStatusHotbarWidget::StatusCellGap)));
            const int32 ManyRows = FMath::DivideAndRoundUp(6, ManyColumns);
            const float MinimapLeft = LogicalViewport.X - UKalmalaMinimapWidget::ViewportInset
                - UKalmalaMinimapWidget::DefaultMapDiameter;
            const float GroupRight = LogicalViewport.X + ScaledGroupPosition.X;
            TestEqual(TEXT("DPI-scaled layout keeps the status group 12 units left of the map"),
                MinimapLeft - GroupRight, UKalmalaMinimapWidget::StatusGroupGap);
            TestEqual(TEXT("Empty state has zero width at every scale and aspect ratio"), ScaledEmptySize.X, 0.0);
            TestEqual(TEXT("Empty state has zero height at every scale and aspect ratio"), ScaledEmptySize.Y, 0.0);
            TestEqual(TEXT("One state has one compact cell"), ScaledOneSize.X,
                static_cast<double>(CellWidth + UKalmalaStatusHotbarWidget::StatusCellGap));
            TestEqual(TEXT("One state remains one row"), ScaledOneSize.Y, static_cast<double>(RowHeight));
            TestEqual(TEXT("Three states fit one compact row"), ThreeSize.X,
                static_cast<double>(3.0f * (CellWidth + UKalmalaStatusHotbarWidget::StatusCellGap)));
            TestEqual(TEXT("Three states remain one row"), ThreeSize.Y, static_cast<double>(RowHeight));
            TestEqual(TEXT("Many states wrap into the predicted number of rows"), ManySize.Y,
                static_cast<double>(ManyRows * RowHeight));
            TestTrue(TEXT("Many states stay within the compact row width"), ManySize.X <= SafeRowWidth);
            TestTrue(TEXT("DPI-scaled status group stays inside the left viewport margin"),
                GroupRight - ManySize.X >= UKalmalaMinimapWidget::ViewportInset);
            TestEqual(TEXT("DPI-scaled layout shares the map's top margin"), ScaledGroupPosition.Y,
                static_cast<double>(UKalmalaMinimapWidget::ViewportInset));
            TestTrue(TEXT("DPI-scaled wrapped layout stays above the bottom safe margin"),
                ScaledGroupPosition.Y + ManySize.Y <= LogicalViewport.Y - UKalmalaMinimapWidget::ViewportInset);
        }
    UGameViewportSubsystem::Get()->RemoveWidget(Minimap);
    UGameViewportSubsystem::Get()->RemoveWidget(Hotbar);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStatusIconCoverageTest, "Kalmala.UI.StatusHotbar.StatusIconCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaStatusIconCoverageTest::RunTest(const FString&)
{
    static const TPair<FName, FName> Expected[] = {
        {TEXT("Wet"), TEXT("Wet")}, {TEXT("SteadyMeal"), TEXT("SteadyMeal")},
        {TEXT("Heat"), TEXT("Heat")}, {TEXT("Cold"), TEXT("Cold")},
        {TEXT("Mending"), TEXT("Mending")}, {TEXT("Shield"), TEXT("HearthShield")},
        {TEXT("Vigor"), TEXT("BearsVigor")}, {TEXT("Call"), TEXT("DeerCall")},
        {TEXT("Weather"), TEXT("Storm")},
    };
    TSet<FName> EntryIds;
    TSet<FName> IconIds;
    for (const TPair<FName, FName>& Mapping : Expected)
    {
        EntryIds.Add(Mapping.Key);
        IconIds.Add(Mapping.Value);
        FName ResolvedIconId = NAME_None;
        TestTrue(*FString::Printf(TEXT("%s has a status-image assignment"), *Mapping.Key.ToString()),
            FKalmalaStatusIconLibrary::GetIconIdForEntry(Mapping.Key, ResolvedIconId));
        TestEqual(TEXT("Entry resolves to its pinned canonical image"), ResolvedIconId, Mapping.Value);

        FSoftObjectPath TexturePath;
        TestTrue(*FString::Printf(TEXT("%s resolves to a texture path"), *Mapping.Value.ToString()),
            FKalmalaStatusIconLibrary::GetTextureObjectPath(Mapping.Value, TexturePath));
        TestEqual(TEXT("Status texture path matches the manifest target"), TexturePath.ToString(),
            FString::Printf(TEXT("/Game/Kalmala/UI/Icons/Status/%s.%s"), *Mapping.Value.ToString(), *Mapping.Value.ToString()));
        TestNotNull(*FString::Printf(TEXT("%s imported status texture loads"), *Mapping.Value.ToString()),
            FKalmalaStatusIconLibrary::LoadTexture(Mapping.Value));
    }
    TestEqual(TEXT("All nine entry identities are mapped once"), EntryIds.Num(), 9);
    TestEqual(TEXT("All nine canonical status images are unique"), IconIds.Num(), 9);

    FName UnknownIconId = NAME_None;
    TestFalse(TEXT("Unknown entries have no status image"),
        FKalmalaStatusIconLibrary::GetIconIdForEntry(TEXT("Forged"), UnknownIconId));
    FSoftObjectPath UnknownPath;
    TestFalse(TEXT("Unknown images cannot form a texture path"),
        FKalmalaStatusIconLibrary::GetTextureObjectPath(TEXT("Forged"), UnknownPath));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCatalogueIconTest, "Kalmala.UI.CatalogueIcons.CompleteCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaCatalogueIconTest::RunTest(const FString&)
{
    TSet<int32> Shapes;
    TSet<FName> CheckedIds;
    const auto Check = [&](FName Id)
    {
        if (CheckedIds.Contains(Id)) return;
        CheckedIds.Add(Id);
        EKalmalaIcon Icon; int32 Variant;
        TestTrue(*Id.ToString(), UKalmalaIconWidget::FindCatalogueIcon(Id, Icon, Variant));
        TestTrue(TEXT("Known original shape"), Icon != EKalmalaIcon::Unknown);
        FSoftObjectPath TexturePath;
        TestTrue(TEXT("Known identity resolves to an imported texture path"),
            FKalmalaCatalogueIconLibrary::GetTextureObjectPath(Id, TexturePath));
        TestEqual(TEXT("Texture path keeps the canonical identity"), TexturePath.ToString(),
            FString::Printf(TEXT("/Game/Kalmala/UI/Icons/Items/%s.%s"), *Id.ToString(), *Id.ToString()));
        TestNotNull(*FString::Printf(TEXT("%s imported texture loads"), *Id.ToString()),
            FKalmalaCatalogueIconLibrary::LoadTexture(Id));
        const int32 Key = static_cast<int32>(Icon)*8 + Variant;
        TestFalse(TEXT("Distinct canonical icon assignment"), Shapes.Contains(Key)); Shapes.Add(Key);
    };
    const auto* Items = UKalmalaItemCatalogue::Get();
    TestEqual(TEXT("Audited inventory item count"), Items->Items.Num(), 41);
    for (const auto& Item : Items->Items) Check(Item.ItemId);
    for (const auto& Tool : FKalmalaToolLifecycleContract::GetDefinitions()) Check(Tool.ToolId);
    for (const auto& Tool : FKalmalaToolLifecycleContract::GetTieredAxeDefinitions()) Check(Tool.ToolId);
    Check(FKalmalaToolLifecycleContract::GetConstructionHammerDefinition().ToolId);
    const auto* Recipes = UKalmalaRecipeCatalogue::Get();
    TestEqual(TEXT("Audited recipe/build count"), Recipes->Recipes.Num(), 19);
    for (const auto& Recipe : Recipes->Recipes)
    {
        const FName OutputIdentity = Recipe.GetOutputIdentity();
        Check(OutputIdentity);
        EKalmalaIcon Icon; int32 Variant;
        TestTrue(*Recipe.RecipeId.ToString(), UKalmalaIconWidget::FindCatalogueIcon(OutputIdentity, Icon, Variant));
        if (Recipe.BuildableOutput.IsNone())
            TestTrue(TEXT("Crafted recipe uses a canonical inventory identity"), Items->FindItem(Recipe.Output) != nullptr);
        else
            TestTrue(TEXT("Direct construction output is a supported buildable"),
                UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.BuildableOutput));
    }
    TestEqual(TEXT("Canonical item, tool, and construction icon count"), CheckedIds.Num(), 48);
    static const TPair<FName, FName> Aliases[] = {
        {TEXT("HearthRing"), TEXT("CampfireKit")}, {TEXT("Workbench"), TEXT("WorkbenchKit")},
        {TEXT("Forge"), TEXT("ForgeKit")}, {TEXT("WorkbenchToolRack"), TEXT("WorkbenchToolRackKit")},
        {TEXT("ForgeAnvil"), TEXT("ForgeAnvilKit")}, {TEXT("GrindingStone"), TEXT("GrindingStoneKit")},
        {TEXT("Storage"), TEXT("StorageKit")}, {TEXT("CookingRack"), TEXT("CookingRackKit")},
        {TEXT("FryingPan"), TEXT("FryingPanKit")}, {TEXT("Cauldron"), TEXT("CauldronKit")},
        {TEXT("Floor"), TEXT("FloorKit")}, {TEXT("Wall"), TEXT("WallKit")},
        {TEXT("Roof"), TEXT("RoofKit")}
    };
    for (const TPair<FName, FName>& Alias : Aliases)
    {
        FSoftObjectPath CanonicalPath;
        FSoftObjectPath AliasPath;
        EKalmalaIcon AliasIcon;
        int32 AliasVariant = 0;
        TestTrue(*FString::Printf(TEXT("%s alias keeps its canonical texture identity"), *Alias.Key.ToString()),
            UKalmalaIconWidget::FindCatalogueIcon(Alias.Value, AliasIcon, AliasVariant)
                && FKalmalaCatalogueIconLibrary::GetTextureObjectPath(Alias.Value, CanonicalPath));
        TestTrue(TEXT("Legacy output identity is canonicalized before icon lookup"), AliasIcon != EKalmalaIcon::Unknown);
        TestFalse(*FString::Printf(TEXT("%s does not create a duplicate icon path"), *Alias.Key.ToString()),
            FKalmalaCatalogueIconLibrary::GetTextureObjectPath(Alias.Key, AliasPath));
    }
    EKalmalaIcon Missing; int32 Variant;
    TestFalse(TEXT("Forged IDs have no assigned icon"), UKalmalaIconWidget::FindCatalogueIcon(TEXT("Forged"), Missing, Variant));
    TestEqual(TEXT("Unknown IDs retain the question-mark vector fallback"), Missing, EKalmalaIcon::Unknown);
    FSoftObjectPath MissingPath;
    TestFalse(TEXT("Forged IDs cannot resolve a texture path"),
        FKalmalaCatalogueIconLibrary::GetTextureObjectPath(TEXT("Forged"), MissingPath));
    auto* KnownWidget = NewObject<UKalmalaIconWidget>();
    KnownWidget->SetCatalogueIcon(TEXT("Wood"));
    TestTrue(TEXT("Imported pilot texture is loaded by the shared icon widget"), KnownWidget->HasCatalogueTexture());
    auto* UnknownWidget = NewObject<UKalmalaIconWidget>();
    UnknownWidget->SetCatalogueIcon(TEXT("Forged"));
    TestFalse(TEXT("Unknown ID draws the vector fallback without a texture"), UnknownWidget->HasCatalogueTexture());
    return true;
}
#endif
