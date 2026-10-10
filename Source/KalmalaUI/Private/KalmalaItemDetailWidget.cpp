#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaIconWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

FString UKalmalaItemDetailWidget::DescribeItem(FName Id, const FString& VisibleState)
{
    const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Id);
    FString Text = Item && !Item->Description.IsEmpty() ? Item->Description : TEXT("Description unavailable.");
    if (!VisibleState.IsEmpty()) Text += TEXT("\n\n") + VisibleState;
    return Text;
}

FString UKalmalaItemDetailWidget::DescribeCarriedTool(FName Id, const FString& VisibleState)
{
    FString Description = TEXT("Description unavailable.");
    if (Id == TEXT("ReedKnife")) Description = TEXT("A light blade for gathering plants and reeds.");
    else if (Id == TEXT("FieldHatchet")) Description = TEXT("A hand hatchet for gathering wood.");
    else if (Id == TEXT("StonePick")) Description = TEXT("A stone-headed pick for mining.");
    else if (Id == TEXT("BronzeAxe")) Description = TEXT("A bronze axe for harvesting Lightwood.");
    else if (Id == TEXT("IronAxe")) Description = TEXT("An iron axe for harvesting Densewood.");
    else if (Id == TEXT("ConstructionHammer")) Description = TEXT("A hand hammer for placing camp structures.");

    FString Text = Description;
    if (!VisibleState.IsEmpty()) Text += TEXT("\n\n") + VisibleState;
    return Text;
}

TSharedRef<SWidget> UKalmalaItemDetailWidget::RebuildWidget()
{
    BuildPanel();
    return Super::RebuildWidget();
}

void UKalmalaItemDetailWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPanel();
}

void UKalmalaItemDetailWidget::BuildPanel()
{
    if (Panel || !WidgetTree) return;
    SetIsFocusable(false);
    Panel = WidgetTree->ConstructWidget<UBorder>();
    auto* Width = WidgetTree->ConstructWidget<USizeBox>();
    Width->SetWidthOverride(300.0f);
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
    IconSize->SetWidthOverride(64.0f); IconSize->SetHeightOverride(64.0f);
    Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
    IconSize->SetContent(Icon);
    Column->AddChildToVerticalBox(IconSize)->SetHorizontalAlignment(HAlign_Left);
    Title = WidgetTree->ConstructWidget<UTextBlock>(); Column->AddChild(Title);
    Title->SetAutoWrapText(true);
    Details = WidgetTree->ConstructWidget<UTextBlock>();
    Details->SetAutoWrapText(true); Column->AddChild(Details);
    Width->SetContent(Column); Panel->SetContent(Width); WidgetTree->RootWidget = Panel;
}

void UKalmalaItemDetailWidget::SetItem(FName Id, const FString& Name, const FString& VisibleState,
    const int32 TextScale, const int32 Contrast)
{
    SetPresentation(Id, Name, DescribeItem(Id, VisibleState), TextScale, Contrast);
}

void UKalmalaItemDetailWidget::SetCarriedTool(FName Id, const FString& Name, const FString& VisibleState,
    const int32 TextScale, const int32 Contrast)
{
    SetPresentation(Id, Name, DescribeCarriedTool(Id, VisibleState), TextScale, Contrast);
}

void UKalmalaItemDetailWidget::SetPresentation(FName Id, const FString& Name, const FString& Description,
    const int32 TextScale, const int32 Contrast)
{
    BuildPanel();
    if (!Panel) return;
    const auto& Theme = FKalmalaUITheme::Get();
    Theme.ApplySolidPanel(*Panel, Contrast);
    Panel->SetPadding(FMargin(8.0f));
    Theme.ApplyText(*Title, Theme.HeadingSize, true, TextScale, Contrast);
    Theme.ApplyText(*Details, Theme.BodySize, false, TextScale, Contrast);
    Title->SetText(FText::FromString(Name));
    Details->SetText(FText::FromString(Description));
    Icon->SetCatalogueIcon(Id);
}
