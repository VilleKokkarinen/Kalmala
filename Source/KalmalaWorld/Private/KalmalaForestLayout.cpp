#include "KalmalaForestLayout.h"

#include "KalmalaBiomeClassifier.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaRegionalTuning.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationSeeds.h"

namespace
{
    using T = FKalmalaRegionalTuning;
    constexpr int32 MeadowTreeCandidateCount = 18;
    constexpr int32 ElderwoodTreeCandidateCount = 72;
    constexpr float TreeEdgeMargin = 260.0f;
    constexpr float MeadowMinimumSurfaceNormalZ = 0.88f;
    constexpr uint64 TreeSeedSalt = 0xD1B54A32D192ED03ull;
}

FString FKalmalaForestLayout::GetPersistentSpawnId(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D& PatchCenter, const int32 CandidateIndex)
{
    if (!Config.IsValid() || PatchCenter.ContainsNaN()
        || CandidateIndex < 0 || CandidateIndex >= ElderwoodTreeCandidateCount) return FString();
    // Seventeen significant digits preserve distinct double patch centers. Normalize signed zero.
    return FString::Printf(TEXT("forest:v1:%llu:%.17g:%.17g:%d"),
        static_cast<unsigned long long>(Config.WorldSeed),
        PatchCenter.X == 0.0 ? 0.0 : PatchCenter.X,
        PatchCenter.Y == 0.0 ? 0.0 : PatchCenter.Y, CandidateIndex);
}

float FKalmalaForestLayout::SampleForestFactor(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D& AbsoluteWorldPosition)
{
    if (!Config.IsValid() || !FMath::IsFinite(AbsoluteWorldPosition.X)
        || !FMath::IsFinite(AbsoluteWorldPosition.Y) || T::ForestFactorScale <= 0.0)
    {
        return 0.0f;
    }

    const double Noise = FKalmalaRegionalGeneration::Noise(
        Config, T::ForestFactorSalt, AbsoluteWorldPosition, 1.0 / T::ForestFactorScale);
    return float(FMath::Clamp(0.5 + 0.5 * Noise, 0.0, 1.0));
}

float FKalmalaForestLayout::CombineMeadowAcceptanceProbability(
    const float FloraProbability, const float ForestFactor)
{
    if (!FMath::IsFinite(FloraProbability) || !FMath::IsFinite(ForestFactor)) return 0.0f;
    return FMath::Clamp(FloraProbability, 0.0f, 1.0f) * FMath::Clamp(ForestFactor, 0.0f, 1.0f);
}

double FKalmalaForestLayout::SampleForestCandidateThreshold(
    const FKalmalaWorldGenerationConfig& Config, const FVector2D& PatchCenter, const int32 CandidateIndex)
{
    if (!Config.IsValid() || !FMath::IsFinite(PatchCenter.X) || !FMath::IsFinite(PatchCenter.Y)
        || CandidateIndex < 0 || CandidateIndex >= ElderwoodTreeCandidateCount)
    {
        return 1.0;
    }

    const double PatchX = PatchCenter.X == 0.0 ? 0.0 : PatchCenter.X;
    const double PatchY = PatchCenter.Y == 0.0 ? 0.0 : PatchCenter.Y;
    uint64 PatchXBits = 0, PatchYBits = 0;
    FMemory::Memcpy(&PatchXBits, &PatchX, sizeof(PatchXBits));
    FMemory::Memcpy(&PatchYBits, &PatchY, sizeof(PatchYBits));

    uint64 Hash = FKalmalaRegionalGeneration::Seed(
        Config, T::ForestCandidateGateSalt, FIntPoint(CandidateIndex, 0));
    Hash ^= PatchXBits * 0xd6e8feb86659fd93ull;
    Hash ^= PatchYBits * 0xa0761d6478bd642full;
    Hash = (Hash ^ (Hash >> 30)) * 0xbf58476d1ce4e5b9ull;
    Hash = (Hash ^ (Hash >> 27)) * 0x94d049bb133111ebull;
    Hash ^= Hash >> 31;
    return double(Hash >> 11) * (1.0 / 9007199254740992.0);
}

bool FKalmalaForestLayout::IsMeadowSurfaceSlopeSuitable(const float SurfaceNormalZ)
{
    return FMath::IsFinite(SurfaceNormalZ) && SurfaceNormalZ >= MeadowMinimumSurfaceNormalZ;
}

bool FKalmalaForestLayout::IsMeadowGroundDry(const bool bRegionalHasWater,
    const bool bOceanSampleValid, const bool bOceanHasWater)
{
    return !bRegionalHasWater && bOceanSampleValid && !bOceanHasWater;
}

