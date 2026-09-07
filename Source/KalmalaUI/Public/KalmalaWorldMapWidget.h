#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
    void UpdateMapTexture();
    void PanByScreenDelta(const FVector2D& ScreenDelta, const FVector2D& MapSize);
    void ZoomAtScreenPosition(float WheelDelta, const FVector2D& ScreenPosition, const FVector2D& MapSize);

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaMinimapViewModel> ViewModel;
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> MapTexture;

    FSlateBrush MapBrush;
    uint32 UploadedRevision = 0;
    bool bMapOpen = false;
    bool bDragging = false;
    FVector2D LastDragPosition = FVector2D::ZeroVector;
    float MapZoom = 18000.0f;
    float RefreshAccumulator = 0.0f;

    static constexpr float MinZoom = 2500.0f;
    static constexpr float MaxZoom = 50000.0f;
    static constexpr float ZoomStep = 0.18f;
};
