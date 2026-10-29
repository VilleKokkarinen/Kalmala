#pragma once

#include "Blueprint/UserWidget.h"
#include "KalmalaWorldDescriptor.h"
#include "KalmalaWorldListWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UKalmalaThemedButton;
DECLARE_MULTICAST_DELEGATE(FKalmalaWorldListBack);

/** Read-only world-list entry point. No save enumeration is fabricated. */
UCLASS()
class KALMALAUI_API UKalmalaWorldListWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static const TCHAR* EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability Availability);
    FKalmalaWorldListBack& OnBackRequested() { return BackRequested; }
    void FocusBack();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
    void RefreshTheme();
    UFUNCTION() void HandleBack();
    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> Column;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CreationStatus;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> NewWorldButton;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> BackButton;
    FKalmalaWorldListBack BackRequested;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
};
