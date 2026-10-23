#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaCombatComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaCraftingSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaInventoryGridWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaThemedButton.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaUITheme.h"
#include "Styling/SlateTypes.h"
#include "Engine/Texture2D.h"

namespace
{
// Keep equipment cells on the inventory tile surface; only their outline responds to input.
void StyleGearButton(UKalmalaThemedButton& Button, const int32 Contrast)
{
    const auto& Theme = FKalmalaUITheme::Get();
    Theme.ApplyButton(Button, Contrast);
    FButtonStyle Style = Button.GetStyle();
    const FLinearColor Outline = Contrast > 0 ? FLinearColor::White : FLinearColor(0.32f, 0.20f, 0.12f, 1.0f);
    Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor::Transparent, 5.0f, Outline, 1.0f));
    Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor::Transparent, 5.0f, Theme.TextColor(true, Contrast), 2.0f));
    Style.SetPressed(Style.Hovered);
    Style.SetDisabled(Style.Normal);
    Style.SetNormalPadding(FMargin(0));
    Style.SetPressedPadding(FMargin(0));
    Button.SetStyle(Style);
}

FString GetCarriedToolDisplayName(const FName ItemId)
{
    if (ItemId == TEXT("StoneMace")) return TEXT("Stone Mace");
    if (ItemId == TEXT("BronzeMace")) return TEXT("Bronze Mace");
    if (ItemId == TEXT("IronMace")) return TEXT("Iron Mace");
    if (ItemId == TEXT("BronzeSword")) return TEXT("Bronze Sword");
    if (ItemId == TEXT("IronSword")) return TEXT("Iron Sword");
    if (ItemId == TEXT("WoodenClub")) return TEXT("Wooden Club");
    if (ItemId == TEXT("StoneKnife")) return TEXT("Stone Knife");
    if (ItemId == TEXT("StoneAxe")) return TEXT("Stone Axe");
    if (ItemId == TEXT("StonePick")) return TEXT("Stone Pick");
    if (ItemId == TEXT("StoneSpear")) return TEXT("Stone Spear");
    if (ItemId == TEXT("BronzeAxe")) return TEXT("Bronze Axe");
    if (ItemId == TEXT("BronzeKnife")) return TEXT("Bronze Knife");
    if (ItemId == TEXT("BronzePick")) return TEXT("Bronze Pick");
    if (ItemId == TEXT("BronzeSpear")) return TEXT("Bronze Spear");
    if (ItemId == TEXT("IronAxe")) return TEXT("Iron Axe");
    if (ItemId == TEXT("IronKnife")) return TEXT("Iron Knife");
    if (ItemId == TEXT("IronPick")) return TEXT("Iron Pick");
    if (ItemId == TEXT("IronSpear")) return TEXT("Iron Spear");
    if (ItemId == TEXT("ConstructionHammer")) return TEXT("Construction Hammer");
    if (ItemId == TEXT("BasicTorch")) return TEXT("Basic Torch");
    if (ItemId == TEXT("PlantingTrowel")) return TEXT("Planting Trowel");
    return ItemId.ToString();
}

FString BuildToolInspectionText(const FKalmalaToolState& Tool)
{
    if (Tool.ItemId == TEXT("BasicTorch"))
    {
        const float Fuel = FMath::IsFinite(Tool.RemainingBurnSeconds)
            ? FMath::Clamp(Tool.RemainingBurnSeconds, 0.0f,
                FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds) : 0.0f;
        return FString::Printf(TEXT("Fuel: %.0f / 300 seconds. Light radius: 6 m. Refuel in inventory with 1 Wood."), Fuel);
    }
    const int32 MaxDurability = FKalmalaToolLifecycleContract::GetMaxDurability(Tool.ItemId, Tool.ToolLevel);
    FString Text = UKalmalaCatalogueRowsWidget::BuildToolDetail(
        Tool.ToolLevel, Tool.Durability, MaxDurability);
    if (MaxDurability <= 0 || Tool.Durability < 0 || Tool.Durability > MaxDurability)
        return Text + TEXT("\nRepair unavailable.");
    const FKalmalaDamageChannels Bonuses = FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(Tool.ItemId, Tool.ToolLevel);
    if (Bonuses.Total() > 0.0f) Text += TEXT("\n") + Bonuses.Describe() + TEXT(".");
    return Text + (Tool.Durability < MaxDurability
        ? TEXT("\nFree repair at a visible Workbench or Forge.")
        : TEXT("\nNo repair needed."));
}

FString GetGearCatalogueSlot(const EKalmalaGearSlot Slot)
{
    switch (Slot)
    {
    case EKalmalaGearSlot::Head: return TEXT("Head");
    case EKalmalaGearSlot::Cape: return TEXT("Cape");
    case EKalmalaGearSlot::Chest: return TEXT("Chest");
    case EKalmalaGearSlot::Shield: return TEXT("Shield");
    case EKalmalaGearSlot::Legs: return TEXT("Legs");
    case EKalmalaGearSlot::Boots: return TEXT("Boots");
    case EKalmalaGearSlot::Necklace: return TEXT("Necklace");
    case EKalmalaGearSlot::Ring: return TEXT("Ring");
    default: return FString();
    }
}
}

void UKalmalaInventoryMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(true);
    if (!WidgetTree) return;
    const auto& Theme = FKalmalaUITheme::Get();
    const int32 TextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    auto* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    auto AddText = [&](const FString& Value, const bool bEmphasis)
    {
        auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Value));
        Text->SetAutoWrapText(true);
        Theme.ApplyText(*Text, bEmphasis ? Theme.EmphasisSize : Theme.BodySize, bEmphasis, TextScale, Contrast);
        Content->AddChildToVerticalBox(Text);
        return Text;
    };
    GridView = WidgetTree->ConstructWidget<UKalmalaInventoryGridWidget>();
    GridView->OnItemSelected.AddUObject(this, &ThisClass::SelectGridItem);
    GridView->OnItemHovered.AddUObject(this, &ThisClass::HoverGridItem);
    GridSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    GridSizeBox->SetHeightOverride((ResponsivePanelWidth - Theme.PaddingX * 2.0f) * 0.4f);
    GridSizeBox->SetContent(GridView);
    Content->AddChildToVerticalBox(GridSizeBox);

    // Reuse the Options ornament inside the existing one-tile equipment gap.
    auto* DividerRow = WidgetTree->ConstructWidget<UOverlay>();
    DividerRow->SetVisibility(ESlateVisibility::HitTestInvisible);
    GearDividerImage = WidgetTree->ConstructWidget<UImage>();
    if (!Theme.OptionsDividerImage.IsEmpty())
        if (auto* Texture = LoadObject<UTexture2D>(nullptr, *Theme.OptionsDividerImage))
            GearDividerImage->SetBrushFromTexture(Texture, false);
    auto* OrnamentSize = WidgetTree->ConstructWidget<USizeBox>();
    OrnamentSize->SetWidthOverride(168.0f);
    OrnamentSize->SetHeightOverride(56.0f);
    OrnamentSize->SetContent(GearDividerImage);
    auto* OrnamentSlot = DividerRow->AddChildToOverlay(OrnamentSize);
    OrnamentSlot->SetHorizontalAlignment(HAlign_Center);
    OrnamentSlot->SetVerticalAlignment(VAlign_Center);
    GearDividerRule = WidgetTree->ConstructWidget<UBorder>();
    GearDividerRule->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 0.0f));
    GearDividerRule->SetPadding(FMargin(0));
    auto* RuleSize = WidgetTree->ConstructWidget<USizeBox>();
    RuleSize->SetHeightOverride(2.0f);
    RuleSize->SetContent(GearDividerRule);
    auto* RuleSlot = DividerRow->AddChildToOverlay(RuleSize);
    RuleSlot->SetHorizontalAlignment(HAlign_Fill);
    RuleSlot->SetVerticalAlignment(VAlign_Center);
    auto* DividerSize = WidgetTree->ConstructWidget<USizeBox>();
    DividerSize->SetHeightOverride(56.0f);
    DividerSize->SetContent(DividerRow);
    Content->AddChildToVerticalBox(DividerSize);
    RefreshGearDivider(Contrast);

    auto* GearLayout = WidgetTree->ConstructWidget<UGridPanel>();
    auto AddGearSlot = [&](const FString& SlotName, const bool bInteractive,
        const EKalmalaGearSlot GearSlot, const EKalmalaIcon EmptyIcon, const int32 Row, const int32 Column)
    {
        auto* SlotSize = WidgetTree->ConstructWidget<USizeBox>();
        SlotSize->SetWidthOverride(56.0f);
        SlotSize->SetHeightOverride(56.0f);
        auto* Fill = WidgetTree->ConstructWidget<UBorder>();
        Fill->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 5.0f));
        Theme.ApplyItemIconTileFill(*Fill, Contrast);
        Fill->SetPadding(FMargin(0));
        GearSlotFills.Add(Fill);
        auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        StyleGearButton(*Button, Contrast);
        Button->SetToolTipText(FText::FromString(SlotName + TEXT(": Empty")));
        Fill->SetContent(Button);
        SlotSize->SetContent(Fill);
        auto* SlotContent = WidgetTree->ConstructWidget<UVerticalBox>();
        auto* IconSize = WidgetTree->ConstructWidget<USizeBox>();
        IconSize->SetWidthOverride(36.0f);
        IconSize->SetHeightOverride(36.0f);
        auto* Icon = WidgetTree->ConstructWidget<UKalmalaIconWidget>();
        Icon->SetIcon(EmptyIcon);
        Icon->SetRenderOpacity(Contrast > 0 ? 0.7f : 0.4f);
        Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        IconSize->SetContent(Icon);
        auto* IconPlacement = SlotContent->AddChildToVerticalBox(IconSize);
        IconPlacement->SetHorizontalAlignment(HAlign_Center);
        IconPlacement->SetVerticalAlignment(VAlign_Center);
        IconPlacement->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        // Keep slot/state metadata available without crowding the icon layout.
        auto* Name = WidgetTree->ConstructWidget<UTextBlock>();
        Name->SetText(FText::FromString(SlotName));
        Name->SetVisibility(ESlateVisibility::Collapsed);
        SlotContent->AddChildToVerticalBox(Name);
        auto* State = WidgetTree->ConstructWidget<UTextBlock>();
        State->SetText(FText::FromString(TEXT("Empty")));
        State->SetVisibility(ESlateVisibility::Collapsed);
        SlotContent->AddChildToVerticalBox(State);
        Button->SetContent(SlotContent);
        if (bInteractive)
        {
            switch (GearSlot)
            {
            case EKalmalaGearSlot::Head: Button->OnClicked.AddDynamic(this, &ThisClass::UseHeadGearSlot); break;
            case EKalmalaGearSlot::Cape: Button->OnClicked.AddDynamic(this, &ThisClass::UseCapeGearSlot); break;
            case EKalmalaGearSlot::Chest: Button->OnClicked.AddDynamic(this, &ThisClass::UseChestGearSlot); break;
            case EKalmalaGearSlot::Shield: Button->OnClicked.AddDynamic(this, &ThisClass::UseShieldGearSlot); break;
            case EKalmalaGearSlot::Legs: Button->OnClicked.AddDynamic(this, &ThisClass::UseLegsGearSlot); break;
            case EKalmalaGearSlot::Boots: Button->OnClicked.AddDynamic(this, &ThisClass::UseBootsGearSlot); break;
            case EKalmalaGearSlot::Necklace: Button->OnClicked.AddDynamic(this, &ThisClass::UseNecklaceGearSlot); break;
            case EKalmalaGearSlot::Ring: Button->OnClicked.AddDynamic(this, &ThisClass::UseRingGearSlot); break;
            default: break;
            }
            GearSlotButtons.Add(Button);
            GearSlotIcons.Add(Icon);
            GearSlotNameTexts.Add(Name);
            GearSlotStateTexts.Add(State);
        }
        else
        {
            MainHandButton = Button;
            MainHandIcon = Icon;
            MainHandButton->OnClicked.AddDynamic(this, &ThisClass::UseMainHandSlot);
            MainHandNameText = Name;
            MainHandStateText = State;
        }
        GearLayout->AddChildToGrid(SlotSize, Row, Column)->SetPadding(FMargin(4.0f));
    };
    // Reference body placement; construction order matches RefreshGearPresentation's slot table.
    AddGearSlot(TEXT("Head"), true, EKalmalaGearSlot::Head, EKalmalaIcon::Helmet, 0, 1);
    AddGearSlot(TEXT("Cape"), true, EKalmalaGearSlot::Cape, EKalmalaIcon::Cape, 1, 0);
    AddGearSlot(TEXT("Chest"), true, EKalmalaGearSlot::Chest, EKalmalaIcon::Tunic, 2, 1);
    AddGearSlot(TEXT("Shield"), true, EKalmalaGearSlot::Shield, EKalmalaIcon::Shield, 2, 2);
    AddGearSlot(TEXT("Legs"), true, EKalmalaGearSlot::Legs, EKalmalaIcon::Leggings, 3, 1);
    AddGearSlot(TEXT("Boots"), true, EKalmalaGearSlot::Boots, EKalmalaIcon::Boots, 4, 1);
    AddGearSlot(TEXT("Main Hand"), false, EKalmalaGearSlot::Head, EKalmalaIcon::Sword, 2, 0);
    AddGearSlot(TEXT("Necklace"), true, EKalmalaGearSlot::Necklace, EKalmalaIcon::Necklace, 1, 1);
    AddGearSlot(TEXT("Ring"), true, EKalmalaGearSlot::Ring, EKalmalaIcon::Ring, 4, 2);
    auto* GearPlacement = Content->AddChildToVerticalBox(GearLayout);
    GearPlacement->SetHorizontalAlignment(HAlign_Center);
    ItemDetailView = WidgetTree->ConstructWidget<UKalmalaItemDetailWidget>();
    Content->AddChildToVerticalBox(ItemDetailView);
    ItemDetailView->SetVisibility(ESlateVisibility::Collapsed);
    auto AddAction = [&](const FString& Label)
    {
        auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
        auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
        Text->SetText(FText::FromString(Label));
        Theme.ApplyText(*Text, Theme.BodySize, false, TextScale, Contrast);
        Button->SetContent(Text);
        Theme.ApplyButton(*Button, Contrast);
        Content->AddChildToVerticalBox(Button);
        Theme.SizeButtonToContent(*Button);
        Button->SetVisibility(ESlateVisibility::Collapsed);
        return Button;
    };
    PreferArrowsButton = AddAction(TEXT("Prioritize selected arrows / automatic"));
    PreferArrowsButton->OnClicked.AddDynamic(this, &ThisClass::PreferSelectedArrows);
    RepairToolButton = AddAction(TEXT("Repair selected tool"));
    RepairToolButton->OnClicked.AddDynamic(this, &ThisClass::RepairSelectedTool);
    UpgradeOpalButton = AddAction(TEXT("Upgrade equipped Opal Helm at Forge"));
    UpgradeOpalButton->OnClicked.AddDynamic(this, &ThisClass::UpgradeOpalHelm);
    RepairOpalButton = AddAction(TEXT("Repair equipped Opal Helm at Forge (free)"));
    RepairOpalButton->OnClicked.AddDynamic(this, &ThisClass::RepairOpalHelm);
    ToolActionStatusText = AddText(TEXT(""), false);
    ToolActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
    EatFoodButton = AddAction(TEXT("Eat one serving"));
    EatFoodButton->OnClicked.AddDynamic(this, &ThisClass::EatSelectedFood);
    FoodActionStatusText = AddText(TEXT(""), false);
    FoodActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
    InventoryPanel = WidgetTree->ConstructWidget<UBorder>();
    Theme.ApplyPanel(*InventoryPanel, Contrast, &Theme.InventoryPanelImage);
    InventoryPanel->SetPadding(FMargin(Theme.PaddingX, Theme.PaddingY));
    InventoryPanel->SetContent(Content);
    PanelBackplate = InventoryPanel;
    CraftingCompanion = GetOwningPlayer()
        ? CreateWidget<UKalmalaCraftingWidget>(GetOwningPlayer(), UKalmalaCraftingWidget::StaticClass())
        : nullptr;
    PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    PanelSizeBox->SetWidthOverride(ResponsivePanelWidth);
    PanelSizeBox->SetHeightOverride(ResponsivePanelHeight);
    PanelSizeBox->SetContent(PanelBackplate);
    WidgetTree->RootWidget = PanelSizeBox;
    SetVisibility(ESlateVisibility::Collapsed);
}
void UKalmalaInventoryMenuWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bMenuOpen) return;
    const FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(this));
    UpdateResponsivePanelSize(Viewport);
    if (bOpeningAnimationActive)
    {
        if (UKalmalaSettingsWidget::IsReducedMotionEnabled())
        {
            bOpeningAnimationActive = false;
            if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D::ZeroVector);
            if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D::ZeroVector);
        }
        else
        {
            const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
            OpeningElapsed = FMath::Min(OpeningElapsed + FMath::Clamp(InDeltaTime, 0.0f, 1.0f / 30.0f),
                Theme.OptionsOpeningDuration);
            const float Progress = Theme.OptionsOpeningDuration > 0.0f
                ? OpeningElapsed / Theme.OptionsOpeningDuration : 1.0f;
            const float Offset = Theme.OptionsOpeningOffset(Progress);
            if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D(0.0f, Offset));
            if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D(-Offset, 0.0f));
            if (Progress >= 1.0f)
            {
                bOpeningAnimationActive = false;
                if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D::ZeroVector);
                if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D::ZeroVector);
            }
        }
    }
}

