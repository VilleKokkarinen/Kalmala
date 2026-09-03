#include "KalmalaGeneratedTerrainPatch.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationSeeds.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace KalmalaGeneratedTerrainPatch
{
    constexpr int32 SurfaceCellsPerSide = 24;
    constexpr int32 RockCandidateCount = 48;
    constexpr float RockEdgeMargin = 150.0f;
    constexpr int32 TreeCandidateCount = 18;
    constexpr float TreeEdgeMargin = 260.0f;
    constexpr uint64 TreeSeedSalt = 0xD1B54A32D192ED03ull;
    constexpr float LakeShoreWidth = 24.0f;

    static bool IsLakeWaterCell(const FKalmalaWorldGenerationConfig& Config, const FVector2D BottomLeft, const float CellSize)
    {
        return FKalmalaShimmeringLakeSampler::IsWater(Config, BottomLeft)
            && FKalmalaShimmeringLakeSampler::IsWater(Config, BottomLeft + FVector2D(CellSize, 0.0f))
            && FKalmalaShimmeringLakeSampler::IsWater(Config, BottomLeft + FVector2D(0.0f, CellSize))
            && FKalmalaShimmeringLakeSampler::IsWater(Config, BottomLeft + FVector2D(CellSize, CellSize));
    }

    static void AppendFlatQuad(
        TArray<FVector>& Vertices,
        TArray<int32>& Triangles,
        TArray<FVector>& Normals,
        TArray<FVector2D>& UVs,
        TArray<FLinearColor>& VertexColors,
        TArray<FProcMeshTangent>& Tangents,
        const FVector2D BottomLeft,
        const FVector2D TopLeft,
        const FVector2D BottomRight,
        const FVector2D TopRight,
        const float Height,
        const FLinearColor Color)
    {
        const int32 FirstVertex = Vertices.Num();
        Vertices.Append({
            FVector(BottomLeft.X, BottomLeft.Y, Height),
            FVector(TopLeft.X, TopLeft.Y, Height),
            FVector(BottomRight.X, BottomRight.Y, Height),
            FVector(TopRight.X, TopRight.Y, Height)});
        Triangles.Append({FirstVertex, FirstVertex + 1, FirstVertex + 2, FirstVertex + 2, FirstVertex + 1, FirstVertex + 3});
        Normals.Append({FVector::UpVector, FVector::UpVector, FVector::UpVector, FVector::UpVector});
        UVs.Append({BottomLeft, TopLeft, BottomRight, TopRight});
        VertexColors.Append({Color, Color, Color, Color});
        Tangents.Append({FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f)});
    }

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
    TerrainSurface->SetCollisionProfileName(TEXT("BlockAll"));
    TerrainSurface->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    TerrainSurface->SetGenerateOverlapEvents(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TerrainMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrain.M_GeneratedTerrain"));
    if (TerrainMaterial.Succeeded())
    {
        TerrainSurface->SetMaterial(0, TerrainMaterial.Object);
    }

    SurfaceWater = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SurfaceWater"));
    SurfaceWater->SetupAttachment(SceneRoot);
    SurfaceWater->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SurfaceWater->SetGenerateOverlapEvents(false);
    SurfaceWater->SetCastShadow(false);

    ShimmeringLakeWater = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShimmeringLakeWater"));
    ShimmeringLakeWater->SetupAttachment(SceneRoot);
    ShimmeringLakeWater->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShimmeringLakeWater->SetGenerateOverlapEvents(false);
    ShimmeringLakeWater->SetCastShadow(false);

    ShimmeringLakeShore = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShimmeringLakeShore"));
    ShimmeringLakeShore->SetupAttachment(SceneRoot);
    ShimmeringLakeShore->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShimmeringLakeShore->SetGenerateOverlapEvents(false);
    ShimmeringLakeShore->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedWater.M_GeneratedWater"));
    if (WaterMaterial.Succeeded())
    {
        SurfaceWater->SetMaterial(0, WaterMaterial.Object);
        ShimmeringLakeWater->SetMaterial(0, WaterMaterial.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> LakeShoreMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedLakeShore.M_GeneratedLakeShore"));
    if (LakeShoreMaterial.Succeeded())
    {
        ShimmeringLakeShore->SetMaterial(0, LakeShoreMaterial.Object);
    }

    MeadowRocks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MeadowRocks"));
    MeadowRocks->SetupAttachment(SceneRoot);
    MeadowRocks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeadowRocks->SetGenerateOverlapEvents(false);
    MeadowRocks->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RockMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (RockMesh.Succeeded())
    {
        MeadowRocks->SetStaticMesh(RockMesh.Object);
    }

    MeadowTreeTrunks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MeadowTreeTrunks"));
    MeadowTreeTrunks->SetupAttachment(SceneRoot);
    MeadowTreeTrunks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeadowTreeTrunks->SetGenerateOverlapEvents(false);
    MeadowTreeTrunks->SetCastShadow(true);

    MeadowTreeCanopies = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("MeadowTreeCanopies"));
    MeadowTreeCanopies->SetupAttachment(SceneRoot);
    MeadowTreeCanopies->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeadowTreeCanopies->SetGenerateOverlapEvents(false);
    MeadowTreeCanopies->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> TrunkMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (TrunkMesh.Succeeded())
    {
        MeadowTreeTrunks->SetStaticMesh(TrunkMesh.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CanopyMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CanopyMesh.Succeeded())
    {
        MeadowTreeCanopies->SetStaticMesh(CanopyMesh.Object);
    }
}

void AKalmalaGeneratedTerrainPatch::Initialize(const FKalmalaWorldGenerationConfig& InConfig, const FVector2D InPatchCenter)
{
    check(HasAuthority());
    WorldGenerationConfig = InConfig;
    PatchCenter = InPatchCenter;
    bIsConfigured = true;
    bVisualSurfaceBuilt = false;
    bSurfaceWaterBuilt = false;
    bShimmeringLakeTreatmentBuilt = false;
    bMeadowRocksBuilt = false;
    bMeadowTreesBuilt = false;
    BuildVisualSurface();
    if (BuildSurfaceWater())
    {
        UE_LOG(LogTemp, Display, TEXT("Server built %d seed-derived surface-water triangles."), SurfaceWater->GetNumSections() > 0 ? SurfaceWater->GetProcMeshSection(0)->ProcIndexBuffer.Num() / 3 : 0);
    }
    if (BuildShimmeringLakeTreatment())
    {
        UE_LOG(LogTemp, Display, TEXT("Server built %d Shimmering Lakes water triangles and %d shoreline triangles."),
            ShimmeringLakeWater->GetNumSections() > 0 ? ShimmeringLakeWater->GetProcMeshSection(0)->ProcIndexBuffer.Num() / 3 : 0,
            ShimmeringLakeShore->GetNumSections() > 0 ? ShimmeringLakeShore->GetProcMeshSection(0)->ProcIndexBuffer.Num() / 3 : 0);
    }
    if (BuildMeadowRocks())
    {
        UE_LOG(LogTemp, Display, TEXT("Server built %d deterministic Meadow rock instances."), MeadowRocks->GetInstanceCount());
    }
    if (BuildMeadowTrees())
    {
        UE_LOG(LogTemp, Display, TEXT("Server built %d deterministic Meadow tree instances."), MeadowTreeTrunks->GetInstanceCount());
    }
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
        if (BuildSurfaceWater())
        {
            UE_LOG(LogTemp, Display, TEXT("Client built %d seed-derived surface-water triangles from the replicated patch descriptor."), SurfaceWater->GetNumSections() > 0 ? SurfaceWater->GetProcMeshSection(0)->ProcIndexBuffer.Num() / 3 : 0);
        }
        if (BuildShimmeringLakeTreatment())
        {
            UE_LOG(LogTemp, Display, TEXT("Client built %d Shimmering Lakes water triangles and %d shoreline triangles from the replicated patch descriptor."),
                ShimmeringLakeWater->GetNumSections() > 0 ? ShimmeringLakeWater->GetProcMeshSection(0)->ProcIndexBuffer.Num() / 3 : 0,
                ShimmeringLakeShore->GetNumSections() > 0 ? ShimmeringLakeShore->GetProcMeshSection(0)->ProcIndexBuffer.Num() / 3 : 0);
        }
        if (BuildMeadowRocks())
        {
            UE_LOG(LogTemp, Display, TEXT("Client built %d deterministic Meadow rock instances from the replicated patch descriptor."), MeadowRocks->GetInstanceCount());
        }
        if (BuildMeadowTrees())
        {
            UE_LOG(LogTemp, Display, TEXT("Client built %d deterministic Meadow tree instances from the replicated patch descriptor."), MeadowTreeTrunks->GetInstanceCount());
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

    TerrainSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
    bVisualSurfaceBuilt = true;
    if (HasAuthority())
    {
        UE_LOG(LogTemp, Display, TEXT("Server built a seed-derived terrain surface with %d collision triangles."), Triangles.Num() / 3);
    }
    return true;
}

bool AKalmalaGeneratedTerrainPatch::BuildSurfaceWater()
{
    if (bSurfaceWaterBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || SurfaceWater == nullptr)
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    constexpr float CellSize = SurfaceSize / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    Vertices.Reserve(KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide * KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide * 4);
    Triangles.Reserve(KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide * KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide * 6);

    for (int32 GridY = 0; GridY < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridY)
    {
        for (int32 GridX = 0; GridX < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridX)
        {
            const FVector2D BottomLeft(-HalfSurfaceSize + GridX * CellSize, -HalfSurfaceSize + GridY * CellSize);
            const FVector2D BottomRight = BottomLeft + FVector2D(CellSize, 0.0f);
            const FVector2D TopLeft = BottomLeft + FVector2D(0.0f, CellSize);
            const FVector2D TopRight = BottomLeft + FVector2D(CellSize, CellSize);
            if (FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, PatchCenter + BottomLeft) > FKalmalaTerrainHeightSampler::SeaLevelWorldHeight
                || FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, PatchCenter + BottomRight) > FKalmalaTerrainHeightSampler::SeaLevelWorldHeight
                || FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, PatchCenter + TopLeft) > FKalmalaTerrainHeightSampler::SeaLevelWorldHeight
                || FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, PatchCenter + TopRight) > FKalmalaTerrainHeightSampler::SeaLevelWorldHeight)
            {
                continue;
            }

            const int32 FirstVertex = Vertices.Num();
            Vertices.Append({
                FVector(BottomLeft.X, BottomLeft.Y, FKalmalaTerrainHeightSampler::SeaLevelWorldHeight),
                FVector(TopLeft.X, TopLeft.Y, FKalmalaTerrainHeightSampler::SeaLevelWorldHeight),
                FVector(BottomRight.X, BottomRight.Y, FKalmalaTerrainHeightSampler::SeaLevelWorldHeight),
                FVector(TopRight.X, TopRight.Y, FKalmalaTerrainHeightSampler::SeaLevelWorldHeight)});
            Triangles.Append({FirstVertex, FirstVertex + 1, FirstVertex + 2, FirstVertex + 2, FirstVertex + 1, FirstVertex + 3});
            Normals.Append({FVector::UpVector, FVector::UpVector, FVector::UpVector, FVector::UpVector});
            UVs.Append({
                FVector2D(static_cast<float>(GridX) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide, static_cast<float>(GridY) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide),
                FVector2D(static_cast<float>(GridX) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide, static_cast<float>(GridY + 1) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide),
                FVector2D(static_cast<float>(GridX + 1) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide, static_cast<float>(GridY) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide),
                FVector2D(static_cast<float>(GridX + 1) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide, static_cast<float>(GridY + 1) / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide)});
            VertexColors.Append({FLinearColor(0.08f, 0.28f, 0.45f), FLinearColor(0.08f, 0.28f, 0.45f), FLinearColor(0.08f, 0.28f, 0.45f), FLinearColor(0.08f, 0.28f, 0.45f)});
            Tangents.Append({FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f)});
        }
    }

    SurfaceWater->ClearAllMeshSections();
    if (!Triangles.IsEmpty())
    {
        SurfaceWater->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
    }
    bSurfaceWaterBuilt = true;
    return true;
}

