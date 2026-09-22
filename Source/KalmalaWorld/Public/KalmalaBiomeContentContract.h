#pragma once

#include "CoreMinimal.h"
#include "KalmalaBiomeClassifier.h"

/** The bounded content identity categories selected by the server for a biome. */
enum class EKalmalaBiomeContentKind : uint8
{
    GatheringSource,
    CreatureNiche,
    RareDiscovery
};

/**
 * Stable first-wave content catalogue entry. These IDs describe server-selected
 * content identities; they are not client-authored item, target, reward, or
 * route data.
 */
struct KALMALAWORLD_API FKalmalaBiomeContentDefinition
{
    EKalmalaBiome Biome = EKalmalaBiome::Ocean;
    FName GatheringSourceId = NAME_None;
    FName GatheringPresentationId = NAME_None;
    FName CreatureNicheId = NAME_None;
    FName RareDiscoverySourceId = NAME_None;
    FName RareDiscoveryPresentationId = NAME_None;
    bool bRareDiscoveryOptional = false;
};

struct KALMALAWORLD_API FKalmalaBiomeContentContract
{
    static bool IsFirstWaveBiome(const EKalmalaBiome Biome)
    {
        return Biome != EKalmalaBiome::Ocean;
    }

    static FKalmalaBiomeContentDefinition GetDefinition(const EKalmalaBiome Biome)
    {
        switch (Biome)
        {
        case EKalmalaBiome::Meadows:
            return { Biome, TEXT("meadows-birch-bark"), TEXT("birch-bark-bundle"), TEXT("meadows-open-grazer"), TEXT("meadows-hidden-stone"), TEXT("stone-hollow-marker"), true };
        case EKalmalaBiome::ShimmeringLakes:
            return { Biome, TEXT("lakes-reed-cluster"), TEXT("reed-cluster"), TEXT("lakes-shore-forager"), TEXT("lakes-island-cache"), TEXT("island-cache-marker"), true };
        case EKalmalaBiome::Elderwood:
            return { Biome, TEXT("elderwood-resinwood"), TEXT("resinwood-bundle"), TEXT("elderwood-canopy-browser"), TEXT("elderwood-root-hollow"), TEXT("root-hollow-marker"), true };
        case EKalmalaBiome::MossyMire:
            return { Biome, TEXT("mire-bog-iron"), TEXT("bog-iron-vein"), TEXT("mire-hummock-scavenger"), TEXT("mire-sunken-cache"), TEXT("sunken-cache-marker"), true };
        case EKalmalaBiome::FreezingTundra:
            return { Biome, TEXT("tundra-frostmoss"), TEXT("frostmoss-clump"), TEXT("tundra-wind-grazer"), TEXT("tundra-ice-spring"), TEXT("ice-spring-marker"), true };
        case EKalmalaBiome::ThunderMountains:
            return { Biome, TEXT("mountains-slate-vein"), TEXT("slate-vein"), TEXT("mountains-ridge-forager"), TEXT("mountains-storm-overlook"), TEXT("storm-overlook-marker"), true };
        case EKalmalaBiome::Ocean:
        default:
            return { Biome, NAME_None, NAME_None, NAME_None, NAME_None, NAME_None, false };
        }
    }

    static FName GetServerContentId(const EKalmalaBiome Biome, const EKalmalaBiomeContentKind Kind)
    {
        const FKalmalaBiomeContentDefinition Definition = GetDefinition(Biome);
        switch (Kind)
        {
        case EKalmalaBiomeContentKind::GatheringSource: return Definition.GatheringSourceId;
        case EKalmalaBiomeContentKind::CreatureNiche: return Definition.CreatureNicheId;
        case EKalmalaBiomeContentKind::RareDiscovery: return Definition.RareDiscoverySourceId;
        default: return NAME_None;
        }
    }

    static bool IsValidFirstWaveDefinition(const FKalmalaBiomeContentDefinition& Definition)
    {
        return IsFirstWaveBiome(Definition.Biome)
            && !Definition.GatheringSourceId.IsNone()
            && !Definition.GatheringPresentationId.IsNone()
            && !Definition.CreatureNicheId.IsNone()
            && !Definition.RareDiscoverySourceId.IsNone()
            && !Definition.RareDiscoveryPresentationId.IsNone()
            && Definition.bRareDiscoveryOptional;
    }

    static FName GetGatheringPresentationId(const FName GatheringSourceId)
    {
        if (GatheringSourceId.IsNone()) return NAME_None;
        for (const EKalmalaBiome Biome : {
            EKalmalaBiome::Meadows,
            EKalmalaBiome::ShimmeringLakes,
            EKalmalaBiome::Elderwood,
            EKalmalaBiome::MossyMire,
            EKalmalaBiome::FreezingTundra,
            EKalmalaBiome::ThunderMountains })
        {
            const FKalmalaBiomeContentDefinition Definition = GetDefinition(Biome);
            if (Definition.GatheringSourceId == GatheringSourceId)
            {
                return Definition.GatheringPresentationId;
            }
        }
        return NAME_None;
    }

    static bool IsValidGatheringSourceId(const FName GatheringSourceId)
    {
        return !GetGatheringPresentationId(GatheringSourceId).IsNone();
    }

    static FName GetRareDiscoveryPresentationId(const FName RareDiscoverySourceId)
    {
        if (RareDiscoverySourceId.IsNone()) return NAME_None;
        for (const EKalmalaBiome Biome : {
            EKalmalaBiome::Meadows,
            EKalmalaBiome::ShimmeringLakes,
            EKalmalaBiome::Elderwood,
            EKalmalaBiome::MossyMire,
            EKalmalaBiome::FreezingTundra,
            EKalmalaBiome::ThunderMountains })
        {
            const FKalmalaBiomeContentDefinition Definition = GetDefinition(Biome);
            if (Definition.RareDiscoverySourceId == RareDiscoverySourceId)
            {
                return Definition.RareDiscoveryPresentationId;
            }
        }
        return NAME_None;
    }

    static bool IsValidRareDiscoverySourceId(const FName RareDiscoverySourceId)
    {
        return !GetRareDiscoveryPresentationId(RareDiscoverySourceId).IsNone();
    }

    static bool IsOptionalRareDiscoverySource(const EKalmalaBiome Biome)
    {
        const FKalmalaBiomeContentDefinition Definition = GetDefinition(Biome);
        return IsValidFirstWaveDefinition(Definition) && Definition.bRareDiscoveryOptional;
    }
};