void UKalmalaInventoryMenuWidget::UpdateResponsivePanelSize(const FVector2D ViewportSize)
{
    if (!PanelSizeBox) return;
    const float NewWidth = FMath::Min(520.0f, FMath::Max(80.0f, ViewportSize.X * 0.38f));
    const float GridHeight = FMath::Max(32.0f, (NewWidth - FKalmalaUITheme::Get().PaddingX * 2.0f) * 0.4f);
    constexpr float WornGearHeight = 56.0f + 5.0f * 64.0f;
    const bool bDetails = bSelectionDetailsRequested || !HoveredItemId.IsNone();
    const float NewHeight = FMath::Min(FMath::Max(80.0f, ViewportSize.Y - 32.0f),
        GridHeight + WornGearHeight + (bDetails ? 280.0f : FKalmalaUITheme::Get().PaddingY * 2.0f));
    if (!FMath::IsNearlyEqual(NewWidth, ResponsivePanelWidth))
    {
        ResponsivePanelWidth = NewWidth;
        PanelSizeBox->SetWidthOverride(ResponsivePanelWidth);
    }
    if (GridSizeBox) GridSizeBox->SetHeightOverride(FMath::Max(40.0f,
        (ResponsivePanelWidth - FKalmalaUITheme::Get().PaddingX * 2.0f) * 0.4f));
    if (!FMath::IsNearlyEqual(NewHeight, ResponsivePanelHeight))
    {
        ResponsivePanelHeight = NewHeight;
        PanelSizeBox->SetHeightOverride(ResponsivePanelHeight);
    }
    SetDesiredSizeInViewport(FVector2D(NewWidth, NewHeight));
    SetPositionInViewport(FVector2D(16.0f, 16.0f), false);
    SetAlignmentInViewport(FVector2D::ZeroVector);
    SetAnchorsInViewport(FAnchors(0.0f, 0.0f));
}

void UKalmalaInventoryMenuWidget::Open()
{
    if (bMenuOpen) return;
    APlayerController* Controller = GetOwningPlayer();
    if (Controller == nullptr || !Controller->IsLocalController()) return;

    HoveredItemId = NAME_None;
    bSelectionDetailsRequested = false;
    if (GridView) GridView->ClearHover();
    RefreshOwnerInventory();
    UpdateResponsivePanelSize(UWidgetLayoutLibrary::GetViewportSize(this) / FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(this)));
    if (CraftingCompanion)
    {
        CraftingCompanion->AddToPlayerScreen(251);
        if (!CraftingCompanion->OpenAsInventoryCompanion()) CraftingCompanion->RemoveFromParent();
    }

    bPreviousCursorVisibility = Controller->bShowMouseCursor;
    bAcquiredMoveIgnore = !Controller->IsMoveInputIgnored();
    bAcquiredLookIgnore = !Controller->IsLookInputIgnored();
    if (bAcquiredMoveIgnore) Controller->SetIgnoreMoveInput(true);
    if (bAcquiredLookIgnore) Controller->SetIgnoreLookInput(true);
    if (GridView) GridView->CancelMove();
    Controller->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    UWidget* FocusTarget = GridView ? static_cast<UWidget*>(GridView.Get()) : this;
    InputMode.SetWidgetToFocus(FocusTarget->TakeWidget());
    InputMode.SetHideCursorDuringCapture(false);
    Controller->SetInputMode(InputMode);


    bMenuOpen = true;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    bOpeningAnimationActive = Theme.ShouldAnimateOptionsOpening();
    OpeningElapsed = 0.0f;
    const float InitialOffset = Theme.OptionsOpeningOffset(0.0f);
    if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D(0.0f, InitialOffset));
    if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D(-InitialOffset, 0.0f));
    SetVisibility(ESlateVisibility::Visible);
    FocusTarget->SetUserFocus(Controller);
    FocusTarget->SetKeyboardFocus();
}

