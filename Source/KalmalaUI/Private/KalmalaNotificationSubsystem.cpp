#include "KalmalaNotificationSubsystem.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaUITheme.h"
#include "KalmalaIconWidget.h"
#include "KalmalaSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

namespace
{
FString NoticeText(const FKalmalaSkillNotice& Notice);
const TCHAR* SkillName(EKalmalaSkill Skill)
{
    switch (Skill)
    {
    case EKalmalaSkill::Gathering: return TEXT("Gathering");
    case EKalmalaSkill::Woodcutting: return TEXT("Woodcutting");
    case EKalmalaSkill::Mining: return TEXT("Mining");
    case EKalmalaSkill::Crafting: return TEXT("Crafting");
    case EKalmalaSkill::Cooking: return TEXT("Cooking");
    case EKalmalaSkill::Survival: return TEXT("Survival");
    default: return TEXT("Skill");
    }
}
EKalmalaIcon SkillIcon(EKalmalaSkill Skill)
{
    switch (Skill)
    {
    case EKalmalaSkill::Gathering: return EKalmalaIcon::Fibre;
    case EKalmalaSkill::Woodcutting: return EKalmalaIcon::Axe;
    case EKalmalaSkill::Mining: return EKalmalaIcon::Pick;
    case EKalmalaSkill::Crafting: return EKalmalaIcon::Hammer;
    case EKalmalaSkill::Cooking: return EKalmalaIcon::Bowl;
    default: return EKalmalaIcon::Shield;
    }
}
FString NoticeText(const FKalmalaSkillNotice& Notice)
{
    if (!Notice.ItemId.IsNone())
    {
        const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Notice.ItemId);
        return FString::Printf(TEXT("Gained %d %s"), Notice.Quantity, Item ? *Item->DisplayName : *Notice.ItemId.ToString());
    }
    if (!Notice.DiscoveryText.IsEmpty()) return Notice.DiscoveryText;
    return FString::Printf(TEXT("%s reached level %d"), SkillName(Notice.Skill), Notice.Level);
}
}

void UKalmalaNotificationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); BuildPanel();
}

TSharedRef<SWidget> UKalmalaNotificationWidget::RebuildWidget()
{
    BuildPanel();
    return Super::RebuildWidget();
}

void UKalmalaNotificationWidget::BuildPanel()
{
    if (Panel || !WidgetTree) return;
    SetIsFocusable(false);
    Panel = WidgetTree->ConstructWidget<UBorder>();
    Rows = WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->SetContent(Rows); WidgetTree->RootWidget = Panel;
}

void UKalmalaNotificationWidget::SetNotices(const TArray<FKalmalaSkillNotice>& Notices, int32 Scale, int32 Contrast)
{
    if (!WidgetTree) Initialize();
    if (!Panel) TakeWidget();
    if (!Panel) return;
    FString Next;
    for (int32 Index = 0; Index < FMath::Min(Notices.Num(), FKalmalaSkillNoticeQueue::MaxRows); ++Index)
        Next += NoticeText(Notices[Index]) + TEXT("\n");
    if (Next == Presentation && Scale == LastScale && Contrast == LastContrast) return;
    Presentation = Next; LastScale = Scale; LastContrast = Contrast;
    Rows->ClearChildren();
    const auto& Theme = FKalmalaUITheme::Get();
    const FString NoImage; Theme.ApplyPanel(*Panel, Contrast, &NoImage);
    for (int32 Index = 0; Index < FMath::Min(Notices.Num(), FKalmalaSkillNoticeQueue::MaxRows); ++Index)
    {
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Box = WidgetTree->ConstructWidget<USizeBox>();
        Box->SetWidthOverride(28); Box->SetHeightOverride(28);
        auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
        EKalmalaIcon Kind = SkillIcon(Notices[Index].Skill); int32 Variant = 0;
        if (!Notices[Index].DiscoveryText.IsEmpty()) Kind = EKalmalaIcon::Discovery;
        else if (!Notices[Index].ItemId.IsNone()) UKalmalaIconWidget::FindCatalogueIcon(Notices[Index].ItemId, Kind, Variant);
        Icon->SetIcon(Kind, Variant);
        Box->SetContent(Icon); Row->AddChild(Box);
        auto* Text = WidgetTree->ConstructWidget<UTextBlock>(); Text->SetAutoWrapText(true);
        Text->SetText(FText::FromString(NoticeText(Notices[Index])));
        Theme.ApplyText(*Text, Theme.BodySize, false, Scale, Contrast);
        auto* TextSlot = Row->AddChildToHorizontalBox(Text); TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextSlot->SetPadding(FMargin(Theme.SlotPadding, 0)); Rows->AddChild(Row);
    }
}

