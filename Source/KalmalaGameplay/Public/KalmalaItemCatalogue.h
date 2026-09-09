#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "KalmalaItemCatalogue.generated.h"

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaItemDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName ItemId;

    UPROPERTY(EditAnywhere)
    FString DisplayName;

    UPROPERTY(EditAnywhere)
    int32 MaxStack = 1;
};

/** Server-local game configuration. Client copies are presentation only, never authority. */
UCLASS(Config=Game, DefaultConfig)
class KALMALAGAMEPLAY_API UKalmalaItemCatalogue : public UObject
{
    GENERATED_BODY()

public:
    // Defensive ceilings apply even to misconfigured server content.
    static constexpr int32 MaxDefinitions = 64;
    static constexpr int32 AbsoluteMaxStack = 999;

    UPROPERTY(Config, EditDefaultsOnly)
    TArray<FKalmalaItemDefinition> Items;

    bool IsValidCatalogue() const;
    const FKalmalaItemDefinition* FindItem(FName ItemId) const;

    /** Pure validation seam for future server inventory operations; grants nothing. */
    bool IsValidStack(FName ItemId, int32 Quantity) const;

    /** Rejects malformed existing stacks and overflow without adding untrusted integers. */
    bool CanAddToStack(FName ItemId, int32 ExistingQuantity, int32 AddedQuantity) const;
};
