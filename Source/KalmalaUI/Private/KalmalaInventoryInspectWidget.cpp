#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaIconWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaUITheme.h"
#include "KalmalaThemedButton.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "InputCoreTypes.h"
#include "Brushes/SlateRoundedBoxBrush.h"

TSharedRef<SWidget> UKalmalaInventoryInspectWidget::RebuildWidget()
{
    Build();
    return Super::RebuildWidget();
}

void UKalmalaInventoryInspectWidget::Build()
{
    if (Column || !WidgetTree) return;
    SetIsFocusable(true);
    Column = WidgetTree->ConstructWidget<UVerticalBox>();
    WidgetTree->RootWidget = Column;
    Instructions = WidgetTree->ConstructWidget<UTextBlock>();
    Instructions->SetAutoWrapText(true); Column->AddChild(Instructions);
    SearchBox = WidgetTree->ConstructWidget<UEditableTextBox>();
    SearchBox->SetHintText(FText::FromString(TEXT("Search your inventory by name")));
    SearchBox->OnTextChanged.AddDynamic(this, &ThisClass::SearchChanged);
    Column->AddChild(SearchBox);
    BrowseStack = WidgetTree->ConstructWidget<UVerticalBox>();
    BrowseRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChild(BrowseStack);
    auto MakeControl = [this]()
    {
        auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetAutoWrapText(true); Button->SetContent(Label);
        return Button;
    };
    CategoryButton = MakeControl(); SortButton = MakeControl(); ClearButton = MakeControl();
    CategoryButton->OnClicked.AddDynamic(this, &ThisClass::CycleCategory);
    SortButton->OnClicked.AddDynamic(this, &ThisClass::CycleSort);
    ClearButton->OnClicked.AddDynamic(this, &ThisClass::ClearSearch);
    Results = WidgetTree->ConstructWidget<UTextBlock>(); Results->SetAutoWrapText(true); Column->AddChild(Results);
    auto* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(); Column->AddChild(Actions);
    for (const TCHAR* Name : {TEXT("Inspect previous item"), TEXT("Inspect next item")})
    {
        auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Name));
        Label->SetAutoWrapText(true); Label->SetJustification(ETextJustify::Center);
        Button->SetContent(Label);
        Actions->AddChildToHorizontalBox(Button)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        Buttons.Add(Button);
    }
    Buttons[0]->OnClicked.AddDynamic(this, &ThisClass::Previous);
    Buttons[1]->OnClicked.AddDynamic(this, &ThisClass::Next);
    ListContent = WidgetTree->ConstructWidget<UVerticalBox>();
    ListScroll = WidgetTree->ConstructWidget<UScrollBox>();
    FKalmalaUITheme::Get().ApplyScroll(*ListScroll);
    Grid = WidgetTree->ConstructWidget<UGridPanel>();
    ListScroll->AddChild(Grid);
    ListContent->AddChildToVerticalBox(ListScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Actions->RemoveFromParent();
    ListContent->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
    ListFrame = WidgetTree->ConstructWidget<UBorder>();
    ListFrame->SetContent(ListContent);
    Detail = WidgetTree->ConstructWidget<UKalmalaItemDetailWidget>();
    DetailFrame = WidgetTree->ConstructWidget<UBorder>();
    DetailFrame->SetContent(Detail);
    Split = WidgetTree->ConstructWidget<UHorizontalBox>();
    ApplyLayout();
}

