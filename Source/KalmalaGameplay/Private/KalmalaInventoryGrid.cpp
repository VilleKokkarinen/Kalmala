#include "KalmalaInventoryComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Engine/World.h"

FName UKalmalaInventoryComponent::GetSlotItem(const int32 Slot) const
{
    const FGuid Guid = GetSlotInstanceGuid(Slot);
    if (!Guid.IsValid()) return NAME_None;
    if (const FKalmalaInventoryStack* Stack = Stacks.FindByPredicate(
        [Guid](const FKalmalaInventoryStack& Candidate) { return Candidate.InstanceGuid == Guid; })) return Stack->ItemId;
    if (const auto* Character = Cast<AKalmalaCharacter>(GetOwner()))
        if (const FKalmalaToolState* Tool = Character->GetCarriedToolInventory().FindByPredicate(
            [Guid](const FKalmalaToolState& Candidate) { return Candidate.InstanceGuid == Guid; })) return Tool->ItemId;
    return NAME_None;
}

FGuid UKalmalaInventoryComponent::GetSlotInstanceGuid(const int32 Slot) const
{
    const FGuid Guid = GridInstanceGuids.IsValidIndex(Slot) ? GridInstanceGuids[Slot] : FGuid();
    if (!Guid.IsValid()) return FGuid();
    if (Stacks.ContainsByPredicate([Guid](const FKalmalaInventoryStack& Stack) { return Stack.InstanceGuid == Guid; })) return Guid;
    const auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    return Character && Character->GetCarriedToolInventory().ContainsByPredicate(
        [Guid](const FKalmalaToolState& Tool) { return Tool.InstanceGuid == Guid; }) ? Guid : FGuid();
}

int32 UKalmalaInventoryComponent::GetSlotQuantity(const int32 Slot) const
{
    const FGuid Guid = GetSlotInstanceGuid(Slot);
    if (!Guid.IsValid()) return 0;
    if (const FKalmalaInventoryStack* Stack = Stacks.FindByPredicate(
        [Guid](const FKalmalaInventoryStack& Candidate) { return Candidate.InstanceGuid == Guid; })) return Stack->Quantity;
    return 1;
}

bool UKalmalaInventoryComponent::OwnsGridItem(const FName Id) const
{
    if (Id.IsNone()) return false;
    if (GetQuantity(Id) > 0) return true;
    const auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    return Character && Character->GetCarriedToolInventory().ContainsByPredicate(
        [Id](const FKalmalaToolState& Tool) { return Tool.ItemId == Id; });
}

bool UKalmalaInventoryComponent::CanFitContents(const TArray<FKalmalaInventoryStack>& Items, const int32 OtherItemCount) const
{
    return OtherItemCount >= 0 && OtherItemCount <= MaxSlots && Items.Num() <= MaxSlots - OtherItemCount;
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

bool UKalmalaInventoryComponent::BuildGridLayout(const TArray<FGuid>& Owned,
    const TArray<FGuid>& Before, TArray<FGuid>& After)
{
    if (Owned.Num() > MaxSlots) return false;
    TSet<FGuid> Remaining;
    for (const FGuid& Guid : Owned)
    {
        if (!Guid.IsValid() || Remaining.Contains(Guid)) return false;
        Remaining.Add(Guid);
    }
    TArray<FGuid> Candidate;
    Candidate.Init(FGuid(), MaxSlots);
    for (int32 Slot = 0; Slot < FMath::Min(Before.Num(), MaxSlots); ++Slot)
        if (Remaining.Remove(Before[Slot])) Candidate[Slot] = Before[Slot];
    for (const FGuid& Guid : Owned)
        if (Remaining.Contains(Guid))
        {
            const int32 Empty = Candidate.IndexOfByKey(FGuid());
            if (Empty == INDEX_NONE) return false;
            Candidate[Empty] = Guid;
            Remaining.Remove(Guid);
        }
    After = MoveTemp(Candidate);
    return true;
}

void UKalmalaInventoryComponent::SynchronizeGridFromServer()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (Character) Character->EnsureToolInstanceGuidsFromServer();

    TArray<FGuid> Owned;
    TMap<FGuid, FName> ItemByGuid;
    TSet<FGuid> Seen;
    auto AddInstance = [&Owned, &ItemByGuid, &Seen](FGuid& Guid, const FName ItemId)
    {
        if (ItemId.IsNone()) return;
        if (!Guid.IsValid() || Seen.Contains(Guid))
        {
            do { Guid = FGuid::NewGuid(); } while (!Guid.IsValid() || Seen.Contains(Guid));
        }
        Seen.Add(Guid);
        Owned.Add(Guid);
        ItemByGuid.Add(Guid, ItemId);
    };
    if (Character)
        for (FKalmalaToolState& Tool : Character->CarriedTools) AddInstance(Tool.InstanceGuid, Tool.ItemId);
    for (FKalmalaInventoryStack& Stack : Stacks) AddInstance(Stack.InstanceGuid, Stack.ItemId);

    TArray<FGuid> CandidateGuids;
    if (!BuildGridLayout(Owned, GridInstanceGuids, CandidateGuids)) return;
    TArray<FName> CandidateItems;
    CandidateItems.Init(NAME_None, MaxSlots);
    for (int32 Slot = 0; Slot < CandidateGuids.Num(); ++Slot)
        if (CandidateGuids[Slot].IsValid()) CandidateItems[Slot] = ItemByGuid.FindRef(CandidateGuids[Slot]);

    bool bChanged = CandidateGuids != GridInstanceGuids || CandidateItems != GridSlots;
    if (bChanged)
    {
        GridInstanceGuids = MoveTemp(CandidateGuids);
        GridSlots = MoveTemp(CandidateItems);
    }
    if (!ActiveInstanceGuid.IsValid() || !ItemByGuid.Contains(ActiveInstanceGuid))
    {
        if (!ActiveItem.IsNone() || ActiveInstanceGuid.IsValid()) bChanged = true;
        ActiveItem = NAME_None;
        ActiveInstanceGuid.Invalidate();
    }
    else
    {
        const FName CurrentActiveId = ItemByGuid.FindRef(ActiveInstanceGuid);
        if (ActiveItem != CurrentActiveId) { ActiveItem = CurrentActiveId; bChanged = true; }
    }
    if (bChanged) GetOwner()->ForceNetUpdate();
}

