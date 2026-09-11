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

    static constexpr int32 CurrentGeneratorRevision = 5;
    // Opt-in debug layout; replicated as identity so stream carving always agrees.
    static constexpr int32 StreamsDebugGeneratorRevision = 6;

    /** Server-generated 64-bit base seed. */
    UPROPERTY(EditAnywhere, Category = "World Generation")
    uint64 WorldSeed = 0;

    /** Version of generation rules used to interpret WorldSeed. */
    UPROPERTY(EditAnywhere, Category = "World Generation", meta = (ClampMin = "1"))
    // Keep the serialized default for revision-one saves; new-world entry points
    // explicitly select CurrentGeneratorRevision.
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
