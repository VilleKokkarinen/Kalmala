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
    constexpr int32 MeadowsLakesTreeCandidateCount = 18;
    constexpr int32 ElderwoodTreeCandidateCount = 72;
    constexpr float TreeEdgeMargin = 260.0f;
    constexpr float ForestMinimumSurfaceNormalZ = 0.88f;
    constexpr uint64 TreeSeedSalt = 0xD1B54A32D192ED03ull;

    uint64 HashForestCandidate(const FKalmalaWorldGenerationConfig& Config,
        const FVector2D& PatchCenter, const int32 CandidateIndex, const uint64 Domain)
    {
        if (!Config.IsValid() || !FMath::IsFinite(PatchCenter.X) || !FMath::IsFinite(PatchCenter.Y)
            || CandidateIndex < 0 || CandidateIndex >= ElderwoodTreeCandidateCount) return 0;

        const double PatchX = PatchCenter.X == 0.0 ? 0.0 : PatchCenter.X;
        const double PatchY = PatchCenter.Y == 0.0 ? 0.0 : PatchCenter.Y;
        uint64 PatchXBits = 0, PatchYBits = 0;
        FMemory::Memcpy(&PatchXBits, &PatchX, sizeof(PatchXBits));
        FMemory::Memcpy(&PatchYBits, &PatchY, sizeof(PatchYBits));

        uint64 Hash = FKalmalaRegionalGeneration::Seed(
            Config, Domain, FIntPoint(CandidateIndex, 0));
        Hash ^= PatchXBits * 0xd6e8feb86659fd93ull;
        Hash ^= PatchYBits * 0xa0761d6478bd642full;
        Hash = (Hash ^ (Hash >> 30)) * 0xbf58476d1ce4e5b9ull;
        Hash = (Hash ^ (Hash >> 27)) * 0x94d049bb133111ebull;
        return Hash ^ (Hash >> 31);
    }
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

float FKalmalaForestLayout::CombineForestAcceptanceProbability(
    const float FloraProbability, const float ForestFactor)
{
    if (!FMath::IsFinite(FloraProbability) || !FMath::IsFinite(ForestFactor)) return 0.0f;
    return FMath::Clamp(FloraProbability, 0.0f, 1.0f) * FMath::Clamp(ForestFactor, 0.0f, 1.0f);
}

float FKalmalaForestLayout::CombineTundraAcceptanceProbability(
    const float FloraProbability, const float ForestFactor)
{
    if (!FMath::IsFinite(FloraProbability) || !FMath::IsFinite(ForestFactor)) return 0.0f;
    return FMath::Clamp(FloraProbability, 0.0f, 1.0f)
        * FMath::Clamp(ForestFactor, 0.0f, 1.0f)
        * T::FreezingTundraForestDensityMultiplier;
}

double FKalmalaForestLayout::SampleForestCandidateThreshold(
    const FKalmalaWorldGenerationConfig& Config, const FVector2D& PatchCenter, const int32 CandidateIndex)
{
    if (!Config.IsValid() || !FMath::IsFinite(PatchCenter.X) || !FMath::IsFinite(PatchCenter.Y)
        || CandidateIndex < 0 || CandidateIndex >= ElderwoodTreeCandidateCount) return 1.0;
    const uint64 Hash = HashForestCandidate(Config, PatchCenter, CandidateIndex, T::ForestCandidateGateSalt);
    return double(Hash >> 11) * (1.0 / 9007199254740992.0);
}

bool FKalmalaForestLayout::IsForestSurfaceSlopeSuitable(const float SurfaceNormalZ)
{
    return FMath::IsFinite(SurfaceNormalZ) && SurfaceNormalZ >= ForestMinimumSurfaceNormalZ;
}

