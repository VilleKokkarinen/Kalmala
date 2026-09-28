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