void UKalmalaNotificationSubsystem::Tick(float DeltaTime)
{
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetLocalPlayer()) return;
    auto* Controller = GetLocalPlayer()->GetPlayerController(GetWorld());
    APawn* Pawn = Controller && Controller->IsLocalController() ? Controller->GetPawn() : nullptr;
    if (OwnerPawn.Get() != Pawn)
    {
        Queue.Reset(); OwnerPawn = Pawn;
        bOwnerBaselineAudited = false;
#if !UE_BUILD_SHIPPING
        bReviewFixtureInitialized = false;
        bReviewSettingsApplied = false;
        ReviewCaptureStage = 0;
        ReviewCaptureElapsed = 0.0f;
        ReviewCaptureBasePath.Reset();
#endif
        if (Widget) Widget->RemoveFromParent(); Widget = nullptr;
    }
#if !UE_BUILD_SHIPPING
    FString NotificationCapturePath;
    const bool bNotificationCaptureRequested = FParse::Value(
        FCommandLine::Get(), TEXT("KalmalaNotificationCapture="), NotificationCapturePath);
    const bool bReviewPresentationActive = bNotificationCaptureRequested && bReviewFixtureInitialized;
    if (bNotificationCaptureRequested && !bReviewSettingsApplied)
    {
        int32 ReviewScale = UKalmalaSettingsWidget::GetTextScalePercent();
        int32 ReviewContrast = UKalmalaSettingsWidget::GetContrastMode();
        if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperTextScale="), ReviewScale))
            UKalmalaSettingsWidget::SetTextScalePercent(ReviewScale);
        if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperContrast="), ReviewContrast))
            UKalmalaSettingsWidget::SetContrastMode(ReviewContrast);
        bReviewSettingsApplied = true;
    }
#endif
    Queue.Tick(DeltaTime);
    const auto* Skills = Pawn ? Pawn->FindComponentByClass<UKalmalaSkillProgressionComponent>() : nullptr;
#if !UE_BUILD_SHIPPING
    const bool bSkillObserved = bReviewPresentationActive || (Skills && Queue.Observe(Skills->GetDetailedProgression(), FKalmalaUITheme::Get().NotificationLifetime));
#else
    const bool bSkillObserved = Skills && Queue.Observe(Skills->GetDetailedProgression(), FKalmalaUITheme::Get().NotificationLifetime);
#endif
    const auto* Inventory = Pawn ? Pawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
#if !UE_BUILD_SHIPPING
    const bool bGainsObserved = bReviewPresentationActive || (Inventory && Queue.ObserveGains(Inventory->GetGainReceipts(), FKalmalaUITheme::Get().NotificationLifetime));
#else
    const bool bGainsObserved = Inventory && Queue.ObserveGains(Inventory->GetGainReceipts(), FKalmalaUITheme::Get().NotificationLifetime);
#endif
    const auto* Discovery = Pawn ? Pawn->FindComponentByClass<UKalmalaDiscoveryProgressComponent>() : nullptr;
