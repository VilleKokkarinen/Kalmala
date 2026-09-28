#include "KalmalaRecipeCatalogue.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaGameCatalogueLoader.h"
#include "KalmalaToolLifecycleContract.h"

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
    const bool bOutputsTool = !Recipe.OutputTool.IsNone();
    const int64 Total = int64(Recipe.OutputCount) * Batch;
    if (bOutputsTool)
    {
        if (!FKalmalaToolLifecycleContract::FindDefinition(Recipe.OutputTool)
            || !Recipe.Output.IsNone() || Recipe.OutputCount != 1 || Recipe.MaxBatch != 1 || Batch != 1
            || Recipe.RequiredStationKit != TEXT("WorkbenchKit")) return false;
    }
    else if (Total < 1 || Total > UKalmalaItemCatalogue::AbsoluteMaxStack
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
        const bool bHasSkillRequirement = Recipe.RequiredSkill != EKalmalaSkill::None;
        if (Recipe.RecipeId.IsNone() || Seen.Contains(Recipe.RecipeId) || Recipe.DisplayName.TrimStartAndEnd().IsEmpty()
            || Recipe.DisplayName.Len() > 64 || (Recipe.bRequiresCampfire && (Recipe.bRequiresLitCampfire || !Recipe.RequiredStationKit.IsNone()
                || !Recipe.AlternateStationKit.IsNone()))
            || (bHasSkillRequirement && (!FKalmalaSkillProgressionContract::IsKnownSkill(Recipe.RequiredSkill)
                || Recipe.RequiredSkillLevel < 2 || Recipe.RequiredSkillLevel > FKalmalaSkillProgressionContract::MaxLevel))
            || (!bHasSkillRequirement && Recipe.RequiredSkillLevel != 0)
            || (bHasExperienceAward && (!FKalmalaSkillProgressionContract::IsKnownSkill(Recipe.ExperienceSkill)
                || Recipe.ExperienceAward < 1 || Recipe.ExperienceAward > FKalmalaSkillProgressionContract::MaxAwardPerAcceptedAction))
            || (!bHasExperienceAward && Recipe.ExperienceAward != 0)
            || (!Recipe.RequiredStationKit.IsNone() && !AKalmalaConstructionActor::IsCraftingStationKit(Recipe.RequiredStationKit))
            || (!Recipe.AlternateStationKit.IsNone() && (Recipe.RequiredStationKit.IsNone()
                || Recipe.AlternateStationKit == Recipe.RequiredStationKit
                || !AKalmalaConstructionActor::IsCraftingStationKit(Recipe.AlternateStationKit)))
            || ( !Recipe.OutputTool.IsNone() && !Recipe.Output.IsNone())
            || !Scale(Recipe, Recipe.MaxBatch, Costs, Count)) return false;
        Seen.Add(Recipe.RecipeId);
    }
    return true;
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
        return Candidate.Output == BuildableId && Candidate.OutputTool.IsNone();
    });
    const FKalmalaRecipe* TimberRecipe = Catalogue->Recipes.FindByPredicate([](const FKalmalaRecipe& Candidate)
    {
        return Candidate.Output == TEXT("ConstructionSupply") && Candidate.OutputTool.IsNone();
    });
    if (!BuildRecipe || !BuildRecipe->bEnabled || !TimberRecipe || TimberRecipe->Ingredients.IsEmpty()
        || BuildRecipe->Ingredients.IsEmpty() || TimberRecipe->OutputCount != 1)
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
        if (Ingredient.ItemId == TEXT("ConstructionSupply"))
        {
            for (const FKalmalaInventoryStack& TimberIngredient : TimberRecipe->Ingredients)
            {
                const int64 ExpandedNumerator = int64(Ingredient.Quantity) * TimberIngredient.Quantity;
                const int64 Expanded = TimberRecipe->OutputCount > 0
                    ? ExpandedNumerator / TimberRecipe->OutputCount : 0;
                if (TimberRecipe->OutputCount <= 0 || ExpandedNumerator % TimberRecipe->OutputCount != 0
                    || Expanded < 1 || Expanded > UKalmalaItemCatalogue::AbsoluteMaxStack
                    || !AddCost(TimberIngredient.ItemId, int32(Expanded)))
                {
                    Reason = TEXT("Build material cost exceeds its bound");
                    return false;
                }
            }
        }
        else if (!AddCost(Ingredient.ItemId, Ingredient.Quantity))
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
