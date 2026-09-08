#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "KalmalaWorldGenerationConfig.h"
#include "UObject/Object.h"
#include "KalmalaMinimapViewModel.generated.h"

class APlayerController;
struct FKalmalaWorldGenerationConfig;

/** A single local, seed-derived terrain sample consumed by the companion minimap presentation. */
USTRUCT(BlueprintType)
struct FKalmalaMinimapTerrainSample
{
    GENERATED_BODY()

    /** Map-space coordinate relative to the owning pawn. The owning pawn is always (0, 0). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
    FVector2D MapPosition = FVector2D::ZeroVector;

    /** Locally derived terrain height, used only for presentation shading. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
    float TerrainHeight = 0.0f;

    /** Locally derived water treatment, never a gameplay boundary. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
    bool bIsWater = false;

    /** Original biome texture sampled in world space; cosmetic output only. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
    FLinearColor TerrainColour = FLinearColor::Black;
};

/**
 * Local presentation model for the companion minimap. It reads the replicated
 * generated-world identity and its owning pawn's replicated transform, then
 * derives terrain/water samples without querying population or mutating state.
 */
UCLASS(BlueprintType)
class KALMALAUI_API UKalmalaMinimapViewModel : public UObject
{
    GENERATED_BODY()

public:
    /** Associates this local-only model with its owning controller. */
    UFUNCTION(BlueprintCallable, Category = "Minimap")
    void Initialize(APlayerController* InOwningPlayer);

    /** Rebuilds the local terrain/water presentation around the owning pawn. */
    UFUNCTION(BlueprintCallable, Category = "Minimap")
    bool Refresh();

    /** Pure seam for widgets and tests: derives a square sample grid centred on PlayerLocation. */
    static TArray<FKalmalaMinimapTerrainSample> BuildTerrainSamples(
        const FKalmalaWorldGenerationConfig& WorldConfig,
        const FVector2D& PlayerLocation,
        float MapRadius,
        int32 SamplesPerAxis);
    static TArray<FKalmalaMinimapTerrainSample> BuildTerrainSamples(
        const FKalmalaWorldGenerationConfig& WorldConfig,
        const FVector2D& PlayerLocation,
        FVector2D MapExtent,
        FIntPoint SampleDimensions);

    UFUNCTION(BlueprintPure, Category = "Minimap")
    bool IsReady() const { return bIsReady; }

    UFUNCTION(BlueprintPure, Category = "Minimap")
    float GetPlayerFacingDegrees() const { return PlayerFacingDegrees; }

    /** Changes only the local presentation radius; it is never replicated or persisted. */
    void SetMapRadius(float InMapRadius);
    void SetMapAspectRatio(float InAspectRatio);
    void SetMapSampleDimensions(FIntPoint InDimensions);
    FVector2D GetMapExtent() const { return FVector2D(MapRadius * MapAspectRatio, MapRadius); }
    FIntPoint GetMapSampleDimensions() const { return SampleDimensions; }

    /** Centres a local map presentation at a chosen world location without changing gameplay state. */
    void SetMapCentre(const FVector2D& InMapCentre);
    void RecenterOnOwningPlayer();
    FVector2D GetMapCentre() const { return bUseCustomCentre ? CustomCentre : LastLocation; }

    uint32 GetPresentationRevision() const { return PresentationRevision; }

    UFUNCTION(BlueprintPure, Category = "Minimap")
    const TArray<FKalmalaMinimapTerrainSample>& GetTerrainSamples() const { return TerrainSamples; }

private:
    friend class FKalmalaMinimapAsyncTest;
    bool RefreshTerrain(const FKalmalaWorldGenerationConfig& Config, const FVector2D& Location);
    // The worker owns value snapshots only; destroying the model never waits for it.
    TFuture<TArray<FKalmalaMinimapTerrainSample>> PendingSamples;
    FKalmalaWorldGenerationConfig PendingConfig;
    FVector2D PendingLocation = FVector2D::ZeroVector;
    float PendingRadius = 0.0f;
    FVector2D PendingExtent = FVector2D::ZeroVector;
    FIntPoint PendingDimensions = FIntPoint::ZeroValue;

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> OwningPlayer;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "100.0"))
    float MapRadius = 5000.0f;

    float MapAspectRatio = 1.0f;
    FIntPoint SampleDimensions = FIntPoint(129, 129);

    FVector2D LastLocation = FVector2D::ZeroVector;
    uint64 LastSeed = 0;
    int32 LastGeneratorRevision = 0;
    float LastRadius = 0.0f;
    FVector2D LastExtent = FVector2D::ZeroVector;
    FIntPoint LastDimensions = FIntPoint::ZeroValue;
    uint32 PresentationRevision = 0;
    FVector2D CustomCentre = FVector2D::ZeroVector;
    bool bUseCustomCentre = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
    float PlayerFacingDegrees = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
    TArray<FKalmalaMinimapTerrainSample> TerrainSamples;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
    bool bIsReady = false;
};
