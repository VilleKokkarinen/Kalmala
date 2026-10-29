#include "KalmalaPlayerStepWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaUITheme.h"

const TCHAR* UKalmalaPlayerStepWidget::IdentityMessage(const bool bIdentityAvailable)
{
    return bIdentityAvailable
        ? TEXT("Saved player progress requires authentication by the server when you enter a world.")
        : TEXT("A player identity is not available in this menu. Saved player progress requires server authentication.");
}

void UKalmalaPlayerStepWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree || WidgetTree->RootWidget) return;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    Backdrop = WidgetTree->ConstructWidget<UBorder>();
    UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Backdrop);
    BackgroundSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    BackgroundSlot->SetOffsets(FMargin(0));
    Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    UCanvasPanelSlot* ScrollSlot = Canvas->AddChildToCanvas(Scroll);
    ScrollSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    ScrollSlot->SetOffsets(FMargin(32));
    Column = WidgetTree->ConstructWidget<USizeBox>();
    Column->SetWidthOverride(420.0f);
    if (UScrollBoxSlot* ScrollColumnSlot = Cast<UScrollBoxSlot>(Scroll->AddChild(Column)))
        ScrollColumnSlot->SetHorizontalAlignment(HAlign_Left);
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Column->SetContent(Content);
    Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(PlayerLabel()));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));
    IdentityStatus = WidgetTree->ConstructWidget<UTextBlock>();
    IdentityStatus->SetAutoWrapText(true);
    IdentityStatus->SetText(FText::FromString(IdentityMessage(false)));
    Content->AddChildToVerticalBox(IdentityStatus)->SetPadding(FMargin(0, 0, 0, 16));
    const auto AddAction = [this, Content](const TCHAR* Text)
    {
        USizeBox* Row = WidgetTree->ConstructWidget<USizeBox>();
        Row->SetMinDesiredHeight(48.0f);
        Content->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 0, 0, 8));
        UKalmalaThemedButton* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Text));
        Label->SetAutoWrapText(true);
        Label->SetJustification(ETextJustify::Center);
        Button->SetContent(Label);
        Row->SetContent(Button);
        return Button;
    };
    ContinueButton = AddAction(TEXT("Continue"));
    JoinButton = AddAction(TEXT("Join by address"));
    BackButton = AddAction(TEXT("Back"));
    ContinueButton->OnClicked.AddDynamic(this, &ThisClass::HandleContinue);
    JoinButton->OnClicked.AddDynamic(this, &ThisClass::HandleJoin);
    BackButton->OnClicked.AddDynamic(this, &ThisClass::HandleBack);
    Unavailable = WidgetTree->ConstructWidget<UTextBlock>();
    Unavailable->SetText(FText::FromString(TEXT("World selection is not available yet.")));
    Unavailable->SetAutoWrapText(true);
    Unavailable->SetVisibility(ESlateVisibility::Collapsed);
    Content->AddChildToVerticalBox(Unavailable)->SetPadding(FMargin(0, 8, 0, 0));
    RefreshTheme();
}

void UKalmalaPlayerStepWidget::NativeTick(const FGeometry& Geometry, const float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (Column) Column->SetWidthOverride(UKalmalaHomeWidget::ActionColumnWidth(Geometry.GetLocalSize().X));
    const APlayerController* Controller = GetOwningPlayer();
    const APlayerState* PlayerState = Controller ? Controller->GetPlayerState<APlayerState>() : nullptr;
    // The identity is only tested for availability; never copy it into presentation or a request.
    const bool bAvailable = PlayerState && PlayerState->GetUniqueId().IsValid();
    if (IdentityStatus && (!bIdentityObserved || bLastIdentityAvailable != bAvailable))
    {
        IdentityStatus->SetText(FText::FromString(IdentityMessage(bAvailable)));
        bLastIdentityAvailable = bAvailable;
        bIdentityObserved = true;
    }
    if (LastTextScale != UKalmalaSettingsWidget::GetTextScalePercent()
        || LastContrast != UKalmalaSettingsWidget::GetContrastMode()
        || bLastReducedMotion != UKalmalaSettingsWidget::IsReducedMotionEnabled()) RefreshTheme();
}

void UKalmalaPlayerStepWidget::RefreshTheme()
{
    const auto& Theme = FKalmalaUITheme::Get();
    LastTextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    LastContrast = UKalmalaSettingsWidget::GetContrastMode();
    bLastReducedMotion = UKalmalaSettingsWidget::IsReducedMotionEnabled();
    if (Backdrop) Theme.ApplySolidPanel(*Backdrop, LastContrast);
    if (Scroll) Theme.ApplyScroll(*Scroll, bLastReducedMotion);
    if (Title) Theme.ApplyText(*Title, Theme.EmphasisSize + 12, true, LastTextScale, LastContrast);
    if (IdentityStatus) Theme.ApplyText(*IdentityStatus, Theme.BodySize + 3, false, LastTextScale, LastContrast);
    if (Unavailable) Theme.ApplyText(*Unavailable, Theme.BodySize, false, LastTextScale, LastContrast);
    UKalmalaThemedButton* Buttons[] = {ContinueButton.Get(), JoinButton.Get(), BackButton.Get()};
    for (UKalmalaThemedButton* Button : Buttons)
    {
        if (!Button) continue;
        Theme.ApplyButton(*Button, LastContrast, bLastReducedMotion);
        if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
            Theme.ApplyText(*Label, Theme.BodySize + 8, false, LastTextScale, LastContrast);
    }
}

void UKalmalaPlayerStepWidget::FocusContinue()
{
    if (!ContinueButton || !GetOwningPlayer()) return;
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(ContinueButton->TakeWidget());
    Controller->SetInputMode(InputMode);
    ContinueButton->SetUserFocus(Controller);
    ContinueButton->SetKeyboardFocus();
}

FReply UKalmalaPlayerStepWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (!Event.IsRepeat() && (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    {
        HandleBack();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UKalmalaPlayerStepWidget::HandleBack() { BackRequested.Broadcast(); }
void UKalmalaPlayerStepWidget::HandleContinue()
{
    if (ContinueRequested.IsBound()) ContinueRequested.Broadcast();
    else if (Unavailable) Unavailable->SetVisibility(ESlateVisibility::Visible);
}
void UKalmalaPlayerStepWidget::HandleJoin() { JoinRequested.Broadcast(); }
