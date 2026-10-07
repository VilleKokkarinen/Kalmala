#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "KalmalaInventoryComponent.h"

namespace
{
constexpr int32 PackColumns = 4;
constexpr int32 ToolColumns = 2;
const FString CatalogueNoPanelImage;

UBorder* MakeSlot(UWidgetTree& Tree, const FKalmalaCatalogueRow* Row,
    const bool bEmpty, const float SlotWidth, const int32 TextScale, const int32 Contrast)
{
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    UBorder* Card = Tree.ConstructWidget<UBorder>();
    Theme.ApplyPanel(*Card, Contrast, &CatalogueNoPanelImage);
    Card->SetPadding(FMargin(3.0f));
    Card->SetBrushColor(Contrast == 0
        ? (bEmpty ? FLinearColor(0.035f, 0.048f, 0.055f, 0.92f) : FLinearColor(0.07f, 0.095f, 0.11f, 0.98f))
        : FLinearColor::White);

    USizeBox* Size = Tree.ConstructWidget<USizeBox>();
    Size->SetWidthOverride(SlotWidth);
    Size->SetMinDesiredHeight(bEmpty ? 74.0f : 86.0f);
    UVerticalBox* Content = Tree.ConstructWidget<UVerticalBox>();

    if (!bEmpty && Row)
    {
        USizeBox* IconBox = Tree.ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(Row->bCarriedTool ? 32.0f : 28.0f);
        IconBox->SetHeightOverride(Row->bCarriedTool ? 32.0f : 28.0f);
        UKalmalaIconWidget* Icon = Tree.ConstructWidget<UKalmalaIconWidget>();
        EKalmalaIcon Kind;
        int32 Variant;
        UKalmalaIconWidget::FindCatalogueIcon(Row->Id, Kind, Variant);
        Icon->SetIcon(Kind, Variant);
        IconBox->SetContent(Icon);
        UVerticalBoxSlot* IconSlot = Content->AddChildToVerticalBox(IconBox);
        IconSlot->SetHorizontalAlignment(HAlign_Center);
        IconSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
    }

    UTextBlock* Name = Tree.ConstructWidget<UTextBlock>();
    Name->SetText(FText::FromString(bEmpty ? TEXT("Empty") : Row->Name));
    Name->SetJustification(ETextJustify::Center);
    Name->SetAutoWrapText(bEmpty || !Row->bCarriedTool);
    Name->SetWrapTextAt(SlotWidth - 8.0f);
    Theme.ApplyText(*Name, 9, !bEmpty && Row->bCarriedTool, TextScale, Contrast);
    Content->AddChild(Name);

    if (!bEmpty && Row && !Row->Detail.IsEmpty())
    {
        UTextBlock* Detail = Tree.ConstructWidget<UTextBlock>();
        Detail->SetText(FText::FromString(Row->Detail));
        Detail->SetJustification(ETextJustify::Center);
        Detail->SetAutoWrapText(!Row->bCarriedTool);
        Detail->SetWrapTextAt(SlotWidth - 8.0f);
        Theme.ApplyText(*Detail, 8, false, TextScale, Contrast);
        UVerticalBoxSlot* DetailSlot = Content->AddChildToVerticalBox(Detail);
        if (Row->bCarriedTool) DetailSlot->SetPadding(FMargin(0.0f, Theme.RowSpacing, 0.0f, 0.0f));
    }

    Size->SetContent(Content);
    Card->SetContent(Size);
    if (bEmpty)
    {
        Card->SetToolTipText(FText::FromString(TEXT("Empty pack slot.")));
    }
    else if (Row)
    {
        auto* ItemDetail = Tree.ConstructWidget<UKalmalaItemDetailWidget>();
        ItemDetail->SetItem(Row->Id, Row->Name, Row->Detail, TextScale, Contrast);
        Card->SetToolTip(ItemDetail);
    }
    return Card;
}
}

FString UKalmalaCatalogueRowsWidget::BuildToolDetail(const int32 Level, const int32 Condition, const int32 MaximumCondition)
{
    if (Level <= 0 || MaximumCondition <= 0 || Condition < 0 || Condition > MaximumCondition)
        return TEXT("Tool state unavailable");
    const TCHAR* State = Condition == 0 ? TEXT("BROKEN")
        : Condition < MaximumCondition ? TEXT("DAMAGED") : TEXT("READY");
    return FString::Printf(TEXT("Level %d\nCondition %d/%d\n%s"), Level, Condition, MaximumCondition, State);
}

