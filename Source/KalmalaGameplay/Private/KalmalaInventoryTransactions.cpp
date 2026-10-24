#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaStorageSaveGame.h"
#include "KalmalaCharacter.h"
#include "GameFramework/Actor.h"

namespace
{
bool IsValidInventoryState(const TArray<FKalmalaInventoryStack>& Stacks, const bool bRequireInstanceGuids = false)
{
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
    if (Stacks.Num() > UKalmalaInventoryComponent::MaxSlots || Catalogue == nullptr) return false;
    TSet<FGuid> SeenStackGuids;
    TSet<FGuid> SeenItemGuids;
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        if (!Catalogue->IsValidStack(Stack.ItemId, Stack.Quantity)) return false;
        if (bRequireInstanceGuids && !Stack.InstanceGuid.IsValid()) return false;
        if (Stack.InstanceGuid.IsValid())
        {
            if (SeenStackGuids.Contains(Stack.InstanceGuid)) return false;
            SeenStackGuids.Add(Stack.InstanceGuid);
        }
        if (Stack.ItemGuids.Num() > Stack.Quantity
            || (bRequireInstanceGuids && Stack.ItemGuids.Num() != Stack.Quantity)) return false;
        for (const FGuid& ItemGuid : Stack.ItemGuids)
        {
            if (!ItemGuid.IsValid() || SeenItemGuids.Contains(ItemGuid)) return false;
            SeenItemGuids.Add(ItemGuid);
        }
    }
    return true;
}

bool IsValidInventoryQuantity(const UKalmalaItemCatalogue* Catalogue, const FName ItemId, const int32 Quantity)
{
    const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(ItemId) : nullptr;
    return Definition && Definition->MaxStack > 0 && Quantity > 0
        && int64(Quantity) <= int64(Definition->MaxStack) * UKalmalaInventoryComponent::MaxSlots;
}

FGuid MakeUniqueGuid(TSet<FGuid>& Seen)
{
    FGuid Guid;
    do { Guid = FGuid::NewGuid(); } while (!Guid.IsValid() || Seen.Contains(Guid));
    Seen.Add(Guid);
    return Guid;
}

bool EnsureInventoryGuids(TArray<FKalmalaInventoryStack>& Stacks)
{
    TSet<FGuid> SeenStackGuids;
    TSet<FGuid> SeenItemGuids;
    for (FKalmalaInventoryStack& Stack : Stacks)
    {
        if (!Stack.InstanceGuid.IsValid()) Stack.InstanceGuid = MakeUniqueGuid(SeenStackGuids);
        else if (SeenStackGuids.Contains(Stack.InstanceGuid)) return false;
        else SeenStackGuids.Add(Stack.InstanceGuid);
        if (Stack.ItemGuids.Num() > Stack.Quantity) return false;
        for (FGuid& ItemGuid : Stack.ItemGuids)
        {
            if (!ItemGuid.IsValid()) ItemGuid = MakeUniqueGuid(SeenItemGuids);
            else if (SeenItemGuids.Contains(ItemGuid)) return false;
            else SeenItemGuids.Add(ItemGuid);
        }
        while (Stack.ItemGuids.Num() < Stack.Quantity)
            Stack.ItemGuids.Add(MakeUniqueGuid(SeenItemGuids));
    }
    return true;
}

