#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaInventoryMenuWidget.generated.h"

class UTextBlock;
class USizeBox;
class UImage;
class UKalmalaItemDetailWidget;
class UKalmalaThemedButton;
class UKalmalaInventoryGridWidget;
class UKalmalaCraftingWidget;
class UBorder;
class UHorizontalBox;
class UKalmalaIconWidget;
enum class EKalmalaGearSlot : uint8;

/** One owner-local inventory grid with existing selected-tool and food actions. */
UCLASS()
class KALMALAUI_API UKalmalaInventoryMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void Open();
    void Close();
    void RefreshOwnerInventory();
    bool IsMenuOpen() const { return bMenuOpen; }
    bool HasTextEntryFocus() const;
#if !UE_BUILD_SHIPPING
    void SetInventoryRowsForVerification(const TArray<FKalmalaCatalogueRow>& Rows, int32 TextScale, int32 Contrast);
    void PrepareMenuRender(FVector2D ViewportSize);
    FName GetSelectedItemForVerification() const;
    void StepSelectionForVerification(int32 Direction);
    void SelectItemForVerification(FName ItemId) { SelectGridItem(ItemId); }
    void SetViewportSizeForVerification(FVector2D ViewportSize);
    FVector2D GetPanelSizeForVerification() const { return FVector2D(ResponsivePanelWidth, ResponsivePanelHeight); }
#endif

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void ApplyInventoryRows(TArray<FKalmalaCatalogueRow>&& Rows, bool bInventoryAvailable, int32 TextScale, int32 Contrast);
    void RebuildVisibleInventoryRows(int32 TextScale, int32 Contrast);
    void UpdateResponsivePanelSize(FVector2D ViewportSize);
    void RefreshSelectionPresentation(int32 TextScale, int32 Contrast);
    void StepSelection(int32 Direction);
    bool NavigateInventoryMenu(FKey Key);
    UFUNCTION() void RepairSelectedTool();
    UFUNCTION() void UpgradeOpalHelm();
    UFUNCTION() void RepairOpalHelm();
    UFUNCTION() void EatSelectedFood();
    void RefreshFoodActionPresentation(const FKalmalaCatalogueRow* SelectedRow, int32 TextScale, int32 Contrast);
    void RefreshGearPresentation(int32 TextScale, int32 Contrast);
    void RefreshGearDivider(int32 Contrast);
    void SelectGridItem(FName ItemId);
    void HoverGridItem(FName ItemId);
    void UseGearSlot(EKalmalaGearSlot GearSlot);
    UFUNCTION() void UseMainHandSlot();
    UFUNCTION() void PreferSelectedArrows();
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> PreferArrowsButton;
    UFUNCTION() void UseHeadGearSlot();
    UFUNCTION() void UseCapeGearSlot();
    UFUNCTION() void UseChestGearSlot();
    UFUNCTION() void UseShieldGearSlot();
    UFUNCTION() void UseLegsGearSlot();
    UFUNCTION() void UseBootsGearSlot();
    UFUNCTION() void UseNecklaceGearSlot();
    UFUNCTION() void UseRingGearSlot();
    UPROPERTY(Transient) TObjectPtr<UKalmalaInventoryGridWidget> GridView;
    UPROPERTY(Transient) TObjectPtr<USizeBox> GridSizeBox;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolActionStatusText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaItemDetailWidget> ItemDetailView;
    UPROPERTY(Transient) TObjectPtr<USizeBox> PanelSizeBox;
    UPROPERTY(Transient) TObjectPtr<UBorder> PanelBackplate;
    UPROPERTY(Transient) TObjectPtr<UBorder> InventoryPanel;
    UPROPERTY(Transient) TObjectPtr<UImage> GearDividerImage;
    UPROPERTY(Transient) TObjectPtr<UBorder> GearDividerRule;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCraftingWidget> CraftingCompanion;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> RepairToolButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> UpgradeOpalButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> RepairOpalButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> EatFoodButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FoodActionStatusText;
    UPROPERTY(Transient) TArray<TObjectPtr<UKalmalaThemedButton>> GearSlotButtons;
    UPROPERTY(Transient) TArray<TObjectPtr<UKalmalaIconWidget>> GearSlotIcons;
    UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> GearSlotFills;
    UPROPERTY(Transient) TObjectPtr<UKalmalaIconWidget> MainHandIcon;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> MainHandButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MainHandNameText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MainHandStateText;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> GearSlotNameTexts;
    UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> GearSlotStateTexts;
    TArray<FKalmalaCatalogueRow> SourceInventoryRows;
    TArray<FKalmalaCatalogueRow> OwnerInventoryRows;
    int32 SelectedInventoryIndex = INDEX_NONE;
    FName RememberedSelectedItemId = NAME_None;
    FGuid SelectedGridInstanceGuid;
    FName HoveredItemId = NAME_None;
    bool bEmptyCellSelected = false;
    bool bSelectionDetailsRequested = false;
    float ResponsivePanelWidth = 1120.0f;
    float ResponsivePanelHeight = 800.0f;
    float OpeningElapsed = 0.0f;
    bool bOpeningAnimationActive = false;
    FString LastSelectedDetailKey;
    FString LastRepairResultText;
    FName LastRepairResultItemId = NAME_None;
    FName AwaitingRepairItemId = NAME_None;
    FName LastPresentedSelectionId = NAME_None;
    uint32 RepairRequestResultSerial = 0;
    uint32 FoodRequestResultSerial = 0;
    uint32 LastSeenGearRequestResultSerial = 0;
    double GearRequestSentAtSeconds = -1.0;
    FName AwaitingFoodItemId = NAME_None;
    FName LastFoodResultItemId = NAME_None;
    FString LastFoodResultText;
    FString LastGearActionStatus;
    bool bAwaitingRepairResult = false;
    bool bAwaitingFoodResult = false;
    bool bAwaitingAcceptedFoodStatus = false;
    bool bAwaitingGearRequestResult = false;
    bool bPendingGearEquip = false;
    bool bMenuOpen = false;
    bool bAcquiredMoveIgnore = false;
    bool bAcquiredLookIgnore = false;
    bool bPreviousCursorVisibility = false;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};
