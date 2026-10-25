#include "KalmalaRegionalGeneration.h"
#include "Misc/ScopeLock.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaGenerationPreview.h"

namespace
{
    using T = FKalmalaRegionalTuning;
    using G = FKalmalaRegionalGeneration;
    double Smooth(double A, double B, double X)
    {
        const double V = FMath::Clamp((X - A) / (B - A), 0.0, 1.0);
        return V * V * (3.0 - 2.0 * V);
    }
    double RegionalUnit(uint64 Bits) { return double(Bits & 0xffffff) / 16777216.0; }
    struct FSegmentProjection { double U; double Distance; };
    FSegmentProjection ProjectHydrologySegment(FVector2D P, const FKalmalaHydrologySegment& S)
    {
        const FVector2D A(S.A), B(S.B), D = B - A;
        const double U = FMath::Clamp(FVector2D::DotProduct(P - A, D) / D.SizeSquared(), 0.0, 1.0);
        const double Distance = FVector2D::Distance(P, A + D * U);
        return { U, Distance };
    }
    FIntPoint CellAt(FVector2D P, double Size) { return { int32(FMath::FloorToInt(P.X / Size)), int32(FMath::FloorToInt(P.Y / Size)) }; }
    float SourceHeight(const FKalmalaWorldGenerationConfig& C, FVector2D P)
    {
        return (FKalmalaWorldFieldSampler::Sample(C, P).Elevation - T::SeaElevation) * 2000.0f;
    }
    struct FNode { FVector2D P; uint64 Id; float Height; };
    FNode Candidate(const FKalmalaWorldGenerationConfig& C, FIntPoint Cell, bool Stream)
    {
        const uint64 Id = G::Seed(C, Stream ? 301 : 300, Cell);
        const FVector2D P = (FVector2D(Cell) + FVector2D(0.1 + 0.8 * RegionalUnit(Id), 0.1 + 0.8 * RegionalUnit(Id >> 24))) * T::RiverSpacing;
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
        const auto Min = CellAt(Lower - FVector2D(T::RiverRange + T::SplineAmplitude + T::RiverBankExtent + 1000), T::RiverSpacing);
        const auto Max = CellAt(Upper + FVector2D(T::RiverRange + T::SplineAmplitude + T::RiverBankExtent + 1000), T::RiverSpacing);
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
            const double Phase = RegionalUnit(A.Id >> 12) * 2 * PI;
            const double Amplitude = T::SplineAmplitude * (Stream ? 0.35 : 1.0) * (0.5 + RegionalUnit(A.Id));
            const double Wavelength = T::SplineWavelength * (0.75 + RegionalUnit(B.Id));
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
                // Spatial index includes the bank shoulder, not just the wet channel.
                const double Extent = Width + (Stream ? 0.0 : T::RiverBankExtent);
                if (FMath::Max(P.X, Q.X) + Extent < Lower.X || FMath::Min(P.X, Q.X) - Extent > Upper.X
                    || FMath::Max(P.Y, Q.Y) + Extent < Lower.Y || FMath::Min(P.Y, Q.Y) - Extent > Upper.Y) continue;
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
        const auto& Preview = FKalmalaGenerationPreview::Get();
        return P + FVector2D(G::Noise(C, 200, P, T::WarpFrequency), G::Noise(C, 201, P, T::WarpFrequency)) * Preview.WarpStrength;
    }

