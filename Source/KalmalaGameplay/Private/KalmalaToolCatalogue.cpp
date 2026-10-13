#include "KalmalaToolCatalogue.h"

#include "KalmalaGameCatalogueLoader.h"
#include "KalmalaItemCatalogue.h"

#include <initializer_list>

namespace
{
    bool IsOneOf(const FString& Value, std::initializer_list<const TCHAR*> AllowedValues)
    {
        for (const TCHAR* Allowed : AllowedValues)
        {
            if (Value == Allowed) return true;
        }
        return false;
    }
}

const UKalmalaToolCatalogue* UKalmalaToolCatalogue::Get()
{
    FKalmalaGameCatalogueLoader::EnsureLoaded();
    return GetDefault<UKalmalaToolCatalogue>();
}

bool UKalmalaToolCatalogue::IsValidCatalogue(const UKalmalaItemCatalogue* ItemDefinitions) const
{
    if (Profiles.IsEmpty() || Profiles.Num() > MaxProfiles
        || Recipes.IsEmpty() || Recipes.Num() > MaxRecipes) return false;

    const UKalmalaItemCatalogue* Items = ItemDefinitions ? ItemDefinitions : UKalmalaItemCatalogue::Get();
    if (!Items || !Items->IsValidCatalogue()) return false;

    TSet<FName> SeenToolIds;
    TSet<FString> SeenToolKinds;
    int32 StarterCount = 0;
    int32 CraftedCount = 0;
    int32 UtilityCount = 0;
    for (const FKalmalaToolProfileData& Profile : Profiles)
    {
        if (Profile.ToolId.IsNone() || SeenToolIds.Contains(Profile.ToolId)
            || Profile.DisplayName.TrimStartAndEnd().IsEmpty() || Profile.DisplayName.Len() > 64
            || Profile.Description.TrimStartAndEnd().IsEmpty() || Profile.Description.Len() > 180
            || SeenToolKinds.Contains(Profile.ToolKind)
            || Profile.ToolId != FName(*Profile.ToolKind)
            || !IsOneOf(Profile.ToolKind, {TEXT("StoneKnife"), TEXT("StoneAxe"), TEXT("StonePick"),
                TEXT("StoneSpear"), TEXT("BronzeAxe"), TEXT("BronzeKnife"), TEXT("BronzePick"),
                TEXT("BronzeSpear"), TEXT("IronAxe"), TEXT("IronKnife"), TEXT("IronPick"),
                TEXT("IronSpear"), TEXT("ConstructionHammer")})
            || !IsOneOf(Profile.ToolSet, {TEXT("Starter"), TEXT("Crafted"), TEXT("Utility")})
            || !IsOneOf(Profile.Action, {TEXT("None"), TEXT("Gathering"), TEXT("Woodcutting"), TEXT("Mining")})
            || !IsOneOf(Profile.RequiredSkill, {TEXT("None"), TEXT("Gathering"), TEXT("Woodcutting"),
                TEXT("Mining"), TEXT("Crafting"), TEXT("Cooking"), TEXT("Survival")})
            || !IsOneOf(Profile.DamageType, {TEXT("None"), TEXT("Slash"), TEXT("Pierce"), TEXT("Blunt")})
            || Profile.MinimumSkillLevel < 1 || Profile.MinimumSkillLevel > 10
            || Profile.DurabilityCost < 0 || Profile.DurabilityCost > 10
            || Profile.Levels.IsEmpty() || Profile.Levels.Num() > 2)
        {
            return false;
        }

        SeenToolIds.Add(Profile.ToolId);
        SeenToolKinds.Add(Profile.ToolKind);
        if (Profile.ToolSet == TEXT("Starter")) ++StarterCount;
        else if (Profile.ToolSet == TEXT("Crafted")) ++CraftedCount;
        else ++UtilityCount;

        TSet<int32> SeenLevels;
        bool bHasLevelOne = false;
        bool bHasLevelTwo = false;
        for (const FKalmalaToolLevelData& Level : Profile.Levels)
        {
            if (Level.ToolLevel < 1 || Level.ToolLevel > 2 || SeenLevels.Contains(Level.ToolLevel)
                || Level.MaxDurability < 1 || Level.MaxDurability > 1000
                || !FMath::IsFinite(Level.CombatDamage) || Level.CombatDamage < 0.0f || Level.CombatDamage > 1000.0f
                || (Profile.DamageType == TEXT("None") && Level.CombatDamage != 0.0f)
                || (Profile.DamageType != TEXT("None") && Level.CombatDamage <= 0.0f))
            {
                return false;
            }
            SeenLevels.Add(Level.ToolLevel);
            bHasLevelOne |= Level.ToolLevel == 1;
            bHasLevelTwo |= Level.ToolLevel == 2;
        }
        if (!bHasLevelOne || (bHasLevelTwo && !SeenLevels.Contains(1))) return false;

        if (Profile.ToolSet == TEXT("Crafted"))
        {
            const bool bHasAcquisition = Recipes.ContainsByPredicate([&Profile](const FKalmalaToolRecipeData& Recipe)
            {
                return Recipe.ToolId == Profile.ToolId && Recipe.TargetToolLevel == 1;
            });
            if (!bHasAcquisition) return false;
        }
    }
    if (StarterCount == 0 || CraftedCount == 0 || UtilityCount == 0) return false;

    TSet<FName> SeenRecipeIds;
    TSet<FString> SeenRecipeTargets;
    for (const FKalmalaToolRecipeData& Recipe : Recipes)
    {
        const FKalmalaToolProfileData* Profile = FindProfile(Recipe.ToolId);
        const FString TargetKey = FString::Printf(TEXT("%s:%d"), *Recipe.ToolId.ToString(), Recipe.TargetToolLevel);
        if (Recipe.RecipeId.IsNone() || SeenRecipeIds.Contains(Recipe.RecipeId)
            || Recipe.DisplayName.TrimStartAndEnd().IsEmpty() || Recipe.DisplayName.Len() > 64
            || Profile == nullptr || Profile->ToolSet != TEXT("Crafted")
            || SeenRecipeTargets.Contains(TargetKey)
            || Recipe.TargetToolLevel < 1 || Recipe.TargetToolLevel > 2
            || Recipe.RequiredStationLevel != Recipe.TargetToolLevel
            || !IsOneOf(Recipe.RequiredStation, {TEXT("Workbench"), TEXT("Forge")})
            || !IsOneOf(Recipe.RequiredSkill, {TEXT("None"), TEXT("Gathering"), TEXT("Woodcutting"),
                TEXT("Mining"), TEXT("Crafting"), TEXT("Cooking"), TEXT("Survival")})
            || Recipe.RequiredSkillLevel < 0 || Recipe.RequiredSkillLevel > 10
            || Recipe.Ingredients.IsEmpty() || Recipe.Ingredients.Num() > MaxIngredientsPerRecipe)
        {
            return false;
        }

        const bool bHasTargetLevel = Profile->Levels.ContainsByPredicate([&Recipe](const FKalmalaToolLevelData& Level)
        {
            return Level.ToolLevel == Recipe.TargetToolLevel;
        });
        if (!bHasTargetLevel) return false;

        if (Recipe.TargetToolLevel == 1)
        {
            if (!Recipe.PreviousToolId.IsNone() || Recipe.PreviousToolLevel != 0
                || Recipe.RequiredSkill != TEXT("None") || Recipe.RequiredSkillLevel != 0
                || Recipe.RequiredStationLevel != 1) return false;
        }
        else if (Recipe.PreviousToolId != Recipe.ToolId || Recipe.PreviousToolLevel != Recipe.TargetToolLevel - 1
            || Recipe.RequiredSkill == TEXT("None") || Recipe.RequiredSkillLevel < 2
            || Recipe.RequiredStation != TEXT("Forge"))
        {
            return false;
        }

        TSet<FName> SeenCostIds;
        for (const FKalmalaToolRecipeIngredientData& Ingredient : Recipe.Ingredients)
        {
            if (SeenCostIds.Contains(Ingredient.ItemId) || !Items->IsValidStack(Ingredient.ItemId, Ingredient.Quantity))
                return false;
            SeenCostIds.Add(Ingredient.ItemId);
        }

        SeenRecipeIds.Add(Recipe.RecipeId);
        SeenRecipeTargets.Add(TargetKey);
    }

    for (const FKalmalaToolProfileData& Profile : Profiles)
    {
        const bool bHasLevelTwo = Profile.Levels.ContainsByPredicate([](const FKalmalaToolLevelData& Level)
        {
            return Level.ToolLevel == 2;
        });
        if (bHasLevelTwo && !SeenRecipeTargets.Contains(FString::Printf(TEXT("%s:2"), *Profile.ToolId.ToString())))
            return false;
    }
    return true;
}

const FKalmalaToolProfileData* UKalmalaToolCatalogue::FindProfile(const FName ToolId) const
{
    if (ToolId.IsNone()) return nullptr;
    return Profiles.FindByPredicate([ToolId](const FKalmalaToolProfileData& Profile)
    {
        return Profile.ToolId == ToolId;
    });
}
