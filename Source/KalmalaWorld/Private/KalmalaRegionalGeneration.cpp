#include "KalmalaRegionalGeneration.h"
#include "Misc/ScopeLock.h"
#include "KalmalaWorldBounds.h"

namespace
{
    using T = FKalmalaRegionalTuning;
    using G = FKalmalaRegionalGeneration;
    double Smooth(double A, double B, double X)
    {
        const double V = FMath::Clamp((X - A) / (B - A), 0.0, 1.0);
        return V * V * (3.0 - 2.0 * V);
    }
    double Unit(uint64 Bits) { return double(Bits & 0xffffff) / 16777216.0; }
    FIntPoint CellAt(FVector2D P, double Size) { return { int32(FMath::FloorToInt(P.X / Size)), int32(FMath::FloorToInt(P.Y / Size)) }; }
    float SourceHeight(const FKalmalaWorldGenerationConfig& C, FVector2D P)
    {
        return (FKalmalaWorldFieldSampler::Sample(C, P).Elevation - T::SeaElevation) * 2000.0f;
    }
    struct FNode { FVector2D P; uint64 Id; float Height; };
    FNode Candidate(const FKalmalaWorldGenerationConfig& C, FIntPoint Cell, bool Stream)
    {
        const uint64 Id = G::Seed(C, Stream ? 301 : 300, Cell);
        const FVector2D P = (FVector2D(Cell) + FVector2D(0.1 + 0.8 * Unit(Id), 0.1 + 0.8 * Unit(Id >> 24))) * T::RiverSpacing;
        return { P, Id, SourceHeight(C, P) };
    }
    bool Retained(const FKalmalaWorldGenerationConfig& C, FIntPoint Cell, bool Stream)
    {
        const auto A = Candidate(C, Cell, Stream);
        // Coalesce nearby candidates to the lowest stable ID, independent of query order.
        for (int32 Y = -1; Y <= 1; ++Y) for (int32 X = -1; X <= 1; ++X)
        {
            const auto B = Candidate(C, Cell + FIntPoint(X, Y), Stream);
            if (B.Id < A.Id && FVector2D::Distance(A.P, B.P) < T::MergeDistance) return false;
        }
        return true;
    }
    TArray<FKalmalaHydrologySegment> BuildCell(const FKalmalaWorldGenerationConfig& C, FIntPoint Index)
    {
        TArray<FKalmalaHydrologySegment> Result;
        const FVector2D Lower = FVector2D(Index) * T::GridCell;
        const FVector2D Upper = Lower + FVector2D(T::GridCell);
        const auto Min = CellAt(Lower - FVector2D(T::RiverRange + T::SplineAmplitude + 1000), T::RiverSpacing);
        const auto Max = CellAt(Upper + FVector2D(T::RiverRange + T::SplineAmplitude + 1000), T::RiverSpacing);
        for (int32 Kind = 0; Kind < (G::AreStreamsEnabled(C) ? 2 : 1); ++Kind)
        for (int32 Y = Min.Y; Y <= Max.Y; ++Y) for (int32 X = Min.X; X <= Max.X; ++X)
        {
            const bool Stream = Kind == 1;
            const FIntPoint Cell(X, Y);
            const auto A = Candidate(C, Cell, Stream);
            if (A.Height < 80 || A.Height > (Stream ? 560 : 1300) || !Retained(C, Cell, Stream)) continue;
            FNode B = A;
            double Best = TNumericLimits<double>::Max();
            for (int32 DY = -1; DY <= 1; ++DY) for (int32 DX = -1; DX <= 1; ++DX)
            {
                const FIntPoint OtherCell = Cell + FIntPoint(DX, DY);
                const auto Other = Candidate(C, OtherCell, Stream);
                const double Distance = FVector2D::Distance(A.P, Other.P);
                if (Distance < T::MergeDistance || Distance > T::RiverRange || Other.Height >= A.Height - 20
                    || (Stream && (Other.Height < 40 || Other.Height > 560)) || !Retained(C, OtherCell, Stream)) continue;
                const double Score = Distance + Other.Height * 8.0;
                if (Score < Best || (Score == Best && Other.Id < B.Id)) { Best = Score; B = Other; }
            }
            if (B.Id == A.Id) continue;
            const FVector2D Delta = B.P - A.P;
            const double Length = Delta.Size();
            const FVector2D Side(-Delta.Y / Length, Delta.X / Length);
            const int32 Count = FMath::CeilToInt(Length / T::SplineStep);
            const double Phase = Unit(A.Id >> 12) * 2 * PI;
            const double Amplitude = T::SplineAmplitude * (Stream ? 0.35 : 1.0) * (0.5 + Unit(A.Id));
            const double Wavelength = T::SplineWavelength * (0.75 + Unit(B.Id));
            auto Point = [&](double U)
            {
                const double Wave = FMath::Sin(PI * U) * FMath::Sin(U * Length * 2 * PI / Wavelength + Phase);
                const auto P = A.P + Delta * U + Side * (Amplitude * Wave);
                return FVector(P, FMath::Lerp(A.Height, B.Height, U) - 25.0);
            };
            for (int32 I = 0; I < Count; ++I)
            {
                const FVector P = Point(double(I) / Count), Q = Point(double(I + 1) / Count);
                const float Width = Stream ? 220.0f : 600.0f;
                // Spatial index includes the full influence envelope, not just endpoints.
                if (FMath::Max(P.X, Q.X) + Width < Lower.X || FMath::Min(P.X, Q.X) - Width > Upper.X
                    || FMath::Max(P.Y, Q.Y) + Width < Lower.Y || FMath::Min(P.Y, Q.Y) - Width > Upper.Y) continue;
                Result.Add({ P, Q, Width, Stream, A.Id });
            }
        }
        return Result;
    }
    FCriticalSection CacheMutex;
    FKalmalaWorldGenerationConfig CacheIdentity;
    TMap<FIntPoint, TArray<FKalmalaHydrologySegment>> SplineIndex;

