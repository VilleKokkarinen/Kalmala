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
    TSet<FGuid> SeenGuids;
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        if (!Catalogue->IsValidStack(Stack.ItemId, Stack.Quantity)) return false;
        if (bRequireInstanceGuids && !Stack.InstanceGuid.IsValid()) return false;
        if (Stack.InstanceGuid.IsValid())
        {
            if (SeenGuids.Contains(Stack.InstanceGuid)) return false;
            SeenGuids.Add(Stack.InstanceGuid);
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

bool EnsureStackInstanceGuids(TArray<FKalmalaInventoryStack>& Stacks)
{
    TSet<FGuid> Seen;
    for (FKalmalaInventoryStack& Stack : Stacks)
    {
        if (!Stack.InstanceGuid.IsValid())
        {
            do { Stack.InstanceGuid = FGuid::NewGuid(); } while (!Stack.InstanceGuid.IsValid() || Seen.Contains(Stack.InstanceGuid));
        }
        if (Seen.Contains(Stack.InstanceGuid)) return false;
        Seen.Add(Stack.InstanceGuid);
    }
    return true;
}

bool AddQuantityToStacks(TArray<FKalmalaInventoryStack>& Stacks, const FName ItemId, const int32 Quantity,
    const TArray<FGuid>& PreferredGuids = {})
{
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(ItemId) : nullptr;
    if (!IsValidInventoryQuantity(Catalogue, ItemId, Quantity) || !Definition) return false;

    int64 Remaining = Quantity;
    for (FKalmalaInventoryStack& Stack : Stacks)
    {
        if (Stack.ItemId != ItemId || Stack.Quantity >= Definition->MaxStack) continue;
        const int32 Added = int32(FMath::Min<int64>(Remaining, Definition->MaxStack - Stack.Quantity));
        Stack.Quantity += Added;
        Remaining -= Added;
        if (Remaining == 0) return true;
    }

    int32 PreferredIndex = 0;
    while (Remaining > 0)
    {
        if (Stacks.Num() >= UKalmalaInventoryComponent::MaxSlots) return false;
        FKalmalaInventoryStack& Stack = Stacks.AddDefaulted_GetRef();
        Stack.ItemId = ItemId;
        Stack.Quantity = int32(FMath::Min<int64>(Remaining, Definition->MaxStack));
        Remaining -= Stack.Quantity;
        if (PreferredGuids.IsValidIndex(PreferredIndex) && PreferredGuids[PreferredIndex].IsValid())
        {
            Stack.InstanceGuid = PreferredGuids[PreferredIndex++];
            if (Stacks.Num() > 1 && Stacks.ContainsByPredicate([&Stack](const FKalmalaInventoryStack& Other)
                { return &Other != &Stack && Other.InstanceGuid == Stack.InstanceGuid; })) return false;
        }
        else
        {
            do { Stack.InstanceGuid = FGuid::NewGuid(); }
            while (!Stack.InstanceGuid.IsValid() || Stacks.ContainsByPredicate([&Stack](const FKalmalaInventoryStack& Other)
                { return &Other != &Stack && Other.InstanceGuid == Stack.InstanceGuid; }));
        }
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
            || Left[Index].InstanceGuid != Right[Index].InstanceGuid) return false;
    }
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
    if (!EnsureStackInstanceGuids(Candidate)) return false;
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
    if (!EnsureStackInstanceGuids(Candidate)) return false;
    if (!AddQuantityToStacks(Candidate, ItemId, Quantity))
    {
        Reason = TEXT("Pack output capacity reached");
        return false;
    }
    After = MoveTemp(Candidate);
    Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::TryCommitStacksFromServer(
    const TArray<FKalmalaInventoryStack>& ExpectedBefore,
    const TArray<FKalmalaInventoryStack>& CandidateAfter)
{
    AActor* Owner = GetOwner();
    TArray<FKalmalaInventoryStack> Candidate = CandidateAfter;
    if (!EnsureStackInstanceGuids(Candidate) || !Owner || !Owner->HasAuthority()
        || !AreInventoryStatesEqual(Stacks, ExpectedBefore) || !IsValidInventoryState(Candidate, true)) return false;
    const auto* Character = Cast<AKalmalaCharacter>(Owner);
    if (!CanFitContents(Candidate, Character ? Character->GetCarriedToolInventory().Num() : 0)) return false;

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
    const TArray<FKalmalaInventoryStack>& Destination, FName ItemId, int32 Quantity,
    TArray<FKalmalaInventoryStack>& NextSource, TArray<FKalmalaInventoryStack>& NextDestination, FString& Reason)
{
    Reason = TEXT("Invalid storage transfer");
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
    if (&NextSource == &NextDestination || !IsValidInventoryQuantity(Catalogue, ItemId, Quantity)
        || !IsValidInventoryState(Source) || !IsValidInventoryState(Destination)) return false;
    TArray<FKalmalaInventoryStack> SourceWithGuids = Source;
    if (!EnsureStackInstanceGuids(SourceWithGuids)) return false;
    TArray<FKalmalaInventoryStack> From;
    if (!BuildExchange(SourceWithGuids, {{ItemId, Quantity}}, NAME_None, 0, From, Reason)) return false;
    auto To = Destination;
    if (!EnsureStackInstanceGuids(To)) return false;
    TArray<FGuid> PreservedGuids;
    const FKalmalaItemDefinition* Definition = Catalogue->FindItem(ItemId);
    if (Definition && Definition->MaxStack == 1)
    {
        int32 Remaining = Quantity;
        for (const FKalmalaInventoryStack& Stack : SourceWithGuids)
        {
            if (Stack.ItemId != ItemId) continue;
            const int32 Moved = FMath::Min(Remaining, Stack.Quantity);
            for (int32 Unit = 0; Unit < Moved; ++Unit) PreservedGuids.Add(Stack.InstanceGuid);
            Remaining -= Moved;
            if (Remaining == 0) break;
        }
    }
    if (!AddQuantityToStacks(To, ItemId, Quantity, PreservedGuids))
    { Reason = TEXT("Destination capacity reached"); return false; }
    NextSource = MoveTemp(From); NextDestination = MoveTemp(To); Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::TransferStorageFromServer(const TArray<FKalmalaInventoryStack>& Storage,
    FName ItemId, bool bDeposit, TFunctionRef<bool(const TArray<FKalmalaInventoryStack>&)> Persist, FString& Reason)
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
    if (!CanFitContents(NextPack, Character ? Character->GetCarriedToolInventory().Num() : 0))
    { Reason = TEXT("Inventory capacity reached"); return false; }
    if (!Persist(NextStorage)) { Reason = TEXT("Storage save failed; nothing transferred"); return false; }
    const auto Before = Stacks;
    Stacks = MoveTemp(NextPack); GetOwner()->ForceNetUpdate();
    SynchronizeGridFromServer();
    RecordAcceptedGains(Before, Stacks);
    Reason = bDeposit ? TEXT("Stored one item") : TEXT("Took one item");
    return true;
}
