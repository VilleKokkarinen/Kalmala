#pragma once

#include "Blueprint/UserWidget.h"
#include "KalmalaPlayerStepWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UKalmalaThemedButton;

DECLARE_MULTICAST_DELEGATE(FKalmalaPlayerStepAction);

/** A local, read-only presentation. It never accepts or emits a save owner identity. */
UCLASS()
class KALMALAUI_API UKalmalaPlayerStepWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static const TCHAR* PlayerLabel() { return TEXT("Current player"); }
    static const TCHAR* IdentityMessage(bool bIdentityAvailable);
    FKalmalaPlayerStepAction& OnBackRequested() { return BackRequested; }
    FKalmalaPlayerStepAction& OnContinueRequested() { return ContinueRequested; }
    void FocusContinue();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void RefreshTheme();
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleContinue();
    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Column;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> IdentityStatus;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Unavailable;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> ContinueButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> BackButton;
    FKalmalaPlayerStepAction BackRequested;
    FKalmalaPlayerStepAction ContinueRequested;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
    bool bLastIdentityAvailable = false;
    bool bIdentityObserved = false;
};
