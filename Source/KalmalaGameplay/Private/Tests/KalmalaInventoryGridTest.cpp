#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaStorageSaveGame.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryGridTest, "Kalmala.Gameplay.Inventory.SharedGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryGridTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Ten columns"), UKalmalaInventoryComponent::Columns, 10);
    TestEqual(TEXT("Four rows"), UKalmalaInventoryComponent::Rows, 4);
    TestEqual(TEXT("World chest capacity retains its existing contract"), UKalmalaStorageSaveGame::MaxStorageSlots, 16);
    TArray<FName> Before;
    Before.Init(NAME_None, 40);
    Before[9] = TEXT("Wood");
    Before[14] = TEXT("FieldHatchet");
    Before[31] = TEXT("Stone");
    TArray<FName> After;
    TestTrue(TEXT("Tools and stacks fit in one grid"), UKalmalaInventoryComponent::BuildGridLayout(
        {TEXT("FieldHatchet"), TEXT("Wood"), TEXT("Stone")}, Before, After));
    TestEqual(TEXT("Custom hotbar assignment survives refresh"), After[9], FName(TEXT("Wood")));
    TestEqual(TEXT("Tool uses a regular inventory cell"), After[14], FName(TEXT("FieldHatchet")));
    TestTrue(TEXT("Reconciliation preserves all gaps"), Before == After);
    TestTrue(TEXT("Exhausted stack is removed"), UKalmalaInventoryComponent::BuildGridLayout(
        {TEXT("FieldHatchet"), TEXT("Stone")}, Before, After));
    TestEqual(TEXT("Exhausted hotbar remains empty"), After[9], NAME_None);
    TestEqual(TEXT("Unrelated material stays in its assigned cell"), After[31], FName(TEXT("Stone")));
    Before = After;
    TestTrue(TEXT("New item uses free space"), UKalmalaInventoryComponent::BuildGridLayout(
        {TEXT("FieldHatchet"), TEXT("Stone"), TEXT("Fibre")}, Before, After));
    TestEqual(TEXT("New stack fills first empty cell"), After[0], FName(TEXT("Fibre")));
    const auto Valid = After;
    TestFalse(TEXT("Duplicate identities fail closed"), UKalmalaInventoryComponent::BuildGridLayout(
        {TEXT("Wood"), TEXT("Wood")}, Before, After));
    TestTrue(TEXT("Rejected input preserves output"), After == Valid);
    TArray<FName> Full;
    for (int32 I = 0; I < 40; ++I) Full.Add(FName(*FString::Printf(TEXT("Item%d"), I)));
    TestTrue(TEXT("Exactly forty identities fit"), UKalmalaInventoryComponent::BuildGridLayout(Full, {}, After));
    Full.Add(TEXT("Overflow"));
    TestFalse(TEXT("Forty-first identity cannot enter the grid"), UKalmalaInventoryComponent::BuildGridLayout(Full, {}, After));
    TestFalse(TEXT("Empty identity cannot occupy a cell"), UKalmalaInventoryComponent::BuildGridLayout({NAME_None}, {}, After));
    auto* Inventory = NewObject<UKalmalaInventoryComponent>();
    TArray<FKalmalaInventoryStack> Stacks;
    Stacks.SetNum(36);
    TestTrue(TEXT("Four tools and thirty-six stacks share forty cells"), Inventory->CanFitContents(Stacks, 4));
    TestFalse(TEXT("A fifth tool needs free inventory space"), Inventory->CanFitContents(Stacks, 5));
    TestEqual(TEXT("Prototype carrying capacity is displayed in kg"), Inventory->GetCarryCapacity(), 300.0f);
    TestEqual(TEXT("Empty inventory has no carried weight"), Inventory->GetCarriedWeight(), 0.0f);
    return true;
}
#endif
