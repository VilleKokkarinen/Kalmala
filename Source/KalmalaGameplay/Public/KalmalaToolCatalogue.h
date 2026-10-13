#pragma once

#include "CoreMinimal.h"
#include "KalmalaToolCatalogue.generated.h"

class UKalmalaItemCatalogue;

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaToolLevelData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    int32 ToolLevel = 0;

    UPROPERTY(EditAnywhere)
    int32 MaxDurability = 0;

    UPROPERTY(EditAnywhere)
    float CombatDamage = 0.0f;
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
    int32 DurabilityCost = 1;

    UPROPERTY(EditAnywhere)
    FString DamageType;

    UPROPERTY(EditAnywhere)
    TArray<FKalmalaToolLevelData> Levels;
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
    int32 TargetToolLevel = 0;

    UPROPERTY(EditAnywhere)
    FName PreviousToolId;

    UPROPERTY(EditAnywhere)
    int32 PreviousToolLevel = 0;

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
    static constexpr int32 MaxRecipes = 32;
    static constexpr int32 MaxIngredientsPerRecipe = 4;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolProfileData> Profiles;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaToolRecipeData> Recipes;

    static const UKalmalaToolCatalogue* Get();
    bool IsValidCatalogue(const UKalmalaItemCatalogue* ItemDefinitions = nullptr) const;
    const FKalmalaToolProfileData* FindProfile(FName ToolId) const;
};
