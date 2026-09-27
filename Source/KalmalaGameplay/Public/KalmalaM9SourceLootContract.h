#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

/** Server-only M9 source reward and sparse identity rules. */
class KALMALAGAMEPLAY_API FKalmalaM9SourceLootContract
{
public:
    static constexpr int32 PrimaryYield = 1;
    static constexpr int32 MaximumYield = 2;
    static constexpr uint64 BonusDivisor = 4;

    static bool IsM9Source(FName SourceId);
    static bool GetPrimaryItemId(FName SourceId, FName& OutItemId);
    static uint64 HashAscii(const FString& Value);
    static bool BuildHarvestRewardQuantity(const FKalmalaWorldGenerationConfig& WorldConfig,
        FName SourceId, const FString& PopulationSpawnId, int32& OutQuantity);
    static FString BuildResourceDepletionId(FName SourceId, const FString& PopulationSpawnId);
};
