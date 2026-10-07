#include "KalmalaInventoryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"

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
    UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>();
    Heading->SetText(FText::FromString(TEXT("Inventory")));
    Theme.ApplyText(*Heading, Theme.EmphasisSize, true, TextScale, Contrast);
    Content->AddChildToVerticalBox(Heading);

    UTextBlock* Section = WidgetTree->ConstructWidget<UTextBlock>();
    Section->SetText(FText::FromString(TEXT("Pack and carried equipment")));
    Theme.ApplyText(*Section, Theme.BodySize, false, TextScale, Contrast);
    Content->AddChildToVerticalBox(Section);

    Panel->SetContent(Content);
    USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
    PanelSize->SetWidthOverride(620.0f);
    PanelSize->SetHeightOverride(260.0f);
    PanelSize->SetContent(Panel);

    UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSize);
    PanelSlot->SetHorizontalAlignment(HAlign_Center);
    PanelSlot->SetVerticalAlignment(VAlign_Center);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaInventoryMenuWidget::Open()
{
    bMenuOpen = true;
    SetVisibility(ESlateVisibility::Visible);
    SetKeyboardFocus();
}

void UKalmalaInventoryMenuWidget::Close()
{
    bMenuOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);
}
