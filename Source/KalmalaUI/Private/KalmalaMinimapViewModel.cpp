#include "KalmalaMinimapViewModel.h"
#include "KalmalaLakeBasin.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaMinimapRaster.h"

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

void UKalmalaMinimapViewModel::SetMapRadius(const float InMapRadius)
{
    MapRadius = FMath::Max(100.0f, InMapRadius);
}

bool UKalmalaMinimapViewModel::Refresh()
{
    if (OwningPlayer == nullptr || !OwningPlayer->IsLocalController() || OwningPlayer->GetPawn() == nullptr || OwningPlayer->GetWorld() == nullptr)
    {
        bIsReady = false;
        return false;
    }

    const AKalmalaWorldGenerationGameState* WorldGenerationState = OwningPlayer->GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr || !WorldGenerationState->GetWorldGenerationConfig().IsValid())
    {
        bIsReady = false;
        return false;
    }

    const APawn* OwningPawn = OwningPlayer->GetPawn();
    const FVector2D Location(OwningPawn->GetActorLocation());
    const FKalmalaWorldGenerationConfig& Config = WorldGenerationState->GetWorldGenerationConfig();
    PlayerFacingDegrees = OwningPawn->GetActorRotation().Yaw;
    if (bIsReady && Location.Equals(LastLocation, 1.0f) && LastRadius == MapRadius
        && LastSeed == Config.WorldSeed && LastGeneratorRevision == Config.GeneratorRevision)
    {
        return true;
    }
    TerrainSamples = BuildTerrainSamples(
        Config,
        Location,
        MapRadius,
        SamplesPerAxis);
    LastLocation = Location;
    LastRadius = MapRadius;
    LastSeed = Config.WorldSeed;
    LastGeneratorRevision = Config.GeneratorRevision;
    ++PresentationRevision;
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

    const int32 ClampedSamplesPerAxis = FMath::Clamp(InSamplesPerAxis, 3, 129);
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
            const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(WorldConfig, WorldPosition);
            Sample.TerrainHeight = Ocean.TerrainHeight;
            const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldConfig, WorldPosition));
            Sample.bIsWater = Ocean.IsWater()
                || FKalmalaLakeBasin::IsVisibleWater(WorldConfig, WorldPosition);
            Sample.TerrainColour = FKalmalaMinimapRaster::SampleBiomeTexture(Sample.bIsWater ? EKalmalaBiome::Ocean : Biome, WorldPosition);
            if (Sample.bIsWater && Biome == EKalmalaBiome::ShimmeringLakes)
            {
                Sample.TerrainColour = Sample.TerrainColour * FLinearColor(1.3f, 1.8f, 1.6f);
            }
        }
    }

    return Samples;
}
