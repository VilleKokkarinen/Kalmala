#include "KalmalaItemCatalogue.h"

bool UKalmalaItemCatalogue::IsValidCatalogue() const
{
    if (Items.IsEmpty() || Items.Num() > MaxDefinitions)
    {
        return false;
    }
    TSet<FName> Seen;
    for (const FKalmalaItemDefinition& Item : Items)
    {
        if (Item.ItemId.IsNone() || Seen.Contains(Item.ItemId)
            || Item.DisplayName.TrimStartAndEnd().IsEmpty() || Item.DisplayName.Len() > 64
            || Item.MaxStack < 1 || Item.MaxStack > AbsoluteMaxStack)
        {
            return false;
        }
        Seen.Add(Item.ItemId);
    }
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