bool AddQuantityToStacks(TArray<FKalmalaInventoryStack>& Stacks, const FName ItemId, const int32 Quantity,
    const TArray<FGuid>& PreferredItemGuids = {})
{
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(ItemId) : nullptr;
    if (!IsValidInventoryQuantity(Catalogue, ItemId, Quantity) || !Definition
        || (!PreferredItemGuids.IsEmpty() && PreferredItemGuids.Num() != Quantity)) return false;

    TSet<FGuid> SeenStackGuids;
    TSet<FGuid> SeenItemGuids;
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        if (Stack.InstanceGuid.IsValid()) SeenStackGuids.Add(Stack.InstanceGuid);
        for (const FGuid& Guid : Stack.ItemGuids)
        {
            if (!Guid.IsValid() || SeenItemGuids.Contains(Guid)) return false;
            SeenItemGuids.Add(Guid);
        }
    }
    int32 PreferredIndex = 0;
    const auto AppendItemGuid = [&PreferredItemGuids, &PreferredIndex, &SeenItemGuids](FKalmalaInventoryStack& Stack)
    {
        FGuid Guid;
        if (PreferredItemGuids.IsValidIndex(PreferredIndex))
        {
            Guid = PreferredItemGuids[PreferredIndex++];
            if (!Guid.IsValid() || SeenItemGuids.Contains(Guid)) return false;
            SeenItemGuids.Add(Guid);
        }
        else Guid = MakeUniqueGuid(SeenItemGuids);
        Stack.ItemGuids.Add(Guid);
        return true;
    };

    int64 Remaining = Quantity;
    for (FKalmalaInventoryStack& Stack : Stacks)
    {
        if (Stack.ItemId != ItemId || Stack.Quantity >= Definition->MaxStack) continue;
        const int32 Added = int32(FMath::Min<int64>(Remaining, Definition->MaxStack - Stack.Quantity));
        for (int32 Unit = 0; Unit < Added; ++Unit)
            if (!AppendItemGuid(Stack)) return false;
        Stack.Quantity += Added;
        Remaining -= Added;
        if (Remaining == 0) return true;
    }

    while (Remaining > 0)
    {
        if (Stacks.Num() >= UKalmalaInventoryComponent::MaxSlots) return false;
        FKalmalaInventoryStack& Stack = Stacks.AddDefaulted_GetRef();
        Stack.ItemId = ItemId;
        Stack.InstanceGuid = MakeUniqueGuid(SeenStackGuids);
        const int32 Added = int32(FMath::Min<int64>(Remaining, Definition->MaxStack));
        for (int32 Unit = 0; Unit < Added; ++Unit)
            if (!AppendItemGuid(Stack)) return false;
        Stack.Quantity = Added;
        Remaining -= Added;
    }
    return true;
}

bool AreInventoryStatesEqual(const TArray<FKalmalaInventoryStack>& Left,
    const TArray<FKalmalaInventoryStack>& Right)
{
    if (Left.Num() != Right.Num()) return false;
    for (int32 Index = 0; Index < Left.Num(); ++Index)
    {
        if (Left[Index].ItemId != Right[Index].ItemId || Left[Index].Quantity != Right[Index].Quantity
            || Left[Index].InstanceGuid != Right[Index].InstanceGuid
            || Left[Index].ItemGuids != Right[Index].ItemGuids) return false;
    }
    return true;
}

bool TryGetGearSlot(const FString& SlotName, EKalmalaGearSlot& OutSlot)
{
    if (SlotName == TEXT("Head")) OutSlot = EKalmalaGearSlot::Head;
    else if (SlotName == TEXT("Cape")) OutSlot = EKalmalaGearSlot::Cape;
    else if (SlotName == TEXT("Chest")) OutSlot = EKalmalaGearSlot::Chest;
    else if (SlotName == TEXT("Shield")) OutSlot = EKalmalaGearSlot::Shield;
    else if (SlotName == TEXT("Legs")) OutSlot = EKalmalaGearSlot::Legs;
    else if (SlotName == TEXT("Boots")) OutSlot = EKalmalaGearSlot::Boots;
    else if (SlotName == TEXT("Necklace")) OutSlot = EKalmalaGearSlot::Necklace;
    else if (SlotName == TEXT("Ring")) OutSlot = EKalmalaGearSlot::Ring;
    else return false;
    return true;
}

bool IsValidGearSlot(const EKalmalaGearSlot Slot)
{
    return static_cast<uint8>(Slot) <= static_cast<uint8>(EKalmalaGearSlot::Ring);
}

