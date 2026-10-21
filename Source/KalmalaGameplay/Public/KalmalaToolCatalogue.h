#pragma once

#include "CoreMinimal.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolCatalogue.generated.h"

class UKalmalaItemCatalogue;

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaToolRewardData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32 MaxDurability = 0;

    UPROPERTY(EditAnywhere)
    FKalmalaDamageChannels DamageBonuses;

    UPROPERTY(EditAnywhere)
    int32 Armor = 0;
};

/** One shared per-level reward for an explicit non-overlapping group of item IDs. */
USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaLevelUpRewardGroupData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TArray<FName> ItemIds;

    UPROPERTY(EditAnywhere)
    FKalmalaToolRewardData LevelUpReward;
};

/** Shared attack styles for all tier identities of a weapon family. */
USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaWeaponAttackListData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName AttackListId;

    UPROPERTY(EditAnywhere)
    TArray<FName> ItemIds;

    UPROPERTY(EditAnywhere)
    TArray<FKalmalaWeaponAttackData> Attacks;
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaToolProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName ItemId;

    UPROPERTY(EditAnywhere)
    FString DisplayName;

    UPROPERTY(EditAnywhere)
    FString Description;

    /** Two-handed items exclude the Shield slot while wielded. */
    UPROPERTY(EditAnywhere)
    bool bTwoHanded = false;

    UPROPERTY(EditAnywhere)
    FString ToolSet;

    UPROPERTY(EditAnywhere)
    FString ToolKind;

    UPROPERTY(EditAnywhere)
    FString Action;

    UPROPERTY(EditAnywhere)
    FString RequiredSkill;

    UPROPERTY(EditAnywhere)
    int32 MinimumSkillLevel = 1;

    /** Maximum resource material tier; required for axes/pickaxes, zero for other profiles. */
    UPROPERTY(EditAnywhere)
    int32 MaterialTier = 0;
    /** Seconds to full draw; zero for non-bows. */
    UPROPERTY(EditAnywhere) float DrawDuration = 0.0f;

    /** Presentation weight used by the inventory's carried-weight summary. */
    UPROPERTY(EditAnywhere) float WeightKg = 2.0f;

    UPROPERTY(EditAnywhere)
    int32 MaxDurability = 0;

    UPROPERTY(EditAnywhere)
    FKalmalaDamageChannels DamageBonuses;

    /** Classification follows the validated item family, without an authored category field. */
    bool IsWeapon() const
    {
        return ToolKind.EndsWith(TEXT("Knife")) || ToolKind.EndsWith(TEXT("Axe"))
            || ToolKind.EndsWith(TEXT("Mace")) || ToolKind.EndsWith(TEXT("Spear"))
            || ToolKind.EndsWith(TEXT("Sword")) || ToolKind.EndsWith(TEXT("Club"))
            || ToolKind == TEXT("Bow") || ToolKind == TEXT("LightBow");
    }

    bool GetLevelStats(int32 ToolLevel, const FKalmalaToolRewardData& Reward, FKalmalaToolLevelStats& OutStats) const;
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaToolRecipeIngredientData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName ItemId;

    UPROPERTY(EditAnywhere)
    int32 Quantity = 0;
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaToolRecipeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName RecipeId;

    UPROPERTY(EditAnywhere)
    FString DisplayName;

    UPROPERTY(EditAnywhere)
    FName ItemId;

    UPROPERTY(EditAnywhere)
    int32 Level = 1;

    UPROPERTY(EditAnywhere)
    FString RequiredStation;

    UPROPERTY(EditAnywhere)
    int32 RequiredStationLevel = 0;

    UPROPERTY(EditAnywhere)
    FString RequiredSkill;

    UPROPERTY(EditAnywhere)
    int32 RequiredSkillLevel = 0;

    UPROPERTY(EditAnywhere)
    TArray<FKalmalaToolRecipeIngredientData> Ingredients;
};

/** JSON-backed carried-item stats, wearable rewards, and station acquisition/upgrade costs. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaToolCatalogue : public UObject
{
    GENERATED_BODY()

public:
    static constexpr int32 MaxLevels = FKalmalaToolLifecycleContract::MaxToolLevel;
    static constexpr int32 MaxIngredientsPerRecipe = 4;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolProfileData> Profiles;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolRecipeData> Recipes;

    /** Level-one upgrade costs; later costs are derived, never separately authored. */
    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolRecipeData> UpgradeCosts;

    /** Shared item-group gains, authored separately from individual profiles. */
    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaLevelUpRewardGroupData> LevelUpRewards;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaWeaponAttackListData> AttackLists;

    const TArray<FKalmalaWeaponAttackData>& GetAttacks(FName ItemId) const;
    static bool ScaleUpgradeCosts(const TArray<FKalmalaToolRecipeIngredientData>& BaseCosts,
        int32 CurrentLevel, TArray<FKalmalaToolRecipeIngredientData>& OutCosts);
    const FKalmalaToolRewardData& GetLevelUpReward(FName ItemId) const;
    bool GetLevelStats(FName ItemId, int32 ToolLevel, FKalmalaToolLevelStats& OutStats) const;
    static const UKalmalaToolCatalogue* Get();
    bool IsValidCatalogue(const UKalmalaItemCatalogue* ItemDefinitions = nullptr) const;
    const FKalmalaToolProfileData* FindProfile(FName ItemId) const;
};
