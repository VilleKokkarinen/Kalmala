#include "KalmalaGeneratedTerrainPatch.h"

#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationSeeds.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"

namespace KalmalaGeneratedTerrainPatch
{
    constexpr int32 SurfaceCellsPerSide = 24;
    constexpr int32 RockCandidateCount = 48;
    constexpr float RockEdgeMargin = 150.0f;
    constexpr int32 TreeCandidateCount = 18;
    constexpr float TreeEdgeMargin = 260.0f;
    constexpr uint64 TreeSeedSalt = 0xD1B54A32D192ED03ull;
    constexpr float LakeShoreWidth = 24.0f;

    FColor GetBiomeDebugColor(const EKalmalaBiome Biome)
    {
        switch (Biome)
        {
        case EKalmalaBiome::Ocean: return FColor(23, 88, 160);
        case EKalmalaBiome::ShimmeringLakes: return FColor(61, 177, 190);
        case EKalmalaBiome::Elderwood: return FColor(30, 100, 47);
        case EKalmalaBiome::MossyMire: return FColor(76, 113, 55);
        case EKalmalaBiome::FreezingTundra: return FColor(213, 236, 238);
        case EKalmalaBiome::ThunderMountains: return FColor(104, 98, 112);
        case EKalmalaBiome::Meadows: return FColor(131, 174, 76);
        default: return FColor::Magenta;
        }
    }

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
    bBiomeDebugOverlayBuilt = false;
    bMeadowRocksBuilt = false;
    bMeadowTreesBuilt = false;
    MeadowRockCount = 0;
    MeadowTreeCount = 0;
    BuildVisualSurface();
    BuildBiomeDebugOverlay();
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
        UE_LOG(LogTemp, Display, TEXT("Server built %d deterministic Meadow trees."), MeadowTreeCount);
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
        BuildBiomeDebugOverlay();
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
            UE_LOG(LogTemp, Display, TEXT("Client built %d deterministic Meadow trees from the replicated patch descriptor."), MeadowTreeCount);
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

bool AKalmalaGeneratedTerrainPatch::BuildBiomeDebugOverlay()
{
    if (bBiomeDebugOverlayBuilt || !bIsConfigured || !WorldGenerationConfig.IsValid() || GetWorld() == nullptr || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaBiomeDebug")))
    {
        return false;
    }

    constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::TilesPerSide * FKalmalaTerrainPatchLayout::TileSize;
    constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
    constexpr int32 CellsPerSide = KalmalaGeneratedTerrainPatch::SurfaceCellsPerSide;
    constexpr int32 BiomeCount = static_cast<int32>(EKalmalaBiome::Ocean) + 1;
    TArray<FVector> VerticesByBiome[BiomeCount];
    TArray<int32> TrianglesByBiome[BiomeCount];

    for (int32 GridY = 0; GridY < CellsPerSide; ++GridY)
    {
        const float BottomY = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(GridY) / CellsPerSide);
        const float TopY = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(GridY + 1) / CellsPerSide);
        for (int32 GridX = 0; GridX < CellsPerSide; ++GridX)
        {
            const float LeftX = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(GridX) / CellsPerSide);
            const float RightX = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(GridX + 1) / CellsPerSide);
            const FVector2D SamplePosition = PatchCenter + FVector2D((LeftX + RightX) * 0.5f, (BottomY + TopY) * 0.5f);
            const int32 BiomeIndex = static_cast<int32>(FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, SamplePosition)));
            TArray<FVector>& Vertices = VerticesByBiome[BiomeIndex];
            TArray<int32>& Triangles = TrianglesByBiome[BiomeIndex];
            const int32 FirstVertex = Vertices.Num();

            const auto AddTerrainVertex = [this](const float X, const float Y)
            {
                const FVector2D Position = PatchCenter + FVector2D(X, Y);
                return GetActorLocation() + FVector(X, Y, FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Position))
                    + FKalmalaTerrainHeightSampler::SampleSurfaceNormal(WorldGenerationConfig, Position) * 16.0f;
            };

            Vertices.Append({
                AddTerrainVertex(LeftX, BottomY),
                AddTerrainVertex(LeftX, TopY),
                AddTerrainVertex(RightX, BottomY),
                AddTerrainVertex(RightX, TopY)});
            Triangles.Append({FirstVertex, FirstVertex + 1, FirstVertex + 2, FirstVertex + 2, FirstVertex + 1, FirstVertex + 3});
        }
    }

    for (int32 BiomeIndex = 0; BiomeIndex < BiomeCount; ++BiomeIndex)
    {
        if (!TrianglesByBiome[BiomeIndex].IsEmpty())
        {
            DrawDebugMesh(GetWorld(), VerticesByBiome[BiomeIndex], TrianglesByBiome[BiomeIndex], GetBiomeDebugColor(static_cast<EKalmalaBiome>(BiomeIndex)), true, -1.0f, SDPG_World);
        }
    }

    bBiomeDebugOverlayBuilt = true;
    UE_LOG(LogTemp, Display, TEXT("Built the local biome debug overlay. Use -KalmalaBiomeDebug only for development launches."));
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

        const float TrunkHeight = RandomStream.FRandRange(340.0f, 620.0f);
        const float TrunkRadius = RandomStream.FRandRange(14.0f, 25.0f);
        const float CanopyRadius = RandomStream.FRandRange(105.0f, 170.0f);
        const float CanopyHeight = RandomStream.FRandRange(140.0f, 240.0f);
        const float SurfaceHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, SamplePosition);
        const float TreeYaw = RandomStream.FRandRange(0.0f, 360.0f);
        KalmalaGeneratedTerrainPatch::AppendTaperedTree(
            TrunkVertices, TrunkTriangles, TrunkNormals, TrunkUVs, TrunkColors, TrunkTangents,
            FVector(LocalPosition.X, LocalPosition.Y, SurfaceHeight), TrunkRadius, TrunkHeight, CanopyRadius, CanopyHeight, TreeYaw,
            CanopyVertices, CanopyTriangles, CanopyNormals, CanopyUVs, CanopyColors, CanopyTangents);
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