    FVector2D Warp(const FKalmalaWorldGenerationConfig& C, FVector2D P)
    {
        return P + FVector2D(G::Noise(C, 200, P, T::WarpFrequency), G::Noise(C, 201, P, T::WarpFrequency)) * T::WarpStrength;
    }

    double WarpedRegion(const FKalmalaWorldGenerationConfig& C, FVector2D P, int32 Biome)
    {
        const uint64 Domain = 100 + Biome * 8;
        const uint64 Bits = G::Seed(C, Domain);
        const FVector2D Offset(Unit(Bits) * T::BiomeScale, Unit(Bits >> 24) * T::BiomeScale);
        P += Offset;
        const auto Base = CellAt(P, T::BiomeScale);
        const double Motion = G::Noise(C, Domain + 1, P, T::RegionFrequency);
        double Weight = 0;
        for (int32 Y = -1; Y <= 1; ++Y) for (int32 X = -1; X <= 1; ++X)
        {
            const FIntPoint Cell = Base + FIntPoint(X, Y);
            const uint64 CellSeed = G::Seed(C, Domain, Cell);
            const FVector2D Center = (FVector2D(Cell) + FVector2D(0.25 + 0.5 * Unit(CellSeed), 0.25 + 0.5 * Unit(CellSeed >> 24))) * T::BiomeScale;
            const FVector2D D = (P - Center) / T::BiomeScale;
            const double Radius = D.Size();
            // Both layers are exactly zero beyond this conservative edge bound.
            const double MaxEdge = FMath::Abs(T::EdgeWaveStrength) + 0.12 * FMath::Abs(Motion);
            if (Radius >= FMath::Max(0.43 + T::RingOverlap, 0.62) + MaxEdge) continue;
            const double Angle = FMath::Atan2(D.Y, D.X);
            const double Edge = (T::EdgeWaveStrength * FMath::Sin(3 * Angle + Unit(CellSeed) * 2 * PI) + 0.12 * Motion) * Smooth(0.0, 0.15, Radius);
            // Two overlapping concentric layers, with independently disturbed inner/outer edges.
            const double Core = 1.0 - Smooth(0.16 + Edge, 0.43 + Edge + T::RingOverlap, Radius);
            const double Ring = Smooth(0.20 - Edge, 0.35 - Edge, Radius)
                * (1.0 - Smooth(0.46 + Edge, 0.62 + Edge, Radius));
            Weight = FMath::Max(Weight, (0.45 + 0.55 * Unit(CellSeed >> 16)) * FMath::Clamp(Core + 0.5 * Ring, 0.0, 1.0));
        }
        return Weight;
    }
}

