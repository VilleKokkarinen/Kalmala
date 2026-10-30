#include "KalmalaSaveInformationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "KalmalaMenuLandscape.h"
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

void UKalmalaSaveInformationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree || WidgetTree->RootWidget) return;

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UKalmalaMenuLandscape::AddToCanvas(*WidgetTree, *Canvas);
    Backdrop = WidgetTree->ConstructWidget<UBorder>();
    UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    BackdropSlot->SetOffsets(FMargin(0));

    UKalmalaMenuLandscape::FitPanel(Backdrop, 1280.f);

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
    Title->SetText(FText::FromString(TEXT("Save information")));
    Title->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 16));

    InformationText = WidgetTree->ConstructWidget<UTextBlock>();
    InformationText->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(InformationText)->SetPadding(FMargin(0, 0, 0, 16));

    USizeBox* BackRow = WidgetTree->ConstructWidget<USizeBox>();
    BackRow->SetMinDesiredHeight(48.0f);
    Content->AddChildToVerticalBox(BackRow);
    BackButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>();
    BackLabel->SetText(FText::FromString(TEXT("Back")));
    BackLabel->SetAutoWrapText(true);
    BackLabel->SetJustification(ETextJustify::Center);
    BackButton->SetContent(BackLabel);
    BackRow->SetContent(BackButton);
    BackButton->OnClicked.AddDynamic(this, &ThisClass::HandleBack);
    RefreshTheme();
}

void UKalmalaSaveInformationWidget::NativeTick(const FGeometry& Geometry, const float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    UKalmalaMenuLandscape::FitPanel(Backdrop, Geometry.GetLocalSize().X);
    if (Column) Column->SetWidthOverride(UKalmalaHomeWidget::ActionColumnWidth(Geometry.GetLocalSize().X));
    if (LastTextScale != UKalmalaSettingsWidget::GetTextScalePercent()
        || LastContrast != UKalmalaSettingsWidget::GetContrastMode()
        || bLastReducedMotion != UKalmalaSettingsWidget::IsReducedMotionEnabled()) RefreshTheme();
}

void UKalmalaSaveInformationWidget::RefreshTheme()
{
    const auto& Theme = FKalmalaUITheme::Get();
    LastTextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    LastContrast = UKalmalaSettingsWidget::GetContrastMode();
    bLastReducedMotion = UKalmalaSettingsWidget::IsReducedMotionEnabled();
    if (Backdrop) Theme.ApplySolidPanel(*Backdrop, LastContrast);
    if (Scroll) Theme.ApplyScroll(*Scroll, bLastReducedMotion);
    if (Title) Theme.ApplyText(*Title, Theme.EmphasisSize + 12, true, LastTextScale, LastContrast);
    if (InformationText) Theme.ApplyText(*InformationText, Theme.BodySize + 3, false, LastTextScale, LastContrast);
    if (BackButton)
    {
        Theme.ApplyButton(*BackButton, LastContrast, bLastReducedMotion);
        if (UTextBlock* Label = Cast<UTextBlock>(BackButton->GetContent()))
            Theme.ApplyText(*Label, Theme.BodySize + 8, false, LastTextScale, LastContrast);
    }
}

void UKalmalaSaveInformationWidget::SetInformation(const FText& Information)
{
    if (InformationText) InformationText->SetText(Information);
}

void UKalmalaSaveInformationWidget::FocusBack()
{
    if (!BackButton || !GetOwningPlayer()) return;
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(BackButton->TakeWidget());
    Controller->SetInputMode(InputMode);
    BackButton->SetUserFocus(Controller);
    BackButton->SetKeyboardFocus();
}

FReply UKalmalaSaveInformationWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (!Event.IsRepeat() && (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    {
        HandleBack();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UKalmalaSaveInformationWidget::HandleBack()
{
    BackRequested.Broadcast();
}
