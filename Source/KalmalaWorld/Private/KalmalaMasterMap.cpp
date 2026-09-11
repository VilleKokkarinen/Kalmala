#include "KalmalaMasterMap.h"

namespace
{
    uint64 Mix(uint64 V)
    {
        V = (V ^ (V >> 30)) * 0xbf58476d1ce4e5b9ull;
        V = (V ^ (V >> 27)) * 0x94d049bb133111ebull;
        return V ^ (V >> 31);
    }
    double Unit(uint64 V) { return double(V & 0xffffff) / 16777216.0; }
}

double FKalmalaMasterMap::SampleMaster(FVector2D P, uint64 Seed)
{
    const uint64 Bits = Mix(Seed);
    const FVector2D Offset(Unit(Bits) * 128, Unit(Bits >> 24) * 128);
    return 0.75 * FMath::PerlinNoise2D(P / Wavelength + Offset)
        + 0.25 * FMath::PerlinNoise2D(P / (Wavelength * 0.43) + Offset + FVector2D(37.1, 91.7));
}

FKalmalaMasterMapCrop FKalmalaMasterMap::Crop(const FKalmalaWorldGenerationConfig& C)
{
    // Cache only the tiny derived transform per worker, never terrain or biome samples.
    static thread_local FKalmalaWorldGenerationConfig CachedIdentity;
    static thread_local FKalmalaMasterMapCrop CachedCrop;
    static thread_local bool bCached = false;
    if (bCached && CachedIdentity == C) return CachedCrop;
    const uint64 Bits = Mix(C.WorldSeed ^ (uint64(C.GeneratorRevision) << 32));
    FKalmalaMasterMapCrop Result;
    Result.Rotation = Unit(Mix(Bits)) * 2 * PI;
    double Best = -2;
    // Select an inland origin without painting land into the master map. The
    // entire rotated circular crop fits the atlas, including its outer edge.
    for (uint64 I = 0; I < 128; ++I)
    {
        const uint64 Candidate = Mix(Bits + I * 0x9e3779b97f4a7c15ull);
        const FVector2D Center = FVector2D(Unit(Candidate) * 2 - 1, Unit(Candidate >> 24) * 2 - 1)
            * (HalfExtent - FKalmalaWorldBounds::Radius);
        const double Land = SampleMaster(Center);
        if (Land > Best) { Best = Land; Result.Center = Center; }
        if (Land > 0.10) break;
    }
    CachedIdentity = C; CachedCrop = Result; bCached = true;
    return Result;
}

FVector2D FKalmalaMasterMap::ToMasterPosition(const FKalmalaMasterMapCrop& C, FVector2D P)
{
    const double Cos = FMath::Cos(C.Rotation), Sin = FMath::Sin(C.Rotation);
    return C.Center + FVector2D(Cos * P.X - Sin * P.Y, Sin * P.X + Cos * P.Y);
}

double FKalmalaMasterMap::Sample(const FKalmalaWorldGenerationConfig& C, FVector2D P)
{
    return SampleMaster(ToMasterPosition(Crop(C), P));
}
