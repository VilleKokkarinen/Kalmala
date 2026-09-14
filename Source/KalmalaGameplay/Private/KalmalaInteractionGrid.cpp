#include "KalmalaInteractionGrid.h"

#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldFieldSampler.h"

FIntPoint FKalmalaInteractionGrid::ToCellKey(const FVector& WorldLocation)
{
    return FIntPoint(FMath::FloorToInt(WorldLocation.X / CellSize), FMath::FloorToInt(WorldLocation.Y / CellSize));
}

FVector2D FKalmalaInteractionGrid::ToCellCenter(const FIntPoint& Key)
{
    return FVector2D((static_cast<float>(Key.X) + 0.5f) * CellSize, (static_cast<float>(Key.Y) + 0.5f) * CellSize);
}

FKalmalaInteractionCellState FKalmalaInteractionGrid::MakeBaseline(const FKalmalaWorldGenerationConfig& Config, const FIntPoint& Key)
{
    FKalmalaInteractionCellState State;
    const FVector2D Center = ToCellCenter(Key);
    const FKalmalaOceanSample Water = FKalmalaOceanSampler::Sample(Config, Center);
    const FKalmalaEnvironmentalExposureSample Exposure = FKalmalaEnvironmentalExposureSampler::Sample(Config, Center);
    const float TerrainHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, Center);
    const float Flora = FKalmalaWorldFieldSampler::Sample(Config, Center).Flora;

    if (Water.WaterDepth > 1.0f)
    {
        State.Material = EKalmalaInteractionMaterial::ShallowWater;
        State.SurfaceWetness = 100.0f;
        State.Temperature = 40.0f;
    }
    else if (TerrainHeight > 1200.0f || Flora < 0.22f)
    {
        State.Material = EKalmalaInteractionMaterial::Stone;
    }
    else if (Flora >= 0.58f)
    {
        State.Material = EKalmalaInteractionMaterial::Vegetation;
    }

    if (State.Material != EKalmalaInteractionMaterial::ShallowWater)
    {
        State.SurfaceWetness = FMath::Clamp(FMath::IsFinite(Exposure.GroundWetness) ? Exposure.GroundWetness * 100.0f : 0.0f, 0.0f, 100.0f);
    }
    State.Temperature = FMath::Clamp(FMath::IsFinite(State.Temperature) ? State.Temperature : 50.0f, 0.0f, 100.0f);
    return State;
}

void FKalmalaInteractionGrid::AdvanceSurfaceMoisture(FKalmalaInteractionCellState& State, const float PrecipitationIntensity, const float DeltaSeconds)
{
    if (!IsValid(State))
    {
        State = FKalmalaInteractionCellState();
    }
    const float Seconds = FMath::Max(0.0f, FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f);
    const float Rain = FMath::Clamp(FMath::IsFinite(PrecipitationIntensity) ? PrecipitationIntensity : 0.0f, 0.0f, 1.0f);
    const float DryingPerSecond = State.Material == EKalmalaInteractionMaterial::ShallowWater ? 0.0f : 2.0f;
    const float WettingPerSecond = State.Material == EKalmalaInteractionMaterial::ShallowWater ? 0.0f : 14.0f * Rain;
    State.SurfaceWetness = FMath::Clamp(State.SurfaceWetness + (WettingPerSecond - DryingPerSecond) * Seconds, 0.0f, 100.0f);
    State.Temperature = FMath::Clamp(State.Temperature, 0.0f, 100.0f);
}

bool FKalmalaInteractionGrid::IsValid(const FKalmalaInteractionCellState& State)
{
    return FMath::IsFinite(State.Temperature) && FMath::IsFinite(State.SurfaceWetness)
        && State.Temperature >= 0.0f && State.Temperature <= 100.0f
        && State.SurfaceWetness >= 0.0f && State.SurfaceWetness <= 100.0f;
}