bool FKalmalaForestLayout::IsForestGroundDry(const bool bRegionalHasWater,
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
        const bool bMeadows = Biome == EKalmalaBiome::Meadows;
        const bool bLakes = Biome == EKalmalaBiome::ShimmeringLakes;
        const bool bElderwood = Biome == EKalmalaBiome::Elderwood;
        const bool bTundra = Biome == EKalmalaBiome::FreezingTundra;
        if (!bMeadows && !bLakes && !bElderwood && !bTundra)
        {
            continue;
        }

        if ((bMeadows || bLakes) && CandidateIndex >= MeadowsLakesTreeCandidateCount)
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
        const float MeadowsLakesAcceptanceProbability = CombineForestAcceptanceProbability(FloraProbability, ForestFactor);
        const float TundraAcceptanceProbability = CombineTundraAcceptanceProbability(FloraProbability, ForestFactor);
        if (bLakes
            && (MeadowsLakesAcceptanceProbability <= 0.0f
                || SampleForestCandidateThreshold(WorldGenerationConfig, PatchCenter, CandidateIndex)
                    > MeadowsLakesAcceptanceProbability))
        {
            continue;
        }
        if (bTundra
            && (TundraAcceptanceProbability <= 0.0f
                || SampleForestCandidateThreshold(WorldGenerationConfig, PatchCenter, CandidateIndex)
                    > TundraAcceptanceProbability))
        {
            continue;
        }

        // Keep the existing candidate positions, dimensions, yaw and IDs. A
        // stable ordinal pattern gives Meadow groves both birch and aspen.
        const bool bAspen = bMeadows && CandidateIndex % 3 == 0;
        float TrunkHeight = 0.0f, TrunkRadius = 0.0f, CanopyRadius = 0.0f, CanopyHeight = 0.0f, TreeYaw = 0.0f;
        if (bTundra || bLakes)
        {
            // New biome candidates use stable per-ID appearance, not the shared
            // stream that determines existing positions and Meadow/Elderwood geometry.
            const uint64 AppearanceHash = HashForestCandidate(WorldGenerationConfig, PatchCenter,
                CandidateIndex, T::ForestCandidateGeometrySalt);
            FRandomStream AppearanceStream(static_cast<int32>(AppearanceHash >> 32));
            TrunkHeight = AppearanceStream.FRandRange(340.0f, 620.0f);
            TrunkRadius = AppearanceStream.FRandRange(14.0f, 25.0f);
            CanopyRadius = AppearanceStream.FRandRange(105.0f, 170.0f);
            CanopyHeight = AppearanceStream.FRandRange(140.0f, 240.0f);
            TreeYaw = AppearanceStream.FRandRange(0.0f, 360.0f);
        }
        else
        {
            TrunkHeight = RandomStream.FRandRange(bElderwood ? 520.0f : 340.0f, bElderwood ? 850.0f : 620.0f);
            TrunkRadius = RandomStream.FRandRange(bElderwood ? 28.0f : 14.0f, bElderwood ? 48.0f : 25.0f);
            CanopyRadius = RandomStream.FRandRange(bElderwood ? 185.0f : 105.0f, bElderwood ? 285.0f : 170.0f);
            CanopyHeight = RandomStream.FRandRange(bElderwood ? 220.0f : 140.0f, bElderwood ? 360.0f : 240.0f);
            TreeYaw = RandomStream.FRandRange(0.0f, 360.0f);
        }
        // Consume the existing candidate draw sequence before applying the new Meadow gate.
        // Its independent per-ID roll cannot shift later positions or dimensions when rejected.
        if (bMeadows
            && (MeadowsLakesAcceptanceProbability <= 0.0f
                || SampleForestCandidateThreshold(WorldGenerationConfig, PatchCenter, CandidateIndex)
                    > MeadowsLakesAcceptanceProbability))
        {
            continue;
        }

        // Only slope-check Meadows candidates that passed the existing gates.
        if ((bMeadows || bLakes)
            && !IsForestSurfaceSlopeSuitable(
                FKalmalaTerrainHeightSampler::SampleSurfaceNormal(WorldGenerationConfig, SamplePosition).Z))
        {
            continue;
        }

        const FKalmalaRegionalSample FinalSurface =
            FKalmalaRegionalGeneration::Sample(WorldGenerationConfig, SamplePosition);
        FKalmalaOceanSample OceanSurface;
        if ((bMeadows || bLakes) && !FinalSurface.bHasWater)
        {
            OceanSurface = FKalmalaOceanSampler::Sample(WorldGenerationConfig, SamplePosition,
                PatchCenter - FVector2D(HalfSurfaceSize, HalfSurfaceSize));
        }
        if ((bMeadows || bLakes)
            && !IsForestGroundDry(FinalSurface.bHasWater, OceanSurface.bIsValid, OceanSurface.IsWater()))
        {
            continue;
        }

        const float SurfaceHeight = FinalSurface.Height;
        // Lakes use the already established generic Birch source and presentation.
        const EKalmalaForestTreeKind TreeKind = bTundra
            ? EKalmalaForestTreeKind::FreezingTundraBirch
            : (bLakes ? EKalmalaForestTreeKind::MeadowsBirch
            : (bElderwood
            ? EKalmalaForestTreeKind::ElderwoodIronheart
            : (bAspen ? EKalmalaForestTreeKind::MeadowsAspen : EKalmalaForestTreeKind::MeadowsBirch)));
        Result.Add({CandidateIndex, FVector(SamplePosition.X, SamplePosition.Y, SurfaceHeight),
            FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight),
            TrunkRadius, TrunkHeight, CanopyRadius, CanopyHeight, TreeYaw, TreeKind});
        // This absolute-world noise sample is stateless and consumes no candidate-stream draws.
        Result.Last().ForestFactor = ForestFactor;
        Result.Last().PersistentSpawnId = GetPersistentSpawnId(WorldGenerationConfig, PatchCenter, CandidateIndex);
    }
    return Result;
}
