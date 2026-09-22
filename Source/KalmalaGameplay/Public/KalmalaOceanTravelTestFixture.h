#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaOceanTravelTestFixture.generated.h"

class UProceduralMeshComponent;
class USceneComponent;

/**
 * Development-only water ribbon used by the ocean-travel regression.
 * The server chooses both endpoints and replicates the descriptor so peers
 * never search unrelated generated terrain for a test prerequisite.
 */
UCLASS(NotPlaceable)
class KALMALAGAMEPLAY_API AKalmalaOceanTravelTestFixture : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaOceanTravelTestFixture();

    void Initialize(FVector2D InEntryPoint, FVector2D InTargetPoint, float InHalfWidth, float InWaterDepth);
    bool TryGetWaterDepth(FVector2D Position, float& OutDepth) const;
    FVector2D GetEntryPoint() const { return EntryPoint; }
    FVector2D GetTargetPoint() const { return TargetPoint; }
    bool IsConfigured() const { return bIsConfigured; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Test Fixture")
    TObjectPtr<UProceduralMeshComponent> WaterSurface;

    UPROPERTY(ReplicatedUsing = OnRep_FixtureData)
    FVector2D EntryPoint = FVector2D::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_FixtureData)
    FVector2D TargetPoint = FVector2D::ZeroVector;

    UPROPERTY(ReplicatedUsing = OnRep_FixtureData)
    float HalfWidth = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_FixtureData)
    float WaterDepth = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_FixtureData)
    bool bIsConfigured = false;

    bool bWaterSurfaceBuilt = false;

    UFUNCTION()
    void OnRep_FixtureData();

    void BuildWaterSurface();
};
