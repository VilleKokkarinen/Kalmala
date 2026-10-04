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
#include "InputCoreTypes.h"

void UKalmalaInventoryInspectWidget::Build()
{
    if (Column || !WidgetTree) return;
    SetIsFocusable(true);
    Column = WidgetTree->ConstructWidget<UVerticalBox>();
    WidgetTree->RootWidget = Column;
    Instructions = WidgetTree->ConstructWidget<UTextBlock>();
    Instructions->SetAutoWrapText(true); Column->AddChild(Instructions);
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
    const FName Old = GetSelectedItem();
    Rows = InRows; TextScale = Scale; ContrastMode = Contrast;
    const int32 Found = Rows.IndexOfByPredicate([Old](const auto& Row) { return Row.Id == Old; });
    Selected = Found == INDEX_NONE ? FMath::Clamp(Selected, 0, FMath::Max(0, Rows.Num()-1)) : Found;
    Build(); Refresh();
}

FName UKalmalaInventoryInspectWidget::GetSelectedItem() const
{
    return Rows.IsValidIndex(Selected) ? Rows[Selected].Id : NAME_None;
}

void UKalmalaInventoryInspectWidget::Refresh()
{
    if (!Column) return;
    const auto& Theme = FKalmalaUITheme::Get();
    Instructions->SetText(FText::FromString(TEXT("YOUR INVENTORY DETAILS — Tab to inspection controls; arrows / D-pad select while focused. > marks the inspected slot. Escape / B closes the menu.")));
    Theme.ApplyText(*Instructions, Theme.BodySize, false, TextScale, ContrastMode);
    for (UButton* Button : Buttons)
    {
        Theme.ApplyButton(*Button, ContrastMode);
        Theme.ApplyText(*CastChecked<UTextBlock>(Button->GetContent()), Theme.BodySize, false, TextScale, ContrastMode);
        Button->SetIsEnabled(!Rows.IsEmpty());
    }
    Grid->ClearChildren();
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        auto* Card = WidgetTree->ConstructWidget<UBorder>();
        const FString NoImage;
        Theme.ApplyPanel(*Card, ContrastMode, &NoImage);
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetAutoWrapText(true); Label->SetWrapTextAt(140.0f);
        Label->SetText(FText::FromString((Index == Selected ? TEXT("> ") : TEXT("")) + Rows[Index].Name + TEXT("\n") + Rows[Index].Detail));
        Theme.ApplyText(*Label, Theme.BodySize, Index == Selected, TextScale, ContrastMode);
        Card->SetContent(Label); Grid->AddChildToUniformGrid(Card, Index / 4, Index % 4);
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
    if (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up) { Previous(); return true; }
    if (Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down) { Next(); return true; }
    return false;
}
FReply UKalmalaInventoryInspectWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Navigate(Event.GetKey())) return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