uint64 FKalmalaRegionalGeneration::Seed(const FKalmalaWorldGenerationConfig& C, uint64 Domain, FIntPoint Cell)
{
    uint64 V = C.WorldSeed ^ (uint64(C.GeneratorRevision) << 32) ^ (Domain * 0x9e3779b97f4a7c15ull)
        ^ (uint64(uint32(Cell.X)) * 0xd6e8feb86659fd93ull) ^ (uint64(uint32(Cell.Y)) * 0xa0761d6478bd642full);
    V = (V ^ (V >> 30)) * 0xbf58476d1ce4e5b9ull;
    V = (V ^ (V >> 27)) * 0x94d049bb133111ebull;
    return V ^ (V >> 31);
}

double FKalmalaRegionalGeneration::Noise(const FKalmalaWorldGenerationConfig& C, uint64 Domain, FVector2D P, double Frequency)
{
    const uint64 S = Seed(C, Domain);
    return FMath::PerlinNoise2D(P * Frequency + FVector2D(Unit(S) * 128, Unit(S >> 24) * 128));
}

TArray<FKalmalaHydrologySegment> FKalmalaRegionalGeneration::GetHydrology(const FKalmalaWorldGenerationConfig& C, FIntPoint Cell)
{
    FScopeLock Lock(&CacheMutex);
    if (!(CacheIdentity == C)) { SplineIndex.Reset(); CacheIdentity = C; }
    if (const auto* Found = SplineIndex.Find(Cell)) return *Found;
    if (SplineIndex.Num() >= T::HydrologyCacheCells) SplineIndex.Reset();
    return SplineIndex.Add(Cell, BuildCell(C, Cell));
}

void FKalmalaRegionalGeneration::ClearHydrologyCache()
{
    FScopeLock Lock(&CacheMutex);
    SplineIndex.Reset();
}

FKalmalaRegionalSample FKalmalaRegionalGeneration::Sample(const FKalmalaWorldGenerationConfig& C, FVector2D P)
{
    return Sample(FKalmalaWorldFieldSampler::Sample(C, P));
}

float FKalmalaRegionalGeneration::DistancePreference(uint8 Biome, double Distance)
{
    // Broad overlapping preferences, never hard concentric biome bands. Lakes
    // and Ocean remain water/terrain decisions rather than progression tiers.
    const double D = FMath::Clamp(Distance / FKalmalaWorldBounds::Radius, 0.0, 1.0);
    const double Preferred[] = {0.0, 0.0, 0.25, 0.50, 0.75, 1.0, 0.0};
    if (Biome == 1 || Biome >= 6) return 1.0f;
    return float(0.08 + 2.92 * FMath::Exp(-FMath::Square((D - Preferred[Biome]) / 0.30)));
}

