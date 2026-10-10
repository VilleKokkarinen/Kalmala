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

FLinearColor FKalmalaMinimapRaster::SampleExpandedMapBiomeTexture(
    const EKalmalaBiome Biome, const FVector2D& WorldPosition, const float SampleSpacingWorld)
{
    // Keep the recognizable landforms broad enough for overview pixels while
    // anchoring every pattern to world coordinates so pan never slides it.
    const FVector2D P = WorldPosition / 1000.0;
    const float Broad = FMath::PerlinNoise2D(P * 0.30);
    const float Medium = FMath::PerlinNoise2D(P * 0.78);
    const float FineVisibility = 1.0f - FMath::Clamp((SampleSpacingWorld - 140.0f) / 520.0f, 0.0f, 1.0f);
    const float Fine = FineVisibility > 0.0f ? FMath::PerlinNoise2D(P * 1.65) : 0.0f;
    const float MeadowRibbons = FMath::Sin(P.X * 1.45 + P.Y * 0.32 + Broad * 1.8);
    const float LakeChannels = FMath::Sin(P.X * 1.05 + FMath::Sin(P.Y * 0.65) * 0.8 + Broad * 1.4);
    const float WindBands = FMath::Sin(P.X * 0.65 + P.Y * 0.18 + Broad * 0.7);
    const float MountainRidges = 1.0f - FMath::Abs(FMath::Sin(P.X * 1.20 + P.Y * 0.42 + Broad * 2.0));

    FLinearColor Base;
    float Detail = 0.0f;
    switch (Biome)
    {
    case EKalmalaBiome::Meadows:
        Base = FLinearColor(0.36f, 0.50f, 0.20f);
        Detail = Broad * 0.18f + Medium * 0.12f + MeadowRibbons * 0.075f + Fine * FineVisibility * 0.05f;
        break;
    case EKalmalaBiome::ShimmeringLakes:
        Base = FLinearColor(0.28f, 0.46f, 0.35f);
        Detail = Broad * 0.14f + Medium * 0.09f - FMath::Abs(LakeChannels) * 0.15f + Fine * FineVisibility * 0.04f;
        break;
    case EKalmalaBiome::Elderwood:
    {
        Base = FLinearColor(0.075f, 0.20f, 0.11f);
        const float Canopy = FMath::Clamp((Medium + 0.20f) / 0.65f, 0.0f, 1.0f);
        const float CanopyClusters = Canopy * Canopy * (3.0f - 2.0f * Canopy);
        Detail = (CanopyClusters - 0.5f) * 0.42f + Broad * 0.13f + Fine * FineVisibility * 0.06f;
        break;
    }
    case EKalmalaBiome::MossyMire:
        Base = FLinearColor(0.25f, 0.29f, 0.12f);
        Detail = Broad * 0.14f + Medium * 0.11f + (0.5f - FMath::Abs(LakeChannels)) * 0.20f + Fine * FineVisibility * 0.04f;
        break;
    case EKalmalaBiome::FreezingTundra:
        Base = FLinearColor(0.68f, 0.76f, 0.76f);
        Detail = Broad * 0.16f + Medium * 0.08f + WindBands * 0.10f + Fine * FineVisibility * 0.035f;
        break;
    case EKalmalaBiome::ThunderMountains:
        Base = FLinearColor(0.34f, 0.38f, 0.43f);
        Detail = (MountainRidges - 0.5f) * 0.34f + Broad * 0.14f + Medium * 0.07f + Fine * FineVisibility * 0.035f;
        break;
    case EKalmalaBiome::Ocean:
    default:
        Base = FLinearColor(0.035f, 0.18f, 0.30f);
        Detail = Broad * 0.12f + Medium * 0.08f + FMath::Sin(P.X * 1.3 + P.Y * 0.28 + FMath::Sin(P.Y * 0.55)) * 0.09f;
        break;
    }

    FLinearColor Result = Base * (1.0f + Detail);
    Result.A = 1.0f;
    return Result;
}

TArray<FColor> FKalmalaMinimapRaster::BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples)
{
    const int32 Side = FMath::RoundToInt(FMath::Sqrt(static_cast<float>(Samples.Num())));
    TArray<FColor> Pixels = BuildPixels(Samples, FIntPoint(Side, Side));
    // Circular clipping belongs only to the HUD viewport, never to terrain tiles.
    for (int32 Index = 0; Index < Pixels.Num(); ++Index)
    {
        const float Edge = FMath::Clamp((1.0f - Samples[Index].MapPosition.Size()) * (Side - 1) * 0.5f, 0.0f, 1.0f);
        Pixels[Index].A = FMath::RoundToInt(Edge * 255.0f);
    }
    return Pixels;
}

TArray<FColor> FKalmalaMinimapRaster::BuildPixels(const TArray<FKalmalaMinimapTerrainSample>& Samples, const FIntPoint Dimensions)
{
    TArray<FColor> Pixels;
    if (Dimensions.X < 3 || Dimensions.Y < 3 || Dimensions.X * Dimensions.Y != Samples.Num()) return Pixels;
    Pixels.Reserve(Samples.Num());
    for (const FKalmalaMinimapTerrainSample& Sample : Samples)
    {
        FLinearColor Colour = Sample.TerrainColour;
        Colour.A = 1.0f;
        Pixels.Add(Colour.ToFColorSRGB());
    }
    return Pixels;
}
