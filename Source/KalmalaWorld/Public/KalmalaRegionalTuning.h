#pragma once

#include "CoreMinimal.h"

/** Current developer tuning, in Unreal centimetres. Never client-controlled. */
struct FKalmalaRegionalTuning
{
    static constexpr double BiomeScale = 60000.0;
    // Biome-height experiments in centimetres. Shared relief scales and salts
    // feed derived terrain candidates; Meadows controls feed their wired layers.
    static constexpr double ReliefBroadScale = 100000.0;
    static constexpr double ReliefMiddleScale = 15000.0;
    static constexpr double ReliefLocalScale = 3000.0;
    // Stable, independent domains for the derived terrain-relief stack.
    static constexpr uint64 ReliefBroadSalt = 0x4B414C4D414C4101ull;
    static constexpr uint64 ReliefMiddleSalt = 0x4B414C4D414C4102ull;
    static constexpr uint64 ReliefLocalSalt = 0x4B414C4D414C4103ull;
    static constexpr uint64 MeadowHillSalt = 0x4B414C4D414C4104ull;
    static constexpr uint64 MeadowRollingSalt = 0x4B414C4D414C4105ull;
    static constexpr uint64 MeadowFloorSalt = 0x4B414C4D414C4106ull;
    static constexpr double MeadowHillScale = 16000.0;
    static constexpr double MeadowHillAmplitude = 600.0;
    static constexpr double MeadowShoulderHeight = 1000.0;
    static constexpr double MeadowShoulderCompressionScale = 3000.0;
    static constexpr double MeadowRollingScale = 4500.0;
    static constexpr double MeadowRollingAmplitude = 120.0;
    static constexpr double MeadowFloorScale = 1200.0;
    static constexpr double MeadowFloorAmplitude = 20.0;
    // Provisional Elderwood/Tundra shared-relief trials, centimetres.
    static constexpr double ElderwoodBroadAmplitude = 900.0;
    static constexpr double ElderwoodMiddleAmplitude = 240.0;
    static constexpr double ElderwoodLocalAmplitude = 40.0;
    // Provisional Shimmering Lakes shoulder/low-pocket trials, centimetres
    // except for the unitless blend share and compression fraction.
    static constexpr double ShimmeringLakesMeadowShare = 0.50;
    static constexpr double ShimmeringLakesLowlandFloor = 200.0;
    static constexpr double ShimmeringLakesLowlandCeiling = 1200.0;
    static constexpr double ShimmeringLakesLowlandCompression = 0.20;
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
    static constexpr double RiverBedDepth = 80.0; // Positive depth below water, cm.
    static constexpr double RiverBankExtent = 1800.0; // Bank shoulder beyond channel width, cm.
    static constexpr double SplineAmplitude = 1200.0;
    static constexpr double SplineWavelength = 10000.0;
    static constexpr double SplineStep = 250.0;
    static constexpr double GridCell = 6000.0;
    static constexpr int32 HydrologyCacheCells = 256;
    static constexpr double BasinSpacing = 40000.0;
    static constexpr bool bEnableAnalyticBasins = false;
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