FKalmalaRegionalSample FKalmalaRegionalGeneration::Sample(const FKalmalaWorldFieldSample& F)
{
    const FKalmalaWorldGenerationConfig C{ F.WorldSeed, F.GeneratorRevision };
    const FVector2D P = F.Position;
    FKalmalaRegionalSample R;
    const float Source = (F.Elevation - T::SeaElevation) * 2000.0f;
    const double Land = Smooth(T::SeaElevation, 0.30, F.Elevation);
    const double Mountain = Smooth(0.68, T::MountainElevation, F.Elevation);
    const FVector2D Warped = Warp(C, P);
    R.Weights[0] = 0.20f + 0.08f * WarpedRegion(C, Warped, 0);
    R.Weights[2] = WarpedRegion(C, Warped, 2) * Smooth(0.22, 0.42, F.Humidity);
    R.Weights[3] = WarpedRegion(C, Warped, 3) * Smooth(0.48, 0.65, F.Humidity)
        * (1 - Smooth(0.43, 0.58, F.Elevation)) * Smooth(0.18, 0.32, F.Temperature);
    R.Weights[4] = WarpedRegion(C, Warped, 4) * (1 - Smooth(0.40, 0.60, F.Temperature)) * Smooth(0.44, 0.64, F.Elevation);
    if (C.GeneratorRevision >= 5)
        for (uint8 I : {uint8(0), uint8(2), uint8(3), uint8(4)}) R.Weights[I] *= DistancePreference(I, P.Size());
    float Total = R.Weights[0] + R.Weights[2] + R.Weights[3] + R.Weights[4];
    for (int32 I = 0; I < 5; ++I) R.Weights[I] = R.Weights[I] / Total * Land * (1 - Mountain);
    R.Weights[5] = Land * Mountain;
    if (C.GeneratorRevision >= 5)
    {
        // High source peaks stay mountains. Lower uplands can increasingly
        // become mountain foothills toward the rim, with continuous blending.
        const float Foothills = (1 - Mountain) * Land * Smooth(0.48, 0.68, F.Elevation)
            * Smooth(0.40, 1.0, P.Size() / FKalmalaWorldBounds::Radius);
        for (int32 I = 0; I < 5; ++I) R.Weights[I] *= 1 - Foothills;
        R.Weights[5] += (1 - R.Weights[5]) * Foothills;
    }
    R.Weights[6] = 1 - Land;
    const double Detail = Noise(C, 250, P, 0.00015);
    const float Shapes[] = { float(40 * Detail), 0, float(65 * Detail), float(-60 + 15 * Detail),
        float(50 * Detail), float(160 * Detail + 100 * WarpedRegion(C, Warped, 5)), float(-60 * (1 - Land) * WarpedRegion(C, Warped, 6)) };
    R.Height = Source;
    for (int32 I = 0; I < 7; ++I) R.Height += R.Weights[I] * Shapes[I] * (I == 6 ? 1 : Land);

    // Revision 4 adds occasional seed-derived emergent islands. They are a
    // smooth deformation of the existing ocean floor, never an actor, map, or
    // saved placement; older identities deliberately retain their terrain.
    double IslandSupport = 0.0;
    if (C.GeneratorRevision >= 4)
    {
        constexpr double IslandSpacing = 60000.0;
        const FIntPoint IslandCell = CellAt(P, IslandSpacing);
        for (int32 Y = -1; Y <= 1; ++Y) for (int32 X = -1; X <= 1; ++X)
        {
            const FIntPoint Cell = IslandCell + FIntPoint(X, Y);
            const uint64 S = Seed(C, 500, Cell);
            const FVector2D Centre = (FVector2D(Cell) + FVector2D(0.2 + 0.6 * Unit(S), 0.2 + 0.6 * Unit(S >> 24))) * IslandSpacing;
            const double Radius = 6500.0 + 3500.0 * Unit(S >> 12);
            const double Distance = FVector2D::Distance(P, Centre) / Radius;
            const double Support = 1.0 - Smooth(1.0, 1.8, Distance);
            if (Support <= IslandSupport) continue;
            const float Summit = 240.0f + 360.0f * Unit(S >> 36);
            // A gentle shore rises above sea only in the central support, with
            // a broad submerged apron that joins the existing ocean floor.
            const float IslandHeight = FMath::Lerp(-220.0f, Summit, float(Smooth(0.78, 0.0, Distance)));
            R.Height = FMath::Lerp(R.Height, IslandHeight, float(Support));
            IslandSupport = Support;
        }
        if (IslandSupport > 0.0)
        {
            const float IslandLand = float(Smooth(0.45, 0.85, IslandSupport));
            R.Weights[0] = FMath::Max(R.Weights[0], IslandLand);
            R.Weights[6] *= 1.0f - IslandLand;
        }
    }

    // Analytic enclosed bowls: source-qualified lowland centers, continuous raised rims.
    // Centers are separated by at least 28,000 cm; bowl supports never overlap.
    const auto BasinCell = CellAt(P, T::BasinSpacing);
    double BasinSupport = 0;
    for (int32 Y = -1; Y <= 1; ++Y) for (int32 X = -1; X <= 1; ++X)
    {
        const auto Cell = BasinCell + FIntPoint(X, Y);
        const uint64 S = Seed(C, 400, Cell);
        const FVector2D Center = (FVector2D(Cell) + FVector2D(0.35 + Unit(S) * 0.3, 0.35 + Unit(S >> 24) * 0.3)) * T::BasinSpacing;
        const double Radius = 4000 + 4500 * Unit(S >> 12);
        const FVector2D Delta = P - Center;
        const double Aspect = 0.65 + 0.35 * Unit(S >> 32);
        const double D = FVector2D(Delta.X, Delta.Y / Aspect).Size() / Radius;
        if (D >= 1.5) continue;
        // Reject out-of-support bowls before evaluating their fields and region.
        const auto CenterFields = FKalmalaWorldFieldSampler::Sample(C, Center);
        if (CenterFields.Elevation < 0.32f || CenterFields.Elevation > 0.58f || CenterFields.Humidity < 0.4f) continue;
        if (WarpedRegion(C, Warp(C, Center), 1) < 0.25) continue;
        const double Support = 1 - Smooth(1.0, 1.5, D);
        const float Level = (CenterFields.Elevation - T::SeaElevation) * 2000.0f;
        const float Bowl = Level - 120.0f + 240.0f * D * D;
        R.Height = FMath::Lerp(R.Height, Bowl, float(Support));
        R.WaterLevel = Level;
        BasinSupport = Support;
        R.BasinWeight = 1 - Smooth(0.65, 1.0, D);
        for (float& W : R.Weights) W *= 1 - R.BasinWeight;
        R.Weights[1] = R.BasinWeight;
    }

    double Sum = 0, Water = 0, Influence = 0;
    const auto Segments = GetHydrology(C, CellAt(P, T::GridCell));
    for (const auto& S : Segments)
    {
        const FVector2D A(S.A), B(S.B), D = B - A;
        const double U = FMath::Clamp(FVector2D::DotProduct(P - A, D) / D.SizeSquared(), 0.0, 1.0);
        const double Distance = FVector2D::Distance(P, A + D * U);
        const double W = 1 - Smooth(S.Width * 0.2, S.Width, Distance);
        if (W <= 0) continue;
        const double Level = FMath::Lerp(S.A.Z, S.B.Z, U);
        Sum += W;
        Water += W * Level;
        Influence = FMath::Max(Influence, W);
        if (S.bStream) R.StreamWeight = FMath::Max(R.StreamWeight, float(W));
        else R.RiverWeight = FMath::Max(R.RiverWeight, float(W));
    }
    if (Sum > 0 && BasinSupport < 1)
    {
        const float Level = Water / Sum;
        const float W = Influence * (1 - BasinSupport);
        R.Height = FMath::Lerp(R.Height, FMath::Min(R.Height, Level - 80.0f), W);
        if (BasinSupport == 0) R.WaterLevel = FMath::Min(Level, R.Height + 80.0f * W);
    }
    // Shores retain the source sea-level sign outside inland basins/courses.
    if (BasinSupport == 0 && Sum == 0) R.WaterLevel = FMath::Min(0.0f, R.Height - 1.0f);
    if (F.Elevation < T::SeaElevation && BasinSupport == 0 && IslandSupport == 0) R.Height = FMath::Min(R.Height, Source);
    R.bHasWater = R.WaterLevel > R.Height;
    for (uint8 I = 1; I < 6; ++I) if (R.Weights[I] > R.Weights[R.Biome]) R.Biome = I;
    if (F.Elevation < T::SeaElevation && IslandSupport < 0.5) R.Biome = 6;
    else if (F.Elevation > T::MountainElevation) R.Biome = 5;
    return R;
}
