#include "KalmalaToolCatalogue.h"

#include "KalmalaGameCatalogueLoader.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolProgressionContract.h"

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
            || Profile.MinimumSkillLevel < 1 || Profile.MinimumSkillLevel > 10
            || Profile.DurabilityCost < 0 || Profile.DurabilityCost > 10
            || Profile.Levels.IsEmpty() || Profile.Levels.Num() > MaxLevels)
        {
            return false;
        }

        SeenToolIds.Add(Profile.ToolId);
        SeenToolKinds.Add(Profile.ToolKind);
        if (Profile.ToolSet == TEXT("Starter")) ++StarterCount;
        else if (Profile.ToolSet == TEXT("Crafted")) ++CraftedCount;
        else ++UtilityCount;

        if (Profile.Attacks.IsEmpty() || Profile.Attacks.Num() > 8) return false;
        TSet<FName> SeenAttacks;
        for (const FKalmalaWeaponAttackData& Attack : Profile.Attacks)
        {
            if (Attack.AttackId.IsNone() || Attack.AttackId.ToString().Len() > 32
                || SeenAttacks.Contains(Attack.AttackId) || !Attack.DamageBonuses.IsValid(1000.0f)) return false;
            SeenAttacks.Add(Attack.AttackId);
        }
        if (!SeenAttacks.Contains(TEXT("Basic"))) return false;

        TSet<int32> SeenLevels;
        bool bHasLevelOne = false;
        for (const FKalmalaToolLevelData& Level : Profile.Levels)
        {
            if (Level.ToolLevel < 1 || Level.ToolLevel > MaxLevels || SeenLevels.Contains(Level.ToolLevel)
                || Level.MaxDurability < 1 || Level.MaxDurability > 1000
                || !Level.DamageBonuses.IsValid(1000.0f)
                || (Profile.ToolKind == TEXT("ConstructionHammer") ? Level.DamageBonuses.Total() != 0.0f
                    : Level.DamageBonuses.Total() <= 0.0f))
            {
                return false;
            }
            SeenLevels.Add(Level.ToolLevel);
            bHasLevelOne |= Level.ToolLevel == 1;
        }
        if (!bHasLevelOne) return false;
        for (int32 Level = 1; Level <= Profile.Levels.Num(); ++Level)
            if (!SeenLevels.Contains(Level)) return false;

        if (Profile.ToolSet == TEXT("Crafted"))
        {
            const bool bHasAcquisition = Recipes.ContainsByPredicate([&Profile](const FKalmalaToolRecipeData& Recipe)
            {
                return Recipe.ToolId == Profile.ToolId && Recipe.Level == 1;
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
        const FString TargetKey = FString::Printf(TEXT("%s:%d"), *Recipe.ToolId.ToString(), Recipe.Level);
        if (Recipe.RecipeId.IsNone() || SeenRecipeIds.Contains(Recipe.RecipeId)
            || Recipe.DisplayName.TrimStartAndEnd().IsEmpty() || Recipe.DisplayName.Len() > 64
            || Profile == nullptr || Profile->ToolSet != TEXT("Crafted")
            || SeenRecipeTargets.Contains(TargetKey)
            || Recipe.Level != 1
            || Recipe.RequiredStationLevel != Recipe.Level
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
            return Level.ToolLevel == Recipe.Level;
        });
        if (!bHasTargetLevel) return false;

        if (Recipe.Level != 1 || Recipe.RecipeId != Recipe.ToolId
            || Recipe.RequiredSkill != TEXT("None") || Recipe.RequiredSkillLevel != 0) return false;

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

    TSet<FName> SeenUpgradeIds;
    for (const FKalmalaToolRecipeData& Upgrade : UpgradeCosts)
    {
        const FKalmalaToolProfileData* Profile = FindProfile(Upgrade.ToolId);
        if (!Profile || Profile->ToolSet != TEXT("Crafted") || Profile->Levels.Num() < 2
            || SeenUpgradeIds.Contains(Upgrade.ToolId) || Upgrade.RecipeId != Upgrade.ToolId
            || Upgrade.Level != 1 || Upgrade.RequiredStationLevel != 0
            || !IsOneOf(Upgrade.RequiredStation, {TEXT("Workbench"), TEXT("Forge")})
            || Upgrade.RequiredSkill != TEXT("None") || Upgrade.RequiredSkillLevel != 0
            || Upgrade.DisplayName.TrimStartAndEnd().IsEmpty() || Upgrade.DisplayName.Len() > 64
            || Upgrade.Ingredients.IsEmpty() || Upgrade.Ingredients.Num() > MaxIngredientsPerRecipe)
            return false;
        SeenUpgradeIds.Add(Upgrade.ToolId);
        TSet<FName> SeenCostIds;
        for (const FKalmalaToolRecipeIngredientData& Cost : Upgrade.Ingredients)
        {
            if (SeenCostIds.Contains(Cost.ItemId) || !Items->IsValidStack(Cost.ItemId, Cost.Quantity)) return false;
            SeenCostIds.Add(Cost.ItemId);
        }
        const int32 StationTargetCap = FMath::Min(Profile->Levels.Num(), FKalmalaToolProgressionContract::MaxStationToolLevel);
        for (int32 CurrentLevel = 1; CurrentLevel < StationTargetCap; ++CurrentLevel)
        {
            TArray<FKalmalaToolRecipeIngredientData> ScaledCosts;
            if (!ScaleUpgradeCosts(Upgrade.Ingredients, CurrentLevel, ScaledCosts)) return false;
            for (const FKalmalaToolRecipeIngredientData& Cost : ScaledCosts)
                if (!Items->FindItem(Cost.ItemId) || Cost.Quantity <= 0) return false;
        }
    }
    if (UpgradeCosts.Num() > MaxRecipes) return false;
    for (const FKalmalaToolProfileData& Profile : Profiles)
        if (Profile.Levels.Num() > 1 && !SeenUpgradeIds.Contains(Profile.ToolId)) return false;
    return true;
}

bool UKalmalaToolCatalogue::ScaleUpgradeCosts(
    const TArray<FKalmalaToolRecipeIngredientData>& BaseCosts, const int32 CurrentLevel,
    TArray<FKalmalaToolRecipeIngredientData>& OutCosts)
{
    OutCosts.Reset();
    if (CurrentLevel < 1 || CurrentLevel > 31 || BaseCosts.IsEmpty()) return false;
    const int64 Multiplier = int64(1) << (CurrentLevel - 1);
    for (const FKalmalaToolRecipeIngredientData& Base : BaseCosts)
    {
        const int64 Quantity = int64(Base.Quantity) * Multiplier;
        if (Base.ItemId.IsNone() || Base.Quantity <= 0 || Quantity > MAX_int32)
        {
            OutCosts.Reset();
            return false;
        }
        FKalmalaToolRecipeIngredientData& Cost = OutCosts.AddDefaulted_GetRef();
        Cost.ItemId = Base.ItemId;
        Cost.Quantity = static_cast<int32>(Quantity);
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
