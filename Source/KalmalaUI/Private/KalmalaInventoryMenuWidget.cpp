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
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaUITheme.h"

namespace
{
FString GetCarriedToolDisplayName(const FName ToolId)
{
    if (ToolId == TEXT("ReedKnife")) return TEXT("Reed Knife");
    if (ToolId == TEXT("FieldHatchet")) return TEXT("Field Hatchet");
    if (ToolId == TEXT("StonePick")) return TEXT("Stone Pick");
    if (ToolId == TEXT("BronzeAxe")) return TEXT("Bronze Axe");
    if (ToolId == TEXT("IronAxe")) return TEXT("Iron Axe");
    if (ToolId == TEXT("ConstructionHammer")) return TEXT("Construction Hammer");
    return ToolId.ToString();
}

FString BuildToolInspectionText(const FKalmalaToolState& Tool, const FKalmalaToolDefinition& Definition)
{
    FString Text = UKalmalaCatalogueRowsWidget::BuildToolDetail(
        Tool.ToolLevel, Tool.Durability, Definition.MaxDurability);
    if (Tool.ToolLevel < 1 || Tool.Durability < 0 || Tool.Durability > Definition.MaxDurability)
        return Text + TEXT("\nRepair unavailable.");
    return Text + (Tool.Durability < Definition.MaxDurability
        ? TEXT("\nFree repair at a visible Workbench or Forge.")
        : TEXT("\nNo repair needed."));
}
}

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

    UHorizontalBox* ToolActions = WidgetTree->ConstructWidget<UHorizontalBox>();
    RepairToolButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* RepairLabel = WidgetTree->ConstructWidget<UTextBlock>();
    RepairLabel->SetText(FText::FromString(TEXT("Repair selected tool")));
    RepairToolButton->SetContent(RepairLabel);
    Theme.ApplyButton(*RepairToolButton, Contrast);
    Theme.ApplyText(*RepairLabel, Theme.BodySize, false, TextScale, Contrast);
    RepairToolButton->OnClicked.AddDynamic(this, &ThisClass::RepairSelectedTool);
    ToolActions->AddChildToHorizontalBox(RepairToolButton)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

    ToolActionStatusText = WidgetTree->ConstructWidget<UTextBlock>();
    ToolActionStatusText->SetAutoWrapText(true);
    Theme.ApplyText(*ToolActionStatusText, Theme.BodySize, false, TextScale, Contrast);
    ToolActions->AddChildToHorizontalBox(ToolActionStatusText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Content->AddChildToVerticalBox(ToolActions);
    RepairToolButton->SetVisibility(ESlateVisibility::Collapsed);
    ToolActionStatusText->SetVisibility(ESlateVisibility::Collapsed);

    UHorizontalBox* PackAndDetail = WidgetTree->ConstructWidget<UHorizontalBox>();
    UScrollBox* PackScroll = WidgetTree->ConstructWidget<UScrollBox>();
    Theme.ApplyScroll(*PackScroll);
    InventoryRowsView = WidgetTree->ConstructWidget<UKalmalaCatalogueRowsWidget>();
    PackScroll->AddChild(InventoryRowsView);
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

    RefreshOwnerInventory();

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

void UKalmalaInventoryMenuWidget::RefreshOwnerInventory()
{
    APlayerController* Controller = GetOwningPlayer();
    if (Controller == nullptr || !Controller->IsLocalController() || InventoryRowsView == nullptr) return;

    TArray<FKalmalaCatalogueRow> InventoryRows;
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
            InventoryRows.Add({ Stack.ItemId, Definition ? Definition->DisplayName : Stack.ItemId.ToString(),
                FString::Printf(TEXT("× %d"), Stack.Quantity), false });
        }
    }

    if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
    {
        const TArray<FKalmalaToolState>& CarriedTools = Character->GetCarriedToolInventory();
        TSet<FName> SeenToolIds;
        const int32 BoundedToolCount = FMath::Min(CarriedTools.Num(), FKalmalaToolLifecycleContract::MaxCarriedToolRecords);
        for (int32 Index = 0; Index < BoundedToolCount; ++Index)
        {
            const FKalmalaToolState& Tool = CarriedTools[Index];
            if (Tool.ToolId.IsNone() || SeenToolIds.Contains(Tool.ToolId)) continue;
            const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Tool.ToolId);
            if (Definition == nullptr) continue;
            SeenToolIds.Add(Tool.ToolId);
            InventoryRows.Add({ Tool.ToolId, GetCarriedToolDisplayName(Tool.ToolId),
                BuildToolInspectionText(Tool, *Definition), true });
        }
    }

    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    ApplyInventoryRows(MoveTemp(InventoryRows), Inventory != nullptr, TextScale, Contrast);

    if (bAwaitingRepairResult)
    {
        if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
        {
            if (const UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
                Crafting && Crafting->GetResultSerial() != RepairRequestResultSerial)
            {
                LastRepairResultText = Crafting->GetLastResult();
                LastRepairResultToolId = AwaitingRepairToolId;
                bAwaitingRepairResult = false;
                AwaitingRepairToolId = NAME_None;
            }
        }
    }

    RefreshSelectionPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::ApplyInventoryRows(TArray<FKalmalaCatalogueRow>&& Rows,
    const bool bInventoryAvailable, const int32 TextScale, const int32 Contrast)
{
    const FName PreviouslySelected = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
        ? OwnerInventoryRows[SelectedInventoryIndex].Id : NAME_None;
    OwnerInventoryRows = MoveTemp(Rows);
    SelectedInventoryIndex = OwnerInventoryRows.IndexOfByPredicate([PreviouslySelected](const FKalmalaCatalogueRow& Row)
    {
        return !PreviouslySelected.IsNone() && Row.Id == PreviouslySelected;
    });
    if (SelectedInventoryIndex == INDEX_NONE && !OwnerInventoryRows.IsEmpty()) SelectedInventoryIndex = 0;

    const int32 PackRowCount = OwnerInventoryRows.CountByPredicate([](const FKalmalaCatalogueRow& Row)
    {
        return !Row.bCarriedTool;
    });
    FString State;
    if (!bInventoryAvailable) State = TEXT("Waiting for your pack.");
    else if (PackRowCount == 0) State = TEXT("Your pack is empty.");
    else State = FString::Printf(TEXT("Your pack has %d of %d slots filled."),
        FMath::Min(PackRowCount, UKalmalaInventoryComponent::MaxSlots), UKalmalaInventoryComponent::MaxSlots);
    if (PackStateText->GetText().ToString() != State) PackStateText->SetText(FText::FromString(State));

    RefreshSelectionPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RefreshSelectionPresentation(const int32 TextScale, const int32 Contrast)
{
    if (InventoryRowsView == nullptr || ItemDetailView == nullptr || SelectedItemText == nullptr
        || PackStateText == nullptr || PreviousItemButton == nullptr || NextItemButton == nullptr
        || RepairToolButton == nullptr || ToolActionStatusText == nullptr) return;

    if (LastTextScalePercent != TextScale || LastContrastMode != Contrast)
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        Theme.ApplyText(*PackStateText, Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*SelectedItemText, Theme.BodySize, true, TextScale, Contrast);
        Theme.ApplyButton(*PreviousItemButton, Contrast);
        Theme.ApplyButton(*NextItemButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(PreviousItemButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(NextItemButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyButton(*RepairToolButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(RepairToolButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*ToolActionStatusText, Theme.BodySize, false, TextScale, Contrast);
        LastTextScalePercent = TextScale;
        LastContrastMode = Contrast;
    }

    const bool bHasSelection = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex);
    const FKalmalaCatalogueRow* SelectedRow = bHasSelection ? &OwnerInventoryRows[SelectedInventoryIndex] : nullptr;
    const FName SelectedItem = SelectedRow ? SelectedRow->Id : NAME_None;
    if (SelectedItem != LastPresentedSelectionId)
    {
        LastPresentedSelectionId = SelectedItem;
        if (!bAwaitingRepairResult)
        {
            LastRepairResultText.Reset();
            LastRepairResultToolId = NAME_None;
        }
    }
    InventoryRowsView->SetRows(OwnerInventoryRows, UKalmalaInventoryComponent::MaxSlots, TextScale, Contrast, SelectedItem);
    PreviousItemButton->SetIsEnabled(bHasSelection);
    NextItemButton->SetIsEnabled(bHasSelection);

    const FString SelectionText = bHasSelection
        ? FString::Printf(TEXT("Selected: %s — use arrows or D-pad to change"), *SelectedRow->Name)
        : TEXT("No item selected.");
    if (SelectedItemText->GetText().ToString() != SelectionText)
        SelectedItemText->SetText(FText::FromString(SelectionText));

    const bool bSelectedTool = SelectedRow != nullptr && SelectedRow->bCarriedTool;
    RepairToolButton->SetVisibility(bSelectedTool ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ToolActionStatusText->SetVisibility(bSelectedTool ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (SelectedRow != nullptr)
    {
        const FKalmalaCatalogueRow& Row = *SelectedRow;
        const FString DetailKey = FString::Printf(TEXT("%s|%d|%s|%s|%d|%d"), *Row.Id.ToString(),
            Row.bCarriedTool, *Row.Name, *Row.Detail, TextScale, Contrast);
        if (DetailKey != LastSelectedDetailKey)
        {
            if (Row.bCarriedTool)
                ItemDetailView->SetCarriedTool(Row.Id, Row.Name, Row.Detail, TextScale, Contrast);
            else
                ItemDetailView->SetItem(Row.Id, Row.Name, Row.Detail, TextScale, Contrast);
            LastSelectedDetailKey = DetailKey;
        }
        ItemDetailView->SetVisibility(ESlateVisibility::Visible);

        if (Row.bCarriedTool)
        {
            const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
            const FKalmalaToolState* Tool = Character
                ? Character->GetCarriedToolInventory().FindByPredicate([&Row](const FKalmalaToolState& Entry)
                    { return Entry.ToolId == Row.Id; })
                : nullptr;
            const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Row.Id);
            const bool bToolStateValid = Tool != nullptr && Definition != nullptr && Tool->ToolLevel >= 1
                && Tool->Durability >= 0 && Tool->Durability <= Definition->MaxDurability;
            const bool bNeedsRepair = bToolStateValid && Tool->Durability < Definition->MaxDurability;
            const bool bHasRepairAction = Character != nullptr
                && Character->FindComponentByClass<UKalmalaCraftingComponent>() != nullptr;
            RepairToolButton->SetIsEnabled(bNeedsRepair && bHasRepairAction);

            FString StatusText;
            if (!bToolStateValid) StatusText = TEXT("Tool state unavailable.");
            else if (bAwaitingRepairResult && AwaitingRepairToolId == Row.Id)
                StatusText = TEXT("Repair requested. Waiting for the server result.");
            else if (LastRepairResultToolId == Row.Id && !LastRepairResultText.IsEmpty())
                StatusText = LastRepairResultText;
            else if (bNeedsRepair)
                StatusText = TEXT("Repair is free at a visible, same-world Workbench or Forge within 2.5 m.");
            else StatusText = TEXT("This tool is at full condition.");
            if (ToolActionStatusText->GetText().ToString() != StatusText)
                ToolActionStatusText->SetText(FText::FromString(StatusText));
        }
    }
    else
    {
        LastSelectedDetailKey.Reset();
        ItemDetailView->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UKalmalaInventoryMenuWidget::StepSelection(const int32 Direction)
{
    if (OwnerInventoryRows.IsEmpty()) return;
    const int32 NumRows = OwnerInventoryRows.Num();
    const int32 Current = SelectedInventoryIndex == INDEX_NONE ? 0 : SelectedInventoryIndex;
    SelectedInventoryIndex = (Current + NumRows + (Direction < 0 ? -1 : 1)) % NumRows;
    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    RefreshSelectionPresentation(TextScale, UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::SelectPreviousItem() { StepSelection(-1); }
void UKalmalaInventoryMenuWidget::SelectNextItem() { StepSelection(1); }

void UKalmalaInventoryMenuWidget::RepairSelectedTool()
{
    if (!OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)) return;
    const FKalmalaCatalogueRow& Row = OwnerInventoryRows[SelectedInventoryIndex];
    if (!Row.bCarriedTool) return;

    APlayerController* Controller = GetOwningPlayer();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Controller ? Controller->GetPawn() : nullptr);
    const FKalmalaToolState* Tool = Character
        ? Character->GetCarriedToolInventory().FindByPredicate([&Row](const FKalmalaToolState& Entry)
            { return Entry.ToolId == Row.Id; })
        : nullptr;
    const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Row.Id);
    UKalmalaCraftingComponent* Crafting = Character
        ? Character->FindComponentByClass<UKalmalaCraftingComponent>()
        : nullptr;
    if (Controller == nullptr || !Controller->IsLocalController() || Tool == nullptr || Definition == nullptr
        || Tool->ToolLevel < 1 || Tool->Durability < 0 || Tool->Durability >= Definition->MaxDurability || Crafting == nullptr)
    {
        RefreshSelectionPresentation(UKalmalaSettingsWidget::ClampTextScale(
            UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());
        return;
    }

    RepairRequestResultSerial = Crafting->GetResultSerial();
    AwaitingRepairToolId = Row.Id;
    bAwaitingRepairResult = true;
    LastRepairResultText.Reset();
    LastRepairResultToolId = NAME_None;
    ToolActionStatusText->SetText(FText::FromString(TEXT("Repair requested. Waiting for the server result.")));
    Crafting->ServerRepairTool(Row.Id);
    RefreshOwnerInventory();
}

#if !UE_BUILD_SHIPPING
void UKalmalaInventoryMenuWidget::SetInventoryRowsForVerification(const TArray<FKalmalaCatalogueRow>& Rows,
    const int32 TextScale, const int32 Contrast)
{
    ApplyInventoryRows(TArray<FKalmalaCatalogueRow>(Rows), true, TextScale, Contrast);
}

FName UKalmalaInventoryMenuWidget::GetSelectedItemForVerification() const
{
    return OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex) ? OwnerInventoryRows[SelectedInventoryIndex].Id : NAME_None;
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
