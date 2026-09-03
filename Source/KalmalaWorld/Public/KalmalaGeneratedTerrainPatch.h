#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaGeneratedTerrainPatch.generated.h"

class USceneComponent;
class UProceduralMeshComponent;
class UInstancedStaticMeshComponent;

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
    TObjectPtr<UInstancedStaticMeshComponent> MeadowRocks;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UInstancedStaticMeshComponent> MeadowTreeTrunks;

    UPROPERTY(VisibleAnywhere, Category = "World Generation")
    TObjectPtr<UInstancedStaticMeshComponent> MeadowTreeCanopies;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FKalmalaWorldGenerationConfig WorldGenerationConfig;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FVector2D PatchCenter = FVector2D::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    bool bIsConfigured = false;

    bool bVisualSurfaceBuilt = false;
    bool bMeadowRocksBuilt = false;
    bool bMeadowTreesBuilt = false;

    UFUNCTION()
    void OnRep_GenerationData();

    bool BuildVisualSurface();
    bool BuildMeadowRocks();
    bool BuildMeadowTrees();
};