TArray<FKalmalaForestTreeDescriptor> FKalmalaForestLayout::Generate(
    const FKalmalaWorldGenerationConfig& WorldGenerationConfig, const FVector2D& PatchCenter)
{
    TArray<FKalmalaForestTreeDescriptor> Result;
    if (!WorldGenerationConfig.IsValid() || PatchCenter.ContainsNaN()) return Result;
    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    const uint64 FloraSeed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(WorldGenerationConfig, EKalmalaWorldField::Flora);
    FRandomStream RandomStream(static_cast<int32>((FloraSeed ^ TreeSeedSalt) >> 32));
    Result.Reserve(ElderwoodTreeCandidateCount);
    for (int32 CandidateIndex = 0; CandidateIndex < ElderwoodTreeCandidateCount; ++CandidateIndex)
    {
        const FVector2D LocalPosition(
            RandomStream.FRandRange(-HalfSurfaceSize + TreeEdgeMargin, HalfSurfaceSize - TreeEdgeMargin),
            RandomStream.FRandRange(-HalfSurfaceSize + TreeEdgeMargin, HalfSurfaceSize - TreeEdgeMargin));
        const FVector2D SamplePosition = PatchCenter + LocalPosition;
        if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, SamplePosition, 1000)) continue;
        const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, SamplePosition);
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(Fields);
        if (Biome != EKalmalaBiome::Meadows && Biome != EKalmalaBiome::Elderwood)
        {
            continue;
        }

        if (Biome == EKalmalaBiome::Meadows && CandidateIndex >= MeadowTreeCandidateCount)
        {
            continue;
        }
        const float FloraProbability = FMath::Clamp((Fields.Flora - 0.60f) / 0.24f, 0.20f, 0.92f);
        // Flora continuously controls Elderwood density: lower-flora pockets stay clear without becoming authored spaces.
        if (Biome == EKalmalaBiome::Elderwood && RandomStream.FRand() > FloraProbability)
        {
            continue;
        }

        const float ForestFactor = SampleForestFactor(WorldGenerationConfig, SamplePosition);
        const float MeadowAcceptanceProbability = CombineMeadowAcceptanceProbability(
            FloraProbability, ForestFactor);

        const bool bElderwood = Biome == EKalmalaBiome::Elderwood;
        // Keep the existing candidate positions, dimensions, yaw and IDs. A
        // stable ordinal pattern gives Meadow groves both birch and aspen.
        const bool bAspen = Biome == EKalmalaBiome::Meadows && CandidateIndex % 3 == 0;
        const float TrunkHeight = RandomStream.FRandRange(bElderwood ? 520.0f : 340.0f, bElderwood ? 850.0f : 620.0f);
        const float TrunkRadius = RandomStream.FRandRange(bElderwood ? 28.0f : 14.0f, bElderwood ? 48.0f : 25.0f);
        const float CanopyRadius = RandomStream.FRandRange(bElderwood ? 185.0f : 105.0f, bElderwood ? 285.0f : 170.0f);
        const float CanopyHeight = RandomStream.FRandRange(bElderwood ? 220.0f : 140.0f, bElderwood ? 360.0f : 240.0f);
        const float TreeYaw = RandomStream.FRandRange(0.0f, 360.0f);
        // Consume the existing candidate draw sequence before applying the new Meadow gate.
        // Its independent per-ID roll cannot shift later positions or dimensions when rejected.
        if (Biome == EKalmalaBiome::Meadows
            && (MeadowAcceptanceProbability <= 0.0f
                || SampleForestCandidateThreshold(WorldGenerationConfig, PatchCenter, CandidateIndex)
                    > MeadowAcceptanceProbability))
        {
            continue;
        }

        // Only slope-check Meadows candidates that passed the existing gates.
        if (Biome == EKalmalaBiome::Meadows
            && !IsMeadowSurfaceSlopeSuitable(
                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(WorldGenerationConfig, SamplePosition).Z))
        {
            continue;
        }

        const FKalmalaRegionalSample FinalSurface =
            FKalmalaRegionalGeneration::Sample(WorldGenerationConfig, SamplePosition);
        FKalmalaOceanSample OceanSurface;
        if (Biome == EKalmalaBiome::Meadows && !FinalSurface.bHasWater)
        {
            OceanSurface = FKalmalaOceanSampler::Sample(WorldGenerationConfig, SamplePosition,
                PatchCenter - FVector2D(HalfSurfaceSize, HalfSurfaceSize));
        }
        if (Biome == EKalmalaBiome::Meadows
            && !IsMeadowGroundDry(FinalSurface.bHasWater, OceanSurface.bIsValid, OceanSurface.IsWater()))
        {
            continue;
        }

        const float SurfaceHeight = FinalSurface.Height;
        const EKalmalaForestTreeKind TreeKind = bElderwood
            ? EKalmalaForestTreeKind::ElderwoodIronheart
            : (bAspen ? EKalmalaForestTreeKind::MeadowsAspen : EKalmalaForestTreeKind::MeadowsBirch);
        Result.Add({CandidateIndex, FVector(SamplePosition.X, SamplePosition.Y, SurfaceHeight),
            FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight),
            TrunkRadius, TrunkHeight, CanopyRadius, CanopyHeight, TreeYaw, TreeKind});
        // This absolute-world noise sample is stateless and consumes no candidate-stream draws.
        Result.Last().ForestFactor = ForestFactor;
        Result.Last().PersistentSpawnId = GetPersistentSpawnId(WorldGenerationConfig, PatchCenter, CandidateIndex);
    }
    return Result;
}
