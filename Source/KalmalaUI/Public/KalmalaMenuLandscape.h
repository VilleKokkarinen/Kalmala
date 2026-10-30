#pragma once

#include "Blueprint/UserWidget.h"
#include "KalmalaMenuLandscape.generated.h"

class UCanvasPanel;
class UWidgetTree;
class UBorder;

/** Original, deterministic front-end illustration. No world actors or input ownership. */
UCLASS()
class KALMALAUI_API UKalmalaMenuLandscape : public UUserWidget
{
    GENERATED_BODY()
public:
    static void AddToCanvas(UWidgetTree& Tree, UCanvasPanel& Canvas);
    static void FitPanel(UBorder* Panel, float ViewportWidth);
protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bEnabled) const override;
};
