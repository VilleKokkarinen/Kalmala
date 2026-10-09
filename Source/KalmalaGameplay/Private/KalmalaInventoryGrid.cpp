#include "KalmalaInventoryComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Engine/World.h"

FName UKalmalaInventoryComponent::GetSlotItem(const int32 Slot) const
{
    const FName Id = GridSlots.IsValidIndex(Slot) ? GridSlots[Slot] : NAME_None;
    return OwnsGridItem(Id) ? Id : NAME_None;
}

bool UKalmalaInventoryComponent::OwnsGridItem(const FName Id) const
{
    if (Id.IsNone()) return false;
    if (GetQuantity(Id) > 0) return true;
    const auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    return Character && Character->GetCarriedToolInventory().ContainsByPredicate(
        [Id](const FKalmalaToolState& Tool) { return Tool.ToolId == Id; });
}

bool UKalmalaInventoryComponent::CanFitContents(const TArray<FKalmalaInventoryStack>& Items, const int32 ToolCount) const
{
    return ToolCount >= 0 && ToolCount <= MaxSlots && Items.Num() <= MaxSlots - ToolCount;
}

float UKalmalaInventoryComponent::GetCarriedWeight() const
{
    float Weight = 0.0f;
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
    for (const auto& Stack : Stacks)
        if (const auto* Item = Catalogue->FindItem(Stack.ItemId)) Weight += Item->WeightKg * Stack.Quantity;
    if (const auto* Character = Cast<AKalmalaCharacter>(GetOwner()))
        Weight += Character->GetCarriedToolInventory().Num() * 2.0f;
    return Weight;
}

bool UKalmalaInventoryComponent::BuildGridLayout(const TArray<FName>& Owned,
    const TArray<FName>& Before, TArray<FName>& After)
{
    if (Owned.Num() > MaxSlots) return false;
    TSet<FName> Remaining;
    for (const FName Id : Owned)
    {
        if (Id.IsNone() || Remaining.Contains(Id)) return false;
        Remaining.Add(Id);
    }
    TArray<FName> Candidate;
    Candidate.Init(NAME_None, MaxSlots);
    for (int32 Slot = 0; Slot < FMath::Min(Before.Num(), MaxSlots); ++Slot)
        if (Remaining.Remove(Before[Slot])) Candidate[Slot] = Before[Slot];
    for (const FName Id : Owned)
        if (Remaining.Contains(Id))
        {
            const int32 Empty = Candidate.IndexOfByKey(NAME_None);
            if (Empty == INDEX_NONE) return false;
            Candidate[Empty] = Id;
            Remaining.Remove(Id);
        }
    After = MoveTemp(Candidate);
    return true;
}

void UKalmalaInventoryComponent::SynchronizeGridFromServer()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    TArray<FName> Owned;
    if (const auto* Character = Cast<AKalmalaCharacter>(GetOwner()))
        for (const auto& Tool : Character->GetCarriedToolInventory()) Owned.Add(Tool.ToolId);
    for (const auto& Stack : Stacks) Owned.Add(Stack.ItemId);
    TArray<FName> Candidate;
    if (!BuildGridLayout(Owned, GridSlots, Candidate)) return;
    bool bChanged = Candidate != GridSlots;
    if (bChanged) GridSlots = MoveTemp(Candidate);
    if (!ActiveItem.IsNone() && !Owned.Contains(ActiveItem)) { ActiveItem = NAME_None; bChanged = true; }
    if (bChanged) GetOwner()->ForceNetUpdate();
}

void UKalmalaInventoryComponent::ReplaceGridItemFromServer(const FName Previous, const FName Replacement)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Previous.IsNone() || !OwnsGridItem(Replacement)) return;
    const int32 Slot = GridSlots.IndexOfByKey(Previous);
    if (Slot != INDEX_NONE && !GridSlots.Contains(Replacement))
    {
        GridSlots[Slot] = Replacement;
        if (ActiveItem == Previous) ActiveItem = Replacement;
    }
    SynchronizeGridFromServer();
}

bool UKalmalaInventoryComponent::MoveSlotFromServer(const int32 Source, const int32 Target,
    const FName ExpectedSource, const FName ExpectedTarget)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Source < 0 || Source >= MaxSlots
        || Target < 0 || Target >= MaxSlots || Source == Target || ExpectedSource.IsNone()) return false;
    SynchronizeGridFromServer();
    if (GridSlots.Num() != MaxSlots || GetSlotItem(Source) != ExpectedSource || GetSlotItem(Target) != ExpectedTarget) return false;
    GridSlots.Swap(Source, Target);
    GetOwner()->ForceNetUpdate();
    return true;
}

void UKalmalaInventoryComponent::ServerMoveSlot_Implementation(const int32 Source, const int32 Target,
    const FName ExpectedSource, const FName ExpectedTarget)
{
    if (!GetWorld()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastLayoutRequestTime >= 0.0 && Now - LastLayoutRequestTime < 0.05) return;
    LastLayoutRequestTime = Now;
    MoveSlotFromServer(Source, Target, ExpectedSource, ExpectedTarget);
}

void UKalmalaInventoryComponent::ServerUseHotbarSlot_Implementation(const int32 Slot)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || Slot < 0 || Slot >= Columns) return;
    const auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!Character || Character->GetHealth() <= 1.0f) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastHotbarRequestTime >= 0.0 && Now - LastHotbarRequestTime < 0.1) return;
    LastHotbarRequestTime = Now;
    SynchronizeGridFromServer();
    const FName Id = GetSlotItem(Slot);
    if (Id.IsNone()) return;
    ActiveItem = Id;
    if (UKalmalaPlayerStatusComponent::IsKnownFoodItem(Id))
        if (auto* Crafting = GetOwner()->FindComponentByClass<UKalmalaCraftingComponent>()) Crafting->ServerConsumeFood(Id);
    GetOwner()->ForceNetUpdate();
}
