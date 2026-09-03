#include "KalmalaGeneratedTerrainPatch.h"

#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "KalmalaBiomeClassifier.h"
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
    bMeadowRocksBuilt = false;
    bMeadowTreesBuilt = false;
    BuildVisualSurface();
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
