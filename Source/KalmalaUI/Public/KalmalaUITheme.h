#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"

class FConfigFile;
class UBorder;
class UTextBlock;

/** Local presentation only. Invalid keys retain the established HUD defaults. */
struct KALMALAUI_API FKalmalaUITheme
{
    FLinearColor Panel = FLinearColor(0.025f, 0.035f, 0.04f, 0.94f);
    FLinearColor Heading = FLinearColor(0.75f, 0.82f, 0.79f, 1.0f);
    FLinearColor Text = FLinearColor(0.93f, 0.96f, 0.94f, 1.0f);
    FLinearColor HighContrastPanel = FLinearColor(0, 0, 0, 0.98f);
    int32 HeadingSize = 10;
    int32 BodySize = 13;
    int32 EmphasisSize = 17;
    float PaddingX = 12;
    float PaddingY = 9;
    float RowSpacing = 5;

    static FKalmalaUITheme FromConfig(const FConfigFile& Config);
    static const FKalmalaUITheme& Get();
    int32 ScaledFontSize(int32 BaseSize, int32 TextScalePercent) const;
    FMargin PanelPadding() const { return FMargin(PaddingX, PaddingY); }
    void ApplyPanel(UBorder& Border, int32 ContrastMode) const;
    void ApplyText(UTextBlock& Label, int32 BaseSize, bool bHeading,
        int32 TextScalePercent, int32 ContrastMode) const;
};
