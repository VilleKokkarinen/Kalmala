#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaMinimapSubsystem.h"
#include "KalmalaMinimapWidget.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaSupportSelectionSubsystem.h"
#include "KalmalaUITheme.h"
#include "KalmalaStatusIconLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/LocalPlayer.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/WrapBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "KalmalaExposureResponse.h"
#include "HAL/PlatformTime.h"

void UKalmalaStatusHotbarWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    EntriesBox = WidgetTree->ConstructWidget<UWrapBox>();
    EntriesBox->SetExplicitWrapSize(true);
    // UWrapBox applies padding to both sides of each slot, so half the desired inter-cell gap.
    EntriesBox->SetInnerSlotPadding(FVector2D(StatusCellGap * 0.5f, StatusCellGap * 0.5f));
    WidgetTree->RootWidget = EntriesBox;
    SetVisibility(ESlateVisibility::Collapsed);
}

FVector2D UKalmalaStatusHotbarWidget::CalculateSize(int32 Count, int32 TextScale, FVector2D Viewport,
    float StatusGroupRightEdge, float StatusGroupLeftEdge)
{
    if (Count <= 0) return FVector2D::ZeroVector;
    const float Scale = UKalmalaSettingsWidget::ClampTextScale(TextScale) / 100.0f;
    const float SafeRightEdge = FMath::Clamp(StatusGroupRightEdge, 0.0f, FMath::Max(0.0f, Viewport.X));
    const float AvailableWidth = FMath::Max(1.0f, SafeRightEdge
        - FMath::Max(UKalmalaMinimapWidget::ViewportInset, StatusGroupLeftEdge));
    const float RowWidth = FMath::Min(MaximumStatusRowWidth, AvailableWidth);
    const float CellWidth = StatusCellWidth * Scale;
    const int32 Columns = FMath::Min(Count,
        FMath::Max(1, FMath::FloorToInt(RowWidth / (CellWidth + StatusCellGap))));
    const int32 Rows = FMath::DivideAndRoundUp(Count, Columns);
    const float Width = Columns * (CellWidth + StatusCellGap);
    const float Height = Rows * (StatusCellContentHeight * Scale + StatusCellGap);
    return FVector2D(Width, Height);
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
    // M12 removes ended icons immediately; retain only transition bookkeeping.


    FString Identity = FString::FromInt(TextScale);
    for (const auto& Entry : Entries)
        Identity += TEXT("|") + Entry.Id.ToString() + TEXT(":") + Entry.Name + TEXT(":") + Entry.StatusIconId.ToString();
    const FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / UWidgetLayoutLibrary::GetViewportScale(this);
    FVector2D Position = UKalmalaMinimapWidget::GetDefaultStatusGroupViewportPosition();
    float SafeLeftEdge = UKalmalaMinimapWidget::ViewportInset;
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (const UKalmalaMinimapSubsystem* MinimapSubsystem = LocalPlayer->GetSubsystem<UKalmalaMinimapSubsystem>())
        {
            if (const UKalmalaMinimapWidget* MinimapWidget = MinimapSubsystem->GetMinimapWidget())
            {
                Position = MinimapWidget->GetStatusGroupViewportPosition();
            }
        }
        if (const auto* SupportSubsystem = LocalPlayer->GetSubsystem<UKalmalaSupportSelectionSubsystem>())
        {
            const auto* Support = SupportSubsystem->GetSelectionWidget();
            if (Support && Support->IsVisible() && Support->GetCachedGeometry().GetLocalSize().X > 0.0f)
            {
                const FGeometry& SupportGeometry = Support->GetCachedGeometry();
                const FVector2D Right = UWidgetLayoutLibrary::GetViewportWidgetGeometry(this).AbsoluteToLocal(
                    SupportGeometry.LocalToAbsolute(SupportGeometry.GetLocalSize()));
                SafeLeftEdge = FMath::Max(SafeLeftEdge, static_cast<float>(Right.X) + 12.0f);
            }
        }
    }
    const FVector2D Size = CalculateSize(Entries.Num(), TextScale, Viewport, Viewport.X + Position.X, SafeLeftEdge);
    ConfigureViewportPlacement(Size, Position);
    if (Identity != LastIdentity)
    {
        EntriesBox->ClearChildren(); Timers.Reset();
        RasterImages.Reset();
        for (const auto& Entry : Entries)
        {
            auto* Cell = WidgetTree->ConstructWidget<USizeBox>();
            Cell->SetWidthOverride(StatusCellWidth * TextScale / 100.0f);
            Cell->SetHeightOverride(StatusCellContentHeight * TextScale / 100.0f);
            auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
            auto* IconBox = WidgetTree->ConstructWidget<USizeBox>();
            const float IconSize = 64.0f * TextScale / 100.0f;
            IconBox->SetWidthOverride(IconSize); IconBox->SetHeightOverride(IconSize);
            auto* IconLayer = WidgetTree->ConstructWidget<UOverlay>();
            UTexture2D* StatusTexture = FKalmalaStatusIconLibrary::LoadTexture(Entry.StatusIconId);
            auto* RasterIcon = WidgetTree->ConstructWidget<UImage>();
            if (StatusTexture != nullptr) RasterIcon->SetBrushFromTexture(StatusTexture, false);
            RasterIcon->SetVisibility(StatusTexture != nullptr ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            auto* RasterSlot = IconLayer->AddChildToOverlay(RasterIcon);
            RasterSlot->SetHorizontalAlignment(HAlign_Fill);
            RasterSlot->SetVerticalAlignment(VAlign_Fill);
            auto* VectorFallback = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
            VectorFallback->SetIcon(Entry.Icon);
            VectorFallback->SetVisibility(StatusTexture == nullptr ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            IconLayer->AddChild(VectorFallback);
            // Keep the player-facing name in the accessibility tree without drawing it beside the image.
            auto* AccessibleName = WidgetTree->ConstructWidget<UTextBlock>();
            AccessibleName->SetText(FText::FromString(Entry.Name));
            AccessibleName->SetAutoWrapText(true);
            AccessibleName->SetVisibility(ESlateVisibility::HitTestInvisible);
            AccessibleName->SetRenderOpacity(0.0f);
            IconLayer->AddChild(AccessibleName);
            IconBox->SetContent(IconLayer);
            const auto IconSlot = Column->AddChildToVerticalBox(IconBox);
            IconSlot->SetHorizontalAlignment(HAlign_Center);
            auto* Timer = WidgetTree->ConstructWidget<UTextBlock>();
            Timer->SetJustification(ETextJustify::Center);
            const auto TimerSlot = Column->AddChildToVerticalBox(Timer);
            TimerSlot->SetHorizontalAlignment(HAlign_Center);
            Cell->SetContent(Column); EntriesBox->AddChild(Cell);
            Timers.Add(Timer);
            RasterImages.Add(RasterIcon);
        }
        LastIdentity = Identity;
    }
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        Timers[Index]->SetText(FText::FromString(Entries[Index].TimerText));
        Timers[Index]->SetVisibility(Entries[Index].TimerText.IsEmpty()
            ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        Theme.ApplyText(*Timers[Index], Theme.HeadingSize, false, TextScale, Contrast);
        auto Font = Timers[Index]->GetFont(); Font.OutlineSettings.OutlineSize = FMath::Max(1, Font.OutlineSettings.OutlineSize);
        Timers[Index]->SetFont(Font);
        const FActiveCue* Cue = ActiveCues.Find(Entries[Index].Id);
        const float Opacity = Cue ? CalculateCueOpacity(static_cast<float>(Now - Cue->StartedAtSeconds),
            Theme.StatusCueDuration, Theme.bAnimateInteractionStates, UKalmalaSettingsWidget::IsReducedMotionEnabled()) : 1.0f;
        RasterImages[Index]->SetRenderOpacity(Opacity);
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

#if !UE_BUILD_SHIPPING
bool UKalmalaStatusHotbarWidget::HasRasterIconGeometryForVerification(const int32 TextScale, const int32 ExpectedCount) const
{
    if (!WidgetTree) return false;
    const float Size = 64.0f * TextScale / 100.0f;
    int32 Count = 0;
    bool bSized = true;
    WidgetTree->ForEachWidget([&](UWidget* Widget) {
        if (const auto* Raster = Cast<UImage>(Widget); Raster && Raster->GetVisibility() == ESlateVisibility::Visible)
        {
            ++Count;
            bSized &= Raster->GetBrush().GetResourceObject() != nullptr
                && Raster->GetCachedGeometry().GetLocalSize().Equals(FVector2D(Size, Size), 0.5f);
        }
    });
    return bSized && Count == ExpectedCount;
}
#endif

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
    const auto TimerForRemaining = [](float Value)
    {
        const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Value));
        return FString::Printf(TEXT("%02d:%02d"), TotalSeconds / 60, TotalSeconds % 60);
    };
    const auto AddEntry = [&Entries](FName Id, const TCHAR* Name, FString TimerText, EKalmalaIcon Icon, float RefreshValue = 0.0f,
        EKalmalaStatusRefreshPolicy RefreshPolicy = EKalmalaStatusRefreshPolicy::None)
    {
        FName StatusIconId = NAME_None;
        FName IconEntryId = Id;
        if (Id == UKalmalaPlayerStatusComponent::WetStatusId) IconEntryId = TEXT("Wet");
        else if (Id == UKalmalaPlayerStatusComponent::SteadyMealStatusId) IconEntryId = TEXT("SteadyMeal");
        FKalmalaStatusIconLibrary::GetIconIdForEntry(IconEntryId, StatusIconId);
        Entries.Add({ Id, Name, MoveTemp(TimerText), Icon, StatusIconId, RefreshValue, RefreshPolicy });
    };
    // Fixed semantic order, never replication-array order. No local ticking of owner status durations.
    for (const FName Id : { UKalmalaPlayerStatusComponent::WetStatusId, UKalmalaPlayerStatusComponent::SteadyMealStatusId })
    {
        const auto* Status = S.Statuses.FindByPredicate([Id](const auto& E) { return E.StatusId == Id && FMath::IsFinite(E.RemainingSeconds) && E.RemainingSeconds > 0; });
        if (Status) AddEntry(Id, Id == UKalmalaPlayerStatusComponent::WetStatusId ? TEXT("Wet") : TEXT("Steady meal"), TimerForRemaining(Status->RemainingSeconds),
            Id == UKalmalaPlayerStatusComponent::WetStatusId ? EKalmalaIcon::Drop : EKalmalaIcon::Bowl,
            Status->RemainingSeconds, EKalmalaStatusRefreshPolicy::RemainingIncreased);
    }
    if (FMath::IsFinite(S.Exposure.HeatIntensity) && S.Exposure.HeatIntensity >= .05f)
        AddEntry(TEXT("Heat"), TEXT("Hot"), FString(), EKalmalaIcon::Sun);
    if (FMath::IsFinite(S.Exposure.ColdIntensity) && S.Exposure.ColdIntensity >= .05f && FMath::IsFinite(S.Exposure.Warmth)
        && S.Exposure.Warmth < FKalmalaExposureResponse::ColdStaminaRecoveryWarmthThreshold)
        AddEntry(TEXT("Cold"), TEXT("Cold"), FString(), EKalmalaIcon::Snow);
    const float Remaining = S.ActiveSupportEffectExpiry - S.ServerTimeSeconds;
    if (FMath::IsFinite(Remaining) && Remaining > 0)
    {
        switch (S.ActiveSupportEffect)
        {
        case EKalmalaSupportEffect::Mending: AddEntry(TEXT("Mending"), TEXT("Mending"), TimerForRemaining(Remaining), EKalmalaIcon::Cross, S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased); break;
        case EKalmalaSupportEffect::HearthShield: AddEntry(TEXT("Shield"), TEXT("Hearth shield"), TimerForRemaining(Remaining), EKalmalaIcon::Shield, S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased); break;
        case EKalmalaSupportEffect::BearsVigor: AddEntry(TEXT("Vigor"), TEXT("Bear's vigor"), TimerForRemaining(Remaining), EKalmalaIcon::Paw, S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased); break;
        case EKalmalaSupportEffect::DeerCall: AddEntry(TEXT("Call"), TEXT("Deer call"), TimerForRemaining(Remaining), EKalmalaIcon::Antlers, S.ActiveSupportEffectExpiry, EKalmalaStatusRefreshPolicy::ExpiryIncreased); break;
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
        AddEntry(TEXT("Weather"), TEXT("Storm"), FString(), EKalmalaIcon::Storm, S.Weather.ServerStartTimeSeconds, EKalmalaStatusRefreshPolicy::AuthorityStampChanged);
    }
    return Entries;
}