void UKalmalaInventoryInspectWidget::ApplyLayout()
{
    if (!Column || !ListFrame || !DetailFrame || !Split || !BrowseStack || !BrowseRow) return;
    for (UButton* Button : {CategoryButton.Get(), SortButton.Get(), ClearButton.Get()})
        Button->RemoveFromParent();
    BrowseStack->RemoveFromParent();
    BrowseRow->RemoveFromParent();
    if (bUseSplitLayout)
    {
        Column->InsertChildAt(2, BrowseRow);
        for (UButton* Button : {CategoryButton.Get(), SortButton.Get(), ClearButton.Get()})
            BrowseRow->AddChildToHorizontalBox(Button)->SetPadding(FMargin(3.0f, 2.0f));
    }
    else
    {
        Column->InsertChildAt(2, BrowseStack);
        BrowseStack->AddChild(CategoryButton);
        BrowseStack->AddChild(SortButton);
        BrowseStack->AddChild(ClearButton);
    }
    ListFrame->RemoveFromParent();
    DetailFrame->RemoveFromParent();
    Split->RemoveFromParent();
    if (bUseSplitLayout)
    {
        auto* ListSlot = Split->AddChildToHorizontalBox(ListFrame);
        FSlateChildSize ListSize(ESlateSizeRule::Fill);
        ListSize.Value = 0.36f;
        ListSlot->SetSize(ListSize);
        ListSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        auto* DetailSlot = Split->AddChildToHorizontalBox(DetailFrame);
        FSlateChildSize DetailSize(ESlateSizeRule::Fill);
        DetailSize.Value = 0.64f;
        DetailSlot->SetSize(DetailSize);
        Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
    else
    {
        Column->AddChildToVerticalBox(ListFrame);
        Column->AddChildToVerticalBox(DetailFrame);
    }
}

FString UKalmalaInventoryInspectWidget::ActionGuidance(FName Id, bool bTool)
{
    if (Id == TEXT("RoastedFieldMeat") || Id == TEXT("HearthBroth") || Id == TEXT("SmokedFieldMeat"))
        return TEXT("Existing action: use the matching Eat button in this menu. The server checks the meal and current effect.");
    if (bTool && Id != TEXT("ConstructionHammer"))
        return TEXT("Existing action: use the matching Repair button near a Workbench or Forge; only damaged or broken tools need repair. Grinding Stone interaction repairs all eligible tools.");
    if (Id == TEXT("ConstructionHammer"))
        return TEXT("Existing actions: choose a recipe above to build or preview placement. The server checks materials and placement.");
    return TEXT("Use existing recipe, hearth or chest controls where this item is accepted. Inspection does not use or transfer an item.");
}

void UKalmalaInventoryInspectWidget::SetRows(const TArray<FKalmalaCatalogueRow>& InRows, int32 Scale, int32 Contrast,
    const FString& InListLabel, const FString& InEmptyLabel, const bool bInAllowToolFilter,
    const int32 InGridColumns, const bool bInUseSplitLayout)
{
    const int32 BoundedGridColumns = FMath::Clamp(InGridColumns, 1, 4);
    FString Key = FString::Printf(TEXT("%d|%d|%s|%s|%d|%d"), Scale, Contrast, *InListLabel,
        *InEmptyLabel, bInAllowToolFilter, BoundedGridColumns);
    Key += bInUseSplitLayout ? TEXT("|split") : TEXT("|stacked");
    for (const auto& Row : InRows) Key += TEXT("|") + Row.Id.ToString() + Row.Name + Row.Detail + (Row.bCarriedTool ? TEXT("T") : TEXT("I"));
    if (Column && Key == LastRows) return;
    LastRows = Key;
    OwnerRows = InRows; TextScale = Scale; ContrastMode = Contrast;
    ListLabel = InListLabel; EmptyLabel = InEmptyLabel; bAllowToolFilter = bInAllowToolFilter;
    GridColumns = BoundedGridColumns;
    const bool bLayoutChanged = bUseSplitLayout != bInUseSplitLayout;
    bUseSplitLayout = bInUseSplitLayout;
    Build();
    if (bLayoutChanged) ApplyLayout();
    Refilter();
}

void UKalmalaInventoryInspectWidget::SetSearch(const FString& Query)
{
    Search = Query.Left(64).TrimStartAndEnd();
    Build(); Refilter();
}
void UKalmalaInventoryInspectWidget::SearchChanged(const FText& Text) { SetSearch(Text.ToString()); }
void UKalmalaInventoryInspectWidget::SetCategory(int32 Value) { Category = FMath::Clamp(Value, 0, 3); Build(); Refilter(); }
void UKalmalaInventoryInspectWidget::SetSort(int32 Value) { Sort = FMath::Clamp(Value, 0, 2); Build(); Refilter(); }
void UKalmalaInventoryInspectWidget::RestoreBrowseState(const FString& Query, const int32 InCategory,
    const int32 InSort, const FName SelectedItemId)
{
    Search = Query.Left(64).TrimStartAndEnd();
    Category = FMath::Clamp(InCategory, 0, 3);
    Sort = FMath::Clamp(InSort, 0, 2);
    if (SearchBox && SearchBox->GetText().ToString() != Search)
        SearchBox->SetText(FText::FromString(Search));
    else
        Refilter();

    const int32 Found = Rows.IndexOfByPredicate([SelectedItemId](const auto& Row)
    {
        return !SelectedItemId.IsNone() && Row.Id == SelectedItemId;
    });
    Selected = Found == INDEX_NONE ? 0 : Found;
    Refresh();
}
void UKalmalaInventoryInspectWidget::CycleCategory() { SetCategory((Category + 1) % 4); }
void UKalmalaInventoryInspectWidget::CycleSort() { SetSort((Sort + 1) % 3); }
void UKalmalaInventoryInspectWidget::ClearSearch() { SearchBox->SetText(FText::GetEmpty()); SetSearch(TEXT("")); }
namespace
{
    int32 EquipmentCategory(const FKalmalaCatalogueRow& Row)
    {
        if (!Row.bCarriedTool) return 1;
        const auto* Profile = UKalmalaToolCatalogue::Get()->FindProfile(Row.Id);
        return Profile && Profile->IsWeapon() ? 3 : 2;
    }
}

void UKalmalaInventoryInspectWidget::Refilter()
{
    const FName Old = GetSelectedItem();
    Rows.Reset();
    for (const auto& Row : OwnerRows)
    {
        const FKalmalaToolProfileData* Profile = Row.bCarriedTool
            ? UKalmalaToolCatalogue::Get()->FindProfile(Row.Id) : nullptr;
        const bool bWeapon = Profile && Profile->IsWeapon();
        const bool bCategoryMatch = Category == 0 || (Category == 1 && !Row.bCarriedTool)
            || (Category == 2 && Row.bCarriedTool && !bWeapon) || (Category == 3 && bWeapon);
        if ((!bAllowToolFilter || bCategoryMatch)
            && (Search.IsEmpty() || Row.Name.Contains(Search, ESearchCase::IgnoreCase))) Rows.Add(Row);
    }
    if (Sort != 0)
        Rows.StableSort([this](const auto& A, const auto& B)
        {
            if (Sort == 2 && EquipmentCategory(A) != EquipmentCategory(B))
                return EquipmentCategory(A) < EquipmentCategory(B);
            const int32 Compare = A.Name.Compare(B.Name, ESearchCase::IgnoreCase);
            return Compare == 0 ? A.Id.LexicalLess(B.Id) : Compare < 0;
        });
    const int32 Found = Rows.IndexOfByPredicate([Old](const auto& Row) { return Row.Id == Old; });
    Selected = Found == INDEX_NONE ? 0 : Found;
    Refresh();
}

FName UKalmalaInventoryInspectWidget::GetSelectedItem() const
{
    return Rows.IsValidIndex(Selected) ? Rows[Selected].Id : NAME_None;
}

void UKalmalaInventoryInspectWidget::Refresh()
{
    if (!Column) return;
    const auto& Theme = FKalmalaUITheme::Get();
    Instructions->SetText(FText::FromString(bUseSplitLayout
        ? TEXT("Select equipment to inspect its level and condition.")
        : ListLabel + TEXT(" details. > marks the selected item.")));
    Theme.ApplyText(*Instructions, Theme.BodySize, false, TextScale, ContrastMode);
    BrowseSearchStyle = SearchBox->GetWidgetStyle();
    SearchBox->SetHintText(FText::FromString(FString::Printf(TEXT("Search %s by name"), *ListLabel.ToLower())));
    BrowseSearchStyle.SetFont(Theme.MakeFont(Theme.BodySize, false, TextScale));
    BrowseSearchStyle.SetBackgroundColor(ContrastMode > 0 ? Theme.HighContrastPanel
        : (bUseSplitLayout ? FLinearColor(.02f, .025f, .03f, 1.f) : Theme.ButtonNormal));
    SearchBox->SetWidgetStyle(BrowseSearchStyle);
    SearchBox->SetForegroundColor(Theme.TextColor(false, ContrastMode));
    const TCHAR* Categories[] = {TEXT("All"), TEXT("Items"), TEXT("Tools"), TEXT("Weapons")};
    const TCHAR* Sorts[] = {TEXT("Owner order"), TEXT("Name"), TEXT("Category / name")};
    CastChecked<UTextBlock>(CategoryButton->GetContent())->SetText(FText::FromString(FString(TEXT("Category: ")) + Categories[Category]));
    CategoryButton->SetVisibility(bAllowToolFilter ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    CastChecked<UTextBlock>(SortButton->GetContent())->SetText(FText::FromString(FString(TEXT("Sort: ")) + Sorts[Sort]));
    CastChecked<UTextBlock>(ClearButton->GetContent())->SetText(FText::FromString(TEXT("Clear search")));
    CastChecked<UTextBlock>(ClearButton->GetContent())->SetAutoWrapText(false);
    for (UButton* Button : {CategoryButton.Get(), SortButton.Get(), ClearButton.Get()})
    {
        Theme.ApplyButton(*Button, ContrastMode);
        Theme.ApplyText(*CastChecked<UTextBlock>(Button->GetContent()), Theme.BodySize, false, TextScale, ContrastMode);
    }
    Results->SetText(FText::FromString(OwnerRows.IsEmpty() ? EmptyLabel : Rows.IsEmpty()
        ? FString::Printf(TEXT("No results. Clear search or view all of %s."), *ListLabel.ToLower())
        : FString::Printf(TEXT("Showing %d of %d owner-visible entries."), Rows.Num(), OwnerRows.Num())));
    Theme.ApplyText(*Results, Theme.BodySize, false, TextScale, ContrastMode);
    for (UButton* Button : Buttons)
    {
        Button->SetIsEnabled(!Rows.IsEmpty());
        Theme.ApplyButton(*Button, ContrastMode);
        Theme.ApplyText(*CastChecked<UTextBlock>(Button->GetContent()), Theme.BodySize, false, TextScale, ContrastMode);
    }
    const FLinearColor PanelFill = ContrastMode
        ? FLinearColor::Black : FLinearColor(.09f, .045f, .02f, .65f);
    const FLinearColor PanelBorder = ContrastMode
        ? FLinearColor::White : FLinearColor(.45f, .27f, .12f, 1.0f);
    for (UBorder* Frame : {ListFrame.Get(), DetailFrame.Get()})
    {
        if (bUseSplitLayout)
        {
            Frame->SetBrush(FSlateRoundedBoxBrush(PanelFill, 3.0f, PanelBorder, 1.0f));
            Frame->SetBrushColor(FLinearColor::White);
            Frame->SetPadding(FMargin(10.0f));
        }
        else
        {
            Frame->SetBrush(FSlateBrush());
            Frame->SetBrushColor(FLinearColor::Transparent);
            Frame->SetPadding(FMargin(0.0f));
        }
    }
    Grid->ClearChildren();
    int32 GridRow = 0, GridColumn = 0;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        if (Sort == 2 && (Index == 0 || EquipmentCategory(Rows[Index]) != EquipmentCategory(Rows[Index-1])))
        {
            if (GridColumn != 0) { ++GridRow; GridColumn = 0; }
            auto* Heading = WidgetTree->ConstructWidget<UTextBlock>();
            Heading->SetText(FText::FromString(Categories[EquipmentCategory(Rows[Index])]));
            Theme.ApplyText(*Heading, Theme.BodySize, true, TextScale, ContrastMode);
            Grid->AddChildToGrid(Heading, GridRow++, 0)->SetColumnSpan(GridColumns);
        }
        auto* Card = WidgetTree->ConstructWidget<UBorder>();
        Theme.ApplySelectablePanel(*Card, Index == Selected, Index == Selected && HasKeyboardFocus(), false, ContrastMode);
        auto* CardContent = WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
        IconSize->SetWidthOverride(28.0f);
        IconSize->SetHeightOverride(28.0f);
        auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
        Icon->SetCatalogueIcon(Rows[Index].Id);
        IconSize->SetContent(Icon);
        auto* IconSlot = CardContent->AddChildToHorizontalBox(IconSize);
        IconSlot->SetVerticalAlignment(VAlign_Center);
        IconSlot->SetPadding(FMargin(0.0f, 0.0f, Theme.SlotPadding, 0.0f));
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetAutoWrapText(true); Label->SetWrapTextAt(140.0f);
        Label->SetText(FText::FromString((!bUseSplitLayout && Index == Selected ? TEXT("> ") : TEXT(""))
            + Rows[Index].Name + TEXT("\n") + Rows[Index].Detail));
        Theme.ApplyText(*Label, Theme.BodySize, Index == Selected, TextScale, ContrastMode);
        auto* LabelSlot = CardContent->AddChildToHorizontalBox(Label);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Card->SetContent(CardContent); Grid->AddChildToGrid(Card, GridRow, GridColumn);
        if (++GridColumn == GridColumns) { ++GridRow; GridColumn = 0; }
    }
    Detail->SetVisibility(Rows.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    DetailFrame->SetVisibility(Rows.IsEmpty() && !bUseSplitLayout
        ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (Rows.IsValidIndex(Selected))
    {
        const auto& Row = Rows[Selected];
        const FString VisibleState = Row.Detail + TEXT("\n\n") + ActionGuidance(Row.Id, Row.bCarriedTool);
        if (Row.bCarriedTool) Detail->SetCarriedTool(Row.Id, Row.Name, VisibleState, TextScale, ContrastMode);
        else Detail->SetItem(Row.Id, Row.Name, VisibleState, TextScale, ContrastMode);
    }
}

void UKalmalaInventoryInspectWidget::Previous() { if (!Rows.IsEmpty()) { Selected = (Selected + Rows.Num()-1) % Rows.Num(); Refresh(); } }
void UKalmalaInventoryInspectWidget::Next() { if (!Rows.IsEmpty()) { Selected = (Selected+1) % Rows.Num(); Refresh(); } }
bool UKalmalaInventoryInspectWidget::Navigate(FKey Key)
{
    if (Key == EKeys::PageUp || Key == EKeys::Gamepad_LeftShoulder) { CycleCategory(); return true; }
    if (Key == EKeys::PageDown || Key == EKeys::Gamepad_RightShoulder) { CycleSort(); return true; }
    if (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up) { Previous(); return true; }
    if (Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down) { Next(); return true; }
    return false;
}
FReply UKalmalaInventoryInspectWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    // Text editing retains cursor keys. Tab/engine navigation reaches all controls.
    if (SearchBox && (SearchBox->HasKeyboardFocus() || SearchBox->HasFocusedDescendants()))
        return Super::NativeOnPreviewKeyDown(Geometry, Event);
    if (Navigate(Event.GetKey())) return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
