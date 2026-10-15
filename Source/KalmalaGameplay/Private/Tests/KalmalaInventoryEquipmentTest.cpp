#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

namespace
{
FKalmalaInventoryStack MakeTestStack(const FName ItemId, const FGuid ItemGuid)
{
    FKalmalaInventoryStack Stack;
    Stack.ItemId = ItemId;
    Stack.Quantity = 1;
    Stack.InstanceGuid = FGuid::NewGuid();
    Stack.ItemGuids.Add(ItemGuid);
    return Stack;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryEquipmentTest,
    "Kalmala.Gameplay.Inventory.WornGearTransactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryEquipmentTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (!TestNotNull(TEXT("Wearable item catalogue is available"), Catalogue)) return false;

    const FGuid BootsGuid = FGuid::NewGuid();
    TArray<FKalmalaInventoryStack> AfterStacks;
    TArray<FKalmalaEquippedGear> AfterGear;
    FString Reason;
    TestTrue(TEXT("A carried wearable equips by its physical item GUID"),
        UKalmalaInventoryComponent::BuildEquipGearTransaction(
            {MakeTestStack(TEXT("BoarHideBoots"), BootsGuid)}, {}, BootsGuid, {}, AfterStacks, AfterGear, Reason));
    TestTrue(TEXT("Equipped gear leaves the carried grid"), AfterStacks.IsEmpty());
    TestEqual(TEXT("Wearable enters its authored slot"), AfterGear.Num(), 1);
    if (AfterGear.Num() == 1)
    {
        TestEqual(TEXT("Boots occupy Boots"), AfterGear[0].Slot, EKalmalaGearSlot::Boots);
        TestTrue(TEXT("Equipped wearable retains its item identity"), AfterGear[0].ItemGuid == BootsGuid);
        TestTrue(TEXT("Equipped wearable retains its catalogue identity"), AfterGear[0].ItemId == TEXT("BoarHideBoots"));
    }

    TArray<FKalmalaInventoryStack> UnequippedStacks;
    TArray<FKalmalaEquippedGear> UnequippedGear;
    TestTrue(TEXT("A worn item can return to carried inventory"),
        UKalmalaInventoryComponent::BuildUnequipGearTransaction(
            AfterStacks, AfterGear, EKalmalaGearSlot::Boots, {}, UnequippedStacks, UnequippedGear, Reason));
    TestEqual(TEXT("Unequipped item returns as one grid row"), UnequippedStacks.Num(), 1);
    if (UnequippedStacks.Num() == 1)
    {
        TestTrue(TEXT("Unequip preserves the physical item GUID"), UnequippedStacks[0].ItemGuids[0] == BootsGuid);
        TestTrue(TEXT("Returned grid row receives its own identity"), UnequippedStacks[0].InstanceGuid.IsValid());
    }
    TestTrue(TEXT("Unequip clears only the occupied slot"), UnequippedGear.IsEmpty());

    const FGuid OldHelmetGuid = FGuid::NewGuid();
    const FGuid NewHelmetGuid = FGuid::NewGuid();
    TArray<FKalmalaInventoryStack> FullPack;
    FullPack.Add(MakeTestStack(TEXT("BoarHideHelmet"), NewHelmetGuid));
    for (int32 Index = 1; Index < UKalmalaInventoryComponent::MaxSlots; ++Index)
        FullPack.Add(MakeTestStack(TEXT("Wood"), FGuid::NewGuid()));
    FKalmalaEquippedGear OldHelmet;
    OldHelmet.Slot = EKalmalaGearSlot::Head;
    OldHelmet.ItemId = TEXT("BoarHideHelmet");
    OldHelmet.ItemGuid = OldHelmetGuid;
    TestTrue(TEXT("Replacing gear succeeds when the displaced item fits in the freed cell"),
        UKalmalaInventoryComponent::BuildEquipGearTransaction(
            FullPack, {OldHelmet}, NewHelmetGuid, {}, AfterStacks, AfterGear, Reason));
    TestEqual(TEXT("Replacement preserves a full forty-cell pack"), AfterStacks.Num(), UKalmalaInventoryComponent::MaxSlots);
    TestTrue(TEXT("Replacement stores the displaced item's GUID in the pack"),
        AfterStacks.ContainsByPredicate([OldHelmetGuid](const FKalmalaInventoryStack& Stack)
            { return Stack.ItemGuids.Contains(OldHelmetGuid); }));
    TestTrue(TEXT("Replacement keeps the new item's GUID equipped"),
        AfterGear.ContainsByPredicate([NewHelmetGuid](const FKalmalaEquippedGear& Entry)
            { return Entry.ItemGuid == NewHelmetGuid; }));

    TArray<FGuid> GridGuids;
    for (const FKalmalaInventoryStack& Stack : AfterStacks) GridGuids.Add(Stack.InstanceGuid);
    TArray<FGuid> GridLayout;
    TestTrue(TEXT("Only carried stack identities enter the forty-cell grid"),
        UKalmalaInventoryComponent::BuildGridLayout(GridGuids, {}, GridLayout));
    TestFalse(TEXT("Equipped physical identity is not a grid cell"), GridLayout.Contains(NewHelmetGuid));

    TArray<FKalmalaInventoryStack> FullGearPack = FullPack;
    TestFalse(TEXT("Unequip rejects when all forty cells are occupied"),
        UKalmalaInventoryComponent::BuildUnequipGearTransaction(
            FullGearPack, {OldHelmet}, EKalmalaGearSlot::Head, {}, UnequippedStacks, UnequippedGear, Reason));

    const FGuid WoodGuid = FGuid::NewGuid();
    FKalmalaGearLevelStats HelmetStats;
    if (!TestTrue(TEXT("Helmet has authored condition"),
        Catalogue->GetGearLevelStats(OldHelmet.ItemId, 1, HelmetStats))) return false;
    FKalmalaEquippedGear WornHelmet = OldHelmet;
    WornHelmet.ConditionSpent = HelmetStats.MaxDurability;
    TestTrue(TEXT("A depleted wearable can still be returned to the pack"),
        UKalmalaInventoryComponent::BuildUnequipGearTransaction(
            {}, {WornHelmet}, EKalmalaGearSlot::Head, {}, UnequippedStacks, UnequippedGear, Reason));
    WornHelmet.ConditionSpent = -1;
    TestFalse(TEXT("Negative condition spent rejects the transaction"),
        UKalmalaInventoryComponent::BuildUnequipGearTransaction(
            {}, {WornHelmet}, EKalmalaGearSlot::Head, {}, UnequippedStacks, UnequippedGear, Reason));
    WornHelmet.ConditionSpent = HelmetStats.MaxDurability + 1;
    TestFalse(TEXT("Wear above the authored maximum rejects the transaction"),
        UKalmalaInventoryComponent::BuildUnequipGearTransaction(
            {}, {WornHelmet}, EKalmalaGearSlot::Head, {}, UnequippedStacks, UnequippedGear, Reason));

    TestFalse(TEXT("Ordinary items cannot be equipped into a gear slot"),
        UKalmalaInventoryComponent::BuildEquipGearTransaction(
            {MakeTestStack(TEXT("Wood"), WoodGuid)}, {}, WoodGuid, {}, UnequippedStacks, UnequippedGear, Reason));

    const UFunction* EquipRpc = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ServerEquipGear"));
    const UFunction* UnequipRpc = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ServerUnequipGear"));
    if (TestNotNull(TEXT("Equip uses a reflected server RPC"), EquipRpc))
        TestTrue(TEXT("Equip RPC is reliable and server-owned"), EquipRpc->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
    if (TestNotNull(TEXT("Unequip uses a reflected server RPC"), UnequipRpc))
        TestTrue(TEXT("Unequip RPC is reliable and server-owned"), UnequipRpc->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));

    auto* OwnerlessInventory = NewObject<UKalmalaInventoryComponent>();
    TestFalse(TEXT("An ownerless component cannot equip gear"), OwnerlessInventory->EquipGearFromServer(BootsGuid, Reason));
    TestFalse(TEXT("An ownerless component cannot unequip gear"),
        OwnerlessInventory->UnequipGearFromServer(EKalmalaGearSlot::Boots, Reason));
    return true;
}
#endif
