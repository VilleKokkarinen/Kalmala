#pragma once

#include "CoreMinimal.h"
#include "KalmalaBiomeClassifier.h"

struct FKalmalaMinimapTerrainSample;

/** Original procedural biome swatches and a disposable circular UI texture. */
struct KALMALAUI_API FKalmalaMinimapRaster
{
    static FLinearColor SampleBiomeTexture(EKalmalaBiome Biome, const FVector2D& WorldPosition);
    static TArray<FColor> BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples);
    static TArray<FColor> BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples, FIntPoint Dimensions);
};
