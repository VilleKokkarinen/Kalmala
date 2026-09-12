#pragma once

#include "KalmalaWorldBounds.h"

struct FKalmalaMasterMapCrop
{
    FVector2D Center = FVector2D::ZeroVector;
    double Rotation = 0;
};

/** Independent, continuous land/water atlas. No baked pixels or saved map data. */
struct KALMALAWORLD_API FKalmalaMasterMap
{
    static constexpr uint64 MasterSeed = 0x4b616c6d616c6137ull;
    static constexpr double HalfExtent = 6400000.0; // 128 km square atlas.
    static constexpr double Wavelength = 300000.0;
    static FKalmalaMasterMapCrop Crop(const FKalmalaWorldGenerationConfig& Config);
    static FVector2D ToMasterPosition(const FKalmalaMasterMapCrop& Crop, FVector2D Position);
    // Positive = land, zero/negative = ocean. Seed is independent of game identity.
    static double SampleMaster(FVector2D Position);
    static double SampleMaster(FVector2D Position, uint64 Seed);
    static double Sample(const FKalmalaWorldGenerationConfig& Config, FVector2D Position);
};
