#include "KalmalaItemCatalogue.h"
#include "KalmalaGameCatalogueLoader.h"
#include "KalmalaToolCatalogue.h"

namespace
{
    bool IsGearSlot(const FString& Slot)
    {
        return Slot == TEXT("Head") || Slot == TEXT("Cape") || Slot == TEXT("Chest")
            || Slot == TEXT("Shield") || Slot == TEXT("Legs") || Slot == TEXT("Boots");
    }

    bool IsZeroDefense(const FKalmalaResistanceProfile& Defense)
    {
        return Defense.Armor == 0.0f
            && Defense.Reductions.Slash == 0.0f && Defense.Reductions.Stab == 0.0f
            && Defense.Reductions.Crush == 0.0f && Defense.Reductions.Pierce == 0.0f
            && Defense.Reductions.Fire == 0.0f && Defense.Reductions.Water == 0.0f
            && Defense.Reductions.Holy == 0.0f && Defense.Reductions.Poison == 0.0f;
    }
}

const UKalmalaItemCatalogue* UKalmalaItemCatalogue::Get()
{
    FKalmalaGameCatalogueLoader::EnsureLoaded();
    return GetDefault<UKalmalaItemCatalogue>();
}

bool UKalmalaItemCatalogue::IsValidCatalogue() const
{
    if (Items.IsEmpty() || Items.Num() > MaxDefinitions)
    {
        return false;
    }
    TSet<FName> Seen;
    for (const FKalmalaItemDefinition& Item : Items)
    {
        const bool bIsGear = !Item.GearSlot.IsEmpty();
        if (Item.ItemId.IsNone() || Seen.Contains(Item.ItemId)
            || Item.DisplayName.TrimStartAndEnd().IsEmpty() || Item.DisplayName.Len() > 64
            || Item.Description.TrimStartAndEnd().IsEmpty() || Item.Description.Len() > 180
            || Item.MaxStack < 1 || Item.MaxStack > AbsoluteMaxStack
            || !FMath::IsFinite(Item.WeightKg) || Item.WeightKg < 0.0f || Item.WeightKg > 300.0f
            || !Item.DefenseBonuses.IsValid()
            || (bIsGear
                ? (!IsGearSlot(Item.GearSlot) || Item.MaxStack != 1
                    || Item.MaxDurability < 1 || Item.MaxDurability > 1000
                    || Item.DefenseBonuses.Armor > 1000.0f
                    || !Item.DefenseBonuses.Reductions.IsValid(1000.0f, -1000.0f))
                : (Item.MaxDurability != 0 || !IsZeroDefense(Item.DefenseBonuses))))
        {
            return false;
        }
        Seen.Add(Item.ItemId);
    }
    return true;
}

bool UKalmalaItemCatalogue::GetGearLevelStats(
    const FName ItemId, const int32 GearLevel, FKalmalaGearLevelStats& OutStats) const
{
    OutStats = {};
    if (GearLevel < 1 || GearLevel > FKalmalaToolLifecycleContract::MaxToolLevel) return false;

    const FKalmalaItemDefinition* Item = FindItem(ItemId);
    if (!Item || !IsGearSlot(Item->GearSlot)) return false;

    const UKalmalaToolCatalogue* ToolCatalogue = UKalmalaToolCatalogue::Get();
    if (!ToolCatalogue) return false;
    const FKalmalaToolRewardData& Reward = ToolCatalogue->GetLevelUpReward(ItemId);
    const int64 MaxDurability = int64(Item->MaxDurability)
        + int64(GearLevel - 1) * Reward.MaxDurability;
    const FKalmalaResistanceProfile DefenseBonuses = Item->DefenseBonuses.Added(
        Reward.DefenseBonuses.Scaled(static_cast<float>(GearLevel - 1)));
    if (MaxDurability > 1000 || !DefenseBonuses.IsValid()
        || DefenseBonuses.Armor > 1000.0f
        || !DefenseBonuses.Reductions.IsValid(1000.0f, -1000.0f)) return false;

    OutStats.ItemId = ItemId;
    OutStats.GearLevel = GearLevel;
    OutStats.MaxDurability = static_cast<int32>(MaxDurability);
    OutStats.DefenseBonuses = DefenseBonuses;
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

bool UKalmalaItemCatalogue::CanAddToStack(FName ItemId, int32 ExistingQuantity, int32 AddedQuantity) const
{
    const FKalmalaItemDefinition* Item = FindItem(ItemId);
    return Item && ExistingQuantity >= 0 && ExistingQuantity <= Item->MaxStack
        && AddedQuantity > 0 && AddedQuantity <= Item->MaxStack - ExistingQuantity;
}
