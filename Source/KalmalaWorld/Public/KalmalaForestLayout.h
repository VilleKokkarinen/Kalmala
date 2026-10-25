#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

/** Explicit forest presentation/resource identity, independent of the height candidate reused by a biome. */
enum class EKalmalaForestTreeKind : uint8
{
    MeadowsBirch,
    MeadowsAspen,
    ElderwoodIronheart,
    FreezingTundraBirch
};

/** A forest candidate's ordinal is stable within a seed and patch center, even across rejected candidates. */
struct KALMALAWORLD_API FKalmalaForestTreeDescriptor
{
    int32 CandidateIndex = INDEX_NONE;
    FVector Location = FVector::ZeroVector;
    FVector LocalPosition = FVector::ZeroVector;
    float TrunkRadius = 0.0f;
    float TrunkHeight = 0.0f;
    float CanopyRadius = 0.0f;
    float CanopyHeight = 0.0f;
    float Yaw = 0.0f;
    EKalmalaForestTreeKind TreeKind = EKalmalaForestTreeKind::MeadowsBirch;
    /** Continuous absolute-world placement mask, independent of ecological fields and candidate draws. */
    float ForestFactor = 0.0f;
    FString PersistentSpawnId;
};

/** Shared deterministic placement for forest rendering and server resource materialization. */
struct KALMALAWORLD_API FKalmalaForestLayout
{
    /** World/patch/candidate identity; empty for invalid inputs. Never derived from filtered array order. */
    static FString GetPersistentSpawnId(const FKalmalaWorldGenerationConfig& Config,
        const FVector2D& PatchCenter, int32 CandidateIndex);

    /** Samples the deterministic forest-placement mask at an absolute world position. */
    static float SampleForestFactor(const FKalmalaWorldGenerationConfig& Config,
        const FVector2D& AbsoluteWorldPosition);

    /** Combines lowland forest Flora suitability with the continuous forest factor. */
    static float CombineForestAcceptanceProbability(float FloraProbability, float ForestFactor);

    /** Combines the existing forest Flora probability/factor with provisional reduced Tundra density. */
    static float CombineTundraAcceptanceProbability(float FloraProbability, float ForestFactor);

    /** Stable unit threshold keyed by world, patch center, and original candidate ordinal. */
    static double SampleForestCandidateThreshold(const FKalmalaWorldGenerationConfig& Config,
        const FVector2D& PatchCenter, int32 CandidateIndex);

    /** True when the final sampled surface normal is within the shared forest-slope limit. */
    static bool IsForestSurfaceSlopeSuitable(float SurfaceNormalZ);

    /** True only when final regional water and a valid collision-lattice sea sample both allow placement. */
    static bool IsForestGroundDry(bool bRegionalHasWater, bool bOceanSampleValid, bool bOceanHasWater);

    static TArray<FKalmalaForestTreeDescriptor> Generate(
        const FKalmalaWorldGenerationConfig& Config, const FVector2D& PatchCenter);
};
