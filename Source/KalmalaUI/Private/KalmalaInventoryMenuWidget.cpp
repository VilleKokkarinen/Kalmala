#include "KalmalaInventoryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaCraftingSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaInventoryGridWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaUITheme.h"
#include "Styling/SlateTypes.h"

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
    if (!WidgetTree) return;
    const auto& Theme = FKalmalaUITheme::Get();
    const int32 TextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    auto* Root = WidgetTree->ConstructWidget<UOverlay>();
    WidgetTree->RootWidget = Root;
    auto* Scrim = WidgetTree->ConstructWidget<UBorder>();
    Scrim->SetBrushColor(FLinearColor(0.008f, 0.012f, 0.014f, 0.25f));
    auto* ScrimSlot = Root->AddChildToOverlay(Scrim);
    ScrimSlot->SetHorizontalAlignment(HAlign_Fill);
    ScrimSlot->SetVerticalAlignment(VAlign_Fill);
    PanelBackplate = WidgetTree->ConstructWidget<UBorder>();
    if (Contrast == 0) Theme.ApplyPanel(*PanelBackplate, Contrast, &Theme.InventoryPanelImage);
    else Theme.ApplySolidPanel(*PanelBackplate, Contrast);
    PanelBackplate->SetPadding(FMargin(8.0f));
    auto* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
    auto* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    MenuContentScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    Theme.ApplyScroll(*MenuContentScrollBox);
    MenuContentScrollBox->OnUserScrolled.AddDynamic(this, &ThisClass::MenuScrolled);
    MenuContentScrollBox->AddChild(Content);
    auto AddText = [&](const FString& Value, const bool bEmphasis)
    {
        auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Value));
        Text->SetAutoWrapText(true);
        Theme.ApplyText(*Text, bEmphasis ? Theme.EmphasisSize : Theme.BodySize, bEmphasis, TextScale, Contrast);
        Content->AddChildToVerticalBox(Text);
        return Text;
    };
    AddText(TEXT("Inventory"), true);
    GridView = WidgetTree->ConstructWidget<UKalmalaInventoryGridWidget>();
    GridView->OnItemSelected.AddUObject(this, &ThisClass::SelectGridItem);
    GridView->OnItemHovered.AddUObject(this, &ThisClass::HoverGridItem);
    GridSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    GridSizeBox->SetHeightOverride((ResponsivePanelWidth * 0.43f - Theme.PaddingX * 2.0f - 24.0f) * 0.4f);
    GridSizeBox->SetContent(GridView);
    Content->AddChildToVerticalBox(GridSizeBox);
    ArmorWeightText = AddText(TEXT("Armor 0 · Weight 0/300"), false);
    ItemDetailView = WidgetTree->ConstructWidget<UKalmalaItemDetailWidget>();
    Content->AddChildToVerticalBox(ItemDetailView);
    ItemDetailView->SetVisibility(ESlateVisibility::Collapsed);
    auto AddAction = [&](const FString& Label)
    {
        auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Label));
        Theme.ApplyText(*Text, Theme.BodySize, false, TextScale, Contrast);
        Button->SetContent(Text);
        Theme.ApplyButton(*Button, Contrast);
        Content->AddChildToVerticalBox(Button);
        Button->SetVisibility(ESlateVisibility::Collapsed);
        return Button;
    };
    RepairToolButton = AddAction(TEXT("Repair selected tool"));
    RepairToolButton->OnClicked.AddDynamic(this, &ThisClass::RepairSelectedTool);
    ToolActionStatusText = AddText(TEXT(""), false);
    ToolActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
    EatFoodButton = AddAction(TEXT("Eat one serving"));
    EatFoodButton->OnClicked.AddDynamic(this, &ThisClass::EatSelectedFood);
    FoodActionStatusText = AddText(TEXT(""), false);
    FoodActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
    InventoryPanel = WidgetTree->ConstructWidget<UBorder>();
    Theme.ApplySolidPanel(*InventoryPanel, Contrast);
    InventoryPanel->SetPadding(FMargin(Theme.PaddingX, Theme.PaddingY));
    InventoryPanel->SetContent(MenuContentScrollBox);
    auto* InventorySlot = Split->AddChildToHorizontalBox(InventoryPanel);
    InventorySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill, 0.43f));
    InventorySlot->SetPadding(FMargin(4.0f));
    CraftingCompanion = GetOwningPlayer()
        ? CreateWidget<UKalmalaCraftingWidget>(GetOwningPlayer(), UKalmalaCraftingWidget::StaticClass())
        : nullptr;
    if (!CraftingCompanion) CraftingCompanion = WidgetTree->ConstructWidget<UKalmalaCraftingWidget>();
    if (CraftingCompanion)
    {
        auto* CraftingSlot = Split->AddChildToHorizontalBox(CraftingCompanion);
        CraftingSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill, 0.57f));
        CraftingSlot->SetPadding(FMargin(4.0f));
    }
    PanelBackplate->SetContent(Split);
    PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    PanelSizeBox->SetWidthOverride(ResponsivePanelWidth);
    PanelSizeBox->SetHeightOverride(ResponsivePanelHeight);
    PanelSizeBox->SetContent(PanelBackplate);
    auto* PanelSlot = Root->AddChildToOverlay(PanelSizeBox);
    PanelSlot->SetHorizontalAlignment(HAlign_Center);
    PanelSlot->SetVerticalAlignment(VAlign_Center);
    PanelSlot->SetPadding(FMargin(0.0f));
    SetVisibility(ESlateVisibility::Collapsed);
}
void UKalmalaInventoryMenuWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bMenuOpen) return;
    UpdateResponsivePanelSize(MyGeometry.GetLocalSize());
    if (bOpeningAnimationActive)
    {
        if (UKalmalaSettingsWidget::IsReducedMotionEnabled())
        {
            bOpeningAnimationActive = false;
            if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D::ZeroVector);
            if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D::ZeroVector);
        }
        else
        {
            const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
            OpeningElapsed = FMath::Min(OpeningElapsed + FMath::Clamp(InDeltaTime, 0.0f, 1.0f / 30.0f),
                Theme.OptionsOpeningDuration);
            const float Progress = Theme.OptionsOpeningDuration > 0.0f
                ? OpeningElapsed / Theme.OptionsOpeningDuration : 1.0f;
            const float Offset = Theme.OptionsOpeningOffset(Progress);
            if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D(0.0f, Offset));
            if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D(-Offset, 0.0f));
            if (Progress >= 1.0f)
            {
                bOpeningAnimationActive = false;
                if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D::ZeroVector);
                if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D::ZeroVector);
            }
        }
    }
}

