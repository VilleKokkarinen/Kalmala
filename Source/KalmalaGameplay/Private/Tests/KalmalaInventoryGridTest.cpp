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
    TestTrue(TEXT("New items use free space in owned-item order"), UKalmalaInventoryComponent::BuildGridLayout(
        {AxeGuid, StoneGuid, WoodGuid, HammerGuidA}, Before, After));
    TestTrue(TEXT("Restored wood fills the first empty cell"), After[0] == WoodGuid);
    TestTrue(TEXT("New hammer fills the next empty cell"), After[1] == HammerGuidA);
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

    // Equipment is a state of a grid item; it never removes the owned cell.
    TArray<FGuid> Stacks;
    for (int32 I=0;I<39;++I) Stacks.Add(FGuid::NewGuid());
    TestTrue(TEXT("A held tool and thirty-nine stacks occupy forty cells"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid}, Stacks, AxeGuid, {}, After));
    const auto FullWithHand = After;
    TestTrue(TEXT("Held item remains represented exactly once"), After.Contains(AxeGuid));
    TestTrue(TEXT("Unwielding a full inventory does not need extra space"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid}, Stacks, FGuid(), After, After));
    TestTrue(TEXT("Unwielding does not move any cell"), After == FullWithHand);
    TestFalse(TEXT("Holding cannot bypass forty-cell capacity"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid,HammerGuidA}, Stacks, HammerGuidA, After, After));
    TestTrue(TEXT("Failed capacity check preserves output"), After == FullWithHand);
    TestFalse(TEXT("Unknown hand identity rejects"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid}, Stacks, HammerGuidB, After, After));
    TestFalse(TEXT("Duplicate physical tool identity rejects"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid,AxeGuid}, {}, AxeGuid, After, After));
    TestFalse(TEXT("Stack and tool identities cannot overlap"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid}, {AxeGuid}, AxeGuid, After, After));
    TArray<FGuid> HotbarBefore; HotbarBefore.Init(FGuid(),40);
    HotbarBefore[2]=AxeGuid; HotbarBefore[5]=HammerGuidA; HotbarBefore[14]=WoodGuid;
    TestTrue(TEXT("Equipping slot three keeps all saved positions"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid,HammerGuidA},{WoodGuid},AxeGuid,HotbarBefore,After));
    TestTrue(TEXT("Axe stays in slot three when equipped"),After==HotbarBefore);
    TestTrue(TEXT("Switching tools does not swap their cells"),
        UKalmalaInventoryComponent::BuildMainHandGrid({AxeGuid,HammerGuidA},{WoodGuid},HammerGuidA,After,After));
    TestTrue(TEXT("Other tools and lower rows remain unchanged"),After==HotbarBefore);
    auto* Inventory = NewObject<UKalmalaInventoryComponent>();
    TArray<FKalmalaInventoryStack> CapacityStacks;
    CapacityStacks.SetNum(36);
    TestTrue(TEXT("Four tools and thirty-six stacks share forty cells"), Inventory->CanFitContents(CapacityStacks, 4));
    TestFalse(TEXT("A fifth tool needs free inventory space"), Inventory->CanFitContents(CapacityStacks, 5));
    TestEqual(TEXT("Prototype carrying capacity is displayed in kg"), Inventory->GetCarryCapacity(), 300.0f);
    TestEqual(TEXT("Empty inventory has no carried weight"), Inventory->GetCarriedWeight(), 0.0f);
    return true;
}
#endif
