#pragma once

#include "CoreMinimal.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationLayout.h"

/** Stable server-side formation and identity for one generated Tundra Wolf pack. */
struct FKalmalaWolfPackContract
{
    static constexpr int32 MemberCount = 3;
    static constexpr float FormationRadiusCentimetres = 240.0f;

    static int32 GetActorBudgetWithPackRoom(const int32 ExistingBudget)
    {
        return FMath::Max(MemberCount, ExistingBudget);
    }

    static FVector2D GetMemberOffset(const uint64 SpawnSeed, const int32 MemberIndex)
    {
        if (MemberIndex < 0 || MemberIndex >= MemberCount) return FVector2D::ZeroVector;

        const uint32 AngleBits = static_cast<uint32>(SpawnSeed ^ (SpawnSeed >> 32));
        const float StartAngle = static_cast<float>(AngleBits)
            * (2.0f * PI / 4294967296.0f);
        const float Angle = StartAngle + static_cast<float>(MemberIndex) * (2.0f * PI / MemberCount);
        return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * FormationRadiusCentimetres;
    }

    static FString GetMemberPersistentSpawnId(const FKalmalaWorldPopulationSpawn& PackSpawn,
        const int32 MemberIndex)
    {
        if (PackSpawn.Kind != EKalmalaWorldPopulationKind::Wildlife
            || MemberIndex < 0 || MemberIndex >= MemberCount) return FString();

        const FString BaseId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(PackSpawn);
        return BaseId.IsEmpty() ? FString() : BaseId + FString::Printf(TEXT("/wolf/%d"), MemberIndex);
    }

    static bool TryBuildMemberSpawn(const FKalmalaWorldGenerationConfig& Config,
        const FKalmalaWorldPopulationSpawn& PackSpawn, const int32 MemberIndex,
        FKalmalaWorldPopulationSpawn& OutMemberSpawn)
    {
        OutMemberSpawn = FKalmalaWorldPopulationSpawn{};
        if (PackSpawn.Kind != EKalmalaWorldPopulationKind::Wildlife
            || MemberIndex < 0 || MemberIndex >= MemberCount
            || PackSpawn.Location.ContainsNaN()) return false;

        const FVector2D PackPosition(PackSpawn.Location.X, PackSpawn.Location.Y);
        if (FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, PackPosition))
                != EKalmalaBiome::FreezingTundra) return false;

        const FVector2D MemberPosition = PackPosition + GetMemberOffset(PackSpawn.SpawnSeed, MemberIndex);
        if (!FKalmalaWorldBounds::Contains(Config, MemberPosition)
            || FKalmalaOceanSampler::Sample(Config, MemberPosition).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(Config, MemberPosition)
            || FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, MemberPosition).Z < 0.82f
            || FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(Config, MemberPosition))
                != EKalmalaBiome::FreezingTundra) return false;

        OutMemberSpawn = PackSpawn;
        OutMemberSpawn.Location = FVector(MemberPosition.X, MemberPosition.Y,
            FKalmalaTerrainHeightSampler::SampleHeight(Config, MemberPosition));
        return !OutMemberSpawn.Location.ContainsNaN()
            && !GetMemberPersistentSpawnId(PackSpawn, MemberIndex).IsEmpty();
    }
};
