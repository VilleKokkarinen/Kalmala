#include "KalmalaItemCatalogue.h"
#include "KalmalaGameCatalogueLoader.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaPlacementPreview.h"

namespace
{
    bool IsWearableGearSlot(const FString& Slot)
    {
        return Slot == TEXT("Head") || Slot == TEXT("Cape") || Slot == TEXT("Chest")
            || Slot == TEXT("Shield") || Slot == TEXT("Legs") || Slot == TEXT("Boots");
    }

    bool IsAccessorySlot(const FString& Slot)
    {
        return Slot == TEXT("Necklace") || Slot == TEXT("Ring");
    }

    bool IsGearSlot(const FString& Slot)
    {
        return IsWearableGearSlot(Slot) || IsAccessorySlot(Slot);
    }
}

const UKalmalaItemCatalogue* UKalmalaItemCatalogue::Get()
{
    FKalmalaGameCatalogueLoader::EnsureLoaded();
    return GetDefault<UKalmalaItemCatalogue>();
}

bool UKalmalaItemCatalogue::IsValidCatalogue() const
{
    if (Items.IsEmpty())
    {
        return false;
    }
    TSet<FName> Seen;
    const auto Validate = [&Seen](const FKalmalaItemDefinition& Item, const bool bBuildable)
    {
        const bool bIsGear = !Item.GearSlot.IsEmpty();
        const bool bIsAccessory = IsAccessorySlot(Item.GearSlot);
        const bool bHasAccessoryEffect = Item.Armor > 0 || Item.MaxHealthBonus > 0
            || Item.MaxStaminaBonus > 0
            || !FMath::IsNearlyZero(Item.StaminaCostMultiplier)
            || !FMath::IsNearlyZero(Item.SupportSpellStaminaCostMultiplier)
            || !FMath::IsNearlyZero(Item.StaminaRegenerationMultiplier)
            || !FMath::IsNearlyZero(Item.HealingReceivedMultiplier);
        const bool bValidAccessoryMetadata = bIsAccessory
            ? (!bBuildable && Item.MaxStack == 1 && Item.MaxDurability == 0
                && bHasAccessoryEffect)
            : (Item.MaxHealthBonus == 0 && Item.MaxStaminaBonus == 0
                && FMath::IsNearlyZero(Item.StaminaCostMultiplier)
                && FMath::IsNearlyZero(Item.SupportSpellStaminaCostMultiplier)
                && FMath::IsNearlyZero(Item.StaminaRegenerationMultiplier)
                && FMath::IsNearlyZero(Item.HealingReceivedMultiplier));
        if (Item.ItemId.IsNone() || Seen.Contains(Item.ItemId)
            || (bBuildable ? (!FKalmalaPlacementPreview::IsSupportedKit(Item.ItemId) || bIsGear)
                : (FKalmalaPlacementPreview::IsSupportedKit(Item.ItemId) || Item.ItemId.ToString().EndsWith(TEXT("Kit"))))
            || Item.DisplayName.TrimStartAndEnd().IsEmpty() || Item.DisplayName.Len() > 64
            || Item.Description.TrimStartAndEnd().IsEmpty() || Item.Description.Len() > 180
            || Item.MaxStack < 1 || Item.MaxStack > AbsoluteMaxStack
            || !FMath::IsFinite(Item.WeightKg) || Item.WeightKg < 0.0f || Item.WeightKg > 300.0f
            || Item.Armor < 0 || Item.Armor > 1000
            || Item.MaxHealthBonus < 0 || Item.MaxHealthBonus > 1000
            || Item.MaxStaminaBonus < 0 || Item.MaxStaminaBonus > 1000
            || !FMath::IsFinite(Item.StaminaCostMultiplier)
            || Item.StaminaCostMultiplier < -0.5f || Item.StaminaCostMultiplier > 1.0f
            || !FMath::IsFinite(Item.SupportSpellStaminaCostMultiplier)
            || Item.SupportSpellStaminaCostMultiplier < -0.5f || Item.SupportSpellStaminaCostMultiplier > 1.0f
            || !FMath::IsFinite(Item.StaminaRegenerationMultiplier)
            || Item.StaminaRegenerationMultiplier < -0.5f || Item.StaminaRegenerationMultiplier > 1.0f
            || !FMath::IsFinite(Item.HealingReceivedMultiplier)
            || Item.HealingReceivedMultiplier < -0.5f || Item.HealingReceivedMultiplier > 1.0f
            || !bValidAccessoryMetadata
            || (bIsGear
                ? (!IsGearSlot(Item.GearSlot) || Item.MaxStack != 1
                    || (IsWearableGearSlot(Item.GearSlot)
                        ? (Item.MaxDurability < 1 || Item.MaxDurability > 1000)
                        : Item.MaxDurability != 0))
                : (Item.MaxDurability != 0 || Item.Armor != 0)))
        {
            return false;
        }
        Seen.Add(Item.ItemId);
        return true;
    };
    for (const FKalmalaItemDefinition& Item : Items) if (!Validate(Item, false)) return false;
    for (const FKalmalaItemDefinition& Item : Buildables) if (!Validate(Item, true)) return false;
    return true;
}

