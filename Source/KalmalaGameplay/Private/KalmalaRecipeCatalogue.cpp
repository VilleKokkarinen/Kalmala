#include "KalmalaRecipeCatalogue.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaGameCatalogueLoader.h"

const UKalmalaRecipeCatalogue* UKalmalaRecipeCatalogue::Get()
{
    FKalmalaGameCatalogueLoader::EnsureLoaded();
    return GetDefault<UKalmalaRecipeCatalogue>();
}

bool UKalmalaRecipeCatalogue::Scale(const FKalmalaRecipe& Recipe, int32 Batch,
    TArray<FKalmalaInventoryStack>& Costs, int32& OutputCount)
{
    const auto* Items = UKalmalaItemCatalogue::Get();
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
        const bool bHasExperienceAward = Recipe.ExperienceSkill != EKalmalaSkill::None;
        if (Recipe.RecipeId.IsNone() || Seen.Contains(Recipe.RecipeId) || Recipe.DisplayName.TrimStartAndEnd().IsEmpty()
            || Recipe.DisplayName.Len() > 64
            || (bHasExperienceAward && (!FKalmalaSkillProgressionContract::IsKnownSkill(Recipe.ExperienceSkill)
                || Recipe.ExperienceAward < 1 || Recipe.ExperienceAward > FKalmalaSkillProgressionContract::MaxAwardPerAcceptedAction))
            || (!bHasExperienceAward && Recipe.ExperienceAward != 0)
            || Recipe.RequiredStation.Num() > 4
            || !Scale(Recipe, Recipe.MaxBatch, Costs, Count)) return false;
        if (!Recipe.RequiredTool.IsNone())
        {
            if (!UKalmalaItemCatalogue::Get()->IsValidStack(Recipe.RequiredTool, 1)
                || Recipe.Ingredients.ContainsByPredicate([&Recipe](const FKalmalaInventoryStack& Ingredient)
                    { return Ingredient.ItemId == Recipe.RequiredTool; })) return false;
        }
        TSet<FName> StationIds;
        for (const FName Station : Recipe.RequiredStation)
        {
            if (Station.IsNone() || StationIds.Contains(Station)
                || !AKalmalaConstructionActor::IsCraftingStationKit(Station)) return false;
            StationIds.Add(Station);
        }
        Seen.Add(Recipe.RecipeId);
    }
    return true;
}

bool UKalmalaRecipeCatalogue::HasRequiredTool(const FName ToolId,
    const TArray<FKalmalaInventoryStack>& Stacks)
{
    if (ToolId.IsNone()) return true;
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    if (!Items || !Items->IsValidStack(ToolId, 1)) return false;
    return Stacks.ContainsByPredicate([Items, ToolId](const FKalmalaInventoryStack& Stack)
    {
        return Stack.ItemId == ToolId && Items->IsValidStack(Stack.ItemId, Stack.Quantity);
    });
}

const FKalmalaRecipe* UKalmalaRecipeCatalogue::Find(FName Id) const
{
    return IsValidCatalogue() ? Recipes.FindByPredicate([Id](const auto& R) { return R.RecipeId == Id; }) : nullptr;
}

bool UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(const FName BuildableId)
{
    return BuildableId == TEXT("CampfireKit") || BuildableId == TEXT("FloorKit") || BuildableId == TEXT("WallKit")
        || BuildableId == TEXT("RoofKit");
}

bool UKalmalaRecipeCatalogue::BuildDirectMaterialCost(const FName BuildableId,
    TArray<FKalmalaInventoryStack>& OutCosts, FString& Reason)
{
    Reason = TEXT("Unknown direct-material buildable");
    if (!IsDirectMaterialBuildable(BuildableId)) return false;

    const UKalmalaRecipeCatalogue* Catalogue = Get();
    if (!Catalogue || !Catalogue->IsValidCatalogue())
    {
        Reason = TEXT("Build material catalogue unavailable");
        return false;
    }
    const FKalmalaRecipe* BuildRecipe = Catalogue->Recipes.FindByPredicate([BuildableId](const FKalmalaRecipe& Candidate)
    {
        return Candidate.Output == BuildableId;
    });
    if (!BuildRecipe || !BuildRecipe->bEnabled || BuildRecipe->Ingredients.IsEmpty())
    {
        Reason = TEXT("Build material catalogue unavailable");
        return false;
    }

    TArray<FKalmalaInventoryStack> CandidateCosts;
    const auto AddCost = [&CandidateCosts](const FName ItemId, const int32 Quantity)
    {
        FKalmalaInventoryStack* Existing = CandidateCosts.FindByPredicate([ItemId](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == ItemId;
        });
        if (Existing)
        {
            if (Quantity < 1 || Existing->Quantity > UKalmalaItemCatalogue::AbsoluteMaxStack - Quantity) return false;
            Existing->Quantity += Quantity;
            return true;
        }
        FKalmalaInventoryStack& NewCost = CandidateCosts.AddDefaulted_GetRef();
        NewCost.ItemId = ItemId;
        NewCost.Quantity = Quantity;
        return Quantity > 0 && CandidateCosts.Num() <= UKalmalaInventoryComponent::MaxSlots;
    };

    for (const FKalmalaInventoryStack& Ingredient : BuildRecipe->Ingredients)
    {
        if (!AddCost(Ingredient.ItemId, Ingredient.Quantity))
        {
            Reason = TEXT("Build material cost exceeds its bound");
            return false;
        }
    }

    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    for (const FKalmalaInventoryStack& Cost : CandidateCosts)
    {
        if (!Items->IsValidStack(Cost.ItemId, Cost.Quantity))
        {
            Reason = TEXT("Build material catalogue is invalid");
            return false;
        }
    }
    OutCosts = MoveTemp(CandidateCosts);
    Reason = TEXT("Ready");
    return true;
}
