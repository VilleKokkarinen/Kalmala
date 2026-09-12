#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.generated.h"

/**
 * Server-owned seed. Development always uses the current compiled generator.
 */
USTRUCT()
struct KALMALAWORLD_API FKalmalaWorldGenerationConfig
{
    GENERATED_BODY()

    /** Server-generated 64-bit base seed. */
    UPROPERTY(EditAnywhere, Category = "World Generation")
    uint64 WorldSeed = 0;

    bool IsValid() const
    {
        return true; // Every uint64 seed, including zero, is supported.
    }

    bool operator==(const FKalmalaWorldGenerationConfig& Other) const
    {
        return WorldSeed == Other.WorldSeed;
    }
};
