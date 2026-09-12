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
    double MireMinimum = FKalmalaRegionalTuning::MireMinimum;
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
