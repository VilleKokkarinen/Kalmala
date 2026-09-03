#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

/** Selects a deterministic, seed-derived initial player position without authored routes. */
struct KALMALAWORLD_API FKalmalaWorldPlayerStartResolver
{
    static FTransform ResolveStartTransform(const FKalmalaWorldGenerationConfig& Config);
};
