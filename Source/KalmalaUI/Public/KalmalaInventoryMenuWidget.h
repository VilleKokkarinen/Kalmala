#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaInventoryMenuWidget.generated.h"

/** Minimal owner-local inventory menu surface; inventory data is added by later M12 increments. */
UCLASS()
class KALMALAUI_API UKalmalaInventoryMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void Open();
    void Close();
    bool IsMenuOpen() const { return bMenuOpen; }
    bool HasTextEntryFocus() const;

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    bool bMenuOpen = false;
    bool bAcquiredMoveIgnore = false;
    bool bAcquiredLookIgnore = false;
    bool bPreviousCursorVisibility = false;
};
