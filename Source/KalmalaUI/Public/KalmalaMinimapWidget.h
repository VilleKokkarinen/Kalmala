#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaMinimapWidget.generated.h"

class UKalmalaMinimapViewModel;
class UTexture2D;

/**
 * Local HUD presentation for the companion minimap.  It deliberately draws
 * only the view model's seed-derived terrain and water samples plus the
 * owning player's centred facing marker.
 */
UCLASS()
class KALMALAUI_API UKalmalaMinimapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    static constexpr float ViewportInset = 12.0f;
    static constexpr float StatusGroupGap = 12.0f;
    static constexpr float DefaultMapDiameter = 208.0f;

    void InitializeForLocalPlayer(APlayerController* InOwningPlayer, float InitialZoom = 5000.0f);
    void ConfigureViewportPlacement();
    /** Position offset for a right-anchored status group immediately left of this map's configured slot. */
    FVector2D GetStatusGroupViewportPosition() const;

    static FVector2D GetDefaultStatusGroupViewportPosition(float InMapDiameter = DefaultMapDiameter);

    /** Pure geometry seam: all drawn map points must remain inside this circle. */
    static bool IsInsideCircularMap(const FVector2D& NormalizedMapPosition);

    /** Tests the top-right HUD footprint independently of viewport aspect ratio and UI scale. */
    static bool IsTopRightPlacementValid(const FVector2D& ViewportSize, float InMapDiameter, float Margin, float UiScale);

    /** Pure local zoom seam used by input routing and automation coverage. */
    static float ClampZoom(float RequestedZoom, float MinZoom, float MaxZoom);
    static bool ShouldAcceptZoomInput(bool bCanProcessNormalGameInput);

    /** Applies a local mouse-wheel delta. No world, server, or save state is changed. */
    void AdjustZoom(float WheelDelta);
    float GetCurrentZoom() const { return CurrentZoom; }

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    void UpdateMapTexture();

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> MapTexture;

    FSlateBrush MapBrush;
    uint32 UploadedRevision = 0;
    mutable bool bLoggedPaint = false;
    bool bRequestedVerificationScreenshot = false;
    float VerificationElapsed = 0.0f;

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaMinimapViewModel> ViewModel;

    UPROPERTY(EditDefaultsOnly, Category = "Minimap", meta = (ClampMin = "96.0", ClampMax = "512.0"))
    float MapDiameter = DefaultMapDiameter;

    UPROPERTY(EditDefaultsOnly, Category = "Minimap|Zoom", meta = (ClampMin = "100.0"))
    float MinZoom = 2500.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Minimap|Zoom", meta = (ClampMin = "100.0"))
    float MaxZoom = 10000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Minimap|Zoom", meta = (ClampMin = "1.0"))
    float ZoomStep = 750.0f;

    UPROPERTY(VisibleAnywhere, Category = "Minimap|Zoom")
    float CurrentZoom = 5000.0f;

    float RefreshAccumulator = 0.0f;
};