bool IsValidGearState(const TArray<FKalmalaInventoryStack>& Stacks,
    const TArray<FKalmalaEquippedGear>& Gear, const TArray<FGuid>& OtherOwnedGuids)
{
    if (Gear.Num() > 8) return false;
    TSet<FGuid> SeenItemGuids;
    for (const FKalmalaInventoryStack& Stack : Stacks)
        for (const FGuid& ItemGuid : Stack.ItemGuids) SeenItemGuids.Add(ItemGuid);

    TSet<FGuid> SeenOtherGuids;
    for (const FGuid& Guid : OtherOwnedGuids)
    {
        if (!Guid.IsValid() || SeenOtherGuids.Contains(Guid) || SeenItemGuids.Contains(Guid)) return false;
        SeenOtherGuids.Add(Guid);
    }

    TSet<uint8> SeenSlots;
    TSet<FGuid> SeenGearGuids;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (!Catalogue) return false;
    for (const FKalmalaEquippedGear& Entry : Gear)
    {
        if (!IsValidGearSlot(Entry.Slot) || !Entry.ItemGuid.IsValid()
            || SeenSlots.Contains(static_cast<uint8>(Entry.Slot)) || SeenGearGuids.Contains(Entry.ItemGuid)
            || SeenItemGuids.Contains(Entry.ItemGuid) || SeenOtherGuids.Contains(Entry.ItemGuid)) return false;
        const FKalmalaItemDefinition* Item = Catalogue->FindItem(Entry.ItemId);
        EKalmalaGearSlot ItemSlot = EKalmalaGearSlot::Head;
        if (!Item || Item->MaxStack != 1 || !TryGetGearSlot(Item->GearSlot, ItemSlot) || ItemSlot != Entry.Slot)
            return false;
        FKalmalaGearLevelStats Stats;
        if (!Catalogue->GetGearLevelStats(Entry.ItemId, Entry.Level, Stats)
            || Entry.ConditionSpent < 0 || Entry.ConditionSpent > Stats.MaxDurability) return false;
        SeenSlots.Add(static_cast<uint8>(Entry.Slot));
        SeenGearGuids.Add(Entry.ItemGuid);
    }
    return true;
}

bool AddEquippedGearToStacks(TArray<FKalmalaInventoryStack>& Stacks, const FKalmalaEquippedGear& Gear,
    const TArray<FKalmalaEquippedGear>& AllGear, const TArray<FGuid>& OtherOwnedGuids, const FGuid MainHandGuid)
{
    const int32 PackedTools = OtherOwnedGuids.Num();
    if (Stacks.Num() >= UKalmalaInventoryComponent::MaxSlots - PackedTools) return false;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaItemDefinition* Item = Catalogue ? Catalogue->FindItem(Gear.ItemId) : nullptr;
    if (!Item || Item->MaxStack != 1 || !Gear.ItemGuid.IsValid()) return false;

    TSet<FGuid> Seen;
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        if (Stack.InstanceGuid.IsValid()) Seen.Add(Stack.InstanceGuid);
        for (const FGuid& ItemGuid : Stack.ItemGuids) Seen.Add(ItemGuid);
    }
    for (const FKalmalaEquippedGear& Entry : AllGear) Seen.Add(Entry.ItemGuid);
    for (const FGuid& Guid : OtherOwnedGuids) Seen.Add(Guid);
    Seen.Add(Gear.ItemGuid);

    FKalmalaInventoryStack ReturnedItem;
    ReturnedItem.ItemId = Gear.ItemId;
    ReturnedItem.Quantity = 1;
    ReturnedItem.InstanceGuid = MakeUniqueGuid(Seen);
    ReturnedItem.ItemGuids.Add(Gear.ItemGuid);
    Stacks.Add(MoveTemp(ReturnedItem));
    return true;
}
}

