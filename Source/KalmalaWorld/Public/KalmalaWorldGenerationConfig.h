#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.generated.h"

/**
 * Immutable identity of a generated base world. The server creates and persists
 * this pair once; changing either value requires a separate world/save.
 */
USTRUCT()
struct KALMALAWORLD_API FKalmalaWorldGenerationConfig
{
    GENERATED_BODY()

    /** Server-generated 64-bit base seed. */
    UPROPERTY(EditAnywhere, Category = "World Generation")
    uint64 WorldSeed = 0;

    /** Version of generation rules used to interpret WorldSeed. */
    UPROPERTY(EditAnywhere, Category = "World Generation", meta = (ClampMin = "1"))
    int32 GeneratorRevision = 1;

    bool IsValid() const
    {
        return GeneratorRevision > 0;
    }

    bool operator==(const FKalmalaWorldGenerationConfig& Other) const
    {
        return WorldSeed == Other.WorldSeed && GeneratorRevision == Other.GeneratorRevision;
    }
};
