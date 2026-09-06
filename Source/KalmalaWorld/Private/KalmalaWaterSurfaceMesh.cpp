#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaLakeBasin.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainPatchLayout.h"

namespace
{
    using FPolygon = TArray<FKalmalaWaterMeshVertex, TInlineAllocator<12>>;

    FKalmalaWaterMeshVertex Interpolate(const FKalmalaWaterMeshVertex& A, const FKalmalaWaterMeshVertex& B, double T)
    {
        FKalmalaWaterMeshVertex V;
        V.Position = FMath::Lerp(A.Position, B.Position, T);
        V.TerrainHeight = FMath::Lerp(A.TerrainHeight, B.TerrainHeight, T);
        V.Fields.Elevation = FMath::Lerp(A.Fields.Elevation, B.Fields.Elevation, T);
        V.Fields.Humidity = FMath::Lerp(A.Fields.Humidity, B.Fields.Humidity, T);
        V.Fields.Temperature = FMath::Lerp(A.Fields.Temperature, B.Fields.Temperature, T);
        V.Fields.Flora = FMath::Lerp(A.Fields.Flora, B.Fields.Flora, T);
        V.Fields.GeneratorRevision = A.Fields.GeneratorRevision;
        return V;
    }

    template<typename FDistance>
    void Clip(FPolygon& Polygon, FDistance Distance)
    {
        FPolygon Result;
        for (int32 I = 0; I < Polygon.Num(); ++I)
        {
            const auto& A = Polygon[I];
            const auto& B = Polygon[(I + 1) % Polygon.Num()];
            const double DA = Distance(A);
            const double DB = Distance(B);
            if (DA >= 0.0) Result.Add(A);
            if ((DA >= 0.0) != (DB >= 0.0)) Result.Add(Interpolate(A, B, DA / (DA - DB)));
        }
        Polygon = MoveTemp(Result);
    }
}

void FKalmalaWaterSurfaceMesh::AppendTriangle(const FKalmalaWaterMeshVertex& A,
    const FKalmalaWaterMeshVertex& B, const FKalmalaWaterMeshVertex& C,
    const bool bLake, const bool bShore, FKalmalaWaterMesh& Mesh)
{
    const float Level = bLake ? FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight
        : FKalmalaTerrainHeightSampler::SeaLevelWorldHeight;
    FPolygon Polygon = { A, B, C };
    Clip(Polygon, [Level](const auto& V) { return Level - V.TerrainHeight; });
    if (bShore)
    {
        // Only actual shallow terrain gets a shore tint. Never frame cells or
        // biome boundaries with a floating rectangular ribbon.
        Clip(Polygon, [Level](const auto& V) { return ShoreDepth - (Level - V.TerrainHeight); });
    }
    if (Polygon.Num() < 3) return;
    const float Z = Level + (bShore ? 0.5f : 0.0f);
    for (int32 I = 1; I + 1 < Polygon.Num(); ++I)
    {
        const auto& P = Polygon[0].Position;
        const auto& Q = Polygon[I].Position;
        const auto& R = Polygon[I + 1].Position;
        if (FMath::Abs(FVector2D::CrossProduct(Q - P, R - P)) < 0.001) continue;
        const int32 Start = Mesh.Vertices.Num();
        Mesh.Vertices.Append({ FVector(P, Z), FVector(Q, Z), FVector(R, Z) });
        Mesh.Triangles.Append({ Start, Start + 1, Start + 2 });
    }
}

FKalmalaWaterMesh FKalmalaWaterSurfaceMesh::BuildPatch(const FKalmalaWorldGenerationConfig& Config,
    const FVector2D PatchCenter, const bool bLake, const bool bShore)
{
    FKalmalaWaterMesh Mesh;
    if (!Config.IsValid()) return Mesh;
    constexpr float Size = FKalmalaTerrainPatchLayout::PatchSize;
    constexpr int32 Side = CellsPerSide + 1;
    TArray<FKalmalaWaterMeshVertex> Grid;
    Grid.SetNum(Side * Side);
    for (int32 Y = 0; Y < Side; ++Y)
    {
        for (int32 X = 0; X < Side; ++X)
        {
            auto& V = Grid[Y * Side + X];
            V.Position = FVector2D(-Size * 0.5f + X * Size / CellsPerSide, -Size * 0.5f + Y * Size / CellsPerSide);
            V.Fields = FKalmalaWorldFieldSampler::Sample(Config, PatchCenter + V.Position);
            V.TerrainHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, PatchCenter + V.Position);
        }
    }
    for (int32 Y = 0; Y < CellsPerSide; ++Y)
    {
        for (int32 X = 0; X < CellsPerSide; ++X)
        {
            const int32 I = Y * Side + X;
            auto AppendBasinTriangle = [&](const auto& A, const auto& B, const auto& C)
            {
                const auto Wet = [&](const auto& V) { return V.TerrainHeight < FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight && FKalmalaLakeBasin::Contains(Config, PatchCenter + V.Position, PatchCenter); };
                if (!bLake || Wet(A) || Wet(B) || Wet(C)) AppendTriangle(A, B, C, bLake, bShore, Mesh);
            };
            AppendBasinTriangle(Grid[I], Grid[I + Side], Grid[I + 1]);
            AppendBasinTriangle(Grid[I + 1], Grid[I + Side], Grid[I + Side + 1]);
        }
    }
    return Mesh;
}
