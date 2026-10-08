#include "KalmalaSelectedResultWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

int32 UKalmalaSelectedResultWidget::GetPreviewIconExtent()
{
    return 88;
}

bool UKalmalaSelectedResultWidget::FindCanonicalIcon(
    const FName OutputId, EKalmalaIcon& OutIcon, int32& OutVariant)
{
    return UKalmalaIconWidget::FindCatalogueIcon(OutputId, OutIcon, OutVariant);
}

TSharedRef<SWidget> UKalmalaSelectedResultWidget::RebuildWidget()
{
    BuildPanel();
    return Super::RebuildWidget();
}

void UKalmalaSelectedResultWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BuildPanel();
}

void UKalmalaSelectedResultWidget::BuildPanel()
{
    if (Panel || !WidgetTree) return;

    SetIsFocusable(false);
    Panel = WidgetTree->ConstructWidget<UBorder>();
    Panel->SetPadding(FMargin(8.0f));

    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    IconSize = WidgetTree->ConstructWidget<USizeBox>();
    IconSize->SetWidthOverride(static_cast<float>(GetPreviewIconExtent()));
    IconSize->SetHeightOverride(static_cast<float>(GetPreviewIconExtent()));
    Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
    IconSize->SetContent(Icon);
    auto* IconSlot = Row->AddChildToHorizontalBox(IconSize);
    IconSlot->SetVerticalAlignment(VAlign_Top);
    IconSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

    auto* Details = WidgetTree->ConstructWidget<UVerticalBox>();
    auto AddLabel = [this, Details](TObjectPtr<UTextBlock>& Target, const bool bAutoWrap)
    {
        Target = WidgetTree->ConstructWidget<UTextBlock>();
        Target->SetAutoWrapText(bAutoWrap);
        Details->AddChild(Target);
    };
    AddLabel(Title, true);
    AddLabel(Description, true);
    AddLabel(PreviewStatus, true);
    AddLabel(Requirements, true);

    auto* DetailsSlot = Row->AddChildToHorizontalBox(Details);
    DetailsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    DetailsSlot->SetVerticalAlignment(VAlign_Top);
    Panel->SetContent(Row);
    WidgetTree->RootWidget = Panel;
    Panel->SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaSelectedResultWidget::SetResult(const FName InOutputId, const FString& Name,
    const FString& InDescription, const FString& InRequirements,
    const int32 TextScalePercent, const int32 ContrastMode)
{
    BuildPanel();
    if (!Panel || InOutputId.IsNone())
    {
        ClearResult();
        return;
    }

    const bool bIdentityChanged = OutputId != InOutputId;
    const bool bTextChanged = CurrentName != Name || CurrentDescription != InDescription
        || CurrentRequirements != InRequirements;
    const bool bStyleChanged = LastTextScalePercent != TextScalePercent || LastContrastMode != ContrastMode;
    if (bIdentityChanged || bTextChanged)
    {
        OutputId = InOutputId;
        CurrentName = Name;
        CurrentDescription = InDescription;
        CurrentRequirements = InRequirements;
        EKalmalaIcon KindTemp = EKalmalaIcon::Unknown;
        int32 VariantTemp = 0;
        bHasCanonicalIcon = FindCanonicalIcon(OutputId, KindTemp, VariantTemp);
        if (Icon)
        {
            Icon->SetIcon(bHasCanonicalIcon ? KindTemp : EKalmalaIcon::Unknown,
                bHasCanonicalIcon ? VariantTemp : 0);
            Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        if (Title) Title->SetText(FText::FromString(Name));
        if (Description) Description->SetText(FText::FromString(InDescription));
        if (Requirements) Requirements->SetText(FText::FromString(InRequirements));
        if (PreviewStatus)
        {
            PreviewStatus->SetText(bHasCanonicalIcon
                ? FText::GetEmpty()
                : FText::FromString(TEXT("Preview icon unavailable for this result.")));
            PreviewStatus->SetVisibility(bHasCanonicalIcon
                ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        }
    }

    if (bStyleChanged)
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        const FString NoImage;
        Theme.ApplyPanel(*Panel, ContrastMode, &NoImage);
        if (Title) Theme.ApplyText(*Title, Theme.HeadingSize, true, TextScalePercent, ContrastMode);
        if (Description) Theme.ApplyText(*Description, Theme.BodySize, false, TextScalePercent, ContrastMode);
        if (PreviewStatus) Theme.ApplyText(*PreviewStatus, Theme.BodySize, true, TextScalePercent, ContrastMode);
        if (Requirements) Theme.ApplyText(*Requirements, Theme.BodySize, false, TextScalePercent, ContrastMode);
        LastTextScalePercent = TextScalePercent;
        LastContrastMode = ContrastMode;
    }

    Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaSelectedResultWidget::ClearResult()
{
    OutputId = NAME_None;
    CurrentName.Reset();
    CurrentDescription.Reset();
    CurrentRequirements.Reset();
    bHasCanonicalIcon = false;
    if (Title) Title->SetText(FText::GetEmpty());
    if (Description) Description->SetText(FText::GetEmpty());
    if (Requirements) Requirements->SetText(FText::GetEmpty());
    if (PreviewStatus)
    {
        PreviewStatus->SetText(FText::GetEmpty());
        PreviewStatus->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (Panel) Panel->SetVisibility(ESlateVisibility::Collapsed);
    SetVisibility(ESlateVisibility::Collapsed);
}

bool UKalmalaSelectedResultWidget::IsShowingResult() const
{
    return !OutputId.IsNone() && GetVisibility() != ESlateVisibility::Collapsed
        && Panel && Panel->GetVisibility() != ESlateVisibility::Collapsed;
}

FString UKalmalaSelectedResultWidget::GetDescriptionText() const
{
    return Description ? Description->GetText().ToString() : FString();
}

FString UKalmalaSelectedResultWidget::GetRequirementsText() const
{
    return Requirements ? Requirements->GetText().ToString() : FString();
}

FString UKalmalaSelectedResultWidget::GetPresentationText() const
{
    if (OutputId.IsNone()) return FString();
    FString Text = CurrentName + TEXT("\n") + GetDescriptionText() + TEXT("\n") + GetRequirementsText();
    if (!bHasCanonicalIcon) Text += TEXT("\nPreview icon unavailable for this result.");
    return Text;
}

FVector2D UKalmalaSelectedResultWidget::GetConfiguredPreviewIconSize() const
{
    return IconSize
        ? FVector2D(IconSize->GetWidthOverride(), IconSize->GetHeightOverride())
        : FVector2D::ZeroVector;
}
