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

    // Round-trip the complete worn set, rather than only the boots/helmet fixtures below.
    const TPair<FName, EKalmalaGearSlot> WearableSlots[] = {
        {TEXT("BoarHideHelmet"), EKalmalaGearSlot::Head},
        {TEXT("BoarHideCape"), EKalmalaGearSlot::Cape},
        {TEXT("BoarHideTunic"), EKalmalaGearSlot::Chest},
        {TEXT("WoodShield"), EKalmalaGearSlot::Shield},
        {TEXT("BoarHideLeggings"), EKalmalaGearSlot::Legs},
        {TEXT("BoarHideBoots"), EKalmalaGearSlot::Boots}
    };
    TArray<FKalmalaInventoryStack> CompletePack;
    TArray<FGuid> WearableGuids;
    for (const auto& Entry : WearableSlots)
    {
        const FGuid Guid = FGuid::NewGuid();
        WearableGuids.Add(Guid);
        CompletePack.Add(MakeTestStack(Entry.Key, Guid));
    }
    TArray<FKalmalaEquippedGear> CompleteGear;
    FString CompleteReason;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(WearableSlots); ++Index)
    {
        TArray<FKalmalaInventoryStack> CandidatePack;
        TArray<FKalmalaEquippedGear> CandidateGear;
        if (!TestTrue(WearableSlots[Index].Key.ToString() + TEXT(" equips alongside other worn slots"),
            UKalmalaInventoryComponent::BuildEquipGearTransaction(CompletePack, CompleteGear,
                WearableGuids[Index], {}, CandidatePack, CandidateGear, CompleteReason))) return false;
        TestTrue(TEXT("Catalogue item enters its exact slot with its physical GUID"),
            CandidateGear.ContainsByPredicate([&](const FKalmalaEquippedGear& Entry)
            {
                return Entry.ItemId == WearableSlots[Index].Key && Entry.Slot == WearableSlots[Index].Value
                    && Entry.ItemGuid == WearableGuids[Index];
            }));
        CompletePack = MoveTemp(CandidatePack);
        CompleteGear = MoveTemp(CandidateGear);
        TestEqual(TEXT("Equipping retains previously worn slots"), CompleteGear.Num(), Index + 1);
    }
    TestTrue(TEXT("Wearing all six items vacates every original pack row"), CompletePack.IsEmpty());
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(WearableSlots); ++Index)
    {
        TArray<FKalmalaInventoryStack> CandidatePack;
        TArray<FKalmalaEquippedGear> CandidateGear;
        if (!TestTrue(WearableSlots[Index].Key.ToString() + TEXT(" returns without disturbing other slots"),
            UKalmalaInventoryComponent::BuildUnequipGearTransaction(CompletePack, CompleteGear,
                WearableSlots[Index].Value, {}, CandidatePack, CandidateGear, CompleteReason))) return false;
        TestTrue(TEXT("Returned item preserves its catalogue and physical identity"),
            CandidatePack.ContainsByPredicate([&](const FKalmalaInventoryStack& Stack)
            {
                return Stack.ItemId == WearableSlots[Index].Key && Stack.Quantity == 1
                    && Stack.ItemGuids.Contains(WearableGuids[Index]);
            }));
        CompletePack = MoveTemp(CandidatePack);
        CompleteGear = MoveTemp(CandidateGear);
        TestEqual(TEXT("Returning one item preserves every other worn item"), CompleteGear.Num(), 5 - Index);
    }
    TestEqual(TEXT("Complete round-trip restores six carried rows"), CompletePack.Num(), 6);
    TestTrue(TEXT("Complete round-trip clears the worn set"), CompleteGear.IsEmpty());

    const FGuid CuirassGuid = FGuid::NewGuid();
    const FGuid VestGuid = FGuid::NewGuid();
    TArray<FKalmalaInventoryStack> CuirassPack = { MakeTestStack(TEXT("ScaleCuirass"), CuirassGuid) };
    FKalmalaEquippedGear WornVest;
    WornVest.Slot = EKalmalaGearSlot::Chest;
    WornVest.ItemId = TEXT("BadgerFurVest");
    WornVest.ItemGuid = VestGuid;
    TArray<FKalmalaInventoryStack> CuirassAfterPack;
    TArray<FKalmalaEquippedGear> CuirassAfterGear;
    FString CuirassReason;
    TestTrue(TEXT("Scale Cuirass can replace a worn chest item by its exact GUID"),
        UKalmalaInventoryComponent::BuildEquipGearTransaction(CuirassPack, {WornVest}, CuirassGuid, {},
            CuirassAfterPack, CuirassAfterGear, CuirassReason));
    TestTrue(TEXT("Scale Cuirass becomes the chest wearable"),
        CuirassAfterGear.ContainsByPredicate([CuirassGuid](const FKalmalaEquippedGear& Entry)
            { return Entry.ItemId == TEXT("ScaleCuirass") && Entry.Slot == EKalmalaGearSlot::Chest
                && Entry.ItemGuid == CuirassGuid; }));
    TestTrue(TEXT("Chest replacement returns the displaced vest with its GUID"),
        CuirassAfterPack.ContainsByPredicate([VestGuid](const FKalmalaInventoryStack& Stack)
            { return Stack.ItemId == TEXT("BadgerFurVest") && Stack.ItemGuids.Contains(VestGuid); }));

    const FGuid MantleGuid = FGuid::NewGuid();
    const FGuid ExistingCapeGuid = FGuid::NewGuid();
    TArray<FKalmalaInventoryStack> MantlePack = { MakeTestStack(TEXT("LynxMantle"), MantleGuid) };
    FKalmalaEquippedGear ExistingCape;
    ExistingCape.Slot = EKalmalaGearSlot::Cape;
    ExistingCape.ItemId = TEXT("BoarHideCape");
    ExistingCape.ItemGuid = ExistingCapeGuid;
    TArray<FKalmalaInventoryStack> MantleAfterPack;
    TArray<FKalmalaEquippedGear> MantleAfterGear;
    FString MantleReason;
    TestTrue(TEXT("Lynx Mantle replaces a worn Cape by its exact GUID"),
        UKalmalaInventoryComponent::BuildEquipGearTransaction(
            MantlePack, {ExistingCape}, MantleGuid, {}, MantleAfterPack, MantleAfterGear, MantleReason));
    TestTrue(TEXT("Lynx Mantle occupies the Cape slot and retains its GUID"),
        MantleAfterGear.ContainsByPredicate([MantleGuid](const FKalmalaEquippedGear& Entry)
            { return Entry.ItemId == TEXT("LynxMantle") && Entry.Slot == EKalmalaGearSlot::Cape
                && Entry.ItemGuid == MantleGuid; }));
    TestTrue(TEXT("Cape replacement returns the displaced item with its GUID"),
        MantleAfterPack.ContainsByPredicate([ExistingCapeGuid](const FKalmalaInventoryStack& Stack)
            { return Stack.ItemId == TEXT("BoarHideCape") && Stack.ItemGuids.Contains(ExistingCapeGuid); }));

    const FGuid CoatGuid = FGuid::NewGuid();
    const FGuid ExistingChestGuid = FGuid::NewGuid();
    TArray<FKalmalaInventoryStack> CoatPack = { MakeTestStack(TEXT("PolarBearCoat"), CoatGuid) };
    FKalmalaEquippedGear ExistingChest;
    ExistingChest.Slot = EKalmalaGearSlot::Chest;
    ExistingChest.ItemId = TEXT("ScaleCuirass");
    ExistingChest.ItemGuid = ExistingChestGuid;
    TArray<FKalmalaInventoryStack> CoatAfterPack;
    TArray<FKalmalaEquippedGear> CoatAfterGear;
    FString CoatReason;
    TestTrue(TEXT("Polar Bear Coat replaces a worn Chest item by its exact GUID"),
        UKalmalaInventoryComponent::BuildEquipGearTransaction(
            CoatPack, {ExistingChest}, CoatGuid, {}, CoatAfterPack, CoatAfterGear, CoatReason));
    TestTrue(TEXT("Polar Bear Coat occupies the Chest slot and retains its GUID"),
        CoatAfterGear.ContainsByPredicate([CoatGuid](const FKalmalaEquippedGear& Entry)
            { return Entry.ItemId == TEXT("PolarBearCoat") && Entry.Slot == EKalmalaGearSlot::Chest
                && Entry.ItemGuid == CoatGuid; }));
    TestTrue(TEXT("Chest replacement returns the displaced item with its GUID"),
        CoatAfterPack.ContainsByPredicate([ExistingChestGuid](const FKalmalaInventoryStack& Stack)
            { return Stack.ItemId == TEXT("ScaleCuirass") && Stack.ItemGuids.Contains(ExistingChestGuid); }));

    const FGuid BootsGuid = FGuid::NewGuid();
    TArray<FKalmalaInventoryStack> AfterStacks;
    TArray<FKalmalaEquippedGear> AfterGear;
    FString Reason;
    TestTrue(TEXT("A carried wearable equips by its physical item GUID"),
        UKalmalaInventoryComponent::BuildEquipGearTransaction(
            {MakeTestStack(TEXT("WolfPeltBoots"), BootsGuid)}, {}, BootsGuid, {}, AfterStacks, AfterGear, Reason));
    TestTrue(TEXT("Equipped gear leaves the carried grid"), AfterStacks.IsEmpty());
    TestEqual(TEXT("Wearable enters its authored slot"), AfterGear.Num(), 1);
    if (AfterGear.Num() == 1)
    {
        TestEqual(TEXT("Boots occupy Boots"), AfterGear[0].Slot, EKalmalaGearSlot::Boots);
        TestTrue(TEXT("Equipped wearable retains its item identity"), AfterGear[0].ItemGuid == BootsGuid);
        TestTrue(TEXT("Wolf Pelt Boots retain their catalogue identity"), AfterGear[0].ItemId == TEXT("WolfPeltBoots"));
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

    FullGearPack.Pop();
    const FGuid HandGuid = FGuid::NewGuid();
    UnequippedStacks.Reset();
    UnequippedGear.Reset();
    TestFalse(TEXT("The carried Main Hand tool still occupies one inventory cell"),
        UKalmalaInventoryComponent::BuildUnequipGearTransaction(
            FullGearPack, {OldHelmet}, EKalmalaGearSlot::Head, {HandGuid},
            UnequippedStacks, UnequippedGear, Reason, HandGuid));
    TestTrue(TEXT("A rejected full-capacity return leaves both candidates untouched"),
        UnequippedStacks.IsEmpty() && UnequippedGear.IsEmpty());
    TestFalse(TEXT("Main Hand still participates in wearable identity collision checks"),
        UKalmalaInventoryComponent::BuildUnequipGearTransaction(
            {}, {OldHelmet}, EKalmalaGearSlot::Head, {OldHelmetGuid},
            UnequippedStacks, UnequippedGear, Reason, OldHelmetGuid));

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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaAccessorySlotsTest,
    "Kalmala.Gameplay.Inventory.AccessorySlots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaAccessorySlotsTest::RunTest(const FString& Parameters)
{
    const FGuid Necklace = FGuid::NewGuid(), Ring = FGuid::NewGuid(), Replacement = FGuid::NewGuid();
    TArray<FKalmalaInventoryStack> Pack = {MakeTestStack(TEXT("QuartzNecklace"), Necklace),
        MakeTestStack(TEXT("MoonstoneRing"), Ring), MakeTestStack(TEXT("DuskRing"), Replacement)};
    TArray<FKalmalaEquippedGear> Gear;
    FString Reason;
    for (const FGuid Guid : {Necklace, Ring, Replacement})
    {
        TArray<FKalmalaInventoryStack> NextPack;
        TArray<FKalmalaEquippedGear> NextGear;
        if (!TestTrue(TEXT("Accessory equips or replaces its own slot"),
            UKalmalaInventoryComponent::BuildEquipGearTransaction(Pack, Gear, Guid, {}, NextPack, NextGear, Reason))) return false;
        Pack = MoveTemp(NextPack);
        Gear = MoveTemp(NextGear);
    }
    TestEqual(TEXT("Replacement retains exactly two slots"), Gear.Num(), 2);
    TestTrue(TEXT("Replaced ring returns with its original GUID"), Pack.Num() == 1 && Pack[0].ItemGuids.Contains(Ring));
    TestTrue(TEXT("Necklace remains equipped"), Gear.ContainsByPredicate([Necklace](const auto& Entry)
        { return Entry.Slot == EKalmalaGearSlot::Necklace && Entry.ItemGuid == Necklace; }));
    TArray<FKalmalaInventoryStack> Output;
    TArray<FKalmalaEquippedGear> OutputGear;
    TestFalse(TEXT("Equipped copy cannot be equipped again"), UKalmalaInventoryComponent::BuildEquipGearTransaction(
        Pack, Gear, Replacement, {}, Output, OutputGear, Reason));
    TArray<FKalmalaEquippedGear> Duplicate = Gear;
    Duplicate.Add(Gear.Last());
    TestFalse(TEXT("Duplicate accessory slots fail closed"), UKalmalaInventoryComponent::BuildUnequipGearTransaction(
        Pack, Duplicate, EKalmalaGearSlot::Ring, {}, Output, OutputGear, Reason));
    TArray<FKalmalaInventoryStack> FullPack;
    for (int32 Cell = 0; Cell < UKalmalaInventoryComponent::MaxSlots; ++Cell)
        FullPack.Add(MakeTestStack(TEXT("Stone"), FGuid::NewGuid()));
    TestFalse(TEXT("Full pack rejects accessory return unchanged"), UKalmalaInventoryComponent::BuildUnequipGearTransaction(
        FullPack, Gear, EKalmalaGearSlot::Ring, {}, Output, OutputGear, Reason));
    TestTrue(TEXT("Rejected requests publish no candidate state"), Output.IsEmpty() && OutputGear.IsEmpty());
    TestTrue(TEXT("Accessory can return to a free cell"), UKalmalaInventoryComponent::BuildUnequipGearTransaction(
        Pack, Gear, EKalmalaGearSlot::Ring, {}, Output, OutputGear, Reason));
    TestEqual(TEXT("Unequipping ring preserves necklace only"), OutputGear.Num(), 1);
    return true;
}
#endif
