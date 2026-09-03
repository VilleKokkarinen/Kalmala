#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaGeneratedTerrainPatch.generated.h"

class USceneComponent;
class AKalmalaGeneratedTerrainTile;

/** Server-owned descriptor that spawns replicated collision tiles around a generated start. */
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

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FKalmalaWorldGenerationConfig WorldGenerationConfig;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    FVector2D PatchCenter = FVector2D::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_GenerationData)
    bool bIsConfigured = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AKalmalaGeneratedTerrainTile>> CollisionTiles;

    UFUNCTION()
    void OnRep_GenerationData();

    void SpawnCollisionTiles();
};
