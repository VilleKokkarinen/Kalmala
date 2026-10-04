#pragma once
#include "CoreMinimal.h"
struct FKalmalaRecipe;
class UKalmalaInventoryComponent;

/** Local text over existing catalogue and supplied owning-player state. */
class KALMALAUI_API FKalmalaRecipeRequirements
{
public:
    static FString Describe(const FKalmalaRecipe& Recipe,
        const UKalmalaInventoryComponent* Inventory, int32 CarriedHammerLevel,
        const FString& Availability);
};