    double WarpedRegion(const FKalmalaWorldGenerationConfig& C, FVector2D P, int32 Biome)
    {
        const auto& Preview = FKalmalaGenerationPreview::Get();
        const uint64 Domain = 100 + Biome * 8;
        const uint64 Bits = G::Seed(C, Domain);
        const FVector2D Offset(RegionalUnit(Bits) * Preview.BiomeScale, RegionalUnit(Bits >> 24) * Preview.BiomeScale);
        P += Offset;
        const auto Base = CellAt(P, Preview.BiomeScale);
        const double Motion = G::Noise(C, Domain + 1, P, (1.0 / Preview.BiomeScale));
        double Weight = 0;
        for (int32 Y = -1; Y <= 1; ++Y) for (int32 X = -1; X <= 1; ++X)
        {
            const FIntPoint Cell = Base + FIntPoint(X, Y);
            const uint64 CellSeed = G::Seed(C, Domain, Cell);
            const FVector2D Center = (FVector2D(Cell) + FVector2D(0.25 + 0.5 * RegionalUnit(CellSeed), 0.25 + 0.5 * RegionalUnit(CellSeed >> 24))) * Preview.BiomeScale;
            const FVector2D D = (P - Center) / Preview.BiomeScale;
            const double Radius = D.Size();
            // Both layers are exactly zero beyond this conservative edge bound.
            const double MaxEdge = FMath::Abs(T::EdgeWaveStrength) + 0.12 * FMath::Abs(Motion);
            if (Radius >= FMath::Max(0.43 + T::RingOverlap, 0.62) + MaxEdge) continue;
            const double Angle = FMath::Atan2(D.Y, D.X);
            const double Edge = (T::EdgeWaveStrength * FMath::Sin(3 * Angle + RegionalUnit(CellSeed) * 2 * PI) + 0.12 * Motion) * Smooth(0.0, 0.15, Radius);
            // Two overlapping concentric layers, with independently disturbed inner/outer edges.
            const double Core = 1.0 - Smooth(0.16 + Edge, 0.43 + Edge + T::RingOverlap, Radius);
            const double Ring = Smooth(0.20 - Edge, 0.35 - Edge, Radius)
                * (1.0 - Smooth(0.46 + Edge, 0.62 + Edge, Radius));
            Weight = FMath::Max(Weight, (0.45 + 0.55 * RegionalUnit(CellSeed >> 16)) * FMath::Clamp(Core + 0.5 * Ring, 0.0, 1.0));
        }
        return Weight;
    }
}

uint64 FKalmalaRegionalGeneration::Seed(const FKalmalaWorldGenerationConfig& C, uint64 Domain, FIntPoint Cell)
{
    uint64 V = C.WorldSeed ^ (Domain * 0x9e3779b97f4a7c15ull)
        ^ (uint64(uint32(Cell.X)) * 0xd6e8feb86659fd93ull) ^ (uint64(uint32(Cell.Y)) * 0xa0761d6478bd642full);
    V = (V ^ (V >> 30)) * 0xbf58476d1ce4e5b9ull;
    V = (V ^ (V >> 27)) * 0x94d049bb133111ebull;
    return V ^ (V >> 31);
}

double FKalmalaRegionalGeneration::Noise(const FKalmalaWorldGenerationConfig& C, uint64 Domain, FVector2D P, double Frequency)
{
    const uint64 S = Seed(C, Domain);
    return FMath::PerlinNoise2D(P * Frequency + FVector2D(RegionalUnit(S) * 128, RegionalUnit(S >> 24) * 128));
}

FKalmalaTerrainReliefSample FKalmalaRegionalGeneration::SampleSharedRelief(
    const FKalmalaWorldGenerationConfig& C, const FVector2D AbsoluteWorldPosition)
{
    const auto& Preview = FKalmalaGenerationPreview::Get();
    FKalmalaTerrainReliefSample Result;
    Result.Broad = Noise(C, T::ReliefBroadSalt, AbsoluteWorldPosition, 1.0 / Preview.ReliefBroadScale);
    Result.Middle = Noise(C, T::ReliefMiddleSalt, AbsoluteWorldPosition, 1.0 / Preview.ReliefMiddleScale);
    Result.Local = Noise(C, T::ReliefLocalSalt, AbsoluteWorldPosition, 1.0 / Preview.ReliefLocalScale);
    return Result;
}

double FKalmalaRegionalGeneration::ShapeMeadows(const double SourceHeight, const double Detail)
{
    return SourceHeight + double(float(40 * Detail));
}

double FKalmalaRegionalGeneration::CompressMeadowShoulder(
    const double Height, const double ShoulderHeight, const double ShoulderCompressionScale)
{
    if (Height <= ShoulderHeight) return Height;
    const double Excess = Height - ShoulderHeight;
    return ShoulderHeight + ShoulderCompressionScale * (1.0 - FMath::Exp(-Excess / ShoulderCompressionScale));
}

