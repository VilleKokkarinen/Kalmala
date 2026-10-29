#include "KalmalaJoinAddressWidget.h"

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
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "KalmalaHomeWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaUITheme.h"

void UKalmalaJoinAddressWidget::NativeOnInitialized()
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
    if (UScrollBoxSlot* Slot = Cast<UScrollBoxSlot>(Scroll->AddChild(Column)))
        Slot->SetHorizontalAlignment(HAlign_Left);
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Column->SetContent(Content);

    Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("Join by address")));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));

    Instructions = WidgetTree->ConstructWidget<UTextBlock>();
    Instructions->SetText(FText::FromString(TEXT(
        "Enter an IPv4 address or DNS hostname. Add :port when needed; otherwise the configured gameplay port is used. "
        "IPv6 addresses, URLs and join codes are not supported.")));
    Instructions->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Instructions)->SetPadding(FMargin(0, 0, 0, 16));

    AddressInput = WidgetTree->ConstructWidget<UEditableTextBox>();
    AddressInput->SetHintText(FText::FromString(TEXT("IP address or hostname[:port]")));
    AddressInput->OnTextCommitted.AddDynamic(this, &ThisClass::HandleAddressCommitted);
    Content->AddChildToVerticalBox(AddressInput)->SetPadding(FMargin(0, 0, 0, 12));

    const auto AddAction = [this, Content](const TCHAR* LabelText)
    {
        USizeBox* Row = WidgetTree->ConstructWidget<USizeBox>();
        Row->SetMinDesiredHeight(48.0f);
        Content->AddChildToVerticalBox(Row)->SetPadding(FMargin(0, 0, 0, 8));
        UKalmalaThemedButton* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(LabelText));
        Label->SetAutoWrapText(true);
        Label->SetJustification(ETextJustify::Center);
        Button->SetContent(Label);
        Row->SetContent(Button);
        return Button;
    };

    JoinButton = AddAction(TEXT("Join"));
    BackButton = AddAction(TEXT("Back to player"));
    JoinButton->OnClicked.AddDynamic(this, &ThisClass::HandleJoin);
    BackButton->OnClicked.AddDynamic(this, &ThisClass::HandleBack);

    Message = WidgetTree->ConstructWidget<UTextBlock>();
    Message->SetAutoWrapText(true);
    Message->SetVisibility(ESlateVisibility::Collapsed);
    Content->AddChildToVerticalBox(Message)->SetPadding(FMargin(0, 8, 0, 0));
    RefreshTheme();
}

void UKalmalaJoinAddressWidget::NativeTick(const FGeometry& Geometry, const float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (Column) Column->SetWidthOverride(UKalmalaHomeWidget::ActionColumnWidth(Geometry.GetLocalSize().X));
    if (LastTextScale != UKalmalaSettingsWidget::GetTextScalePercent()
        || LastContrast != UKalmalaSettingsWidget::GetContrastMode()
        || bLastReducedMotion != UKalmalaSettingsWidget::IsReducedMotionEnabled()) RefreshTheme();
}

void UKalmalaJoinAddressWidget::RefreshTheme()
{
    const auto& Theme = FKalmalaUITheme::Get();
    LastTextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    LastContrast = UKalmalaSettingsWidget::GetContrastMode();
    bLastReducedMotion = UKalmalaSettingsWidget::IsReducedMotionEnabled();
    if (Backdrop) Theme.ApplySolidPanel(*Backdrop, LastContrast);
    if (Scroll) Theme.ApplyScroll(*Scroll, bLastReducedMotion);
    if (Title) Theme.ApplyText(*Title, Theme.EmphasisSize + 12, true, LastTextScale, LastContrast);
    if (Instructions) Theme.ApplyText(*Instructions, Theme.BodySize + 3, false, LastTextScale, LastContrast);
    if (Message) Theme.ApplyText(*Message, Theme.BodySize + 3, false, LastTextScale, LastContrast);
    if (AddressInput)
    {
        FEditableTextBoxStyle InputStyle = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
        InputStyle.SetFont(Theme.MakeFont(Theme.BodySize + 4, false, LastTextScale));
        InputStyle.SetForegroundColor(FSlateColor(Theme.TextColor(false, LastContrast)));
        InputStyle.SetBackgroundColor(FSlateColor(LastContrast > 0
            ? FLinearColor::Black : FLinearColor(.02f, .025f, .03f, 1.f)));
        AddressInput->SetWidgetStyle(InputStyle);
    }

    UKalmalaThemedButton* Buttons[] = {JoinButton.Get(), BackButton.Get()};
    for (UKalmalaThemedButton* Button : Buttons)
    {
        if (!Button) continue;
        Theme.ApplyButton(*Button, LastContrast, bLastReducedMotion);
        if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
            Theme.ApplyText(*Label, Theme.BodySize + 8, false, LastTextScale, LastContrast);
    }
}

void UKalmalaJoinAddressWidget::FocusAddress()
{
    if (!AddressInput || !GetOwningPlayer()) return;
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(AddressInput->TakeWidget());
    Controller->SetInputMode(InputMode);
    AddressInput->SetUserFocus(Controller);
    AddressInput->SetKeyboardFocus();
}

void UKalmalaJoinAddressWidget::SetJoinEnabled(const bool bEnabled)
{
    if (JoinButton) JoinButton->SetIsEnabled(bEnabled);
}

void UKalmalaJoinAddressWidget::ShowMessage(const FText& Text)
{
    if (!Message) return;
    Message->SetText(Text);
    Message->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

FReply UKalmalaJoinAddressWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (!Event.IsRepeat() && (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    {
        HandleBack();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UKalmalaJoinAddressWidget::HandleBack()
{
    BackRequested.Broadcast();
}

void UKalmalaJoinAddressWidget::HandleJoin()
{
    if (!JoinButton || !JoinButton->GetIsEnabled() || !AddressInput) return;
    JoinRequested.Broadcast(AddressInput->GetText().ToString());
}

void UKalmalaJoinAddressWidget::HandleAddressCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter) HandleJoin();
}
