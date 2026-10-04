#include "KalmalaInventoryInspectWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "InputCoreTypes.h"

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
    auto* Browse = WidgetTree->ConstructWidget<UVerticalBox>(); Column->AddChild(Browse);
    auto MakeControl = [this, Browse]()
    {
        auto* Button = WidgetTree->ConstructWidget<UButton>();
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetAutoWrapText(true); Button->SetContent(Label); Browse->AddChild(Button);
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
        auto* Button = WidgetTree->ConstructWidget<UButton>();
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Name)); Button->SetContent(Label);
        Actions->AddChild(Button); Buttons.Add(Button);
    }
    Buttons[0]->OnClicked.AddDynamic(this, &ThisClass::Previous);
    Buttons[1]->OnClicked.AddDynamic(this, &ThisClass::Next);
    Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    Column->AddChild(Grid);
    Detail = WidgetTree->ConstructWidget<UKalmalaItemDetailWidget>(); Column->AddChild(Detail);
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

void UKalmalaInventoryInspectWidget::SetRows(const TArray<FKalmalaCatalogueRow>& InRows, int32 Scale, int32 Contrast)
{
    FString Key = FString::Printf(TEXT("%d|%d"), Scale, Contrast);
    for (const auto& Row : InRows) Key += TEXT("|") + Row.Id.ToString() + Row.Name + Row.Detail + (Row.bCarriedTool ? TEXT("T") : TEXT("I"));
    if (Column && Key == LastRows) return;
    LastRows = Key;
    OwnerRows = InRows; TextScale = Scale; ContrastMode = Contrast;
    Build(); Refilter();
}

