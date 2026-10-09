#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaMinimapWidget.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaCatalogueIconLibrary.h"
#include "KalmalaIconWidget.h"
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
    TestEqual(TEXT("Replicated duration rounded up"), Entries[0].Duration, FString(TEXT("32 s")));
    TestEqual(TEXT("Untimed heat"), Entries[2].Duration, FString(TEXT("ongoing")));
    TestEqual(TEXT("Storm distinct"), Entries[5].Icon, EKalmalaIcon::Storm);
    TestEqual(TEXT("Server weather interval"), Entries[5].Duration, FString(TEXT("90 s")));
    S.Statuses.Reset(); S.ActiveSupportEffectExpiry = 45;
    TestEqual(TEXT("Expired statuses removed together"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 3);
    S.bHasWeatherState = false; S.Exposure.HeatIntensity = 0; S.Exposure.ColdIntensity = 0;
    TestEqual(TEXT("Last expiry returns to empty"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 0);
    UKalmalaMinimapWidget* Minimap = NewObject<UKalmalaMinimapWidget>();
    Minimap->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot MinimapSlot = UGameViewportSubsystem::Get()->GetWidgetSlot(Minimap);
    const FVector2D GroupPosition = Minimap->GetStatusGroupViewportPosition();
    TestEqual(TEXT("Status group shares the minimap's top inset"), GroupPosition.Y, MinimapSlot.Offsets.Top);
    TestEqual(TEXT("Status group right edge uses the minimap's actual left edge and 12-unit gap"),
        GroupPosition.X, MinimapSlot.Offsets.Left - MinimapSlot.Offsets.Right - UKalmalaMinimapWidget::StatusGroupGap);

    UKalmalaStatusHotbarWidget* Hotbar = NewObject<UKalmalaStatusHotbarWidget>();
    const FVector2D SlotSize = UKalmalaStatusHotbarWidget::CalculateSize(6, 100, FVector2D(1280, 720));
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
            const FVector2D Size = UKalmalaStatusHotbarWidget::CalculateSize(6, TextScale, LogicalViewport);
            const FVector2D ScaledGroupPosition = UKalmalaMinimapWidget::GetDefaultStatusGroupViewportPosition();
            const float MinimapLeft = LogicalViewport.X - UKalmalaMinimapWidget::ViewportInset
                - UKalmalaMinimapWidget::DefaultMapDiameter;
            const float GroupRight = LogicalViewport.X + ScaledGroupPosition.X;
            const float GroupLeft = GroupRight - Size.X;
            TestEqual(TEXT("DPI-scaled layout keeps the status group 12 units left of the map"),
                MinimapLeft - GroupRight, UKalmalaMinimapWidget::StatusGroupGap);
            TestTrue(TEXT("DPI-scaled status group stays inside the left viewport edge"), GroupLeft >= 0.0f);
            TestEqual(TEXT("DPI-scaled layout shares the map's top margin"), ScaledGroupPosition.Y, UKalmalaMinimapWidget::ViewportInset);
            TestTrue(TEXT("DPI-scaled status group fits vertically"), ScaledGroupPosition.Y + Size.Y <= LogicalViewport.Y);
            TestEqual(TEXT("Empty has zero height"), UKalmalaStatusHotbarWidget::CalculateSize(0, TextScale, LogicalViewport).Y, 0.0);
        }
    UGameViewportSubsystem::Get()->RemoveWidget(Minimap);
    UGameViewportSubsystem::Get()->RemoveWidget(Hotbar);
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
