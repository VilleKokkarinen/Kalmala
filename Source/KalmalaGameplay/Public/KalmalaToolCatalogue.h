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
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaToolProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName ToolId;

    UPROPERTY(EditAnywhere)
    FString DisplayName;

    UPROPERTY(EditAnywhere)
    FString Description;

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

    UPROPERTY(EditAnywhere)
    TArray<FKalmalaWeaponAttackData> Attacks;

    UPROPERTY(EditAnywhere)
    int32 MaxDurability = 0;

    UPROPERTY(EditAnywhere)
    FKalmalaDamageChannels DamageBonuses;

    /** Flat gains added for each level after level one. */
    UPROPERTY(EditAnywhere)
    FKalmalaToolRewardData LevelUpReward;

    bool GetLevelStats(int32 ToolLevel, FKalmalaToolLevelStats& OutStats) const;
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
    FName ToolId;

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

/** JSON-backed authored tool stats and station acquisition/upgrade recipes. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaToolCatalogue : public UObject
{
    GENERATED_BODY()

public:
    static constexpr int32 MaxProfiles = 32;
    static constexpr int32 MaxLevels = FKalmalaToolLifecycleContract::MaxToolLevel;
    static constexpr int32 MaxRecipes = 32;
    static constexpr int32 MaxIngredientsPerRecipe = 4;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolProfileData> Profiles;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolRecipeData> Recipes;

    /** Level-one upgrade costs; later costs are derived, never separately authored. */
    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolRecipeData> UpgradeCosts;

    static bool ScaleUpgradeCosts(const TArray<FKalmalaToolRecipeIngredientData>& BaseCosts,
        int32 CurrentLevel, TArray<FKalmalaToolRecipeIngredientData>& OutCosts);
    static const UKalmalaToolCatalogue* Get();
    bool IsValidCatalogue(const UKalmalaItemCatalogue* ItemDefinitions = nullptr) const;
    const FKalmalaToolProfileData* FindProfile(FName ToolId) const;
};
