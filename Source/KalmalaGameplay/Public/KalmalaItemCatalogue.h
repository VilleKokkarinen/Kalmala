#pragma once

#include "CoreMinimal.h"
#include "KalmalaDamageProfile.h"
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
    FString Description;

    UPROPERTY(EditAnywhere)
    int32 MaxStack = 1;

    UPROPERTY(EditAnywhere)
    float WeightKg = 1.0f;

    /** Empty for ordinary items; one of Head, Cape, Chest, Shield, Legs, or Boots for wearable gear. */
    UPROPERTY(EditAnywhere)
    FString GearSlot;

    /** Gear-only condition capacity at level one. Ordinary items keep this at zero. */
    UPROPERTY(EditAnywhere)
    int32 MaxDurability = 0;

    /** Gear-only flat reduction of combined Slash, Crush, Pierce and Stab damage. */
    UPROPERTY(EditAnywhere)
    int32 Armor = 0;
};

struct KALMALAGAMEPLAY_API FKalmalaGearLevelStats
{
    FName ItemId = NAME_None;
    int32 GearLevel = 0;
    int32 MaxDurability = 0;
    int32 Armor = 0;
};

/** JSON-backed game data. Client copies are presentation only, never authority. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaItemCatalogue : public UObject
{
    GENERATED_BODY()

public:
    // Defensive ceilings apply even to misconfigured server content.
    static constexpr int32 MaxDefinitions = 64;
    static constexpr int32 AbsoluteMaxStack = 999;

    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaItemDefinition> Items;

    /** Presentation metadata for placed objects. These are never inventory items. */
    UPROPERTY(EditDefaultsOnly)
    TArray<FKalmalaItemDefinition> Buildables;

    /** Loads and validates the packaged item catalogue before returning the immutable runtime view. */
    static const UKalmalaItemCatalogue* Get();

    bool IsValidCatalogue() const;
    const FKalmalaItemDefinition* FindItem(FName ItemId) const;
    const FKalmalaItemDefinition* FindBuildable(FName BuildableId) const;
    /** Presentation only; inventory operations must use FindItem/IsValidStack. */
    const FKalmalaItemDefinition* FindDefinition(FName Id) const;

    /** Pure validation seam for future server inventory operations; grants nothing. */
    bool IsValidStack(FName ItemId, int32 Quantity) const;

    /** Computes authored condition and defense for a known wearable gear item. */
    bool GetGearLevelStats(FName ItemId, int32 GearLevel, FKalmalaGearLevelStats& OutStats) const;

    /** Rejects malformed existing stacks and overflow without adding untrusted integers. */
    bool CanAddToStack(FName ItemId, int32 ExistingQuantity, int32 AddedQuantity) const;
};
