#include "KalmalaSurvivalStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaSettingsWidget.h"
#include "Styling/CoreStyle.h"

namespace
{
    constexpr float ActiveStatusThresholdSeconds = 0.0f;
    constexpr float WeatherSignalThreshold = 0.05f;
    constexpr float ExposureSignalThreshold = 0.05f;

    FString FormatSeconds(const float Seconds)
    {
        return FString::Printf(TEXT("%d s"), FMath::Max(0, FMath::CeilToInt(Seconds)));
    }

    const TCHAR* StatusWidgetSupportEffectName(const EKalmalaSupportEffect Effect)
    {
        switch (Effect)
        {
        case EKalmalaSupportEffect::Mending: return TEXT("MENDING");
        case EKalmalaSupportEffect::HearthShield: return TEXT("HEARTH SHIELD");
        case EKalmalaSupportEffect::BearsVigor: return TEXT("BEAR'S VIGOR");
        case EKalmalaSupportEffect::DeerCall: return TEXT("DEER CALL");
        default: return nullptr;
        }
    }

    const FKalmalaPlayerStatusEntry* FindStatus(const TArray<FKalmalaPlayerStatusEntry>& Statuses, const FName StatusId)
    {
        return Statuses.FindByPredicate([StatusId](const FKalmalaPlayerStatusEntry& Entry)
        {
            return Entry.StatusId == StatusId && FMath::IsFinite(Entry.RemainingSeconds)
                && Entry.RemainingSeconds > ActiveStatusThresholdSeconds;
        });
    }
}

void UKalmalaSurvivalStatusWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);

    Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SurvivalStatusBackground"));
    Background->SetPadding(FMargin(12.0f, 9.0f));
    ContentWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SurvivalStatusWidth"));
    ContentWidth->SetWidthOverride(StatusPanelContentWidth);

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SurvivalStatusContent"));
    HeadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SurvivalStatusHeading"));
    HeadingText->SetText(FText::FromString(TEXT("SURVIVAL STATUS")));
    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SurvivalStatusRows"));
    StatusText->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(HeadingText);
    Content->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));
    ContentWidth->SetContent(Content);
    Background->SetContent(ContentWidth);
    WidgetTree->RootWidget = Background;
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaSurvivalStatusWidget::ConfigureViewportPlacement()
{
    // Keep the wrapped status column left of the centered arrival card and below the minimap/weather badge.
    // Set anchors last because UE 5.8 viewport setters reset the current anchor.
    SetPositionInViewport(FVector2D(24.0f, -24.0f), false);
    SetAlignmentInViewport(FVector2D(0.0f, 1.0f));
    SetAnchorsInViewport(FAnchors(0.0f, 1.0f));
}

