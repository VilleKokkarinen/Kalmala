#include "KalmalaInventoryComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolProgressionContract.h"

bool UKalmalaInventoryComponent::UpgradeEquippedOpalFromServer(const FGuid ItemGuid, FString& Reason)
{
    auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    Reason = TEXT("Equip the exact Opal Helm before upgrading");
    auto* Gear = EquippedGear.FindByPredicate([ItemGuid](const auto& Entry)
        { return Entry.ItemGuid == ItemGuid && Entry.ItemId == TEXT("OpalHelm") && Entry.Slot == EKalmalaGearSlot::Head; });
    if (!Character || !Character->HasAuthority() || Character->GetHealth() <= 1.0f || !ItemGuid.IsValid() || !Gear
        || Gear->Level < 1 || Gear->Level >= FKalmalaToolProgressionContract::MaxStationToolLevel
        || (!SessionGearLevels.Contains(ItemGuid) && SessionGearLevels.Num() >= 256)) return false;
    const auto* Tools = UKalmalaToolCatalogue::Get();
    const auto* Upgrade = Tools ? Tools->UpgradeCosts.FindByPredicate([](const auto& Row) { return Row.ItemId == TEXT("OpalHelm"); }) : nullptr;
    FKalmalaGearLevelStats Current, Next;
    const auto* Items = UKalmalaItemCatalogue::Get();
    TArray<FKalmalaToolRecipeIngredientData> Scaled;
    if (!Upgrade || Upgrade->RequiredStation != TEXT("Forge") || !Items
        || !Items->GetGearLevelStats(Gear->ItemId, Gear->Level, Current)
        || !Items->GetGearLevelStats(Gear->ItemId, Gear->Level + 1, Next)
        || Gear->ConditionSpent < 0 || Gear->ConditionSpent > Current.MaxDurability
        || !UKalmalaToolCatalogue::ScaleUpgradeCosts(Upgrade->Ingredients, Gear->Level, Scaled)) return false;
    TArray<FKalmalaInventoryStack> Costs, After;
    for (const auto& Cost : Scaled) Costs.Add({Cost.ItemId, Cost.Quantity});
    if (!BuildExchange(Stacks, Costs, NAME_None, 0, After, Reason)) return false;
    const auto Before = Stacks;
    if (!TryCommitStacksFromServer(Before, After)) return false;
    ++Gear->Level;
    Gear->ConditionSpent = 0;
    SessionGearWear.Add(ItemGuid, 0);
    SessionGearLevels.Add(ItemGuid, Gear->Level);
    Character->ForceNetUpdate();
    Reason = FString::Printf(TEXT("Opal Helm upgraded to level %d"), Gear->Level);
    return true;
}

bool UKalmalaInventoryComponent::RepairEquippedOpalFromServer(const FGuid ItemGuid, FString& Reason)
{
    auto* Character = Cast<AKalmalaCharacter>(GetOwner());
    auto* Gear = EquippedGear.FindByPredicate([ItemGuid](const auto& Entry)
        { return Entry.ItemGuid == ItemGuid && Entry.ItemId == TEXT("OpalHelm") && Entry.Slot == EKalmalaGearSlot::Head; });
    Reason = TEXT("Equip the exact worn Opal Helm before repairing");
    FKalmalaGearLevelStats Stats;
    if (!Character || !Character->HasAuthority() || Character->GetHealth() <= 1.0f || !ItemGuid.IsValid() || !Gear
        || !UKalmalaItemCatalogue::Get()->GetGearLevelStats(Gear->ItemId, Gear->Level, Stats)
        || Gear->ConditionSpent <= 0 || Gear->ConditionSpent > Stats.MaxDurability) return false;
    Gear->ConditionSpent = 0;
    SessionGearWear.Add(ItemGuid, 0);
    Character->ForceNetUpdate();
    Reason = TEXT("Opal Helm repaired to full condition at no cost");
    return true;
}

bool UKalmalaCraftingComponent::MaintainOpalHelmFromServer(const FGuid ItemGuid, const bool bUpgrade, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Need a visible same-world Forge within 2.5 m at the target upgrade level");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const auto* Forge = FindNearbyToolProgressionStation(TEXT("ForgeKit"));
    if (!Forge) return false;
    auto* Inventory = Character->GetInventoryComponent();
    const auto* Gear = Inventory ? Inventory->FindEquippedGear(EKalmalaGearSlot::Head) : nullptr;
    if (!Gear || Gear->ItemId != TEXT("OpalHelm") || Gear->ItemGuid != ItemGuid
        || (bUpgrade && FKalmalaToolProgressionContract::GetEffectiveStationLevel(Forge) != Gear->Level + 1)) return false;
    return Inventory && (bUpgrade ? Inventory->UpgradeEquippedOpalFromServer(ItemGuid, Reason)
        : Inventory->RepairEquippedOpalFromServer(ItemGuid, Reason));
}

void UKalmalaCraftingComponent::ServerUpgradeOpalHelm_Implementation(const FGuid ItemGuid)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = MaintainOpalHelmFromServer(ItemGuid, true, Reason);
    PublishResult(Reason, bAccepted);
}

void UKalmalaCraftingComponent::ServerRepairOpalHelm_Implementation(const FGuid ItemGuid)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = MaintainOpalHelmFromServer(ItemGuid, false, Reason);
    PublishResult(Reason, bAccepted);
}
