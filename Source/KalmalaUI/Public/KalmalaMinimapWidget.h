#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaMinimapWidget.generated.h"

class UKalmalaMinimapViewModel;

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
    void InitializeForLocalPlayer(APlayerController* InOwningPlayer);

    /** Pure geometry seam: all drawn map points must remain inside this circle. */
    static bool IsInsideCircularMap(const FVector2D& NormalizedMapPosition);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UKalmalaMinimapViewModel> ViewModel;

    UPROPERTY(EditDefaultsOnly, Category = "Minimap", meta = (ClampMin = "96.0", ClampMax = "512.0"))
    float MapDiameter = 208.0f;

    float RefreshAccumulator = 0.0f;
};
