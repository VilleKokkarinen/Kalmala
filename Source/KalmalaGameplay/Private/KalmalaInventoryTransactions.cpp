#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
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
