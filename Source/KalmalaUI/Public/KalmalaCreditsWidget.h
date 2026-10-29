#pragma once

#include "Blueprint/UserWidget.h"
#include "KalmalaCreditsWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UKalmalaThemedButton;

DECLARE_MULTICAST_DELEGATE(FKalmalaCreditsBack);

/** Scrollable project attribution page with local Back navigation. */
UCLASS()
class KALMALAUI_API UKalmalaCreditsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    FKalmalaCreditsBack& OnBackRequested() { return BackRequested; }
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
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ProjectName;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> AttributionStatus;
    UPROPERTY(Transient) TObjectPtr<UKalmalaThemedButton> BackButton;
    FKalmalaCreditsBack BackRequested;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
};
