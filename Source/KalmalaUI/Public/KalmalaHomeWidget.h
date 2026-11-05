#pragma once

#include "Blueprint/UserWidget.h"
#include "KalmalaHomeWidget.generated.h"

class UBorder;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UKalmalaThemedButton;

enum class EKalmalaHomeAction : uint8 { Play, Options, SaveInformation, Credits, Exit };
DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaHomeActionRequested, EKalmalaHomeAction);

/** Local Home presentation; the local-player subsystem owns branch navigation. */
UCLASS()
class KALMALAUI_API UKalmalaHomeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static float ActionColumnWidth(float LogicalViewportWidth);
    static bool TryGetAction(int32 Index, EKalmalaHomeAction& OutAction);
    static const TCHAR* GetActionLabel(EKalmalaHomeAction Action);
    FKalmalaHomeActionRequested& OnActionRequested() { return ActionRequested; }
    // Call after adding the widget to the owning player's screen.
    void FocusPlay();
    void FocusPlayNextTick();
    void FocusOptions();
    void FocusSaveInformation();
    void FocusCredits();
    void FocusExit();
    // A subscriber may report an action whose later branch is not wired yet.
    void ShowUnavailable();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;

private:
    void RefreshTheme();
    void FocusAction(int32 Index);
    void Request(EKalmalaHomeAction Action);
    UFUNCTION() void HandlePlay();
    UFUNCTION() void HandleOptions();
    UFUNCTION() void HandleSaveInformation();
    UFUNCTION() void HandleCredits();
    UFUNCTION() void HandleExit();

    UPROPERTY(Transient) TObjectPtr<UBorder> Backdrop;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> ActionsScroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> ActionColumn;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Footer;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> UnavailableMessage;
    UPROPERTY(Transient) TArray<TObjectPtr<UKalmalaThemedButton>> Buttons;
    FKalmalaHomeActionRequested ActionRequested;
    int32 LastTextScale = -1;
    int32 LastContrast = -2;
    bool bLastReducedMotion = false;
};
