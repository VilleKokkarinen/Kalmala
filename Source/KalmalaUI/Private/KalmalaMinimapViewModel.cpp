#include "KalmalaMinimapViewModel.h"
#include "KalmalaLakeBasin.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaMinimapRaster.h"
#include "Async/Async.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"

void UKalmalaMinimapViewModel::Initialize(APlayerController* InOwningPlayer)
{
    OwningPlayer = InOwningPlayer;
    PendingSamples = {};
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
    return RefreshTerrain(Config, Location);
}

bool UKalmalaMinimapViewModel::RefreshTerrain(const FKalmalaWorldGenerationConfig& Config, const FVector2D& Location)
{
    if (LastSeed != Config.WorldSeed || LastGeneratorRevision != Config.GeneratorRevision)
    {
        bIsReady = false;
    }
    // Never wait on procedural generation from a widget tick. Coalesce movement
    // while a job is running, then request the latest position on completion.
    if (PendingSamples.IsValid())
    {
        if (!PendingSamples.IsReady()) return bIsReady;
        auto CompletedSamples = PendingSamples.Get();
        PendingSamples = {};
        if (PendingConfig == Config && PendingRadius == MapRadius)
        {
            TerrainSamples = MoveTemp(CompletedSamples);
            LastLocation = PendingLocation;
            LastRadius = PendingRadius;
            LastSeed = PendingConfig.WorldSeed;
            LastGeneratorRevision = PendingConfig.GeneratorRevision;
            ++PresentationRevision;
            bIsReady = !TerrainSamples.IsEmpty();
        }
    }
    if (bIsReady && Location.Equals(LastLocation, 1.0f) && LastRadius == MapRadius
        && LastSeed == Config.WorldSeed && LastGeneratorRevision == Config.GeneratorRevision)
    {
        return true;
    }
    PendingConfig = Config;
    PendingLocation = Location;
    PendingRadius = MapRadius;
    PendingSamples = Async(EAsyncExecution::ThreadPool,
        [Config, Location, Radius = MapRadius, Count = SamplesPerAxis]()
        {
            return BuildTerrainSamples(Config, Location, Radius, Count);
        });
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
    // Scratch results live only for this raster build. Adjacent pixels share
    // collision vertices; sample each once for both terrain and inland water.
    // This is not a persistent biome/height map or an authoritative world cache.
    TMap<FIntPoint, FKalmalaRegionalSample> Vertices;
    FVector2D GridOrigin = FVector2D::ZeroVector;
    if (WorldConfig.GeneratorRevision >= 3)
    {
        static thread_local FKalmalaWorldGenerationConfig OriginConfig;
        static thread_local FVector2D Origin;
        static thread_local bool bHasOrigin = false;
        if (!bHasOrigin || !(OriginConfig == WorldConfig))
        {
            Origin = FVector2D(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(WorldConfig).GetLocation());
            OriginConfig = WorldConfig;
            bHasOrigin = true;
        }
        GridOrigin = Origin;
    }
    auto Vertex = [&](FIntPoint Key)
    {
        if (const auto* Found = Vertices.Find(Key)) return *Found;
        return Vertices.Add(Key, FKalmalaRegionalGeneration::Sample(WorldConfig,
            GridOrigin + FVector2D(Key) * FKalmalaLakeBasin::GridSpacing));
    };
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
            EKalmalaBiome Biome;
            if (WorldConfig.GeneratorRevision >= 3)
            {
                const FVector2D Cell = (WorldPosition - GridOrigin) / FKalmalaLakeBasin::GridSpacing;
                const FIntPoint Base(FMath::FloorToInt(Cell.X), FMath::FloorToInt(Cell.Y));
                const double U = Cell.X - Base.X, V = Cell.Y - Base.Y;
                const bool bUpper = U + V > 1.0;
                const auto East = Vertex(Base + FIntPoint(1, 0));
                const auto North = Vertex(Base + FIntPoint(0, 1));
                const auto Opposite = Vertex(Base + (bUpper ? FIntPoint(1, 1) : FIntPoint(0, 0)));
                const double A = bUpper ? 1.0 - V : U, B = bUpper ? 1.0 - U : V;
                const double D = bUpper ? U + V - 1.0 : 1.0 - U - V;
                Sample.TerrainHeight = double(East.Height) * A + double(North.Height) * B + double(Opposite.Height) * D;
                const double InlandDepth = (East.WaterLevel - East.Height) * A
                    + (North.WaterLevel - North.Height) * B + (Opposite.WaterLevel - Opposite.Height) * D;
                Sample.bIsWater = Sample.TerrainHeight < 0.0f || InlandDepth > 0.0;
                Biome = static_cast<EKalmalaBiome>(FKalmalaRegionalGeneration::Sample(WorldConfig, WorldPosition).Biome);
            }
            else
            {
                const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(WorldConfig, WorldPosition);
                Sample.TerrainHeight = Ocean.TerrainHeight;
                Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldConfig, WorldPosition));
                Sample.bIsWater = Ocean.IsWater() || FKalmalaLakeBasin::IsVisibleWater(WorldConfig, WorldPosition);
            }
            Sample.TerrainColour = FKalmalaMinimapRaster::SampleBiomeTexture(Sample.bIsWater ? EKalmalaBiome::Ocean : Biome, WorldPosition);
            if (Sample.bIsWater && Biome == EKalmalaBiome::ShimmeringLakes)
            {
                Sample.TerrainColour = Sample.TerrainColour * FLinearColor(1.3f, 1.8f, 1.6f);
            }
        }
    }

    return Samples;
}
