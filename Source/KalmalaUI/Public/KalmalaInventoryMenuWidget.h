#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "Styling/SlateTypes.h"
#include "KalmalaInventoryMenuWidget.generated.h"

class UTextBlock;
class UEditableTextBox;
class USizeBox;
class UKalmalaItemDetailWidget;
class UScrollBox;
class UKalmalaThemedButton;

/** Owner-local pack, carried-tool and supported-food actions through existing server paths. */
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
    FName GetSelectedItemForVerification() const;
    void StepSelectionForVerification(int32 Direction);
    void SetInventoryBrowseForVerification(const FString& Search, int32 Category, int32 Sort);
    bool NavigateForVerification(FKey Key);
    TArray<FName> GetVisibleItemIdsForVerification() const;
    FString GetInventorySearchForVerification() const { return InventorySearchQuery; }
    int32 GetInventoryCategoryForVerification() const { return InventoryCategoryIndex; }
    int32 GetInventorySortForVerification() const { return InventorySortIndex; }
    bool HasBrowseFocusTargetsForVerification() const;
    void SetInventoryScrollOffsetForVerification(float Offset);
    float GetInventoryScrollOffsetForVerification() const { return RememberedInventoryScrollOffset; }
    void SetViewportSizeForVerification(FVector2D ViewportSize);
    FVector2D GetPanelSizeForVerification() const { return FVector2D(ResponsivePanelWidth, ResponsivePanelHeight); }
#endif

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void ApplyInventoryRows(TArray<FKalmalaCatalogueRow>&& Rows, bool bInventoryAvailable, int32 TextScale, int32 Contrast);
    void RebuildVisibleInventoryRows(int32 TextScale, int32 Contrast);
    void UpdateBrowseControls(int32 TextScale, int32 Contrast);
    void UpdateResponsivePanelSize(FVector2D ViewportSize);
    void RefreshSelectionPresentation(int32 TextScale, int32 Contrast);
    void StepSelection(int32 Direction);
    void SetInventorySearch(const FString& Search);
    void SetInventoryCategory(int32 Category);
    void SetInventorySort(int32 Sort);
    bool NavigateInventoryMenu(FKey Key);
    UFUNCTION() void RepairSelectedTool();
    UFUNCTION() void EatSelectedFood();
    UFUNCTION() void SelectPreviousItem();
    UFUNCTION() void SelectNextItem();
    UFUNCTION() void InventorySearchChanged(const FText& Search);
    UFUNCTION() void ClearInventorySearch();
    UFUNCTION() void CycleInventoryCategory();
    UFUNCTION() void CycleInventorySort();
    UFUNCTION() void InventoryScrolled(float Offset);
    UFUNCTION() void MenuScrolled(float Offset);
    void RefreshFoodActionPresentation(const FKalmalaCatalogueRow* SelectedRow, int32 TextScale, int32 Contrast);
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PackStateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BrowseStateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BrowseSearchLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectedItemText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolActionStatusText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCatalogueRowsWidget> InventoryRowsView;
    UPROPERTY(Transient) TObjectPtr<UKalmalaItemDetailWidget> ItemDetailView;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> InventorySearchBox;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> InventoryScrollBox;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> MenuContentScrollBox;
    UPROPERTY(Transient) TObjectPtr<USizeBox> PanelSizeBox;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> PreviousItemButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> NextItemButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> RepairToolButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> EatFoodButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> CategoryButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> SortButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> ClearSearchButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CategoryButtonLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SortButtonLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ClearSearchButtonLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FoodActionStatusText;
    UPROPERTY(Transient) FEditableTextBoxStyle InventorySearchStyle;
    TArray<FKalmalaCatalogueRow> SourceInventoryRows;
    TArray<FKalmalaCatalogueRow> OwnerInventoryRows;
    int32 SelectedInventoryIndex = INDEX_NONE;
    FString InventorySearchQuery;
    FName RememberedSelectedItemId = NAME_None;
    int32 InventoryCategoryIndex = 0;
    int32 InventorySortIndex = 0;
    float RememberedInventoryScrollOffset = 0.0f;
    float RememberedMenuScrollOffset = 0.0f;
    float ResponsivePanelWidth = 640.0f;
    float ResponsivePanelHeight = 560.0f;
    bool bApplyingInventoryRows = false;
    FString LastSelectedDetailKey;
    FString LastRepairResultText;
    FName LastRepairResultToolId = NAME_None;
    FName AwaitingRepairToolId = NAME_None;
    FName LastPresentedSelectionId = NAME_None;
    uint32 RepairRequestResultSerial = 0;
    uint32 FoodRequestResultSerial = 0;
    FName AwaitingFoodItemId = NAME_None;
    FName LastFoodResultItemId = NAME_None;
    FString LastFoodResultText;
    bool bAwaitingRepairResult = false;
    bool bAwaitingFoodResult = false;
    bool bAwaitingAcceptedFoodStatus = false;
    bool bMenuOpen = false;
    bool bAcquiredMoveIgnore = false;
    bool bAcquiredLookIgnore = false;
    bool bPreviousCursorVisibility = false;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};
