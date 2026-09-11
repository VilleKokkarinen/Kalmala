#include "KalmalaGeneratedTerrainPatch.h"

#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationSeeds.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"

namespace KalmalaGeneratedTerrainPatch
{
    constexpr int32 SurfaceCellsPerSide = FKalmalaWaterSurfaceMesh::CellsPerSide;
    constexpr int32 RockCandidateCount = 48;
    constexpr float RockEdgeMargin = 150.0f;
    constexpr int32 MeadowTreeCandidateCount = 18;
    constexpr int32 ElderwoodTreeCandidateCount = 72;
    constexpr float TreeEdgeMargin = 260.0f;
    constexpr uint64 TreeSeedSalt = 0xD1B54A32D192ED03ull;
    constexpr float LakeShoreWidth = 24.0f;

    FLinearColor GetBiomeDebugColor(const EKalmalaBiome Biome)
    {
        switch (Biome)
        {
        case EKalmalaBiome::Ocean: return FLinearColor(23.0f / 255.0f, 88.0f / 255.0f, 160.0f / 255.0f);
        case EKalmalaBiome::ShimmeringLakes: return FLinearColor(61.0f / 255.0f, 177.0f / 255.0f, 190.0f / 255.0f);
        case EKalmalaBiome::Elderwood: return FLinearColor(30.0f / 255.0f, 100.0f / 255.0f, 47.0f / 255.0f);
        case EKalmalaBiome::MossyMire: return FLinearColor(76.0f / 255.0f, 113.0f / 255.0f, 55.0f / 255.0f);
        case EKalmalaBiome::FreezingTundra: return FLinearColor(213.0f / 255.0f, 236.0f / 255.0f, 238.0f / 255.0f);
        case EKalmalaBiome::ThunderMountains: return FLinearColor(104.0f / 255.0f, 98.0f / 255.0f, 112.0f / 255.0f);
        case EKalmalaBiome::Meadows: return FLinearColor(131.0f / 255.0f, 174.0f / 255.0f, 76.0f / 255.0f);
        default: return FLinearColor(1.0f, 0.0f, 1.0f);
        }
    }

