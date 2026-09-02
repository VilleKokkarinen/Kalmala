#pragma once

#include "KalmalaWorldFieldSampler.h"

enum class EKalmalaBiome : uint8 { Meadows, ShimmeringLakes, Elderwood, MossyMire, FreezingTundra, ThunderMountains, Ocean };

struct KALMALAWORLD_API FKalmalaBiomeClassifier
{
    static EKalmalaBiome Classify(const FKalmalaWorldFieldSample& Field)
    {
        if (Field.Elevation < 0.22f) return EKalmalaBiome::Ocean;
        if (Field.Elevation > 0.78f) return EKalmalaBiome::ThunderMountains;
        if (Field.Temperature < 0.28f) return EKalmalaBiome::FreezingTundra;
        if (Field.Humidity > 0.72f && Field.Elevation < 0.45f) return EKalmalaBiome::MossyMire;
        if (Field.Humidity > 0.63f && Field.Elevation < 0.55f) return EKalmalaBiome::ShimmeringLakes;
        if (Field.Flora > 0.64f) return EKalmalaBiome::Elderwood;
        return EKalmalaBiome::Meadows;
    }
};
