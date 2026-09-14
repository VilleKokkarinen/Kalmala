#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

/**
 * Transient server simulation state for one 200 cm ground-aligned interaction cell.
 * This intentionally contains no player status, replicated payload, or save identity.
 */
UENUM()
enum class EKalmalaInteractionMaterial : uint8
{
    Ground,
    Vegetation,
    Stone,
    ShallowWater
};

struct KALMALAGAMEPLAY_API FKalmalaInteractionCellState
{
    EKalmalaInteractionMaterial Material = EKalmalaInteractionMaterial::Ground;
    float Temperature = 50.0f;
    float SurfaceWetness = 0.0f;
};

/** Server-only deterministic baseline and bounded transient moisture helpers. */
struct KALMALAGAMEPLAY_API FKalmalaInteractionGrid
{
    static constexpr float CellSize = 200.0f;
    static constexpr int32 PawnRadiusCells = 4;
    static constexpr int32 HearthRadiusCells = 3;
    static constexpr int32 MaxActiveCells = 1024;

    static FIntPoint ToCellKey(const FVector& WorldLocation);
    static FVector2D ToCellCenter(const FIntPoint& Key);
    static FKalmalaInteractionCellState MakeBaseline(const FKalmalaWorldGenerationConfig& Config, const FIntPoint& Key);
    static void AdvanceSurfaceMoisture(FKalmalaInteractionCellState& State, float PrecipitationIntensity, float DeltaSeconds);
    static bool IsValid(const FKalmalaInteractionCellState& State);
};
