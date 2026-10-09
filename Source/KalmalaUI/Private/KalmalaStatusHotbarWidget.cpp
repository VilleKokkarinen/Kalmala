#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaMinimapSubsystem.h"
#include "KalmalaMinimapWidget.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"
#include "KalmalaStatusIconLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/LocalPlayer.h"
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
    FVector2D Position = UKalmalaMinimapWidget::GetDefaultStatusGroupViewportPosition();
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (const UKalmalaMinimapSubsystem* MinimapSubsystem = LocalPlayer->GetSubsystem<UKalmalaMinimapSubsystem>())
        {
            if (const UKalmalaMinimapWidget* MinimapWidget = MinimapSubsystem->GetMinimapWidget())
            {
                Position = MinimapWidget->GetStatusGroupViewportPosition();
            }
        }
    }
    ConfigureViewportPlacement(Size, Position);
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

void UKalmalaStatusHotbarWidget::ConfigureViewportPlacement(const FVector2D& Size, const FVector2D& Position)
{
    if (EntriesBox != nullptr)
    {
        EntriesBox->SetWrapSize(Size.X);
    }
    SetDesiredSizeInViewport(Size);
    SetPositionInViewport(Position, false);
    SetAlignmentInViewport(FVector2D(1.0f, 0.0f));
    SetAnchorsInViewport(FAnchors(1.0f, 0.0f)); // UE viewport setters reset anchors; set last.
}

TArray<FKalmalaStatusHotbarEntry> UKalmalaStatusHotbarWidget::BuildEntries(const FKalmalaSurvivalStatusSnapshot& S)
{
    TArray<FKalmalaStatusHotbarEntry> Entries;
    if (!S.bHasCharacter) return Entries;
    const auto Seconds = [](float Value) { return FString::Printf(TEXT("%d s"), FMath::Max(0, FMath::CeilToInt(Value))); };
    const auto AddEntry = [&Entries](FName Id, const TCHAR* Name, FString Duration, EKalmalaIcon Icon)
    {
        FName StatusIconId = NAME_None;
        FKalmalaStatusIconLibrary::GetIconIdForEntry(Id, StatusIconId);
        Entries.Add({ Id, Name, MoveTemp(Duration), Icon, StatusIconId });
    };
    // Fixed semantic order, never replication-array order. No local ticking of owner status durations.
    for (const FName Id : { UKalmalaPlayerStatusComponent::WetStatusId, UKalmalaPlayerStatusComponent::SteadyMealStatusId })
    {
        const auto* Status = S.Statuses.FindByPredicate([Id](const auto& E) { return E.StatusId == Id && FMath::IsFinite(E.RemainingSeconds) && E.RemainingSeconds > 0; });
        if (Status) AddEntry(Id, Id == UKalmalaPlayerStatusComponent::WetStatusId ? TEXT("Wet") : TEXT("Steady meal"), Seconds(Status->RemainingSeconds),
            Id == UKalmalaPlayerStatusComponent::WetStatusId ? EKalmalaIcon::Drop : EKalmalaIcon::Bowl);
    }
    if (FMath::IsFinite(S.Exposure.HeatIntensity) && S.Exposure.HeatIntensity >= .05f)
        AddEntry(TEXT("Heat"), TEXT("Hot"), TEXT("ongoing"), EKalmalaIcon::Sun);
    if (FMath::IsFinite(S.Exposure.ColdIntensity) && S.Exposure.ColdIntensity >= .05f && FMath::IsFinite(S.Exposure.Warmth)
        && S.Exposure.Warmth < FKalmalaExposureResponse::ColdStaminaRecoveryWarmthThreshold)
        AddEntry(TEXT("Cold"), TEXT("Cold"), TEXT("ongoing"), EKalmalaIcon::Snow);
    const float Remaining = S.ActiveSupportEffectExpiry - S.ServerTimeSeconds;
    if (FMath::IsFinite(Remaining) && Remaining > 0)
    {
        switch (S.ActiveSupportEffect)
        {
        case EKalmalaSupportEffect::Mending: AddEntry(TEXT("Mending"), TEXT("Mending"), Seconds(Remaining), EKalmalaIcon::Cross); break;
        case EKalmalaSupportEffect::HearthShield: AddEntry(TEXT("Shield"), TEXT("Hearth shield"), Seconds(Remaining), EKalmalaIcon::Shield); break;
        case EKalmalaSupportEffect::BearsVigor: AddEntry(TEXT("Vigor"), TEXT("Bear's vigor"), Seconds(Remaining), EKalmalaIcon::Paw); break;
        case EKalmalaSupportEffect::DeerCall: AddEntry(TEXT("Call"), TEXT("Deer call"), Seconds(Remaining), EKalmalaIcon::Antlers); break;
        default: break;
        }
    }
    const bool bCurrentWeatherInterval = S.bHasWeatherState && S.Weather.IsValid()
        && FMath::IsFinite(S.ServerTimeSeconds)
        && S.ServerTimeSeconds >= S.Weather.ServerStartTimeSeconds
        && S.ServerTimeSeconds < S.Weather.ServerStartTimeSeconds + S.Weather.DurationSeconds;
    if (bCurrentWeatherInterval
        && S.Weather.GetStormIntensity() >= FKalmalaWeatherState::HighlyActiveStormThreshold)
    {
        const float WeatherRemaining = S.Weather.ServerStartTimeSeconds + S.Weather.DurationSeconds - S.ServerTimeSeconds;
        AddEntry(TEXT("Weather"), TEXT("Storm"), Seconds(WeatherRemaining), EKalmalaIcon::Storm);
    }
    return Entries;
}
