#include "KalmalaUITheme.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Containers/Ticker.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "Blueprint/WidgetTree.h"

namespace
{
    const TCHAR* ThemeSection = TEXT("Kalmala.UI.Theme");
    TMap<TWeakObjectPtr<UBorder>, uint64> SelectableFillTransitionIds;
    uint64 NextSelectableFillTransitionId = 1;

    void TransitionBorderFill(UBorder& Border, const FLinearColor& Target, const float Duration, const bool bAnimate)
    {
        const TWeakObjectPtr<UBorder> WeakBorder(&Border);
        const FLinearColor Start = Border.GetBrushColor();
        if (!bAnimate || Duration <= 0.0f)
        {
            SelectableFillTransitionIds.Remove(WeakBorder);
            Border.SetBrushColor(Target);
            return;
        }

        const uint64 TransitionId = NextSelectableFillTransitionId++;
        SelectableFillTransitionIds.Add(WeakBorder, TransitionId);
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
            [WeakBorder, Start, Target, Duration, TransitionId, Elapsed = 0.0f](const float DeltaSeconds) mutable
            {
                UBorder* CurrentBorder = WeakBorder.Get();
                const uint64* CurrentId = SelectableFillTransitionIds.Find(WeakBorder);
                if (CurrentBorder == nullptr)
                {
                    if (CurrentId != nullptr && *CurrentId == TransitionId)
                    {
                        SelectableFillTransitionIds.Remove(WeakBorder);
                    }
                    return false;
                }
                if (CurrentId == nullptr || *CurrentId != TransitionId)
                {
                    return false;
                }
                if (UKalmalaSettingsWidget::IsReducedMotionEnabled())
                {
                    CurrentBorder->SetBrushColor(Target);
                    SelectableFillTransitionIds.Remove(WeakBorder);
                    return false;
                }
                Elapsed += FMath::Max(0.0f, DeltaSeconds);
                const float Progress = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
                const float Eased = Progress * Progress * (3.0f - 2.0f * Progress);
                CurrentBorder->SetBrushColor(FMath::Lerp(Start, Target, Eased));
                if (Progress < 1.0f) return true;
                CurrentBorder->SetBrushColor(Target);
                SelectableFillTransitionIds.Remove(WeakBorder);
                return false;
            }), 0.0f);
    }

    UObject* LoadThemeAsset(const FString& Path)
    {
        if (!FPackageName::IsValidObjectPath(Path)) return nullptr;
        if (UObject* Loaded = FindObject<UObject>(nullptr, *Path)) return Loaded;
        if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path))) return nullptr;
        return LoadObject<UObject>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
    }

    void ReadColor(const FConfigFile& Config, const TCHAR* Key, FLinearColor& Value)
    {
        FString Raw;
        FLinearColor Candidate;
        if (!Config.GetString(ThemeSection, Key, Raw) || !Candidate.InitFromString(Raw)) return;
        if (!FMath::IsFinite(Candidate.R) || !FMath::IsFinite(Candidate.G)
            || !FMath::IsFinite(Candidate.B) || !FMath::IsFinite(Candidate.A)) return;
        if (Candidate.R < 0 || Candidate.R > 1 || Candidate.G < 0 || Candidate.G > 1
            || Candidate.B < 0 || Candidate.B > 1 || Candidate.A < 0 || Candidate.A > 1) return;
        Value = Candidate;
    }

    void ReadNumber(const FConfigFile& Config, const TCHAR* Key, float& Value, float Min, float Max)
    {
        FString Raw;
        double Candidate = 0;
        if (Config.GetString(ThemeSection, Key, Raw) && LexTryParseString(Candidate, *Raw)
            && FMath::IsFinite(Candidate) && Candidate >= Min && Candidate <= Max)
        {
            Value = static_cast<float>(Candidate);
        }
    }
}

