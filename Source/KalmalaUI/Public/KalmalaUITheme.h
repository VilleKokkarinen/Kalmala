#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"

class FConfigFile;
class UBorder;
class UTextBlock;
class UButton;
class USizeBox;
class UScrollBox;

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
    FString FontAsset;
    FName FontFace = TEXT("Regular");
    FName HeadingFace = TEXT("Bold");
    FString PanelImage;
    float OutlineSize = 0;
    float BorderWidth = 1;
    float CornerRadius = 3;
    FLinearColor BorderColor = FLinearColor(0.35f, 0.45f, 0.42f, 1);
    FLinearColor ButtonNormal = FLinearColor(0.11f, 0.16f, 0.19f, 1);
    FLinearColor ButtonHovered = FLinearColor(0.18f, 0.25f, 0.28f, 1);
    FLinearColor ButtonPressed = FLinearColor(0.07f, 0.10f, 0.12f, 1);
    FLinearColor ButtonDisabled = FLinearColor(0.08f, 0.08f, 0.08f, 1);
    float SlotPadding = 3;
    float IconWidth = 64;
    float IconHeight = 42;
    bool bAnimateScrolling = false;
    float ScrollSpeed = 15;

    static FKalmalaUITheme FromConfig(const FConfigFile& Config);
    static const FKalmalaUITheme& Get();
    int32 ScaledFontSize(int32 BaseSize, int32 TextScalePercent) const;
    FMargin PanelPadding() const { return FMargin(PaddingX, PaddingY); }
    void ApplyPanel(UBorder& Border, int32 ContrastMode) const;
    void ApplyButton(UButton& Button, int32 ContrastMode) const;
    void ApplyIconSlot(USizeBox& Slot) const;
    void ApplyScroll(UScrollBox& Scroll, bool bReducedMotion = false) const;
    void ApplyText(UTextBlock& Label, int32 BaseSize, bool bHeading,
        int32 TextScalePercent, int32 ContrastMode) const;
};