bool AKalmalaGeneratedTerrainPatch::BuildShimmeringLakeTreatment()
{
    if (bShimmeringLakeTreatmentBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || ShimmeringLakeWater == nullptr || ShimmeringLakeShore == nullptr)
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    constexpr float CellSize = SurfaceSize / KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide;
    constexpr int32 CellCount = KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide * KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide;

    TArray<bool> LakeWaterCells;
    LakeWaterCells.SetNumZeroed(CellCount);

    TArray<FVector> WaterVertices;
    TArray<int32> WaterTriangles;
    TArray<FVector> WaterNormals;
    TArray<FVector2D> WaterUVs;
    TArray<FLinearColor> WaterColors;
    TArray<FProcMeshTangent> WaterTangents;
    TArray<FVector> ShoreVertices;
    TArray<int32> ShoreTriangles;
    TArray<FVector> ShoreNormals;
    TArray<FVector2D> ShoreUVs;
    TArray<FLinearColor> ShoreColors;
    TArray<FProcMeshTangent> ShoreTangents;

    for (int32 GridY = 0; GridY < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridY)
    {
        for (int32 GridX = 0; GridX < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridX)
        {
            const FVector2D BottomLeft(-HalfSurfaceSize + GridX * CellSize, -HalfSurfaceSize + GridY * CellSize);
            const FVector2D WorldBottomLeft = PatchCenter + BottomLeft;
            const bool bIsLakeWater = KalmalaGeneratedTerrainPatch::IsLakeWaterCell(WorldGenerationConfig, WorldBottomLeft, CellSize);
            LakeWaterCells[GridY * KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide + GridX] = bIsLakeWater;
            if (!bIsLakeWater)
            {
                continue;
            }

            KalmalaGeneratedTerrainPatch::AppendFlatQuad(
                WaterVertices, WaterTriangles, WaterNormals, WaterUVs, WaterColors, WaterTangents,
                BottomLeft, BottomLeft + FVector2D(0.0f, CellSize), BottomLeft + FVector2D(CellSize, 0.0f), BottomLeft + FVector2D(CellSize, CellSize),
                FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight, FLinearColor(0.10f, 0.42f, 0.62f));
        }
    }

    const auto IsNeighborWater = [this, &LakeWaterCells, CellSize, HalfSurfaceSize](const int32 GridX, const int32 GridY)
    {
        if (GridX >= 0 && GridX < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide && GridY >= 0 && GridY < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide)
        {
            return LakeWaterCells[GridY * KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide + GridX];
        }

        const FVector2D NeighborBottomLeft(-HalfSurfaceSize + GridX * CellSize, -HalfSurfaceSize + GridY * CellSize);
        return KalmalaGeneratedTerrainPatch::IsLakeWaterCell(WorldGenerationConfig, PatchCenter + NeighborBottomLeft, CellSize);
    };

    for (int32 GridY = 0; GridY < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridY)
    {
        for (int32 GridX = 0; GridX < KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide; ++GridX)
        {
            if (!LakeWaterCells[GridY * KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide + GridX])
            {
                continue;
            }

            const FVector2D BottomLeft(-HalfSurfaceSize + GridX * CellSize, -HalfSurfaceSize + GridY * CellSize);
            const FVector2D BottomRight = BottomLeft + FVector2D(CellSize, 0.0f);
            const FVector2D TopLeft = BottomLeft + FVector2D(0.0f, CellSize);
            const FVector2D TopRight = BottomLeft + FVector2D(CellSize, CellSize);
            const float ShoreHeight = FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight + 1.0f;
            const FLinearColor ShoreColor(0.48f, 0.70f, 0.67f);

            if (!IsNeighborWater(GridX, GridY - 1))
            {
                KalmalaGeneratedTerrainPatch::AppendFlatQuad(ShoreVertices, ShoreTriangles, ShoreNormals, ShoreUVs, ShoreColors, ShoreTangents,
                    BottomLeft, BottomLeft + FVector2D(0.0f, KalmalaGeneratedTerrainPatch::LakeShoreWidth), BottomRight, BottomRight + FVector2D(0.0f, KalmalaGeneratedTerrainPatch::LakeShoreWidth), ShoreHeight, ShoreColor);
            }
            if (!IsNeighborWater(GridX, GridY + 1))
            {
                KalmalaGeneratedTerrainPatch::AppendFlatQuad(ShoreVertices, ShoreTriangles, ShoreNormals, ShoreUVs, ShoreColors, ShoreTangents,
                    TopLeft - FVector2D(0.0f, KalmalaGeneratedTerrainPatch::LakeShoreWidth), TopLeft, TopRight - FVector2D(0.0f, KalmalaGeneratedTerrainPatch::LakeShoreWidth), TopRight, ShoreHeight, ShoreColor);
            }
            if (!IsNeighborWater(GridX - 1, GridY))
            {
                KalmalaGeneratedTerrainPatch::AppendFlatQuad(ShoreVertices, ShoreTriangles, ShoreNormals, ShoreUVs, ShoreColors, ShoreTangents,
                    BottomLeft, TopLeft, BottomLeft + FVector2D(KalmalaGeneratedTerrainPatch::LakeShoreWidth, 0.0f), TopLeft + FVector2D(KalmalaGeneratedTerrainPatch::LakeShoreWidth, 0.0f), ShoreHeight, ShoreColor);
            }
            if (!IsNeighborWater(GridX + 1, GridY))
            {
                KalmalaGeneratedTerrainPatch::AppendFlatQuad(ShoreVertices, ShoreTriangles, ShoreNormals, ShoreUVs, ShoreColors, ShoreTangents,
                    BottomRight - FVector2D(KalmalaGeneratedTerrainPatch::LakeShoreWidth, 0.0f), TopRight - FVector2D(KalmalaGeneratedTerrainPatch::LakeShoreWidth, 0.0f), BottomRight, TopRight, ShoreHeight, ShoreColor);
            }
        }
    }

    ShimmeringLakeWater->ClearAllMeshSections();
    ShimmeringLakeShore->ClearAllMeshSections();
    if (!WaterTriangles.IsEmpty())
    {
        ShimmeringLakeWater->CreateMeshSection_LinearColor(0, WaterVertices, WaterTriangles, WaterNormals, WaterUVs, WaterColors, WaterTangents, false);
    }
    if (!ShoreTriangles.IsEmpty())
    {
        ShimmeringLakeShore->CreateMeshSection_LinearColor(0, ShoreVertices, ShoreTriangles, ShoreNormals, ShoreUVs, ShoreColors, ShoreTangents, false);
    }

    bShimmeringLakeTreatmentBuilt = true;
    return true;
}