FKalmalaUITheme FKalmalaUITheme::FromConfig(const FConfigFile& Config)
{
    FKalmalaUITheme Theme;
    ReadColor(Config, TEXT("Panel"), Theme.Panel);
    ReadColor(Config, TEXT("Heading"), Theme.Heading);
    ReadColor(Config, TEXT("Text"), Theme.Text);
    ReadColor(Config, TEXT("HighContrastPanel"), Theme.HighContrastPanel);
    // Contrast mode must retain an opaque, black-backed white-text surface.
    Theme.HighContrastPanel = FLinearColor(0, 0, 0, FMath::Max(0.98f, Theme.HighContrastPanel.A));
    float HeadingSize = Theme.HeadingSize, BodySize = Theme.BodySize, EmphasisSize = Theme.EmphasisSize;
    ReadNumber(Config, TEXT("HeadingSize"), HeadingSize, 8, 32);
    ReadNumber(Config, TEXT("BodySize"), BodySize, 8, 32);
    ReadNumber(Config, TEXT("EmphasisSize"), EmphasisSize, 8, 32);
    Theme.HeadingSize = FMath::RoundToInt(HeadingSize);
    Theme.BodySize = FMath::RoundToInt(BodySize);
    Theme.EmphasisSize = FMath::RoundToInt(EmphasisSize);
    ReadNumber(Config, TEXT("PaddingX"), Theme.PaddingX, 0, 24);
    ReadNumber(Config, TEXT("PaddingY"), Theme.PaddingY, 0, 24);
    ReadNumber(Config, TEXT("RowSpacing"), Theme.RowSpacing, 0, 16);
    ReadNumber(Config, TEXT("OutlineSize"), Theme.OutlineSize, 0, 3);
    ReadNumber(Config, TEXT("BorderWidth"), Theme.BorderWidth, 0, 3);
    ReadNumber(Config, TEXT("CornerRadius"), Theme.CornerRadius, 0, 12);
    ReadNumber(Config, TEXT("SlotPadding"), Theme.SlotPadding, 0, 16);
    ReadNumber(Config, TEXT("IconWidth"), Theme.IconWidth, 24, 96);
    ReadNumber(Config, TEXT("IconHeight"), Theme.IconHeight, 24, 96);
    ReadNumber(Config, TEXT("ScrollSpeed"), Theme.ScrollSpeed, 1, 60);
    ReadNumber(Config, TEXT("NotificationLifetime"), Theme.NotificationLifetime, 1, 10);
    ReadNumber(Config, TEXT("StatusCueDuration"), Theme.StatusCueDuration, 0.4f, 3.0f);
    ReadNumber(Config, TEXT("OptionsOpeningDuration"), Theme.OptionsOpeningDuration, 0, 0.8f);
    ReadNumber(Config, TEXT("OptionsOpeningTravel"), Theme.OptionsOpeningTravel, 0, 96);
    ReadColor(Config, TEXT("BorderColor"), Theme.BorderColor);
    ReadColor(Config, TEXT("ButtonNormal"), Theme.ButtonNormal);
    ReadColor(Config, TEXT("ButtonHovered"), Theme.ButtonHovered);
    ReadColor(Config, TEXT("ButtonPressed"), Theme.ButtonPressed);
    ReadColor(Config, TEXT("ButtonDisabled"), Theme.ButtonDisabled);
    ReadColor(Config, TEXT("ButtonFocused"), Theme.ButtonFocused);
    ReadColor(Config, TEXT("ButtonSelected"), Theme.ButtonSelected);
    ReadColor(Config, TEXT("StatusCueStartedColor"), Theme.StatusCueStartedColor);
    ReadColor(Config, TEXT("StatusCueRefreshedColor"), Theme.StatusCueRefreshedColor);
    ReadColor(Config, TEXT("StatusCueEndedColor"), Theme.StatusCueEndedColor);
    ReadColor(Config, TEXT("FavoriteMarkerColor"), Theme.FavoriteMarkerColor);
    ReadColor(Config, TEXT("RankGoldColor"), Theme.RankGoldColor);
    ReadColor(Config, TEXT("RankSilverColor"), Theme.RankSilverColor);
    ReadColor(Config, TEXT("RankBronzeColor"), Theme.RankBronzeColor);
    ReadColor(Config, TEXT("RecentMarkerColor"), Theme.RecentMarkerColor);
    ReadNumber(Config, TEXT("FavoriteMarkerBorderWidth"), Theme.FavoriteMarkerBorderWidth, 1, 4);
    FString FavoriteStyle;
    if (Config.GetString(ThemeSection, TEXT("FavoriteMarkerStyle"), FavoriteStyle)
        && (FavoriteStyle == TEXT("Star") || FavoriteStyle == TEXT("Border") || FavoriteStyle == TEXT("Both")))
    {
        Theme.FavoriteMarkerStyle = FName(*FavoriteStyle);
    }
    ReadNumber(Config, TEXT("FocusBorderWidth"), Theme.FocusBorderWidth, 1, 5);
    ReadNumber(Config, TEXT("SelectedBorderWidth"), Theme.SelectedBorderWidth, 1, 5);
    ReadNumber(Config, TEXT("DisabledBorderWidth"), Theme.DisabledBorderWidth, 1, 5);
    ReadNumber(Config, TEXT("InteractionTransitionDuration"), Theme.InteractionTransitionDuration, 0, 0.5f);
    Config.GetBool(ThemeSection, TEXT("AnimateScrolling"), Theme.bAnimateScrolling);
    Config.GetBool(ThemeSection, TEXT("AnimateInteractionStates"), Theme.bAnimateInteractionStates);
    Config.GetBool(ThemeSection, TEXT("AnimateOptionsOpening"), Theme.bAnimateOptionsOpening);
    FString OptionsOpeningEasing;
    if (Config.GetString(ThemeSection, TEXT("OptionsOpeningEasing"), OptionsOpeningEasing)
        && (OptionsOpeningEasing == TEXT("EaseOutCubic") || OptionsOpeningEasing == TEXT("EaseOutQuad")
            || OptionsOpeningEasing == TEXT("Linear")))
    {
        Theme.OptionsOpeningEasing = FName(*OptionsOpeningEasing);
    }
    const auto ReadAsset = [&Config](const TCHAR* Key, FString& Value)
    {
        FString Path;
        if (!Config.GetString(ThemeSection, Key, Path) || !Path.StartsWith(TEXT("/Game/")) || Path.Len() >= 180)
        {
            return;
        }
        FString ClassName;
        FString PackageName;
        FString ObjectName;
        FString SubObjectName;
        FPackageName::SplitFullObjectPath(Path, ClassName, PackageName, ObjectName, SubObjectName, false);
        if (!ObjectName.IsEmpty() && SubObjectName.IsEmpty()
            && FPackageName::IsValidLongPackageName(PackageName)
            && FPackageName::IsValidObjectPath(Path))
        {
            Value = Path;
        }
    };
    ReadAsset(TEXT("FontAsset"), Theme.FontAsset);
    ReadAsset(TEXT("PanelImage"), Theme.PanelImage);
    ReadAsset(TEXT("InventoryPanelImage"), Theme.InventoryPanelImage);
    ReadAsset(TEXT("BuildPanelImage"), Theme.BuildPanelImage);
    ReadAsset(TEXT("WorldMapPanelImage"), Theme.WorldMapPanelImage);
    ReadAsset(TEXT("EscapePanelImage"), Theme.EscapePanelImage);
    ReadAsset(TEXT("VideoOptionsPanelImage"), Theme.VideoOptionsPanelImage);
    ReadAsset(TEXT("AudioOptionsPanelImage"), Theme.AudioOptionsPanelImage);
    ReadAsset(TEXT("ControlsOptionsPanelImage"), Theme.ControlsOptionsPanelImage);
    ReadAsset(TEXT("SettingsOptionsPanelImage"), Theme.SettingsOptionsPanelImage);
    FString Face;
    if (Config.GetString(ThemeSection, TEXT("FontFace"), Face)
        && (Face == TEXT("Regular") || Face == TEXT("Bold"))) Theme.FontFace = FName(*Face);
    if (Config.GetString(ThemeSection, TEXT("HeadingFace"), Face)
        && (Face == TEXT("Regular") || Face == TEXT("Bold"))) Theme.HeadingFace = FName(*Face);
    return Theme;
}

