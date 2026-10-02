#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
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
void UKalmalaCatalogueRowsWidget::SetRows(const TArray<FKalmalaCatalogueRow>& Rows, int32 TextScale, int32 Contrast)
{
    if (!Column) return;
    FString Key = FString::Printf(TEXT("%d|%d"), TextScale, Contrast);
    for (const auto& R : Rows) Key += TEXT("|") + R.Id.ToString() + TEXT(":") + R.Text;
    if (Key == LastRows) return;
    Column->ClearChildren();
    const auto& Theme = FKalmalaUITheme::Get();
    for (const auto& R : Rows)
    {
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Box = WidgetTree->ConstructWidget<USizeBox>();
        Box->SetWidthOverride(FMath::Min(Theme.IconWidth, 32.0f)); Box->SetHeightOverride(FMath::Min(Theme.IconHeight, 32.0f));
        auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
        EKalmalaIcon Kind; int32 Variant;
        UKalmalaIconWidget::FindCatalogueIcon(R.Id, Kind, Variant); Icon->SetIcon(Kind, Variant);
        Box->SetContent(Icon); Row->AddChild(Box);
        auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(R.Text));
        Label->SetAutoWrapText(true); Label->SetWrapTextAt(260);
        Theme.ApplyText(*Label, Theme.BodySize, false, TextScale, Contrast);
        auto Font = Label->GetFont(); Font.OutlineSettings.OutlineSize=FMath::Max(1,Font.OutlineSettings.OutlineSize); Label->SetFont(Font);
        Row->AddChildToHorizontalBox(Label)->SetPadding(FMargin(Theme.RowSpacing, 0, 0, 0));
        Column->AddChild(Row);
    }
    LastRows = Key;
}
