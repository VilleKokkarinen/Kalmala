#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

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
    bool bElderwood = false;
    FString PersistentSpawnId;
};

/** Shared deterministic placement for forest rendering and server resource materialization. */
struct KALMALAWORLD_API FKalmalaForestLayout
{
    /** World/patch/candidate identity; empty for invalid inputs. Never derived from filtered array order. */
    static FString GetPersistentSpawnId(const FKalmalaWorldGenerationConfig& Config,
        const FVector2D& PatchCenter, int32 CandidateIndex);

    static TArray<FKalmalaForestTreeDescriptor> Generate(
        const FKalmalaWorldGenerationConfig& Config, const FVector2D& PatchCenter);
};