double FKalmalaRegionalGeneration::ShapeMeadows(
    const double SourceHeight, const double HillNoise, const double HillAmplitude,
    const double ShoulderHeight, const double ShoulderCompressionScale)
{
    return ShapeMeadows(SourceHeight, HillNoise, HillAmplitude, 0.0, 0.0,
        ShoulderHeight, ShoulderCompressionScale);
}

double FKalmalaRegionalGeneration::ShapeMeadows(
    const double SourceHeight, const double HillNoise, const double HillAmplitude,
    const double RollingNoise, const double RollingAmplitude, const double ShoulderHeight,
    const double ShoulderCompressionScale)
{
    return ShapeMeadows(SourceHeight, HillNoise, HillAmplitude, RollingNoise, RollingAmplitude,
        0.0, 0.0, ShoulderHeight, ShoulderCompressionScale);
}

double FKalmalaRegionalGeneration::ShapeMeadows(
    const double SourceHeight, const double HillNoise, const double HillAmplitude,
    const double RollingNoise, const double RollingAmplitude, const double FloorNoise,
    const double FloorAmplitude, const double ShoulderHeight, const double ShoulderCompressionScale)
{
    const double HeightWithHills = SourceHeight
        + double(float(HillNoise * HillAmplitude))
        + double(float(RollingNoise * RollingAmplitude))
        + double(float(FloorNoise * FloorAmplitude));
    return CompressMeadowShoulder(HeightWithHills, ShoulderHeight, ShoulderCompressionScale);
}

double FKalmalaRegionalGeneration::ShapeElderwood(
    const double SourceHeight, const double Detail, const FKalmalaTerrainReliefSample& SharedRelief,
    const double BroadAmplitude, const double MiddleAmplitude, const double LocalAmplitude)
{
    const double SharedBase = SourceHeight
        + double(float(SharedRelief.Broad * BroadAmplitude))
        + double(float(SharedRelief.Middle * MiddleAmplitude))
        + double(float(SharedRelief.Local * LocalAmplitude));
    return SharedBase + double(float(-60 + 15 * Detail));
}

double FKalmalaRegionalGeneration::CompressShimmeringLakesLowland(
    const double SourceHeight, const double Humidity, const double HumidityMinimum,
    const double HumidityFull, const double LowlandFloor, const double LowlandCeiling,
    const double Compression)
{
    if (!FMath::IsFinite(SourceHeight) || !FMath::IsFinite(Humidity)
        || !FMath::IsFinite(HumidityMinimum) || !FMath::IsFinite(HumidityFull)
        || !FMath::IsFinite(LowlandFloor) || !FMath::IsFinite(LowlandCeiling)
        || !FMath::IsFinite(Compression) || HumidityFull <= HumidityMinimum
        || LowlandCeiling <= LowlandFloor)
    {
        return SourceHeight;
    }

    const double BandWidth = (LowlandCeiling - LowlandFloor) * 0.25;
    const double LowlandEntry = Smooth(LowlandFloor, LowlandFloor + BandWidth, SourceHeight);
    const double LowlandExit = 1.0 - Smooth(LowlandCeiling - BandWidth, LowlandCeiling, SourceHeight);
    const double LowlandBand = LowlandEntry * LowlandExit;
    const double HumidityWeight = Smooth(HumidityMinimum, HumidityFull, Humidity);
    const double Strength = FMath::Clamp(Compression, 0.0, 0.35) * HumidityWeight * LowlandBand;
    const double AboveFloor = FMath::Max(0.0, SourceHeight - LowlandFloor);
    return SourceHeight - AboveFloor * Strength;
}

double FKalmalaRegionalGeneration::ShapeShimmeringLakes(
    const double SourceHeight, const double MeadowCandidate, const double ElderwoodCandidate,
    const double Humidity, const double HumidityMinimum, const double HumidityFull,
    const double MeadowShare, const double LowlandFloor, const double LowlandCeiling,
    const double LowlandCompression)
{
    const double RoundedShoulders = FMath::Lerp(
        ElderwoodCandidate, MeadowCandidate, FMath::Clamp(MeadowShare, 0.0, 1.0));
    const double CompressedSource = CompressShimmeringLakesLowland(
        SourceHeight, Humidity, HumidityMinimum, HumidityFull,
        LowlandFloor, LowlandCeiling, LowlandCompression);
    return RoundedShoulders + (CompressedSource - SourceHeight);
}

