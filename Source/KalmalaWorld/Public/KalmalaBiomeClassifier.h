#pragma once

#include "KalmalaWorldFieldSampler.h"
#include "KalmalaTerrainHeightSampler.h"

enum class EKalmalaBiome : uint8 { Meadows, ShimmeringLakes, Elderwood, MossyMire, FreezingTundra, ThunderMountains, Ocean };

struct KALMALAWORLD_API FKalmalaBiomeClassifier
{
    /** Dominant biome from normalized continuous fields; does not model slope or basin connectivity. */
    static EKalmalaBiome Classify(const FKalmalaWorldFieldSample& Field)
    {
        // Preserve the biome layout (and derived gameplay placements) of existing worlds.
        if (Field.GeneratorRevision == 1)
        {
            return ClassifyRevisionOne(Field);
        }

        // Physical terrain takes precedence over climate and vegetation.
        if (Field.Elevation < FKalmalaTerrainHeightSampler::SeaLevelElevation)
        {
            return EKalmalaBiome::Ocean;
        }
        if (Field.Elevation > MountainElevation)
        {
            return EKalmalaBiome::ThunderMountains;
        }

        // Tundra occupies cold uplands; a cold coastal sample is not an alpine biome.
        if (Field.Elevation >= UplandElevation && Field.Temperature < TundraTemperature)
        {
            return EKalmalaBiome::FreezingTundra;
        }

        // Saturated, temperate lowlands form mire. Lakes also occur in cold lowlands;
        // the separate basin query determines whether actual standing water exists.
        if (Field.Elevation < MireElevation && Field.Humidity > MireHumidity
            && Field.Temperature >= MireMinimumTemperature)
        {
            return EKalmalaBiome::MossyMire;
        }
        if (Field.Elevation < UplandElevation && Field.Humidity > LakeHumidity)
        {
            return EKalmalaBiome::ShimmeringLakes;
        }

        // Dense vegetation needs moisture support; open ground remains Meadows.
        if (Field.Flora > ForestFlora && Field.Humidity >= ForestMinimumHumidity)
        {
            return EKalmalaBiome::Elderwood;
        }
        return EKalmalaBiome::Meadows;
    }

private:
    static constexpr float MountainElevation = 0.78f;
    static constexpr float UplandElevation = 0.55f;
    static constexpr float TundraTemperature = 0.35f;
    static constexpr float MireElevation = 0.45f;
    static constexpr float MireHumidity = 0.72f;
    static constexpr float MireMinimumTemperature = 0.28f;
    static constexpr float LakeHumidity = 0.63f;
    static constexpr float ForestFlora = 0.64f;
    static constexpr float ForestMinimumHumidity = 0.35f;

    static EKalmalaBiome ClassifyRevisionOne(const FKalmalaWorldFieldSample& Field)
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
