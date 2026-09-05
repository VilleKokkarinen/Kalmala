#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldFieldSampler.h"

struct FKalmalaWaterMeshVertex
{
    FVector2D Position;
    float TerrainHeight = 0.0f;
    FKalmalaWorldFieldSample Fields;
};

struct FKalmalaWaterMesh
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
};

/** Clips water to the same linear triangles used by terrain rendering/collision.
 *  No terrain, water classification, population or save rule is changed. */
struct KALMALAWORLD_API FKalmalaWaterSurfaceMesh
{
    static constexpr int32 CellsPerSide = 24;
    static constexpr float ShoreDepth = 12.0f;
    static FKalmalaWaterMesh BuildPatch(const FKalmalaWorldGenerationConfig& Config,
        FVector2D PatchCenter, bool bLake, bool bShore = false);
    static void AppendTriangle(const FKalmalaWaterMeshVertex& A, const FKalmalaWaterMeshVertex& B,
        const FKalmalaWaterMeshVertex& C, bool bLake, bool bShore, FKalmalaWaterMesh& Mesh);
};