#if !UE_BUILD_SHIPPING
void UKalmalaCatalogueRowsWidget::SetVerificationBackground()
{
    auto* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(FLinearColor::Black);
    Background->SetPadding(FMargin(4)); Background->SetContent(Column);
    WidgetTree->RootWidget=Background;
}
#endif
void UKalmalaCatalogueRowsWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized(); SetIsFocusable(false);
    Column = WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget = Column;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaCatalogueRowsWidget::SetRows(const TArray<FKalmalaCatalogueRow>& Rows, const int32 InSlotCapacity,
    const int32 TextScale, const int32 Contrast)
{
    if (!Column) return;
    const int32 BoundedCapacity = FMath::Clamp(InSlotCapacity, 0, UKalmalaInventoryComponent::MaxSlots);
    FString Key = FString::Printf(TEXT("%d|%d|%d"), BoundedCapacity, TextScale, Contrast);
    for (const auto& R : Rows)
    {
        Key += TEXT("|") + R.Id.ToString() + TEXT(":") + R.Name + TEXT(":") + R.Detail
            + (R.bCarriedTool ? TEXT(":tool") : TEXT(":item"));
    }
    if (Key == LastRows) return;
    Column->ClearChildren();
    const auto& Theme = FKalmalaUITheme::Get();

    TArray<const FKalmalaCatalogueRow*> PackRows;
    TArray<const FKalmalaCatalogueRow*> ToolRows;
    for (const FKalmalaCatalogueRow& Row : Rows)
    {
        (Row.bCarriedTool ? ToolRows : PackRows).Add(&Row);
    }

    SlotCapacity = BoundedCapacity;
    FilledSlotCount = FMath::Min(PackRows.Num(), SlotCapacity);
    CarriedToolCount = ToolRows.Num();

    UTextBlock* PackHeading = WidgetTree->ConstructWidget<UTextBlock>();
    PackHeading->SetText(FText::FromString(FString::Printf(TEXT("PACK SLOTS · %d/%d"), FilledSlotCount, SlotCapacity)));
    Theme.ApplyText(*PackHeading, Theme.HeadingSize, true, TextScale, Contrast);
    Column->AddChild(PackHeading);

    UUniformGridPanel* PackGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    Column->AddChild(PackGrid);
    const float PackSlotWidth = 70.0f;
    for (int32 Index = 0; Index < SlotCapacity; ++Index)
    {
        const FKalmalaCatalogueRow* Row = PackRows.IsValidIndex(Index) ? PackRows[Index] : nullptr;
        UBorder* Card = MakeSlot(*WidgetTree, Row, Row == nullptr, PackSlotWidth, TextScale, Contrast);
        UBorder* CellMargin = WidgetTree->ConstructWidget<UBorder>();
        CellMargin->SetBrushColor(FLinearColor::Transparent);
        CellMargin->SetPadding(FMargin(2.0f));
        CellMargin->SetContent(Card);
        UUniformGridSlot* GridPanelSlot = PackGrid->AddChildToUniformGrid(CellMargin, Index / PackColumns, Index % PackColumns);
        GridPanelSlot->SetHorizontalAlignment(HAlign_Fill);
        GridPanelSlot->SetVerticalAlignment(VAlign_Fill);
    }

    if (!ToolRows.IsEmpty())
    {
        UTextBlock* ToolHeading = WidgetTree->ConstructWidget<UTextBlock>();
        ToolHeading->SetText(FText::FromString(TEXT("CARRIED TOOLS")));
        Theme.ApplyText(*ToolHeading, Theme.HeadingSize, true, TextScale, Contrast);
        Column->AddChild(ToolHeading);

        UUniformGridPanel* ToolGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
        Column->AddChild(ToolGrid);
        for (int32 Index = 0; Index < ToolRows.Num(); ++Index)
        {
            UBorder* Card = MakeSlot(*WidgetTree, ToolRows[Index], false, 154.0f, TextScale, Contrast);
            UBorder* CellMargin = WidgetTree->ConstructWidget<UBorder>();
            CellMargin->SetBrushColor(FLinearColor::Transparent);
            CellMargin->SetPadding(FMargin(2.0f));
            CellMargin->SetContent(Card);
            UUniformGridSlot* GridPanelSlot = ToolGrid->AddChildToUniformGrid(CellMargin, Index / ToolColumns, Index % ToolColumns);
            GridPanelSlot->SetHorizontalAlignment(HAlign_Fill);
            GridPanelSlot->SetVerticalAlignment(VAlign_Fill);
        }
    }
    LastRows = Key;
}

void UKalmalaCatalogueRowsWidget::SetRows(const TArray<FKalmalaCatalogueRow>& Rows,
    const int32 TextScale, const int32 Contrast)
{
    if (!Column) return;
    FString Key = FString::Printf(TEXT("gallery|%d|%d"), TextScale, Contrast);
    for (const FKalmalaCatalogueRow& Row : Rows)
    {
        Key += TEXT("|") + Row.Id.ToString() + TEXT(":") + Row.Name;
    }
    if (Key == LastRows) return;

    Column->ClearChildren();
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
    Heading->SetText(FText::FromString(TEXT("CATALOGUE ICONS")));
    Theme.ApplyText(*Heading, Theme.HeadingSize, true, TextScale, Contrast);
    Column->AddChild(Heading);

    UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>();
    Column->AddChild(Grid);
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        UBorder* Card = MakeSlot(*WidgetTree, &Rows[Index], false, 70.0f, TextScale, Contrast);
        UBorder* CellMargin = WidgetTree->ConstructWidget<UBorder>();
        CellMargin->SetBrushColor(FLinearColor::Transparent);
        CellMargin->SetPadding(FMargin(2.0f));
        CellMargin->SetContent(Card);
        UUniformGridSlot* GridPanelSlot = Grid->AddChildToUniformGrid(CellMargin, Index / PackColumns, Index % PackColumns);
        GridPanelSlot->SetHorizontalAlignment(HAlign_Fill);
        GridPanelSlot->SetVerticalAlignment(VAlign_Fill);
    }

    SlotCapacity = 0;
    FilledSlotCount = Rows.Num();
    CarriedToolCount = 0;
    LastRows = Key;
}
