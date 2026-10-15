#include "KalmalaInventoryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaCraftingSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaInventoryGridWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaUITheme.h"
#include "Styling/SlateTypes.h"

namespace
{
FString GetCarriedToolDisplayName(const FName ItemId)
{
    if (ItemId == TEXT("StoneMace")) return TEXT("Stone Mace");
    if (ItemId == TEXT("BronzeMace")) return TEXT("Bronze Mace");
    if (ItemId == TEXT("IronMace")) return TEXT("Iron Mace");
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
    return ItemId.ToString();
}

FString BuildToolInspectionText(const FKalmalaToolState& Tool)
{
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
    MenuContentScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    Theme.ApplyScroll(*MenuContentScrollBox);
    MenuContentScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
    MenuContentScrollBox->OnUserScrolled.AddDynamic(this, &ThisClass::MenuScrolled);
    MenuContentScrollBox->AddChild(Content);
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

    auto* WornGearPanel = WidgetTree->ConstructWidget<UBorder>();
    Theme.ApplySolidPanel(*WornGearPanel, Contrast);
    WornGearPanel->SetPadding(FMargin(Theme.PaddingX * 0.5f, Theme.PaddingY * 0.5f));
    auto* WornGearContent = WidgetTree->ConstructWidget<UVerticalBox>();
    WornGearPanel->SetContent(WornGearContent);
    auto* WornGearHeading = WidgetTree->ConstructWidget<UTextBlock>();
    WornGearHeading->SetText(FText::FromString(TEXT("Worn Gear")));
    Theme.ApplyText(*WornGearHeading, Theme.EmphasisSize, true, TextScale, Contrast);
    WornGearContent->AddChildToVerticalBox(WornGearHeading)->SetPadding(FMargin(0, 0, 0, Theme.RowSpacing));

    auto AddGearSlot = [&](UHorizontalBox& Row, const FString& SlotName,
        const bool bInteractive, const EKalmalaGearSlot GearSlot)
    {
        auto* SlotSize = WidgetTree->ConstructWidget<USizeBox>();
        SlotSize->SetHeightOverride(72.0f);
        auto* SlotContent = WidgetTree->ConstructWidget<UVerticalBox>();
        auto* Name = WidgetTree->ConstructWidget<UTextBlock>();
        Name->SetText(FText::FromString(SlotName));
        Name->SetAutoWrapText(true);
        Name->SetJustification(ETextJustify::Center);
        Theme.ApplyText(*Name, Theme.BodySize, true, TextScale, Contrast);
        SlotContent->AddChildToVerticalBox(Name);
        auto* Empty = WidgetTree->ConstructWidget<UTextBlock>();
        Empty->SetText(FText::FromString(TEXT("Empty")));
        Empty->SetJustification(ETextJustify::Center);
        Theme.ApplyText(*Empty, Theme.BodySize, false, TextScale, Contrast);
        SlotContent->AddChildToVerticalBox(Empty);
        if (bInteractive)
        {
            auto* Button = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
            Button->SetContent(SlotContent);
            Theme.ApplyButton(*Button, Contrast);
            switch (GearSlot)
            {
            case EKalmalaGearSlot::Head: Button->OnClicked.AddDynamic(this, &ThisClass::UseHeadGearSlot); break;
            case EKalmalaGearSlot::Cape: Button->OnClicked.AddDynamic(this, &ThisClass::UseCapeGearSlot); break;
            case EKalmalaGearSlot::Chest: Button->OnClicked.AddDynamic(this, &ThisClass::UseChestGearSlot); break;
            case EKalmalaGearSlot::Shield: Button->OnClicked.AddDynamic(this, &ThisClass::UseShieldGearSlot); break;
            case EKalmalaGearSlot::Legs: Button->OnClicked.AddDynamic(this, &ThisClass::UseLegsGearSlot); break;
            case EKalmalaGearSlot::Boots: Button->OnClicked.AddDynamic(this, &ThisClass::UseBootsGearSlot); break;
            default: break;
            }
            GearSlotButtons.Add(Button);
            GearSlotNameTexts.Add(Name);
            GearSlotStateTexts.Add(Empty);
            SlotSize->SetContent(Button);
        }
        else
        {
            MainHandButton = WidgetTree->ConstructWidget<UKalmalaThemedButton>();
            Theme.ApplyButton(*MainHandButton, Contrast);
            MainHandButton->SetContent(SlotContent);
            MainHandButton->OnClicked.AddDynamic(this, &ThisClass::UseMainHandSlot);
            MainHandNameText = Name;
            MainHandStateText = Empty;
            SlotSize->SetContent(MainHandButton);
        }
        auto* RowSlot = Row.AddChildToHorizontalBox(SlotSize);
        FSlateChildSize RowSize(ESlateSizeRule::Fill);
        RowSize.Value = 1.0f;
        RowSlot->SetSize(RowSize);
        RowSlot->SetPadding(FMargin(0, 0, Theme.RowSpacing, 0));
    };

    auto* GearRowOne = WidgetTree->ConstructWidget<UHorizontalBox>();
    AddGearSlot(*GearRowOne, TEXT("Main Hand"), false, EKalmalaGearSlot::Head);
    AddGearSlot(*GearRowOne, TEXT("Head"), true, EKalmalaGearSlot::Head);
    AddGearSlot(*GearRowOne, TEXT("Cape"), true, EKalmalaGearSlot::Cape);
    AddGearSlot(*GearRowOne, TEXT("Chest"), true, EKalmalaGearSlot::Chest);
    WornGearContent->AddChildToVerticalBox(GearRowOne);
    auto* GearRowTwo = WidgetTree->ConstructWidget<UHorizontalBox>();
    AddGearSlot(*GearRowTwo, TEXT("Shield"), true, EKalmalaGearSlot::Shield);
    AddGearSlot(*GearRowTwo, TEXT("Legs"), true, EKalmalaGearSlot::Legs);
    AddGearSlot(*GearRowTwo, TEXT("Boots"), true, EKalmalaGearSlot::Boots);
    auto* GearSpacer = WidgetTree->ConstructWidget<USizeBox>();
    GearSpacer->SetHeightOverride(72.0f);
    GearSpacer->SetVisibility(ESlateVisibility::Hidden);
    FSlateChildSize GearSpacerSize(ESlateSizeRule::Fill);
    GearSpacerSize.Value = 1.0f;
    GearRowTwo->AddChildToHorizontalBox(GearSpacer)->SetSize(GearSpacerSize);
    WornGearContent->AddChildToVerticalBox(GearRowTwo)->SetPadding(FMargin(0, Theme.RowSpacing, 0, 0));
    GearActionStatusText = WidgetTree->ConstructWidget<UTextBlock>();
    GearActionStatusText->SetAutoWrapText(true);
    GearActionStatusText->SetText(FText::FromString(
        TEXT("Select a wearable in the grid to equip it. Click worn gear to return it to your inventory.")));
    Theme.ApplyText(*GearActionStatusText, Theme.BodySize, false, TextScale, Contrast);
    WornGearContent->AddChildToVerticalBox(GearActionStatusText)->SetPadding(FMargin(0, Theme.RowSpacing, 0, 0));
    Content->AddChildToVerticalBox(WornGearPanel)->SetPadding(FMargin(0, Theme.RowSpacing, 0, Theme.RowSpacing));

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
    RepairToolButton = AddAction(TEXT("Repair selected tool"));
    RepairToolButton->OnClicked.AddDynamic(this, &ThisClass::RepairSelectedTool);
    ToolActionStatusText = AddText(TEXT(""), false);
    ToolActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
    EatFoodButton = AddAction(TEXT("Eat one serving"));
    EatFoodButton->OnClicked.AddDynamic(this, &ThisClass::EatSelectedFood);
    FoodActionStatusText = AddText(TEXT(""), false);
    FoodActionStatusText->SetVisibility(ESlateVisibility::Collapsed);
    InventoryPanel = WidgetTree->ConstructWidget<UBorder>();
    Theme.ApplyPanel(*InventoryPanel, Contrast, &Theme.InventoryPanelImage);
    InventoryPanel->SetPadding(FMargin(Theme.PaddingX, Theme.PaddingY));
    InventoryPanel->SetContent(MenuContentScrollBox);
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
    constexpr float WornGearHeight = 218.0f;
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
    if (MenuContentScrollBox) MenuContentScrollBox->SetScrollOffset(RememberedMenuScrollOffset);


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
            if (const UKalmalaPlayerStatusComponent* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
                Status && Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
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
void UKalmalaInventoryMenuWidget::MenuScrolled(const float Offset)
{
    RememberedMenuScrollOffset = Offset;
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
        for (UKalmalaThemedButton* Button : GearSlotButtons)
            if (Button) Theme.ApplyButton(*Button, Contrast);
        if (MainHandButton) Theme.ApplyButton(*MainHandButton, Contrast);
        if (MainHandNameText) Theme.ApplyText(*MainHandNameText, Theme.BodySize, true, TextScale, Contrast);
        if (MainHandStateText) Theme.ApplyText(*MainHandStateText, Theme.BodySize, false, TextScale, Contrast);
        for (UTextBlock* Name : GearSlotNameTexts)
            if (Name) Theme.ApplyText(*Name, Theme.BodySize, true, TextScale, Contrast);
        for (UTextBlock* State : GearSlotStateTexts)
            if (State) Theme.ApplyText(*State, Theme.BodySize, false, TextScale, Contrast);
        if (GearActionStatusText) Theme.ApplyText(*GearActionStatusText, Theme.BodySize, false, TextScale, Contrast);

        if (MenuContentScrollBox) Theme.ApplyScroll(*MenuContentScrollBox);
        LastTextScalePercent = TextScale;
        LastContrastMode = Contrast;
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
        const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
        const FKalmalaToolState* Tool = Character
            ? Character->GetCarriedToolInventory().FindByPredicate([SelectedRow](const FKalmalaToolState& Entry)
                { return Entry.ItemId == SelectedRow->Id; }) : nullptr;
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(SelectedRow->Id);
        const int32 MaxDurability = Tool
            ? FKalmalaToolLifecycleContract::GetMaxDurability(Tool->ItemId, Tool->ToolLevel) : 0;
        const bool bToolStateValid = Tool != nullptr && Definition != nullptr && Tool->ToolLevel >= 1
            && MaxDurability > 0 && Tool->Durability >= 0 && Tool->Durability <= MaxDurability;
        const bool bNeedsRepair = bToolStateValid && Tool->Durability < MaxDurability;
        const bool bHasRepairAction = Character != nullptr
            && Character->FindComponentByClass<UKalmalaCraftingComponent>() != nullptr;
        RepairToolButton->SetIsEnabled(bNeedsRepair && bHasRepairAction);
        FString StatusText;
        if (!bToolStateValid) StatusText = TEXT("Tool state unavailable.");
        else if (bAwaitingRepairResult && AwaitingRepairItemId == SelectedRow->Id)
            StatusText = TEXT("Repair requested. Waiting for the server result.");
        else if (LastRepairResultItemId == SelectedRow->Id && !LastRepairResultText.IsEmpty())
            StatusText = LastRepairResultText;
        else if (bNeedsRepair)
            StatusText = TEXT("Repair is free at a visible, same-world Workbench or Forge within 2.5 m.");
        else StatusText = TEXT("This tool is at full condition.");
        if (ToolActionStatusText->GetText().ToString() != StatusText)
            ToolActionStatusText->SetText(FText::FromString(StatusText));
    }
    RefreshFoodActionPresentation(bShowSelectedActions ? SelectedRow : nullptr, TextScale, Contrast);
    RefreshGearPresentation(TextScale, Contrast);
}

void UKalmalaInventoryMenuWidget::RefreshGearPresentation(const int32 TextScale, const int32 Contrast)
{
    if (GearSlotButtons.Num() != 6 || GearSlotStateTexts.Num() != 6 || GearActionStatusText == nullptr) return;

    static const EKalmalaGearSlot Slots[] = {
        EKalmalaGearSlot::Head, EKalmalaGearSlot::Cape, EKalmalaGearSlot::Chest,
        EKalmalaGearSlot::Shield, EKalmalaGearSlot::Legs, EKalmalaGearSlot::Boots};
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

    const bool bTwoHanded = Inventory && FKalmalaToolLifecycleContract::IsTwoHanded(Inventory->GetActiveItem());
    for (int32 Index = 0; Index < 6; ++Index)
    {
        UKalmalaThemedButton* Button = GearSlotButtons[Index];
        UTextBlock* StateText = GearSlotStateTexts[Index];
        if (!Button || !StateText) continue;

        const EKalmalaGearSlot GearSlot = Slots[Index];
        const FKalmalaEquippedGear* Equipped = Inventory ? Inventory->FindEquippedGear(GearSlot) : nullptr;
        FString State = TEXT("Empty");
        const bool bCanEquip = SelectedDefinition && SelectedRow && SelectedGuid.IsValid()
            && SelectedDefinition->GearSlot == GetGearCatalogueSlot(GearSlot)
            && (!Equipped || Equipped->ItemGuid != SelectedGuid);
        if (Equipped)
        {
            const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(Equipped->ItemId) : nullptr;
            State = Definition ? Definition->DisplayName : Equipped->ItemId.ToString();
            FKalmalaGearLevelStats Stats;
            if (Catalogue && Catalogue->GetGearLevelStats(Equipped->ItemId, 1, Stats))
                State += FString::Printf(TEXT(" (%d/%d)"),
                    FMath::Max(0, Stats.MaxDurability - Equipped->ConditionSpent), Stats.MaxDurability);
            Button->SetToolTipText(FText::FromString(bCanEquip
                ? FString::Printf(TEXT("Click to replace %s with the selected wearable."), *State)
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
                Button->SetToolTipText(FText::FromString(TEXT("Select a compatible wearable in the inventory grid.")));
            }
        }

        if (GearSlot == EKalmalaGearSlot::Shield && bTwoHanded)
        {
            State = bCanEquip ? TEXT("Equip shield / return weapon") : TEXT("Both hands occupied");
            Button->SetToolTipText(FText::FromString(
                TEXT("A two-handed weapon uses this hand. Equipping a shield first returns that weapon to the pack.")));
        }
        Button->SetIsEnabled(!bAwaitingGearRequestResult && Inventory
            && (Equipped != nullptr || bCanEquip));
        if (StateText->GetText().ToString() != State)
            StateText->SetText(FText::FromString(State));
    }

    if (MainHandButton && MainHandStateText)
    {
        const bool bHasTool = Inventory && FKalmalaToolLifecycleContract::FindDefinition(Inventory->GetActiveItem());
        const bool bCanSelect = Inventory && SelectedRow && SelectedRow->bCarriedTool
            && SelectedGuid.IsValid() && SelectedGuid != Inventory->GetActiveInstanceGuid();
        const FString State = bHasTool ? GetCarriedToolDisplayName(Inventory->GetActiveItem()) + (bTwoHanded ? TEXT(" (Two hands)") : TEXT(""))
            : bCanSelect ? FString::Printf(TEXT("Select %s"), *SelectedRow->Name) : TEXT("Empty");
        MainHandStateText->SetText(FText::FromString(State));
        MainHandButton->SetIsEnabled(!bAwaitingGearRequestResult && (bCanSelect || bHasTool));
        MainHandButton->SetToolTipText(FText::FromString(bCanSelect
            ? TEXT("Click to use the selected weapon or tool in Main Hand.")
            : TEXT("Click to return Main Hand to a free inventory cell, or select another weapon or tool first.")));
    }

    FString Status = LastGearActionStatus.IsEmpty()
        ? TEXT("Select gear in the grid, then click its slot. Select a weapon or tool for Main Hand.")
        : LastGearActionStatus;
    if (bAwaitingGearRequestResult) Status = TEXT("Gear request sent. Waiting for the server result.");
    if (GearActionStatusText->GetText().ToString() != Status)
        GearActionStatusText->SetText(FText::FromString(Status));
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
        && UKalmalaPlayerStatusComponent::IsKnownFoodItem(SelectedRow->Id);
    const bool bShowFoodStatus = bSelectedFood || bAwaitingFoodResult || !LastFoodResultText.IsEmpty();
    EatFoodButton->SetVisibility(bSelectedFood ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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

    bool bCanEat = false;
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
        const bool bMealActive = Status && Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
        if (!Controller || !Controller->IsLocalController() || !Inventory || !Status || !Crafting)
        {
            StatusText = TEXT("Food use is unavailable while owner data is loading.");
        }
        else if (bMealActive)
        {
            StatusText = FString::Printf(TEXT("A steady meal is active for %.0f seconds. Wait for it to expire."),
                Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId));
        }
        else if (bAwaitingAcceptedFoodStatus)
        {
            StatusText = TEXT("Food was accepted. Waiting for the owner meal status to update.");
        }
        else if (Quantity <= 0)
        {
            StatusText = TEXT("No serving of this food is available in your pack.");
        }
        else
        {
            StatusText = TEXT("Ready: eat one serving for 120 seconds of 10% lower stamina use.");
            bCanEat = !bAwaitingFoodResult && !bAwaitingAcceptedFoodStatus;
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

    EatFoodButton->SetIsEnabled(bCanEat);
    if (FoodActionStatusText->GetText().ToString() != StatusText)
        FoodActionStatusText->SetText(FText::FromString(StatusText));
}

void UKalmalaInventoryMenuWidget::EatSelectedFood()
{
    if (bAwaitingFoodResult || bAwaitingAcceptedFoodStatus
        || !OwnerInventoryRows.IsValidIndex(SelectedInventoryIndex)) return;

    const FKalmalaCatalogueRow& Row = OwnerInventoryRows[SelectedInventoryIndex];
    if (Row.bCarriedTool || !UKalmalaPlayerStatusComponent::IsKnownFoodItem(Row.Id)) return;

    APlayerController* Controller = GetOwningPlayer();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Controller ? Controller->GetPawn() : nullptr);
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    UKalmalaPlayerStatusComponent* Status = Character
        ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    UKalmalaCraftingComponent* Crafting = Character
        ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    if (!Controller || !Controller->IsLocalController() || !Inventory || !Status || !Crafting
        || Inventory->GetQuantity(Row.Id) <= 0
        || Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
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
    // allowlisted item, owner pack and meal slot, then publishes an owner result.
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
            { return Entry.ItemId == Row.Id; })
        : nullptr;
    const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Row.Id);
    UKalmalaCraftingComponent* Crafting = Character
        ? Character->FindComponentByClass<UKalmalaCraftingComponent>()
        : nullptr;
    const int32 MaxDurability = Tool
        ? FKalmalaToolLifecycleContract::GetMaxDurability(Tool->ItemId, Tool->ToolLevel) : 0;
    if (Controller == nullptr || !Controller->IsLocalController() || Tool == nullptr || Definition == nullptr
        || Tool->ToolLevel < 1 || MaxDurability <= 0 || Tool->Durability < 0
        || Tool->Durability >= MaxDurability || Crafting == nullptr)
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
    ToolActionStatusText->SetText(FText::FromString(TEXT("Repair requested. Waiting for the server result.")));
    Crafting->ServerRepairTool(Row.Id);
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
    if (MenuContentScrollBox) RememberedMenuScrollOffset = MenuContentScrollBox->GetScrollOffset();
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
