#pragma once

#include "KalmalaWorldFieldSampler.h"
#include "KalmalaRegionalGeneration.h"

enum class EKalmalaBiome : uint8 { Meadows, ShimmeringLakes, Elderwood, MossyMire, FreezingTundra, ThunderMountains, Ocean };

struct KALMALAWORLD_API FKalmalaBiomeClassifier
{
    static EKalmalaBiome Classify(const FKalmalaWorldFieldSample& Field)
    {
        return static_cast<EKalmalaBiome>(FKalmalaRegionalGeneration::SampleBiome(Field));
    }
};
