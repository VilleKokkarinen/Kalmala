#include "KalmalaSurvivalStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"
#include "KalmalaWeatherActivityWidget.h"

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
    Background->SetPadding(FKalmalaUITheme::Get().PanelPadding());
    ContentWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SurvivalStatusWidth"));
    ContentWidth->SetWidthOverride(StatusPanelContentWidth);

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SurvivalStatusContent"));
    HeadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SurvivalStatusHeading"));
    HeadingText->SetText(FText::FromString(TEXT("TRAVEL")));
    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SurvivalStatusRows"));
    StatusText->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(HeadingText);
    Content->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, FKalmalaUITheme::Get().RowSpacing, 0.0f, 0.0f));
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
    if (Snapshot.bHasWeatherState && Weather.IsValid())
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

        Rows.Add(FString::Printf(
            TEXT("▲ HAZARD · WEATHER · rain %d%% / fog %d%% / wind %d%% / storm %d%% · Source: server weather · Recovery: %s."),
            FMath::RoundToInt(Weather.PrecipitationIntensity * 100.0f),
            FMath::RoundToInt(Weather.FogIntensity * 100.0f),
            FMath::RoundToInt(Weather.WindStrength * 100.0f),
            FMath::RoundToInt(Weather.StormIntensity * 100.0f),
            Counterplay.IsEmpty() ? TEXT("wait for the next weather interval") : *Counterplay));
        Rows.Add(UKalmalaWeatherActivityWidget::BuildActivityLabel(Weather.ActivityLevel));
    }

    const float SafeHeat = FMath::IsFinite(Snapshot.Exposure.HeatIntensity)
        ? FMath::Clamp(Snapshot.Exposure.HeatIntensity, 0.0f, 1.0f) : 0.0f;
    if (SafeHeat >= ExposureSignalThreshold)
    {
        Rows.Add(FString::Printf(
            TEXT("▲ HAZARD · HEAT SIGNAL · intensity %d%% · Source: local ambient conditions · Guidance: seek shade or cooler cover."),
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
            TEXT("◇ HAZARD · COLD · signal %d%% / warmth %d%% / stamina recovery %d%% · Source: local ambient conditions · Recovery: shelter or a lit hearth restores warmth."),
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

    const FString TravelText = BuildOceanTravelText(Snapshot);
    if (!TravelText.IsEmpty()) Rows.Add(TravelText);

    return Rows.IsEmpty() ? TEXT("○ STATUS · NO ACTIVE EFFECTS") : FString::Join(Rows, TEXT("\n"));
}

FString UKalmalaSurvivalStatusWidget::BuildOceanTravelText(const FKalmalaSurvivalStatusSnapshot& Snapshot)
{
    TArray<FString> Lines;
    if (Snapshot.bShowOceanTravelFeedback)
    {
        const FString Feedback = UKalmalaOceanTravelFeedbackComponent::GetFeedbackText(Snapshot.OceanTravelFeedback);
        if (!Feedback.IsEmpty()) Lines.Add(Feedback);
    }

    if (Snapshot.bInOceanSkiff)
    {
        Lines.Add(Snapshot.bAtOceanSkiffHelm
            ? TEXT("⚓ TRAVEL · HELM · W/S throttle; A/D steer. Hull needs ≥1 m of open ocean at all nine samples; generated land and the world edge block movement. Disembark stopped at ≤0.5 m/s with controls neutral.")
            : TEXT("⚓ TRAVEL · PASSENGER · The helm steers. Hull needs ≥1 m of open ocean at all nine samples; generated land and the world edge block movement. Disembark stopped at ≤0.5 m/s."));

        if (Snapshot.bHasWeatherState && Snapshot.Weather.IsValid() && Snapshot.Weather.WindStrength >= 0.15f)
        {
            Lines.Add(TEXT("▲ TRAVEL · CROSSWIND · Server wind can turn the underway hull; steer against the drift. Pressure eases as the wind subsides."));
        }

        if (Snapshot.OceanSkiffMode == EKalmalaOceanSkiffMode::Blocked)
        {
            switch (Snapshot.OceanSkiffBlockReason)
            {
            case EKalmalaOceanSkiffBlockReason::InvalidOceanFootprint:
	                Lines.Add(TEXT("▲ TRAVEL · HULL BLOCKED · Shallow water, inland water, or the finite-world edge stopped the hull. Steer back into deep open ocean."));
                break;
            case EKalmalaOceanSkiffBlockReason::GeneratedTerrainCollision:
                Lines.Add(TEXT("▲ TRAVEL · COLLISION BLOCKED · Generated terrain stopped the skiff. Turn away from the coast and steer through open water."));
                break;
            case EKalmalaOceanSkiffBlockReason::SweepLimit:
                Lines.Add(TEXT("▲ TRAVEL · MOVEMENT BLOCKED · The server could not safely sweep this step. Release controls and steer away at low speed."));
                break;
            default:
                Lines.Add(TEXT("▲ TRAVEL · MOVEMENT BLOCKED · The server stopped at the last safe position. Steer away from the obstruction."));
                break;
            }
        }
        else if (Snapshot.OceanSkiffMode == EKalmalaOceanSkiffMode::Underway)
        {
            Lines.Add(TEXT("◇ TRAVEL · UNDERWAY · The server checks deep water and generated-terrain collision; release controls to slow down."));
        }
    }

    return FString::Join(Lines, TEXT("\n"));
}

void UKalmalaSurvivalStatusWidget::SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot,
    const int32 TextScalePercent, const int32 ContrastMode)
{
    if (StatusText == nullptr || HeadingText == nullptr || Background == nullptr) return;

    const FString NewStatusText = BuildOceanTravelText(Snapshot);
    if (LastStatusText != NewStatusText)
    {
        StatusText->SetText(FText::FromString(NewStatusText));
        LastStatusText = NewStatusText;
    }
    ApplyAccessibilityPresentation(TextScalePercent, ContrastMode);
    SetVisibility(NewStatusText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UKalmalaSurvivalStatusWidget::ApplyAccessibilityPresentation(const int32 TextScalePercent, const int32 ContrastMode)
{
    const int32 BoundedTextScale = UKalmalaSettingsWidget::ClampTextScale(TextScalePercent);
    const int32 BoundedContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode);
    if (LastTextScalePercent == BoundedTextScale && LastContrastMode == BoundedContrast) return;

    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    Theme.ApplyPanel(*Background, BoundedContrast);
    Theme.ApplyText(*HeadingText, Theme.HeadingSize, true, BoundedTextScale, BoundedContrast);
    Theme.ApplyText(*StatusText, Theme.BodySize, false, BoundedTextScale, BoundedContrast);
    LastTextScalePercent = BoundedTextScale;
    LastContrastMode = BoundedContrast;
}
