#include "KalmalaMinimapRaster.h"
#include "KalmalaMinimapViewModel.h"

FLinearColor FKalmalaMinimapRaster::SampleBiomeTexture(const EKalmalaBiome Biome, const FVector2D& WorldPosition)
{
    // These small world-anchored patterns are presentation textures, not another
    // world field. Moving or zooming the map never moves a pattern in the world.
    const FVector2D P = WorldPosition / 180.0;
    const float Grain = FMath::PerlinNoise2D(P * 2.7);
    const float Broad = FMath::PerlinNoise2D(P * 0.43);
    const float Ripple = FMath::Sin(P.Y * 4.0 + FMath::Sin(P.X));
    FLinearColor Base;
    float Detail = 0.0f;
    switch (Biome)
    {
    case EKalmalaBiome::Meadows:
        Base = FLinearColor(0.34f, 0.46f, 0.15f);
        Detail = Grain * 0.20f + FMath::Sin(P.X * 7.0 + P.Y * 2.0) * 0.045f;
        break;
    case EKalmalaBiome::ShimmeringLakes:
        Base = FLinearColor(0.30f, 0.48f, 0.35f);
        Detail = Broad * 0.25f + Grain * 0.12f;
        break;
    case EKalmalaBiome::Elderwood:
        Base = FLinearColor(0.065f, 0.20f, 0.105f);
        Detail = FMath::Sin(P.X * 2.0) * FMath::Sin(P.Y * 2.0) * 0.30f + Grain * 0.18f;
        break;
    case EKalmalaBiome::MossyMire:
        Base = FLinearColor(0.25f, 0.28f, 0.095f);
        Detail = -FMath::Abs(Broad) * 0.65f + Ripple * 0.08f;
        break;
    case EKalmalaBiome::FreezingTundra:
        Base = FLinearColor(0.72f, 0.80f, 0.79f);
        Detail = Grain * 0.13f + Broad * 0.10f;
        break;
    case EKalmalaBiome::ThunderMountains:
        Base = FLinearColor(0.34f, 0.36f, 0.40f);
        Detail = FMath::Abs(FMath::Sin(P.X * 2.0 + P.Y * 3.0 + Broad)) * 0.40f - 0.20f;
        break;
    case EKalmalaBiome::Ocean:
    default:
        Base = FLinearColor(0.025f, 0.16f, 0.29f);
        Detail = Ripple * 0.13f + Grain * 0.08f;
        break;
    }
    FLinearColor Result = Base * (1.0f + Detail);
    Result.A = 1.0f;
    return Result;
}

TArray<FColor> FKalmalaMinimapRaster::BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples)
{
    const int32 Side = FMath::RoundToInt(FMath::Sqrt(static_cast<float>(Samples.Num())));
    return BuildPixels(Samples, FIntPoint(Side, Side));
}

TArray<FColor> FKalmalaMinimapRaster::BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples, const FIntPoint Dimensions)
{
    TArray<FColor> Pixels;
    if (Dimensions.X < 3 || Dimensions.Y < 3 || Dimensions.X * Dimensions.Y != Samples.Num()) return Pixels;
    Pixels.Reserve(Samples.Num());
    for (const FKalmalaMinimapTerrainSample& Sample : Samples)
    {
        FLinearColor Colour = Sample.TerrainColour;
        // Square maps render fully; minimaps retain their circular feathered edge.
        const float Edge = Dimensions.X == Dimensions.Y
            ? FMath::Clamp((1.0f - Sample.MapPosition.Size()) * (Dimensions.X - 1) * 0.5f, 0.0f, 1.0f)
            : 1.0f;
        Colour.A = Edge;
        Pixels.Add(Colour.ToFColorSRGB());
    }
    return Pixels;
}