void UKalmalaInventoryComponent::ReplaceGridItemFromServer(const FName Previous, const FName Replacement)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Previous.IsNone() || Replacement.IsNone()) return;
    const int32 Slot = GridSlots.IndexOfByKey(Previous);
    if (Slot != INDEX_NONE)
    {
        GridSlots[Slot] = Replacement;
        if (ActiveInstanceGuid.IsValid() && GridInstanceGuids.IsValidIndex(Slot)
            && GridInstanceGuids[Slot] == ActiveInstanceGuid) ActiveItem = Replacement;
    }
    SynchronizeGridFromServer();
}

bool UKalmalaInventoryComponent::MoveSlotByInstanceFromServer(const int32 Source, const int32 Target,
    const FGuid ExpectedSource, const FGuid ExpectedTarget)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || Source < 0 || Source >= MaxSlots
        || Target < 0 || Target >= MaxSlots || Source == Target || !ExpectedSource.IsValid()) return false;
    SynchronizeGridFromServer();
    if (GridInstanceGuids.Num() != MaxSlots || GetSlotInstanceGuid(Source) != ExpectedSource
        || GetSlotInstanceGuid(Target) != ExpectedTarget) return false;
    GridInstanceGuids.Swap(Source, Target);
    GridSlots.Swap(Source, Target);
    GetOwner()->ForceNetUpdate();
    return true;
}

bool UKalmalaInventoryComponent::MoveSlotFromServer(const int32 Source, const int32 Target,
    const FName ExpectedSource, const FName ExpectedTarget)
{
    if (GetSlotItem(Source) != ExpectedSource || GetSlotItem(Target) != ExpectedTarget) return false;
    return MoveSlotByInstanceFromServer(Source, Target, GetSlotInstanceGuid(Source), GetSlotInstanceGuid(Target));
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

void UKalmalaInventoryComponent::ServerMoveSlotByInstance_Implementation(const int32 Source, const int32 Target,
    const FGuid ExpectedSource, const FGuid ExpectedTarget)
{
    if (!GetWorld()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastLayoutRequestTime >= 0.0 && Now - LastLayoutRequestTime < 0.05) return;
    LastLayoutRequestTime = Now;
    MoveSlotByInstanceFromServer(Source, Target, ExpectedSource, ExpectedTarget);
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
    const FGuid Guid = GetSlotInstanceGuid(Slot);
    if (Id.IsNone() || !Guid.IsValid()) return;
    const EKalmalaSupportEffect SupportEffect = UKalmalaSupportMagicComponent::FromScrollItemId(Id);
    if (SupportEffect != EKalmalaSupportEffect::None)
    {
        if (UKalmalaSupportMagicComponent* Support = Character->GetSupportMagicComponent())
            Support->ActivateSupportEffectFromHotbarServer(SupportEffect);
        return;
    }
    ActiveItem = Id;
    ActiveInstanceGuid = Guid;
    if (UKalmalaPlayerStatusComponent::IsKnownFoodItem(Id))
        if (auto* Crafting = GetOwner()->FindComponentByClass<UKalmalaCraftingComponent>()) Crafting->ServerConsumeFood(Id);
    GetOwner()->ForceNetUpdate();
}
