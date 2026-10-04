#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaIconWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

FString UKalmalaItemDetailWidget::DescribeItem(FName Id, const FString& VisibleState)
{
    const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Id);
    FString Text = Item && !Item->Description.IsEmpty() ? Item->Description : TEXT("Description unavailable.");
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
    IconSize->SetContent(Icon); Column->AddChild(IconSize);
    Title = WidgetTree->ConstructWidget<UTextBlock>(); Column->AddChild(Title);
    Title->SetAutoWrapText(true);
    Details = WidgetTree->ConstructWidget<UTextBlock>();
    Details->SetAutoWrapText(true); Column->AddChild(Details);
    Width->SetContent(Column); Panel->SetContent(Width); WidgetTree->RootWidget = Panel;
}

void UKalmalaItemDetailWidget::SetItem(FName Id, const FString& Name, const FString& VisibleState,
    const int32 TextScale, const int32 Contrast)
{
    BuildPanel();
    if (!Panel) return;
    const auto& Theme = FKalmalaUITheme::Get();
    const FString NoImage;
    Theme.ApplyPanel(*Panel, Contrast, &NoImage);
    Theme.ApplyText(*Title, Theme.HeadingSize, true, TextScale, Contrast);
    Theme.ApplyText(*Details, Theme.BodySize, false, TextScale, Contrast);
    Title->SetText(FText::FromString(Name));
    Details->SetText(FText::FromString(DescribeItem(Id, VisibleState)));
    EKalmalaIcon Kind; int32 Variant;
    UKalmalaIconWidget::FindCatalogueIcon(Id, Kind, Variant);
    Icon->SetIcon(Kind, Variant);
}
