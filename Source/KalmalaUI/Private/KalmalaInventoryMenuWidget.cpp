#include "KalmalaInventoryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"

void UKalmalaInventoryMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);

    if (WidgetTree == nullptr) return;

    UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;

    UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>();
    Scrim->SetBrushColor(FLinearColor(0.008f, 0.012f, 0.014f, 0.72f));
    UOverlaySlot* ScrimSlot = Root->AddChildToOverlay(Scrim);
    ScrimSlot->SetHorizontalAlignment(HAlign_Fill);
    ScrimSlot->SetVerticalAlignment(VAlign_Fill);

    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    const int32 TextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
    Theme.ApplyPanel(*Panel, Contrast, &Theme.InventoryPanelImage);
    Panel->SetPadding(FMargin(Theme.PaddingX * 1.5f, Theme.PaddingY * 1.5f));

    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
    Heading->SetText(FText::FromString(TEXT("Inventory")));
    Theme.ApplyText(*Heading, Theme.EmphasisSize, true, TextScale, Contrast);
    Content->AddChildToVerticalBox(Heading);

    UTextBlock* Section = WidgetTree->ConstructWidget<UTextBlock>();
    Section->SetText(FText::FromString(TEXT("Pack and carried equipment")));
    Theme.ApplyText(*Section, Theme.BodySize, false, TextScale, Contrast);
    Content->AddChildToVerticalBox(Section);

    Panel->SetContent(Content);
    USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
    PanelSize->SetWidthOverride(620.0f);
    PanelSize->SetHeightOverride(260.0f);
    PanelSize->SetContent(Panel);

    UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSize);
    PanelSlot->SetHorizontalAlignment(HAlign_Center);
    PanelSlot->SetVerticalAlignment(VAlign_Center);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaInventoryMenuWidget::Open()
{
    if (bMenuOpen) return;
    APlayerController* Controller = GetOwningPlayer();
    if (Controller == nullptr || !Controller->IsLocalController()) return;

    bPreviousCursorVisibility = Controller->bShowMouseCursor;
    bAcquiredMoveIgnore = !Controller->IsMoveInputIgnored();
    bAcquiredLookIgnore = !Controller->IsLookInputIgnored();
    if (bAcquiredMoveIgnore) Controller->SetIgnoreMoveInput(true);
    if (bAcquiredLookIgnore) Controller->SetIgnoreLookInput(true);
    Controller->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    Controller->SetInputMode(InputMode);

    bMenuOpen = true;
    SetVisibility(ESlateVisibility::Visible);
    SetUserFocus(Controller);
    SetKeyboardFocus();
}

void UKalmalaInventoryMenuWidget::Close()
{
    if (!bMenuOpen) return;
    bMenuOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);

    if (APlayerController* Controller = GetOwningPlayer())
    {
        if (bAcquiredMoveIgnore) Controller->SetIgnoreMoveInput(false);
        if (bAcquiredLookIgnore) Controller->SetIgnoreLookInput(false);
        Controller->bShowMouseCursor = bPreviousCursorVisibility;
        Controller->SetInputMode(FInputModeGameOnly());
    }
    bAcquiredMoveIgnore = false;
    bAcquiredLookIgnore = false;
}

bool UKalmalaInventoryMenuWidget::HasTextEntryFocus() const
{
    if (!FSlateApplication::IsInitialized()) return false;

    const TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    return FocusedWidget.IsValid()
        && FocusedWidget->GetTypeAsString().Contains(TEXT("EditableText"), ESearchCase::CaseSensitive);
}

FReply UKalmalaInventoryMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (bMenuOpen && !HasTextEntryFocus() && Event.GetKey() == EKeys::Gamepad_FaceButton_Right)
    {
        Close();
        return FReply::Handled();
    }

    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
