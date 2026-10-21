#include "KalmalaCombatComponent.h"
#include "KalmalaPlayerModelComponent.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCultivationRakeTest,
    "Kalmala.Gameplay.Tools.CultivationRakeCatalogueAndRepair",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaCultivationRakeTest::RunTest(const FString& Parameters)
{
    const UKalmalaToolCatalogue* Tools = UKalmalaToolCatalogue::Get();
    const FKalmalaToolProfileData* Profile = Tools ? Tools->FindProfile(TEXT("CultivationRake")) : nullptr;
    const FKalmalaToolDefinition* Rake = FKalmalaToolLifecycleContract::FindDefinition(TEXT("CultivationRake"));
    if (!TestNotNull(TEXT("Carried-tool catalogue loads"), Tools)
        || !TestNotNull(TEXT("Cultivation Rake profile is registered"), Profile)
        || !TestNotNull(TEXT("Cultivation Rake lifecycle definition is registered"), Rake)) return false;

    TestTrue(TEXT("Tool catalogue validates with the Rake profile"), Tools->IsValidCatalogue());
    TestEqual(TEXT("Rake is a Utility carried tool"), Profile->ToolSet, FString(TEXT("Utility")));
    TestEqual(TEXT("Rake has its own tool identity"), Rake->Kind, EKalmalaToolKind::CultivationRake);
    TestEqual(TEXT("Rake is held for plot interactions instead of generic harvesting"),
        Rake->Action, EKalmalaToolAction::None);
    TestEqual(TEXT("Rake has 100 condition"), Rake->MaxDurability, 100);
    TestEqual(TEXT("Rake weighs one kilogram"), Profile->WeightKg, 1.0f);
    TestTrue(TEXT("Rake has no attack data"), Rake->Attacks.IsEmpty());
    TestFalse(TEXT("Rake cannot be used for generic melee"),
        UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("CultivationRake")));
    TestEqual(TEXT("Rake uses its held visual"),
        UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(TEXT("CultivationRake")),
        EKalmalaHeldToolVisual::Rake);

    const FKalmalaToolRecipeData* Recipe = Tools->Recipes.FindByPredicate(
        [](const FKalmalaToolRecipeData& Candidate)
        {
            return Candidate.RecipeId == TEXT("CultivationRake")
                && Candidate.ItemId == TEXT("CultivationRake") && Candidate.Level == 1;
        });
    if (!TestNotNull(TEXT("Level-one Forge acquisition recipe exists"), Recipe)) return false;
    TestEqual(TEXT("Rake requires Forge level one"), Recipe->RequiredStation, FString(TEXT("Forge")));
    TestEqual(TEXT("Rake station level"), Recipe->RequiredStationLevel, 1);
    const FKalmalaToolRecipeIngredientData* Wood = Recipe->Ingredients.FindByPredicate(
        [](const FKalmalaToolRecipeIngredientData& Cost) { return Cost.ItemId == TEXT("Wood"); });
    const FKalmalaToolRecipeIngredientData* Bronze = Recipe->Ingredients.FindByPredicate(
        [](const FKalmalaToolRecipeIngredientData& Cost) { return Cost.ItemId == TEXT("Bronze"); });
    if (!TestNotNull(TEXT("Recipe includes Wood"), Wood)
        || !TestNotNull(TEXT("Recipe includes Bronze"), Bronze)) return false;
    TestEqual(TEXT("Recipe consumes ten Wood"), Wood->Quantity, 10);
    TestEqual(TEXT("Recipe consumes five Bronze bars"), Bronze->Quantity, 5);

    FKalmalaToolState Worn;
    Worn.ItemId = TEXT("CultivationRake");
    Worn.ToolLevel = 1;
    Worn.Durability = 73;
    Worn.InstanceGuid = FGuid::NewGuid();
    FKalmalaToolState Repaired;
    TestTrue(TEXT("Workbench or Forge free repair accepts a worn Rake"),
        FKalmalaToolLifecycleContract::BuildServerFreeRepair(true, true, Worn, Repaired));
    TestEqual(TEXT("Free repair restores Rake condition"), Repaired.Durability, 100);
    return true;
}

#endif
