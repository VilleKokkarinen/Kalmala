#include "KalmalaIngredientWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

TSharedRef<SWidget> UKalmalaIngredientWidget::RebuildWidget()
{
    if (!Rows && WidgetTree)
    {
        Rows = WidgetTree->ConstructWidget<UVerticalBox>();
        WidgetTree->RootWidget = Rows;
        SetIsFocusable(false);
    }
    return Super::RebuildWidget();
}

void UKalmalaIngredientWidget::SetIngredients(const TArray<FKalmalaInventoryStack>& Costs,
    const UKalmalaInventoryComponent* Inventory, int32 TextScale, int32 Contrast)
{
    if (!WidgetTree) Initialize();
    if (!Rows) TakeWidget();
    if (!Rows) return;
    TArray<FString> Labels;
    FString Next = Costs.IsEmpty() ? FString() : TEXT("Ingredients — one craft/build\n");
    for (const auto& Cost : Costs)
    {
        const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Cost.ItemId);
        const FString Name = Item ? Item->DisplayName : Cost.ItemId.ToString();
        const int32 Owned = Inventory ? Inventory->GetQuantity(Cost.ItemId) : 0;
        const FString Label = Inventory
            ? FString::Printf(TEXT("%s: owned %d / required %d — %s"), *Name, Owned, Cost.Quantity,
                Owned >= Cost.Quantity ? TEXT("Enough") : TEXT("Missing"))
            : FString::Printf(TEXT("%s: owned pending / required %d — Waiting for pack"), *Name, Cost.Quantity);
        Labels.Add(Label);
        // Include identity as well as text so a same-name replacement refreshes its icon.
        Next += Cost.ItemId.ToString() + TEXT(" | ") + Label + TEXT("\n");
    }
    if (Next == Presentation && LastScale == TextScale && LastContrast == Contrast) return;
    Presentation = Next; LastScale = TextScale; LastContrast = Contrast;
    Rows->ClearChildren();
    const auto& Theme = FKalmalaUITheme::Get();
    if (!Costs.IsEmpty())
    {
        auto* Heading = WidgetTree->ConstructWidget<UTextBlock>();
        Heading->SetText(FText::FromString(TEXT("Ingredients — one craft/build")));
        Heading->SetAutoWrapText(true);
        Theme.ApplyText(*Heading, Theme.BodySize, true, TextScale, Contrast);
        Rows->AddChild(Heading);
    }
    for (int32 Index = 0; Index < Costs.Num(); ++Index)
    {
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Box = WidgetTree->ConstructWidget<USizeBox>();
        Theme.ApplyIconSlot(*Box);
        auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
        EKalmalaIcon Kind; int32 Variant;
        UKalmalaIconWidget::FindCatalogueIcon(Costs[Index].ItemId, Kind, Variant);
        Icon->SetIcon(Kind, Variant); Box->SetContent(Icon); Row->AddChild(Box);
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Labels[Index])); Label->SetAutoWrapText(true);
        Theme.ApplyText(*Label, Theme.BodySize, false, TextScale, Contrast);
        auto* LabelSlot = Row->AddChildToHorizontalBox(Label);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetPadding(FMargin(Theme.SlotPadding, 0));
        Rows->AddChild(Row);
    }
}
