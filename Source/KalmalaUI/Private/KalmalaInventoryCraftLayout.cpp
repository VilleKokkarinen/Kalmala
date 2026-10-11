#include "KalmalaCraftingSubsystem.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaIconWidget.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

FVector2D UKalmalaCraftingWidget::GetInventoryCraftPanelSize(const FVector2D Viewport)
{
    return FVector2D(FMath::Min(860.f, FMath::Max(80.f, Viewport.X * .52f)),
        FMath::Min(760.f, FMath::Max(80.f, Viewport.Y - 32.f)));
}

void UKalmalaCraftingWidget::BuildInventoryCraftLayout()
{
    auto* Root = WidgetTree->ConstructWidget<UOverlay>();
    auto* ServiceSlot = Root->AddChildToOverlay(MenuBackground);
    ServiceSlot->SetHorizontalAlignment(HAlign_Fill);
    ServiceSlot->SetVerticalAlignment(VAlign_Fill);
    InventoryCraftBackground = WidgetTree->ConstructWidget<UBorder>();
    auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
    InventoryCraftBackground->SetContent(Column);
    auto* CraftSlot = Root->AddChildToOverlay(InventoryCraftBackground);
    CraftSlot->SetHorizontalAlignment(HAlign_Fill);
    CraftSlot->SetVerticalAlignment(VAlign_Fill);
    WidgetTree->RootWidget = Root;
    InventoryCraftBackground->SetVisibility(ESlateVisibility::Collapsed);

    auto* SearchRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(SearchRow)->SetPadding(FMargin(0, 0, 0, 8));
    InventoryCraftSearch = WidgetTree->ConstructWidget<UEditableTextBox>();
    InventoryCraftSearch->SetHintText(FText::FromString(TEXT("Search items")));
    InventoryCraftSearch->OnTextChanged.AddDynamic(this, &ThisClass::RecipeSearchChanged);
    auto* SearchSize = WidgetTree->ConstructWidget<USizeBox>();
    SearchSize->SetWidthOverride(200);
    SearchSize->SetMinDesiredHeight(32);
    SearchSize->SetContent(InventoryCraftSearch);
    SearchRow->AddChild(SearchSize);
    InventoryCraftClear = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    auto* ClearText = WidgetTree->ConstructWidget<UTextBlock>();
    ClearText->SetText(FText::FromString(TEXT("x")));
    ClearText->SetJustification(ETextJustify::Center);
    InventoryCraftClear->SetContent(ClearText);
    InventoryCraftClear->SetToolTipText(FText::FromString(TEXT("Clear search")));
    InventoryCraftClear->OnClicked.AddDynamic(this, &ThisClass::ClearRecipeSearch);
    auto* ClearSize = WidgetTree->ConstructWidget<USizeBox>();
    ClearSize->SetWidthOverride(32);
    ClearSize->SetContent(InventoryCraftClear);
    SearchRow->AddChildToHorizontalBox(ClearSize)->SetPadding(FMargin(4, 0, 0, 0));

    auto* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
    Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* ListScroll = WidgetTree->ConstructWidget<UScrollBox>();
    FKalmalaUITheme::Get().ApplyScroll(*ListScroll);
    InventoryCraftList = WidgetTree->ConstructWidget<UVerticalBox>();
    ListScroll->AddChild(InventoryCraftList);
    InventoryCraftListFrame = WidgetTree->ConstructWidget<UBorder>();
    InventoryCraftListFrame->SetContent(ListScroll);
    auto* ListSlot = Split->AddChildToHorizontalBox(InventoryCraftListFrame);
    FSlateChildSize ListSize(ESlateSizeRule::Fill);
    ListSize.Value = 0.36f;
    ListSlot->SetSize(ListSize);
    ListSlot->SetPadding(FMargin(0, 0, 12, 0));
    auto* Details = WidgetTree->ConstructWidget<UVerticalBox>();
    InventoryCraftDetailFrame = WidgetTree->ConstructWidget<UBorder>();
    InventoryCraftDetailFrame->SetContent(Details);
    auto* DetailSlot = Split->AddChildToHorizontalBox(InventoryCraftDetailFrame);
    FSlateChildSize DetailSize(ESlateSizeRule::Fill);
    DetailSize.Value = 0.64f;
    DetailSlot->SetSize(DetailSize);
    auto* DetailScroll = WidgetTree->ConstructWidget<UScrollBox>();
    FKalmalaUITheme::Get().ApplyScroll(*DetailScroll);
    Details->AddChildToVerticalBox(DetailScroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* DetailContent = WidgetTree->ConstructWidget<UVerticalBox>();
    DetailScroll->AddChild(DetailContent);
    auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
    IconSize->SetWidthOverride(88);
    IconSize->SetHeightOverride(88);
    InventoryCraftIcon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
    IconSize->SetContent(InventoryCraftIcon);
    DetailContent->AddChildToVerticalBox(IconSize)->SetHorizontalAlignment(HAlign_Center);
    const auto AddText = [&](TObjectPtr<UTextBlock>& Label)
    {
        Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetAutoWrapText(true);
        DetailContent->AddChildToVerticalBox(Label)->SetPadding(FMargin(0, 8, 0, 8));
    };
    AddText(InventoryCraftTitle);
    InventoryCraftTitle->SetJustification(ETextJustify::Center);
    AddText(InventoryCraftDescription);
    AddText(InventoryCraftStats);
    InventoryCraftCosts = WidgetTree->ConstructWidget<UHorizontalBox>();
    Details->AddChildToVerticalBox(InventoryCraftCosts)->SetPadding(FMargin(0, 8));
    InventoryCraftButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
    auto* CraftText = WidgetTree->ConstructWidget<UTextBlock>();
    CraftText->SetText(FText::FromString(TEXT("Craft")));
    CraftText->SetJustification(ETextJustify::Center);
    InventoryCraftButton->SetContent(CraftText);
    InventoryCraftButton->OnClicked.AddDynamic(this, &ThisClass::Craft);
    Details->AddChildToVerticalBox(InventoryCraftButton)->SetPadding(FMargin(0, 4, 0, 0));
}

void UKalmalaCraftingWidget::SelectInventoryCraftRecipe()
{
    // A button can be activated by pointer or by keyboard/controller focus.
    for (int32 Index = 0; Index < InventoryCraftChoices.Num(); ++Index)
        if (InventoryCraftChoices[Index]->HasKeyboardFocus()) { Selected = Index; Refresh(); return; }
    for (int32 Index = 0; Index < InventoryCraftChoices.Num(); ++Index)
        if (InventoryCraftChoices[Index]->IsHovered()) { Selected = Index; Refresh(); return; }
}

void UKalmalaCraftingWidget::RefreshInventoryCraftLayout()
{
    if (!InventoryCraftBackground) return;
    const auto& Theme = FKalmalaUITheme::Get();
    const int32 Scale = UKalmalaSettingsWidget::GetTextScalePercent();
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    Theme.ApplyPanel(*InventoryCraftBackground, Contrast, &Theme.InventoryPanelImage);
    InventoryCraftBackground->SetPadding(FMargin(12));
    for (UBorder* Section : {InventoryCraftListFrame.Get(), InventoryCraftDetailFrame.Get()})
    {
        Section->SetBrush(FSlateRoundedBoxBrush(Contrast ? FLinearColor::Black : FLinearColor(.09f, .045f, .02f, .65f),
            3.f, Contrast ? FLinearColor::White : FLinearColor(.45f, .27f, .12f, 1.f), 1.f));
        Section->SetBrushColor(FLinearColor::White);
        Section->SetPadding(FMargin(10));
    }
    RecipeSearchStyle.SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FMath::RoundToInt(14 * Scale / 100.f)));
    InventoryCraftSearch->SetWidgetStyle(RecipeSearchStyle);
    const auto StyleButton = [&](UButton* Button)
    {
        Theme.ApplyButton(*Button, Contrast);
        if (auto* Label = Cast<UTextBlock>(Button->GetContent())) Theme.ApplyText(*Label, 16, false, Scale, Contrast);
    };
    StyleButton(InventoryCraftClear);
    StyleButton(InventoryCraftButton);
    const auto& Recipes = UKalmalaRecipeCatalogue::Get()->Recipes;
    const TArray<int32> Visible = GetVisibleRecipeIndices();
    if (InventoryCraftIndices != Visible)
    {
        InventoryCraftIndices = Visible;
        InventoryCraftList->ClearChildren();
        InventoryCraftChoices.Reset();
        for (int32 RecipeIndex : Visible)
        {
            auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
            Button->OnClicked.AddDynamic(this, &ThisClass::SelectInventoryCraftRecipe);
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
            IconSize->SetWidthOverride(28); IconSize->SetHeightOverride(28);
            auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
            Icon->SetCatalogueIcon(Recipes[RecipeIndex].GetOutputIdentity());
            IconSize->SetContent(Icon);
            Row->AddChildToHorizontalBox(IconSize)->SetVerticalAlignment(VAlign_Center);
            auto* Name = WidgetTree->ConstructWidget<UTextBlock>();
            Name->SetText(FText::FromString(Recipes[RecipeIndex].DisplayName));
            Name->SetJustification(ETextJustify::Center);
            Name->SetAutoWrapText(true);
            auto* NameSlot = Row->AddChildToHorizontalBox(Name);
            NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            NameSlot->SetVerticalAlignment(VAlign_Center);
            NameSlot->SetPadding(FMargin(4));
            Button->SetContent(Row);
            InventoryCraftList->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 0, 0, 4));
            InventoryCraftChoices.Add(Button);
        }
    }
    Selected = FMath::Clamp(Selected, 0, FMath::Max(0, Visible.Num() - 1));
    for (int32 Index = 0; Index < InventoryCraftChoices.Num(); ++Index)
    {
        auto* Button = CastChecked<UKalmalaThemedButton>(InventoryCraftChoices[Index]);
        Theme.ApplyButton(*Button, Contrast);
        Button->SetInteractionSelected(Index == Selected);
        auto* Row = CastChecked<UHorizontalBox>(Button->GetContent());
        Theme.ApplyText(*CastChecked<UTextBlock>(Row->GetChildAt(1)), 14, Index == Selected, Scale, Contrast);
    }
    Theme.ApplyText(*InventoryCraftTitle, 22, true, Scale, Contrast);
    Theme.ApplyText(*InventoryCraftDescription, 16, false, Scale, Contrast);
    Theme.ApplyText(*InventoryCraftStats, 16, false, Scale, Contrast);
    if (!Visible.IsValidIndex(Selected))
    {
        InventoryCraftButton->SetIsEnabled(false);
        InventoryCraftTitle->SetText(FText::FromString(TEXT("No matching items")));
        InventoryCraftDescription->SetText(FText::GetEmpty());
        InventoryCraftStats->SetText(FText::GetEmpty());
        InventoryCraftIcon->SetVisibility(ESlateVisibility::Collapsed);
        InventoryCraftCosts->ClearChildren(); InventoryCraftCostSignature.Reset();
        return;
    }
    const FKalmalaRecipe& Recipe = Recipes[Visible[Selected]];
    auto* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    auto* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    auto* Crafting = Model();
    InventoryCraftTitle->SetText(FText::FromString(Recipe.DisplayName));
    InventoryCraftDescription->SetText(FText::FromString(Crafting ? Crafting->GetRecipeDescription(Recipe.RecipeId) : FString()));
    InventoryCraftIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
    InventoryCraftIcon->SetCatalogueIcon(Recipe.GetOutputIdentity());
    const auto* Tool = FKalmalaToolLifecycleContract::FindDefinition(Recipe.Output);
    InventoryCraftStats->SetText(FText::FromString(Tool ? FString::Printf(TEXT("Durability: %d"), Tool->MaxDurability) : FString()));
    TArray<FKalmalaInventoryStack> Costs, After;
    int32 OutputCount = 0;
    const bool bValidCosts = UKalmalaRecipeCatalogue::Scale(Recipe, 1, Costs, OutputCount);
    FString Signature = FString::Printf(TEXT("%s/%d/%d"), *Recipe.RecipeId.ToString(), Scale, Contrast);
    TArray<int32> Owned;
    bool bMaterials = bValidCosts;
    for (const auto& Cost : Costs)
    {
        int32 Count = 0;
        if (Inventory) for (const auto& Stack : Inventory->GetStacks()) if (Stack.ItemId == Cost.ItemId) Count += Stack.Quantity;
        Owned.Add(Count);
        bMaterials &= Count >= Cost.Quantity;
        Signature += FString::Printf(TEXT("/%s:%d:%d"), *Cost.ItemId.ToString(), Cost.Quantity, Count);
    }
    if (Signature != InventoryCraftCostSignature)
    {
        InventoryCraftCostSignature = Signature;
        InventoryCraftCosts->ClearChildren();
        for (int32 Index = 0; Index < Costs.Num(); ++Index)
        {
            auto* CostColumn = WidgetTree->ConstructWidget<UVerticalBox>();
            auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
            IconSize->SetWidthOverride(44); IconSize->SetHeightOverride(44);
            auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
            Icon->SetCatalogueIcon(Costs[Index].ItemId); IconSize->SetContent(Icon);
            CostColumn->AddChildToVerticalBox(IconSize)->SetHorizontalAlignment(HAlign_Center);
            auto* Amount = WidgetTree->ConstructWidget<UTextBlock>();
            Amount->SetText(FText::AsNumber(Costs[Index].Quantity)); Amount->SetJustification(ETextJustify::Center);
            Theme.ApplyText(*Amount, 16, true, Scale, Contrast);
            if (Contrast == 0 && Owned[Index] < Costs[Index].Quantity) Amount->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, .35f, .25f)));
            CostColumn->AddChild(Amount);
            const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Costs[Index].ItemId);
            CostColumn->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s: %d owned, %d required"),
                Item ? *Item->DisplayName : *Costs[Index].ItemId.ToString(), Owned[Index], Costs[Index].Quantity)));
            InventoryCraftCosts->AddChildToHorizontalBox(CostColumn)->SetPadding(FMargin(6, 0));
        }
    }
    FString Reason;
    const bool bCanCraft = Recipe.bEnabled && bMaterials && Inventory && Character
        && Character->GetCarriedToolLevel(Recipe.Output) == 0
        && Character->GetCarriedToolInventory().Num() < FKalmalaToolLifecycleContract::MaxCarriedToolRecords
        && UKalmalaInventoryComponent::BuildExchange(Inventory->GetStacks(), Costs, NAME_None, 0, After, Reason)
        && Inventory->CanFitContents(After, Character->GetCarriedToolInventory().Num() + 1);
    InventoryCraftButton->SetIsEnabled(bCanCraft);
    InventoryCraftButton->SetToolTipText(FText::FromString(Crafting ? Crafting->GetRecipeAvailability(Recipe.RecipeId) : TEXT("Waiting for inventory")));
}