const FKalmalaUITheme& FKalmalaUITheme::Get()
{
    static const FKalmalaUITheme Theme = []
    {
        FConfigFile Config;
        FConfigCacheIni::LoadLocalIniFile(Config, TEXT("KalmalaTheme"), true);
        return FromConfig(Config);
    }();
    return Theme;
}

int32 FKalmalaUITheme::ScaledFontSize(const int32 BaseSize, const int32 TextScalePercent) const
{
    return FMath::RoundToInt(FMath::Clamp(BaseSize, 8, 32)
        * UKalmalaSettingsWidget::ClampTextScale(TextScalePercent) / 100.0f);
}

FLinearColor FKalmalaUITheme::RankMarkerColor(const int32 Rank, const int32 ContrastMode) const
{
    if (ContrastMode != 0) return FLinearColor::White;
    switch (Rank)
    {
    case 1: return RankGoldColor;
    case 2: return RankSilverColor;
    case 3: return RankBronzeColor;
    default: return Text;
    }
}

void FKalmalaUITheme::ApplyPanel(UBorder& Border, const int32 ContrastMode, const FString* ImageOverride) const
{
    Border.SetPadding(PanelPadding());
    Border.SetBrushColor(FLinearColor::White);
    Border.SetBrush(MakePanelBrush(ContrastMode, ImageOverride));
}

