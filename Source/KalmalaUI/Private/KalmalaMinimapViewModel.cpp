#include "KalmalaMinimapViewModel.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldGenerationGameState.h"

void UKalmalaMinimapViewModel::Initialize(APlayerController* InOwningPlayer)
{
    OwningPlayer = InOwningPlayer;
    TerrainSamples.Reset();
    bIsReady = false;
}

bool UKalmalaMinimapViewModel::Refresh()
{
    bIsReady = false;
    TerrainSamples.Reset();

    if (OwningPlayer == nullptr || !OwningPlayer->IsLocalController() || OwningPlayer->GetPawn() == nullptr || OwningPlayer->GetWorld() == nullptr)
    {
        return false;
    }

    const AKalmalaWorldGenerationGameState* WorldGenerationState = OwningPlayer->GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr || !WorldGenerationState->GetWorldGenerationConfig().IsValid())
    {
        return false;
    }

    const APawn* OwningPawn = OwningPlayer->GetPawn();
    TerrainSamples = BuildTerrainSamples(
        WorldGenerationState->GetWorldGenerationConfig(),
        FVector2D(OwningPawn->GetActorLocation()),
        MapRadius,
        SamplesPerAxis);
    PlayerFacingDegrees = OwningPawn->GetActorRotation().Yaw;
    bIsReady = TerrainSamples.Num() > 0;
    return bIsReady;
}

TArray<FKalmalaMinimapTerrainSample> UKalmalaMinimapViewModel::BuildTerrainSamples(
    const FKalmalaWorldGenerationConfig& WorldConfig,
    const FVector2D& PlayerLocation,
    const float InMapRadius,
    const int32 InSamplesPerAxis)
{
    TArray<FKalmalaMinimapTerrainSample> Samples;
    if (!WorldConfig.IsValid() || InMapRadius <= 0.0f || InSamplesPerAxis < 3)
    {
        return Samples;
    }

    const int32 ClampedSamplesPerAxis = FMath::Clamp(InSamplesPerAxis, 3, 33);
    Samples.Reserve(ClampedSamplesPerAxis * ClampedSamplesPerAxis);
    for (int32 Y = 0; Y < ClampedSamplesPerAxis; ++Y)
    {
        for (int32 X = 0; X < ClampedSamplesPerAxis; ++X)
        {
            const FVector2D MapPosition(
                -1.0f + (2.0f * X / (ClampedSamplesPerAxis - 1)),
                -1.0f + (2.0f * Y / (ClampedSamplesPerAxis - 1)));
            const FVector2D WorldPosition = PlayerLocation + MapPosition * InMapRadius;

            FKalmalaMinimapTerrainSample& Sample = Samples.AddDefaulted_GetRef();
            Sample.MapPosition = MapPosition;
            Sample.TerrainHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldConfig, WorldPosition);
            Sample.bIsWater = FKalmalaShimmeringLakeSampler::IsWater(WorldConfig, WorldPosition);
        }
    }

    return Samples;
}