double FKalmalaRegionalGeneration::ShapeMire(
    const double SourceHeight, const double Detail, const double TargetHeight,
    const double CompressionScale, const double ResidualDetailAmplitude)
{
    if (!FMath::IsFinite(SourceHeight) || !FMath::IsFinite(Detail)
        || !FMath::IsFinite(TargetHeight) || !FMath::IsFinite(CompressionScale)
        || !FMath::IsFinite(ResidualDetailAmplitude) || CompressionScale <= 0.0
        || ResidualDetailAmplitude < 0.0)
    {
        return SourceHeight;
    }

    // The rational response is strictly monotonic and asymptotes to a narrow
    // band instead of clamping into a flat shelf. The selected tuning bounds
    // keep even the lowest source-land sample above zero after residual detail.
    const double Offset = SourceHeight - TargetHeight;
    const double CompressedOffset = Offset / (1.0 + FMath::Abs(Offset) / CompressionScale);
    const double Residual = FMath::Clamp(Detail, -1.0, 1.0) * ResidualDetailAmplitude;
    return TargetHeight + CompressedOffset + Residual;
}

double FKalmalaRegionalGeneration::ShapeTundra(
    const double SourceHeight, const double Detail, const FKalmalaTerrainReliefSample& SharedRelief,
    const double BroadAmplitude, const double MiddleAmplitude, const double LocalAmplitude)
{
    return ShapeElderwood(SourceHeight, Detail, SharedRelief,
        BroadAmplitude, MiddleAmplitude, LocalAmplitude);
}

FKalmalaTerrainCandidateDiagnostics FKalmalaRegionalGeneration::SampleTerrainCandidateDiagnostics(
    const FKalmalaWorldGenerationConfig& C, const FKalmalaWorldFieldSample& F)
{
    const auto& Preview = FKalmalaGenerationPreview::Get();
    const FVector2D P = F.Position;
    FKalmalaTerrainCandidateDiagnostics Result;
    const float SourceHeight = (F.Elevation - T::SeaElevation) * 2000.0f;
    Result.SourceHeight = SourceHeight;
    Result.WetlandHumidityMinimum = Preview.WetlandHumidityMinimum;
    Result.WetlandHumidityFull = Preview.WetlandHumidityFull;
    const double Detail = Noise(C, 250, P, 0.00015);
    const double MeadowHill = Noise(C, T::MeadowHillSalt, P, 1.0 / Preview.MeadowHillScale);
    const double MeadowRolling = Noise(C, T::MeadowRollingSalt, P, 1.0 / Preview.MeadowRollingScale);
    const double MeadowFloor = Noise(C, T::MeadowFloorSalt, P, 1.0 / Preview.MeadowFloorScale);
    Result.SharedRelief = SampleSharedRelief(C, P);
    Result.MeadowCandidate = ShapeMeadows(Result.SourceHeight, MeadowHill, Preview.MeadowHillAmplitude,
        MeadowRolling, Preview.MeadowRollingAmplitude, MeadowFloor, Preview.MeadowFloorAmplitude,
        Preview.MeadowShoulderHeight, Preview.MeadowShoulderCompressionScale);
    Result.ElderwoodCandidate = ShapeElderwood(Result.SourceHeight, Detail, Result.SharedRelief,
        Preview.ElderwoodBroadAmplitude, Preview.ElderwoodMiddleAmplitude, Preview.ElderwoodLocalAmplitude);
    Result.TundraCandidate = ShapeTundra(Result.SourceHeight, Detail, Result.SharedRelief,
        Preview.ElderwoodBroadAmplitude, Preview.ElderwoodMiddleAmplitude, Preview.ElderwoodLocalAmplitude);
    Result.ShimmeringLakesLowlandCandidate = CompressShimmeringLakesLowland(
        Result.SourceHeight, F.Humidity, Preview.WetlandHumidityMinimum, Preview.WetlandHumidityFull,
        Preview.ShimmeringLakesLowlandFloor, Preview.ShimmeringLakesLowlandCeiling,
        Preview.ShimmeringLakesLowlandCompression);
    Result.ShimmeringLakesCandidate = ShapeShimmeringLakes(Result.SourceHeight, Result.MeadowCandidate,
        Result.ElderwoodCandidate, F.Humidity, Preview.WetlandHumidityMinimum, Preview.WetlandHumidityFull,
        Preview.ShimmeringLakesMeadowShare, Preview.ShimmeringLakesLowlandFloor,
        Preview.ShimmeringLakesLowlandCeiling, Preview.ShimmeringLakesLowlandCompression);
    Result.MireCandidate = ShapeMire(Result.SourceHeight, Detail, Preview.MireTargetHeight,
        Preview.MireCompressionScale, Preview.MireResidualDetailAmplitude);
    return Result;
}