    static void ApplyWaterMesh(UProceduralMeshComponent* Component, const FKalmalaWaterMesh& Mesh, const FLinearColor Colour)
    {
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;
        for (const FVector& V : Mesh.Vertices)
        {
            Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(V.X, V.Y) / 1000.0f);
            Colors.Add(Colour);
            Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        }
        Component->ClearAllMeshSections();
        if (!Mesh.Triangles.IsEmpty())
        {
            Component->CreateMeshSection_LinearColor(0, Mesh.Vertices, Mesh.Triangles, Normals, UVs, Colors, Tangents, false);
        }
    }
    static void AppendLowPolyRock(
        TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& VertexColors, TArray<FProcMeshTangent>& Tangents,
        const FVector Center, const float Radius, const float Height, const float YawDegrees)
    {
        constexpr int32 SideCount = 6;
        const int32 FirstVertex = Vertices.Num();
        for (int32 SideIndex = 0; SideIndex < SideCount; ++SideIndex)
        {
            const float Angle = FMath::DegreesToRadians(YawDegrees + 360.0f * SideIndex / SideCount);
            const float RadiusScale = SideIndex % 2 == 0 ? 1.0f : 0.76f;
            Vertices.Add(Center + FVector(FMath::Cos(Angle) * Radius * RadiusScale, FMath::Sin(Angle) * Radius * RadiusScale, 0.0f));
            Normals.Add(FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.45f).GetSafeNormal());
            UVs.Add(FVector2D(static_cast<float>(SideIndex) / SideCount, 0.0f));
            VertexColors.Add(FLinearColor(0.26f, 0.29f, 0.25f));
            Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        }
        const int32 ApexVertex = Vertices.Add(Center + FVector(Radius * 0.12f, -Radius * 0.08f, Height));
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D(0.5f, 1.0f));
        VertexColors.Add(FLinearColor(0.30f, 0.33f, 0.28f));
        Tangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        for (int32 SideIndex = 0; SideIndex < SideCount; ++SideIndex)
        {
            const int32 NextIndex = (SideIndex + 1) % SideCount;
            Triangles.Append({FirstVertex + SideIndex, ApexVertex, FirstVertex + NextIndex});
        }
    }

    static void AppendTaperedTree(
        TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& VertexColors, TArray<FProcMeshTangent>& Tangents,
        const FVector BaseCenter, const float TrunkRadius, const float TrunkHeight, const float CanopyRadius, const float CanopyHeight, const float YawDegrees,
        TArray<FVector>& CanopyVertices, TArray<int32>& CanopyTriangles, TArray<FVector>& CanopyNormals, TArray<FVector2D>& CanopyUVs, TArray<FLinearColor>& CanopyColors, TArray<FProcMeshTangent>& CanopyTangents)
    {
        constexpr int32 SideCount = 6;
        const int32 TrunkFirstVertex = Vertices.Num();
        const int32 CanopyFirstVertex = CanopyVertices.Num();
        for (int32 SideIndex = 0; SideIndex < SideCount; ++SideIndex)
        {
            const float Angle = FMath::DegreesToRadians(YawDegrees + 360.0f * SideIndex / SideCount);
            const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
            Vertices.Add(BaseCenter + Direction * TrunkRadius);
            Vertices.Add(BaseCenter + Direction * (TrunkRadius * 0.62f) + FVector(0.0f, 0.0f, TrunkHeight));
            Normals.Append({Direction, Direction});
            UVs.Append({FVector2D(static_cast<float>(SideIndex) / SideCount, 0.0f), FVector2D(static_cast<float>(SideIndex) / SideCount, 1.0f)});
            VertexColors.Append({FLinearColor(0.18f, 0.12f, 0.08f), FLinearColor(0.22f, 0.15f, 0.10f)});
            Tangents.Append({FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f)});

            CanopyVertices.Add(BaseCenter + FVector(0.0f, 0.0f, TrunkHeight * 0.68f) + Direction * CanopyRadius);
            CanopyNormals.Add((Direction + FVector(0.0f, 0.0f, 0.4f)).GetSafeNormal());
            CanopyUVs.Add(FVector2D(static_cast<float>(SideIndex) / SideCount, 0.0f));
            CanopyColors.Add(FLinearColor(0.16f, 0.31f, 0.17f));
            CanopyTangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        }
        const int32 CanopyApex = CanopyVertices.Add(BaseCenter + FVector(0.0f, 0.0f, TrunkHeight + CanopyHeight));
        CanopyNormals.Add(FVector::UpVector);
        CanopyUVs.Add(FVector2D(0.5f, 1.0f));
        CanopyColors.Add(FLinearColor(0.20f, 0.38f, 0.20f));
        CanopyTangents.Add(FProcMeshTangent(1.0f, 0.0f, 0.0f));
        for (int32 SideIndex = 0; SideIndex < SideCount; ++SideIndex)
        {
            const int32 NextIndex = (SideIndex + 1) % SideCount;
            Triangles.Append({TrunkFirstVertex + SideIndex * 2, TrunkFirstVertex + NextIndex * 2, TrunkFirstVertex + SideIndex * 2 + 1, TrunkFirstVertex + SideIndex * 2 + 1, TrunkFirstVertex + NextIndex * 2, TrunkFirstVertex + NextIndex * 2 + 1});
            CanopyTriangles.Append({CanopyFirstVertex + SideIndex, CanopyApex, CanopyFirstVertex + NextIndex});
        }
    }

    static void AppendRootButtresses(
        TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FLinearColor>& VertexColors, TArray<FProcMeshTangent>& Tangents,
        const FVector BaseCenter, const float Radius, const float YawDegrees)
    {
        constexpr int32 RootCount = 4;
        for (int32 RootIndex = 0; RootIndex < RootCount; ++RootIndex)
        {
            const float Angle = FMath::DegreesToRadians(YawDegrees + 360.0f * RootIndex / RootCount);
            const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
            const FVector Side(-Direction.Y, Direction.X, 0.0f);
            const int32 FirstVertex = Vertices.Num();
            Vertices.Append({ BaseCenter + Side * Radius * 0.55f, BaseCenter - Side * Radius * 0.55f, BaseCenter + Direction * Radius * 3.2f, BaseCenter + FVector(0.0f, 0.0f, Radius * 2.1f) });
            Normals.Append({ FVector::UpVector, FVector::UpVector, FVector::UpVector, (Direction + FVector::UpVector).GetSafeNormal() });
            UVs.Append({ FVector2D::ZeroVector, FVector2D(1.0f, 0.0f), FVector2D(0.5f, 1.0f), FVector2D(0.5f, 0.5f) });
            VertexColors.Append({ FLinearColor(0.16f, 0.10f, 0.06f), FLinearColor(0.16f, 0.10f, 0.06f), FLinearColor(0.13f, 0.08f, 0.05f), FLinearColor(0.20f, 0.13f, 0.08f) });
            Tangents.Append({ FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f), FProcMeshTangent(1.0f, 0.0f, 0.0f) });
            Triangles.Append({ FirstVertex, FirstVertex + 2, FirstVertex + 3, FirstVertex + 2, FirstVertex + 1, FirstVertex + 3 });
        }
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

    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaBiomeDebug")))
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> BiomeDebugMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrainBiomeDebug.M_GeneratedTerrainBiomeDebug"));
        if (BiomeDebugMaterial.Succeeded())
        {
            TerrainSurface->SetMaterial(0, BiomeDebugMaterial.Object);
            UE_LOG(LogTemp, Display, TEXT("Biome debug material is active on the generated terrain surface."));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Biome debug material is unavailable; run the CreateWorldMaterials commandlet before using -KalmalaBiomeDebug."));
        }
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

    MeadowRocks = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeadowRocks"));
    MeadowRocks->SetupAttachment(SceneRoot);
    MeadowRocks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeadowRocks->SetGenerateOverlapEvents(false);
    MeadowRocks->SetCastShadow(true);

    MeadowTreeTrunks = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeadowTreeTrunks"));
    MeadowTreeTrunks->SetupAttachment(SceneRoot);
    MeadowTreeTrunks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeadowTreeTrunks->SetGenerateOverlapEvents(false);
    MeadowTreeTrunks->SetCastShadow(true);

    MeadowTreeCanopies = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MeadowTreeCanopies"));
    MeadowTreeCanopies->SetupAttachment(SceneRoot);
    MeadowTreeCanopies->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeadowTreeCanopies->SetGenerateOverlapEvents(false);
    MeadowTreeCanopies->SetCastShadow(true);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> RockMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock.M_GeneratedRock"));
    if (RockMaterial.Succeeded())
    {
        MeadowRocks->SetMaterial(0, RockMaterial.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BarkMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedBark.M_GeneratedBark"));
    if (BarkMaterial.Succeeded())
    {
        MeadowTreeTrunks->SetMaterial(0, BarkMaterial.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> CanopyMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedCanopy.M_GeneratedCanopy"));
    if (CanopyMaterial.Succeeded())
    {
        MeadowTreeCanopies->SetMaterial(0, CanopyMaterial.Object);
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
    MeadowRockCount = 0;
    MeadowTreeCount = 0;
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
        UE_LOG(LogTemp, Display, TEXT("Server built %d deterministic Meadow rocks."), MeadowRockCount);
    }
    if (BuildMeadowTrees())
    {
        UE_LOG(LogTemp, Display, TEXT("Server built %d deterministic Meadow/Elderwood trees and roots."), MeadowTreeCount);
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
            UE_LOG(LogTemp, Display, TEXT("Client built %d deterministic Meadow rocks from the replicated patch descriptor."), MeadowRockCount);
        }
        if (BuildMeadowTrees())
        {
            UE_LOG(LogTemp, Display, TEXT("Client built %d deterministic Meadow/Elderwood trees and roots from the replicated patch descriptor."), MeadowTreeCount);
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
            const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, SamplePosition));
            VertexColors.Add(FParse::Param(FCommandLine::Get(), TEXT("KalmalaBiomeDebug")) ? KalmalaGeneratedTerrainPatch::GetBiomeDebugColor(Biome) : FLinearColor::White);
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

    if (FKalmalaWorldBounds::IsBounded(WorldGenerationConfig) && PatchCenter.Size() + HalfSurfaceSize * 2 > FKalmalaWorldBounds::Radius)
    {
        FKalmalaWorldBounds::ClipMesh(WorldGenerationConfig, PatchCenter, Vertices, Triangles);
        Normals.Reset(); UVs.Reset(); VertexColors.Reset(); Tangents.Reset();
        for (const FVector& V : Vertices)
        {
            const FVector2D P = PatchCenter + FVector2D(V);
            Normals.Add(FKalmalaTerrainHeightSampler::SampleSurfaceNormal(WorldGenerationConfig, P));
            UVs.Add((FVector2D(V) + FVector2D(HalfSurfaceSize)) / (HalfSurfaceSize * 2));
            const auto Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, P));
            VertexColors.Add(FParse::Param(FCommandLine::Get(), TEXT("KalmalaBiomeDebug")) ? KalmalaGeneratedTerrainPatch::GetBiomeDebugColor(Biome) : FLinearColor::White);
            Tangents.Add(FProcMeshTangent(1, 0, 0));
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
    if (bSurfaceWaterBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || SurfaceWater == nullptr) return false;
    KalmalaGeneratedTerrainPatch::ApplyWaterMesh(SurfaceWater,
        FKalmalaWaterSurfaceMesh::BuildPatch(WorldGenerationConfig, PatchCenter, false), FLinearColor(0.08f, 0.28f, 0.45f));
    bSurfaceWaterBuilt = true;
    return true;
}

bool AKalmalaGeneratedTerrainPatch::BuildShimmeringLakeTreatment()
{
    if (bShimmeringLakeTreatmentBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid()
        || ShimmeringLakeWater == nullptr || ShimmeringLakeShore == nullptr) return false;
    KalmalaGeneratedTerrainPatch::ApplyWaterMesh(ShimmeringLakeWater,
        FKalmalaWaterSurfaceMesh::BuildPatch(WorldGenerationConfig, PatchCenter, true), FLinearColor(0.10f, 0.42f, 0.62f));
    KalmalaGeneratedTerrainPatch::ApplyWaterMesh(ShimmeringLakeShore,
        FKalmalaWaterSurfaceMesh::BuildPatch(WorldGenerationConfig, PatchCenter, true, true), FLinearColor(0.48f, 0.70f, 0.67f));
    bShimmeringLakeTreatmentBuilt = true;
    return true;
}
bool AKalmalaGeneratedTerrainPatch::BuildMeadowRocks()
{
    if (bMeadowRocksBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || MeadowRocks == nullptr)
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    const uint64 Seed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(WorldGenerationConfig, EKalmalaWorldField::Flora);
    FRandomStream RandomStream(static_cast<int32>(Seed ^ (Seed >> 32)));

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FLinearColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;
    MeadowRockCount = 0;
    for (int32 CandidateIndex = 0; CandidateIndex < KalmalaGeneratedTerrainPatch::RockCandidateCount; ++CandidateIndex)
    {
        const FVector2D LocalPosition(
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::RockEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::RockEdgeMargin),
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::RockEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::RockEdgeMargin));
        const FVector2D SamplePosition = PatchCenter + LocalPosition;
        if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, SamplePosition, 1000)) continue;
        if (FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, SamplePosition)) != EKalmalaBiome::Meadows)
        {
            continue;
        }

        const float RockRadius = RandomStream.FRandRange(22.0f, 52.0f);
        const float RockHeight = RandomStream.FRandRange(14.0f, 38.0f);
        KalmalaGeneratedTerrainPatch::AppendLowPolyRock(
            Vertices, Triangles, Normals, UVs, VertexColors, Tangents,
            FVector(LocalPosition.X, LocalPosition.Y, FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, SamplePosition)),
            RockRadius, RockHeight, RandomStream.FRandRange(0.0f, 360.0f));
        ++MeadowRockCount;
    }

    MeadowRocks->ClearAllMeshSections();
    if (!Triangles.IsEmpty())
    {
        MeadowRocks->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
    }
    bMeadowRocksBuilt = true;
    return true;
}

