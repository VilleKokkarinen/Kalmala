#pragma once

#include "Blueprint/UserWidget.h"
#include "KalmalaExitConfirmationWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UKalmalaThemedButton;

DECLARE_MULTICAST_DELEGATE(FKalmalaExitConfirmationCancel);
DECLARE_MULTICAST_DELEGATE(FKalmalaExitConfirmationAccepted);

/** Local confirmation that defaults focus to the safe Cancel action. */
UCLASS()
class KALMALAUI_API UKalmalaExitConfirmationWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FKalmalaExitConfirmationCancel& OnCancelRequested() { return CancelRequested; }
    FKalmalaExitConfirmationAccepted& OnExitAccepted() { return ExitAccepted; }
    void FocusCancel();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void RefreshTheme();
    UFUNCTION() void HandleCancel();
    UFUNCTION() void HandleExit();
    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Column;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> CancelButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> ExitButton;
    FKalmalaExitConfirmationCancel CancelRequested;
    FKalmalaExitConfirmationAccepted ExitAccepted;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
};
