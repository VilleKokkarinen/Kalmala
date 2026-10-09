#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaCatalogueIconLibrary.h"
#include "KalmalaIconWidget.h"
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
    for (const FVector2D View : {FVector2D(1024,768), FVector2D(1280,720), FVector2D(2560,1080)})
        for (const int32 Scale : {100,125,150})
        {
            const auto Size = UKalmalaStatusHotbarWidget::CalculateSize(6, Scale, View);
            TestTrue(TEXT("Below minimap and inside viewport"), Size.X + 48 <= View.X && Size.Y + 244 + 24 <= View.Y);
            TestEqual(TEXT("Empty has zero height"), UKalmalaStatusHotbarWidget::CalculateSize(0,Scale,View).Y, 0.0);
        }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCatalogueIconTest, "Kalmala.UI.CatalogueIcons.CompleteCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaCatalogueIconTest::RunTest(const FString&)
{
    TSet<int32> Shapes;
    const auto Check = [&](FName Id)
    {
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
        TestFalse(TEXT("Distinct item/tool assignment"), Shapes.Contains(Key)); Shapes.Add(Key);
    };
    const auto* Items = UKalmalaItemCatalogue::Get();
    TestEqual(TEXT("Audited inventory count"), Items->Items.Num(), 42);
    for (const auto& Item : Items->Items) Check(Item.ItemId);
    for (const auto& Tool : FKalmalaToolLifecycleContract::GetDefinitions()) Check(Tool.ToolId);
    for (const auto& Tool : FKalmalaToolLifecycleContract::GetTieredAxeDefinitions()) Check(Tool.ToolId);
    Check(FKalmalaToolLifecycleContract::GetConstructionHammerDefinition().ToolId);
    const auto* Recipes = UKalmalaRecipeCatalogue::Get();
    TestEqual(TEXT("Audited recipe/build count"), Recipes->Recipes.Num(), 19);
    for (const auto& Recipe : Recipes->Recipes)
    {
        const FName OutputIdentity = Recipe.GetOutputIdentity();
        EKalmalaIcon Icon; int32 Variant;
        TestTrue(*Recipe.RecipeId.ToString(), UKalmalaIconWidget::FindCatalogueIcon(OutputIdentity, Icon, Variant));
        if (Recipe.BuildableOutput.IsNone())
            TestTrue(TEXT("Crafted recipe uses a canonical inventory identity"), Items->FindItem(Recipe.Output) != nullptr);
        else
            TestTrue(TEXT("Direct construction output is a supported buildable"),
                UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe.BuildableOutput));
    }
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
