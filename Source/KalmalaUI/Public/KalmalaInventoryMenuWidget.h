#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaInventoryMenuWidget.generated.h"

class UTextBlock;

/** Owner-local inventory menu with a read-only pack grid; details and actions remain later M12 work. */
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

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PackStateText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCatalogueRowsWidget> PackRowsView;
    bool bMenuOpen = false;
    bool bAcquiredMoveIgnore = false;
    bool bAcquiredLookIgnore = false;
    bool bPreviousCursorVisibility = false;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};
