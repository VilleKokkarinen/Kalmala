#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaInventoryMenuWidget.generated.h"

class UTextBlock;
class USizeBox;
class UKalmalaItemDetailWidget;
class UScrollBox;
class UKalmalaThemedButton;
class UKalmalaInventoryGridWidget;
class UKalmalaCraftingWidget;
class UBorder;
class UHorizontalBox;

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
    UFUNCTION() void EatSelectedFood();
    UFUNCTION() void MenuScrolled(float Offset);
    void RefreshFoodActionPresentation(const FKalmalaCatalogueRow* SelectedRow, int32 TextScale, int32 Contrast);
    void SelectGridItem(FName ItemId);
    void HoverGridItem(FName ItemId);
    UPROPERTY(Transient) TObjectPtr<UKalmalaInventoryGridWidget> GridView;
    UPROPERTY(Transient) TObjectPtr<USizeBox> GridSizeBox;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ArmorWeightText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ToolActionStatusText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaItemDetailWidget> ItemDetailView;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> MenuContentScrollBox;
    UPROPERTY(Transient) TObjectPtr<USizeBox> PanelSizeBox;
    UPROPERTY(Transient) TObjectPtr<UBorder> PanelBackplate;
    UPROPERTY(Transient) TObjectPtr<UBorder> InventoryPanel;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCraftingWidget> CraftingCompanion;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> RepairToolButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> EatFoodButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FoodActionStatusText;
    TArray<FKalmalaCatalogueRow> SourceInventoryRows;
    TArray<FKalmalaCatalogueRow> OwnerInventoryRows;
    int32 SelectedInventoryIndex = INDEX_NONE;
    FName RememberedSelectedItemId = NAME_None;
    FName HoveredItemId = NAME_None;
    bool bEmptyCellSelected = false;
    bool bSelectionDetailsRequested = false;
    float RememberedMenuScrollOffset = 0.0f;
    float ResponsivePanelWidth = 1120.0f;
    float ResponsivePanelHeight = 800.0f;
    float OpeningElapsed = 0.0f;
    bool bOpeningAnimationActive = false;
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
