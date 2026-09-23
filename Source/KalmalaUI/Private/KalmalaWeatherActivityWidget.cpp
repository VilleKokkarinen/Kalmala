#include "KalmalaWeatherActivityWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaWeatherState.h"

void UKalmalaWeatherActivityWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);

    Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WeatherActivityBackground"));
    Background->SetPadding(FMargin(10.0f, 7.0f));

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WeatherActivityContent"));
    HeadingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeatherActivityHeading"));
    HeadingText->SetText(FText::FromString(TEXT("WEATHER ACTIVITY")));
    ActivityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeatherActivityTier"));
    ActivityText->SetAutoWrapText(false);
    Content->AddChildToVerticalBox(HeadingText);
    Content->AddChildToVerticalBox(ActivityText);
    Background->SetContent(Content);
    WidgetTree->RootWidget = Background;
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaWeatherActivityWidget::ConfigureViewportPlacement()
{
    // The minimap ends at y=232; the 12-unit gap keeps this badge below it.
    // UE 5.8 resets anchors in the size and position setters, so set anchors last.
    SetDesiredSizeInViewport(FVector2D(252.0f, 72.0f));
    SetPositionInViewport(FVector2D(-24.0f, 244.0f), false);
    SetAlignmentInViewport(FVector2D(1.0f, 0.0f));
    SetAnchorsInViewport(FAnchors(1.0f, 0.0f));
}

FString UKalmalaWeatherActivityWidget::BuildActivityLabel(const EKalmalaWeatherActivityLevel ActivityLevel)
{
    switch (ActivityLevel)
    {
    case EKalmalaWeatherActivityLevel::Calm: return TEXT("○ CALM");
    case EKalmalaWeatherActivityLevel::Active: return TEXT("◇ ACTIVE");
    case EKalmalaWeatherActivityLevel::HighlyActive: return TEXT("▲ HIGHLY ACTIVE");
    default: return TEXT("? WEATHER UNKNOWN");
    }
}

void UKalmalaWeatherActivityWidget::SetWeatherActivity(const EKalmalaWeatherActivityLevel ActivityLevel,
    const int32 TextScalePercent, const int32 ContrastMode)
{
    if (ActivityText == nullptr || HeadingText == nullptr || Background == nullptr) return;

    const uint8 ActivityValue = static_cast<uint8>(ActivityLevel);
    if (LastActivityLevel != ActivityValue)
    {
        ActivityText->SetText(FText::FromString(BuildActivityLabel(ActivityLevel)));
        LastActivityLevel = ActivityValue;
    }
    ApplyAccessibilityPresentation(TextScalePercent, ContrastMode);
}

void UKalmalaWeatherActivityWidget::ApplyAccessibilityPresentation(const int32 TextScalePercent, const int32 ContrastMode)
{
    const int32 BoundedTextScale = UKalmalaSettingsWidget::ClampTextScale(TextScalePercent);
    const int32 BoundedContrast = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode);
    if (LastTextScalePercent == BoundedTextScale && LastContrastMode == BoundedContrast) return;

    const bool bHighContrast = BoundedContrast != 0;
    Background->SetBrushColor(bHighContrast
        ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
        : FLinearColor(0.025f, 0.035f, 0.04f, 0.94f));
    HeadingText->SetColorAndOpacity(FSlateColor(bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.75f, 0.82f, 0.79f, 1.0f)));
    ActivityText->SetColorAndOpacity(FSlateColor(bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.93f, 0.96f, 0.94f, 1.0f)));
    HeadingText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::RoundToInt(10.0f * BoundedTextScale / 100.0f)));
    ActivityText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::RoundToInt(17.0f * BoundedTextScale / 100.0f)));
    LastTextScalePercent = BoundedTextScale;
    LastContrastMode = BoundedContrast;
}