bool AKalmalaGeneratedTerrainPatch::BuildMeadowRocks()
{
    if (bMeadowRocksBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || MeadowRocks == nullptr || MeadowRocks->GetStaticMesh() == nullptr)
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    const uint64 Seed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(WorldGenerationConfig, EKalmalaWorldField::Flora);
    FRandomStream RandomStream(static_cast<int32>(Seed ^ (Seed >> 32)));

    MeadowRocks->ClearInstances();
    for (int32 CandidateIndex = 0; CandidateIndex < KalmalaGeneratedTerrainPatch::RockCandidateCount; ++CandidateIndex)
    {
        const FVector2D LocalPosition(
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::RockEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::RockEdgeMargin),
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::RockEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::RockEdgeMargin));
        const FVector2D SamplePosition = PatchCenter + LocalPosition;
        if (FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, SamplePosition)) != EKalmalaBiome::Meadows)
        {
            continue;
        }

        const float ZScale = RandomStream.FRandRange(0.18f, 0.42f);
        const FVector RockScale(RandomStream.FRandRange(0.35f, 0.75f), RandomStream.FRandRange(0.30f, 0.65f), ZScale);
        const float RockRadius = 50.0f * ZScale;
        MeadowRocks->AddInstance(FTransform(
            FRotator(0.0f, RandomStream.FRandRange(0.0f, 360.0f), RandomStream.FRandRange(-12.0f, 12.0f)),
            FVector(LocalPosition.X, LocalPosition.Y, FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, SamplePosition) + RockRadius),
            RockScale));
    }

    bMeadowRocksBuilt = true;
    return true;
}

