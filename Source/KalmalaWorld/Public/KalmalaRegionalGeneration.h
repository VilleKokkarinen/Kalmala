#pragma once

#include "KalmalaWorldFieldSampler.h"

struct FKalmalaRegionalSample
{
    // Same stable order as EKalmalaBiome. Normalized and deterministic tie-breaking.
    float Weights[7] = {};
    uint8 Biome = 0;
    float Height = 0;
    float WaterLevel = 0;
    float RiverWeight = 0;
    float StreamWeight = 0;
    float BasinWeight = 0;
    bool bHasWater = false;
};

struct FKalmalaHydrologySegment
{
    FVector A = FVector::ZeroVector;
    FVector B = FVector::ZeroVector;
    float Width = 0;
    bool bStream = false;
    uint64 Id = 0;
};

/** Signed normalized samples from independent, absolute-world terrain-noise domains. */
struct FKalmalaTerrainReliefSample
{
    double Broad = 0.0;
    double Middle = 0.0;
    double Local = 0.0;
};

/** Raw deterministic height candidates exposed for commandlet profile diagnostics. */
struct FKalmalaTerrainCandidateDiagnostics
{
    FKalmalaTerrainReliefSample SharedRelief;
    double SourceHeight = 0.0;
    double MeadowCandidate = 0.0;
    double ElderwoodCandidate = 0.0;
    double TundraCandidate = 0.0;
    double ShimmeringLakesLowlandCandidate = 0.0;
    double ShimmeringLakesCandidate = 0.0;
    double MireCandidate = 0.0;
    double WetlandHumidityMinimum = 0.0;
    double WetlandHumidityFull = 0.0;
};

/** Pure regions/shaping plus a bounded, disposable spline-only spatial cache. */
struct KALMALAWORLD_API FKalmalaRegionalGeneration
{
    static FKalmalaRegionalSample Sample(const FKalmalaWorldGenerationConfig& Config, FVector2D Position);
    static FKalmalaRegionalSample Sample(const FKalmalaWorldFieldSample& Fields);
    /** Exact current biome identity, excluding hydrology/height-only work. */
    static uint8 SampleBiome(const FKalmalaWorldFieldSample& Fields);
    static TArray<FKalmalaHydrologySegment> GetHydrology(const FKalmalaWorldGenerationConfig& Config, FIntPoint GridCell);
    static void ClearHydrologyCache();
    static bool AreStreamsEnabled(const FKalmalaWorldGenerationConfig& C)
    { return false; }
    static uint64 Seed(const FKalmalaWorldGenerationConfig& Config, uint64 Domain, FIntPoint Cell = FIntPoint::ZeroValue);
    static double Noise(const FKalmalaWorldGenerationConfig& Config, uint64 Domain, FVector2D Position, double Frequency);
    /** Samples shared relief layers without applying them to ecological fields or terrain height. */
    static FKalmalaTerrainReliefSample SampleSharedRelief(
        const FKalmalaWorldGenerationConfig& Config, FVector2D AbsoluteWorldPosition);
    /** Samples exact production candidates without changing biome identity or terrain. */
    static FKalmalaTerrainCandidateDiagnostics SampleTerrainCandidateDiagnostics(
        const FKalmalaWorldGenerationConfig& Config, const FKalmalaWorldFieldSample& Fields);
private:
    friend class FKalmalaBiomeHeightFunctionTest;
    friend class FKalmalaMossyMireShapeTest;
    friend class FKalmalaBiomeHeightBlendTest;
    friend class FKalmalaShimmeringLakesShapeTest;
    friend class FKalmalaMeadowShoulderCompressionTest;
    friend class FKalmalaMeadowFloorDetailTest;
    friend class FKalmalaMasterMapTest;
    friend class FKalmalaRiverBankCrossSectionTest;
    friend class FKalmalaRiverUnchangedControlsTest;
    // Named candidates keep the existing land-biome shape behavior explicit.
    static double ShapeMeadows(double SourceHeight, double Detail);
    static double ShapeMeadows(double SourceHeight, double HillNoise,
        double HillAmplitude, double ShoulderHeight, double ShoulderCompressionScale);
    static double ShapeMeadows(double SourceHeight, double HillNoise, double HillAmplitude,
        double RollingNoise, double RollingAmplitude, double ShoulderHeight, double ShoulderCompressionScale);
    static double ShapeMeadows(double SourceHeight, double HillNoise, double HillAmplitude,
        double RollingNoise, double RollingAmplitude, double FloorNoise, double FloorAmplitude,
        double ShoulderHeight, double ShoulderCompressionScale);
    static double CompressMeadowShoulder(double Height, double ShoulderHeight, double ShoulderCompressionScale);
    static double ShapeElderwood(double SourceHeight, double Detail,
        const FKalmalaTerrainReliefSample& SharedRelief,
        double BroadAmplitude, double MiddleAmplitude, double LocalAmplitude);
    static double CompressShimmeringLakesLowland(double SourceHeight, double Humidity,
        double HumidityMinimum, double HumidityFull, double LowlandFloor,
        double LowlandCeiling, double Compression);
    static double ShapeShimmeringLakes(double SourceHeight, double MeadowCandidate,
        double ElderwoodCandidate, double Humidity, double HumidityMinimum,
        double HumidityFull, double MeadowShare, double LowlandFloor,
        double LowlandCeiling, double LowlandCompression);
    static double ShapeMire(double SourceHeight, double Detail,
        double TargetHeight, double CompressionScale, double ResidualDetailAmplitude);
    static double ShapeTundra(double SourceHeight, double Detail,
        const FKalmalaTerrainReliefSample& SharedRelief,
        double BroadAmplitude, double MiddleAmplitude, double LocalAmplitude);
    static double ShapeMountains(double SourceHeight, double Detail, double MountainRegion);
    /** Blend complete land candidates by coast-scaled weights, adding the shared base once. */
    static float BlendCandidateHeights(
        float SourceHeight, const double CandidateHeights[6], float OceanOffset, const float Weights[7]);
    // Private regression control: retain all land/basin shaping, omit only courses.
    static FKalmalaRegionalSample SampleInternal(const FKalmalaWorldFieldSample& Fields, bool bBiomeOnly, bool bSkipHydrology = false);
};