double FKalmalaRegionalGeneration::ShapeMountains(
    const double SourceHeight, const double Detail, const double MountainRegion,
    const double HighlandThreshold, const double HighlandBlendRange, const double HighlandGain,
    const double DetailAmplitude)
{
    if (!FMath::IsFinite(SourceHeight) || !FMath::IsFinite(Detail) || !FMath::IsFinite(MountainRegion)
        || !FMath::IsFinite(HighlandThreshold) || !FMath::IsFinite(HighlandBlendRange)
        || !FMath::IsFinite(HighlandGain) || !FMath::IsFinite(DetailAmplitude)
        || HighlandBlendRange <= 0.0 || HighlandGain < 0.0 || DetailAmplitude < 0.0)
    {
        return SourceHeight;
    }

    const double HighBaseBlend = Smooth(HighlandThreshold,
        HighlandThreshold + HighlandBlendRange, SourceHeight);
    const double HeightAboveThreshold = FMath::Max(0.0, SourceHeight - HighlandThreshold);
    const double HighlandAmplification = HeightAboveThreshold * HighBaseBlend
        * FMath::Clamp(MountainRegion, 0.0, 1.0) * FMath::Clamp(HighlandGain, 0.0, 0.8);
    const double BoundedDetail = FMath::Clamp(Detail, -1.0, 1.0)
        * FMath::Clamp(DetailAmplitude, 0.0, 160.0);
    return SourceHeight + double(float(HighlandAmplification)) + double(float(BoundedDetail));
}

float FKalmalaRegionalGeneration::BlendCandidateHeights(
    const float SourceHeight, const double CandidateHeights[6], const float OceanOffset, const float Weights[7])
{
    float Result = SourceHeight;
    for (int32 Index = 0; Index < 6; ++Index)
    {
        // Land weights are normalized against the six land biomes, then already
        // attenuated by the continuous coast factor. Do not apply that factor twice.
        const float Offset = float(CandidateHeights[Index] - double(SourceHeight));
        Result += Weights[Index] * Offset;
    }
    Result += Weights[6] * OceanOffset;
    return Result;
}