FSlateBrush FKalmalaUITheme::MakePanelBrush(const int32 ContrastMode, const FString* ImageOverride) const
{
    const bool bContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0;
    FSlateBrush Brush = FSlateRoundedBoxBrush(bContrast ? HighContrastPanel : Panel, CornerRadius,
        bContrast ? FLinearColor::White : BorderColor, bContrast ? FMath::Max(1.0f, BorderWidth) : BorderWidth);
    const FString& SelectedImage = ImageOverride ? *ImageOverride : PanelImage;
    if (!bContrast && !SelectedImage.IsEmpty())
    {
        if (UTexture2D* Image = Cast<UTexture2D>(LoadThemeAsset(SelectedImage)))
        {
            Brush.SetResourceObject(Image);
            Brush.DrawAs = ESlateBrushDrawType::Image;
        }
    }
    return Brush;
}

FSlateBrush FKalmalaUITheme::MakeSolidPanelBrush(const int32 ContrastMode) const
{
    const bool bContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0;
    FLinearColor Fill = bContrast ? HighContrastPanel : Panel;
    Fill.A = 1.0f;
    return FSlateRoundedBoxBrush(Fill, CornerRadius,
        bContrast ? FLinearColor::White : BorderColor,
        bContrast ? FMath::Max(1.0f, BorderWidth) : BorderWidth);
}

void FKalmalaUITheme::ApplySolidPanel(UBorder& Border, const int32 ContrastMode) const
{
    Border.SetPadding(PanelPadding());
    Border.SetBrush(MakeSolidPanelBrush(ContrastMode));
    Border.SetBrushColor(FLinearColor::White);
}

void FKalmalaUITheme::ApplyText(UTextBlock& Label, const int32 BaseSize, const bool bHeading,
    const int32 TextScalePercent, const int32 ContrastMode) const
{
    Label.SetColorAndOpacity(FSlateColor(TextColor(bHeading, ContrastMode)));
    Label.SetFont(MakeFont(BaseSize, bHeading, TextScalePercent));
}

FLinearColor FKalmalaUITheme::TextColor(const bool bHeading, const int32 ContrastMode) const
{
    return UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0
        ? FLinearColor::White : (bHeading ? Heading : Text);
}

