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

    if (LevelUpRewards.Num() > MaxProfiles) return false;
    TSet<FName> RewardedIds;
    for (const FKalmalaLevelUpRewardGroupData& Group : LevelUpRewards)
    {
        if (Group.ItemIds.IsEmpty() || Group.ItemIds.Num() > MaxProfiles
            || Group.LevelUpReward.MaxDurability < 0 || Group.LevelUpReward.MaxDurability > 1000
            || !Group.LevelUpReward.DamageBonuses.IsValid(1000.0f)
            || !Group.LevelUpReward.DefenseBonuses.IsValid()
            || Group.LevelUpReward.DefenseBonuses.Armor > 1000.0f
            || !Group.LevelUpReward.DefenseBonuses.Reductions.IsValid(1000.0f, -1000.0f)) return false;
        for (const FName ItemId : Group.ItemIds)
        {
            const FKalmalaToolProfileData* ToolProfile = FindProfile(ItemId);
            const FKalmalaItemDefinition* GearItem = Items->FindItem(ItemId);
            const bool bIsWearableGear = GearItem && !GearItem->GearSlot.IsEmpty();
            if (ItemId.IsNone() || RewardedIds.Contains(ItemId) || (!ToolProfile && !bIsWearableGear)
                || (ToolProfile && (Group.LevelUpReward.DefenseBonuses.Armor != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Slash != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Stab != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Crush != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Pierce != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Fire != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Water != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Holy != 0.0f
                    || Group.LevelUpReward.DefenseBonuses.Reductions.Poison != 0.0f))
                || (bIsWearableGear && Group.LevelUpReward.DamageBonuses.Total() != 0.0f)) return false;
            if (bIsWearableGear)
            {
                const int64 MaxDurability = int64(GearItem->MaxDurability)
                    + int64(UKalmalaToolCatalogue::MaxLevels - 1) * Group.LevelUpReward.MaxDurability;
                const FKalmalaResistanceProfile MaxDefense = GearItem->DefenseBonuses.Added(
                    Group.LevelUpReward.DefenseBonuses.Scaled(static_cast<float>(UKalmalaToolCatalogue::MaxLevels - 1)));
                if (MaxDurability > 1000 || !MaxDefense.IsValid() || MaxDefense.Armor > 1000.0f
                    || !MaxDefense.Reductions.IsValid(1000.0f, -1000.0f)) return false;
            }
            RewardedIds.Add(ItemId);
        }
    }

    if (AttackLists.IsEmpty() || AttackLists.Num() > MaxProfiles) return false;
    TSet<FName> AttackListIds;
    TSet<FName> AttackItemIds;
    for (const FKalmalaWeaponAttackListData& List : AttackLists)
    {
        if (List.AttackListId.IsNone() || List.AttackListId.ToString().Len() > 32
            || AttackListIds.Contains(List.AttackListId) || List.ItemIds.IsEmpty() || List.ItemIds.Num() > MaxProfiles
            || List.Attacks.IsEmpty() || List.Attacks.Num() > 8) return false;
        AttackListIds.Add(List.AttackListId);
        TSet<FName> SeenAttacks;
        for (const FKalmalaWeaponAttackData& Attack : List.Attacks)
        {
            if (Attack.AttackId.IsNone() || Attack.AttackId.ToString().Len() > 32
                || SeenAttacks.Contains(Attack.AttackId) || !Attack.DamageBonuses.IsValid(1000.0f)) return false;
            SeenAttacks.Add(Attack.AttackId);
        }
        if (!SeenAttacks.Contains(TEXT("Basic"))) return false;
        for (const FName ItemId : List.ItemIds)
        {
            if (ItemId.IsNone() || AttackItemIds.Contains(ItemId) || !FindProfile(ItemId)) return false;
            AttackItemIds.Add(ItemId);
        }
    }

    TSet<FName> SeenItemIds;
    TSet<FString> SeenToolKinds;
    int32 StarterCount = 0;
    int32 CraftedCount = 0;
    int32 UtilityCount = 0;
    for (const FKalmalaToolProfileData& Profile : Profiles)
    {
        if (Profile.ItemId.IsNone() || SeenItemIds.Contains(Profile.ItemId) || !AttackItemIds.Contains(Profile.ItemId)
            || Profile.DisplayName.TrimStartAndEnd().IsEmpty() || Profile.DisplayName.Len() > 64
            || Profile.Description.TrimStartAndEnd().IsEmpty() || Profile.Description.Len() > 180
            || SeenToolKinds.Contains(Profile.ToolKind)
            || Profile.ItemId != FName(*Profile.ToolKind)
            || !IsOneOf(Profile.ToolKind, {TEXT("StoneKnife"), TEXT("StoneAxe"), TEXT("StonePick"),
                TEXT("StoneSpear"), TEXT("BronzeAxe"), TEXT("BronzeKnife"), TEXT("BronzePick"),
                TEXT("BronzeSpear"), TEXT("IronAxe"), TEXT("IronKnife"), TEXT("IronPick"),
                TEXT("IronSpear"), TEXT("StoneMace"), TEXT("BronzeMace"), TEXT("IronMace"), TEXT("ConstructionHammer")})
            || !IsOneOf(Profile.ToolSet, {TEXT("Starter"), TEXT("Crafted"), TEXT("Utility")})
            || !IsOneOf(Profile.Action, {TEXT("None"), TEXT("Gathering"), TEXT("Woodcutting"), TEXT("Mining")})
            || !IsOneOf(Profile.RequiredSkill, {TEXT("None"), TEXT("Gathering"), TEXT("Woodcutting"),
                TEXT("Mining"), TEXT("Crafting"), TEXT("Cooking"), TEXT("Survival")})
            || ((Profile.Action == TEXT("Woodcutting") || Profile.Action == TEXT("Mining"))
                ? (Profile.MaterialTier < 1 || Profile.MaterialTier > 3) : Profile.MaterialTier != 0)
            || Profile.MinimumSkillLevel < 1 || Profile.MinimumSkillLevel > 10
            || Profile.MaxDurability < 1 || Profile.MaxDurability > 1000
            || !Profile.DamageBonuses.IsValid(1000.0f))
        {
            return false;
        }

        SeenItemIds.Add(Profile.ItemId);
        SeenToolKinds.Add(Profile.ToolKind);
        if (Profile.ToolSet == TEXT("Starter")) ++StarterCount;
        else if (Profile.ToolSet == TEXT("Crafted")) ++CraftedCount;
        else ++UtilityCount;

        for (int32 ToolLevel = 1; ToolLevel <= MaxLevels; ++ToolLevel)
        {
            FKalmalaToolLevelStats Stats;
            if (!Profile.GetLevelStats(ToolLevel, GetLevelUpReward(Profile.ItemId), Stats)
                || (Profile.ToolKind == TEXT("ConstructionHammer") ? Stats.DamageBonuses.Total() != 0.0f
                    : Stats.DamageBonuses.Total() <= 0.0f)) return false;
        }

        if (Profile.ToolSet == TEXT("Crafted"))
        {
            const bool bHasAcquisition = Recipes.ContainsByPredicate([&Profile](const FKalmalaToolRecipeData& Recipe)
            {
                return Recipe.ItemId == Profile.ItemId && Recipe.Level == 1;
            });
            if (!bHasAcquisition) return false;
        }
    }
    if (StarterCount == 0 || CraftedCount == 0 || UtilityCount == 0) return false;

    TSet<FName> SeenRecipeIds;
    TSet<FString> SeenRecipeTargets;
    for (const FKalmalaToolRecipeData& Recipe : Recipes)
    {
        const FKalmalaToolProfileData* Profile = FindProfile(Recipe.ItemId);
        const FString TargetKey = FString::Printf(TEXT("%s:%d"), *Recipe.ItemId.ToString(), Recipe.Level);
        const bool bCraftableToolSet = Profile
            && (Profile->ToolSet == TEXT("Starter") || Profile->ToolSet == TEXT("Crafted"));
        const bool bStarterRequiresWorkbench = Profile && Profile->ToolSet == TEXT("Starter")
            && Recipe.RequiredStation != TEXT("Workbench");
        if (Recipe.RecipeId.IsNone() || SeenRecipeIds.Contains(Recipe.RecipeId)
            || Recipe.DisplayName.TrimStartAndEnd().IsEmpty() || Recipe.DisplayName.Len() > 64
            || !bCraftableToolSet || bStarterRequiresWorkbench
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

        if (Recipe.Level != 1 || Recipe.RecipeId != Recipe.ItemId
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
        const FKalmalaToolProfileData* Profile = FindProfile(Upgrade.ItemId);
        const FKalmalaItemDefinition* GearItem = Items->FindItem(Upgrade.ItemId);
        const bool bUpgradeableProfile = Profile
            && IsOneOf(Profile->ToolSet, {TEXT("Starter"), TEXT("Crafted"), TEXT("Utility")});
        const bool bUpgradeableGear = GearItem && !GearItem->GearSlot.IsEmpty();
        if ((!bUpgradeableProfile && !bUpgradeableGear)
            || SeenUpgradeIds.Contains(Upgrade.ItemId)
            || Upgrade.Level != 1 || Upgrade.RequiredStationLevel != 0
            || !IsOneOf(Upgrade.RequiredStation, {TEXT("Workbench"), TEXT("Forge")})
            || Upgrade.RequiredSkill != TEXT("None") || Upgrade.RequiredSkillLevel != 0
            || Upgrade.DisplayName.TrimStartAndEnd().IsEmpty() || Upgrade.DisplayName.Len() > 64
            || Upgrade.Ingredients.IsEmpty() || Upgrade.Ingredients.Num() > MaxIngredientsPerRecipe)
            return false;
        SeenUpgradeIds.Add(Upgrade.ItemId);
        TSet<FName> SeenCostIds;
        for (const FKalmalaToolRecipeIngredientData& Cost : Upgrade.Ingredients)
        {
            if (SeenCostIds.Contains(Cost.ItemId) || !Items->IsValidStack(Cost.ItemId, Cost.Quantity)) return false;
            SeenCostIds.Add(Cost.ItemId);
        }
        const int32 StationTargetCap = FKalmalaToolProgressionContract::MaxStationToolLevel;
        for (int32 CurrentLevel = 1; CurrentLevel < StationTargetCap; ++CurrentLevel)
        {
            TArray<FKalmalaToolRecipeIngredientData> ScaledCosts;
            if (!ScaleUpgradeCosts(Upgrade.Ingredients, CurrentLevel, ScaledCosts)) return false;
            for (const FKalmalaToolRecipeIngredientData& Cost : ScaledCosts)
                if (!Items->FindItem(Cost.ItemId) || Cost.Quantity <= 0) return false;
        }
    }
    if (UpgradeCosts.Num() > MaxRecipes) return false;
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

const FKalmalaToolProfileData* UKalmalaToolCatalogue::FindProfile(const FName ItemId) const
{
    if (ItemId.IsNone()) return nullptr;
    return Profiles.FindByPredicate([ItemId](const FKalmalaToolProfileData& Profile)
    {
        return Profile.ItemId == ItemId;
    });
}

bool FKalmalaToolProfileData::GetLevelStats(const int32 ToolLevel, const FKalmalaToolRewardData& Reward, FKalmalaToolLevelStats& OutStats) const
{
    OutStats = {};
    if (ToolLevel < 1 || ToolLevel > FKalmalaToolLifecycleContract::MaxToolLevel
        || MaxDurability < 1 || MaxDurability > 1000 || !DamageBonuses.IsValid(1000.0f)
        || Reward.MaxDurability < 0 || Reward.MaxDurability > 1000
        || !Reward.DamageBonuses.IsValid(1000.0f)) return false;
    const int64 Durability = int64(MaxDurability) + int64(ToolLevel - 1) * Reward.MaxDurability;
    const FKalmalaDamageChannels Bonuses = DamageBonuses.Added(Reward.DamageBonuses.Scaled(ToolLevel - 1));
    if (Durability > MAX_int32 || !Bonuses.IsValid()) return false;
    OutStats.ToolLevel = ToolLevel;
    OutStats.MaxDurability = static_cast<int32>(Durability);
    OutStats.DamageBonuses = Bonuses;
    return true;
}

const FKalmalaToolRewardData& UKalmalaToolCatalogue::GetLevelUpReward(const FName ItemId) const
{
    const FKalmalaLevelUpRewardGroupData* Group = LevelUpRewards.FindByPredicate(
        [ItemId](const FKalmalaLevelUpRewardGroupData& Entry) { return Entry.ItemIds.Contains(ItemId); });
    static const FKalmalaToolRewardData NoReward;
    return Group ? Group->LevelUpReward : NoReward;
}

bool UKalmalaToolCatalogue::GetLevelStats(const FName ItemId, const int32 ToolLevel, FKalmalaToolLevelStats& OutStats) const
{
    OutStats = {};
    const FKalmalaToolProfileData* Profile = FindProfile(ItemId);
    return Profile && Profile->GetLevelStats(ToolLevel, GetLevelUpReward(ItemId), OutStats);
}

const TArray<FKalmalaWeaponAttackData>& UKalmalaToolCatalogue::GetAttacks(const FName ItemId) const
{
    const FKalmalaWeaponAttackListData* List = AttackLists.FindByPredicate(
        [ItemId](const FKalmalaWeaponAttackListData& Entry) { return Entry.ItemIds.Contains(ItemId); });
    static const TArray<FKalmalaWeaponAttackData> NoAttacks;
    return List ? List->Attacks : NoAttacks;
}
