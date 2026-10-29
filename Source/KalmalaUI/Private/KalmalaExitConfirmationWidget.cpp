#include "KalmalaExitConfirmationWidget.h"

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
#include "InputCoreTypes.h"
#include "Input/Events.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaUITheme.h"

void UKalmalaExitConfirmationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree || WidgetTree->RootWidget) return;

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    Backdrop = WidgetTree->ConstructWidget<UBorder>();
    UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    BackdropSlot->SetOffsets(FMargin(0));

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
    Title->SetText(FText::FromString(TEXT("Exit Kalmala?")));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));

    Message = WidgetTree->ConstructWidget<UTextBlock>();
    Message->SetText(FText::FromString(TEXT("Are you sure you want to close the game?")));
    Message->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Message)->SetPadding(FMargin(0, 0, 0, 16));

    const auto AddAction = [this, Content](const TCHAR* Label)
    {
        USizeBox* Row = WidgetTree->ConstructWidget<USizeBox>();
        Row->SetMinDesiredHeight(48.0f);
        Content->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 0, 0, 8));
        UKalmalaThemedButton* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
        ButtonLabel->SetText(FText::FromString(Label));
        ButtonLabel->SetAutoWrapText(true);
        ButtonLabel->SetJustification(ETextJustify::Center);
        Button->SetContent(ButtonLabel);
        Row->SetContent(Button);
        return Button;
    };
    CancelButton = AddAction(TEXT("Cancel"));
    ExitButton = AddAction(TEXT("Exit"));
    CancelButton->OnClicked.AddDynamic(this, &ThisClass::HandleCancel);
    ExitButton->OnClicked.AddDynamic(this, &ThisClass::HandleExit);
    RefreshTheme();
}

void UKalmalaExitConfirmationWidget::NativeTick(const FGeometry& Geometry, const float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (Column) Column->SetWidthOverride(UKalmalaHomeWidget::ActionColumnWidth(Geometry.GetLocalSize().X));
    if (LastTextScale != UKalmalaSettingsWidget::GetTextScalePercent()
        || LastContrast != UKalmalaSettingsWidget::GetContrastMode()
        || bLastReducedMotion != UKalmalaSettingsWidget::IsReducedMotionEnabled()) RefreshTheme();
}

void UKalmalaExitConfirmationWidget::RefreshTheme()
{
    const auto& Theme = FKalmalaUITheme::Get();
    LastTextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    LastContrast = UKalmalaSettingsWidget::GetContrastMode();
    bLastReducedMotion = UKalmalaSettingsWidget::IsReducedMotionEnabled();
    if (Backdrop) Theme.ApplySolidPanel(*Backdrop, LastContrast);
    if (Scroll) Theme.ApplyScroll(*Scroll, bLastReducedMotion);
    if (Title) Theme.ApplyText(*Title, Theme.EmphasisSize + 12, true, LastTextScale, LastContrast);
    if (Message) Theme.ApplyText(*Message, Theme.BodySize + 3, false, LastTextScale, LastContrast);
    UKalmalaThemedButton* Buttons[] = {CancelButton.Get(), ExitButton.Get()};
    for (UKalmalaThemedButton* Button : Buttons)
    {
        if (!Button) continue;
        Theme.ApplyButton(*Button, LastContrast, bLastReducedMotion);
        if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
            Theme.ApplyText(*Label, Theme.BodySize + 8, false, LastTextScale, LastContrast);
    }
}

void UKalmalaExitConfirmationWidget::FocusCancel()
{
    if (!CancelButton || !GetOwningPlayer()) return;
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(CancelButton->TakeWidget());
    Controller->SetInputMode(InputMode);
    CancelButton->SetUserFocus(Controller);
    CancelButton->SetKeyboardFocus();
}

FReply UKalmalaExitConfirmationWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.IsRepeat()) return Super::NativeOnKeyDown(Geometry, Event);
    const FKey Key = Event.GetKey();
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
    {
        HandleCancel();
        return FReply::Handled();
    }
    if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        if (CancelButton && CancelButton->HasAnyUserFocus())
        {
            HandleCancel();
            return FReply::Handled();
        }
        if (ExitButton && ExitButton->HasAnyUserFocus())
        {
            HandleExit();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UKalmalaExitConfirmationWidget::HandleCancel()
{
    CancelRequested.Broadcast();
}

void UKalmalaExitConfirmationWidget::HandleExit()
{
    ExitAccepted.Broadcast();
}
