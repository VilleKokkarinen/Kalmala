#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "KalmalaJoinAddressWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UKalmalaThemedButton;

DECLARE_MULTICAST_DELEGATE(FKalmalaJoinAddressBack);
DECLARE_MULTICAST_DELEGATE(FKalmalaJoinAddressCancel);
DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaJoinAddressRequested, FString);

/** Local direct-address entry; travel is dispatched only after structured validation. */
UCLASS()
class KALMALAUI_API UKalmalaJoinAddressWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FKalmalaJoinAddressBack& OnBackRequested() { return BackRequested; }
    FKalmalaJoinAddressCancel& OnCancelRequested() { return CancelRequested; }
    FKalmalaJoinAddressRequested& OnJoinRequested() { return JoinRequested; }
    void FocusAddress();
    void FocusCancel();
    void SetConnecting(bool bConnecting);
    void SetJoinEnabled(bool bEnabled);
    void ShowMessage(const FText& Text);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void RefreshTheme();
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleCancel();
    UFUNCTION() void HandleJoin();
    UFUNCTION() void HandleAddressCommitted(const FText& Text, ETextCommit::Type CommitMethod);

    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Column;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Instructions;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> AddressInput;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> JoinButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> BackButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> CancelButton;
    FKalmalaJoinAddressBack BackRequested;
    FKalmalaJoinAddressCancel CancelRequested;
    FKalmalaJoinAddressRequested JoinRequested;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
    bool bIsConnecting = false;
};
