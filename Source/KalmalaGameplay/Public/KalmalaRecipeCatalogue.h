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
    /** Item produced by a normal inventory recipe. Empty for direct construction recipes. */
    UPROPERTY(EditAnywhere) FName Output;
    /** Construction descriptor placed directly from Ingredients; never an inventory output. */
    UPROPERTY(EditAnywhere) FName BuildableOutput;
    UPROPERTY(EditAnywhere) int32 OutputCount = 1;
    UPROPERTY(EditAnywhere) int32 MaxBatch = 1;
    /** Any one of these visible nearby stations can satisfy the recipe. */
    UPROPERTY(EditAnywhere) TArray<FName> RequiredStation;
    /** Optional reusable item that must be present in the server-owned inventory. */
    UPROPERTY(EditAnywhere) FName RequiredTool;
    UPROPERTY(EditAnywhere) EKalmalaSkill ExperienceSkill = EKalmalaSkill::None;
    UPROPERTY(EditAnywhere) int32 ExperienceAward = 0;
    UPROPERTY(EditAnywhere) bool bEnabled = true;

    FName GetOutputIdentity() const
    {
        return BuildableOutput.IsNone() ? Output : BuildableOutput;
    }
};

UCLASS()
class KALMALAGAMEPLAY_API UKalmalaRecipeCatalogue : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly) TArray<FKalmalaRecipe> Recipes;
    /** Loads and validates the packaged recipe catalogue before returning the immutable runtime view. */
    static const UKalmalaRecipeCatalogue* Get();
    bool IsValidCatalogue(const class UKalmalaItemCatalogue* ItemDefinitions = nullptr) const;
    const FKalmalaRecipe* Find(FName Id) const;
    static bool HasRequiredTool(FName ToolId, const TArray<FKalmalaInventoryStack>& Stacks);
    static bool IsDirectMaterialBuildable(FName BuildableId);
    static bool BuildDirectMaterialCost(FName BuildableId,
        TArray<FKalmalaInventoryStack>& OutCosts, FString& Reason);
    static bool Scale(const FKalmalaRecipe& Recipe, int32 Batch,
        TArray<FKalmalaInventoryStack>& Costs, int32& OutputCount,
        const class UKalmalaItemCatalogue* ItemDefinitions = nullptr);
};
