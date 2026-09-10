#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldMapPinsSaveGame.h"
#include "UObject/StrongObjectPtr.h"
#include "KalmalaWorldMapWidget.generated.h"

class UKalmalaMinimapViewModel;
class UKalmalaWorldMapExplorationSaveGame;
class UKalmalaWorldMapPinsSaveGame;
class UTexture2D;

/** Local fog treatments deliberately reserve a distinct colour for a later, opt-in shared-cartography feature. */
enum class EKalmalaWorldMapFogTreatment : uint8
{
    Unexplored,
    CurrentPersonal,
    RememberedPersonal,
    ReservedShared
};

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
    /** Records the owning pawn's travels even while the map is collapsed. No raster work. */
    void TickExploration(float DeltaTime);

    static float ClampMapZoom(float RequestedZoom, float MinZoom, float MaxZoom);
    /** Pure local fog seam. Only the owning pawn position may open the reveal circle. */
    static bool IsWithinLocalRevealRadius(FVector2D WorldPosition, FVector2D OwningPawnLocation, float RevealRadius);
    static bool CanShowCoopLocation(FVector2D WorldPosition, FVector2D OwningPawnLocation,
        const UKalmalaWorldMapExplorationSaveGame* Exploration);
    /** Converts a world location to the local map's normalized presentation space. */
    static FVector2D WorldToMapNormalized(FVector2D WorldPosition, FVector2D MapCentre, FVector2D MapExtent);
    /** Inverse local presentation conversion used for pointer/pin coordinates at every zoom. */
    static FVector2D MapNormalizedToWorld(FVector2D NormalizedPosition, FVector2D MapCentre, FVector2D MapExtent);
    /** Local owning-pawn heading in map space; no other pawn is queried. */
    static FVector2D GetFacingDirection(float FacingDegrees);
    /** Chooses a world-aligned reference grid spacing from the current local map radius. */
    static float ChooseGridSpacing(float MapRadius);
    /** Bounds a local player-entered label before it can become presentation state. */
    static FString SanitizePinLabel(FString Label);
    static bool IsValidPinLabel(const FString& Label);
    /** Textual pin state stays distinct when colour, marker shape, or visibility is unavailable. */
    static FString GetPinAccessibilityLabel(const FKalmalaWorldMapPersonalPin& Pin);
    /** Original map-fog palette; ReservedShared has no data source until shared cartography is authorized. */
    static FColor GetFogTreatmentColor(EKalmalaWorldMapFogTreatment Treatment);
    static TArray<FColor> BuildFogPixels(FVector2D MapCentre, FVector2D MapExtent, FVector2D OwningPawnLocation, FIntPoint Dimensions,
        const UKalmalaWorldMapExplorationSaveGame* Exploration = nullptr);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent) override;

