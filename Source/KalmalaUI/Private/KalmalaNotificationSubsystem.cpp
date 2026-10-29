#include "KalmalaNotificationSubsystem.h"
#include "KalmalaMenuContext.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaCharacter.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaUITheme.h"
#include "KalmalaCatalogueIconLibrary.h"
#include "KalmalaStatusIconLibrary.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaMinimapSubsystem.h"
#include "KalmalaMinimapWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
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
FName SkillIconItemId(EKalmalaSkill Skill)
{
    switch (Skill)
    {
    case EKalmalaSkill::Gathering: return TEXT("StoneKnife");
    case EKalmalaSkill::Woodcutting: return TEXT("StoneAxe");
    case EKalmalaSkill::Mining: return TEXT("StonePick");
    case EKalmalaSkill::Crafting: return TEXT("ConstructionHammer");
    case EKalmalaSkill::Cooking: return TEXT("CauldronKit");
    case EKalmalaSkill::Survival: return TEXT("BasicTorch");
    default: return NAME_None;
    }
}
FName NoticeIconItemId(const FKalmalaSkillNotice& Notice)
{
    switch (Notice.Kind)
    {
    case EKalmalaNoticeKind::ItemGain: return Notice.ItemId;
    case EKalmalaNoticeKind::Discovery: return TEXT("MendingScroll");
    case EKalmalaNoticeKind::Support: return NAME_None;
    case EKalmalaNoticeKind::Skill: return SkillIconItemId(Notice.Skill);
    default: return NAME_None;
    }
}
FString NoticeIconKey(const FKalmalaSkillNotice& Notice)
{
    return Notice.Kind == EKalmalaNoticeKind::Support
        ? FString::Printf(TEXT("Status:%s"), *Notice.StatusIconId.ToString())
        : FString::Printf(TEXT("Item:%s"), *NoticeIconItemId(Notice).ToString());
}
FName SupportStatusIconId(const UKalmalaSupportMagicComponent& Support)
{
    if (Support.GetFeedback() != EKalmalaSupportFeedback::Accepted)
        return TEXT("HearthShield");
    switch (Support.GetActiveEffect())
    {
    case EKalmalaSupportEffect::Mending: return TEXT("Mending");
    case EKalmalaSupportEffect::HearthShield: return TEXT("HearthShield");
    case EKalmalaSupportEffect::BearsVigor: return TEXT("BearsVigor");
    case EKalmalaSupportEffect::DeerCall: return TEXT("DeerCall");
    default: return TEXT("HearthShield");
    }
}
FString NoticeText(const FKalmalaSkillNotice& Notice)
{
    if (!Notice.ActionText.IsEmpty()) return Notice.ActionText;
    if (!Notice.ItemId.IsNone())
    {
        const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Notice.ItemId);
        return FString::Printf(TEXT("You gained %d %s"), Notice.Quantity, Item ? *Item->DisplayName : *Notice.ItemId.ToString());
    }
    if (!Notice.DiscoveryText.IsEmpty()) return Notice.DiscoveryText;
    return FString::Printf(TEXT("You leveled up %s to Level %d"), SkillName(Notice.Skill), Notice.Level);
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
    FString NextIcons;
    for (int32 Index = 0; Index < FMath::Min(Notices.Num(), FKalmalaSkillNoticeQueue::MaxRows); ++Index)
    {
        Next += NoticeText(Notices[Index]) + TEXT("\n");
        NextIcons += NoticeIconKey(Notices[Index]) + TEXT("\n");
    }
    if (Next == Presentation && NextIcons == IconPresentation && Scale == LastScale && Contrast == LastContrast) return;
    Presentation = Next; IconPresentation = NextIcons; LastScale = Scale; LastContrast = Contrast;
    Rows->ClearChildren();
    const auto& Theme = FKalmalaUITheme::Get();
    const FString NoImage; Theme.ApplyPanel(*Panel, Contrast, &NoImage);
    for (int32 Index = 0; Index < FMath::Min(Notices.Num(), FKalmalaSkillNoticeQueue::MaxRows); ++Index)
    {
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Box = WidgetTree->ConstructWidget<USizeBox>();
        Box->SetWidthOverride(28); Box->SetHeightOverride(28);
        auto* Icon = WidgetTree->ConstructWidget<UImage>();
        UTexture2D* Texture = Notices[Index].Kind == EKalmalaNoticeKind::Support
            ? FKalmalaStatusIconLibrary::LoadTexture(Notices[Index].StatusIconId)
            : FKalmalaCatalogueIconLibrary::LoadTexture(NoticeIconItemId(Notices[Index]));
        if (Texture != nullptr)
            Icon->SetBrushFromTexture(Texture, false);
        Icon->SetVisibility(Texture != nullptr ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    APlayerController* Controller = World && LocalPlayer ? LocalPlayer->GetPlayerController(World) : nullptr;
    if (!World || !World->IsGameWorld() || !LocalPlayer
        || (Controller && Controller->IsA<AKalmalaMenuPlayerController>()))
    {
#if !UE_BUILD_SHIPPING
        if (ReviewCaptureStage == 1)
            if (APlayerController* PreviousController = OwnerController.Get())
                PreviousController->SetIgnoreMoveInput(bReviewPriorMoveInputIgnored);
#endif
        if (Widget) Widget->RemoveFromParent();
        Widget = nullptr;
        Queue.Reset(); OwnerController.Reset(); OwnerPawn.Reset(); bOwnerBaselineAudited = false;
#if !UE_BUILD_SHIPPING
        bReviewFixtureInitialized = false;
        bReviewSettingsApplied = false;
        ReviewCaptureStage = 0;
        ReviewCaptureElapsed = 0.0f;
        ReviewCaptureBasePath.Reset();
#endif
        return;
    }
    if (OwnerController.Get() != Controller || (Controller == nullptr && Widget != nullptr))
    {
#if !UE_BUILD_SHIPPING
        if (ReviewCaptureStage == 1)
            if (APlayerController* PreviousController = OwnerController.Get())
                PreviousController->SetIgnoreMoveInput(bReviewPriorMoveInputIgnored);
#endif
        if (Widget) Widget->RemoveFromParent();
        Widget = nullptr;
        OwnerController = Controller;
        bOwnerBaselineAudited = false;
#if !UE_BUILD_SHIPPING
        bReviewFixtureInitialized = false;
        bReviewSettingsApplied = false;
        ReviewCaptureStage = 0;
        ReviewCaptureElapsed = 0.0f;
        ReviewCaptureBasePath.Reset();
#endif
    }
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
    const auto* Character = Cast<AKalmalaCharacter>(Pawn);
    const auto* Support = Character ? Character->GetSupportMagicComponent() : nullptr;
#if !UE_BUILD_SHIPPING
    const bool bSupportObserved = bReviewPresentationActive || (Support && Queue.ObserveSupport(
        Support->GetFeedbackSerial(), Support->GetFeedback(), FKalmalaUITheme::Get().NotificationLifetime,
        SupportStatusIconId(*Support)));
#else
    const bool bSupportObserved = Support && Queue.ObserveSupport(
        Support->GetFeedbackSerial(), Support->GetFeedback(), FKalmalaUITheme::Get().NotificationLifetime,
        SupportStatusIconId(*Support));
#endif

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaNotificationBaselineAudit"))
        && !bOwnerBaselineAudited && !bReviewPresentationActive && bSkillObserved && bGainsObserved
        && bDiscoveryObserved && bSupportObserved)
    {
        bOwnerBaselineAudited = true;
        const bool bSilent = Queue.GetRows().IsEmpty();
        UE_LOG(LogTemp, Display, TEXT("Notification owner baseline: Silent=%d Rows=%d Sources=4"),
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
        UE_LOG(LogTemp, Display, TEXT("Notification reconnect baseline: Silent=%d Rows=%d Sources=4"),
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
    const float LogicalWidth = Width / DPI;
    const float LogicalHeight = Height / DPI;
    const auto& Theme = FKalmalaUITheme::Get();
    const float FontSize = Theme.MakeFont(Theme.BodySize, false, UKalmalaSettingsWidget::GetTextScalePercent()).Size;
    const float PanelHeight = Queue.GetRows().Num() * (FontSize * 2 + 28) + Theme.PaddingY * 2;
    FVector2D Position = UKalmalaMinimapWidget::GetDefaultNotificationQueueViewportPosition();
    if (const ULocalPlayer* QueueLocalPlayer = GetLocalPlayer())
    {
        if (const UKalmalaMinimapSubsystem* MinimapSubsystem =
            QueueLocalPlayer->GetSubsystem<UKalmalaMinimapSubsystem>())
        {
            if (const UKalmalaMinimapWidget* MinimapWidget = MinimapSubsystem->GetMinimapWidget())
                Position = MinimapWidget->GetNotificationQueueViewportPosition();
        }
    }
    if (LogicalWidth <= 48.0f || Position.Y + PanelHeight > LogicalHeight - UKalmalaMinimapWidget::ViewportInset)
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    Widget->SetDesiredSizeInViewport(FVector2D(FMath::Max(1.f, FMath::Min(320.f, LogicalWidth - 48.f)), PanelHeight));
    Widget->SetPositionInViewport(Position, false);
    Widget->SetAlignmentInViewport(FVector2D(1.0f, 0.0f));
    Widget->SetAnchorsInViewport(FAnchors(1.0f, 0.0f));
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
            const bool bTextComplete = Presented.Contains(TEXT("You leveled up Crafting to Level 2"))
                && Presented.Contains(TEXT("You gained 2 Wood")) && bOwnerLocalLabel;
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
        if (APlayerController* PreviousController = OwnerController.Get())
            PreviousController->SetIgnoreMoveInput(bReviewPriorMoveInputIgnored);
    }
#endif
    if (Widget) Widget->RemoveFromParent(); Widget = nullptr;
    Queue.Reset(); OwnerController.Reset(); OwnerPawn.Reset(); bOwnerBaselineAudited = false;
#if !UE_BUILD_SHIPPING
    bReviewFixtureInitialized = false;
    bReviewSettingsApplied = false;
    ReviewCaptureStage = 0;
    ReviewCaptureElapsed = 0.0f;
    ReviewCaptureBasePath.Reset();
#endif
    Super::Deinitialize();
}