bool UKalmalaInventoryComponent::BuildExchange(const TArray<FKalmalaInventoryStack>& Before,
    const TArray<FKalmalaInventoryStack>& Costs, FName Output, int32 OutputCount,
    TArray<FKalmalaInventoryStack>& After, FString& Reason)
{
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
    Reason = TEXT("Invalid transaction");
    if (Before.Num() > MaxSlots || Costs.IsEmpty() || Costs.Num() > MaxSlots
        || (Output.IsNone() ? OutputCount != 0 : !IsValidInventoryQuantity(Catalogue, Output, OutputCount))) return false;
    if (!IsValidInventoryState(Before)) return false;
    auto Candidate = Before;
    if (!EnsureInventoryGuids(Candidate)) return false;
    TSet<FName> SeenCosts;
    for (const auto& Cost : Costs)
    {
        if (SeenCosts.Contains(Cost.ItemId) || !IsValidInventoryQuantity(Catalogue, Cost.ItemId, Cost.Quantity)) return false;
        SeenCosts.Add(Cost.ItemId);
        int64 Available = 0;
        for (const FKalmalaInventoryStack& Stack : Candidate)
            if (Stack.ItemId == Cost.ItemId) Available += Stack.Quantity;
        if (Available < Cost.Quantity)
        {
            const auto* Item = Catalogue->FindItem(Cost.ItemId);
            Reason = FString::Printf(TEXT("Need %d %s"), Cost.Quantity, *Item->DisplayName);
            return false;
        }
        int32 Remaining = Cost.Quantity;
        for (int32 Index = 0; Index < Candidate.Num() && Remaining > 0;)
        {
            FKalmalaInventoryStack& Stack = Candidate[Index];
            if (Stack.ItemId != Cost.ItemId) { ++Index; continue; }
            const int32 Removed = FMath::Min(Remaining, Stack.Quantity);
            Stack.Quantity -= Removed;
            Stack.ItemGuids.RemoveAt(0, Removed);
            Remaining -= Removed;
            if (Stack.Quantity == 0) Candidate.RemoveAt(Index);
            else ++Index;
        }
    }
    if (!Output.IsNone() && !AddQuantityToStacks(Candidate, Output, OutputCount))
    {
        Reason = TEXT("Pack output capacity reached"); return false;
    }
    After = MoveTemp(Candidate);
    Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::BuildGrant(const TArray<FKalmalaInventoryStack>& Before,
    const FName ItemId, const int32 Quantity, TArray<FKalmalaInventoryStack>& After, FString& Reason)
{
    Reason = TEXT("Invalid grant");
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
    if (!IsValidInventoryState(Before) || !IsValidInventoryQuantity(Catalogue, ItemId, Quantity)) return false;

    TArray<FKalmalaInventoryStack> Candidate = Before;
    if (!EnsureInventoryGuids(Candidate)) return false;
    if (!AddQuantityToStacks(Candidate, ItemId, Quantity))
    {
        Reason = TEXT("Pack output capacity reached");
        return false;
    }
    After = MoveTemp(Candidate);
    Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::BuildEquipGearTransaction(
    const TArray<FKalmalaInventoryStack>& BeforeStacks, const TArray<FKalmalaEquippedGear>& BeforeGear,
    const FGuid ItemGuid, const TArray<FGuid>& OtherOwnedGuids,
    TArray<FKalmalaInventoryStack>& AfterStacks, TArray<FKalmalaEquippedGear>& AfterGear, FString& Reason,
    const FGuid MainHandGuid)
{
    const int32 PackedTools = OtherOwnedGuids.Num();
    Reason = TEXT("Invalid equipment transaction");
    if (!ItemGuid.IsValid() || !IsValidInventoryState(BeforeStacks, true)
        || !IsValidGearState(BeforeStacks, BeforeGear, OtherOwnedGuids)
        || BeforeStacks.Num() > MaxSlots - PackedTools) return false;

    int32 SourceIndex = INDEX_NONE;
    int32 SourceItemIndex = INDEX_NONE;
    for (int32 Index = 0; Index < BeforeStacks.Num(); ++Index)
    {
        const int32 ItemIndex = BeforeStacks[Index].ItemGuids.IndexOfByKey(ItemGuid);
        if (ItemIndex == INDEX_NONE) continue;
        SourceIndex = Index;
        SourceItemIndex = ItemIndex;
        break;
    }
    if (SourceIndex == INDEX_NONE)
    {
        Reason = TEXT("Item is not in carried inventory");
        return false;
    }

    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaInventoryStack& Source = BeforeStacks[SourceIndex];
    const FKalmalaItemDefinition* Item = Catalogue ? Catalogue->FindItem(Source.ItemId) : nullptr;
    EKalmalaGearSlot Slot = EKalmalaGearSlot::Head;
    if (!Item || Item->MaxStack != 1 || Source.Quantity != 1
        || Source.ItemGuids.Num() != 1 || !TryGetGearSlot(Item->GearSlot, Slot))
    {
        Reason = TEXT("Item cannot occupy a gear slot");
        return false;
    }

    TArray<FKalmalaInventoryStack> CandidateStacks = BeforeStacks;
    TArray<FKalmalaEquippedGear> CandidateGear = BeforeGear;
    CandidateStacks[SourceIndex].ItemGuids.RemoveAt(SourceItemIndex);
    --CandidateStacks[SourceIndex].Quantity;
    CandidateStacks.RemoveAt(SourceIndex);

    const int32 ExistingSlotIndex = CandidateGear.IndexOfByPredicate(
        [Slot](const FKalmalaEquippedGear& Entry) { return Entry.Slot == Slot; });
    if (ExistingSlotIndex != INDEX_NONE
        && !AddEquippedGearToStacks(CandidateStacks, CandidateGear[ExistingSlotIndex], CandidateGear, OtherOwnedGuids, MainHandGuid))
    {
        Reason = TEXT("No inventory slot for replaced gear");
        return false;
    }

    FKalmalaEquippedGear Equipped;
    Equipped.Slot = Slot;
    Equipped.ItemId = Source.ItemId;
    Equipped.ItemGuid = ItemGuid;
    if (ExistingSlotIndex == INDEX_NONE) CandidateGear.Add(Equipped);
    else CandidateGear[ExistingSlotIndex] = Equipped;

    if (CandidateStacks.Num() > MaxSlots - PackedTools
        || !IsValidInventoryState(CandidateStacks, true)
        || !IsValidGearState(CandidateStacks, CandidateGear, OtherOwnedGuids))
    {
        Reason = TEXT("Inventory or gear state is invalid");
        return false;
    }
    AfterStacks = MoveTemp(CandidateStacks);
    AfterGear = MoveTemp(CandidateGear);
    Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::BuildUnequipGearTransaction(
    const TArray<FKalmalaInventoryStack>& BeforeStacks, const TArray<FKalmalaEquippedGear>& BeforeGear,
    const EKalmalaGearSlot Slot, const TArray<FGuid>& OtherOwnedGuids,
    TArray<FKalmalaInventoryStack>& AfterStacks, TArray<FKalmalaEquippedGear>& AfterGear, FString& Reason,
    const FGuid MainHandGuid)
{
    Reason = TEXT("Invalid equipment transaction");
    if (!IsValidGearSlot(Slot) || !IsValidInventoryState(BeforeStacks, true)
        || !IsValidGearState(BeforeStacks, BeforeGear, OtherOwnedGuids)) return false;

    const int32 GearIndex = BeforeGear.IndexOfByPredicate(
        [Slot](const FKalmalaEquippedGear& Entry) { return Entry.Slot == Slot; });
    if (GearIndex == INDEX_NONE)
    {
        Reason = TEXT("Gear slot is empty");
        return false;
    }

    TArray<FKalmalaInventoryStack> CandidateStacks = BeforeStacks;
    TArray<FKalmalaEquippedGear> CandidateGear = BeforeGear;
    const FKalmalaEquippedGear RemovedGear = CandidateGear[GearIndex];
    if (!AddEquippedGearToStacks(CandidateStacks, RemovedGear, CandidateGear, OtherOwnedGuids, MainHandGuid))
    {
        Reason = TEXT("No inventory space for gear");
        return false;
    }
    CandidateGear.RemoveAt(GearIndex);
    if (!IsValidInventoryState(CandidateStacks, true)
        || !IsValidGearState(CandidateStacks, CandidateGear, OtherOwnedGuids))
    {
        Reason = TEXT("Inventory or gear state is invalid");
        return false;
    }

    AfterStacks = MoveTemp(CandidateStacks);
    AfterGear = MoveTemp(CandidateGear);
    Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::TryCommitStacksFromServer(
    const TArray<FKalmalaInventoryStack>& ExpectedBefore,
    const TArray<FKalmalaInventoryStack>& CandidateAfter)
{
    AActor* Owner = GetOwner();
    TArray<FKalmalaInventoryStack> Candidate = CandidateAfter;
    if (!EnsureInventoryGuids(Candidate) || !Owner || !Owner->HasAuthority()
        || !AreInventoryStatesEqual(Stacks, ExpectedBefore) || !IsValidInventoryState(Candidate, true)) return false;
    const auto* Character = Cast<AKalmalaCharacter>(Owner);
    if (!CanFitContents(Candidate, Character ? CountPackedTools(Character->GetCarriedToolInventory()) : 0)) return false;

    const auto Before = Stacks;
    Stacks = MoveTemp(Candidate);
    SynchronizeGridFromServer();
    RecordAcceptedGains(Before, Stacks);
    Owner->ForceNetUpdate();
    return true;
}

bool UKalmalaInventoryComponent::TryExchangeFromServer(const TArray<FKalmalaInventoryStack>& Costs,
    FName Output, int32 OutputCount, FString& Reason)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) { Reason = TEXT("Server authority required"); return false; }
    TArray<FKalmalaInventoryStack> Next;
    if (!BuildExchange(Stacks, Costs, Output, OutputCount, Next, Reason)) return false;
    if (!TryCommitStacksFromServer(Stacks, Next)) { Reason = TEXT("Inventory capacity reached"); return false; }
    return true;
}

bool UKalmalaInventoryComponent::BuildTransfer(const TArray<FKalmalaInventoryStack>& Source,
    const TArray<FKalmalaInventoryStack>& Destination, const FGuid ItemGuid,
    TArray<FKalmalaInventoryStack>& NextSource, TArray<FKalmalaInventoryStack>& NextDestination, FString& Reason)
{
    Reason = TEXT("Invalid storage transfer");
    if (&NextSource == &NextDestination || !ItemGuid.IsValid()
        || !IsValidInventoryState(Source, true) || !IsValidInventoryState(Destination, true)) return false;
    TArray<FKalmalaInventoryStack> From = Source;
    TArray<FKalmalaInventoryStack> To = Destination;
    int32 SourceStackIndex = INDEX_NONE;
    int32 SourceUnitIndex = INDEX_NONE;
    for (int32 StackIndex = 0; StackIndex < From.Num() && SourceStackIndex == INDEX_NONE; ++StackIndex)
    {
        SourceUnitIndex = From[StackIndex].ItemGuids.IndexOfByKey(ItemGuid);
        if (SourceUnitIndex != INDEX_NONE) SourceStackIndex = StackIndex;
    }
    if (SourceStackIndex == INDEX_NONE) return false;
    for (const FKalmalaInventoryStack& Stack : To)
        if (Stack.ItemGuids.Contains(ItemGuid)) return false;
    const FName ItemId = From[SourceStackIndex].ItemId;
    FKalmalaInventoryStack& SourceStack = From[SourceStackIndex];
    SourceStack.ItemGuids.RemoveAt(SourceUnitIndex);
    --SourceStack.Quantity;
    if (SourceStack.Quantity == 0) From.RemoveAt(SourceStackIndex);
    if (!AddQuantityToStacks(To, ItemId, 1, {ItemGuid}))
    { Reason = TEXT("Destination capacity reached"); return false; }
    NextSource = MoveTemp(From); NextDestination = MoveTemp(To); Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::BuildTransfer(const TArray<FKalmalaInventoryStack>& Source,
    const TArray<FKalmalaInventoryStack>& Destination, const FName ItemId, const int32 Quantity,
    TArray<FKalmalaInventoryStack>& NextSource, TArray<FKalmalaInventoryStack>& NextDestination, FString& Reason)
{
    Reason = TEXT("Invalid storage transfer");
    if (!IsValidInventoryQuantity(UKalmalaItemCatalogue::Get(), ItemId, Quantity)
        || !IsValidInventoryState(Source) || !IsValidInventoryState(Destination)) return false;
    TArray<FKalmalaInventoryStack> CurrentSource = Source;
    TArray<FKalmalaInventoryStack> CurrentDestination = Destination;
    if (!EnsureInventoryGuids(CurrentSource) || !EnsureInventoryGuids(CurrentDestination)) return false;
    TArray<FGuid> SelectedGuids;
    for (const FKalmalaInventoryStack& Stack : CurrentSource)
    {
        if (Stack.ItemId != ItemId) continue;
        for (const FGuid& Guid : Stack.ItemGuids)
        {
            SelectedGuids.Add(Guid);
            if (SelectedGuids.Num() == Quantity) break;
        }
        if (SelectedGuids.Num() == Quantity) break;
    }
    if (SelectedGuids.Num() != Quantity) return false;
    for (const FGuid& Guid : SelectedGuids)
    {
        TArray<FKalmalaInventoryStack> UpdatedSource, UpdatedDestination;
        if (!BuildTransfer(CurrentSource, CurrentDestination, Guid, UpdatedSource, UpdatedDestination, Reason)) return false;
        CurrentSource = MoveTemp(UpdatedSource);
        CurrentDestination = MoveTemp(UpdatedDestination);
    }
    NextSource = MoveTemp(CurrentSource);
    NextDestination = MoveTemp(CurrentDestination);
    Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::TransferStorageFromServer(const TArray<FKalmalaInventoryStack>& Storage,
    const FGuid ItemGuid, const bool bDeposit,
    TFunctionRef<bool(const TArray<FKalmalaInventoryStack>&)> Persist, FString& Reason)
{
    return TransferStorageFromServer(Storage, ItemGuid, bDeposit,
        UKalmalaStorageSaveGame::MaxStorageSlots, Persist, Reason);
}

bool UKalmalaInventoryComponent::TransferStorageFromServer(const TArray<FKalmalaInventoryStack>& Storage,
    const FGuid ItemGuid, const bool bDeposit, const int32 StorageCapacity,
    TFunctionRef<bool(const TArray<FKalmalaInventoryStack>&)> Persist, FString& Reason)
{
    Reason = TEXT("Server authority required");
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    TArray<FKalmalaInventoryStack> NextPack, NextStorage;
    const bool Built = bDeposit ? BuildTransfer(Stacks, Storage, ItemGuid, NextPack, NextStorage, Reason)
        : BuildTransfer(Storage, Stacks, ItemGuid, NextStorage, NextPack, Reason);
    if (!Built) return false;
    if (!UKalmalaStorageSaveGame::IsValidStacks(NextStorage, StorageCapacity))
    { Reason = TEXT("Storage capacity reached"); return false; }
    const auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!CanFitContents(NextPack, Character ? CountPackedTools(Character->GetCarriedToolInventory()) : 0))
    { Reason = TEXT("Inventory capacity reached"); return false; }
    if (!Persist(NextStorage)) { Reason = TEXT("Storage save failed; nothing transferred"); return false; }
    const auto Before = Stacks;
    Stacks = MoveTemp(NextPack); GetOwner()->ForceNetUpdate();
    SynchronizeGridFromServer();
    RecordAcceptedGains(Before, Stacks);
    Reason = bDeposit ? TEXT("Stored one item") : TEXT("Took one item");
    return true;
}

bool UKalmalaInventoryComponent::TransferStorageFromServer(const TArray<FKalmalaInventoryStack>& Storage,
    const FName ItemId, const bool bDeposit,
    TFunctionRef<bool(const TArray<FKalmalaInventoryStack>&)> Persist, FString& Reason)
{
    Reason = TEXT("Server authority required");
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    TArray<FKalmalaInventoryStack> NextPack, NextStorage;
    const bool Built = bDeposit ? BuildTransfer(Stacks, Storage, ItemId, 1, NextPack, NextStorage, Reason)
        : BuildTransfer(Storage, Stacks, ItemId, 1, NextStorage, NextPack, Reason);
    if (!Built) return false;
    if (!UKalmalaStorageSaveGame::IsValidStacks(NextStorage))
    { Reason = TEXT("Storage capacity reached"); return false; }
    const auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!CanFitContents(NextPack, Character ? CountPackedTools(Character->GetCarriedToolInventory()) : 0))
    { Reason = TEXT("Inventory capacity reached"); return false; }
    if (!Persist(NextStorage)) { Reason = TEXT("Storage save failed; nothing transferred"); return false; }
    const auto Before = Stacks;
    Stacks = MoveTemp(NextPack); GetOwner()->ForceNetUpdate();
    SynchronizeGridFromServer();
    RecordAcceptedGains(Before, Stacks);
    Reason = bDeposit ? TEXT("Stored one item") : TEXT("Took one item");
    return true;
}
