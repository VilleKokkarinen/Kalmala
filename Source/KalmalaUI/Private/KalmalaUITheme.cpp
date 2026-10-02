#include "KalmalaUITheme.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "KalmalaSettingsWidget.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"
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
    ReadColor(Config, TEXT("BorderColor"), Theme.BorderColor);
    ReadColor(Config, TEXT("ButtonNormal"), Theme.ButtonNormal);
    ReadColor(Config, TEXT("ButtonHovered"), Theme.ButtonHovered);
    ReadColor(Config, TEXT("ButtonPressed"), Theme.ButtonPressed);
    ReadColor(Config, TEXT("ButtonDisabled"), Theme.ButtonDisabled);
    Config.GetBool(ThemeSection, TEXT("AnimateScrolling"), Theme.bAnimateScrolling);
    const auto ReadAsset = [&Config](const TCHAR* Key, FString& Value)
    {
        FString Path;
        if (Config.GetString(ThemeSection, Key, Path) && Path.StartsWith(TEXT("/Game/"))
            && Path.Len() < 180 && FPackageName::IsValidObjectPath(Path)) Value = Path;
    };
    ReadAsset(TEXT("FontAsset"), Theme.FontAsset);
    ReadAsset(TEXT("PanelImage"), Theme.PanelImage);
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

void FKalmalaUITheme::ApplyPanel(UBorder& Border, const int32 ContrastMode) const
{
    Border.SetPadding(PanelPadding());
    Border.SetBrushColor(FLinearColor::White);
    Border.SetBrush(MakePanelBrush(ContrastMode));
}

FSlateBrush FKalmalaUITheme::MakePanelBrush(const int32 ContrastMode) const
{
    const bool bContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0;
    FSlateBrush Brush = FSlateRoundedBoxBrush(bContrast ? HighContrastPanel : Panel, CornerRadius,
        bContrast ? FLinearColor::White : BorderColor, bContrast ? FMath::Max(1.0f, BorderWidth) : BorderWidth);
    if (!bContrast && !PanelImage.IsEmpty())
    {
        if (UTexture2D* Image = Cast<UTexture2D>(LoadThemeAsset(PanelImage)))
        {
            Brush.SetResourceObject(Image);
            Brush.DrawAs = ESlateBrushDrawType::Image;
        }
    }
    return Brush;
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

void FKalmalaUITheme::ApplyButton(UButton& Button, const int32 ContrastMode) const
{
    FButtonStyle Style = Button.GetStyle();
    const bool bContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0;
    const auto Brush = [this, bContrast](FLinearColor Color, float ContrastShade)
    {
        return FSlateRoundedBoxBrush(bContrast ? FLinearColor(ContrastShade, ContrastShade, ContrastShade, 1) : Color,
            CornerRadius, bContrast ? FLinearColor::White : BorderColor, bContrast ? FMath::Max(1.0f, BorderWidth) : BorderWidth);
    };
    Style.SetNormal(Brush(ButtonNormal, 0.08f)).SetHovered(Brush(ButtonHovered, 0.20f))
        .SetPressed(Brush(ButtonPressed, 0.0f)).SetDisabled(Brush(ButtonDisabled, 0.04f));
    Style.SetNormalPadding(FMargin(SlotPadding)).SetPressedPadding(FMargin(SlotPadding + 1));
    Button.SetStyle(Style);
    Button.SetBackgroundColor(FLinearColor::White);
    Button.SetColorAndOpacity(FLinearColor::White);
}

void FKalmalaUITheme::ApplyIconSlot(USizeBox& Slot) const
{
    Slot.SetWidthOverride(IconWidth);
    Slot.SetHeightOverride(IconHeight);
}

void FKalmalaUITheme::ApplyScroll(UScrollBox& Scroll, const bool bReducedMotion) const
{
    Scroll.SetAnimateWheelScrolling(bAnimateScrolling && !bReducedMotion);
    Scroll.SetScrollAnimationInterpolationSpeed(ScrollSpeed);
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
