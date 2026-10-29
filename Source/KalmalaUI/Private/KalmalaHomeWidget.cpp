#include "KalmalaHomeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaUITheme.h"
#include "Misc/App.h"

float UKalmalaHomeWidget::ActionColumnWidth(const float LogicalViewportWidth)
{
    return FMath::IsFinite(LogicalViewportWidth)
        ? FMath::Clamp(LogicalViewportWidth - 64.0f, 0.0f, 420.0f) : 0.0f;
}

bool UKalmalaHomeWidget::TryGetAction(const int32 Index, EKalmalaHomeAction& OutAction)
{
    static constexpr EKalmalaHomeAction Actions[] = { EKalmalaHomeAction::Play,
        EKalmalaHomeAction::Options, EKalmalaHomeAction::SaveInformation,
        EKalmalaHomeAction::Credits, EKalmalaHomeAction::Exit };
    if (Index < 0 || Index >= UE_ARRAY_COUNT(Actions)) return false;
    OutAction = Actions[Index];
    return true;
}

const TCHAR* UKalmalaHomeWidget::GetActionLabel(const EKalmalaHomeAction Action)
{
    switch (Action)
    {
    case EKalmalaHomeAction::Play: return TEXT("Play");
    case EKalmalaHomeAction::Options: return TEXT("Options");
    case EKalmalaHomeAction::SaveInformation: return TEXT("Save information");
    case EKalmalaHomeAction::Credits: return TEXT("Credits");
    case EKalmalaHomeAction::Exit: return TEXT("Exit");
    default: return TEXT("");
    }
}

void UKalmalaHomeWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree || WidgetTree->RootWidget) return;
    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HomeCanvas"));
    WidgetTree->RootWidget = Canvas;
    Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HomeBackdrop"));
    FKalmalaUITheme::Get().ApplySolidPanel(*Backdrop, UKalmalaSettingsWidget::GetContrastMode());
    UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop);
    BackdropSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    BackdropSlot->SetOffsets(FMargin(0));

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HomeContent"));
    UCanvasPanelSlot* ContentSlot = Canvas->AddChildToCanvas(Content);
    ContentSlot->SetAnchors(FAnchors(0, 0, 1, 1));
    ContentSlot->SetOffsets(FMargin(32));

    Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HomeTitle"));
    Title->SetText(FText::FromString(TEXT("Kalmala")));
    Title->SetAutoWrapText(true);
    UVerticalBoxSlot* TitleSlot = Content->AddChildToVerticalBox(Title);
    TitleSlot->SetPadding(FMargin(0, 0, 0, 16));

    ActionsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("HomeActionsScroll"));
    FKalmalaUITheme::Get().ApplyScroll(*ActionsScroll, UKalmalaSettingsWidget::IsReducedMotionEnabled());
    UVerticalBoxSlot* ScrollSlot = Content->AddChildToVerticalBox(ActionsScroll);
    ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ActionColumn = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HomeActionColumn"));
    ActionColumn->SetWidthOverride(420.0f);
    UScrollBoxSlot* ColumnSlot = Cast<UScrollBoxSlot>(ActionsScroll->AddChild(ActionColumn));
    if (ColumnSlot) ColumnSlot->SetHorizontalAlignment(HAlign_Left);
    UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HomeActionRows"));
    ActionColumn->SetContent(Rows);

    constexpr int32 ActionCount = 5;
    for (int32 Index = 0; Index < ActionCount; ++Index)
    {
        USizeBox* Row = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Row->SetMinDesiredHeight(48.0f);
        UVerticalBoxSlot* RowSlot = Rows->AddChildToVerticalBox(Row);
        RowSlot->SetPadding(FMargin(0, 0, 0, Index + 1 < ActionCount ? 8.0f : 0.0f));
        UKalmalaThemedButton* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>(UKalmalaThemedButton::StaticClass());
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        EKalmalaHomeAction Action;
        if (!TryGetAction(Index, Action)) return;
        Label->SetText(FText::FromString(GetActionLabel(Action)));
        Label->SetAutoWrapText(true);
        Label->SetJustification(ETextJustify::Center);
        Button->SetContent(Label);
        Row->SetContent(Button);
        Buttons.Add(Button);
    }
    Buttons[0]->OnClicked.AddDynamic(this, &ThisClass::HandlePlay);
    Buttons[1]->OnClicked.AddDynamic(this, &ThisClass::HandleOptions);
    Buttons[2]->OnClicked.AddDynamic(this, &ThisClass::HandleSaveInformation);
    Buttons[3]->OnClicked.AddDynamic(this, &ThisClass::HandleCredits);
    Buttons[4]->OnClicked.AddDynamic(this, &ThisClass::HandleExit);

    UnavailableMessage = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HomeUnavailable"));
    UnavailableMessage->SetText(FText::FromString(TEXT("This option is not available yet.")));
    UnavailableMessage->SetAutoWrapText(true);
    UnavailableMessage->SetVisibility(ESlateVisibility::Collapsed);
    UVerticalBoxSlot* MessageSlot = Rows->AddChildToVerticalBox(UnavailableMessage);
    MessageSlot->SetPadding(FMargin(0, 12, 0, 0));

    Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HomeBuildVersion"));
    const FString BuildVersion(FApp::GetBuildVersion());
    const FString FooterText = BuildVersion.IsEmpty() ? FString(TEXT("Build version unavailable"))
        : FString::Printf(TEXT("Build %s"), *BuildVersion);
    Footer->SetText(FText::FromString(FooterText));
    Footer->SetAutoWrapText(true);
    UVerticalBoxSlot* FooterSlot = Content->AddChildToVerticalBox(Footer);
    FooterSlot->SetPadding(FMargin(0, 16, 0, 0));
    RefreshTheme();
}

void UKalmalaHomeWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (ActionColumn) ActionColumn->SetWidthOverride(ActionColumnWidth(Geometry.GetLocalSize().X));
    if (LastTextScale != UKalmalaSettingsWidget::GetTextScalePercent()
        || LastContrast != UKalmalaSettingsWidget::GetContrastMode()
        || bLastReducedMotion != UKalmalaSettingsWidget::IsReducedMotionEnabled()) RefreshTheme();
}

void UKalmalaHomeWidget::RefreshTheme()
{
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    LastTextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    LastContrast = UKalmalaSettingsWidget::GetContrastMode();
    bLastReducedMotion = UKalmalaSettingsWidget::IsReducedMotionEnabled();
    if (Backdrop) Theme.ApplySolidPanel(*Backdrop, LastContrast);
    if (ActionsScroll) Theme.ApplyScroll(*ActionsScroll, bLastReducedMotion);
    if (Title) Theme.ApplyText(*Title, Theme.EmphasisSize + 12, true, LastTextScale, LastContrast);
    if (Footer) Theme.ApplyText(*Footer, Theme.BodySize, false, LastTextScale, LastContrast);
    if (UnavailableMessage) Theme.ApplyText(*UnavailableMessage, Theme.BodySize, false, LastTextScale, LastContrast);
    for (UKalmalaThemedButton* Button : Buttons)
    {
        if (!Button) continue;
        Theme.ApplyButton(*Button, LastContrast, bLastReducedMotion);
        if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
            Theme.ApplyText(*Label, Theme.BodySize + 8, false, LastTextScale, LastContrast);
    }
}

void UKalmalaHomeWidget::FocusPlay()
{
    if (Buttons.IsEmpty() || !Buttons[0] || !GetOwningPlayer()) return;
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(Buttons[0]->TakeWidget());
    Controller->SetInputMode(InputMode);
    Buttons[0]->SetUserFocus(Controller);
    Buttons[0]->SetKeyboardFocus();
}

void UKalmalaHomeWidget::Request(const EKalmalaHomeAction Action)
{
    if (ActionRequested.IsBound())
    {
        if (UnavailableMessage) UnavailableMessage->SetVisibility(ESlateVisibility::Collapsed);
        ActionRequested.Broadcast(Action);
    }
    else ShowUnavailable();
}

void UKalmalaHomeWidget::ShowUnavailable()
{
    if (UnavailableMessage) UnavailableMessage->SetVisibility(ESlateVisibility::Visible);
}
void UKalmalaHomeWidget::HandlePlay() { Request(EKalmalaHomeAction::Play); }
void UKalmalaHomeWidget::HandleOptions() { Request(EKalmalaHomeAction::Options); }
void UKalmalaHomeWidget::HandleSaveInformation() { Request(EKalmalaHomeAction::SaveInformation); }
void UKalmalaHomeWidget::HandleCredits() { Request(EKalmalaHomeAction::Credits); }
void UKalmalaHomeWidget::HandleExit() { Request(EKalmalaHomeAction::Exit); }
