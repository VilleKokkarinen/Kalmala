#pragma once

#include "KalmalaRegionalTuning.h"
#include "KalmalaMasterMap.h"

/** Temporary commandlet-only experiments. Never replicated or saved as world identity. */
struct FKalmalaGenerationPreviewSettings
{
    uint64 MasterSeed = FKalmalaMasterMap::MasterSeed;
    double MasterWavelength = FKalmalaMasterMap::Wavelength;
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
};

struct KALMALAWORLD_API FKalmalaGenerationPreview
{
    static const FKalmalaGenerationPreviewSettings& Get();
    static uint64 Serial();
    // Refuses installation in game/editor play; only a headless commandlet may tune.
    static bool Set(const FKalmalaGenerationPreviewSettings& Settings);
    static void Reset();
};
