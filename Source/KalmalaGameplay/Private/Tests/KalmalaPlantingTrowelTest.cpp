#include "KalmalaCombatComponent.h"
#include "KalmalaPlayerModelComponent.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlantingTrowelTest,
    "Kalmala.Gameplay.Tools.PlantingTrowelCatalogueAndRepair",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlantingTrowelTest::RunTest(const FString& Parameters)
{
    const UKalmalaToolCatalogue* Tools = UKalmalaToolCatalogue::Get();
    const FKalmalaToolProfileData* Profile = Tools ? Tools->FindProfile(TEXT("PlantingTrowel")) : nullptr;
    const FKalmalaToolDefinition* Trowel = FKalmalaToolLifecycleContract::FindDefinition(TEXT("PlantingTrowel"));
    if (!TestNotNull(TEXT("Carried-tool catalogue loads"), Tools)
        || !TestNotNull(TEXT("Planting Trowel profile is registered"), Profile)
        || !TestNotNull(TEXT("Planting Trowel lifecycle definition is registered"), Trowel)) return false;

    TestTrue(TEXT("Tool catalogue validates with the Trowel profile"), Tools->IsValidCatalogue());
    TestEqual(TEXT("Trowel is a Utility carried tool"), Profile->ToolSet, FString(TEXT("Utility")));
    TestEqual(TEXT("Trowel has its own tool identity"), Trowel->Kind, EKalmalaToolKind::PlantingTrowel);
    TestEqual(TEXT("Trowel is held for plot interactions instead of generic harvesting"),
        Trowel->Action, EKalmalaToolAction::None);
    TestEqual(TEXT("Trowel has 100 condition"), Trowel->MaxDurability, 100);
    TestEqual(TEXT("Trowel weighs one kilogram"), Profile->WeightKg, 1.0f);
    TestTrue(TEXT("Trowel has no attack data"), Trowel->Attacks.IsEmpty());
    TestFalse(TEXT("Trowel cannot be used for generic melee"),
        UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("PlantingTrowel")));
    TestEqual(TEXT("Trowel uses its held visual"),
        UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(TEXT("PlantingTrowel")),
        EKalmalaHeldToolVisual::Trowel);

    const FKalmalaToolRecipeData* Recipe = Tools->Recipes.FindByPredicate(
        [](const FKalmalaToolRecipeData& Candidate)
        {
            return Candidate.RecipeId == TEXT("PlantingTrowel")
                && Candidate.ItemId == TEXT("PlantingTrowel") && Candidate.Level == 1;
        });
    if (!TestNotNull(TEXT("Level-one Forge acquisition recipe exists"), Recipe)) return false;
    TestEqual(TEXT("Trowel requires Forge level one"), Recipe->RequiredStation, FString(TEXT("Forge")));
    TestEqual(TEXT("Trowel station level"), Recipe->RequiredStationLevel, 1);
    const FKalmalaToolRecipeIngredientData* Wood = Recipe->Ingredients.FindByPredicate(
        [](const FKalmalaToolRecipeIngredientData& Cost) { return Cost.ItemId == TEXT("Wood"); });
    const FKalmalaToolRecipeIngredientData* Bronze = Recipe->Ingredients.FindByPredicate(
        [](const FKalmalaToolRecipeIngredientData& Cost) { return Cost.ItemId == TEXT("Bronze"); });
    if (!TestNotNull(TEXT("Recipe includes Wood"), Wood)
        || !TestNotNull(TEXT("Recipe includes Bronze"), Bronze)) return false;
    TestEqual(TEXT("Recipe consumes ten Wood"), Wood->Quantity, 10);
    TestEqual(TEXT("Recipe consumes five Bronze bars"), Bronze->Quantity, 5);

    FKalmalaToolState Worn;
    Worn.ItemId = TEXT("PlantingTrowel");
    Worn.ToolLevel = 1;
    Worn.Durability = 73;
    Worn.InstanceGuid = FGuid::NewGuid();
    FKalmalaToolState Repaired;
    TestTrue(TEXT("Workbench or Forge free repair accepts a worn Trowel"),
        FKalmalaToolLifecycleContract::BuildServerFreeRepair(true, true, Worn, Repaired));
    TestEqual(TEXT("Free repair restores Trowel condition"), Repaired.Durability, 100);
    return true;
}

#endif
