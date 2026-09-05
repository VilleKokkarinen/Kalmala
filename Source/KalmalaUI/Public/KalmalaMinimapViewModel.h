#pragma once

#include "CoreMinimal.h"
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

    UFUNCTION(BlueprintPure, Category = "Minimap")
    bool IsReady() const { return bIsReady; }

    UFUNCTION(BlueprintPure, Category = "Minimap")
    float GetPlayerFacingDegrees() const { return PlayerFacingDegrees; }

    /** Changes only the local presentation radius; it is never replicated or persisted. */
    void SetMapRadius(float InMapRadius);

    uint32 GetPresentationRevision() const { return PresentationRevision; }

    UFUNCTION(BlueprintPure, Category = "Minimap")
    const TArray<FKalmalaMinimapTerrainSample>& GetTerrainSamples() const { return TerrainSamples; }

private:
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> OwningPlayer;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "100.0"))
    float MapRadius = 5000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "3", ClampMax = "129"))
    int32 SamplesPerAxis = 129;

    FVector2D LastLocation = FVector2D::ZeroVector;
    uint64 LastSeed = 0;
    int32 LastGeneratorRevision = 0;
    float LastRadius = 0.0f;
    uint32 PresentationRevision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
    float PlayerFacingDegrees = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
    TArray<FKalmalaMinimapTerrainSample> TerrainSamples;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap", meta = (AllowPrivateAccess = "true"))
    bool bIsReady = false;
};
