#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"

bool UKalmalaRecipeCatalogue::Scale(const FKalmalaRecipe& Recipe, int32 Batch,
    TArray<FKalmalaInventoryStack>& Costs, int32& OutputCount)
{
    const auto* Items = GetDefault<UKalmalaItemCatalogue>();
    if (Recipe.MaxBatch < 1 || Recipe.MaxBatch > 10 || Batch < 1 || Batch > Recipe.MaxBatch
        || Recipe.Ingredients.IsEmpty() || Recipe.Ingredients.Num() > 8) return false;
    const int64 Total = int64(Recipe.OutputCount) * Batch;
    if (Total < 1 || Total > UKalmalaItemCatalogue::AbsoluteMaxStack
        || !Items->IsValidStack(Recipe.Output, int32(Total))) return false;
    TSet<FName> Seen;
    TArray<FKalmalaInventoryStack> Next;
    for (const auto& Cost : Recipe.Ingredients)
    {
        const int64 Count = int64(Cost.Quantity) * Batch;
        if (Seen.Contains(Cost.ItemId) || Count < 1 || Count > UKalmalaItemCatalogue::AbsoluteMaxStack
            || !Items->IsValidStack(Cost.ItemId, int32(Count))) return false;
        Seen.Add(Cost.ItemId);
        auto& Entry = Next.AddDefaulted_GetRef(); Entry.ItemId = Cost.ItemId; Entry.Quantity = int32(Count);
    }
    Costs = MoveTemp(Next); OutputCount = int32(Total); return true;
}

bool UKalmalaRecipeCatalogue::IsValidCatalogue() const
{
    if (Recipes.IsEmpty() || Recipes.Num() > 32) return false;
    TSet<FName> Seen;
    for (const auto& Recipe : Recipes)
    {
        TArray<FKalmalaInventoryStack> Costs; int32 Count;
        if (Recipe.RecipeId.IsNone() || Seen.Contains(Recipe.RecipeId) || Recipe.DisplayName.TrimStartAndEnd().IsEmpty()
            || Recipe.DisplayName.Len() > 64 || !Scale(Recipe, Recipe.MaxBatch, Costs, Count)) return false;
        Seen.Add(Recipe.RecipeId);
    }
    return true;
}

const FKalmalaRecipe* UKalmalaRecipeCatalogue::Find(FName Id) const
{
    return IsValidCatalogue() ? Recipes.FindByPredicate([Id](const auto& R) { return R.RecipeId == Id; }) : nullptr;
}
