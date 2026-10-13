#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"
#include "Styling/SlateBrush.h"
#include "Fonts/SlateFontInfo.h"

class FConfigFile;
class UBorder;
class UTextBlock;
class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class USizeBox;
class UScrollBox;
class UWidgetTree;

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
    FString InventoryPanelImage;
    FString BuildPanelImage;
    FString WorldMapPanelImage;
    FString ButtonImage;
    FString SliderThumbImage;
    FString OptionsDividerImage;
    FString EscapePanelImage;
    FString VideoOptionsPanelImage;
    FString AudioOptionsPanelImage;
    FString ControlsOptionsPanelImage;
    FString SettingsOptionsPanelImage;
    float OutlineSize = 0;
    float BorderWidth = 1;
    float CornerRadius = 3;
    FLinearColor BorderColor = FLinearColor(0.35f, 0.45f, 0.42f, 1);
    FLinearColor ButtonNormal = FLinearColor(0.11f, 0.16f, 0.19f, 1);
    FLinearColor ButtonHovered = FLinearColor(0.18f, 0.25f, 0.28f, 1);
    FLinearColor ButtonPressed = FLinearColor(0.07f, 0.10f, 0.12f, 1);
    FLinearColor ButtonDisabled = FLinearColor(0.08f, 0.08f, 0.08f, 1);
    FLinearColor ButtonFocused = FLinearColor(0.58f, 0.43f, 0.20f, 1);
    FLinearColor ButtonSelected = FLinearColor(0.18f, 0.23f, 0.17f, 1);
    FLinearColor ListViewRowNormal = FLinearColor(0.12f, 0.075f, 0.045f, 0.35f);
    FLinearColor ListViewRowHovered = FLinearColor(0.28f, 0.16f, 0.065f, 0.70f);
    FLinearColor ListViewRowSelected = FLinearColor(0.48f, 0.29f, 0.08f, 0.90f);
    FLinearColor ListViewRowDisabled = FLinearColor(0.10f, 0.08f, 0.06f, 0.55f);
    FLinearColor ListViewRowBorderColor = FLinearColor(0.45f, 0.27f, 0.12f, 1.0f);
    FLinearColor RequirementIconBorderColor = FLinearColor(0.45f, 0.27f, 0.12f, 1.0f);
    // CSS rgba(159, 86, 28, 0.5), converted from sRGB to the linear color space.
    FLinearColor ItemIconTileFill = FLinearColor(0.346704f, 0.093059f, 0.011612f, 0.5f);
    FLinearColor StatusCueStartedColor = FLinearColor(0.28f, 0.72f, 0.42f, 1);
    FLinearColor StatusCueRefreshedColor = FLinearColor(0.95f, 0.69f, 0.24f, 1);
    FLinearColor StatusCueEndedColor = FLinearColor(0.88f, 0.36f, 0.29f, 1);
    FName FavoriteMarkerStyle = TEXT("Both");
    float FavoriteMarkerBorderWidth = 2.0f;
    FLinearColor FavoriteMarkerColor = FLinearColor(0.98f, 0.78f, 0.20f, 1);
    FLinearColor RankGoldColor = FLinearColor(1.0f, 0.78f, 0.20f, 1);
    FLinearColor RankSilverColor = FLinearColor(0.78f, 0.84f, 0.90f, 1);
    FLinearColor RankBronzeColor = FLinearColor(0.82f, 0.51f, 0.31f, 1);
    FLinearColor RecentMarkerColor = FLinearColor(0.38f, 0.82f, 0.90f, 1);
    float StatusCueDuration = 1.25f;
    float FocusBorderWidth = 2.0f;
    float SelectedBorderWidth = 2.0f;
    float DisabledBorderWidth = 2.0f;
    float InteractionTransitionDuration = 0.12f;
    bool bAnimateInteractionStates = true;
    float SlotPadding = 3;
    float ButtonPadding = 9;
    float ListViewRowPadding = 5;
    float ListViewRowCornerRadius = 2;
    float ListViewRowBorderWidth = 0;
    float RequirementIconBorderWidth = 1;
    float ItemIconTileSize = 64;
    // Total gap between neighboring tiles; each layout applies half on either edge.
    float ItemIconTileMargin = 4;
    float IconWidth = 64;
    float IconHeight = 42;
    bool bAnimateScrolling = false;
    float ScrollSpeed = 15;
    float NotificationLifetime = 4;
    bool bAnimateOptionsOpening = true;
    float OptionsOpeningDuration = 0.18f;
    float OptionsOpeningTravel = 32.0f;
    FName OptionsOpeningEasing = TEXT("EaseOutCubic");

    static FKalmalaUITheme FromConfig(const FConfigFile& Config);
    static const FKalmalaUITheme& Get();
    int32 ScaledFontSize(int32 BaseSize, int32 TextScalePercent) const;
    FMargin PanelPadding() const { return FMargin(PaddingX, PaddingY); }
    FSlateBrush MakePanelBrush(int32 ContrastMode, const FString* ImageOverride = nullptr) const;
    FSlateBrush MakeSolidPanelBrush(int32 ContrastMode) const;
    FSlateFontInfo MakeFont(int32 BaseSize, bool bHeading, int32 TextScalePercent) const;
    FLinearColor TextColor(bool bHeading, int32 ContrastMode) const;
    void ApplyMenu(UWidgetTree& Tree, UTextBlock* HeadingLabel, int32 TextScalePercent, int32 ContrastMode) const;
    void ApplyPanel(UBorder& Border, int32 ContrastMode, const FString* ImageOverride = nullptr) const;
    void ApplySolidPanel(UBorder& Border, int32 ContrastMode) const;
    void ApplyButton(UButton& Button, int32 ContrastMode, bool bReducedMotion = false) const;
    /** Applies the reusable stone toggle-checkbox style, with a geometric high-contrast fallback. */
    void ApplyCheckBox(UCheckBox& CheckBox, int32 ContrastMode) const;
    /** Applies the shared stone dropdown and themed option-row styles. */
    void ApplyDropdown(UComboBoxString& Dropdown, int32 ContrastMode) const;
    void ApplyListViewItem(UButton& Button, int32 ContrastMode, bool bReducedMotion = false) const;
    void ApplyItemIconTile(UBorder& Border, int32 ContrastMode) const;
    void ApplyItemIconTileFill(UBorder& Border, int32 ContrastMode) const;
    void ApplyRequirementIconFrame(UBorder& Border, int32 ContrastMode) const;
    void SizeButtonToContent(UButton& Button) const;
    void ApplySlider(USlider& Slider, int32 ContrastMode) const;
    void ApplySelectablePanel(UBorder& Border, bool bSelected, bool bFocused, bool bUnavailable,
        int32 ContrastMode, bool bReducedMotion = false, bool bAnimateFill = true) const;
    void ApplyIconSlot(USizeBox& Slot) const;
    void ApplyScroll(UScrollBox& Scroll, bool bReducedMotion = false) const;
    bool ShouldAnimateOptionsOpening() const;
    float OptionsOpeningOffset(float Progress) const;
    void ApplyText(UTextBlock& Label, int32 BaseSize, bool bHeading,
        int32 TextScalePercent, int32 ContrastMode) const;
    bool UsesFavoriteMarkerStar() const
    {
        return FavoriteMarkerStyle == TEXT("Star") || FavoriteMarkerStyle == TEXT("Both");
    }
    bool UsesFavoriteMarkerBorder() const
    {
        return FavoriteMarkerStyle == TEXT("Border") || FavoriteMarkerStyle == TEXT("Both");
    }
    FLinearColor RankMarkerColor(int32 Rank, int32 ContrastMode) const;
};
