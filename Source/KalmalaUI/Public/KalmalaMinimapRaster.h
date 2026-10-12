#pragma once

#include "CoreMinimal.h"
#include "KalmalaBiomeClassifier.h"

struct FKalmalaMinimapTerrainSample;

/** Original procedural biome swatches and a disposable circular UI texture. */
struct KALMALAUI_API FKalmalaMinimapRaster
{
    static FLinearColor SampleBiomeTexture(EKalmalaBiome Biome, const FVector2D& WorldPosition);
    /** Broad world-anchored biome patterns for the expanded map; fine detail fades as raster spacing grows. */
    static FLinearColor SampleExpandedMapBiomeTexture(EKalmalaBiome Biome, const FVector2D& WorldPosition, float SampleSpacingWorld);
    /** Player-centred circular HUD crop of the shared terrain raster. */
    static TArray<FColor> BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples);
    /** Opaque terrain surface, including square world-map tiles. No viewport mask. */
    static TArray<FColor> BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples, FIntPoint Dimensions);
};