void UKalmalaInventoryMenuWidget::UpdateResponsivePanelSize(const FVector2D ViewportSize)
{
    if (!PanelSizeBox) return;
    const float NewWidth = FMath::Clamp(ViewportSize.X - 32.0f, 160.0f, 1220.0f);
    const float NewHeight = FMath::Clamp(ViewportSize.Y - 32.0f, 160.0f, 900.0f);
    if (!FMath::IsNearlyEqual(NewWidth, ResponsivePanelWidth))
    {
        ResponsivePanelWidth = NewWidth;
        PanelSizeBox->SetWidthOverride(ResponsivePanelWidth);
    }
    if (GridSizeBox) GridSizeBox->SetHeightOverride(FMath::Max(40.0f,
        (ResponsivePanelWidth * 0.43f - FKalmalaUITheme::Get().PaddingX * 2.0f - 24.0f) * 0.4f));
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

    HoveredItemId = NAME_None;
    bSelectionDetailsRequested = false;
    if (GridView) GridView->ClearHover();
    RefreshOwnerInventory();
    if (CraftingCompanion) CraftingCompanion->OpenAsInventoryCompanion();

    bPreviousCursorVisibility = Controller->bShowMouseCursor;
    bAcquiredMoveIgnore = !Controller->IsMoveInputIgnored();
    bAcquiredLookIgnore = !Controller->IsLookInputIgnored();
    if (bAcquiredMoveIgnore) Controller->SetIgnoreMoveInput(true);
    if (bAcquiredLookIgnore) Controller->SetIgnoreLookInput(true);
    if (GridView) GridView->CancelMove();
    Controller->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    UWidget* FocusTarget = GridView ? static_cast<UWidget*>(GridView.Get()) : this;
    InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    Controller->SetInputMode(InputMode);
    if (MenuContentScrollBox) MenuContentScrollBox->SetScrollOffset(RememberedMenuScrollOffset);


    bMenuOpen = true;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    bOpeningAnimationActive = Theme.ShouldAnimateOptionsOpening();
    OpeningElapsed = 0.0f;
    const float InitialOffset = Theme.OptionsOpeningOffset(0.0f);
    if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D(0.0f, InitialOffset));
    if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D(-InitialOffset, 0.0f));
    SetVisibility(ESlateVisibility::Visible);
    FocusTarget->SetUserFocus(Controller);
    FocusTarget->SetKeyboardFocus();
}

