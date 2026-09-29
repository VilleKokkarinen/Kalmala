#pragma once
#include "CoreMinimal.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaRecipeCatalogue.generated.h"

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaRecipe
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName RecipeId;
    UPROPERTY(EditAnywhere) FString DisplayName;
    UPROPERTY(EditAnywhere) TArray<FKalmalaInventoryStack> Ingredients;
    UPROPERTY(EditAnywhere) FName Output;
    UPROPERTY(EditAnywhere) int32 OutputCount = 1;
    UPROPERTY(EditAnywhere) int32 MaxBatch = 1;
    /** Any one of these visible nearby stations can satisfy the recipe. */
    UPROPERTY(EditAnywhere) TArray<FName> RequiredStation;
    UPROPERTY(EditAnywhere) EKalmalaSkill ExperienceSkill = EKalmalaSkill::None;
    UPROPERTY(EditAnywhere) int32 ExperienceAward = 0;
    UPROPERTY(EditAnywhere) bool bEnabled = true;
};

UCLASS()
class KALMALAGAMEPLAY_API UKalmalaRecipeCatalogue : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) TArray<FKalmalaRecipe> Recipes;
    /** Loads and validates the packaged recipe catalogue before returning the immutable runtime view. */
    static const UKalmalaRecipeCatalogue* Get();
    bool IsValidCatalogue() const;
    const FKalmalaRecipe* Find(FName Id) const;
    static bool IsDirectMaterialBuildable(FName BuildableId);
    static bool BuildDirectMaterialCost(FName BuildableId,
        TArray<FKalmalaInventoryStack>& OutCosts, FString& Reason);
    static bool Scale(const FKalmalaRecipe& Recipe, int32 Batch,
        TArray<FKalmalaInventoryStack>& Costs, int32& OutputCount);
};