double FKalmalaRegionalGeneration::PreserveSourceLandSign(
    const double Height, const double SourceHeight, const double TransitionScale)
{
    if (!FMath::IsFinite(Height) || !FMath::IsFinite(SourceHeight)
        || !FMath::IsFinite(TransitionScale) || SourceHeight <= 0.0 || TransitionScale <= 0.0)
    {
        return Height;
    }

    // A source-relative cap makes the transition vanish with source clearance
    // at the coast. Stable softplus is identity far above zero and approaches
    // zero smoothly below it, avoiding a broad flat minimum-height shelf.
    constexpr double LnTwo = 0.69314718055994530942;
    const double Scale = FMath::Min(TransitionScale, SourceHeight / LnTwo);
    const double ScaledHeight = Height / Scale;
    if (ScaledHeight >= 40.0) return Height;
    return Scale * FMath::Loge(1.0 + FMath::Exp(FMath::Max(ScaledHeight, -80.0)));
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

FKalmalaRegionalSample FKalmalaRegionalGeneration::Sample(const FKalmalaWorldFieldSample& F)
{
    return SampleInternal(F, false);
}

uint8 FKalmalaRegionalGeneration::SampleBiome(const FKalmalaWorldFieldSample& F)
{
    return SampleInternal(F, true).Biome;
}

FKalmalaRegionalSample FKalmalaRegionalGeneration::SampleInternal(const FKalmalaWorldFieldSample& F, bool bBiomeOnly, bool bSkipHydrology)
{
    const auto& Preview = FKalmalaGenerationPreview::Get();
    const FKalmalaWorldGenerationConfig C{ F.WorldSeed };
    const FVector2D P = F.Position;
    const double OriginDistance = P.Size();
    FKalmalaRegionalSample R;
    const float Source = (F.Elevation - T::SeaElevation) * 2000.0f;
    const double Land = Smooth(T::SeaElevation, 0.30, F.Elevation);
    const double Mountain = Smooth(0.68, T::MountainElevation, F.Elevation);
    const FVector2D Warped = Warp(C, P);
    auto RangeWeight = [&](double Min, double Max)
    {
        // Inclusive eligibility, smoothly tapered entirely inside each range.
        const double Width = FMath::Min(Preview.EligibilityBlend, (Max - Min) * .5);
        return Smooth(Min, Min + Width, OriginDistance) * (1 - Smooth(Max - Width, Max, OriginDistance));
    };
    const double Wetland = WarpedRegion(C, Warped, 3)
        * Smooth(Preview.WetlandHumidityMinimum, Preview.WetlandHumidityFull, F.Humidity)
        * (1 - Smooth(Preview.WetlandElevationFull, Preview.WetlandElevationMaximum, F.Elevation))
        * Smooth(Preview.WetlandTemperatureMinimum, Preview.WetlandTemperatureFull, F.Temperature);
    R.Weights[1] = Wetland * RangeWeight(Preview.LakesMinimum, Preview.LakesMaximum);
    R.Weights[3] = Wetland * RangeWeight(Preview.MireMinimum, Preview.MireMaximum);
    R.Weights[2] = WarpedRegion(C, Warped, 2) * Smooth(0.22, 0.42, F.Humidity)
        * Smooth(Preview.ElderwoodMinimum, Preview.ElderwoodMinimum + Preview.EligibilityBlend, OriginDistance);
    R.Weights[4] = WarpedRegion(C, Warped, 4) * (1 - Smooth(0.40, 0.60, F.Temperature))
        * Smooth(0.44, 0.64, F.Elevation)
        * Smooth(Preview.TundraMinimum, Preview.TundraMinimum + Preview.EligibilityBlend, OriginDistance);
    const double MountainFallback = FMath::Max(Mountain * Smooth(Preview.StarterRadius, Preview.StarterRadius + Preview.EligibilityBlend, OriginDistance),
        Smooth(Preview.MeadowsMaximum - Preview.EligibilityBlend, Preview.MeadowsMaximum, OriginDistance));
    const float Fallback = 0.20f + 0.08f * WarpedRegion(C, Warped, 0);
    R.Weights[0] = Fallback * (1 - MountainFallback);
    R.Weights[5] = Fallback * MountainFallback;
    float Total = 0;
    for (int32 I = 0; I < 6; ++I) Total += R.Weights[I];
    for (int32 I = 0; I < 6; ++I) R.Weights[I] = R.Weights[I] / Total * Land;
    R.Weights[6] = 1 - Land;
    R.Height = Source;
    if (!bBiomeOnly)
    {
        const double Detail = Noise(C, 250, P, 0.00015);
        const bool bNeedsMeadowCandidate = R.Weights[0] > 0.0f || R.Weights[1] > 0.0f;
        const double MeadowHill = bNeedsMeadowCandidate
            ? Noise(C, T::MeadowHillSalt, P, 1.0 / Preview.MeadowHillScale)
            : 0.0;
        const double MeadowRolling = bNeedsMeadowCandidate
            ? Noise(C, T::MeadowRollingSalt, P, 1.0 / Preview.MeadowRollingScale)
            : 0.0;
        const double MeadowFloor = bNeedsMeadowCandidate
            ? Noise(C, T::MeadowFloorSalt, P, 1.0 / Preview.MeadowFloorScale)
            : 0.0;
        FKalmalaTerrainReliefSample ElderwoodRelief;
        if (R.Weights[1] > 0.0f || R.Weights[2] > 0.0f || R.Weights[4] > 0.0f)
        {
            ElderwoodRelief = SampleSharedRelief(C, P);
        }
        const double ElderwoodHeight = ShapeElderwood(Source, Detail, ElderwoodRelief,
            Preview.ElderwoodBroadAmplitude, Preview.ElderwoodMiddleAmplitude, Preview.ElderwoodLocalAmplitude);
        const double MeadowHeight = ShapeMeadows(Source, MeadowHill, Preview.MeadowHillAmplitude,
            MeadowRolling, Preview.MeadowRollingAmplitude, MeadowFloor, Preview.MeadowFloorAmplitude,
            Preview.MeadowShoulderHeight, Preview.MeadowShoulderCompressionScale);
        const double ShimmeringLakesHeight = ShapeShimmeringLakes(Source, MeadowHeight, ElderwoodHeight,
            F.Humidity, Preview.WetlandHumidityMinimum, Preview.WetlandHumidityFull,
            Preview.ShimmeringLakesMeadowShare, Preview.ShimmeringLakesLowlandFloor,
            Preview.ShimmeringLakesLowlandCeiling, Preview.ShimmeringLakesLowlandCompression);
        const double CandidateHeights[] = {
            MeadowHeight,
            ShimmeringLakesHeight,
            ElderwoodHeight,
            ShapeMire(Source, Detail, Preview.MireTargetHeight,
                Preview.MireCompressionScale, Preview.MireResidualDetailAmplitude),
            ShapeTundra(Source, Detail, ElderwoodRelief,
                Preview.ElderwoodBroadAmplitude, Preview.ElderwoodMiddleAmplitude, Preview.ElderwoodLocalAmplitude),
            ShapeMountains(Source, Detail, WarpedRegion(C, Warped, 5),
                Preview.MountainHighlandThreshold, Preview.MountainHighlandBlendRange,
                Preview.MountainHighlandGain, Preview.MountainDetailAmplitude),
        };
        const float OceanOffset = float(-60 * (1 - Land) * WarpedRegion(C, Warped, 6));
        R.Height = BlendCandidateHeights(Source, CandidateHeights, OceanOffset, R.Weights);
    }

    double BasinSupport = 0;
    if (T::bEnableAnalyticBasins)
    {
        // Analytic enclosed bowls use source-qualified lowland centers and
        // continuous raised rims. This compiled policy is the sole switch for
        // their height, water-level and BasinWeight outputs.
        const auto BasinCell = CellAt(P, T::BasinSpacing);
        for (int32 Y = -1; Y <= 1; ++Y) for (int32 X = -1; X <= 1; ++X)
        {
            const auto Cell = BasinCell + FIntPoint(X, Y);
            const uint64 S = Seed(C, 400, Cell);
            const FVector2D Center = (FVector2D(Cell) + FVector2D(0.35 + RegionalUnit(S) * 0.3, 0.35 + RegionalUnit(S >> 24) * 0.3)) * T::BasinSpacing;
            const double Radius = 4000 + 4500 * RegionalUnit(S >> 12);
            const FVector2D Delta = P - Center;
            const double Aspect = 0.65 + 0.35 * RegionalUnit(S >> 32);
            const double D = FVector2D(Delta.X, Delta.Y / Aspect).Size() / Radius;
            if (D >= 1.5) continue;
            // Keep the entire bowl/rim beyond the starter boundary and on atlas
            // land. Fade its influence at coast/distance edges without a height step.
            if (Center.Size() - Radius * 1.5 <= Preview.StarterRadius || F.Elevation <= T::SeaElevation) continue;
            // Reject out-of-support bowls before evaluating their fields and region.
            const auto CenterFields = FKalmalaWorldFieldSampler::Sample(C, Center);
            const double CenterWetland = WarpedRegion(C, Warp(C, Center), 3)
                * Smooth(Preview.WetlandHumidityMinimum, Preview.WetlandHumidityFull, CenterFields.Humidity)
                * (1 - Smooth(Preview.WetlandElevationFull, Preview.WetlandElevationMaximum, CenterFields.Elevation))
                * Smooth(Preview.WetlandTemperatureMinimum, Preview.WetlandTemperatureFull, CenterFields.Temperature);
            if (CenterFields.Elevation < 0.32f || CenterWetland < 0.25) continue;
            // Lakes and Mire use exactly the same bowl parameters and suitability.
            // Bowls shape terrain/water only; they never override the distance-gated label.
            const double LakeEligibility = FMath::Clamp(double(R.Weights[1] + R.Weights[3]) * 2, 0.0, 1.0);
            if (LakeEligibility <= 0) continue;
            const double Support = (1 - Smooth(1.0, 1.5, D)) * LakeEligibility;
            const float Level = (CenterFields.Elevation - T::SeaElevation) * 2000.0f;
            const float Bowl = Level - 120.0f + 240.0f * D * D;
            R.Height = FMath::Lerp(R.Height, Bowl, float(Support));
            R.WaterLevel = Level;
            BasinSupport = Support;
            R.BasinWeight = (1 - Smooth(0.65, 1.0, D)) * LakeEligibility;
        }
    }

    double Sum = 0, Water = 0, Influence = 0, RiverBankInfluence = 0;
    const auto Segments = (bBiomeOnly || bSkipHydrology) ? TArray<FKalmalaHydrologySegment>() : GetHydrology(C, CellAt(P, T::GridCell));
    for (const auto& S : Segments)
    {
        const auto Projection = ProjectHydrologySegment(P, S);
        const double U = Projection.U;
        const double Distance = Projection.Distance;
        const double W = 1 - Smooth(S.Width * 0.2, S.Width, Distance);
        if (!S.bStream)
        {
            R.RiverWeight = FMath::Max(R.RiverWeight, float(W));
            // One smooth distance profile spans the bed and both bank shoulders.
            const double Bank = 1 - Smooth(S.Width * 0.2, S.Width + T::RiverBankExtent, Distance);
            RiverBankInfluence = FMath::Max(RiverBankInfluence, Bank);
            continue;
        }
        if (W <= 0) continue;
        const double Level = FMath::Lerp(S.A.Z, S.B.Z, U);
        Sum += W;
        Water += W * Level;
        Influence = FMath::Max(Influence, W);
        R.StreamWeight = FMath::Max(R.StreamWeight, float(W));
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
    {
        // Preserve the source coast sign without a fixed-height land shelf.
        R.Height = F.Elevation <= T::SeaElevation ? FMath::Min(R.Height, Source)
            : float(PreserveSourceLandSign(R.Height, Source, T::CoastSignTransitionScale));
        if (F.Elevation <= T::SeaElevation) R.WaterLevel = 0;
    }
    if (RiverBankInfluence > 0 && BasinSupport < 1)
    {
        // Carve after the land clamp so high ground cannot refill the channel.
        // Maximum influence across segments avoids repeated carving at joins.
        // The bank envelope extends beyond RiverWeight. Fade its target from
        // the positive land floor into the full bed as narrow channel weight
        // rises, so relief cannot turn a dry bank into new sea-level water.
        const float W = RiverBankInfluence * (1 - BasinSupport);
        // Lift the channel target toward the retained basin surface through its
        // continuous rim support instead of targeting sea depth throughout a lake.
        // Full basin support keeps both its original bed and water level.
        const float ChannelLevel = FMath::Lerp(0.0f, R.WaterLevel, float(BasinSupport));
        const float Bed = ChannelLevel - float(T::RiverBedDepth);
        const float BankFloor = F.Elevation > T::SeaElevation ? FMath::Min(R.Height, 1.0f) : R.Height;
        const float ChannelShare = float(Smooth(0.0, 1.0, R.RiverWeight));
        const float CarveTarget = FMath::Lerp(BankFloor, Bed, ChannelShare);
        R.Height = FMath::Lerp(R.Height, FMath::Min(R.Height, CarveTarget), W);
        if (BasinSupport == 0) R.WaterLevel = 0;
    }
    R.bHasWater = R.WaterLevel > R.Height;
    for (uint8 I = 1; I < 6; ++I) if (R.Weights[I] > R.Weights[R.Biome]) R.Biome = I;
    {
        if (F.Elevation <= T::SeaElevation)
        {
            for (float& W : R.Weights) W = 0;
            R.Weights[6] = 1;
            R.Biome = 6;
        }
        else if (OriginDistance <= Preview.StarterRadius) R.Biome = 0;
    }
    return R;
}