bool AKalmalaGeneratedTerrainPatch::BuildMeadowTrees()
{
    if (bMeadowTreesBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || MeadowTreeTrunks == nullptr || MeadowTreeCanopies == nullptr
        || MeadowTreeTrunks->GetStaticMesh() == nullptr || MeadowTreeCanopies->GetStaticMesh() == nullptr)
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    const uint64 FloraSeed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(WorldGenerationConfig, EKalmalaWorldField::Flora);
    FRandomStream RandomStream(static_cast<int32>((FloraSeed ^ KalmalaGeneratedTerrainPatch::TreeSeedSalt) >> 32));

    MeadowTreeTrunks->ClearInstances();
    MeadowTreeCanopies->ClearInstances();
    for (int32 CandidateIndex = 0; CandidateIndex < KalmalaGeneratedTerrainPatch::TreeCandidateCount; ++CandidateIndex)
    {
        const FVector2D LocalPosition(
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::TreeEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::TreeEdgeMargin),
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::TreeEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::TreeEdgeMargin));
        const FVector2D SamplePosition = PatchCenter + LocalPosition;
        if (FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, SamplePosition)) != EKalmalaBiome::Meadows)
        {
            continue;
        }

        const float TrunkHeightScale = RandomStream.FRandRange(3.5f, 6.5f);
        const float TrunkRadiusScale = RandomStream.FRandRange(0.10f, 0.18f);
        const float CanopyScale = RandomStream.FRandRange(1.1f, 1.7f);
        const float SurfaceHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, SamplePosition);
        const float TreeYaw = RandomStream.FRandRange(0.0f, 360.0f);
        const FVector TrunkScale(TrunkRadiusScale, TrunkRadiusScale, TrunkHeightScale);
        const float TrunkHeight = 100.0f * TrunkHeightScale;

        MeadowTreeTrunks->AddInstance(FTransform(FRotator(0.0f, TreeYaw, 0.0f), FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight + TrunkHeight * 0.5f), TrunkScale));
        MeadowTreeCanopies->AddInstance(FTransform(
            FRotator(0.0f, TreeYaw, RandomStream.FRandRange(-8.0f, 8.0f)),
            FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight + TrunkHeight + CanopyScale * 25.0f),
            FVector(CanopyScale, CanopyScale, CanopyScale * RandomStream.FRandRange(0.8f, 1.2f))));
    }

    bMeadowTreesBuilt = true;
    return true;
}

void AKalmalaGeneratedTerrainPatch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, WorldGenerationConfig);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, PatchCenter);
    DOREPLIFETIME(AKalmalaGeneratedTerrainPatch, bIsConfigured);
}
