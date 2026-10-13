#include "KalmalaItemDetailWidget.h"
#include "KalmalaToolCatalogue.h"
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
    if (Id == TEXT("StoneKnife")) Description = TEXT("A light stone blade for gathering plants and reeds.");
    else if (Id == TEXT("StoneAxe")) Description = TEXT("A stone hand axe for gathering wood.");
    else if (Id == TEXT("StonePick")) Description = TEXT("A stone-headed pick for mining.");
    else if (Id == TEXT("BronzeAxe")) Description = TEXT("A bronze axe for harvesting Lightwood.");
    else if (Id == TEXT("IronAxe")) Description = TEXT("An iron axe for harvesting Densewood.");
    else if (Id == TEXT("IronKnife")) Description = TEXT("A forged iron blade for gathering plants and reeds.");
    else if (Id == TEXT("IronPick")) Description = TEXT("A forged iron pick for mining ore and stone.");
    else if (Id == TEXT("IronSpear")) Description = TEXT("A long iron spear built for piercing attacks.");
    else if (Id == TEXT("StoneMace")) Description = TEXT("A stone mace for crushing attacks.");
    else if (Id == TEXT("BronzeMace")) Description = TEXT("A bronze mace for crushing attacks.");
    else if (Id == TEXT("IronMace")) Description = TEXT("An iron mace for crushing attacks.");
    else if (Id == TEXT("ConstructionHammer")) Description = TEXT("A hand hammer for placing camp structures.");

    const FKalmalaToolProfileData* Profile = UKalmalaToolCatalogue::Get()->FindProfile(Id);
    FString Text = FString(Profile && Profile->IsWeapon()
        ? TEXT("Weapon\n\n") : TEXT("Tool\n\n")) + Description;
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
    auto* IconTileSize = WidgetTree->ConstructWidget<USizeBox>();
    IconTileSize->SetWidthOverride(FKalmalaUITheme::Get().ItemIconTileSize);
    IconTileSize->SetHeightOverride(FKalmalaUITheme::Get().ItemIconTileSize);
    IconTile = WidgetTree->ConstructWidget<UBorder>();
    auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
    IconSize->SetWidthOverride(FKalmalaUITheme::Get().ItemIconTileSize);
    IconSize->SetHeightOverride(FKalmalaUITheme::Get().ItemIconTileSize);
    Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
    IconSize->SetContent(Icon);
    IconTile->SetContent(IconSize);
    IconTileSize->SetContent(IconTile);
    Column->AddChildToVerticalBox(IconTileSize)->SetHorizontalAlignment(HAlign_Left);
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
    Theme.ApplyItemIconTile(*IconTile, Contrast);
    Theme.ApplyText(*Title, Theme.HeadingSize, true, TextScale, Contrast);
    Theme.ApplyText(*Details, Theme.BodySize, false, TextScale, Contrast);
    Title->SetText(FText::FromString(Name));
    Details->SetText(FText::FromString(Description));
    Icon->SetCatalogueIcon(Id);
}