bool AKalmalaGeneratedTerrainPatch::BuildMeadowTrees()
{
    if (bMeadowTreesBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || MeadowTreeTrunks == nullptr || MeadowTreeCanopies == nullptr)
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    const uint64 FloraSeed = FKalmalaWorldGenerationSeeds::DeriveFieldSeed(WorldGenerationConfig, EKalmalaWorldField::Flora);
    FRandomStream RandomStream(static_cast<int32>((FloraSeed ^ KalmalaGeneratedTerrainPatch::TreeSeedSalt) >> 32));

    TArray<FVector> TrunkVertices;
    TArray<int32> TrunkTriangles;
    TArray<FVector> TrunkNormals;
    TArray<FVector2D> TrunkUVs;
    TArray<FLinearColor> TrunkColors;
    TArray<FProcMeshTangent> TrunkTangents;
    TArray<FVector> CanopyVertices;
    TArray<int32> CanopyTriangles;
    TArray<FVector> CanopyNormals;
    TArray<FVector2D> CanopyUVs;
    TArray<FLinearColor> CanopyColors;
    TArray<FProcMeshTangent> CanopyTangents;
    MeadowTreeCount = 0;
    for (int32 CandidateIndex = 0; CandidateIndex < KalmalaGeneratedTerrainPatch::ElderwoodTreeCandidateCount; ++CandidateIndex)
    {
        const FVector2D LocalPosition(
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::TreeEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::TreeEdgeMargin),
            RandomStream.FRandRange(-HalfSurfaceSize + KalmalaGeneratedTerrainPatch::TreeEdgeMargin, HalfSurfaceSize - KalmalaGeneratedTerrainPatch::TreeEdgeMargin));
        const FVector2D SamplePosition = PatchCenter + LocalPosition;
        if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, SamplePosition, 1000)) continue;
        const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, SamplePosition);
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(Fields);
        if (Biome != EKalmalaBiome::Meadows && Biome != EKalmalaBiome::Elderwood)
        {
            continue;
        }

        if (Biome == EKalmalaBiome::Meadows && CandidateIndex >= KalmalaGeneratedTerrainPatch::MeadowTreeCandidateCount)
        {
            continue;
        }
        // Flora continuously controls Elderwood density: lower-flora pockets stay clear without becoming authored spaces.
        if (Biome == EKalmalaBiome::Elderwood && RandomStream.FRand() > FMath::Clamp((Fields.Flora - 0.60f) / 0.24f, 0.20f, 0.92f))
        {
            continue;
        }

        const bool bElderwood = Biome == EKalmalaBiome::Elderwood;
        const float TrunkHeight = RandomStream.FRandRange(bElderwood ? 520.0f : 340.0f, bElderwood ? 850.0f : 620.0f);
        const float TrunkRadius = RandomStream.FRandRange(bElderwood ? 28.0f : 14.0f, bElderwood ? 48.0f : 25.0f);
        const float CanopyRadius = RandomStream.FRandRange(bElderwood ? 185.0f : 105.0f, bElderwood ? 285.0f : 170.0f);
        const float CanopyHeight = RandomStream.FRandRange(bElderwood ? 220.0f : 140.0f, bElderwood ? 360.0f : 240.0f);
        const float SurfaceHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, SamplePosition);
        const float TreeYaw = RandomStream.FRandRange(0.0f, 360.0f);
        KalmalaGeneratedTerrainPatch::AppendTaperedTree(
            TrunkVertices, TrunkTriangles, TrunkNormals, TrunkUVs, TrunkColors, TrunkTangents,
            FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight), TrunkRadius, TrunkHeight, CanopyRadius, CanopyHeight, TreeYaw,
            CanopyVertices, CanopyTriangles, CanopyNormals, CanopyUVs, CanopyColors, CanopyTangents);
        if (bElderwood)
        {
            KalmalaGeneratedTerrainPatch::AppendRootButtresses(TrunkVertices, TrunkTriangles, TrunkNormals, TrunkUVs, TrunkColors, TrunkTangents, FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight), TrunkRadius, TreeYaw);
        }
        ++MeadowTreeCount;
    }

    MeadowTreeTrunks->ClearAllMeshSections();
    MeadowTreeCanopies->ClearAllMeshSections();
    if (!TrunkTriangles.IsEmpty())
    {
        MeadowTreeTrunks->CreateMeshSection_LinearColor(0, TrunkVertices, TrunkTriangles, TrunkNormals, TrunkUVs, TrunkColors, TrunkTangents, false);
    }
    if (!CanopyTriangles.IsEmpty())
    {
        MeadowTreeCanopies->CreateMeshSection_LinearColor(0, CanopyVertices, CanopyTriangles, CanopyNormals, CanopyUVs, CanopyColors, CanopyTangents, false);
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
