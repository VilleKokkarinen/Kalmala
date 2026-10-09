#include "KalmalaRecipeCatalogue.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaGameCatalogueLoader.h"

namespace
{
    bool BuildScaledIngredientCosts(const FKalmalaRecipe& Recipe, const int32 Batch,
        const UKalmalaItemCatalogue* Items, TArray<FKalmalaInventoryStack>& OutCosts)
    {
        if (!Items || Recipe.Ingredients.IsEmpty() || Recipe.Ingredients.Num() > 8) return false;
        TSet<FName> Seen;
        TArray<FKalmalaInventoryStack> CandidateCosts;
        for (const FKalmalaInventoryStack& Ingredient : Recipe.Ingredients)
        {
            const int64 Count = int64(Ingredient.Quantity) * Batch;
            if (Seen.Contains(Ingredient.ItemId) || Count < 1
                || Count > UKalmalaItemCatalogue::AbsoluteMaxStack
                || !Items->IsValidStack(Ingredient.ItemId, int32(Count))) return false;
            Seen.Add(Ingredient.ItemId);
            FKalmalaInventoryStack& Entry = CandidateCosts.AddDefaulted_GetRef();
            Entry.ItemId = Ingredient.ItemId;
            Entry.Quantity = int32(Count);
        }
        OutCosts = MoveTemp(CandidateCosts);
        return true;
    }
}

const UKalmalaRecipeCatalogue* UKalmalaRecipeCatalogue::Get()
{
    FKalmalaGameCatalogueLoader::EnsureLoaded();
    return GetDefault<UKalmalaRecipeCatalogue>();
}

bool UKalmalaRecipeCatalogue::Scale(const FKalmalaRecipe& Recipe, int32 Batch,
    TArray<FKalmalaInventoryStack>& Costs, int32& OutputCount,
    const UKalmalaItemCatalogue* ItemDefinitions)
{
    const auto* Items = ItemDefinitions ? ItemDefinitions : UKalmalaItemCatalogue::Get();
    if (Recipe.MaxBatch < 1 || Recipe.MaxBatch > 10 || Batch < 1 || Batch > Recipe.MaxBatch
        || !Recipe.BuildableOutput.IsNone() || Recipe.Output.IsNone()
        || !Items) return false;
    const int64 Total = int64(Recipe.OutputCount) * Batch;
    if (Total < 1 || Total > UKalmalaItemCatalogue::AbsoluteMaxStack
        || !Items->IsValidStack(Recipe.Output, int32(Total))) return false;
    TArray<FKalmalaInventoryStack> Next;
    if (!BuildScaledIngredientCosts(Recipe, Batch, Items, Next)) return false;
    Costs = MoveTemp(Next);
    OutputCount = int32(Total);
    return true;
}

bool UKalmalaRecipeCatalogue::IsValidCatalogue(const UKalmalaItemCatalogue* ItemDefinitions) const
{
    if (Recipes.IsEmpty() || Recipes.Num() > 32) return false;
    const UKalmalaItemCatalogue* Items = ItemDefinitions ? ItemDefinitions : UKalmalaItemCatalogue::Get();
    if (!Items || !Items->IsValidCatalogue()) return false;
    TSet<FName> Seen;
    TSet<FName> BuildableOutputs;
    for (const auto& Recipe : Recipes)
    {
        TArray<FKalmalaInventoryStack> Costs; int32 Count;
        const bool bHasExperienceAward = Recipe.ExperienceSkill != EKalmalaSkill::None;
        const bool bDirectBuildable = !Recipe.BuildableOutput.IsNone();
        bool bValidOutput = false;
        if (bDirectBuildable)
        {
            TArray<FKalmalaInventoryStack> IngredientCosts;
            bValidOutput = IsDirectMaterialBuildable(Recipe.BuildableOutput)
                && Recipe.Output.IsNone() && Recipe.OutputCount == 1 && Recipe.MaxBatch == 1
                && Recipe.RequiredStation.IsEmpty() && Recipe.RequiredTool.IsNone()
                && !BuildableOutputs.Contains(Recipe.BuildableOutput)
                && BuildScaledIngredientCosts(Recipe, Recipe.MaxBatch, Items, IngredientCosts);
        }
        else
        {
            bValidOutput = Recipe.BuildableOutput.IsNone()
                && Scale(Recipe, Recipe.MaxBatch, Costs, Count, Items);
        }
        if (Recipe.RecipeId.IsNone() || Seen.Contains(Recipe.RecipeId) || Recipe.DisplayName.TrimStartAndEnd().IsEmpty()
            || Recipe.DisplayName.Len() > 64
            || (bHasExperienceAward && (!FKalmalaSkillProgressionContract::IsKnownSkill(Recipe.ExperienceSkill)
                || Recipe.ExperienceAward < 1 || Recipe.ExperienceAward > FKalmalaSkillProgressionContract::MaxAwardPerAcceptedAction))
            || (!bHasExperienceAward && Recipe.ExperienceAward != 0)
            || Recipe.RequiredStation.Num() > 4
            || !bValidOutput) return false;
        if (!Recipe.RequiredTool.IsNone())
        {
            if (!Items->IsValidStack(Recipe.RequiredTool, 1)
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
        if (bDirectBuildable) BuildableOutputs.Add(Recipe.BuildableOutput);
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
        return Candidate.BuildableOutput == BuildableId;
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
