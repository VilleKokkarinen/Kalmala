#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaInventoryMenuWidget.generated.h"

class UTextBlock;
class UKalmalaItemDetailWidget;
class UKalmalaThemedButton;

/** Owner-local read-only pack grid with selected-item details; tools and actions remain later M12 work. */
UCLASS()
class KALMALAUI_API UKalmalaInventoryMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void Open();
    void Close();
    void RefreshOwnerPack();
    bool IsMenuOpen() const { return bMenuOpen; }
    bool HasTextEntryFocus() const;
#if !UE_BUILD_SHIPPING
    void SetPackRowsForVerification(const TArray<FKalmalaCatalogueRow>& Rows, int32 TextScale, int32 Contrast);
    FName GetSelectedItemForVerification() const;
    void StepSelectionForVerification(int32 Direction);
#endif

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void ApplyPackRows(TArray<FKalmalaCatalogueRow>&& Rows, bool bInventoryAvailable, int32 TextScale, int32 Contrast);
    void RefreshSelectionPresentation(int32 TextScale, int32 Contrast);
    void StepSelection(int32 Direction);
    UFUNCTION() void SelectPreviousItem();
    UFUNCTION() void SelectNextItem();
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PackStateText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectedItemText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCatalogueRowsWidget> PackRowsView;
    UPROPERTY(Transient) TObjectPtr<UKalmalaItemDetailWidget> ItemDetailView;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> PreviousItemButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> NextItemButton;
    TArray<FKalmalaCatalogueRow> OwnerPackRows;
    int32 SelectedPackIndex = INDEX_NONE;
    FString LastSelectedDetailKey;
    bool bMenuOpen = false;
    bool bAcquiredMoveIgnore = false;
    bool bAcquiredLookIgnore = false;
    bool bPreviousCursorVisibility = false;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};
