#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/WrapBox.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "KalmalaExposureResponse.h"

void UKalmalaStatusHotbarWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    EntriesBox = WidgetTree->ConstructWidget<UWrapBox>();
    EntriesBox->SetExplicitWrapSize(true);
    EntriesBox->SetInnerSlotPadding(FVector2D(6, 6));
    WidgetTree->RootWidget = EntriesBox;
    SetVisibility(ESlateVisibility::Collapsed);
}

FVector2D UKalmalaStatusHotbarWidget::CalculateSize(int32 Count, int32 TextScale, FVector2D Viewport)
{
    const float Width = FMath::Min(450.0f, FMath::Max(1.0f, static_cast<float>(Viewport.X) - 48.0f));
    const float Cell = 140.0f * UKalmalaSettingsWidget::ClampTextScale(TextScale) / 100.0f;
    const int32 Columns = FMath::Max(1, FMath::FloorToInt((Width + 6) / (Cell + 6)));
    return FVector2D(Width, FMath::DivideAndRoundUp(FMath::Max(0, Count), Columns) * (48.0f * TextScale / 100.0f + 6));
}

void UKalmalaStatusHotbarWidget::SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot, int32 TextScale, int32 Contrast)
{
    if (!EntriesBox) return;
    TextScale = UKalmalaSettingsWidget::ClampTextScale(TextScale);
    const TArray<FKalmalaStatusHotbarEntry> Entries = BuildEntries(Snapshot);
    FString Identity = FString::FromInt(TextScale);
    for (const auto& Entry : Entries) Identity += TEXT("|") + Entry.Id.ToString();
    const FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / UWidgetLayoutLibrary::GetViewportScale(this);
    const FVector2D Size = CalculateSize(Entries.Num(), TextScale, Viewport);
    EntriesBox->SetWrapSize(Size.X);
    SetDesiredSizeInViewport(Size);
    SetPositionInViewport(FVector2D(-24, 244), false);
    SetAlignmentInViewport(FVector2D(1, 0));
    SetAnchorsInViewport(FAnchors(1, 0)); // UE viewport setters reset anchors; set last.
    if (Identity != LastIdentity)
    {
        EntriesBox->ClearChildren(); Labels.Reset(); Durations.Reset();
        for (const auto& Entry : Entries)
        {
            auto* Cell = WidgetTree->ConstructWidget<USizeBox>();
            Cell->SetWidthOverride(140.0f * TextScale / 100.0f);
            Cell->SetHeightOverride(48.0f * TextScale / 100.0f);
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* IconBox = WidgetTree->ConstructWidget<USizeBox>();
            IconBox->SetWidthOverride(32); IconBox->SetHeightOverride(32);
            auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>(); Icon->SetIcon(Entry.Icon);
            IconBox->SetContent(Icon); Row->AddChild(IconBox);
            auto* Text = WidgetTree->ConstructWidget<UVerticalBox>();
            auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetAutoWrapText(true);
            auto* Duration = WidgetTree->ConstructWidget<UTextBlock>();
            Text->AddChild(Label); Text->AddChild(Duration); Row->AddChild(Text);
            Cell->SetContent(Row); EntriesBox->AddChild(Cell);
            Labels.Add(Label); Durations.Add(Duration);
        }
        LastIdentity = Identity;
    }
    const auto& Theme = FKalmalaUITheme::Get();
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        Labels[Index]->SetText(FText::FromString(Entries[Index].Name));
        Durations[Index]->SetText(FText::FromString(Entries[Index].Duration));
        Theme.ApplyText(*Labels[Index], Theme.BodySize, true, TextScale, Contrast);
        Theme.ApplyText(*Durations[Index], Theme.HeadingSize, false, TextScale, Contrast);
        for (UTextBlock* Text : { Labels[Index].Get(), Durations[Index].Get() })
        {
            auto Font = Text->GetFont(); Font.OutlineSettings.OutlineSize = FMath::Max(1, Font.OutlineSettings.OutlineSize);
            Text->SetFont(Font);
        }
    }
    SetVisibility(Entries.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

TArray<FKalmalaStatusHotbarEntry> UKalmalaStatusHotbarWidget::BuildEntries(const FKalmalaSurvivalStatusSnapshot& S)
{
    TArray<FKalmalaStatusHotbarEntry> Entries;
    if (!S.bHasCharacter) return Entries;
    const auto Seconds = [](float Value) { return FString::Printf(TEXT("%d s"), FMath::Max(0, FMath::CeilToInt(Value))); };
    // Fixed semantic order, never replication-array order. No local ticking of owner status durations.
    for (const FName Id : { UKalmalaPlayerStatusComponent::WetStatusId, UKalmalaPlayerStatusComponent::SteadyMealStatusId })
    {
        const auto* Status = S.Statuses.FindByPredicate([Id](const auto& E) { return E.StatusId == Id && FMath::IsFinite(E.RemainingSeconds) && E.RemainingSeconds > 0; });
        if (Status) Entries.Add({ Id, Id == UKalmalaPlayerStatusComponent::WetStatusId ? TEXT("Wet") : TEXT("Steady meal"), Seconds(Status->RemainingSeconds),
            Id == UKalmalaPlayerStatusComponent::WetStatusId ? EKalmalaIcon::Drop : EKalmalaIcon::Bowl });
    }
    if (FMath::IsFinite(S.Exposure.HeatIntensity) && S.Exposure.HeatIntensity >= .05f)
        Entries.Add({ TEXT("Heat"), TEXT("Hot"), TEXT("ongoing"), EKalmalaIcon::Sun });
    if (FMath::IsFinite(S.Exposure.ColdIntensity) && S.Exposure.ColdIntensity >= .05f && FMath::IsFinite(S.Exposure.Warmth)
        && S.Exposure.Warmth < FKalmalaExposureResponse::ColdStaminaRecoveryWarmthThreshold)
        Entries.Add({ TEXT("Cold"), TEXT("Cold"), TEXT("ongoing"), EKalmalaIcon::Snow });
    const float Remaining = S.ActiveSupportEffectExpiry - S.ServerTimeSeconds;
    if (FMath::IsFinite(Remaining) && Remaining > 0)
    {
        switch (S.ActiveSupportEffect)
        {
        case EKalmalaSupportEffect::Mending: Entries.Add({ TEXT("Mending"), TEXT("Mending"), Seconds(Remaining), EKalmalaIcon::Cross }); break;
        case EKalmalaSupportEffect::HearthShield: Entries.Add({ TEXT("Shield"), TEXT("Hearth shield"), Seconds(Remaining), EKalmalaIcon::Shield }); break;
        case EKalmalaSupportEffect::BearsVigor: Entries.Add({ TEXT("Vigor"), TEXT("Bear's vigor"), Seconds(Remaining), EKalmalaIcon::Paw }); break;
        case EKalmalaSupportEffect::DeerCall: Entries.Add({ TEXT("Call"), TEXT("Deer call"), Seconds(Remaining), EKalmalaIcon::Antlers }); break;
        default: break;
        }
    }
    if (S.bHasWeatherState && S.Weather.IsValid() && FMath::IsFinite(S.ServerTimeSeconds))
    {
        const bool bStorm = S.Weather.StormIntensity >= .05f;
        const float WeatherRemaining = FMath::Clamp(S.Weather.DurationSeconds - FMath::Max(0.0f, S.ServerTimeSeconds - S.Weather.ServerStartTimeSeconds), 0.0f, S.Weather.DurationSeconds);
        const FString Name = bStorm ? TEXT("Storm") : S.Weather.ActivityLevel == EKalmalaWeatherActivityLevel::HighlyActive
            ? TEXT("High activity") : S.Weather.ActivityLevel == EKalmalaWeatherActivityLevel::Active ? TEXT("Active weather") : TEXT("Calm weather");
        Entries.Add({ TEXT("Weather"), Name, WeatherRemaining > 0 ? Seconds(WeatherRemaining) : TEXT("awaiting update"), bStorm ? EKalmalaIcon::Storm : EKalmalaIcon::Cloud });
    }
    return Entries;
}
