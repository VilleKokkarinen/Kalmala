#include "KalmalaCombatComponent.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaPlayerModelComponent.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBasicTorchTest,
    "Kalmala.Gameplay.Tools.BasicTorchCatalogueAndFuelSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaBasicTorchTest::RunTest(const FString& Parameters)
{
    const UKalmalaToolCatalogue* Tools = UKalmalaToolCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    const FKalmalaToolDefinition* Torch = FKalmalaToolLifecycleContract::FindDefinition(TEXT("BasicTorch"));
    if (!TestNotNull(TEXT("Torch profile catalogue loads"), Tools)
        || !TestNotNull(TEXT("Inventory recipe catalogue loads"), Recipes)
        || !TestNotNull(TEXT("Basic Torch profile is registered"), Torch)) return false;

    TestTrue(TEXT("Torch profile validates"), Tools->IsValidCatalogue());
    TestEqual(TEXT("Torch has its own utility kind"), Torch->Kind, EKalmalaToolKind::BasicTorch);
    TestEqual(TEXT("Torch has no harvest action"), Torch->Action, EKalmalaToolAction::None);
    TestTrue(TEXT("Torch has no attack entries"), Torch->Attacks.IsEmpty());
    TestFalse(TEXT("Torch is excluded from generic melee"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("BasicTorch")));
    TestEqual(TEXT("Torch uses its carried silhouette"), UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(TEXT("BasicTorch")),
        EKalmalaHeldToolVisual::Torch);

    const FKalmalaRecipe* Recipe = Recipes->Find(TEXT("BasicTorch"));
    if (!TestNotNull(TEXT("Basic Torch Inventory Craft recipe exists"), Recipe)) return false;
    TArray<FKalmalaInventoryStack> Costs;
    int32 OutputCount = 0;
    TestTrue(TEXT("Basic Torch recipe scales without an inventory stack output"),
        UKalmalaRecipeCatalogue::Scale(*Recipe, 1, Costs, OutputCount));
    TestEqual(TEXT("Torch recipe output count"), OutputCount, 1);
    const FKalmalaInventoryStack* WoodCost = Costs.FindByPredicate(
        [](const FKalmalaInventoryStack& Cost) { return Cost.ItemId == TEXT("Wood"); });
    const FKalmalaInventoryStack* AmberCost = Costs.FindByPredicate(
        [](const FKalmalaInventoryStack& Cost) { return Cost.ItemId == TEXT("Amber"); });
    if (!TestNotNull(TEXT("Torch recipe includes Wood"), WoodCost)
        || !TestNotNull(TEXT("Torch recipe includes Amber"), AmberCost)) return false;
    TestEqual(TEXT("Torch recipe consumes 2 Wood"), WoodCost->Quantity, 2);
    TestEqual(TEXT("Torch recipe consumes 1 Amber"), AmberCost->Quantity, 1);

    FKalmalaPlayerToolSaveRecord Record;
    Record.ItemId = TEXT("BasicTorch");
    Record.ToolLevel = 1;
    Record.Condition = 1;
    Record.InstanceGuid = FGuid::NewGuid();
    Record.RemainingBurnSeconds = FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds;
    TestTrue(TEXT("A fully fuelled torch is a valid owner save record"),
        UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(Record));
    Record.RemainingBurnSeconds = FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds + 1.0f;
    TestFalse(TEXT("Torch fuel above its cap is rejected"), UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(Record));
    Record.ItemId = TEXT("StoneAxe");
    Record.Condition = 1;
    Record.RemainingBurnSeconds = 1.0f;
    TestFalse(TEXT("Non-torch tools cannot persist torch fuel"), UKalmalaPlayerDiscoverySaveGameV2::IsValidToolRecord(Record));
    return true;
}

#endif
