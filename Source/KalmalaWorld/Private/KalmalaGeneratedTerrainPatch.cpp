#include "KalmalaGeneratedTerrainPatch.h"

#include "Components/SceneComponent.h"
#include "KalmalaGeneratedTerrainTile.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"

namespace KalmalaGeneratedTerrainPatch
{
    constexpr int32 SurfaceCellsPerSide = 24;

    static_assert(FKalmalaTerrainPatchLayout::TilesPerSide % 2 == 1, "The terrain patch requires a centered tile layout.");
}

AKalmalaGeneratedTerrainPatch::AKalmalaGeneratedTerrainPatch()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    TerrainSurface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainSurface"));
    TerrainSurface->SetupAttachment(SceneRoot);
    TerrainSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TerrainSurface->SetGenerateOverlapEvents(false);
}

void AKalmalaGeneratedTerrainPatch::Initialize(const FKalmalaWorldGenerationConfig& InConfig, const FVector2D InPatchCenter)
{
    check(HasAuthority());
    WorldGenerationConfig = InConfig;
    PatchCenter = InPatchCenter;
    bIsConfigured = true;
    bVisualSurfaceBuilt = false;
    BuildVisualSurface();
    SpawnCollisionTiles();
    ForceNetUpdate();
}

void AKalmalaGeneratedTerrainPatch::OnRep_GenerationData()
{
    if (bIsConfigured)
    {
        if (BuildVisualSurface())
        {
            UE_LOG(LogTemp, Display, TEXT("Client built the seed-derived terrain surface from the replicated patch descriptor."));
        }
    }
}

bool AKalmalaGeneratedTerrainPatch::BuildVisualSurface()
{
    if (bVisualSurfaceBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || TerrainSurface == nullptr)
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    constexpr int32 VerticesPerSide = KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide + 1;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    Vertices.Reserve(VerticesPerSide * VerticesPerSide);
    Normals.Reserve(VerticesPerSide * VerticesPerSide);
    UVs.Reserve(VerticesPerSide * VerticesPerSide);
    VertexColors.Reserve(VerticesPerSide * VerticesPerSide);
    Tangents.Reserve(VerticesPerSide * VerticesPerSide);
    Triangles.Reserve(KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide * KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide * 6);

    for (int32 GridY = 0; GridY < VerticesPerSide; ++GridY)
    {
        const float Y = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(GridY) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide);
        for (int32 GridX = 0; GridX < VerticesPerSide; ++GridX)
        {
            const float X = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(GridX) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide);
            const FVector2D SamplePosition = PatchCenter + FVector2D(X, Y);
            const float Height = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, SamplePosition);

            Vertices.Add(FVector(X, Y, Height));
            Normals.Add(FKalmalaTerrainHeightSampler::SampleSurfaceNormal(WorldGenerationConfig, SamplePosition));
            UVs.Add(FVector2D(static_cast<float>(GridX) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide, static_cast<float>(GridY) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide));
            VertexColors.Add(FLinearColor::White);
            Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        }
    }

    for (int32 GridY = 0; GridY < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridY)
    {
        for (int32 GridX = 0; GridX < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridX)
        {
            const int32 BottomLeft = GridY * VerticesPerSide + GridX;
            const int32 BottomRight = BottomLeft + 1;
            const int32 TopLeft = BottomLeft + VerticesPerSide;
            const int32 TopRight = TopLeft + 1;

            Triangles.Append({BottomLeft, TopLeft, BottomRight, BottomRight, TopLeft, TopRight});
        }
    }

    TerrainSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
    bVisualSurfaceBuilt = true;
    return true;
}

void AKalmalaGeneratedTerrainPatch::SpawnCollisionTiles()
{
    if (!HasAuthority() || !bIsConfigured || !WorldGenerationConfig.IsValid())
    {
        return;
    }

    for (AKalmalaGeneratedTerrainTile* CollisionTile : CollisionTiles)
    {
        if (CollisionTile != nullptr)
        {
            CollisionTile->Destroy();
        }
    }
    CollisionTiles.Reset();

    const int32 HalfTileCount = FKalmalaTerrainPatchLayout::TilesPerSide / 2;
    for (int32 TileY = -HalfTileCount; TileY <= HalfTileCount; ++TileY)
    {
        for (int32 TileX = -HalfTileCount; TileX <= HalfTileCount; ++TileX)
        {
            const FTransform TileTransform(FRotator::ZeroRotator, FKalmalaTerrainPatchLayout::GetTileCenter(WorldGenerationConfig, PatchCenter, TileX, TileY));
            if (AKalmalaGeneratedTerrainTile* CollisionTile = GetWorld()->SpawnActor<AKalmalaGeneratedTerrainTile>(AKalmalaGeneratedTerrainTile::StaticClass(), TileTransform))
            {
                CollisionTiles.Add(CollisionTile);
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Server spawned %d seed-derived terrain collision tiles."), CollisionTiles.Num());
}

void AKalmalaGeneratedTerrainPatch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, WorldGenerationConfig);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, PatchCenter);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, bIsConfigured);
}
