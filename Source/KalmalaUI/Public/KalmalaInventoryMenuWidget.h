#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaInventoryMenuWidget.generated.h"

class UTextBlock;
class UKalmalaItemDetailWidget;
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
#endif

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void ApplyInventoryRows(TArray<FKalmalaCatalogueRow>&& Rows, bool bInventoryAvailable, int32 TextScale, int32 Contrast);
    void RefreshSelectionPresentation(int32 TextScale, int32 Contrast);
    void StepSelection(int32 Direction);
    UFUNCTION() void RepairSelectedTool();
    UFUNCTION() void EatSelectedFood();
    UFUNCTION() void SelectPreviousItem();
    UFUNCTION() void SelectNextItem();
    void RefreshFoodActionPresentation(const FKalmalaCatalogueRow* SelectedRow, int32 TextScale, int32 Contrast);
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PackStateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectedItemText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolActionStatusText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCatalogueRowsWidget> InventoryRowsView;
    UPROPERTY(Transient) TObjectPtr<UKalmalaItemDetailWidget> ItemDetailView;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> PreviousItemButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> NextItemButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> RepairToolButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> EatFoodButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FoodActionStatusText;
    TArray<FKalmalaCatalogueRow> OwnerInventoryRows;
    int32 SelectedInventoryIndex = INDEX_NONE;
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