private:
    friend class FKalmalaWorldMapWidgetTest;
    struct FWorldMapTile
    {
        struct FBuildResult
        {
            TArray<FColor> Pixels;
            double WorkerSeconds = 0.0;
        };
        TFuture<FBuildResult> PendingPixels;
        TStrongObjectPtr<UTexture2D> Texture;
        uint32 RequestEpoch = 0;
        uint32 LastUsedEpoch = 0;
        uint32 PixelHash = 0;
    };
    void RefreshTiles(const FVector2D& MapSize);
    void StartTile(const FIntPoint& TileCoordinate, const FKalmalaWorldGenerationConfig& Config);
    void UploadCompletedTiles();
    void EvictUnusedTiles();
    void UpdateFogTexture(FVector2D MapCentre, FVector2D MapExtent, FVector2D OwningPawnLocation, FIntPoint Dimensions);
    void EnsureExplorationForWorld(const FKalmalaWorldGenerationConfig& Config);
    void RecordLocalExploration(FVector2D OwningPawnLocation);
    FString GetExplorationSaveSlot(const FKalmalaWorldGenerationConfig& Config) const;
    void EnsurePinsForWorld(const FKalmalaWorldGenerationConfig& Config);
    void PersistPins();
    FString GetPinsSaveSlot(const FKalmalaWorldGenerationConfig& Config) const;
    void LogDeveloperTileFingerprint();
    void LogDeveloperProfile();
    void InvalidateOutstandingTileJobs();
    static TArray<FColor> BuildTilePixels(FKalmalaWorldGenerationConfig Config, FIntPoint TileCoordinate);
    static TArray<FIntPoint> BuildPrioritizedTileCoordinates(const FVector2D& Centre, const FVector2D& Extent);
    void PanByScreenDelta(const FVector2D& ScreenDelta, const FVector2D& MapSize);
    void ZoomAtScreenPosition(float WheelDelta, const FVector2D& ScreenPosition, const FVector2D& MapSize);
    FVector2D ScreenToWorld(const FVector2D& ScreenPosition, const FVector2D& MapSize) const;
    int32 FindVisiblePinAtScreenPosition(const FVector2D& ScreenPosition, const FVector2D& MapSize) const;
    void BeginPinPlacement(const FVector2D& WorldLocation);
    void CommitPinPlacement();
    void CancelPinPlacement();
    void SelectNextPin();
    bool ToggleSelectedPinCompletion();
    bool ToggleSelectedPinVisibility();
    bool RemoveSelectedPin();
    void PanByKeyboardDelta(const FVector2D& ScreenDelta);
    void ZoomAtMapCentre(float WheelDelta);
    void DrawPins(const FGeometry& AllottedGeometry, const FVector2D& MapSize, int32 LayerId, FSlateWindowElementList& OutDrawElements) const;
    void DrawCoopAwareness(const FGeometry& Geometry, FVector2D MapSize, int32 LayerId, FSlateWindowElementList& Elements) const;
    void SendMapPing(FVector2D Location);
    FString PingFeedback;
    static FLinearColor GetPinColour(EKalmalaWorldMapPinStyle Style);

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaMinimapViewModel> ViewModel;
    TMap<FIntPoint, FWorldMapTile> Tiles;
    TArray<FKalmalaWorldMapPersonalPin> LocalPins;
    TStrongObjectPtr<UTexture2D> FogTexture;
    FSlateBrush FogBrush;
    UPROPERTY(Transient)
    TObjectPtr<UKalmalaWorldMapExplorationSaveGame> ExplorationSave;
    UPROPERTY(Transient)
    TObjectPtr<UKalmalaWorldMapPinsSaveGame> PinsSave;
    FKalmalaWorldGenerationConfig TileConfig;
    uint32 TileEpoch = 1;
    bool bMapOpen = false;
    bool bDragging = false;
    bool bPinLabelEntry = false;
    int32 SelectedPinIndex = INDEX_NONE;
    FVector2D LastDragPosition = FVector2D::ZeroVector;
    FVector2D PendingPinLocation = FVector2D::ZeroVector;
    FString PendingPinLabel;
    EKalmalaWorldMapPinStyle PendingPinStyle = EKalmalaWorldMapPinStyle::Cairn;
    float MapZoom = 18000.0f;
    float RefreshAccumulator = 0.0f;
    float ExplorationAccumulator = 0.5f;
    bool bDeveloperVerificationLogged = false;
    bool bDeveloperTileFingerprintLogged = false;
    bool bDeveloperFogVerificationLogged = false;
    bool bDeveloperClosedExplorationLogged = false;
    bool bDeveloperTileInputsUnavailableLogged = false;
    bool bDeveloperProfileLogged = false;
    bool bRequestedVerificationScreenshot = false;
    mutable bool bDeveloperPaintVerified = false;
    /** True only when this widget instance accepted a matching personal-coverage slot. */
    bool bExplorationLoadedForWorld = false;
    float VerificationElapsed = 0.0f;
    double DeveloperProfileOpenedAt = 0.0;
    double DeveloperProfileWorkerSeconds = 0.0;
    double DeveloperProfileMaxWorkerSeconds = 0.0;
    double DeveloperProfileGameThreadSeconds = 0.0;
    double DeveloperProfileMaxGameThreadSeconds = 0.0;
    int32 DeveloperProfileGameThreadTicks = 0;

    static constexpr float MinZoom = 2500.0f;
    static constexpr float MaxZoom = 50000.0f;
    static constexpr float ZoomStep = 0.18f;
    static constexpr float TileWorldSize = 10000.0f;
    static constexpr int32 TileSamplesPerAxis = 33;
    static constexpr int32 MaxCachedTiles = 64;
    // Local coverage is versioned and persisted independently from world deltas.
    static constexpr float LocalRevealRadius = 6500.0f;
    static constexpr int32 MaxPinLabelLength = UKalmalaWorldMapPinsSaveGame::MaxPinLabelLength;
};
