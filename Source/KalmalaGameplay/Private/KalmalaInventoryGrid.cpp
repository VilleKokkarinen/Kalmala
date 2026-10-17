#include "KalmalaInventoryComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaToolLifecycleContract.h"
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

FGuid UKalmalaInventoryComponent::GetMainHandGuid() const
{
    return FKalmalaToolLifecycleContract::FindDefinition(ActiveItem) ? ActiveInstanceGuid : FGuid();
}

int32 UKalmalaInventoryComponent::CountPackedTools(const TArray<FKalmalaToolState>& Tools) const
{
    return Tools.Num();
}

bool UKalmalaInventoryComponent::BuildMainHandGrid(const TArray<FGuid>& ToolGuids,
    const TArray<FGuid>& StackGuids, const FGuid HandGuid, const TArray<FGuid>& Before, TArray<FGuid>& After)
{
    if (HandGuid.IsValid() && !ToolGuids.Contains(HandGuid)) return false;
    TSet<FGuid> Seen;
    TArray<FGuid> Packed;
    for (const FGuid& Guid : ToolGuids)
    {
        if (!Guid.IsValid() || Seen.Contains(Guid)) return false;
        Seen.Add(Guid);
        Packed.Add(Guid);
    }
    for (const FGuid& Guid : StackGuids)
    {
        if (!Guid.IsValid() || Seen.Contains(Guid)) return false;
        Seen.Add(Guid);
        Packed.Add(Guid);
    }
    return BuildGridLayout(Packed, Before, After);
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

    TMap<FGuid, FName> ItemByGuid;
    TSet<FGuid> Seen;
    auto AddInstance = [&ItemByGuid, &Seen](FGuid& Guid, const FName ItemId)
    {
        if (ItemId.IsNone()) return;
        if (!Guid.IsValid() || Seen.Contains(Guid))
        {
            do { Guid = FGuid::NewGuid(); } while (!Guid.IsValid() || Seen.Contains(Guid));
        }
        Seen.Add(Guid);
        ItemByGuid.Add(Guid, ItemId);
    };
    if (Character)
        for (FKalmalaToolState& Tool : Character->CarriedTools) AddInstance(Tool.InstanceGuid, Tool.ItemId);
    for (FKalmalaInventoryStack& Stack : Stacks) AddInstance(Stack.InstanceGuid, Stack.ItemId);

    TArray<FGuid> ToolGuids, StackGuids;
    if (Character)
        for (const FKalmalaToolState& Tool : Character->CarriedTools) ToolGuids.Add(Tool.InstanceGuid);
    for (const FKalmalaInventoryStack& Stack : Stacks) StackGuids.Add(Stack.InstanceGuid);
    const FGuid Hand = GetMainHandGuid();
    TArray<FGuid> CandidateGuids;
    if (!BuildMainHandGrid(ToolGuids, StackGuids,
        ToolGuids.Contains(Hand) ? Hand : FGuid(), GridInstanceGuids, CandidateGuids)) return;
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

bool UKalmalaInventoryComponent::SetMainHandFromServer(const FGuid ItemGuid, FString& Reason)
{
    auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    Reason = TEXT("Server authority and a living owner required");
    if (!Character || !Character->HasAuthority() || !FMath::IsFinite(Character->GetHealth())
        || Character->GetHealth() <= 1.0f) return false;

    FName SelectedId = NAME_None;
    if (ItemGuid.IsValid())
    {
        const FKalmalaToolState* Tool = nullptr;
        for (const FKalmalaToolState& Candidate : Character->GetCarriedToolInventory())
        {
            if (Candidate.InstanceGuid != ItemGuid) continue;
            if (Tool) { Reason = TEXT("Ambiguous carried item identity"); return false; }
            Tool = &Candidate;
        }
        const int32 Maximum = Tool
            ? FKalmalaToolLifecycleContract::GetMaxDurability(Tool->ItemId, Tool->ToolLevel) : 0;
        if (!Tool || Maximum <= 0 || Tool->Durability < 0 || Tool->Durability > Maximum
            || !GridInstanceGuids.Contains(ItemGuid)
            || Stacks.ContainsByPredicate([ItemGuid](const FKalmalaInventoryStack& Stack)
                { return Stack.InstanceGuid == ItemGuid || Stack.ItemGuids.Contains(ItemGuid); })
            || EquippedGear.ContainsByPredicate([ItemGuid](const FKalmalaEquippedGear& Gear)
                { return Gear.ItemGuid == ItemGuid; }))
        {
            Reason = TEXT("Select a valid carried weapon or tool");
            return false;
        }
        SelectedId = Tool->ItemId;
    }
    else if (!FKalmalaToolLifecycleContract::FindDefinition(ActiveItem))
    {
        Reason = TEXT("Main Hand is empty");
        return false;
    }

    TArray<FGuid> ToolGuids, StackGuids;
    TSet<FGuid> PhysicalGuids;
    for (const FKalmalaToolState& Tool : Character->GetCarriedToolInventory())
    {
        if (!Tool.InstanceGuid.IsValid() || PhysicalGuids.Contains(Tool.InstanceGuid))
        { Reason = TEXT("Invalid carried tool identity"); return false; }
        PhysicalGuids.Add(Tool.InstanceGuid);
        ToolGuids.Add(Tool.InstanceGuid);
    }
    for (const FKalmalaInventoryStack& Stack : Stacks)
    {
        StackGuids.Add(Stack.InstanceGuid);
        for (const FGuid& Guid : Stack.ItemGuids)
            if (PhysicalGuids.Contains(Guid))
            { Reason = TEXT("Tool and inventory identities overlap"); return false; }
    }
    for (const FKalmalaEquippedGear& Gear : EquippedGear)
        if (PhysicalGuids.Contains(Gear.ItemGuid))
        { Reason = TEXT("Tool and wearable identities overlap"); return false; }

    TArray<FKalmalaInventoryStack> CandidateStacks = Stacks;
    TArray<FKalmalaEquippedGear> CandidateGear = EquippedGear;
    if (FKalmalaToolLifecycleContract::IsTwoHanded(SelectedId)
        && FindEquippedGear(EKalmalaGearSlot::Shield))
    {
        // Tools retain their cells; returning the shield requires one free cell.
        // Validate the complete transaction before publishing either state.
        if (!BuildUnequipGearTransaction(Stacks, EquippedGear, EKalmalaGearSlot::Shield,
            ToolGuids, CandidateStacks, CandidateGear, Reason, ItemGuid))
        { Reason = TEXT("Cannot return the held items; free an inventory cell before wielding a two-handed weapon"); return false; }
        StackGuids.Reset();
        for (const FKalmalaInventoryStack& Stack : CandidateStacks) StackGuids.Add(Stack.InstanceGuid);
    }
    TArray<FGuid> CandidateGrid;
    if (!BuildMainHandGrid(ToolGuids, StackGuids, ItemGuid, GridInstanceGuids, CandidateGrid))
    { Reason = TEXT("No inventory space to return Main Hand"); return false; }

    // Keep the physical tool record for condition, combat and current development saves.
    // Equipping changes held state only; all tools retain their grid cells.
    ActiveItem = SelectedId;
    ActiveInstanceGuid = ItemGuid;
    Stacks = MoveTemp(CandidateStacks);
    EquippedGear = MoveTemp(CandidateGear);
    GridInstanceGuids = MoveTemp(CandidateGrid);
    GridSlots.Init(NAME_None, MaxSlots);
    for (int32 Slot = 0; Slot < MaxSlots; ++Slot) GridSlots[Slot] = GetSlotItem(Slot);
    Character->ForceNetUpdate();
    Reason = ItemGuid.IsValid() ? TEXT("Main Hand selected") : TEXT("Main Hand cleared");
    return true;
}

void UKalmalaInventoryComponent::ServerSetMainHand_Implementation(const FGuid ItemGuid)
{
    if (!GetWorld()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastGearRequestTime >= 0.0 && Now - LastGearRequestTime < 0.1) return;
    LastGearRequestTime = Now;
    FString Reason;
    const bool bAccepted = SetMainHandFromServer(ItemGuid, Reason);
    ClientGearRequestResult(bAccepted, Reason);
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
    if (FKalmalaToolLifecycleContract::FindDefinition(Id))
    {
        FString Reason;
        SetMainHandFromServer(GetMainHandGuid() == Guid ? FGuid() : Guid, Reason);
        return;
    }
    // Consuming food does not displace a tool into a potentially full pack.
    if (UKalmalaPlayerStatusComponent::IsKnownFoodItem(Id))
    {
        if (auto* Crafting = GetOwner()->FindComponentByClass<UKalmalaCraftingComponent>()) Crafting->ServerConsumeFood(Id);
        return;
    }
    if (GetMainHandGuid().IsValid())
    {
        FString Reason;
        if (!SetMainHandFromServer(FGuid(), Reason)) return;
    }
    ActiveItem = Id;
    ActiveInstanceGuid = Guid;
    GetOwner()->ForceNetUpdate();
}