#if !UE_BUILD_SHIPPING
    const bool bDiscoveryObserved = bReviewPresentationActive || (Discovery && Queue.ObserveDiscovery(Discovery->GetFeedbackSerial(), Discovery->GetFeedback(),
        Discovery->GetFeedbackLabel(), FKalmalaUITheme::Get().NotificationLifetime));
#else
    const bool bDiscoveryObserved = Discovery && Queue.ObserveDiscovery(Discovery->GetFeedbackSerial(), Discovery->GetFeedback(),
        Discovery->GetFeedbackLabel(), FKalmalaUITheme::Get().NotificationLifetime);
#endif

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaNotificationBaselineAudit"))
        && !bOwnerBaselineAudited && !bReviewPresentationActive && bSkillObserved && bGainsObserved && bDiscoveryObserved)
    {
        bOwnerBaselineAudited = true;
        const bool bSilent = Queue.GetRows().IsEmpty();
        UE_LOG(LogTemp, Display, TEXT("Notification owner baseline: Silent=%d Rows=%d Sources=3"),
            bSilent, Queue.GetRows().Num());
    }

    if (bNotificationCaptureRequested && !bReviewFixtureInitialized && Controller
        && !Controller->IsMoveInputIgnored() && Skills && Inventory && Discovery
        && bSkillObserved && bGainsObserved && bDiscoveryObserved)
    {
        const float Lifetime = FKalmalaUITheme::Get().NotificationLifetime;
        const auto LiveSkills = Skills->GetDetailedProgression();
        const auto LiveGains = Inventory->GetGainReceipts();
        const uint32 LiveDiscoverySerial = Discovery->GetFeedbackSerial();
        const auto LiveDiscoveryFeedback = Discovery->GetFeedback();
        const FString LiveDiscoveryLabel = Discovery->GetFeedbackLabel();

        Queue.Reset();
        const bool bFirstBaselineReady = Queue.Observe(LiveSkills, Lifetime)
            && Queue.ObserveGains(LiveGains, Lifetime)
            && Queue.ObserveDiscovery(LiveDiscoverySerial, LiveDiscoveryFeedback, LiveDiscoveryLabel, Lifetime);
        const bool bFirstBaselineSilent = bFirstBaselineReady && Queue.GetRows().IsEmpty();
        Queue.Reset();
        const bool bReconnectBaselineReady = Queue.Observe(LiveSkills, Lifetime)
            && Queue.ObserveGains(LiveGains, Lifetime)
            && Queue.ObserveDiscovery(LiveDiscoverySerial, LiveDiscoveryFeedback, LiveDiscoveryLabel, Lifetime);
        const bool bReconnectBaselineSilent = bReconnectBaselineReady && Queue.GetRows().IsEmpty();

        FKalmalaSkillProgressionLedger ReviewSkills;
        ReviewSkills.Initialize();
        Queue.Reset();
        const bool bSkillBaselineReady = Queue.Observe(ReviewSkills.Skills, Lifetime);
        for (int32 Award = 0; Award < 4; ++Award)
            ReviewSkills.AwardExperienceFromServer(EKalmalaSkill::Crafting, true, true, 25);
        const bool bSkillNoticeReady = Queue.Observe(ReviewSkills.Skills, Lifetime);

        TArray<FKalmalaItemGainReceipt> ReviewGains{{1, TEXT("Wood"), 1}};
        const bool bGainBaselineReady = Queue.ObserveGains(ReviewGains, Lifetime);
        ReviewGains.Add({2, TEXT("Wood"), 2});
        const bool bGainNoticeReady = Queue.ObserveGains(ReviewGains, Lifetime);

        const bool bDiscoveryBaselineReady = Queue.ObserveDiscovery(
            1, EKalmalaDiscoveryFeedback::Unavailable, FString(), Lifetime);
        const FString ReviewLabel = Pawn->HasAuthority() ? TEXT("Host owner discovery") : TEXT("Client owner discovery");
        const bool bDiscoveryNoticeReady = Queue.ObserveDiscovery(
            2, EKalmalaDiscoveryFeedback::LandmarkFound, ReviewLabel, Lifetime);
        const int32 SkillRows = Queue.GetRows().FilterByPredicate([](const auto& Row)
            { return Row.ItemId.IsNone() && Row.DiscoveryText.IsEmpty(); }).Num();
        const int32 ItemRows = Queue.GetRows().FilterByPredicate([](const auto& Row)
            { return !Row.ItemId.IsNone(); }).Num();
        const int32 DiscoveryRows = Queue.GetRows().FilterByPredicate([](const auto& Row)
            { return !Row.DiscoveryText.IsEmpty(); }).Num();
        const bool bCombinedReady = bFirstBaselineSilent && bReconnectBaselineSilent
            && bSkillBaselineReady && bSkillNoticeReady && bGainBaselineReady && bGainNoticeReady
            && bDiscoveryBaselineReady && bDiscoveryNoticeReady && Queue.GetRows().Num() == 3
            && SkillRows == 1 && ItemRows == 1 && DiscoveryRows == 1;
        UE_LOG(LogTemp, Display, TEXT("Notification reconnect baseline: Silent=%d Rows=%d Sources=3"),
            bFirstBaselineSilent && bReconnectBaselineSilent,
            (bFirstBaselineSilent && bReconnectBaselineSilent) ? 0 : Queue.GetRows().Num());
        UE_LOG(LogTemp, Display, TEXT("Notification combined fixture: Ready=%d Rows=%d Skill=%d Item=%d Discovery=%d Owner=%s"),
            bCombinedReady, Queue.GetRows().Num(), SkillRows, ItemRows, DiscoveryRows,
            Pawn->HasAuthority() ? TEXT("Host") : TEXT("Client"));
        if (bCombinedReady)
        {
            ReviewCaptureBasePath = MoveTemp(NotificationCapturePath);
            bReviewFixtureInitialized = true;
            ReviewCaptureStage = 0;
            ReviewCaptureElapsed = 0.0f;
        }
    }
    else if (!bNotificationCaptureRequested)
    {
        bReviewFixtureInitialized = false;
        bReviewSettingsApplied = false;
    }
