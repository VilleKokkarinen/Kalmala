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
            && InRange(MeadowFloorAmplitude, 10.0, 35.0);
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
