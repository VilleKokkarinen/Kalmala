#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaGeneratedTerrainPatch.generated.h"

class USceneComponent;
class UProceduralMeshComponent;

/**
 * Server-owned descriptor for a continuous authoritative terrain surface and
 * its locally-derived client counterpart around a generated start.
 */
UCLASS(NotPlaceable)
class KALMALAWORLD_API AKalmalaGeneratedTerrainPatch : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaGeneratedTerrainPatch();

    void Initialize(const FKalmalaWorldGenerationConfig& InConfig, const FVector2D InPatchCenter);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UProceduralMeshComponent> TerrainSurface;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UProceduralMeshComponent> SurfaceWater;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UProceduralMeshComponent> ShimmeringLakeWater;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UProceduralMeshComponent> ShimmeringLakeShore;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UProceduralMeshComponent> MeadowRocks;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UProceduralMeshComponent> MeadowTreeTrunks;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UProceduralMeshComponent> MeadowTreeCanopies;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FKalmalaWorldGenerationConfig WorldGenerationConfig;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FVector2D PatchCenter = FVector2D::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    bool bIsConfigured = false;

    bool bVisualSurfaceBuilt = false;
    bool bSurfaceWaterBuilt = false;
    bool bShimmeringLakeTreatmentBuilt = false;
    bool bBiomeDebugOverlayBuilt = false;
    bool bMeadowRocksBuilt = false;
    bool bMeadowTreesBuilt = false;
    int32 MeadowRockCount = 0;
    int32 MeadowTreeCount = 0;

    UFUNCTION()
    void OnRep_GenerationData();

    bool BuildVisualSurface();
    bool BuildSurfaceWater();
    bool BuildShimmeringLakeTreatment();
    bool BuildBiomeDebugOverlay();
    bool BuildMeadowRocks();
    bool BuildMeadowTrees();
};