void UKalmalaInventoryMenuWidget::RefreshOwnerInventory()
{
    APlayerController* Controller = GetOwningPlayer();
    if (Controller == nullptr || !Controller->IsLocalController()) return;

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

    if (Inventory)
    {
        const auto& Slots = Inventory->GetGridSlots();
        InventoryRows.StableSort([&Slots](const FKalmalaCatalogueRow& A, const FKalmalaCatalogueRow& B)
        {
            const int32 Left = Slots.IndexOfByKey(A.Id), Right = Slots.IndexOfByKey(B.Id);
            return (Left == INDEX_NONE ? 40 : Left) < (Right == INDEX_NONE ? 40 : Right);
        });
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
    (void)bInventoryAvailable;
    SourceInventoryRows = MoveTemp(Rows);
    if (!RememberedSelectedItemId.IsNone() && !SourceInventoryRows.ContainsByPredicate([this](const FKalmalaCatalogueRow& Row)
        { return Row.Id == RememberedSelectedItemId; }))
    {
        RememberedSelectedItemId = NAME_None;
    }

    auto* Inventory = GetOwningPlayerPawn() ? GetOwningPlayerPawn()->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (GridView) GridView->Refresh(Inventory, false, RememberedSelectedItemId);
    if (ArmorWeightText)
    {
        const float Weight = Inventory ? Inventory->GetCarriedWeight() : 0.0f;
        const float Capacity = Inventory ? Inventory->GetCarryCapacity() : 300.0f;
        const FString WeightText = FMath::IsNearlyEqual(Weight, FMath::RoundToFloat(Weight))
            ? FString::Printf(TEXT("%.0f"), Weight) : FString::Printf(TEXT("%.1f"), Weight);
        ArmorWeightText->SetText(FText::FromString(FString::Printf(
            TEXT("Armor 0 · Weight %s/%.0f"), *WeightText, Capacity)));
    }

    RebuildVisibleInventoryRows(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RebuildVisibleInventoryRows(const int32 TextScale, const int32 Contrast)
{
    OwnerInventoryRows = SourceInventoryRows;
    SelectedInventoryIndex = OwnerInventoryRows.IndexOfByPredicate([this](const FKalmalaCatalogueRow& Row)
    { return Row.Id == RememberedSelectedItemId; });
    if (SelectedInventoryIndex == INDEX_NONE && !OwnerInventoryRows.IsEmpty() && !bEmptyCellSelected) SelectedInventoryIndex = 0;
    RememberedSelectedItemId = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
        ? OwnerInventoryRows[SelectedInventoryIndex].Id : NAME_None;
    RefreshSelectionPresentation(TextScale, Contrast);
}
void UKalmalaInventoryMenuWidget::SelectGridItem(const FName ItemId)
{
    bEmptyCellSelected = ItemId.IsNone();
    bSelectionDetailsRequested = !ItemId.IsNone();
    RememberedSelectedItemId = ItemId;
    SelectedInventoryIndex = OwnerInventoryRows.IndexOfByPredicate(
        [ItemId](const FKalmalaCatalogueRow& Row) { return Row.Id == ItemId; });
    RefreshSelectionPresentation(UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::HoverGridItem(const FName ItemId)
{
    if (HoveredItemId == ItemId) return;
    HoveredItemId = ItemId;
    RefreshSelectionPresentation(UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
}

bool UKalmalaInventoryMenuWidget::NavigateInventoryMenu(const FKey Key)
{
    if (GridView && (GridView->HasKeyboardFocus()
        || (GetOwningPlayer() && GridView->HasUserFocus(GetOwningPlayer())))) return false;
    if (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up)
    { StepSelection(-1); return true; }
    if (Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down)
    { StepSelection(1); return true; }
    return false;
}
void UKalmalaInventoryMenuWidget::MenuScrolled(const float Offset)
{
    RememberedMenuScrollOffset = Offset;
}

void UKalmalaInventoryMenuWidget::RefreshSelectionPresentation(const int32 TextScale, const int32 Contrast)
{
    if (GridView == nullptr || ItemDetailView == nullptr || ArmorWeightText == nullptr
        || RepairToolButton == nullptr || ToolActionStatusText == nullptr
        || EatFoodButton == nullptr || FoodActionStatusText == nullptr) return;

    if (LastTextScalePercent != TextScale || LastContrastMode != Contrast)
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        Theme.ApplyText(*ArmorWeightText, Theme.BodySize, false, TextScale, Contrast);
        if (InventoryPanel) Theme.ApplySolidPanel(*InventoryPanel, Contrast);
        if (PanelBackplate)
        {
            if (Contrast == 0) Theme.ApplyPanel(*PanelBackplate, Contrast, &Theme.InventoryPanelImage);
            else Theme.ApplySolidPanel(*PanelBackplate, Contrast);
            PanelBackplate->SetPadding(FMargin(8.0f));
        }

        Theme.ApplyButton(*RepairToolButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(RepairToolButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*ToolActionStatusText, Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyButton(*EatFoodButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(EatFoodButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*FoodActionStatusText, Theme.BodySize, false, TextScale, Contrast);

        if (MenuContentScrollBox) Theme.ApplyScroll(*MenuContentScrollBox);
        LastTextScalePercent = TextScale;
        LastContrastMode = Contrast;
    }

    const bool bHasSelection = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex);
    const FKalmalaCatalogueRow* SelectedRow = bHasSelection ? &OwnerInventoryRows[SelectedInventoryIndex] : nullptr;
    const FName SelectedItem = SelectedRow ? SelectedRow->Id : NAME_None;
    if (!HoveredItemId.IsNone() && !OwnerInventoryRows.ContainsByPredicate([this](const FKalmalaCatalogueRow& Row)
        { return Row.Id == HoveredItemId; })) HoveredItemId = NAME_None;
    if (SelectedItem != LastPresentedSelectionId)
    {
        LastPresentedSelectionId = SelectedItem;
        if (!bAwaitingRepairResult)
        {
            LastRepairResultText.Reset();
            LastRepairResultToolId = NAME_None;
        }
    }


    if (GridView)
    {
        auto* Inventory = GetOwningPlayerPawn() ? GetOwningPlayerPawn()->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        GridView->Refresh(Inventory, false, SelectedItem);
    }

    const FKalmalaCatalogueRow* HoveredRow = HoveredItemId.IsNone() ? nullptr
        : OwnerInventoryRows.FindByPredicate([this](const FKalmalaCatalogueRow& Row) { return Row.Id == HoveredItemId; });
    const FKalmalaCatalogueRow* DetailRow = HoveredRow ? HoveredRow
        : (bSelectionDetailsRequested ? SelectedRow : nullptr);
    const bool bShowSelectedActions = bSelectionDetailsRequested && HoveredItemId.IsNone();
    const bool bSelectedTool = SelectedRow != nullptr && SelectedRow->bCarriedTool;
    RepairToolButton->SetVisibility(bSelectedTool && bShowSelectedActions
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ToolActionStatusText->SetVisibility(bSelectedTool && bShowSelectedActions
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (DetailRow != nullptr)
    {
        const FKalmalaCatalogueRow& Row = *DetailRow;
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
    }
    else
    {
        LastSelectedDetailKey.Reset();
        ItemDetailView->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (bSelectedTool && bShowSelectedActions)
    {
        const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
        const FKalmalaToolState* Tool = Character
            ? Character->GetCarriedToolInventory().FindByPredicate([SelectedRow](const FKalmalaToolState& Entry)
                { return Entry.ToolId == SelectedRow->Id; }) : nullptr;
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(SelectedRow->Id);
        const bool bToolStateValid = Tool != nullptr && Definition != nullptr && Tool->ToolLevel >= 1
            && Tool->Durability >= 0 && Tool->Durability <= Definition->MaxDurability;
        const bool bNeedsRepair = bToolStateValid && Tool->Durability < Definition->MaxDurability;
        const bool bHasRepairAction = Character != nullptr
            && Character->FindComponentByClass<UKalmalaCraftingComponent>() != nullptr;
        RepairToolButton->SetIsEnabled(bNeedsRepair && bHasRepairAction);
        FString StatusText;
        if (!bToolStateValid) StatusText = TEXT("Tool state unavailable.");
        else if (bAwaitingRepairResult && AwaitingRepairToolId == SelectedRow->Id)
            StatusText = TEXT("Repair requested. Waiting for the server result.");
        else if (LastRepairResultToolId == SelectedRow->Id && !LastRepairResultText.IsEmpty())
            StatusText = LastRepairResultText;
        else if (bNeedsRepair)
            StatusText = TEXT("Repair is free at a visible, same-world Workbench or Forge within 2.5 m.");
        else StatusText = TEXT("This tool is at full condition.");
        if (ToolActionStatusText->GetText().ToString() != StatusText)
            ToolActionStatusText->SetText(FText::FromString(StatusText));
    }
    RefreshFoodActionPresentation(bShowSelectedActions ? SelectedRow : nullptr, TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RefreshFoodActionPresentation(const FKalmalaCatalogueRow* SelectedRow,
    const int32 TextScale, const int32 Contrast)
{
    if (!bSelectionDetailsRequested || !HoveredItemId.IsNone())
    {
        EatFoodButton->SetVisibility(ESlateVisibility::Collapsed);
        FoodActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
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
    HoveredItemId = NAME_None;
    bSelectionDetailsRequested = true;
    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    RefreshSelectionPresentation(TextScale, UKalmalaSettingsWidget::GetContrastMode());
}

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
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        NativeOnInitialized();
    }

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

void UKalmalaInventoryMenuWidget::SetViewportSizeForVerification(const FVector2D ViewportSize)
{
    UpdateResponsivePanelSize(ViewportSize);
}
#endif

void UKalmalaInventoryMenuWidget::Close()
{
    if (!bMenuOpen) return;
    if (MenuContentScrollBox) RememberedMenuScrollOffset = MenuContentScrollBox->GetScrollOffset();
    if (GridView) GridView->CancelMove();
    if (CraftingCompanion) CraftingCompanion->CloseInventoryCompanion();

    bMenuOpen = false;
    bOpeningAnimationActive = false;
    OpeningElapsed = 0.0f;
    HoveredItemId = NAME_None;
    bSelectionDetailsRequested = false;
    if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D::ZeroVector);
    if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D::ZeroVector);
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
    if (bMenuOpen && NavigateInventoryMenu(Event.GetKey())) return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

FReply UKalmalaInventoryMenuWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (bMenuOpen && (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    { Close(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry, Event);
}
