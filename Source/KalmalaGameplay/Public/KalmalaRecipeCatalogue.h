#pragma once
#include "CoreMinimal.h"
#include "KalmalaInventoryComponent.h"
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
    UPROPERTY(EditAnywhere) bool bRequiresCampfire = false;
    UPROPERTY(EditAnywhere) bool bEnabled = true;
};

UCLASS(Config=Game, DefaultConfig)
class KALMALAGAMEPLAY_API UKalmalaRecipeCatalogue : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditDefaultsOnly) TArray<FKalmalaRecipe> Recipes;
    bool IsValidCatalogue() const;
    const FKalmalaRecipe* Find(FName Id) const;
    static bool Scale(const FKalmalaRecipe& Recipe, int32 Batch,
        TArray<FKalmalaInventoryStack>& Costs, int32& OutputCount);
};