FSlateFontInfo FKalmalaUITheme::MakeFont(const int32 BaseSize, const bool bHeading, const int32 TextScalePercent) const
{
    UFont* FontAssetObject = FontAsset.IsEmpty() ? nullptr : Cast<UFont>(LoadThemeAsset(FontAsset));
    // Offline bitmap fonts do not provide a composite face for shared Slate text.
    if (FontAssetObject && !FontAssetObject->GetCompositeFont()) FontAssetObject = nullptr;
    FSlateFontInfo Font = FontAssetObject ? FSlateFontInfo(FontAssetObject, ScaledFontSize(BaseSize, TextScalePercent))
        : FSlateFontInfo(FCoreStyle::GetDefaultFont(), ScaledFontSize(BaseSize, TextScalePercent));
    Font.TypefaceFontName = bHeading ? HeadingFace : FontFace;
    if (FontAssetObject && !FontAssetObject->GetCompositeFont()->DefaultTypeface.Fonts.ContainsByPredicate(
        [&Font](const FTypefaceEntry& Entry) { return Entry.Name == Font.TypefaceFontName; }))
    {
        Font.TypefaceFontName = NAME_None;
    }
    Font.OutlineSettings.OutlineSize = FMath::RoundToInt(OutlineSize);
    Font.OutlineSettings.OutlineColor = FLinearColor::Black;
    return Font;
}

void FKalmalaUITheme::ApplyButton(UButton& Button, const int32 ContrastMode, const bool bReducedMotion) const
{
    FButtonStyle Style = Button.GetStyle();
    const bool bContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0;
    const bool bSuppressMotion = bReducedMotion || UKalmalaSettingsWidget::IsReducedMotionEnabled();
    const auto Brush = [this, bContrast](FLinearColor Color, float ContrastShade)
    {
        return FSlateRoundedBoxBrush(bContrast ? FLinearColor(ContrastShade, ContrastShade, ContrastShade, 1) : Color,
            CornerRadius, bContrast ? FLinearColor::White : BorderColor, bContrast ? FMath::Max(1.0f, BorderWidth) : BorderWidth);
    };
    Style.SetNormal(Brush(ButtonNormal, 0.08f)).SetHovered(Brush(ButtonHovered, 0.20f))
        .SetPressed(Brush(ButtonPressed, 0.0f)).SetDisabled(Brush(ButtonDisabled, 0.04f));
    const FLinearColor FocusOutline = bContrast ? FLinearColor::White : ButtonFocused;
    Style.Normal.OutlineSettings.Color = FSlateColor(bContrast ? FLinearColor::White : BorderColor);
    Style.Hovered.OutlineSettings.Color = FSlateColor(FocusOutline);
    Style.Pressed.OutlineSettings.Color = FSlateColor(FocusOutline);
    Style.Disabled.OutlineSettings.Color = FSlateColor(bContrast ? FLinearColor::White : BorderColor);
    Style.Disabled.OutlineSettings.Width = FMath::Max(DisabledBorderWidth, BorderWidth);
    Style.SetNormalForeground(FSlateColor(bContrast ? FLinearColor::White : Text));
    Style.SetHoveredForeground(FSlateColor(bContrast ? FLinearColor::White : Text));
    Style.SetPressedForeground(FSlateColor(bContrast ? FLinearColor::White : Text));
    Style.SetDisabledForeground(FSlateColor(bContrast ? FLinearColor(0.72f, 0.72f, 0.72f, 1)
        : FLinearColor(0.55f, 0.58f, 0.57f, 1)));
    Style.SetNormalPadding(FMargin(SlotPadding)).SetPressedPadding(FMargin(SlotPadding + 1));
    if (UKalmalaThemedButton* ThemedButton = Cast<UKalmalaThemedButton>(&Button))
    {
        ThemedButton->SetThemeStyle(Style, FocusOutline,
            bContrast ? FLinearColor(0.12f, 0.12f, 0.12f, 1.0f) : ButtonSelected,
            bContrast ? FMath::Max(2.0f, FocusBorderWidth) : FocusBorderWidth,
            bContrast ? FMath::Max(2.0f, SelectedBorderWidth) : SelectedBorderWidth,
            bContrast ? FMath::Max(DisabledBorderWidth, 1.0f) : DisabledBorderWidth,
            InteractionTransitionDuration, bAnimateInteractionStates && !bSuppressMotion);
    }
    else
    {
        Button.SetStyle(Style);
    }
    Button.SetBackgroundColor(FLinearColor::White);
    Button.SetColorAndOpacity(FLinearColor::White);
}