void UKalmalaInventoryInspectWidget::SetSearch(const FString& Query)
{
    Search = Query.Left(64).TrimStartAndEnd();
    Build(); Refilter();
}
void UKalmalaInventoryInspectWidget::SearchChanged(const FText& Text) { SetSearch(Text.ToString()); }
void UKalmalaInventoryInspectWidget::SetCategory(int32 Value) { Category = FMath::Clamp(Value, 0, 2); Build(); Refilter(); }
void UKalmalaInventoryInspectWidget::SetSort(int32 Value) { Sort = FMath::Clamp(Value, 0, 2); Build(); Refilter(); }
void UKalmalaInventoryInspectWidget::CycleCategory() { SetCategory((Category + 1) % 3); }
void UKalmalaInventoryInspectWidget::CycleSort() { SetSort((Sort + 1) % 3); }
void UKalmalaInventoryInspectWidget::ClearSearch() { SearchBox->SetText(FText::GetEmpty()); SetSearch(TEXT("")); }
void UKalmalaInventoryInspectWidget::Refilter()
{
    const FName Old = GetSelectedItem();
    Rows.Reset();
    for (const auto& Row : OwnerRows)
        if ((Category == 0 || Row.bCarriedTool == (Category == 2))
            && (Search.IsEmpty() || Row.Name.Contains(Search, ESearchCase::IgnoreCase))) Rows.Add(Row);
    if (Sort != 0)
        Rows.StableSort([this](const auto& A, const auto& B)
        {
            if (Sort == 2 && A.bCarriedTool != B.bCarriedTool) return !A.bCarriedTool;
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
    Instructions->SetText(FText::FromString(TEXT("YOUR INVENTORY DETAILS — Tab to search/controls; arrows / D-pad select. Page Up / left shoulder cycles category; Page Down / right shoulder cycles sort. > marks selection. Escape / B closes.")));
    Theme.ApplyText(*Instructions, Theme.BodySize, false, TextScale, ContrastMode);
    BrowseSearchStyle = SearchBox->GetWidgetStyle();
    BrowseSearchStyle.SetFont(Theme.MakeFont(Theme.BodySize, false, TextScale));
    BrowseSearchStyle.SetBackgroundColor(ContrastMode != 0 ? Theme.HighContrastPanel : Theme.ButtonNormal);
    SearchBox->SetWidgetStyle(BrowseSearchStyle);
    SearchBox->SetForegroundColor(Theme.TextColor(false, ContrastMode));
    const TCHAR* Categories[] = {TEXT("All"), TEXT("Items"), TEXT("Carried tools")};
    const TCHAR* Sorts[] = {TEXT("Owner order"), TEXT("Name"), TEXT("Category / name")};
    CastChecked<UTextBlock>(CategoryButton->GetContent())->SetText(FText::FromString(FString(TEXT("Category: ")) + Categories[Category] + TEXT(" (activate to cycle)")));
    CastChecked<UTextBlock>(SortButton->GetContent())->SetText(FText::FromString(FString(TEXT("Sort: ")) + Sorts[Sort] + TEXT(" (activate to cycle)")));
    CastChecked<UTextBlock>(ClearButton->GetContent())->SetText(FText::FromString(TEXT("Clear search")));
    CastChecked<UTextBlock>(ClearButton->GetContent())->SetAutoWrapText(false);
    for (UButton* Button : {CategoryButton.Get(), SortButton.Get(), ClearButton.Get()})
    {
        Theme.ApplyButton(*Button, ContrastMode);
        Theme.ApplyText(*CastChecked<UTextBlock>(Button->GetContent()), Theme.BodySize, false, TextScale, ContrastMode);
    }
    Results->SetText(FText::FromString(OwnerRows.IsEmpty() ? TEXT("Your inventory is empty.") : Rows.IsEmpty()
        ? TEXT("No results. Clear search or choose All to see your inventory.")
        : FString::Printf(TEXT("Showing %d of %d owner-visible entries."), Rows.Num(), OwnerRows.Num())));
    Theme.ApplyText(*Results, Theme.BodySize, false, TextScale, ContrastMode);
    for (UButton* Button : Buttons)
    {
        Theme.ApplyButton(*Button, ContrastMode);
        Theme.ApplyText(*CastChecked<UTextBlock>(Button->GetContent()), Theme.BodySize, false, TextScale, ContrastMode);
        Button->SetIsEnabled(!Rows.IsEmpty());
    }
    Grid->ClearChildren();
    int32 GridRow = 0, GridColumn = 0;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        if (Sort == 2 && (Index == 0 || Rows[Index].bCarriedTool != Rows[Index-1].bCarriedTool))
        {
            if (GridColumn != 0) { ++GridRow; GridColumn = 0; }
            auto* Heading = WidgetTree->ConstructWidget<UTextBlock>();
            Heading->SetText(FText::FromString(Rows[Index].bCarriedTool ? TEXT("Carried tools") : TEXT("Items")));
            Theme.ApplyText(*Heading, Theme.BodySize, true, TextScale, ContrastMode);
            Grid->AddChildToUniformGrid(Heading, GridRow++, 0);
        }
        auto* Card = WidgetTree->ConstructWidget<UBorder>();
        const FString NoImage;
        Theme.ApplyPanel(*Card, ContrastMode, &NoImage);
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetAutoWrapText(true); Label->SetWrapTextAt(140.0f);
        Label->SetText(FText::FromString((Index == Selected ? TEXT("> ") : TEXT("")) + Rows[Index].Name + TEXT("\n") + Rows[Index].Detail));
        Theme.ApplyText(*Label, Theme.BodySize, Index == Selected, TextScale, ContrastMode);
        Card->SetContent(Label); Grid->AddChildToUniformGrid(Card, GridRow, GridColumn);
        if (++GridColumn == 4) { ++GridRow; GridColumn = 0; }
    }
    Detail->SetVisibility(Rows.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (Rows.IsValidIndex(Selected))
    {
        const auto& Row = Rows[Selected];
        Detail->SetItem(Row.Id, Row.Name, Row.Detail + TEXT("\n\n") + ActionGuidance(Row.Id, Row.bCarriedTool), TextScale, ContrastMode);
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
