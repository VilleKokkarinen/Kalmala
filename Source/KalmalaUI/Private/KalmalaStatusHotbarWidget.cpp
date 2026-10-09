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
#include "HAL/PlatformTime.h"

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

void UKalmalaStatusHotbarWidget::SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot, int32 TextScale,
    int32 Contrast, const bool bResetTransitionHistory)
{
    if (!EntriesBox) return;
    TextScale = UKalmalaSettingsWidget::ClampTextScale(TextScale);
    const TArray<FKalmalaStatusHotbarEntry> CurrentEntries = BuildEntries(Snapshot);
    const double Now = FPlatformTime::Seconds();
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();

    if (!Snapshot.bHasCharacter || bResetTransitionHistory)
    {
        PreviousEntries.Reset();
        ActiveCues.Reset();
        CueOrder.Reset();
        TransitionArmedAtSeconds = 0.0;
        bHasTransitionBaseline = false;
    }
    else
    {
        for (int32 Index = CueOrder.Num() - 1; Index >= 0; --Index)
        {
            const FName Id = CueOrder[Index];
            const FActiveCue* Cue = ActiveCues.Find(Id);
            if (Cue == nullptr || Now >= Cue->ExpiresAtSeconds)
            {
                ActiveCues.Remove(Id);
                CueOrder.RemoveAt(Index);
            }
        }

        if (!bHasTransitionBaseline)
        {
            // Wait until the owner and initial server weather snapshot are present. Existing
            // effects then become a silent baseline instead of replaying on reconnect.
            if (Snapshot.bHasWeatherState && Snapshot.Weather.IsValid())
            {
                PreviousEntries = CurrentEntries;
                bHasTransitionBaseline = true;
                TransitionArmedAtSeconds = Now + 0.75;
            }
        }
        else if (Now < TransitionArmedAtSeconds)
        {
            // Fold late initial property replication into the baseline after owner/pawn changes.
            PreviousEntries = CurrentEntries;
        }
        else
        {
            for (const FKalmalaStatusHotbarTransition& Transition : BuildTransitions(
                PreviousEntries, CurrentEntries, true))
            {
                FActiveCue* Existing = ActiveCues.Find(Transition.Id);
                if (Existing != nullptr)
                {
                    if (!ShouldReplaceActiveCue(Existing->Kind, Transition.Kind)) continue;
                    Existing->Entry = Transition.Entry;
                    Existing->Kind = Transition.Kind;
                    Existing->StartedAtSeconds = Now;
                    Existing->ExpiresAtSeconds = Now + Theme.StatusCueDuration;
                    continue;
                }

                FActiveCue& Cue = ActiveCues.Add(Transition.Id);
                Cue.Entry = Transition.Entry;
                Cue.Kind = Transition.Kind;
                Cue.StartedAtSeconds = Now;
                Cue.ExpiresAtSeconds = Now + Theme.StatusCueDuration;
                CueOrder.Add(Transition.Id);
            }
            PreviousEntries = CurrentEntries;
        }
    }

    TArray<FKalmalaStatusHotbarEntry> Entries = CurrentEntries;
    for (FKalmalaStatusHotbarEntry& Entry : Entries)
    {
        if (const FActiveCue* Cue = ActiveCues.Find(Entry.Id)) Entry.Cue = Cue->Kind;
    }
    for (const FName Id : CueOrder)
    {
        const FActiveCue* Cue = ActiveCues.Find(Id);
        if (Cue == nullptr || Cue->Kind != EKalmalaStatusCueKind::Ended
            || Entries.ContainsByPredicate([Id](const FKalmalaStatusHotbarEntry& Entry) { return Entry.Id == Id; }))
        {
            continue;
        }
        FKalmalaStatusHotbarEntry EndedEntry = Cue->Entry;
        EndedEntry.Cue = EKalmalaStatusCueKind::Ended;
        EndedEntry.Duration = TEXT("Ended");
        Entries.Add(MoveTemp(EndedEntry));
    }

    FString Identity = FString::FromInt(TextScale);
    for (const auto& Entry : Entries)
    {
        Identity += FString::Printf(TEXT("|%s:%d"), *Entry.Id.ToString(), static_cast<int32>(Entry.Icon));
    }
    const FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / UWidgetLayoutLibrary::GetViewportScale(this);
    const FVector2D Size = CalculateSize(Entries.Num(), TextScale, Viewport);
    EntriesBox->SetWrapSize(Size.X);
    SetDesiredSizeInViewport(Size);
    SetPositionInViewport(FVector2D(-24, 244), false);
    SetAlignmentInViewport(FVector2D(1, 0));
    SetAnchorsInViewport(FAnchors(1, 0)); // UE viewport setters reset anchors; set last.
    if (Identity != LastIdentity)
    {
        EntriesBox->ClearChildren(); Labels.Reset(); Durations.Reset(); Icons.Reset();
        for (const auto& Entry : Entries)
        {
            auto* Cell = WidgetTree->ConstructWidget<USizeBox>();
            Cell->SetWidthOverride(140.0f * TextScale / 100.0f);
            Cell->SetHeightOverride(48.0f * TextScale / 100.0f);
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* IconBox = WidgetTree->ConstructWidget<USizeBox>();
            IconBox->SetWidthOverride(32); IconBox->SetHeightOverride(32);
            auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>(); Icon->SetIcon(Entry.Icon);
            Icons.Add(Icon);
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
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        const FKalmalaStatusHotbarEntry& Entry = Entries[Index];
        Icons[Index]->SetIcon(Entry.Icon);
        const FActiveCue* Cue = ActiveCues.Find(Entry.Id);
        FString DurationText = Entry.Duration;
        FLinearColor CueColor = FLinearColor::Transparent;
        if (Cue != nullptr && Entry.Cue != EKalmalaStatusCueKind::None)
        {
            switch (Entry.Cue)
            {
            case EKalmalaStatusCueKind::Started:
                DurationText = TEXT("Started");
                CueColor = Contrast > 0 ? FLinearColor::White : Theme.StatusCueStartedColor;
                break;
            case EKalmalaStatusCueKind::Refreshed:
                DurationText = TEXT("Refreshed");
                CueColor = Contrast > 0 ? FLinearColor::White : Theme.StatusCueRefreshedColor;
                break;
            case EKalmalaStatusCueKind::Ended:
                DurationText = TEXT("Ended");
                CueColor = Contrast > 0 ? FLinearColor::White : Theme.StatusCueEndedColor;
                break;
            default:
                break;
            }
            const float Opacity = CalculateCueOpacity(static_cast<float>(Now - Cue->StartedAtSeconds),
                Theme.StatusCueDuration, Theme.bAnimateInteractionStates, UKalmalaSettingsWidget::IsReducedMotionEnabled());
            CueColor.A *= Opacity;
        }
        Icons[Index]->SetCueColor(CueColor);
        Labels[Index]->SetText(FText::FromString(Entry.Name));
        Durations[Index]->SetText(FText::FromString(DurationText));
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

TArray<FKalmalaStatusHotbarTransition> UKalmalaStatusHotbarWidget::BuildTransitions(
    const TArray<FKalmalaStatusHotbarEntry>& Previous,
    const TArray<FKalmalaStatusHotbarEntry>& Current,
    const bool bHasBaseline)
{
    TArray<FKalmalaStatusHotbarTransition> Transitions;
    if (!bHasBaseline) return Transitions;

    TSet<FName> SeenCurrent;
    for (const FKalmalaStatusHotbarEntry& Entry : Current)
    {
        if (Entry.Id.IsNone() || SeenCurrent.Contains(Entry.Id)) continue;
        SeenCurrent.Add(Entry.Id);
        const FKalmalaStatusHotbarEntry* PreviousEntry = Previous.FindByPredicate(
            [&Entry](const FKalmalaStatusHotbarEntry& Candidate) { return Candidate.Id == Entry.Id; });
        if (PreviousEntry == nullptr)
        {
            Transitions.Add({Entry.Id, EKalmalaStatusCueKind::Started, Entry});
        }
        else if (Entry.Name != PreviousEntry->Name || Entry.Icon != PreviousEntry->Icon
            || (Entry.RefreshPolicy == PreviousEntry->RefreshPolicy
                && FMath::IsFinite(Entry.RefreshValue) && FMath::IsFinite(PreviousEntry->RefreshValue)
                && ((Entry.RefreshPolicy == EKalmalaStatusRefreshPolicy::RemainingIncreased
                        && Entry.RefreshValue > PreviousEntry->RefreshValue + 0.5f)
                    || (Entry.RefreshPolicy == EKalmalaStatusRefreshPolicy::ExpiryIncreased
                        && Entry.RefreshValue > PreviousEntry->RefreshValue + 0.25f)
                    || (Entry.RefreshPolicy == EKalmalaStatusRefreshPolicy::AuthorityStampChanged
                        && !FMath::IsNearlyEqual(Entry.RefreshValue, PreviousEntry->RefreshValue, 0.25f)))))
        {
            Transitions.Add({Entry.Id, EKalmalaStatusCueKind::Refreshed, Entry});
        }
    }

    TSet<FName> SeenPrevious;
    for (const FKalmalaStatusHotbarEntry& Entry : Previous)
    {
        if (Entry.Id.IsNone() || SeenPrevious.Contains(Entry.Id)) continue;
        SeenPrevious.Add(Entry.Id);
        if (!SeenCurrent.Contains(Entry.Id))
        {
            Transitions.Add({Entry.Id, EKalmalaStatusCueKind::Ended, Entry});
        }
    }
    return Transitions;
}

bool UKalmalaStatusHotbarWidget::ShouldReplaceActiveCue(const EKalmalaStatusCueKind Existing,
    const EKalmalaStatusCueKind Incoming)
{
    // One visible cue per status coalesces repeated updates. A final end or a new start
    // supersedes it; refreshes do not restart an active start/refresh cue.
    return Incoming == EKalmalaStatusCueKind::Ended
        || (Incoming == EKalmalaStatusCueKind::Started && Existing != EKalmalaStatusCueKind::Started);
}

float UKalmalaStatusHotbarWidget::CalculateCueOpacity(const float ElapsedSeconds, const float DurationSeconds,
    const bool bAnimate, const bool bReducedMotion)
{
    if (!FMath::IsFinite(ElapsedSeconds) || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.0f
        || ElapsedSeconds >= DurationSeconds)
    {
        return 0.0f;
    }
    if (!bAnimate || bReducedMotion) return 1.0f;

    const float Progress = FMath::Clamp(ElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
    const float Pulse = 0.75f + 0.25f * (0.5f + 0.5f * FMath::Cos(Progress * 4.0f * PI));
    return FMath::Clamp((1.0f - Progress) * Pulse, 0.0f, 1.0f);
}

#if !UE_BUILD_SHIPPING
EKalmalaStatusCueKind UKalmalaStatusHotbarWidget::GetActiveCueKindForVerification(const FName Id) const
{
    const FActiveCue* Cue = ActiveCues.Find(Id);
    return Cue != nullptr ? Cue->Kind : EKalmalaStatusCueKind::None;
}

float UKalmalaStatusHotbarWidget::GetActiveCueOpacityForVerification(const FName Id) const
{
    const FActiveCue* Cue = ActiveCues.Find(Id);
    if (Cue == nullptr) return 0.0f;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    return CalculateCueOpacity(static_cast<float>(FPlatformTime::Seconds() - Cue->StartedAtSeconds),
        Theme.StatusCueDuration, Theme.bAnimateInteractionStates, UKalmalaSettingsWidget::IsReducedMotionEnabled());
}
#endif

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
            Id == UKalmalaPlayerStatusComponent::WetStatusId ? EKalmalaIcon::Drop : EKalmalaIcon::Bowl,
            Status->RemainingSeconds, EKalmalaStatusRefreshPolicy::RemainingIncreased });
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
        case EKalmalaSupportEffect::Mending: Entries.Add({ TEXT("Mending"), TEXT("Mending"), Seconds(Remaining), EKalmalaIcon::Cross,
            S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased }); break;
        case EKalmalaSupportEffect::HearthShield: Entries.Add({ TEXT("Shield"), TEXT("Hearth shield"), Seconds(Remaining), EKalmalaIcon::Shield,
            S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased }); break;
        case EKalmalaSupportEffect::BearsVigor: Entries.Add({ TEXT("Vigor"), TEXT("Bear's vigor"), Seconds(Remaining), EKalmalaIcon::Paw,
            S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased }); break;
        case EKalmalaSupportEffect::DeerCall: Entries.Add({ TEXT("Call"), TEXT("Deer call"), Seconds(Remaining), EKalmalaIcon::Antlers,
            S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased }); break;
        default: break;
        }
    }
    if (S.bHasWeatherState && S.Weather.IsValid() && FMath::IsFinite(S.ServerTimeSeconds))
    {
        const bool bStorm = S.Weather.StormIntensity >= .05f;
        const float WeatherRemaining = FMath::Clamp(S.Weather.DurationSeconds - FMath::Max(0.0f, S.ServerTimeSeconds - S.Weather.ServerStartTimeSeconds), 0.0f, S.Weather.DurationSeconds);
        const FString Name = bStorm ? TEXT("Storm") : S.Weather.ActivityLevel == EKalmalaWeatherActivityLevel::HighlyActive
            ? TEXT("High activity") : S.Weather.ActivityLevel == EKalmalaWeatherActivityLevel::Active ? TEXT("Active weather") : TEXT("Calm weather");
        Entries.Add({ TEXT("Weather"), Name, WeatherRemaining > 0 ? Seconds(WeatherRemaining) : TEXT("awaiting update"),
            bStorm ? EKalmalaIcon::Storm : EKalmalaIcon::Cloud, S.Weather.ServerStartTimeSeconds,
            EKalmalaStatusRefreshPolicy::AuthorityStampChanged });
    }
    return Entries;
}
