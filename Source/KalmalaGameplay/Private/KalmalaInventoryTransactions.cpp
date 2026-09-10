#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaStorageSaveGame.h"
#include "GameFramework/Actor.h"

bool UKalmalaInventoryComponent::BuildExchange(const TArray<FKalmalaInventoryStack>& Before,
    const TArray<FKalmalaInventoryStack>& Costs, FName Output, int32 OutputCount,
    TArray<FKalmalaInventoryStack>& After, FString& Reason)
{
    const auto* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    Reason = TEXT("Invalid transaction");
    if (Before.Num() > MaxSlots || Costs.IsEmpty() || Costs.Num() > MaxSlots
        || (Output.IsNone() ? OutputCount != 0 : !Catalogue->IsValidStack(Output, OutputCount))) return false;
    TSet<FName> Seen;
    for (const auto& Stack : Before)
    {
        if (Seen.Contains(Stack.ItemId) || !Catalogue->IsValidStack(Stack.ItemId, Stack.Quantity)) return false;
        Seen.Add(Stack.ItemId);
    }
    auto Candidate = Before;
    Seen.Reset();
    for (const auto& Cost : Costs)
    {
        if (Seen.Contains(Cost.ItemId) || !Catalogue->IsValidStack(Cost.ItemId, Cost.Quantity)) return false;
        Seen.Add(Cost.ItemId);
        const int32 Index = Candidate.IndexOfByPredicate([&](const auto& S) { return S.ItemId == Cost.ItemId; });
        if (Index == INDEX_NONE || Candidate[Index].Quantity < Cost.Quantity)
        {
            const auto* Item = Catalogue->FindItem(Cost.ItemId);
            Reason = FString::Printf(TEXT("Need %d %s"), Cost.Quantity, *Item->DisplayName);
            return false;
        }
        Candidate[Index].Quantity -= Cost.Quantity;
        if (!Candidate[Index].Quantity) Candidate.RemoveAt(Index);
    }
    if (!Output.IsNone())
    {
        int32 Index = Candidate.IndexOfByPredicate([&](const auto& S) { return S.ItemId == Output; });
        if (!Catalogue->CanAddToStack(Output, Index == INDEX_NONE ? 0 : Candidate[Index].Quantity, OutputCount)
            || (Index == INDEX_NONE && Candidate.Num() >= MaxSlots))
        { Reason = TEXT("Pack output capacity reached"); return false; }
        if (Index == INDEX_NONE) { Index = Candidate.AddDefaulted(); Candidate[Index].ItemId = Output; }
        Candidate[Index].Quantity += OutputCount;
    }
    After = MoveTemp(Candidate);
    Reason = TEXT("Ready");
    return true;
}

bool UKalmalaInventoryComponent::TryExchangeFromServer(const TArray<FKalmalaInventoryStack>& Costs,
    FName Output, int32 OutputCount, FString& Reason)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) { Reason = TEXT("Server authority required"); return false; }
    TArray<FKalmalaInventoryStack> Next;
    if (!BuildExchange(Stacks, Costs, Output, OutputCount, Next, Reason)) return false;
    Stacks = MoveTemp(Next);
    GetOwner()->ForceNetUpdate();
    return true;
}

bool UKalmalaInventoryComponent::BuildTransfer(const TArray<FKalmalaInventoryStack>& Source,
    const TArray<FKalmalaInventoryStack>& Destination, FName ItemId, int32 Quantity,
    TArray<FKalmalaInventoryStack>& NextSource, TArray<FKalmalaInventoryStack>& NextDestination, FString& Reason)
{
    Reason = TEXT("Invalid storage transfer");
    const auto* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    if (&NextSource == &NextDestination || !Catalogue->IsValidStack(ItemId, Quantity)
        || !UKalmalaStorageSaveGame::IsValidStacks(Source) || !UKalmalaStorageSaveGame::IsValidStacks(Destination)) return false;
    TArray<FKalmalaInventoryStack> From;
    if (!BuildExchange(Source, {{ItemId, Quantity}}, NAME_None, 0, From, Reason)) return false;
    auto To = Destination;
    int32 Index = To.IndexOfByPredicate([&](const auto& Stack) { return Stack.ItemId == ItemId; });
    if (!Catalogue->CanAddToStack(ItemId, Index == INDEX_NONE ? 0 : To[Index].Quantity, Quantity)
        || (Index == INDEX_NONE && To.Num() >= MaxSlots))
    { Reason = TEXT("Destination capacity reached"); return false; }
    if (Index == INDEX_NONE) { Index = To.AddDefaulted(); To[Index].ItemId = ItemId; }
    To[Index].Quantity += Quantity;
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
    if (!Persist(NextStorage)) { Reason = TEXT("Storage save failed; nothing transferred"); return false; }
    Stacks = MoveTemp(NextPack); GetOwner()->ForceNetUpdate();
    Reason = bDeposit ? TEXT("Stored one item") : TEXT("Took one item");
    return true;
}