void FKalmalaUITheme::ApplySelectablePanel(UBorder& Border, const bool bSelected, const bool bFocused,
    const bool bUnavailable, const int32 ContrastMode, const bool bReducedMotion) const
{
    const bool bContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0;
    const FLinearColor Fill = bContrast ? HighContrastPanel
        : bUnavailable ? ButtonDisabled : bSelected ? ButtonSelected : ButtonNormal;
    const FLinearColor Outline = bContrast ? FLinearColor::White
        : bFocused ? ButtonFocused : BorderColor;
    const float OutlineWidth = bContrast
        ? FMath::Max(1.0f, bFocused ? FocusBorderWidth : bSelected ? SelectedBorderWidth
            : bUnavailable ? DisabledBorderWidth : BorderWidth)
        : bFocused ? FocusBorderWidth : bSelected ? SelectedBorderWidth
            : bUnavailable ? DisabledBorderWidth : BorderWidth;
    const FLinearColor StartFill = Border.GetBrushColor();
    Border.SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, CornerRadius, Outline, OutlineWidth));
    Border.SetPadding(FMargin(SlotPadding + ((bSelected || bFocused) ? 1.0f : 0.0f)));
    // Selection and focus labels remain the non-colour cue; the fill transition is decorative.
    if (!bAnimateInteractionStates || bReducedMotion || UKalmalaSettingsWidget::IsReducedMotionEnabled()
        || InteractionTransitionDuration <= 0.0f)
    {
        TransitionBorderFill(Border, Fill, 0.0f, false);
    }
    else
    {
        Border.SetBrushColor(StartFill);
        TransitionBorderFill(Border, Fill, InteractionTransitionDuration, true);
    }
}

void FKalmalaUITheme::ApplyIconSlot(USizeBox& Slot) const
{
    Slot.SetWidthOverride(IconWidth);
    Slot.SetHeightOverride(IconHeight);
}

void FKalmalaUITheme::ApplyScroll(UScrollBox& Scroll, const bool bReducedMotion) const
{
    Scroll.SetAnimateWheelScrolling(bAnimateScrolling && !bReducedMotion
        && !UKalmalaSettingsWidget::IsReducedMotionEnabled());
    Scroll.SetScrollAnimationInterpolationSpeed(ScrollSpeed);
}

bool FKalmalaUITheme::ShouldAnimateOptionsOpening() const
{
    return bAnimateOptionsOpening && !UKalmalaSettingsWidget::IsReducedMotionEnabled()
        && OptionsOpeningDuration > 0.0f && OptionsOpeningTravel > 0.0f;
}

float FKalmalaUITheme::OptionsOpeningOffset(const float Progress) const
{
    if (!ShouldAnimateOptionsOpening()) return 0.0f;
    const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
    float EasedProgress = ClampedProgress;
    if (OptionsOpeningEasing == TEXT("EaseOutCubic"))
    {
        const float Remaining = 1.0f - ClampedProgress;
        EasedProgress = 1.0f - Remaining * Remaining * Remaining;
    }
    else if (OptionsOpeningEasing == TEXT("EaseOutQuad"))
    {
        EasedProgress = ClampedProgress * (2.0f - ClampedProgress);
    }
    return -OptionsOpeningTravel * (1.0f - EasedProgress);
}

void FKalmalaUITheme::ApplyMenu(UWidgetTree& Tree, UTextBlock* HeadingLabel,
    const int32 TextScalePercent, const int32 ContrastMode) const
{
    Tree.ForEachWidget([&](UWidget* Widget)
    {
        if (auto* Label = Cast<UTextBlock>(Widget))
            ApplyText(*Label, Label == HeadingLabel ? EmphasisSize + 11 : BodySize + 5,
                Label == HeadingLabel, TextScalePercent, ContrastMode);
        else if (auto* Border = Cast<UBorder>(Widget)) ApplyPanel(*Border, ContrastMode);
        else if (auto* Button = Cast<UButton>(Widget)) ApplyButton(*Button, ContrastMode);
        else if (auto* Scroll = Cast<UScrollBox>(Widget)) ApplyScroll(*Scroll);
    });
}
