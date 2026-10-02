#include "KalmalaUITheme.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "KalmalaSettingsWidget.h"
#include "Misc/ConfigCacheIni.h"
#include "Styling/CoreStyle.h"

namespace
{
    const TCHAR* ThemeSection = TEXT("Kalmala.UI.Theme");

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
    Border.SetBrushColor(UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0 ? HighContrastPanel : Panel);
}

void FKalmalaUITheme::ApplyText(UTextBlock& Label, const int32 BaseSize, const bool bHeading,
    const int32 TextScalePercent, const int32 ContrastMode) const
{
    Label.SetColorAndOpacity(FSlateColor(UKalmalaSettingsWidget::ClampContrastMode(ContrastMode) != 0
        ? FLinearColor::White : (bHeading ? Heading : Text)));
    Label.SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), ScaledFontSize(BaseSize, TextScalePercent)));
}
