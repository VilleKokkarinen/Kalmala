#include "KalmalaForestLayout.h"

#include "KalmalaBiomeClassifier.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationSeeds.h"

namespace
{
    constexpr int32 MeadowTreeCandidateCount = 18;
    constexpr int32 ElderwoodTreeCandidateCount = 72;
    constexpr float TreeEdgeMargin = 260.0f;
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
        // Flora continuously controls Elderwood density: lower-flora pockets stay clear without becoming authored spaces.
        if (Biome == EKalmalaBiome::Elderwood && RandomStream.FRand() > FMath::Clamp((Fields.Flora - 0.60f) / 0.24f, 0.20f, 0.92f))
        {
            continue;
        }

        const bool bElderwood = Biome == EKalmalaBiome::Elderwood;
        // Keep the existing candidate positions, dimensions, yaw and IDs. A
        // stable ordinal pattern gives Meadow groves both birch and aspen.
        const bool bAspen = Biome == EKalmalaBiome::Meadows && CandidateIndex % 3 == 0;
        const float TrunkHeight = RandomStream.FRandRange(bElderwood ? 520.0f : 340.0f, bElderwood ? 850.0f : 620.0f);
        const float TrunkRadius = RandomStream.FRandRange(bElderwood ? 28.0f : 14.0f, bElderwood ? 48.0f : 25.0f);
        const float CanopyRadius = RandomStream.FRandRange(bElderwood ? 185.0f : 105.0f, bElderwood ? 285.0f : 170.0f);
        const float CanopyHeight = RandomStream.FRandRange(bElderwood ? 220.0f : 140.0f, bElderwood ? 360.0f : 240.0f);
        const float SurfaceHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, SamplePosition);
        const float TreeYaw = RandomStream.FRandRange(0.0f, 360.0f);
        Result.Add({CandidateIndex, FVector(SamplePosition.X, SamplePosition.Y, SurfaceHeight),
            FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight),
            TrunkRadius, TrunkHeight, CanopyRadius, CanopyHeight, TreeYaw, bElderwood, bAspen});
        Result.Last().PersistentSpawnId = GetPersistentSpawnId(WorldGenerationConfig, PatchCenter, CandidateIndex);
    }
    return Result;
}