bool UKalmalaItemCatalogue::GetGearLevelStats(
    const FName ItemId, const int32 GearLevel, FKalmalaGearLevelStats& OutStats) const
{
    OutStats = {};
    if (GearLevel < 1 || GearLevel > FKalmalaToolLifecycleContract::MaxToolLevel) return false;

    const FKalmalaItemDefinition* Item = FindItem(ItemId);
    if (!Item || !IsGearSlot(Item->GearSlot)) return false;

    if (IsAccessorySlot(Item->GearSlot))
    {
        if (GearLevel != 1) return false;
        OutStats.ItemId = ItemId;
        OutStats.GearLevel = 1;
        OutStats.MaxDurability = 0;
        OutStats.Armor = Item->Armor;
        return true;
    }

    const UKalmalaToolCatalogue* ToolCatalogue = UKalmalaToolCatalogue::Get();
    if (!ToolCatalogue) return false;
    const FKalmalaToolRewardData& Reward = ToolCatalogue->GetLevelUpReward(ItemId);
    const int64 MaxDurability = int64(Item->MaxDurability)
        + int64(GearLevel - 1) * Reward.MaxDurability;
    const int64 Armor = int64(Item->Armor) + int64(GearLevel - 1) * Reward.Armor;
    if (MaxDurability < 1 || MaxDurability > 1000 || Armor < 0 || Armor > 1000) return false;

    OutStats.ItemId = ItemId;
    OutStats.GearLevel = GearLevel;
    OutStats.MaxDurability = static_cast<int32>(MaxDurability);
    OutStats.Armor = static_cast<int32>(Armor);
    return true;
}

const FKalmalaItemDefinition* UKalmalaItemCatalogue::FindItem(FName ItemId) const
{
    if (ItemId.IsNone() || !IsValidCatalogue())
    {
        return nullptr;
    }
    return Items.FindByPredicate([ItemId](const FKalmalaItemDefinition& Item) { return Item.ItemId == ItemId; });
}

bool UKalmalaItemCatalogue::IsValidStack(FName ItemId, int32 Quantity) const
{
    const FKalmalaItemDefinition* Item = FindItem(ItemId);
    return Item && Quantity > 0 && Quantity <= Item->MaxStack;
}

const FKalmalaItemDefinition* UKalmalaItemCatalogue::FindBuildable(const FName BuildableId) const
{
    if (BuildableId.IsNone() || !IsValidCatalogue()) return nullptr;
    return Buildables.FindByPredicate([BuildableId](const FKalmalaItemDefinition& Definition)
        { return Definition.ItemId == BuildableId; });
}

const FKalmalaItemDefinition* UKalmalaItemCatalogue::FindDefinition(const FName Id) const
{
    if (const auto* Item = FindItem(Id)) return Item;
    return FindBuildable(Id);
}

bool UKalmalaItemCatalogue::CanAddToStack(FName ItemId, int32 ExistingQuantity, int32 AddedQuantity) const
{
    const FKalmalaItemDefinition* Item = FindItem(ItemId);
    return Item && ExistingQuantity >= 0 && ExistingQuantity <= Item->MaxStack
        && AddedQuantity > 0 && AddedQuantity <= Item->MaxStack - ExistingQuantity;
}
