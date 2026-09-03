#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaGeneratedTerrainPatch.generated.h"

class USceneComponent;
class UProceduralMeshComponent;
class AKalmalaGeneratedTerrainTile;

/**
 * Server-owned descriptor for an invisible authoritative collision patch and
 * a locally-derived visual terrain surface around a generated start.
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

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FKalmalaWorldGenerationConfig WorldGenerationConfig;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FVector2D PatchCenter = FVector2D::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    bool bIsConfigured = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AKalmalaGeneratedTerrainTile>> CollisionTiles;

    bool bVisualSurfaceBuilt = false;

    UFUNCTION()
    void OnRep_GenerationData();

    void SpawnCollisionTiles();
    bool BuildVisualSurface();
};
