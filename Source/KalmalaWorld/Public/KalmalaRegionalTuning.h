#pragma once

#include "CoreMinimal.h"

/** Current developer tuning, in Unreal centimetres. Never client-controlled. */
struct FKalmalaRegionalTuning
{
    static constexpr double BiomeScale = 60000.0;
    static constexpr double ElevationFrequency = 1.0 / 100000.0;
    static constexpr double ClimateFrequency = 1.0 / 150000.0;
    static constexpr double LocalReliefFrequency = 0.00035;
    static constexpr double FloraFrequency = 0.00075;
    static constexpr float ElevationAmplitude = 0.85f;
    static constexpr float ClimateAmplitude = 0.8f;
    static constexpr float LocalReliefAmplitude = 0.015f;
    static constexpr double RegionFrequency = 1.0 / BiomeScale;
    static constexpr double WarpFrequency = 1.0 / 120000.0;
    static constexpr double WarpStrength = 8000.0;
    static constexpr double EdgeWaveStrength = 0.12;
    static constexpr double RingOverlap = 0.22;
    static constexpr double RiverSpacing = 18000.0;
    static constexpr double MergeDistance = 5000.0;
    static constexpr double RiverRange = 28000.0;
    static constexpr double SplineAmplitude = 1200.0;
    static constexpr double SplineWavelength = 10000.0;
    static constexpr double SplineStep = 250.0;
    static constexpr double GridCell = 6000.0;
    static constexpr int32 HydrologyCacheCells = 256;
    static constexpr double BasinSpacing = 40000.0;
    static constexpr float SeaElevation = 0.22f;
    static constexpr float MountainElevation = 0.78f;
    // Hard eligibility distances from world zero, centimetres.
    static constexpr double StarterRadius = 35000.0;
    static constexpr double ElderwoodMinimum = 75000.0;
    static constexpr double LakesMinimum = 35000.0;
    static constexpr double LakesMaximum = 300000.0;
    static constexpr double MireMinimum = 300000.0;
    static constexpr double MireMaximum = 1600000.0;
    static constexpr double WetlandHumidityMinimum = 0.48;
    static constexpr double WetlandHumidityFull = 0.65;
    static constexpr double WetlandElevationFull = 0.43;
    static constexpr double WetlandElevationMaximum = 0.58;
    static constexpr double WetlandTemperatureMinimum = 0.18;
    static constexpr double WetlandTemperatureFull = 0.32;
    static constexpr double TundraMinimum = 400000.0;
    static constexpr double MeadowsMaximum = 400000.0;
    static constexpr double EligibilityBlend = 5000.0;
};
