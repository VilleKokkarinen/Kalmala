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
    FName CreatureNicheId = NAME_None;
    FName RareDiscoverySourceId = NAME_None;
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
            return { Biome, TEXT("meadows-birch-bark"), TEXT("meadows-open-grazer"), TEXT("meadows-hidden-stone"), true };
        case EKalmalaBiome::ShimmeringLakes:
            return { Biome, TEXT("lakes-reed-cluster"), TEXT("lakes-shore-forager"), TEXT("lakes-island-cache"), true };
        case EKalmalaBiome::Elderwood:
            return { Biome, TEXT("elderwood-resinwood"), TEXT("elderwood-canopy-browser"), TEXT("elderwood-root-hollow"), true };
        case EKalmalaBiome::MossyMire:
            return { Biome, TEXT("mire-bog-iron"), TEXT("mire-hummock-scavenger"), TEXT("mire-sunken-cache"), true };
        case EKalmalaBiome::FreezingTundra:
            return { Biome, TEXT("tundra-frostmoss"), TEXT("tundra-wind-grazer"), TEXT("tundra-ice-spring"), true };
        case EKalmalaBiome::ThunderMountains:
            return { Biome, TEXT("mountains-slate-vein"), TEXT("mountains-ridge-forager"), TEXT("mountains-storm-overlook"), true };
        case EKalmalaBiome::Ocean:
        default:
            return { Biome, NAME_None, NAME_None, NAME_None, false };
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
            && !Definition.CreatureNicheId.IsNone()
            && !Definition.RareDiscoverySourceId.IsNone()
            && Definition.bRareDiscoveryOptional;
    }
};