void UKalmalaInventoryMenuWidget::RefreshOwnerInventory()
{
    APlayerController* Controller = GetOwningPlayer();
    if (Controller == nullptr || !Controller->IsLocalController()) return;

    TArray<FKalmalaCatalogueRow> InventoryRows;
    APawn* OwnerPawn = Controller->GetPawn();
    const UKalmalaInventoryComponent* Inventory = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (Inventory != nullptr)
    {
        const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
        TArray<FName> StackIds;
        TMap<FName, int32> StackQuantities;
        TMap<FName, FGuid> StackGuids;
        for (const FKalmalaInventoryStack& Stack : Inventory->GetStacks())
        {
            if (Stack.ItemId.IsNone() || Stack.Quantity <= 0) continue;
            if (!StackQuantities.Contains(Stack.ItemId)) StackIds.Add(Stack.ItemId);
            StackQuantities.FindOrAdd(Stack.ItemId) += Stack.Quantity;
            if (!StackGuids.Contains(Stack.ItemId) && !Stack.ItemGuids.IsEmpty())
                StackGuids.Add(Stack.ItemId, Stack.ItemGuids[0]);
        }
        for (const FName ItemId : StackIds)
        {
            const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(ItemId) : nullptr;
            InventoryRows.Add({ ItemId, Definition ? Definition->DisplayName : ItemId.ToString(),
                FString::Printf(TEXT("× %d"), StackQuantities.FindRef(ItemId)), false,
                StackGuids.FindRef(ItemId) });
        }
    }

    if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
    {
        const TArray<FKalmalaToolState>& CarriedTools = Character->GetCarriedToolInventory();
        TSet<FName> SeenItemIds;
        const int32 BoundedToolCount = CarriedTools.Num();
        for (int32 Index = 0; Index < BoundedToolCount; ++Index)
        {
            const FKalmalaToolState& Tool = CarriedTools[Index];
            if (Inventory && Tool.InstanceGuid == Inventory->GetMainHandGuid()) continue;
            const FName CanonicalItemId = FKalmalaToolLifecycleContract::CanonicalizeItemId(Tool.ItemId);
            if (CanonicalItemId.IsNone() || SeenItemIds.Contains(CanonicalItemId)) continue;
            const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(CanonicalItemId);
            if (Definition == nullptr) continue;
            SeenItemIds.Add(CanonicalItemId);
            int32 Copies = 0;
            for (int32 CopyIndex = 0; CopyIndex < BoundedToolCount; ++CopyIndex)
                if ((!Inventory || CarriedTools[CopyIndex].InstanceGuid != Inventory->GetMainHandGuid())
                    && FKalmalaToolLifecycleContract::CanonicalizeItemId(CarriedTools[CopyIndex].ItemId) == CanonicalItemId)
                    ++Copies;
            FString DisplayName = GetCarriedToolDisplayName(CanonicalItemId);
            if (Copies > 1) DisplayName += FString::Printf(TEXT(" × %d"), Copies);
            InventoryRows.Add({ CanonicalItemId, DisplayName,
                BuildToolInspectionText(Tool), true, Tool.InstanceGuid });
        }
    }

    if (Inventory)
    {
        const auto& Slots = Inventory->GetGridSlots();
        InventoryRows.StableSort([&Slots](const FKalmalaCatalogueRow& A, const FKalmalaCatalogueRow& B)
        {
            const int32 Left = Slots.IndexOfByKey(A.Id), Right = Slots.IndexOfByKey(B.Id);
            return (Left == INDEX_NONE ? 40 : Left) < (Right == INDEX_NONE ? 40 : Right);
        });
    }
    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    ApplyInventoryRows(MoveTemp(InventoryRows), Inventory != nullptr, TextScale, Contrast);

    if (bAwaitingRepairResult)
    {
        if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
        {
            if (const UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
                Crafting && Crafting->GetResultSerial() != RepairRequestResultSerial)
            {
                LastRepairResultText = Crafting->GetLastResult();
                LastRepairResultItemId = AwaitingRepairItemId;
                bAwaitingRepairResult = false;
                AwaitingRepairItemId = NAME_None;
            }
        }
    }

    if (bAwaitingFoodResult)
    {
        if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
        {
            if (const UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
                Crafting && Crafting->GetResultSerial() != FoodRequestResultSerial)
            {
                LastFoodResultText = Crafting->GetLastResult();
                LastFoodResultItemId = AwaitingFoodItemId;
                bAwaitingAcceptedFoodStatus = Crafting->WasLastResultAccepted();
                bAwaitingFoodResult = false;
                AwaitingFoodItemId = NAME_None;
            }
        }
    }

    if (bAwaitingAcceptedFoodStatus)
    {
        if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn))
        {
            const FName ExpectedStatus = UKalmalaPlayerStatusComponent::IsKnownDraughtItem(LastFoodResultItemId)
                ? UKalmalaPlayerStatusComponent::DraughtCooldownStatusId
                : UKalmalaPlayerStatusComponent::SteadyMealStatusId;
            if (const UKalmalaPlayerStatusComponent* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
                Status && Status->HasStatus(ExpectedStatus))
            {
                bAwaitingAcceptedFoodStatus = false;
            }
        }
    }

    if (Inventory)
    {
        const uint32 ResultSerial = Inventory->GetGearRequestResultSerial();
        if (bAwaitingGearRequestResult && ResultSerial != LastSeenGearRequestResultSerial)
        {
            bAwaitingGearRequestResult = false;
            GearRequestSentAtSeconds = -1.0;
            LastSeenGearRequestResultSerial = ResultSerial;
            if (Inventory->WasLastGearRequestAccepted())
                LastGearActionStatus = Inventory->GetLastGearRequestReason().IsEmpty()
                    || Inventory->GetLastGearRequestReason() == TEXT("Ready")
                    ? (bPendingGearEquip ? TEXT("Gear equipped.") : TEXT("Gear returned to your inventory."))
                    : Inventory->GetLastGearRequestReason();
            else
                LastGearActionStatus = Inventory->GetLastGearRequestReason().IsEmpty()
                    ? TEXT("The server rejected the gear request.") : Inventory->GetLastGearRequestReason();
        }
        else if (!bAwaitingGearRequestResult)
        {
            LastSeenGearRequestResultSerial = ResultSerial;
        }
    }
    if (bAwaitingGearRequestResult && GetWorld()
        && GetWorld()->GetTimeSeconds() - GearRequestSentAtSeconds > 3.0)
    {
        bAwaitingGearRequestResult = false;
        GearRequestSentAtSeconds = -1.0;
        LastGearActionStatus = TEXT("No server response received yet. Displayed gear remains server-confirmed.");
    }

    RefreshSelectionPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::ApplyInventoryRows(TArray<FKalmalaCatalogueRow>&& Rows,
    const bool bInventoryAvailable, const int32 TextScale, const int32 Contrast)
{
    (void)bInventoryAvailable;
    SourceInventoryRows = MoveTemp(Rows);
    if (!RememberedSelectedItemId.IsNone() && !SourceInventoryRows.ContainsByPredicate([this](const FKalmalaCatalogueRow& Row)
        { return Row.Id == RememberedSelectedItemId; }))
    {
        RememberedSelectedItemId = NAME_None;
        SelectedGridInstanceGuid.Invalidate();
    }

    auto* Inventory = GetOwningPlayerPawn() ? GetOwningPlayerPawn()->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (GridView) GridView->Refresh(Inventory, false, RememberedSelectedItemId);
    RebuildVisibleInventoryRows(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RebuildVisibleInventoryRows(const int32 TextScale, const int32 Contrast)
{
    OwnerInventoryRows = SourceInventoryRows;
    SelectedInventoryIndex = OwnerInventoryRows.IndexOfByPredicate([this](const FKalmalaCatalogueRow& Row)
    { return Row.Id == RememberedSelectedItemId; });
    if (SelectedInventoryIndex == INDEX_NONE && !OwnerInventoryRows.IsEmpty() && !bEmptyCellSelected) SelectedInventoryIndex = 0;
    RememberedSelectedItemId = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
        ? OwnerInventoryRows[SelectedInventoryIndex].Id : NAME_None;
    RefreshSelectionPresentation(TextScale, Contrast);
}
void UKalmalaInventoryMenuWidget::SelectGridItem(const FName ItemId)
{
    SelectedGridInstanceGuid = GridView ? GridView->GetFocusedInstanceGuid() : FGuid();
    bEmptyCellSelected = ItemId.IsNone();
    bSelectionDetailsRequested = !ItemId.IsNone();
    RememberedSelectedItemId = ItemId;
    SelectedInventoryIndex = OwnerInventoryRows.IndexOfByPredicate(
        [ItemId](const FKalmalaCatalogueRow& Row) { return Row.Id == ItemId; });
    RefreshSelectionPresentation(UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::HoverGridItem(const FName ItemId)
{
    if (HoveredItemId == ItemId) return;
    HoveredItemId = ItemId;
    RefreshSelectionPresentation(UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
}

bool UKalmalaInventoryMenuWidget::NavigateInventoryMenu(const FKey Key)
{
    if (GridView && (GridView->HasKeyboardFocus()
        || (GetOwningPlayer() && GridView->HasUserFocus(GetOwningPlayer())))) return false;
    if (Key == EKeys::Left || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Up)
    { StepSelection(-1); return true; }
    if (Key == EKeys::Right || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Down)
    { StepSelection(1); return true; }
    return false;
}

void UKalmalaInventoryMenuWidget::RefreshSelectionPresentation(const int32 TextScale, const int32 Contrast)
{
    if (GridView == nullptr || ItemDetailView == nullptr
        || RepairToolButton == nullptr || ToolActionStatusText == nullptr
        || EatFoodButton == nullptr || FoodActionStatusText == nullptr) return;

    if (LastTextScalePercent != TextScale || LastContrastMode != Contrast)
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        if (InventoryPanel)
        {
            Theme.ApplyPanel(*InventoryPanel, Contrast, &Theme.InventoryPanelImage);
            InventoryPanel->SetPadding(FMargin(Theme.PaddingX, Theme.PaddingY));
        }

        Theme.ApplyButton(*RepairToolButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(RepairToolButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*ToolActionStatusText, Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyButton(*EatFoodButton, Contrast);
        Theme.ApplyText(*CastChecked<UTextBlock>(EatFoodButton->GetContent()), Theme.BodySize, false, TextScale, Contrast);
        Theme.ApplyText(*FoodActionStatusText, Theme.BodySize, false, TextScale, Contrast);
        for (UBorder* Fill : GearSlotFills)
        {
            if (!Fill) continue;
            Theme.ApplyItemIconTileFill(*Fill, Contrast);
            if (auto* Button = Cast<UKalmalaThemedButton>(Fill->GetContent())) StyleGearButton(*Button, Contrast);
        }
        if (MainHandNameText) Theme.ApplyText(*MainHandNameText, Theme.BodySize, true, TextScale, Contrast);
        if (MainHandStateText) Theme.ApplyText(*MainHandStateText, Theme.BodySize, false, TextScale, Contrast);
        for (UTextBlock* Name : GearSlotNameTexts)
            if (Name) Theme.ApplyText(*Name, Theme.BodySize, true, TextScale, Contrast);
        for (UTextBlock* State : GearSlotStateTexts)
            if (State) Theme.ApplyText(*State, Theme.BodySize, false, TextScale, Contrast);

        LastTextScalePercent = TextScale;
        LastContrastMode = Contrast;
        RefreshGearDivider(Contrast);
    }

    const bool bHasSelection = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex);
    const FKalmalaCatalogueRow* SelectedRow = bHasSelection ? &OwnerInventoryRows[SelectedInventoryIndex] : nullptr;
    const FName SelectedItem = SelectedRow ? SelectedRow->Id : NAME_None;
    if (!HoveredItemId.IsNone() && !OwnerInventoryRows.ContainsByPredicate([this](const FKalmalaCatalogueRow& Row)
        { return Row.Id == HoveredItemId; })) HoveredItemId = NAME_None;
    if (SelectedItem != LastPresentedSelectionId)
    {
        LastPresentedSelectionId = SelectedItem;
        if (!bAwaitingRepairResult)
        {
            LastRepairResultText.Reset();
            LastRepairResultItemId = NAME_None;
        }
    }


    if (GridView)
    {
        auto* Inventory = GetOwningPlayerPawn() ? GetOwningPlayerPawn()->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        GridView->Refresh(Inventory, false, SelectedItem);
    }

    const FKalmalaCatalogueRow* HoveredRow = HoveredItemId.IsNone() ? nullptr
        : OwnerInventoryRows.FindByPredicate([this](const FKalmalaCatalogueRow& Row) { return Row.Id == HoveredItemId; });
    const FKalmalaCatalogueRow* DetailRow = HoveredRow ? HoveredRow
        : (bSelectionDetailsRequested ? SelectedRow : nullptr);
    const bool bShowSelectedActions = bSelectionDetailsRequested && HoveredItemId.IsNone();
    const bool bSelectedTool = SelectedRow != nullptr && SelectedRow->bCarriedTool;
    if (PreferArrowsButton)
    {
        const bool bSelectedArrow = SelectedRow && (SelectedRow->Id == TEXT("WoodenArrow")
            || (SelectedRow->Id.ToString().EndsWith(TEXT("Arrow"))
                && !UKalmalaToolCatalogue::Get()->GetAttacks(SelectedRow->Id).IsEmpty()));
        const bool bArrows = bSelectedArrow && bShowSelectedActions;
        PreferArrowsButton->SetVisibility(bArrows ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        if (bArrows)
        {
            auto* Combat = GetOwningPlayerPawn()->FindComponentByClass<UKalmalaCombatComponent>();
            const auto* Inventory = GetOwningPlayerPawn()->FindComponentByClass<UKalmalaInventoryComponent>();
            bool bPreferred = false;
            if (Combat && Inventory)
                for (const auto& Stack : Inventory->GetStacks())
                    if (Stack.InstanceGuid == Combat->GetPreferredArrowStack()
                        && (Stack.InstanceGuid == SelectedGridInstanceGuid || Stack.ItemGuids.Contains(SelectedGridInstanceGuid))) bPreferred = true;
            CastChecked<UTextBlock>(PreferArrowsButton->GetContent())->SetText(FText::FromString(
                bPreferred ? TEXT("Preferred arrows — use automatic selection") : TEXT("Prioritize this arrow stack")));
        }
    }
    RepairToolButton->SetVisibility(bSelectedTool && bShowSelectedActions
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    ToolActionStatusText->SetVisibility(bSelectedTool && bShowSelectedActions
        ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (DetailRow != nullptr)
    {
        const FKalmalaCatalogueRow& Row = *DetailRow;
        const FString DetailKey = FString::Printf(TEXT("%s|%d|%s|%s|%d|%d"), *Row.Id.ToString(),
            Row.bCarriedTool, *Row.Name, *Row.Detail, TextScale, Contrast);
        if (DetailKey != LastSelectedDetailKey)
        {
            if (Row.bCarriedTool)
                ItemDetailView->SetCarriedTool(Row.Id, Row.Name, Row.Detail, TextScale, Contrast);
            else
            {
                FString VisibleState = Row.Detail;
                if (UKalmalaPlayerStatusComponent::IsKnownFoodItem(Row.Id))
                {
                    VisibleState += TEXT("\n\nEffect: Steady Meal; 10% lower stamina use for 120 seconds.");
                }
                else if (UKalmalaPlayerStatusComponent::IsKnownDraughtItem(Row.Id))
                {
                    if (Row.Id == UKalmalaPlayerStatusComponent::BerryRecoveryDraughtItemId)
                        VisibleState += TEXT("\n\nEffect: restores up to 20 health over 10 seconds. Draughts share a 60-second recovery cooldown.");
                    else if (Row.Id == UKalmalaPlayerStatusComponent::RatTailTonicItemId)
                        VisibleState += TEXT("\n\nEffect: restores up to 30 health over 10 seconds. Draughts share a 60-second recovery cooldown.");
                    else if (Row.Id == UKalmalaPlayerStatusComponent::AntidoteItemId)
                        VisibleState += TEXT("\n\nEffect: clears weak poison, reduces active major poison to weak for up to 5 seconds, and grants 10 minutes of poison resistance. Draughts share a 60-second recovery cooldown.");
                    else
                        VisibleState += TEXT("\n\nEffect: restores up to 20 stamina immediately, up to the current maximum. Draughts share a 60-second recovery cooldown.");
                }
                ItemDetailView->SetItem(Row.Id, Row.Name, VisibleState, TextScale, Contrast);
            }
            LastSelectedDetailKey = DetailKey;
        }
        ItemDetailView->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        LastSelectedDetailKey.Reset();
        ItemDetailView->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (bSelectedTool && bShowSelectedActions)
    {
        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
        const FKalmalaToolState* Tool = Character
            ? Character->GetCarriedToolInventory().FindByPredicate([SelectedRow](const FKalmalaToolState& Entry)
                { return Entry.InstanceGuid == SelectedRow->ItemGuid && Entry.ItemId == SelectedRow->Id; }) : nullptr;
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(SelectedRow->Id);
        const int32 MaxDurability = Tool
            ? FKalmalaToolLifecycleContract::GetMaxDurability(Tool->ItemId, Tool->ToolLevel) : 0;
        const bool bTorch = Tool != nullptr && Tool->ItemId == TEXT("BasicTorch");
        const bool bTorchFuelValid = bTorch && FMath::IsFinite(Tool->RemainingBurnSeconds)
            && Tool->RemainingBurnSeconds >= 0.0f
            && Tool->RemainingBurnSeconds <= FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds;
        const bool bToolStateValid = Tool != nullptr && Definition != nullptr && Tool->ToolLevel >= 1
            && MaxDurability > 0 && (bTorch ? bTorchFuelValid
                : Tool->Durability >= 0 && Tool->Durability <= MaxDurability);
        int32 WoodCount = 0;
        if (bTorch && Character)
            if (const UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent())
                for (const FKalmalaInventoryStack& Stack : Inventory->GetStacks())
                    if (Stack.ItemId == TEXT("Wood")) WoodCount += Stack.Quantity;
        const bool bNeedsRepair = bToolStateValid && !bTorch && Tool->Durability < MaxDurability;
        const bool bNeedsRefuel = bToolStateValid && bTorch
            && Tool->RemainingBurnSeconds < FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds
            && WoodCount > 0;
        const bool bHasRepairAction = Character != nullptr
            && Character->FindComponentByClass<UKalmalaCraftingComponent>() != nullptr;
        CastChecked<UTextBlock>(RepairToolButton->GetContent())->SetText(FText::FromString(
            bTorch ? TEXT("Refuel selected torch") : TEXT("Repair selected tool")));
        RepairToolButton->SetIsEnabled((bNeedsRepair || bNeedsRefuel) && bHasRepairAction);
        FString StatusText;
        if (!bToolStateValid) StatusText = TEXT("Tool state unavailable.");
        else if (bAwaitingRepairResult && AwaitingRepairItemId == SelectedRow->Id)
            StatusText = bTorch ? TEXT("Refuel requested. Waiting for the server result.")
                : TEXT("Repair requested. Waiting for the server result.");
        else if (LastRepairResultItemId == SelectedRow->Id && !LastRepairResultText.IsEmpty())
            StatusText = LastRepairResultText;
        else if (bTorch && Tool->RemainingBurnSeconds >= FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds)
            StatusText = TEXT("This torch is fully fuelled.");
        else if (bTorch && WoodCount == 0)
            StatusText = TEXT("Refuelling needs 1 Wood. This torch casts light 6 m and burns for 300 seconds.");
        else if (bTorch)
            StatusText = TEXT("Refuel with 1 Wood. A full torch burns for 300 seconds and casts light 6 m.");
        else if (bNeedsRepair)
            StatusText = TEXT("Repair is free at a visible, same-world Workbench or Forge within 2.5 m.");
        else StatusText = TEXT("This tool is at full condition.");
        if (ToolActionStatusText->GetText().ToString() != StatusText)
            ToolActionStatusText->SetText(FText::FromString(StatusText));
    }
    RefreshFoodActionPresentation(bShowSelectedActions ? SelectedRow : nullptr, TextScale, Contrast);
    RefreshGearPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RefreshGearDivider(const int32 Contrast)
{
    if (!GearDividerImage || !GearDividerRule) return;
    const bool bShowOrnament = Contrast <= 0 && GearDividerImage->GetBrush().GetResourceObject();
    GearDividerImage->SetVisibility(bShowOrnament ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    GearDividerRule->SetVisibility(bShowOrnament ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    GearDividerRule->SetBrushColor(Contrast > 0 ? FLinearColor::White : FKalmalaUITheme::Get().TextColor(true, Contrast));
}

void UKalmalaInventoryMenuWidget::RefreshGearPresentation(const int32 TextScale, const int32 Contrast)
{
    if (GearSlotButtons.Num() != 8 || GearSlotStateTexts.Num() != 8
        || GearSlotIcons.Num() != 8) return;

    static const EKalmalaGearSlot Slots[] = {
        EKalmalaGearSlot::Head, EKalmalaGearSlot::Cape, EKalmalaGearSlot::Chest,
        EKalmalaGearSlot::Shield, EKalmalaGearSlot::Legs, EKalmalaGearSlot::Boots,
        EKalmalaGearSlot::Necklace, EKalmalaGearSlot::Ring};
    static const EKalmalaIcon EmptyIcons[] = {
        EKalmalaIcon::Helmet, EKalmalaIcon::Cape, EKalmalaIcon::Tunic,
        EKalmalaIcon::Shield, EKalmalaIcon::Leggings, EKalmalaIcon::Boots,
        EKalmalaIcon::Necklace, EKalmalaIcon::Ring};
    APlayerController* Controller = GetOwningPlayer();
    UKalmalaInventoryComponent* Inventory = Controller && Controller->GetPawn()
        ? Controller->GetPawn()->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaCatalogueRow* SelectedRow = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
        ? &OwnerInventoryRows[SelectedInventoryIndex] : nullptr;
    const FKalmalaItemDefinition* SelectedDefinition = SelectedRow && Catalogue
        ? Catalogue->FindItem(SelectedRow->Id) : nullptr;
    const FGuid SelectedGuid = SelectedGridInstanceGuid.IsValid() ? SelectedGridInstanceGuid
        : SelectedRow ? SelectedRow->ItemGuid : FGuid();

    const auto* Opal = Inventory ? Inventory->FindEquippedGear(EKalmalaGearSlot::Head) : nullptr;
    const bool bOpal = Opal && Opal->ItemId == TEXT("OpalHelm");
    if (UpgradeOpalButton)
    {
        UpgradeOpalButton->SetVisibility(bOpal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        UpgradeOpalButton->SetIsEnabled(bOpal && Opal->Level < 10);
        UpgradeOpalButton->SetToolTipText(FText::FromString(TEXT("Forge level 1 within 2.5 m. Base cost: 5 Iron + 2 Deer Hide; doubles for each current level. Maximum level 10.")));
    }
    if (RepairOpalButton)
    {
        RepairOpalButton->SetVisibility(bOpal ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        RepairOpalButton->SetIsEnabled(bOpal && Opal->ConditionSpent > 0);
    }
    const bool bTwoHanded = Inventory && FKalmalaToolLifecycleContract::IsTwoHanded(Inventory->GetActiveItem());
    for (int32 Index = 0; Index < 8; ++Index)
    {
        UKalmalaThemedButton* Button = GearSlotButtons[Index];
        UTextBlock* StateText = GearSlotStateTexts[Index];
        if (!Button || !StateText) continue;

        const EKalmalaGearSlot GearSlot = Slots[Index];
        const FKalmalaEquippedGear* Equipped = Inventory ? Inventory->FindEquippedGear(GearSlot) : nullptr;
        UKalmalaIconWidget* Icon = GearSlotIcons[Index];
        if (Equipped) Icon->SetCatalogueIcon(Equipped->ItemId);
        else Icon->SetIcon(EmptyIcons[Index]);
        Icon->SetRenderOpacity(Equipped ? 1.0f : (Contrast > 0 ? 0.7f : 0.4f));
        FString State = TEXT("Empty");
        const bool bCanEquip = SelectedDefinition && SelectedRow && SelectedGuid.IsValid()
            && SelectedDefinition->GearSlot == GetGearCatalogueSlot(GearSlot)
            && (!Equipped || Equipped->ItemGuid != SelectedGuid);
        if (Equipped)
        {
            const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(Equipped->ItemId) : nullptr;
            State = Definition ? Definition->DisplayName : Equipped->ItemId.ToString();
            FKalmalaGearLevelStats Stats;
            if (Catalogue && Catalogue->GetGearLevelStats(Equipped->ItemId, Equipped->Level, Stats)
                && Stats.MaxDurability > 0)
                State += FString::Printf(TEXT(" L%d (%d/%d)"), Equipped->Level,
                    FMath::Max(0, Stats.MaxDurability - Equipped->ConditionSpent), Stats.MaxDurability);
            Button->SetToolTipText(FText::FromString(bCanEquip
                ? FString::Printf(TEXT("Click to replace %s with the selected gear."), *State)
                : FString::Printf(TEXT("Select an empty grid cell, then click to return %s to your inventory."), *State)));
        }
        else
        {
            if (bCanEquip)
            {
                State = FString::Printf(TEXT("Equip %s"), *SelectedDefinition->DisplayName);
                Button->SetToolTipText(FText::FromString(FString::Printf(
                    TEXT("Click to equip %s."), *SelectedDefinition->DisplayName)));
            }
            else
            {
                Button->SetToolTipText(FText::FromString(TEXT("Select compatible gear in the inventory grid.")));
            }
        }

        if (GearSlot == EKalmalaGearSlot::Shield && bTwoHanded)
        {
            State = bCanEquip ? TEXT("Equip shield / return weapon") : TEXT("Both hands occupied");
            Button->SetToolTipText(FText::FromString(
                TEXT("A two-handed weapon uses this hand. Equipping a shield first returns that weapon to the pack.")));
        }
        Button->SetToolTipText(FText::FromString(GetGearCatalogueSlot(GearSlot) + TEXT(": ") + State
            + TEXT("\n") + Button->GetToolTipText().ToString()));
        Button->SetIsEnabled(!bAwaitingGearRequestResult && Inventory
            && (Equipped != nullptr || bCanEquip));
        if (StateText->GetText().ToString() != State)
            StateText->SetText(FText::FromString(State));
    }

    if (MainHandButton && MainHandStateText)
    {
        const bool bHasTool = Inventory && FKalmalaToolLifecycleContract::FindDefinition(Inventory->GetActiveItem());
        if (MainHandIcon)
        {
            if (bHasTool) MainHandIcon->SetCatalogueIcon(Inventory->GetActiveItem());
            else MainHandIcon->SetIcon(EKalmalaIcon::Sword);
            MainHandIcon->SetRenderOpacity(bHasTool ? 1.0f : (Contrast > 0 ? 0.7f : 0.4f));
        }
        const bool bCanSelect = Inventory && SelectedRow && SelectedRow->bCarriedTool
            && SelectedGuid.IsValid() && SelectedGuid != Inventory->GetActiveInstanceGuid();
        const FString State = bHasTool ? GetCarriedToolDisplayName(Inventory->GetActiveItem()) + (bTwoHanded ? TEXT(" (Two hands)") : TEXT(""))
            : bCanSelect ? FString::Printf(TEXT("Select %s"), *SelectedRow->Name) : TEXT("Empty");
        MainHandStateText->SetText(FText::FromString(State));
        MainHandButton->SetIsEnabled(!bAwaitingGearRequestResult && (bCanSelect || bHasTool));
        MainHandButton->SetToolTipText(FText::FromString(bCanSelect
            ? TEXT("Click to use the selected weapon or tool in Main Hand.")
            : TEXT("Click to return Main Hand to a free inventory cell, or select another weapon or tool first.")));
        MainHandButton->SetToolTipText(FText::FromString(TEXT("Main Hand: ") + State
            + TEXT("\n") + MainHandButton->GetToolTipText().ToString()));
    }

    // Keep request results available on hover without adding a footer below the equipment.
    const FString Feedback = bAwaitingGearRequestResult
        ? TEXT("Gear request sent. Waiting for the server result.") : LastGearActionStatus;
    if (!Feedback.IsEmpty())
    {
        for (UKalmalaThemedButton* Button : GearSlotButtons)
            if (Button) Button->SetToolTipText(FText::FromString(Button->GetToolTipText().ToString() + TEXT("\n") + Feedback));
        if (MainHandButton) MainHandButton->SetToolTipText(FText::FromString(
            MainHandButton->GetToolTipText().ToString() + TEXT("\n") + Feedback));
    }
    (void)TextScale;
    (void)Contrast;
}

void UKalmalaInventoryMenuWidget::UseGearSlot(const EKalmalaGearSlot GearSlot)
{
    APlayerController* Controller = GetOwningPlayer();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Controller ? Controller->GetPawn() : nullptr);
    UKalmalaInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
    if (!Controller || !Controller->IsLocalController() || !Inventory || bAwaitingGearRequestResult) return;

    LastSeenGearRequestResultSerial = Inventory->GetGearRequestResultSerial();
    const FKalmalaEquippedGear* ExistingGear = Inventory->FindEquippedGear(GearSlot);
    const FKalmalaCatalogueRow* SelectedRow = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
        ? &OwnerInventoryRows[SelectedInventoryIndex] : nullptr;
    const FGuid SelectedGuid = SelectedGridInstanceGuid.IsValid() ? SelectedGridInstanceGuid
        : SelectedRow ? SelectedRow->ItemGuid : FGuid();
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaItemDefinition* Definition = SelectedRow && Catalogue
        ? Catalogue->FindItem(SelectedRow->Id) : nullptr;
    const bool bCanEquip = SelectedRow && SelectedGuid.IsValid() && Definition
        && Definition->GearSlot == GetGearCatalogueSlot(GearSlot)
        && (!ExistingGear || ExistingGear->ItemGuid != SelectedGuid);
    if (ExistingGear && !bCanEquip)
    {
        bPendingGearEquip = false;
        Inventory->ServerUnequipGear(GearSlot);
    }
    else if (bCanEquip)
    {
        bPendingGearEquip = true;
        Inventory->ServerEquipGear(SelectedGuid);
    }
    else return;

    bAwaitingGearRequestResult = true;
    GearRequestSentAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    LastGearActionStatus = TEXT("Gear request sent.");
    RefreshGearPresentation(UKalmalaSettingsWidget::GetTextScalePercent(),
        UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::UseMainHandSlot()
{
    APlayerController* Controller = GetOwningPlayer();
    auto* Character = Cast<AKalmalaCharacter>(Controller ? Controller->GetPawn() : nullptr);
    auto* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
    if (!Controller || !Controller->IsLocalController() || !Inventory || bAwaitingGearRequestResult) return;
    const FKalmalaCatalogueRow* Row = OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)
        ? &OwnerInventoryRows[SelectedInventoryIndex] : nullptr;
    const FGuid SelectedGuid = SelectedGridInstanceGuid.IsValid() ? SelectedGridInstanceGuid
        : Row ? Row->ItemGuid : FGuid();
    const bool bSelect = Row && Row->bCarriedTool && SelectedGuid.IsValid()
        && SelectedGuid != Inventory->GetActiveInstanceGuid();
    if (!bSelect && !FKalmalaToolLifecycleContract::FindDefinition(Inventory->GetActiveItem())) return;
    LastSeenGearRequestResultSerial = Inventory->GetGearRequestResultSerial();
    bPendingGearEquip = bSelect;
    bAwaitingGearRequestResult = true;
    GearRequestSentAtSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    Inventory->ServerSetMainHand(bSelect ? SelectedGuid : FGuid());
    RefreshGearPresentation(UKalmalaSettingsWidget::GetTextScalePercent(),
        UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::UseHeadGearSlot() { UseGearSlot(EKalmalaGearSlot::Head); }
void UKalmalaInventoryMenuWidget::UseCapeGearSlot() { UseGearSlot(EKalmalaGearSlot::Cape); }
void UKalmalaInventoryMenuWidget::UseChestGearSlot() { UseGearSlot(EKalmalaGearSlot::Chest); }
void UKalmalaInventoryMenuWidget::UseShieldGearSlot() { UseGearSlot(EKalmalaGearSlot::Shield); }
void UKalmalaInventoryMenuWidget::UseLegsGearSlot() { UseGearSlot(EKalmalaGearSlot::Legs); }
void UKalmalaInventoryMenuWidget::UseBootsGearSlot() { UseGearSlot(EKalmalaGearSlot::Boots); }
void UKalmalaInventoryMenuWidget::UseNecklaceGearSlot() { UseGearSlot(EKalmalaGearSlot::Necklace); }
void UKalmalaInventoryMenuWidget::UseRingGearSlot() { UseGearSlot(EKalmalaGearSlot::Ring); }

void UKalmalaInventoryMenuWidget::RefreshFoodActionPresentation(const FKalmalaCatalogueRow* SelectedRow,
    const int32 TextScale, const int32 Contrast)
{
    if (!bSelectionDetailsRequested || !HoveredItemId.IsNone())
    {
        EatFoodButton->SetVisibility(ESlateVisibility::Collapsed);
        FoodActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    const bool bSelectedFood = SelectedRow != nullptr && !SelectedRow->bCarriedTool
        && (UKalmalaPlayerStatusComponent::IsKnownFoodItem(SelectedRow->Id)
            || UKalmalaPlayerStatusComponent::IsKnownDraughtItem(SelectedRow->Id));
    const bool bSelectedDraught = bSelectedFood
        && UKalmalaPlayerStatusComponent::IsKnownDraughtItem(SelectedRow->Id);
    const bool bSelectedBerryDraught = bSelectedDraught
        && SelectedRow->Id == UKalmalaPlayerStatusComponent::BerryRecoveryDraughtItemId;
    const bool bSelectedRatTailTonic = bSelectedDraught
        && SelectedRow->Id == UKalmalaPlayerStatusComponent::RatTailTonicItemId;
    const bool bSelectedAntidote = bSelectedDraught
        && SelectedRow->Id == UKalmalaPlayerStatusComponent::AntidoteItemId;
    const bool bSelectedRootStaminaDraught = bSelectedDraught
        && SelectedRow->Id == UKalmalaPlayerStatusComponent::RootStaminaDraughtItemId;
    const bool bSelectedHealthDraught = bSelectedBerryDraught || bSelectedRatTailTonic;
    const bool bShowFoodStatus = bSelectedFood || bAwaitingFoodResult || !LastFoodResultText.IsEmpty();
    EatFoodButton->SetVisibility(bSelectedFood ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (auto* ButtonLabel = Cast<UTextBlock>(EatFoodButton->GetContent()))
    {
        const FText ActionLabel = FText::FromString(bSelectedDraught ? TEXT("Use one dose") : TEXT("Eat one serving"));
        if (ButtonLabel->GetText().ToString() != ActionLabel.ToString()) ButtonLabel->SetText(ActionLabel);
    }
    FoodActionStatusText->SetVisibility(bShowFoodStatus ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (!bShowFoodStatus) return;

    const APlayerController* Controller = GetOwningPlayer();
    const APawn* OwnerPawn = Controller ? Controller->GetPawn() : nullptr;
    const UKalmalaInventoryComponent* Inventory = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const UKalmalaPlayerStatusComponent* Status = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    const UKalmalaCraftingComponent* Crafting = OwnerPawn
        ? OwnerPawn->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(OwnerPawn);
    const auto* Movement = Character
        ? Cast<UKalmalaCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;

    bool bCanUse = false;
    FString StatusText;
    if (bAwaitingFoodResult)
    {
        const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
        const FKalmalaItemDefinition* PendingDefinition = Catalogue ? Catalogue->FindItem(AwaitingFoodItemId) : nullptr;
        FString PendingName(TEXT("food"));
        if (PendingDefinition) PendingName = PendingDefinition->DisplayName;
        StatusText = FString::Printf(TEXT("Waiting for the server result for %s."), *PendingName);
    }
    else if (bSelectedFood)
    {
        const int32 Quantity = Inventory ? Inventory->GetQuantity(SelectedRow->Id) : 0;
        if (!Controller || !Controller->IsLocalController() || !Inventory || !Status || !Crafting)
        {
            StatusText = TEXT("Food use is unavailable while owner data is loading.");
        }
        else if (bSelectedDraught && Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId))
        {
            StatusText = FString::Printf(TEXT("Draught recovery cooldown: %.0f seconds remain. No draught dose was used."),
                Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId));
        }
        else if (bSelectedHealthDraught
            && (Status->HasStatus(UKalmalaPlayerStatusComponent::BerryRecoveryStatusId)
                || Status->HasStatus(UKalmalaPlayerStatusComponent::RatTailTonicStatusId)))
        {
            const float BerryRecoveryRemaining = Status->GetRemainingSeconds(
                UKalmalaPlayerStatusComponent::BerryRecoveryStatusId);
            const float TonicRecoveryRemaining = Status->GetRemainingSeconds(
                UKalmalaPlayerStatusComponent::RatTailTonicStatusId);
            StatusText = FString::Printf(TEXT("Health draught healing is active for %.0f seconds; the shared cooldown continues."),
                FMath::Max(BerryRecoveryRemaining, TonicRecoveryRemaining));
        }
        else if (bSelectedDraught && (!Character || Character->GetHealth() <= 1.0f))
        {
            StatusText = TEXT("A draught cannot be used while dead.");
        }
        else if (bSelectedAntidote && Status->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId))
        {
            StatusText = FString::Printf(TEXT("Poison resistance is already active for %.0f seconds; the Antidote will not be consumed."),
                Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId));
        }
        else if (bSelectedHealthDraught && Character->GetHealth() >= Character->GetMaximumHealth())
        {
            StatusText = TEXT("Health is full; the draught will not be consumed.");
        }
        else if (bSelectedRootStaminaDraught && !Movement)
        {
            StatusText = TEXT("Stamina is unavailable while owner data is loading.");
        }
        else if (bSelectedRootStaminaDraught
            && Movement->GetStamina() >= Movement->GetMaximumStamina())
        {
            StatusText = TEXT("Stamina is full; the draught will not be consumed.");
        }
        else if (!bSelectedDraught && Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
        {
            StatusText = FString::Printf(TEXT("A steady meal is active for %.0f seconds. Wait for it to expire."),
                Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId));
        }
        else if (bAwaitingAcceptedFoodStatus)
        {
            StatusText = bSelectedDraught
                ? TEXT("Draught was accepted. Waiting for the owner recovery status to update.")
                : TEXT("Food was accepted. Waiting for the owner meal status to update.");
        }
        else if (Quantity <= 0)
        {
            StatusText = bSelectedDraught
                ? TEXT("No draught dose is available in your pack.")
                : TEXT("No serving of this food is available in your pack.");
        }
        else
        {
            StatusText = bSelectedDraught
                ? (bSelectedBerryDraught
                    ? TEXT("Ready: use one dose to restore up to 20 health over 10 seconds. Shared draught cooldown: 60 seconds.")
                    : bSelectedRatTailTonic
                        ? TEXT("Ready: use one dose to restore up to 30 health over 10 seconds. Shared draught cooldown: 60 seconds.")
                        : bSelectedAntidote
                            ? TEXT("Ready: use one dose to clear weak poison, reduce major poison to weak for up to 5 seconds, and gain 10 minutes of poison resistance. Shared draught cooldown: 60 seconds.")
                            : TEXT("Ready: use one dose to restore up to 20 stamina immediately, up to the current maximum. Shared draught cooldown: 60 seconds."))
                : TEXT("Ready: eat one serving for 120 seconds of 10% lower stamina use.");
            bCanUse = !bAwaitingFoodResult && !bAwaitingAcceptedFoodStatus;
        }

        if (LastFoodResultItemId == SelectedRow->Id && !LastFoodResultText.IsEmpty())
        {
            StatusText += TEXT("\nLast server result: ") + LastFoodResultText;
        }
    }
    else
    {
        StatusText = TEXT("Last food result: ") + LastFoodResultText;
    }

    EatFoodButton->SetIsEnabled(bCanUse);
    if (FoodActionStatusText->GetText().ToString() != StatusText)
        FoodActionStatusText->SetText(FText::FromString(StatusText));
}

void UKalmalaInventoryMenuWidget::EatSelectedFood()
{
    if (bAwaitingFoodResult || bAwaitingAcceptedFoodStatus
        || !OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)) return;

    const FKalmalaCatalogueRow& Row = OwnerInventoryRows[SelectedInventoryIndex];
    const bool bDraught = UKalmalaPlayerStatusComponent::IsKnownDraughtItem(Row.Id);
    const bool bHealthDraught = Row.Id == UKalmalaPlayerStatusComponent::BerryRecoveryDraughtItemId
        || Row.Id == UKalmalaPlayerStatusComponent::RatTailTonicItemId;
    if (Row.bCarriedTool || (!bDraught && !UKalmalaPlayerStatusComponent::IsKnownFoodItem(Row.Id))) return;

    APlayerController* Controller = GetOwningPlayer();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Controller ? Controller->GetPawn() : nullptr);
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    UKalmalaPlayerStatusComponent* Status = Character
        ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    UKalmalaCraftingComponent* Crafting = Character
        ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    const bool bEffectAlreadyActive = Status && (bDraught
        ? (Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId)
            || (Row.Id == UKalmalaPlayerStatusComponent::AntidoteItemId
                && Status->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId)))
        : Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId));
    if (!Controller || !Controller->IsLocalController() || !Inventory || !Status || !Crafting
        || Inventory->GetQuantity(Row.Id) <= 0
        || bEffectAlreadyActive || (bDraught && (!Character || Character->GetHealth() <= 1.0f
            || (bHealthDraught && Character->GetHealth() >= Character->GetMaximumHealth()))))
    {
        RefreshSelectionPresentation(UKalmalaSettingsWidget::ClampTextScale(
            UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());
        return;
    }

    FoodRequestResultSerial = Crafting->GetResultSerial();
    AwaitingFoodItemId = Row.Id;
    bAwaitingFoodResult = true;
    bAwaitingAcceptedFoodStatus = false;
    LastFoodResultText.Reset();
    LastFoodResultItemId = NAME_None;
    RefreshSelectionPresentation(UKalmalaSettingsWidget::ClampTextScale(
        UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());

    // This existing server transaction is station-free. It rechecks the
    // item, owner pack and effect slot before publishing an owner result.
    Crafting->ServerConsumeFood(Row.Id);
}

void UKalmalaInventoryMenuWidget::StepSelection(const int32 Direction)
{
    if (OwnerInventoryRows.IsEmpty()) return;
    SelectedGridInstanceGuid.Invalidate();
    const int32 NumRows = OwnerInventoryRows.Num();
    const int32 Current = SelectedInventoryIndex == INDEX_NONE ? 0 : SelectedInventoryIndex;
    SelectedInventoryIndex = (Current + NumRows + (Direction < 0 ? -1 : 1)) % NumRows;
    RememberedSelectedItemId = OwnerInventoryRows[SelectedInventoryIndex].Id;
    HoveredItemId = NAME_None;
    bSelectionDetailsRequested = true;
    const int32 TextScale = UKalmalaSettingsWidget::ClampTextScale(UKalmalaSettingsWidget::GetTextScalePercent());
    RefreshSelectionPresentation(TextScale, UKalmalaSettingsWidget::GetContrastMode());
}

void UKalmalaInventoryMenuWidget::RepairSelectedTool()
{
    if (!OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)) return;
    const FKalmalaCatalogueRow& Row = OwnerInventoryRows[SelectedInventoryIndex];
    if (!Row.bCarriedTool) return;

    APlayerController* Controller = GetOwningPlayer();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Controller ? Controller->GetPawn() : nullptr);
    const FKalmalaToolState* Tool = Character
        ? Character->GetCarriedToolInventory().FindByPredicate([&Row](const FKalmalaToolState& Entry)
            { return Entry.InstanceGuid == Row.ItemGuid && Entry.ItemId == Row.Id; })
        : nullptr;
    const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Row.Id);
    UKalmalaCraftingComponent* Crafting = Character
        ? Character->FindComponentByClass<UKalmalaCraftingComponent>()
        : nullptr;
    const int32 MaxDurability = Tool
        ? FKalmalaToolLifecycleContract::GetMaxDurability(Tool->ItemId, Tool->ToolLevel) : 0;
    const bool bTorch = Tool != nullptr && Tool->ItemId == TEXT("BasicTorch");
    const bool bTorchCanRefuel = bTorch && FMath::IsFinite(Tool->RemainingBurnSeconds)
        && Tool->RemainingBurnSeconds >= 0.0f
        && Tool->RemainingBurnSeconds < FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds;
    if (Controller == nullptr || !Controller->IsLocalController() || Tool == nullptr || Definition == nullptr
        || Tool->ToolLevel < 1 || MaxDurability <= 0 || Crafting == nullptr
        || (bTorch ? !bTorchCanRefuel : Tool->Durability < 0 || Tool->Durability >= MaxDurability))
    {
        RefreshSelectionPresentation(UKalmalaSettingsWidget::ClampTextScale(
            UKalmalaSettingsWidget::GetTextScalePercent()), UKalmalaSettingsWidget::GetContrastMode());
        return;
    }

    RepairRequestResultSerial = Crafting->GetResultSerial();
    AwaitingRepairItemId = Row.Id;
    bAwaitingRepairResult = true;
    LastRepairResultText.Reset();
    LastRepairResultItemId = NAME_None;
    ToolActionStatusText->SetText(FText::FromString(bTorch
        ? TEXT("Refuel requested. Waiting for the server result.")
        : TEXT("Repair requested. Waiting for the server result.")));
    if (bTorch) Crafting->ServerRefuelTorch(Row.ItemGuid);
    else Crafting->ServerRepairTool(Row.Id);
    RefreshOwnerInventory();
}

#if !UE_BUILD_SHIPPING
void UKalmalaInventoryMenuWidget::SetInventoryRowsForVerification(const TArray<FKalmalaCatalogueRow>& Rows,
    const int32 TextScale, const int32 Contrast)
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        NativeOnInitialized();
    }

    ApplyInventoryRows(TArray<FKalmalaCatalogueRow>(Rows), true, TextScale, Contrast);
}

FName UKalmalaInventoryMenuWidget::GetSelectedItemForVerification() const
{
    return OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex) ? OwnerInventoryRows[SelectedInventoryIndex].Id : NAME_None;
}

void UKalmalaInventoryMenuWidget::StepSelectionForVerification(const int32 Direction)
{
    StepSelection(Direction);
}

void UKalmalaInventoryMenuWidget::SetViewportSizeForVerification(const FVector2D ViewportSize)
{
    UpdateResponsivePanelSize(ViewportSize);
}

void UKalmalaInventoryMenuWidget::PrepareMenuRender(const FVector2D ViewportSize)
{
    RefreshOwnerInventory();
    UpdateResponsivePanelSize(ViewportSize);
    SetVisibility(ESlateVisibility::Visible);
}
#endif

void UKalmalaInventoryMenuWidget::Close()
{
    if (!bMenuOpen) return;
    if (GridView) GridView->CancelMove();
    if (CraftingCompanion) { CraftingCompanion->CloseInventoryCompanion(); CraftingCompanion->RemoveFromParent(); }

    bMenuOpen = false;
    bOpeningAnimationActive = false;
    OpeningElapsed = 0.0f;
    HoveredItemId = NAME_None;
    bSelectionDetailsRequested = false;
    if (InventoryPanel) InventoryPanel->SetRenderTranslation(FVector2D::ZeroVector);
    if (CraftingCompanion) CraftingCompanion->SetRenderTranslation(FVector2D::ZeroVector);
    SetVisibility(ESlateVisibility::Collapsed);

    if (APlayerController* Controller = GetOwningPlayer())
    {
        if (bAcquiredMoveIgnore) Controller->SetIgnoreMoveInput(false);
        if (bAcquiredLookIgnore) Controller->SetIgnoreLookInput(false);
        Controller->bShowMouseCursor = bPreviousCursorVisibility;
        Controller->SetInputMode(FInputModeGameOnly());
    }
    bAcquiredMoveIgnore = false;
    bAcquiredLookIgnore = false;
}

bool UKalmalaInventoryMenuWidget::HasTextEntryFocus() const
{
    if (!FSlateApplication::IsInitialized()) return false;

    const TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
    return FocusedWidget.IsValid()
        && FocusedWidget->GetTypeAsString().Contains(TEXT("EditableText"), ESearchCase::CaseSensitive);
}

FReply UKalmalaInventoryMenuWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (bMenuOpen && (Event.GetKey() == EKeys::Tab || (Event.GetKey() == EKeys::I && !HasTextEntryFocus())))
    { Close(); return FReply::Handled(); }
    if (bMenuOpen && NavigateInventoryMenu(Event.GetKey())) return FReply::Handled();
    return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

FReply UKalmalaInventoryMenuWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (bMenuOpen && (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right))
    { Close(); return FReply::Handled(); }
    return Super::NativeOnKeyDown(Geometry, Event);
}

void UKalmalaInventoryMenuWidget::PreferSelectedArrows()
{
    auto* Pawn = GetOwningPlayerPawn();
    auto* Combat = Pawn ? Pawn->FindComponentByClass<UKalmalaCombatComponent>() : nullptr;
    auto* Inventory = Pawn ? Pawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Combat || !Inventory || !SelectedGridInstanceGuid.IsValid()) return;
    for (const auto& Stack : Inventory->GetStacks())
        if ((Stack.ItemId == TEXT("WoodenArrow") || (Stack.ItemId.ToString().EndsWith(TEXT("Arrow"))
            && !UKalmalaToolCatalogue::Get()->GetAttacks(Stack.ItemId).IsEmpty()))
            && (Stack.InstanceGuid == SelectedGridInstanceGuid || Stack.ItemGuids.Contains(SelectedGridInstanceGuid)))
        { Combat->ServerPreferArrowStack(Combat->GetPreferredArrowStack() == Stack.InstanceGuid ? FGuid() : SelectedGridInstanceGuid); return; }
}

void UKalmalaInventoryMenuWidget::UpgradeOpalHelm()
{
    auto* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    auto* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
    const auto* Gear = Inventory ? Inventory->FindEquippedGear(EKalmalaGearSlot::Head) : nullptr;
    auto* Crafting = Character ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    if (Character && Character->IsLocallyControlled() && Gear && Gear->ItemId == TEXT("OpalHelm") && Crafting)
        Crafting->ServerUpgradeOpalHelm(Gear->ItemGuid);
}

void UKalmalaInventoryMenuWidget::RepairOpalHelm()
{
    auto* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    auto* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
    const auto* Gear = Inventory ? Inventory->FindEquippedGear(EKalmalaGearSlot::Head) : nullptr;
    auto* Crafting = Character ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    if (Character && Character->IsLocallyControlled() && Gear && Gear->ItemId == TEXT("OpalHelm") && Crafting)
        Crafting->ServerRepairOpalHelm(Gear->ItemGuid);
}