FString UKalmalaSurvivalStatusWidget::BuildStatusText(const FKalmalaSurvivalStatusSnapshot& Snapshot)
{
    if (!Snapshot.bHasCharacter)
    {
        return TEXT("Waiting for player character.");
    }

    TArray<FString> Rows;
    const FKalmalaPlayerStatusEntry* Wet = FindStatus(Snapshot.Statuses, UKalmalaPlayerStatusComponent::WetStatusId);
    if (Wet != nullptr)
    {
        Rows.Add(FString::Printf(
            TEXT("○ STATUS · WET · %s · movement −8%% / stamina cost +15%% · Source: exposed rain or water · Recovery: warm up near a lit campfire."),
            *FormatSeconds(FMath::Clamp(Wet->RemainingSeconds, 0.0f, UKalmalaPlayerStatusComponent::WetMaximumSeconds))));
    }

    const FKalmalaPlayerStatusEntry* Meal = FindStatus(Snapshot.Statuses, UKalmalaPlayerStatusComponent::SteadyMealStatusId);
    if (Meal != nullptr)
    {
        Rows.Add(FString::Printf(
            TEXT("◆ FOOD · STEADY MEAL · %s · stamina cost −10%% · Source: prepared food · Recovery: wait for expiry; meals do not stack or replace."),
            *FormatSeconds(FMath::Clamp(Meal->RemainingSeconds, 0.0f, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds))));
    }

    const FKalmalaWeatherState& Weather = Snapshot.Weather;
    if (Snapshot.bHasWeatherState && Weather.IsValid()
        && (Weather.PrecipitationIntensity >= WeatherSignalThreshold
            || Weather.FogIntensity >= WeatherSignalThreshold
            || Weather.WindStrength >= WeatherSignalThreshold))
    {
        FString Counterplay;
        if (Weather.PrecipitationIntensity >= WeatherSignalThreshold)
        {
            Counterplay += TEXT("roof cover limits rain exposure; a lit hearth clears Wet");
        }
        if (Weather.WindStrength >= WeatherSignalThreshold)
        {
            if (!Counterplay.IsEmpty()) Counterplay += TEXT("; ");
            Counterplay += TEXT("a windbreak reduces exposed hearth fuel wetting");
        }
        if (Weather.FogIntensity >= WeatherSignalThreshold)
        {
            if (!Counterplay.IsEmpty()) Counterplay += TEXT("; ");
            Counterplay += TEXT("wait for the server weather to shift");
        }

        const float Elapsed = FMath::Max(0.0f, Snapshot.ServerTimeSeconds - Weather.ServerStartTimeSeconds);
        const float Remaining = FMath::Clamp(Weather.DurationSeconds - Elapsed, 0.0f, Weather.DurationSeconds);
        Rows.Add(FString::Printf(
            TEXT("▲ HAZARD · WEATHER · rain %d%% / fog %d%% / wind %d%% / storm %d%% · %s · Source: server weather · Recovery: %s."),
            FMath::RoundToInt(Weather.PrecipitationIntensity * 100.0f),
            FMath::RoundToInt(Weather.FogIntensity * 100.0f),
            FMath::RoundToInt(Weather.WindStrength * 100.0f),
            FMath::RoundToInt(Weather.StormIntensity * 100.0f),
            *FormatSeconds(Remaining),
            Counterplay.IsEmpty() ? TEXT("wait for the next weather interval") : *Counterplay));
    }

    const float SafeHeat = FMath::IsFinite(Snapshot.Exposure.HeatIntensity)
        ? FMath::Clamp(Snapshot.Exposure.HeatIntensity, 0.0f, 1.0f) : 0.0f;
    if (SafeHeat >= ExposureSignalThreshold)
    {
        Rows.Add(FString::Printf(
            TEXT("▲ HAZARD · HEAT SIGNAL · intensity %d%% · ongoing · Source: local ambient conditions · Guidance: seek shade or cooler cover."),
            FMath::RoundToInt(SafeHeat * 100.0f)));
    }

    const float SafeCold = FMath::IsFinite(Snapshot.Exposure.ColdIntensity)
        ? FMath::Clamp(Snapshot.Exposure.ColdIntensity, 0.0f, 1.0f) : 0.0f;
    const float SafeWarmth = FMath::IsFinite(Snapshot.Exposure.Warmth)
        ? FMath::Clamp(Snapshot.Exposure.Warmth, 0.0f, 100.0f) : 100.0f;
    if (SafeCold >= ExposureSignalThreshold && SafeWarmth < FKalmalaExposureResponse::ColdStaminaRecoveryWarmthThreshold)
    {
        const int32 RecoveryPercent = FMath::RoundToInt(FKalmalaExposureResponse::GetStaminaRecoveryMultiplier(SafeWarmth) * 100.0f);
        Rows.Add(FString::Printf(
            TEXT("◇ HAZARD · COLD · signal %d%% / warmth %d%% / stamina recovery %d%% · ongoing · Source: local ambient conditions · Recovery: shelter or a lit hearth restores warmth."),
            FMath::RoundToInt(SafeCold * 100.0f), FMath::RoundToInt(SafeWarmth), RecoveryPercent));
    }

    const TCHAR* ActiveEffectName = StatusWidgetSupportEffectName(Snapshot.ActiveSupportEffect);
    const float SupportRemaining = Snapshot.ActiveSupportEffectExpiry - Snapshot.ServerTimeSeconds;
    if (ActiveEffectName != nullptr && FMath::IsFinite(SupportRemaining) && SupportRemaining > ActiveStatusThresholdSeconds)
    {
        FString Magnitude;
        if (Snapshot.ActiveSupportEffect == EKalmalaSupportEffect::HearthShield && FMath::IsFinite(Snapshot.HearthShieldStrength))
        {
            Magnitude = FString::Printf(TEXT(" · absorption %d"), FMath::Max(0, FMath::RoundToInt(Snapshot.HearthShieldStrength)));
        }
        else if (Snapshot.ActiveSupportEffect == EKalmalaSupportEffect::BearsVigor
            && FMath::IsFinite(Snapshot.BearsVigorStrengthMultiplier) && Snapshot.BearsVigorStrengthMultiplier > 1.0f)
        {
            Magnitude = FString::Printf(TEXT(" · strength ×%.1f"), Snapshot.BearsVigorStrengthMultiplier);
        }
        Rows.Add(FString::Printf(
            TEXT("□ SUPPORT · %s · %s%s · Source: accepted support magic · Recovery: effect expires on server time."),
            ActiveEffectName, *FormatSeconds(SupportRemaining), *Magnitude));
    }

    return Rows.IsEmpty() ? TEXT("○ STATUS · NO ACTIVE EFFECTS") : FString::Join(Rows, TEXT("\n"));
}

void UKalmalaSurvivalStatusWidget::SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot,
    const int32 TextScalePercent, const int32 ContrastMode)
{
    if (StatusText == nullptr || HeadingText == nullptr || Background == nullptr) return;

    const FString NewStatusText = BuildStatusText(Snapshot);
    if (LastStatusText != NewStatusText)
    {
        StatusText->SetText(FText::FromString(NewStatusText));
        LastStatusText = NewStatusText;
    }
    ApplyAccessibilityPresentation(TextScalePercent, ContrastMode);
}

void UKalmalaSurvivalStatusWidget::ApplyAccessibilityPresentation(const int32 TextScalePercent, const int32 ContrastMode)
{
    const int32 BoundedTextScale = UKalmalaSettingsWidget::ClampTextScale(TextScalePercent);
    const int32 BoundedContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode);
    if (LastTextScalePercent == BoundedTextScale && LastContrastMode == BoundedContrast) return;

    const bool bHighContrast = BoundedContrast != 0;
    Background->SetBrushColor(bHighContrast
        ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
        : FLinearColor(0.025f, 0.035f, 0.04f, 0.94f));
    HeadingText->SetColorAndOpacity(FSlateColor(bHighContrast ? FLinearColor::White : FLinearColor(0.75f, 0.82f, 0.79f, 1.0f)));
    StatusText->SetColorAndOpacity(FSlateColor(bHighContrast ? FLinearColor::White : FLinearColor(0.93f, 0.96f, 0.94f, 1.0f)));
    HeadingText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::RoundToInt(10.0f * BoundedTextScale / 100.0f)));
    StatusText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::RoundToInt(13.0f * BoundedTextScale / 100.0f)));
    LastTextScalePercent = BoundedTextScale;
    LastContrastMode = BoundedContrast;
}