#if !UE_BUILD_SHIPPING
void UKalmalaCraftingWidget::PrepareMenuRender(const bool bPlayerCrafting)
{
    bInventoryCompanion = bPlayerCrafting;
    bEmbeddedContext = bPlayerCrafting;
    StationFilterKit = NAME_None;
    RecipeQuery.Reset();
    RecipeCategory = bPlayerCrafting ? 0 : 3;
    Selected = 0;
    ConfigureStationContextPresentation(FString());
    Refresh();
    SetVisibility(ESlateVisibility::Visible);
}

bool UKalmalaCraftingWidget::VerifyInventoryCraftLayoutForTest()
{
    if (!InventoryCraftBackground || !InventoryCraftList) return false;
    const bool PreviousCompanion = bInventoryCompanion;
    const FString PreviousQuery = RecipeQuery;
    const int32 PreviousSelected = Selected;
    bInventoryCompanion = true;
    RecipeQuery.Reset();
    Selected = 0;
    Refresh();
    const bool bLayout = MenuBackground->GetVisibility() == ESlateVisibility::Collapsed
        && InventoryCraftBackground->GetVisibility() == ESlateVisibility::Visible
        && InventoryCraftChoices.Num() == 1
        && InventoryCraftTitle->GetText().ToString() == TEXT("Building Hammer")
        && InventoryCraftCosts->GetChildrenCount() == 2
        && !InventoryCraftButton->GetIsEnabled(); // No owning character or materials in this fixture.
    const auto* BackgroundSlot = Cast<UOverlaySlot>(InventoryCraftBackground->Slot);
    const bool bFill = BackgroundSlot && BackgroundSlot->GetHorizontalAlignment() == HAlign_Fill
        && BackgroundSlot->GetVerticalAlignment() == VAlign_Fill
        && InventoryCraftListFrame && InventoryCraftDetailFrame;
    RecipeQuery = TEXT("no-matching-player-recipe");
    Refresh();
    const bool bEmpty = InventoryCraftChoices.IsEmpty() && InventoryCraftCosts->GetChildrenCount() == 0
        && !InventoryCraftButton->GetIsEnabled();
    bInventoryCompanion = PreviousCompanion;
    RecipeQuery = PreviousQuery;
    Selected = PreviousSelected;
    return bLayout && bEmpty && bFill;
}
#endif
