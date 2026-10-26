#pragma once

#include "KalmalaRegionalTuning.h"
#include "KalmalaMasterMap.h"

/** Temporary commandlet-only experiments. Never replicated or saved as world identity. */
struct FKalmalaGenerationPreviewSettings
{
    uint64 MasterSeed = FKalmalaMasterMap::MasterSeed;
    double MasterWavelength = FKalmalaMasterMap::Wavelength;
    // Commandlet-only terrain controls, centimetres. Not-yet-integrated layers
    // remain preview-only until their corresponding sampler work is complete.
    double ReliefBroadScale = FKalmalaRegionalTuning::ReliefBroadScale;
    double ReliefMiddleScale = FKalmalaRegionalTuning::ReliefMiddleScale;
    double ReliefLocalScale = FKalmalaRegionalTuning::ReliefLocalScale;
    double MeadowHillScale = FKalmalaRegionalTuning::MeadowHillScale;
    double MeadowHillAmplitude = FKalmalaRegionalTuning::MeadowHillAmplitude;
    double MeadowShoulderHeight = FKalmalaRegionalTuning::MeadowShoulderHeight;
    double MeadowShoulderCompressionScale = FKalmalaRegionalTuning::MeadowShoulderCompressionScale;
    double MeadowRollingScale = FKalmalaRegionalTuning::MeadowRollingScale;
    double MeadowRollingAmplitude = FKalmalaRegionalTuning::MeadowRollingAmplitude;
    double MeadowFloorScale = FKalmalaRegionalTuning::MeadowFloorScale;
    double MeadowFloorAmplitude = FKalmalaRegionalTuning::MeadowFloorAmplitude;
    double ElderwoodBroadAmplitude = FKalmalaRegionalTuning::ElderwoodBroadAmplitude;
    double ElderwoodMiddleAmplitude = FKalmalaRegionalTuning::ElderwoodMiddleAmplitude;
    double ElderwoodLocalAmplitude = FKalmalaRegionalTuning::ElderwoodLocalAmplitude;
    double ShimmeringLakesMeadowShare = FKalmalaRegionalTuning::ShimmeringLakesMeadowShare;
    double ShimmeringLakesLowlandFloor = FKalmalaRegionalTuning::ShimmeringLakesLowlandFloor;
    double ShimmeringLakesLowlandCeiling = FKalmalaRegionalTuning::ShimmeringLakesLowlandCeiling;
    double ShimmeringLakesLowlandCompression = FKalmalaRegionalTuning::ShimmeringLakesLowlandCompression;
    double MireTargetHeight = FKalmalaRegionalTuning::MireTargetHeight;
    double MireCompressionScale = FKalmalaRegionalTuning::MireCompressionScale;
    double MireResidualDetailAmplitude = FKalmalaRegionalTuning::MireResidualDetailAmplitude;
    double MountainHighlandThreshold = FKalmalaRegionalTuning::MountainHighlandThreshold;
    double MountainHighlandBlendRange = FKalmalaRegionalTuning::MountainHighlandBlendRange;
    double MountainHighlandGain = FKalmalaRegionalTuning::MountainHighlandGain;
    double MountainDetailAmplitude = FKalmalaRegionalTuning::MountainDetailAmplitude;
    double LandThreshold = 0;
    double BiomeScale = FKalmalaRegionalTuning::BiomeScale;
    double WarpStrength = FKalmalaRegionalTuning::WarpStrength;
    double StarterRadius = FKalmalaRegionalTuning::StarterRadius;
    double ElderwoodMinimum = FKalmalaRegionalTuning::ElderwoodMinimum;
    double LakesMinimum = FKalmalaRegionalTuning::LakesMinimum;
    double LakesMaximum = FKalmalaRegionalTuning::LakesMaximum;
    double MireMinimum = FKalmalaRegionalTuning::MireMinimum;
    double MireMaximum = FKalmalaRegionalTuning::MireMaximum;
    double WetlandHumidityMinimum = FKalmalaRegionalTuning::WetlandHumidityMinimum;
    double WetlandHumidityFull = FKalmalaRegionalTuning::WetlandHumidityFull;
    double WetlandElevationFull = FKalmalaRegionalTuning::WetlandElevationFull;
    double WetlandElevationMaximum = FKalmalaRegionalTuning::WetlandElevationMaximum;
    double WetlandTemperatureMinimum = FKalmalaRegionalTuning::WetlandTemperatureMinimum;
    double WetlandTemperatureFull = FKalmalaRegionalTuning::WetlandTemperatureFull;
    double TundraMinimum = FKalmalaRegionalTuning::TundraMinimum;
    double MeadowsMaximum = FKalmalaRegionalTuning::MeadowsMaximum;
    double EligibilityBlend = FKalmalaRegionalTuning::EligibilityBlend;

    bool HasValidReliefControls() const
    {
        const auto InRange = [](double Value, double Minimum, double Maximum)
        {
            return FMath::IsFinite(Value) && Value >= Minimum && Value <= Maximum;
        };
        return InRange(ReliefBroadScale, 50000.0, 150000.0)
            && InRange(ReliefMiddleScale, 8000.0, 25000.0)
            && InRange(ReliefLocalScale, 1000.0, 5000.0)
            && InRange(MeadowHillScale, 10000.0, 25000.0)
            && InRange(MeadowHillAmplitude, 400.0, 1000.0)
            && InRange(MeadowShoulderHeight, 600.0, 1600.0)
            && InRange(MeadowShoulderCompressionScale, 1000.0, 6000.0)
            && InRange(MeadowRollingScale, 3000.0, 7000.0)
            && InRange(MeadowRollingAmplitude, 60.0, 200.0)
            && InRange(MeadowFloorScale, 800.0, 2000.0)
            && InRange(MeadowFloorAmplitude, 10.0, 35.0)
            && InRange(ElderwoodBroadAmplitude, 400.0, 1600.0)
            && InRange(ElderwoodMiddleAmplitude, 60.0, 400.0)
            && InRange(ElderwoodLocalAmplitude, 10.0, 100.0)
            && InRange(ShimmeringLakesMeadowShare, 0.35, 0.65)
            && InRange(ShimmeringLakesLowlandFloor, 100.0, 500.0)
            && InRange(ShimmeringLakesLowlandCeiling, 500.0, 2000.0)
            && ShimmeringLakesLowlandCeiling > ShimmeringLakesLowlandFloor
            && InRange(ShimmeringLakesLowlandCompression, 0.05, 0.35)
            && InRange(MireTargetHeight, 200.0, 500.0)
            && InRange(MireCompressionScale, 300.0, 1200.0)
            && InRange(MireResidualDetailAmplitude, 10.0, 60.0)
            && MireTargetHeight * MireCompressionScale / (MireTargetHeight + MireCompressionScale)
                > MireResidualDetailAmplitude
            && InRange(MountainHighlandThreshold, 500.0, 2000.0)
            && InRange(MountainHighlandBlendRange, 500.0, 2000.0)
            && InRange(MountainHighlandGain, 0.1, 0.8)
            && InRange(MountainDetailAmplitude, 40.0, 160.0);
    }
};

struct KALMALAWORLD_API FKalmalaGenerationPreview
{
    static const FKalmalaGenerationPreviewSettings& Get();
    static uint64 Serial();
    // Refuses installation in game/editor play; only a headless commandlet may tune.
    static bool Set(const FKalmalaGenerationPreviewSettings& Settings);
    static void Reset();
};
