#include "KalmalaInventoryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
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
    Section->SetText(FText::FromString(TEXT("Pack contents")));
    Theme.ApplyText(*Section, Theme.BodySize, false, TextScale, Contrast);
    Content->AddChildToVerticalBox(Section);

    PackStateText = WidgetTree->ConstructWidget<UTextBlock>();
    PackStateText->SetText(FText::FromString(TEXT("Waiting for your pack.")));
    Theme.ApplyText(*PackStateText, Theme.BodySize, false, TextScale, Contrast);
    Content->AddChildToVerticalBox(PackStateText);

    UHorizontalBox* SelectionControls = WidgetTree->ConstructWidget<UHorizontalBox>();
    PreviousItemButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* PreviousLabel = WidgetTree->ConstructWidget<UTextBlock>();
    PreviousLabel->SetText(FText::FromString(TEXT("Previous item")));
    PreviousItemButton->SetContent(PreviousLabel);
    Theme.ApplyButton(*PreviousItemButton, Contrast);
    Theme.ApplyText(*PreviousLabel, Theme.BodySize, false, TextScale, Contrast);
    SelectionControls->AddChildToHorizontalBox(PreviousItemButton)->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));

    SelectedItemText = WidgetTree->ConstructWidget<UTextBlock>();
    SelectedItemText->SetJustification(ETextJustify::Center);
    SelectedItemText->SetAutoWrapText(true);
    Theme.ApplyText(*SelectedItemText, Theme.BodySize, true, TextScale, Contrast);
    SelectionControls->AddChildToHorizontalBox(SelectedItemText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    NextItemButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* NextLabel = WidgetTree->ConstructWidget<UTextBlock>();
    NextLabel->SetText(FText::FromString(TEXT("Next item")));
    NextItemButton->SetContent(NextLabel);
    Theme.ApplyButton(*NextItemButton, Contrast);
    Theme.ApplyText(*NextLabel, Theme.BodySize, false, TextScale, Contrast);
    SelectionControls->AddChildToHorizontalBox(NextItemButton)->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
    PreviousItemButton->OnClicked.AddDynamic(this, &ThisClass::SelectPreviousItem);
    NextItemButton->OnClicked.AddDynamic(this, &ThisClass::SelectNextItem);
    Content->AddChildToVerticalBox(SelectionControls);

    UHorizontalBox* PackAndDetail = WidgetTree->ConstructWidget<UHorizontalBox>();
    UScrollBox* PackScroll = WidgetTree->ConstructWidget<UScrollBox>();
    Theme.ApplyScroll(*PackScroll);
    PackRowsView = WidgetTree->ConstructWidget<UKalmalaCatalogueRowsWidget>();
    PackScroll->AddChild(PackRowsView);
    PackAndDetail->AddChildToHorizontalBox(PackScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ItemDetailView = WidgetTree->ConstructWidget<UKalmalaItemDetailWidget>();
    PackAndDetail->AddChildToHorizontalBox(ItemDetailView)->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
    Content->AddChildToVerticalBox(PackAndDetail)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    Panel->SetContent(Content);
    USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
    PanelSize->SetWidthOverride(640.0f);
    PanelSize->SetHeightOverride(560.0f);
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

    RefreshOwnerPack();

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

void UKalmalaInventoryMenuWidget::RefreshOwnerPack()
{
    APlayerController* Controller = GetOwningPlayer();
    if (Controller == nullptr || !Controller->IsLocalController() || PackRowsView == nullptr) return;

    TArray<FKalmalaCatalogueRow> PackRows;
    APawn* OwnerPawn = Controller->GetPawn();
    const UKalmalaInventoryComponent* Inventory = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (Inventory != nullptr)
    {
        const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
        for (const FKalmalaInventoryStack& Stack : Inventory->GetStacks())
        {
            if (Stack.ItemId.IsNone() || Stack.Quantity <= 0) continue;
            const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(Stack.ItemId) : nullptr;
            PackRows.Add({ Stack.ItemId, Definition ? Definition->DisplayName : Stack.ItemId.ToString(),
                FString::Printf(TEXT("× %d"), Stack.Quantity), false });
        }
    }

    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    ApplyPackRows(MoveTemp(PackRows), Inventory != nullptr, TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::ApplyPackRows(TArray<FKalmalaCatalogueRow>&& Rows,
    const bool bInventoryAvailable, const int32 TextScale, const int32 Contrast)
{
    const FName PreviouslySelected = OwnerPackRows.IsValidIndex(SelectedPackIndex)
        ? OwnerPackRows[SelectedPackIndex].Id : NAME_None;
    OwnerPackRows = MoveTemp(Rows);
    SelectedPackIndex = OwnerPackRows.IndexOfByPredicate([PreviouslySelected](const FKalmalaCatalogueRow& Row)
    {
        return !PreviouslySelected.IsNone() && Row.Id == PreviouslySelected;
    });
    if (SelectedPackIndex == INDEX_NONE && !OwnerPackRows.IsEmpty()) SelectedPackIndex = 0;

    FString State;
    if (!bInventoryAvailable) State = TEXT("Waiting for your pack.");
    else if (OwnerPackRows.IsEmpty()) State = TEXT("Your pack is empty.");
    else State = FString::Printf(TEXT("Your pack has %d of %d slots filled."),
        FMath::Min(OwnerPackRows.Num(), UKalmalaInventoryComponent::MaxSlots), UKalmalaInventoryComponent::MaxSlots);
    if (PackStateText->GetText().ToString() != State) PackStateText->SetText(FText::FromString(State));

    RefreshSelectionPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RefreshSelectionPresentation(const int32 TextScale, const int32 Contrast)
{
    if (PackRowsView == nullptr || ItemDetailView == nullptr || SelectedItemText == nullptr
        || PackStateText == nullptr || PreviousItemButton == nullptr || NextItemButton == nullptr) return;

    if (LastTextScalePercent != TextScale || LastContrastMode != Contrast)
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        Theme.ApplyText(*PackStateText, Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*SelectedItemText, Theme.BodySize, true, TextScale, Contrast);
        Theme.ApplyButton(*PreviousItemButton, Contrast);
        Theme.ApplyButton(*NextItemButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(PreviousItemButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(NextItemButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        LastTextScalePercent = TextScale;
        LastContrastMode = Contrast;
    }

    const bool bHasSelection = OwnerPackRows.IsValidIndex(SelectedPackIndex);
    const FName SelectedItem = bHasSelection ? OwnerPackRows[SelectedPackIndex].Id : NAME_None;
    PackRowsView->SetRows(OwnerPackRows, UKalmalaInventoryComponent::MaxSlots, TextScale, Contrast, SelectedItem);
    PreviousItemButton->SetIsEnabled(bHasSelection);
    NextItemButton->SetIsEnabled(bHasSelection);

    const FString SelectionText = bHasSelection
        ? FString::Printf(TEXT("Selected: %s — use arrows or D-pad to change"), *OwnerPackRows[SelectedPackIndex].Name)
        : TEXT("No item selected.");
    if (SelectedItemText->GetText().ToString() != SelectionText)
        SelectedItemText->SetText(FText::FromString(SelectionText));

    if (bHasSelection)
    {
        const FKalmalaCatalogueRow& Row = OwnerPackRows[SelectedPackIndex];
        const FString DetailKey = FString::Printf(TEXT("%s|%s|%s|%d|%d"), *Row.Id.ToString(), *Row.Name,
            *Row.Detail, TextScale, Contrast);
        if (DetailKey != LastSelectedDetailKey)
        {
            ItemDetailView->SetItem(Row.Id, Row.Name, Row.Detail, TextScale, Contrast);
            LastSelectedDetailKey = DetailKey;
        }
        ItemDetailView->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        LastSelectedDetailKey.Reset();
        ItemDetailView->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UKalmalaInventoryMenuWidget::StepSelection(const int32 Direction)
{
    if (OwnerPackRows.IsEmpty()) return;
    const int32 NumRows = OwnerPackRows.Num();
    const int32 Current = SelectedPackIndex == INDEX_NONE ? 0 : SelectedPackIndex;
    SelectedPackIndex = (Current + NumRows + (Direction < 0 ? -1 : 1)) % NumRows;
    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    RefreshSelectionPresentation(TextScale, UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::SelectPreviousItem() { StepSelection(-1); }
void UKalmalaInventoryMenuWidget::SelectNextItem() { StepSelection(1); }

#if !UE_BUILD_SHIPPING
void UKalmalaInventoryMenuWidget::SetPackRowsForVerification(const TArray<FKalmalaCatalogueRow>& Rows,
    const int32 TextScale, const int32 Contrast)
{
    ApplyPackRows(TArray<FKalmalaCatalogueRow>(Rows), true, TextScale, Contrast);
}

FName UKalmalaInventoryMenuWidget::GetSelectedItemForVerification() const
{
    return OwnerPackRows.IsValidIndex(SelectedPackIndex) ? OwnerPackRows[SelectedPackIndex].Id : NAME_None;
}

void UKalmalaInventoryMenuWidget::StepSelectionForVerification(const int32 Direction)
{
    StepSelection(Direction);
}
#endif

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
    if (bMenuOpen && !HasTextEntryFocus())
    {
        const FKey Key = Event.GetKey();
        if (Key == EKeys::Gamepad_FaceButton_Right)
        {
            Close();
            return FReply::Handled();
        }
        if (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up)
        {
            StepSelection(-1);
            return FReply::Handled();
        }
        if (Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down)
        {
            StepSelection(1);
            return FReply::Handled();
        }
    }

    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
