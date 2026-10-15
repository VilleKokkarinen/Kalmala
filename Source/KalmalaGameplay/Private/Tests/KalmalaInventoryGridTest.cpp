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
    const FGuid WoodGuid = FGuid::NewGuid();
    const FGuid AxeGuid = FGuid::NewGuid();
    const FGuid HammerGuidA = FGuid::NewGuid();
    const FGuid HammerGuidB = FGuid::NewGuid();
    const FGuid StoneGuid = FGuid::NewGuid();
    TArray<FGuid> Before;
    Before.Init(FGuid(), 40);
    Before[9] = WoodGuid;
    Before[14] = AxeGuid;
    Before[31] = StoneGuid;
    TArray<FGuid> After;
    TestTrue(TEXT("Tools and stacks fit in one GUID-addressed grid"), UKalmalaInventoryComponent::BuildGridLayout(
        {AxeGuid, WoodGuid, StoneGuid}, Before, After));
    TestTrue(TEXT("Custom hotbar assignment survives refresh"), After[9] == WoodGuid);
    TestTrue(TEXT("Tool uses a regular inventory cell"), After[14] == AxeGuid);
    TestTrue(TEXT("Reconciliation preserves all gaps"), Before == After);
    TestTrue(TEXT("Exhausted stack is removed"), UKalmalaInventoryComponent::BuildGridLayout(
        {AxeGuid, StoneGuid}, Before, After));
    TestFalse(TEXT("Exhausted hotbar remains empty"), After[9].IsValid());
    TestTrue(TEXT("Unrelated material stays in its assigned cell"), After[31] == StoneGuid);
    Before = After;
    TestTrue(TEXT("New item uses free space"), UKalmalaInventoryComponent::BuildGridLayout(
        {AxeGuid, StoneGuid, WoodGuid, HammerGuidA}, Before, After));
    TestTrue(TEXT("New item fills first empty cell"), After[0] == HammerGuidA);
    TestTrue(TEXT("Same-type items occupy distinct cells through unique GUIDs"),
        UKalmalaInventoryComponent::BuildGridLayout({HammerGuidA, HammerGuidB}, {}, After));
    TestTrue(TEXT("First identical hammer has its own slot"), After[0] == HammerGuidA);
    TestTrue(TEXT("Second identical hammer has a separate slot"), After[1] == HammerGuidB);
    const auto BeforeRejectedDuplicateGuid = After;
    TestFalse(TEXT("Duplicate instance GUIDs fail closed"), UKalmalaInventoryComponent::BuildGridLayout(
        {HammerGuidA, HammerGuidA}, Before, After));
    TestTrue(TEXT("Rejected input preserves output"), After == BeforeRejectedDuplicateGuid);
    TArray<FGuid> Full;
    for (int32 I = 0; I < 40; ++I) Full.Add(FGuid::NewGuid());
    TestTrue(TEXT("Exactly forty identities fit"), UKalmalaInventoryComponent::BuildGridLayout(Full, {}, After));
    Full.Add(FGuid::NewGuid());
    TestFalse(TEXT("Forty-first identity cannot enter the grid"), UKalmalaInventoryComponent::BuildGridLayout(Full, {}, After));
    TestFalse(TEXT("Invalid GUID cannot occupy a cell"), UKalmalaInventoryComponent::BuildGridLayout({FGuid()}, {}, After));

    // A physical tool stays in the tool ledger but only packed tools occupy cells.
    TArray<FGuid> FortyStacks;
    for (int32 I = 0; I < 40; ++I) FortyStacks.Add(FGuid::NewGuid());
    TestTrue(TEXT("Main Hand permits forty packed stacks"), UKalmalaInventoryComponent::BuildMainHandGrid(
        {AxeGuid}, FortyStacks, AxeGuid, {}, After));
    TestFalse(TEXT("Main Hand is excluded from the grid"), After.Contains(AxeGuid));
    const auto FullWithHand = After;
    TestFalse(TEXT("Returning Main Hand rejects a full pack"), UKalmalaInventoryComponent::BuildMainHandGrid(
        {AxeGuid}, FortyStacks, FGuid(), FullWithHand, After));
    TestTrue(TEXT("Rejected return preserves the candidate output"), After == FullWithHand);
    FortyStacks.Pop();
    TestTrue(TEXT("Returning Main Hand succeeds with one free cell"), UKalmalaInventoryComponent::BuildMainHandGrid(
        {AxeGuid}, FortyStacks, FGuid(), FullWithHand, After));
    TestTrue(TEXT("Returned tool retains its exact GUID"), After.Contains(AxeGuid));
    TestTrue(TEXT("Replacement uses the cell freed by the incoming tool"), UKalmalaInventoryComponent::BuildMainHandGrid(
        {AxeGuid, HammerGuidA}, FortyStacks, HammerGuidA, After, After));
    TestTrue(TEXT("Displaced tool remains packed"), After.Contains(AxeGuid));
    TestFalse(TEXT("Replacement leaves the incoming tool outside the grid"), After.Contains(HammerGuidA));
    const auto ReplacementGrid = After;
    TestFalse(TEXT("Unknown hand identity rejects"), UKalmalaInventoryComponent::BuildMainHandGrid(
        {AxeGuid}, FortyStacks, HammerGuidB, After, After));
    TestFalse(TEXT("Duplicate tool identity rejects even when excluded from cells"), UKalmalaInventoryComponent::BuildMainHandGrid(
        {AxeGuid, AxeGuid}, {}, AxeGuid, After, After));
    TestFalse(TEXT("A stack cannot share the hand identity"), UKalmalaInventoryComponent::BuildMainHandGrid(
        {AxeGuid}, {AxeGuid}, AxeGuid, After, After));
    TestTrue(TEXT("Invalid transactions leave the prior layout intact"), After == ReplacementGrid);
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
