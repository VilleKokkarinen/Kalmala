#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

class USaveGame;

/** A missing component is not evidence that a world is empty or absent. */
enum class EKalmalaWorldSaveComponentStatus : uint8 { Absent, Accepted, Rejected };

struct KALMALAGAMEPLAY_API FKalmalaWorldSaveInputs
{
    bool bPopulationExists = false;
    USaveGame* Population = nullptr;
    bool bConstructionExists = false;
    USaveGame* Construction = nullptr;
    bool bStorageExists = false;
    USaveGame* Storage = nullptr;
    bool bOceanExists = false;
    USaveGame* Ocean = nullptr;
};

/** Component validation only, not a complete world or player-save compatibility decision. */
struct KALMALAGAMEPLAY_API FKalmalaWorldSaveInspection
{
    uint64 Seed = 0;
    EKalmalaWorldSaveComponentStatus Population = EKalmalaWorldSaveComponentStatus::Absent;
    EKalmalaWorldSaveComponentStatus Construction = EKalmalaWorldSaveComponentStatus::Absent;
    EKalmalaWorldSaveComponentStatus Storage = EKalmalaWorldSaveComponentStatus::Absent;
    EKalmalaWorldSaveComponentStatus Ocean = EKalmalaWorldSaveComponentStatus::Absent;
};

/** Read-only inspection of the existing world-scoped slots for an explicitly supplied seed. */
class KALMALAGAMEPLAY_API FKalmalaWorldSaveInspector
{
public:
    static FKalmalaWorldSaveInspection Inspect(uint64 Seed);
    // Also used by in-memory tests. Existing-but-null or wrong-type data fails closed.
    static FKalmalaWorldSaveInspection Evaluate(uint64 Seed, const FKalmalaWorldSaveInputs& Inputs);
};
