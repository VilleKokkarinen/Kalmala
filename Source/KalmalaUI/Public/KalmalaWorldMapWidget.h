#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaWorldGenerationConfig.h"
#include "UObject/StrongObjectPtr.h"
#include "KalmalaWorldMapWidget.generated.h"

class UKalmalaMinimapViewModel;
class UTexture2D;

/** Large local map surface built from the same disposable seed-derived samples as the minimap. */
UCLASS()
class KALMALAUI_API UKalmalaWorldMapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeForLocalPlayer(APlayerController* InOwningPlayer);
    void ConfigureViewportPlacement();
    void Open();
    void Close();
    bool IsMapOpen() const { return bMapOpen; }
    void Recenter();
    void RunDeveloperVerification();
    /** Services local asynchronous tiles from either Slate or the owning local-player subsystem. */
    void TickTilePresentation(float DeltaTime);

    static float ClampMapZoom(float RequestedZoom, float MinZoom, float MaxZoom);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    friend class FKalmalaWorldMapWidgetTest;
    struct FWorldMapTile
    {
        TFuture<TArray<FColor>> PendingPixels;
        TStrongObjectPtr<UTexture2D> Texture;
        uint32 RequestEpoch = 0;
        uint32 LastUsedEpoch = 0;
        uint32 PixelHash = 0;
    };

    void RefreshTiles(const FVector2D& MapSize);
    void StartTile(const FIntPoint& TileCoordinate, const FKalmalaWorldGenerationConfig& Config);
    void UploadCompletedTiles();
    void EvictUnusedTiles();
    void LogDeveloperTileFingerprint();
    void InvalidateOutstandingTileJobs();
    static TArray<FColor> BuildTilePixels(FKalmalaWorldGenerationConfig Config, FIntPoint TileCoordinate);
    static TArray<FIntPoint> BuildPrioritizedTileCoordinates(const FVector2D& Centre, const FVector2D& Extent);
    void PanByScreenDelta(const FVector2D& ScreenDelta, const FVector2D& MapSize);
    void ZoomAtScreenPosition(float WheelDelta, const FVector2D& ScreenPosition, const FVector2D& MapSize);

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaMinimapViewModel> ViewModel;
    TMap<FIntPoint, FWorldMapTile> Tiles;
    FKalmalaWorldGenerationConfig TileConfig;
    uint32 TileEpoch = 1;
    bool bMapOpen = false;
    bool bDragging = false;
    FVector2D LastDragPosition = FVector2D::ZeroVector;
    float MapZoom = 18000.0f;
    float RefreshAccumulator = 0.0f;
    bool bDeveloperVerificationLogged = false;
    bool bDeveloperTileFingerprintLogged = false;
    bool bDeveloperTileInputsUnavailableLogged = false;
    bool bRequestedVerificationScreenshot = false;
    float VerificationElapsed = 0.0f;

    static constexpr float MinZoom = 2500.0f;
    static constexpr float MaxZoom = 50000.0f;
    static constexpr float ZoomStep = 0.18f;
    static constexpr float TileWorldSize = 10000.0f;
    static constexpr int32 TileSamplesPerAxis = 33;
    static constexpr int32 MaxCachedTiles = 64;
};
