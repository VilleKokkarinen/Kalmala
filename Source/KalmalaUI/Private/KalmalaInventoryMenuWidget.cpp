#include "KalmalaInventoryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
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
#include "Styling/CoreStyle.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
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
    MenuContentScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    Theme.ApplyScroll(*MenuContentScrollBox);
    MenuContentScrollBox->OnUserScrolled.AddDynamic(this, &ThisClass::MenuScrolled);
    MenuContentScrollBox->AddChild(Content);
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

    BrowseSearchLabel = WidgetTree->ConstructWidget<UTextBlock>();
    BrowseSearchLabel->SetText(FText::FromString(TEXT("Search carried items and tools")));
    Theme.ApplyText(*BrowseSearchLabel, Theme.BodySize, true, TextScale, Contrast);
    Content->AddChildToVerticalBox(BrowseSearchLabel);

    UHorizontalBox* SearchControls = WidgetTree->ConstructWidget<UHorizontalBox>();
    InventorySearchBox = WidgetTree->ConstructWidget<UEditableTextBox>();
    InventorySearchBox->SetIsFocusable(true);
    InventorySearchBox->SetHintText(FText::FromString(TEXT("Search by visible item name")));
    InventorySearchStyle = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>(TEXT("NormalEditableTextBox"));
    InventorySearchBox->OnTextChanged.AddDynamic(this, &ThisClass::InventorySearchChanged);
    SearchControls->AddChildToHorizontalBox(InventorySearchBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    ClearSearchButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    ClearSearchButton->SetIsFocusable(true);
    ClearSearchButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
    ClearSearchButtonLabel->SetText(FText::FromString(TEXT("Clear search")));
    ClearSearchButton->SetContent(ClearSearchButtonLabel);
    ClearSearchButton->OnClicked.AddDynamic(this, &ThisClass::ClearInventorySearch);
    SearchControls->AddChildToHorizontalBox(ClearSearchButton)->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
    Content->AddChildToVerticalBox(SearchControls);

    UHorizontalBox* BrowseControls = WidgetTree->ConstructWidget<UHorizontalBox>();
    CategoryButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    CategoryButton->SetIsFocusable(true);
    CategoryButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
    CategoryButtonLabel->SetAutoWrapText(true);
    CategoryButton->SetContent(CategoryButtonLabel);
    CategoryButton->OnClicked.AddDynamic(this, &ThisClass::CycleInventoryCategory);
    BrowseControls->AddChildToHorizontalBox(CategoryButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    SortButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    SortButton->SetIsFocusable(true);
    SortButtonLabel = WidgetTree->ConstructWidget<UTextBlock>();
    SortButtonLabel->SetAutoWrapText(true);
    SortButton->SetContent(SortButtonLabel);
    SortButton->OnClicked.AddDynamic(this, &ThisClass::CycleInventorySort);
    UHorizontalBoxSlot* SortSlot = BrowseControls->AddChildToHorizontalBox(SortButton);
    SortSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
    SortSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Content->AddChildToVerticalBox(BrowseControls);

    BrowseStateText = WidgetTree->ConstructWidget<UTextBlock>();
    BrowseStateText->SetAutoWrapText(true);
    Theme.ApplyText(*BrowseStateText, Theme.BodySize, false, TextScale, Contrast);
    Content->AddChildToVerticalBox(BrowseStateText);

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

    UHorizontalBox* FoodActions = WidgetTree->ConstructWidget<UHorizontalBox>();
    EatFoodButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    UTextBlock* EatFoodLabel = WidgetTree->ConstructWidget<UTextBlock>();
    EatFoodLabel->SetText(FText::FromString(TEXT("Eat one serving")));
    EatFoodButton->SetContent(EatFoodLabel);
    Theme.ApplyButton(*EatFoodButton, Contrast);
    Theme.ApplyText(*EatFoodLabel, Theme.BodySize, false, TextScale, Contrast);
    EatFoodButton->OnClicked.AddDynamic(this, &ThisClass::EatSelectedFood);
    FoodActions->AddChildToHorizontalBox(EatFoodButton)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

    FoodActionStatusText = WidgetTree->ConstructWidget<UTextBlock>();
    FoodActionStatusText->SetAutoWrapText(true);
    Theme.ApplyText(*FoodActionStatusText, Theme.BodySize, false, TextScale, Contrast);
    FoodActions->AddChildToHorizontalBox(FoodActionStatusText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Content->AddChildToVerticalBox(FoodActions);
    EatFoodButton->SetVisibility(ESlateVisibility::Collapsed);
    FoodActionStatusText->SetVisibility(ESlateVisibility::Collapsed);

    UHorizontalBox* PackAndDetail = WidgetTree->ConstructWidget<UHorizontalBox>();
    InventoryScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    Theme.ApplyScroll(*InventoryScrollBox);
    InventoryScrollBox->OnUserScrolled.AddDynamic(this, &ThisClass::InventoryScrolled);
    InventoryRowsView = WidgetTree->ConstructWidget<UKalmalaCatalogueRowsWidget>();
    InventoryScrollBox->AddChild(InventoryRowsView);
    PackAndDetail->AddChildToHorizontalBox(InventoryScrollBox)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ItemDetailView = WidgetTree->ConstructWidget<UKalmalaItemDetailWidget>();
    PackAndDetail->AddChildToHorizontalBox(ItemDetailView)->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
    USizeBox* InventoryArea = WidgetTree->ConstructWidget<USizeBox>();
    InventoryArea->SetHeightOverride(300.0f);
    InventoryArea->SetContent(PackAndDetail);
    Content->AddChildToVerticalBox(InventoryArea);

    UpdateBrowseControls(TextScale, Contrast);

    Panel->SetContent(MenuContentScrollBox);
    PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    PanelSizeBox->SetWidthOverride(ResponsivePanelWidth);
    PanelSizeBox->SetHeightOverride(ResponsivePanelHeight);
    PanelSizeBox->SetContent(Panel);

    UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSizeBox);
    PanelSlot->SetHorizontalAlignment(HAlign_Center);
    PanelSlot->SetVerticalAlignment(VAlign_Center);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaInventoryMenuWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (bMenuOpen) UpdateResponsivePanelSize(MyGeometry.GetLocalSize());
}

void UKalmalaInventoryMenuWidget::UpdateResponsivePanelSize(const FVector2D ViewportSize)
{
    if (!PanelSizeBox) return;
    const float NewWidth = FMath::Clamp(ViewportSize.X - 32.0f, 160.0f, 640.0f);
    const float NewHeight = FMath::Clamp(ViewportSize.Y - 32.0f, 160.0f, 560.0f);
    if (!FMath::IsNearlyEqual(NewWidth, ResponsivePanelWidth))
    {
        ResponsivePanelWidth = NewWidth;
        PanelSizeBox->SetWidthOverride(ResponsivePanelWidth);
    }
    if (!FMath::IsNearlyEqual(NewHeight, ResponsivePanelHeight))
    {
        ResponsivePanelHeight = NewHeight;
        PanelSizeBox->SetHeightOverride(ResponsivePanelHeight);
    }
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
    UWidget* FocusTarget = InventorySearchBox ? static_cast<UWidget*>(InventorySearchBox.Get()) : this;
    InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    Controller->SetInputMode(InputMode);
    if (MenuContentScrollBox) MenuContentScrollBox->SetScrollOffset(RememberedMenuScrollOffset);
    if (InventoryScrollBox) InventoryScrollBox->SetScrollOffset(RememberedInventoryScrollOffset);

    bMenuOpen = true;
    SetVisibility(ESlateVisibility::Visible);
    FocusTarget->SetUserFocus(Controller);
    FocusTarget->SetKeyboardFocus();
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

    if (bAwaitingFoodResult)
    {
        if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
        {
            if (const UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
                Crafting && Crafting->GetResultSerial() != FoodRequestResultSerial)
            {
                LastFoodResultText = Crafting->GetLastResult();
                LastFoodResultItemId = AwaitingFoodItemId;
                bAwaitingAcceptedFoodStatus = Crafting->WasLastResultAccepted();
                bAwaitingFoodResult = false;
                AwaitingFoodItemId = NAME_None;
            }
        }
    }

    if (bAwaitingAcceptedFoodStatus)
    {
        if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
        {
            if (const UKalmalaPlayerStatusComponent* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
                Status && Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
            {
                bAwaitingAcceptedFoodStatus = false;
            }
        }
    }

    RefreshSelectionPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::ApplyInventoryRows(TArray<FKalmalaCatalogueRow>&& Rows,
    const bool bInventoryAvailable, const int32 TextScale, const int32 Contrast)
{
    SourceInventoryRows = MoveTemp(Rows);
    if (!RememberedSelectedItemId.IsNone() && !SourceInventoryRows.ContainsByPredicate([this](const FKalmalaCatalogueRow& Row)
        { return Row.Id == RememberedSelectedItemId; }))
    {
        RememberedSelectedItemId = NAME_None;
    }

    const int32 PackRowCount = SourceInventoryRows.CountByPredicate([](const FKalmalaCatalogueRow& Row)
    {
        return !Row.bCarriedTool;
    });
    FString State;
    if (!bInventoryAvailable) State = TEXT("Waiting for your pack.");
    else if (PackRowCount == 0) State = TEXT("Your pack is empty.");
    else State = FString::Printf(TEXT("Your pack has %d of %d slots filled."),
        FMath::Min(PackRowCount, UKalmalaInventoryComponent::MaxSlots), UKalmalaInventoryComponent::MaxSlots);
    if (PackStateText->GetText().ToString() != State) PackStateText->SetText(FText::FromString(State));

    RebuildVisibleInventoryRows(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RebuildVisibleInventoryRows(const int32 TextScale, const int32 Contrast)
{
    const FName PreviouslySelected = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
        ? OwnerInventoryRows[SelectedInventoryIndex].Id : NAME_None;

    TArray<FKalmalaCatalogueRow> VisibleRows;
    for (const FKalmalaCatalogueRow& Row : SourceInventoryRows)
    {
        const bool bCategoryMatches = InventoryCategoryIndex == 0
            || Row.bCarriedTool == (InventoryCategoryIndex == 2);
        const bool bSearchMatches = InventorySearchQuery.IsEmpty()
            || Row.Name.Contains(InventorySearchQuery, ESearchCase::IgnoreCase);
        if (bCategoryMatches && bSearchMatches) VisibleRows.Add(Row);
    }

    if (InventorySortIndex != 0)
    {
        VisibleRows.StableSort([this](const FKalmalaCatalogueRow& A, const FKalmalaCatalogueRow& B)
        {
            // The shared rows widget renders pack slots before the equipment group.
            // Keep navigation in that same visible order for both name sorts.
            if (A.bCarriedTool != B.bCarriedTool) return !A.bCarriedTool;
            const int32 NameComparison = A.Name.Compare(B.Name, ESearchCase::IgnoreCase);
            return NameComparison == 0 ? A.Id.LexicalLess(B.Id) : NameComparison < 0;
        });
    }

    OwnerInventoryRows = MoveTemp(VisibleRows);
    SelectedInventoryIndex = OwnerInventoryRows.IndexOfByPredicate([this](const FKalmalaCatalogueRow& Row)
    {
        return !RememberedSelectedItemId.IsNone() && Row.Id == RememberedSelectedItemId;
    });
    if (SelectedInventoryIndex == INDEX_NONE)
    {
        SelectedInventoryIndex = OwnerInventoryRows.IndexOfByPredicate([PreviouslySelected](const FKalmalaCatalogueRow& Row)
        {
            return !PreviouslySelected.IsNone() && Row.Id == PreviouslySelected;
        });
    }
    if (SelectedInventoryIndex == INDEX_NONE && !OwnerInventoryRows.IsEmpty()) SelectedInventoryIndex = 0;

    const bool bRememberedItemStillOwned = !RememberedSelectedItemId.IsNone()
        && SourceInventoryRows.ContainsByPredicate([this](const FKalmalaCatalogueRow& Row)
            { return Row.Id == RememberedSelectedItemId; });
    if (!bRememberedItemStillOwned)
    {
        RememberedSelectedItemId = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
            ? OwnerInventoryRows[SelectedInventoryIndex].Id : NAME_None;
    }

    RefreshSelectionPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::SetInventorySearch(const FString& Search)
{
    const FString DisplaySearch = Search.Left(64);
    if (InventorySearchBox && InventorySearchBox->GetText().ToString() != DisplaySearch)
        InventorySearchBox->SetText(FText::FromString(DisplaySearch));
    const FString EffectiveSearch = DisplaySearch.TrimStartAndEnd();
    if (InventorySearchQuery == EffectiveSearch) return;
    InventorySearchQuery = EffectiveSearch;
    RebuildVisibleInventoryRows(UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::SetInventoryCategory(const int32 Category)
{
    const int32 NewCategory = FMath::Clamp(Category, 0, 2);
    if (InventoryCategoryIndex == NewCategory) return;
    InventoryCategoryIndex = NewCategory;
    RebuildVisibleInventoryRows(UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::SetInventorySort(const int32 Sort)
{
    const int32 NewSort = FMath::Clamp(Sort, 0, 2);
    if (InventorySortIndex == NewSort) return;
    InventorySortIndex = NewSort;
    RebuildVisibleInventoryRows(UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::InventorySearchChanged(const FText& Search)
{
    SetInventorySearch(Search.ToString());
}

void UKalmalaInventoryMenuWidget::ClearInventorySearch()
{
    SetInventorySearch(TEXT(""));
}

void UKalmalaInventoryMenuWidget::CycleInventoryCategory()
{
    SetInventoryCategory((InventoryCategoryIndex + 1) % 3);
}

void UKalmalaInventoryMenuWidget::CycleInventorySort()
{
    SetInventorySort((InventorySortIndex + 1) % 3);
}

bool UKalmalaInventoryMenuWidget::NavigateInventoryMenu(const FKey Key)
{
    if (Key == EKeys::PageUp || Key == EKeys::Gamepad_LeftShoulder)
    {
        CycleInventoryCategory();
        return true;
    }
    if (Key == EKeys::PageDown || Key == EKeys::Gamepad_RightShoulder)
    {
        CycleInventorySort();
        return true;
    }
    if (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up)
    {
        StepSelection(-1);
        return true;
    }
    if (Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down)
    {
        StepSelection(1);
        return true;
    }
    return false;
}

void UKalmalaInventoryMenuWidget::InventoryScrolled(const float Offset)
{
    if (!bApplyingInventoryRows && !OwnerInventoryRows.IsEmpty()) RememberedInventoryScrollOffset = Offset;
}

void UKalmalaInventoryMenuWidget::MenuScrolled(const float Offset)
{
    RememberedMenuScrollOffset = Offset;
}

void UKalmalaInventoryMenuWidget::UpdateBrowseControls(const int32 TextScale, const int32 Contrast)
{
    if (!InventorySearchBox || !BrowseSearchLabel || !CategoryButton || !SortButton || !ClearSearchButton
        || !CategoryButtonLabel || !SortButtonLabel || !ClearSearchButtonLabel || !BrowseStateText) return;

    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    Theme.ApplyText(*BrowseSearchLabel, Theme.BodySize, true, TextScale, Contrast);
    InventorySearchStyle.SetFont(Theme.MakeFont(Theme.BodySize, false, TextScale));
    InventorySearchStyle.SetBackgroundColor(Contrast != 0 ? Theme.HighContrastPanel : Theme.ButtonNormal);
    InventorySearchBox->SetWidgetStyle(InventorySearchStyle);
    InventorySearchBox->SetForegroundColor(Theme.TextColor(false, Contrast));

    const TCHAR* Categories[] = {TEXT("All"), TEXT("Items"), TEXT("Carried tools")};
    const TCHAR* Sorts[] = {TEXT("Owner order"), TEXT("Name"), TEXT("Category / name")};
    CategoryButtonLabel->SetText(FText::FromString(FString(TEXT("Category: ")) + Categories[InventoryCategoryIndex]));
    SortButtonLabel->SetText(FText::FromString(FString(TEXT("Sort: ")) + Sorts[InventorySortIndex]));
    ClearSearchButtonLabel->SetText(FText::FromString(TEXT("Clear search")));
    ClearSearchButton->SetIsEnabled(!InventorySearchQuery.IsEmpty());
    for (UKalmalaThemedButton* Button : {CategoryButton.Get(), SortButton.Get(), ClearSearchButton.Get()})
    {
        Theme.ApplyButton(*Button, Contrast);
        UTextBlock* Label = CastChecked<UTextBlock>(Button->GetContent());
        Label->SetAutoWrapText(true);
        Theme.ApplyText(*Label, Theme.BodySize, false, TextScale, Contrast);
    }

    FString BrowseState;
    if (SourceInventoryRows.IsEmpty()) BrowseState = TEXT("Your inventory has no carried items or tools.");
    else if (OwnerInventoryRows.IsEmpty()) BrowseState = TEXT("No results. Clear search or choose All to see your inventory.");
    else BrowseState = FString::Printf(TEXT("Showing %d of %d owner-visible entries."),
        OwnerInventoryRows.Num(), SourceInventoryRows.Num());
    BrowseStateText->SetText(FText::FromString(BrowseState));
    Theme.ApplyText(*BrowseStateText, Theme.BodySize, false, TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RefreshSelectionPresentation(const int32 TextScale, const int32 Contrast)
{
    if (InventoryRowsView == nullptr || ItemDetailView == nullptr || SelectedItemText == nullptr
        || PackStateText == nullptr || PreviousItemButton == nullptr || NextItemButton == nullptr
        || RepairToolButton == nullptr || ToolActionStatusText == nullptr
        || EatFoodButton == nullptr || FoodActionStatusText == nullptr || InventoryScrollBox == nullptr) return;

    UpdateBrowseControls(TextScale, Contrast);

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
        Theme.ApplyButton(*EatFoodButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(EatFoodButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*FoodActionStatusText, Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyScroll(*InventoryScrollBox);
        if (MenuContentScrollBox) Theme.ApplyScroll(*MenuContentScrollBox);
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
    bApplyingInventoryRows = true;
    InventoryRowsView->SetRows(OwnerInventoryRows, UKalmalaInventoryComponent::MaxSlots, TextScale, Contrast, SelectedItem);
    InventoryScrollBox->SetScrollOffset(RememberedInventoryScrollOffset);
    bApplyingInventoryRows = false;
    PreviousItemButton->SetIsEnabled(bHasSelection);
    NextItemButton->SetIsEnabled(bHasSelection);

    const FString SelectionText = bHasSelection
        ? FString::Printf(TEXT("Selected: %s"), *SelectedRow->Name)
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
            {
                FString VisibleState = Row.Detail;
                if (UKalmalaPlayerStatusComponent::IsKnownFoodItem(Row.Id))
                {
                    VisibleState += TEXT("\n\nEffect: Steady Meal; 10% lower stamina use for 120 seconds.");
                }
                ItemDetailView->SetItem(Row.Id, Row.Name, VisibleState, TextScale, Contrast);
            }
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

    RefreshFoodActionPresentation(SelectedRow, TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RefreshFoodActionPresentation(const FKalmalaCatalogueRow* SelectedRow,
    const int32 TextScale, const int32 Contrast)
{
    const bool bSelectedFood = SelectedRow != nullptr && !SelectedRow->bCarriedTool
        && UKalmalaPlayerStatusComponent::IsKnownFoodItem(SelectedRow->Id);
    const bool bShowFoodStatus = bSelectedFood || bAwaitingFoodResult || !LastFoodResultText.IsEmpty();
    EatFoodButton->SetVisibility(bSelectedFood ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    FoodActionStatusText->SetVisibility(bShowFoodStatus ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (!bShowFoodStatus) return;

    const APlayerController* Controller = GetOwningPlayer();
    const APawn* OwnerPawn = Controller ? Controller->GetPawn() : nullptr;
    const UKalmalaInventoryComponent* Inventory = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const UKalmalaPlayerStatusComponent* Status = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    const UKalmalaCraftingComponent* Crafting = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;

    bool bCanEat = false;
    FString StatusText;
    if (bAwaitingFoodResult)
    {
        const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
        const FKalmalaItemDefinition* PendingDefinition = Catalogue ? Catalogue->FindItem(AwaitingFoodItemId) : nullptr;
        FString PendingName(TEXT("food"));
        if (PendingDefinition) PendingName = PendingDefinition->DisplayName;
        StatusText = FString::Printf(TEXT("Waiting for the server result for %s."), *PendingName);
    }
    else if (bSelectedFood)
    {
        const int32 Quantity = Inventory ? Inventory->GetQuantity(SelectedRow->Id) : 0;
        const bool bMealActive = Status && Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
        if (!Controller || !Controller->IsLocalController() || !Inventory || !Status || !Crafting)
        {
            StatusText = TEXT("Food use is unavailable while owner data is loading.");
        }
        else if (bMealActive)
        {
            StatusText = FString::Printf(TEXT("A steady meal is active for %.0f seconds. Wait for it to expire."),
                Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId));
        }
        else if (bAwaitingAcceptedFoodStatus)
        {
            StatusText = TEXT("Food was accepted. Waiting for the owner meal status to update.");
        }
        else if (Quantity <= 0)
        {
            StatusText = TEXT("No serving of this food is available in your pack.");
        }
        else
        {
            StatusText = TEXT("Ready: eat one serving for 120 seconds of 10% lower stamina use.");
            bCanEat = !bAwaitingFoodResult && !bAwaitingAcceptedFoodStatus;
        }

        if (LastFoodResultItemId == SelectedRow->Id && !LastFoodResultText.IsEmpty())
        {
            StatusText += TEXT("\nLast server result: ") + LastFoodResultText;
        }
    }
    else
    {
        StatusText = TEXT("Last food result: ") + LastFoodResultText;
    }

    EatFoodButton->SetIsEnabled(bCanEat);
    if (FoodActionStatusText->GetText().ToString() != StatusText)
        FoodActionStatusText->SetText(FText::FromString(StatusText));
}

void UKalmalaInventoryMenuWidget::EatSelectedFood()
{
    if (bAwaitingFoodResult || bAwaitingAcceptedFoodStatus
        || !OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)) return;

    const FKalmalaCatalogueRow& Row = OwnerInventoryRows[SelectedInventoryIndex];
    if (Row.bCarriedTool || !UKalmalaPlayerStatusComponent::IsKnownFoodItem(Row.Id)) return;

    APlayerController* Controller = GetOwningPlayer();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Controller ? Controller->GetPawn() : nullptr);
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    UKalmalaPlayerStatusComponent* Status = Character
        ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    UKalmalaCraftingComponent* Crafting = Character
        ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    if (!Controller || !Controller->IsLocalController() || !Inventory || !Status || !Crafting
        || Inventory->GetQuantity(Row.Id) <= 0
        || Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
    {
        RefreshSelectionPresentation(UKalmalaSettingsWidget::ClampTextScale(
            UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());
        return;
    }

    FoodRequestResultSerial = Crafting->GetResultSerial();
    AwaitingFoodItemId = Row.Id;
    bAwaitingFoodResult = true;
    bAwaitingAcceptedFoodStatus = false;
    LastFoodResultText.Reset();
    LastFoodResultItemId = NAME_None;
    RefreshSelectionPresentation(UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());

    // This existing server transaction is station-free. It rechecks the
    // allowlisted item, owner pack and meal slot, then publishes an owner result.
    Crafting->ServerConsumeFood(Row.Id);
}

void UKalmalaInventoryMenuWidget::StepSelection(const int32 Direction)
{
    if (OwnerInventoryRows.IsEmpty()) return;
    const int32 NumRows = OwnerInventoryRows.Num();
    const int32 Current = SelectedInventoryIndex == INDEX_NONE ? 0 : SelectedInventoryIndex;
    SelectedInventoryIndex = (Current + NumRows + (Direction < 0 ? -1 : 1)) % NumRows;
    RememberedSelectedItemId = OwnerInventoryRows[SelectedInventoryIndex].Id;
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

void UKalmalaInventoryMenuWidget::SetInventoryBrowseForVerification(const FString& Search,
    const int32 Category, const int32 Sort)
{
    SetInventorySearch(Search);
    SetInventoryCategory(Category);
    SetInventorySort(Sort);
}

bool UKalmalaInventoryMenuWidget::NavigateForVerification(const FKey Key)
{
    return NavigateInventoryMenu(Key);
}

TArray<FName> UKalmalaInventoryMenuWidget::GetVisibleItemIdsForVerification() const
{
    TArray<FName> Ids;
    for (const FKalmalaCatalogueRow& Row : OwnerInventoryRows) Ids.Add(Row.Id);
    return Ids;
}

bool UKalmalaInventoryMenuWidget::HasBrowseFocusTargetsForVerification() const
{
    return InventorySearchBox && InventorySearchBox->IsFocusable()
        && CategoryButton && CategoryButton->IsFocusable()
        && SortButton && SortButton->IsFocusable()
        && ClearSearchButton && ClearSearchButton->IsFocusable();
}

void UKalmalaInventoryMenuWidget::SetInventoryScrollOffsetForVerification(const float Offset)
{
    RememberedInventoryScrollOffset = FMath::Max(0.0f, Offset);
}

void UKalmalaInventoryMenuWidget::SetViewportSizeForVerification(const FVector2D ViewportSize)
{
    UpdateResponsivePanelSize(ViewportSize);
}
#endif

void UKalmalaInventoryMenuWidget::Close()
{
    if (!bMenuOpen) return;
    if (MenuContentScrollBox) RememberedMenuScrollOffset = MenuContentScrollBox->GetScrollOffset();
    if (InventoryScrollBox && !OwnerInventoryRows.IsEmpty())
        RememberedInventoryScrollOffset = InventoryScrollBox->GetScrollOffset();
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
        const bool bBrowseButtonFocused = (CategoryButton && CategoryButton->HasKeyboardFocus())
            || (SortButton && SortButton->HasKeyboardFocus()) || (ClearSearchButton && ClearSearchButton->HasKeyboardFocus());
        if (!bBrowseButtonFocused && NavigateInventoryMenu(Key)) return FReply::Handled();
    }

    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
