#pragma once

#include "Blueprint/UserWidget.h"
#include "KalmalaSaveInformationWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UKalmalaThemedButton;

DECLARE_MULTICAST_DELEGATE(FKalmalaSaveInformationBack);

/** Read-only summary of the save records and ownership checks supported today. */
UCLASS()
class KALMALAUI_API UKalmalaSaveInformationWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FKalmalaSaveInformationBack& OnBackRequested() { return BackRequested; }
    void SetInformation(const FText& Information);
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
    UPROPERTY(Transient) TObjectPtr<UTextBlock> InformationText;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> BackButton;
    FKalmalaSaveInformationBack BackRequested;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
};