#endif

    if (!Pawn || !Controller || Queue.GetRows().IsEmpty() || Controller->IsMoveInputIgnored())
    {
        if (Widget) Widget->SetVisibility(ESlateVisibility::Collapsed);
#if !UE_BUILD_SHIPPING
        if (bReviewFixtureInitialized && Controller && Controller->IsMoveInputIgnored() && ReviewCaptureStage == 1)
        {
            ReviewCaptureElapsed += DeltaTime;
            if (ReviewCaptureElapsed >= 0.5f)
            {
                UE_LOG(LogTemp, Display, TEXT("Notification modal fixture: Collapsed=%d Rows=%d"),
                    Widget && Widget->GetVisibility() == ESlateVisibility::Collapsed, Queue.GetRows().Num());
                FScreenshotRequest::RequestScreenshot(ReviewCaptureBasePath + TEXT("-modal.png"), true, false);
                Controller->SetIgnoreMoveInput(bReviewPriorMoveInputIgnored);
                ReviewCaptureStage = 2;
                ReviewCaptureElapsed = 0.0f;
            }
        }
#endif
        return;
    }
    if (!Widget)
    {
        Widget = CreateWidget<UKalmalaNotificationWidget>(Controller);
        if (!Widget) return;
        Widget->AddToPlayerScreen(110);
    }
    int32 Width = 0, Height = 0; Controller->GetViewportSize(Width, Height);
    if (Width <= 48 || Height <= 200)
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    const float DPI = FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(Widget));
    Widget->SetAlignmentInViewport(FVector2D(1,1));
    Widget->SetPositionInViewport(FVector2D(Width / DPI - 24, Height / DPI - 280), false);
    const auto& Theme = FKalmalaUITheme::Get();
    const float FontSize = Theme.MakeFont(Theme.BodySize, false, UKalmalaSettingsWidget::GetTextScalePercent()).Size;
    const float PanelHeight = Queue.GetRows().Num() * (FontSize * 2 + 28) + Theme.PaddingY * 2;
    Widget->SetDesiredSizeInViewport(FVector2D(FMath::Max(1.f, FMath::Min(320.f, Width / DPI - 48)), PanelHeight));
    Widget->SetNotices(Queue.GetRows(), UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
#if !UE_BUILD_SHIPPING
    if (bReviewFixtureInitialized && ReviewCaptureStage < 3)
    {
        ReviewCaptureElapsed += DeltaTime;
        if (ReviewCaptureStage == 0 && ReviewCaptureElapsed >= 0.75f)
        {
            const FString Presented = Widget->GetPresentationText();
            const FString OwnerLabel = Pawn->HasAuthority() ? TEXT("Host owner discovery") : TEXT("Client owner discovery");
            const FString PeerLabel = Pawn->HasAuthority() ? TEXT("Client owner discovery") : TEXT("Host owner discovery");
            const bool bOwnerLocalLabel = Presented.Contains(OwnerLabel);
            const bool bPeerPrivateLabelHidden = !Presented.Contains(PeerLabel);
            const bool bTextComplete = Presented.Contains(TEXT("Crafting reached level 2"))
                && Presented.Contains(TEXT("Gained 2 Wood")) && bOwnerLocalLabel;
            const bool bPassive = !Widget->IsFocusable() && Widget->GetVisibility() == ESlateVisibility::HitTestInvisible;
            UE_LOG(LogTemp, Display, TEXT("Notification combined layout: Complete=%d OwnerLocal=%d PeerPrivateHidden=%d Passive=%d Rows=%d Scale=%d Contrast=%d Motion=Static"),
                bTextComplete, bOwnerLocalLabel, bPeerPrivateLabelHidden, bPassive, Queue.GetRows().Num(), UKalmalaSettingsWidget::GetTextScalePercent(),
                UKalmalaSettingsWidget::GetContrastMode());
            FScreenshotRequest::RequestScreenshot(ReviewCaptureBasePath + TEXT("-combined.png"), true, false);
            ReviewCaptureStage = 1;
            ReviewCaptureElapsed = 0.0f;
            bReviewPriorMoveInputIgnored = Controller->IsMoveInputIgnored();
            Controller->SetIgnoreMoveInput(true);
        }
        else if (ReviewCaptureStage == 2 && ReviewCaptureElapsed >= 0.5f)
        {
            UE_LOG(LogTemp, Display, TEXT("Notification restored fixture: Visible=%d Rows=%d"),
                Widget->GetVisibility() == ESlateVisibility::HitTestInvisible, Queue.GetRows().Num());
            FScreenshotRequest::RequestScreenshot(ReviewCaptureBasePath + TEXT("-restored.png"), true, false);
            ReviewCaptureStage = 3;
        }
    }
#endif
}

void UKalmalaNotificationSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
    if (ReviewCaptureStage == 1)
    {
        if (APlayerController* Controller = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr)
            Controller->SetIgnoreMoveInput(bReviewPriorMoveInputIgnored);
    }
#endif
    if (Widget) Widget->RemoveFromParent(); Widget = nullptr;
    Queue.Reset(); OwnerPawn.Reset(); bOwnerBaselineAudited = false;
#if !UE_BUILD_SHIPPING
    bReviewFixtureInitialized = false;
    bReviewSettingsApplied = false;
    ReviewCaptureStage = 0;
    ReviewCaptureElapsed = 0.0f;
    ReviewCaptureBasePath.Reset();
#endif
    Super::Deinitialize();
}
