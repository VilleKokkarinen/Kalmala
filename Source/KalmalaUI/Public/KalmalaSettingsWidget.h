#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaThemedButton.h"
#include "KalmalaSettingsWidget.generated.h"

class UTextBlock;
class UCanvasPanelSlot;
class UCheckBox;
class UHorizontalBox;
class USlider;

enum class EKalmalaAudioCategory : uint8
{
    Ambient,
    Music,
    InteractionCombat
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FKalmalaControlBindingClicked, FName, ControlName, bool, bGamepad,
    bool, bAxisPairSide, bool, bPositiveAxisSide);

UCLASS()
class KALMALAUI_API UKalmalaControlButton : public UKalmalaThemedButton
{
    GENERATED_BODY()

public:
    UKalmalaControlButton(const FObjectInitializer& ObjectInitializer);

    void Configure(FName InControlName, bool bInGamepad, bool bInAxisPairSide = false,
        bool bInPositiveAxisSide = true);
    void SetDisplayText(const FText& Text);
    FName GetControlName() const { return ControlName; }
    bool IsGamepadBinding() const { return bGamepad; }
    bool IsAxisPairSideBinding() const { return bAxisPairSide; }
    bool IsPositiveAxisSideBinding() const { return bPositiveAxisSide; }

    UPROPERTY(BlueprintAssignable)
    FKalmalaControlBindingClicked OnControlBindingClicked;

private:
    UFUNCTION()
    void HandleButtonClicked();

    FName ControlName;
    bool bGamepad = false;
    bool bAxisPairSide = false;
    bool bPositiveAxisSide = true;
};

/**
 * Local-only pause and options presentation. Graphics settings are applied
 * through UGameUserSettings; this widget never changes replicated gameplay state.
 */
UCLASS()
class KALMALAUI_API UKalmalaSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void Open(APlayerController* InOwningPlayer);
    void Close();
    bool IsMenuOpen() const { return bMenuOpen; }
#if !UE_BUILD_SHIPPING
    /** Opens a requested local tab for the development-only settings acceptance probe. */
    void OpenForVerification(APlayerController* InOwningPlayer, int32 TabIndex);
    void SetVerificationTab(int32 TabIndex);
    void PrepareMenuRender(int32 TabIndex);
    int32 GetRememberedOptionsTabForVerification() const { return LastOptionsTabIndex; }
    FString GetPanelImagePathForVerification() const;
    bool HasFocusableContentForVerification() const;
    bool HasFocusedContentForVerification() const;
    bool IsHomeMenuSurfaceTransparentForVerification() const;
    bool HasOnlyHomeMenuOptionsForVerification() const;
    bool HasVisibleFocusedHomeMenuOptionForVerification() const;
    bool HasFocusableControlsForVerification() const;
    bool IsOptionsOpeningAnimationActiveForVerification() const { return bOptionsOpeningAnimationActive; }
    float GetOptionsPanelPositionYForVerification() const;
    FVector2D GetOptionsPanelSizeForVerification() const;
    FVector2D GetLayoutViewportSizeForVerification() const { return LastLayoutViewportSize; }
    bool DoesOptionsPanelFitViewportForVerification() const;
    bool IsOptionsPanelCenterAnchoredForVerification() const;
#endif

    static int32 ClampViewDistanceQuality(int32 Quality);
    static float ClampMasterVolume(float Volume);
    static float GetStoredMasterVolume();
    static bool IsAudioMuted();
    static void SetMasterVolume(float Volume, bool bFlush = true);
    static void ToggleAudioMute();
    static void ApplySavedMasterVolume();
    static float ClampAudioCategoryVolume(float Volume);
    static float GetAudioCategoryVolume(EKalmalaAudioCategory Category);
    static void SetAudioCategoryVolume(EKalmalaAudioCategory Category, float Volume, bool bFlush = true);
    static int32 ClampTextScale(int32 Percent);
    static int32 GetTextScalePercent();
    static void SetTextScalePercent(int32 Percent);
    static int32 ClampInterfaceScalePercent(int32 Percent);
    static int32 GetInterfaceScalePercent();
    static void SetInterfaceScalePercent(int32 Percent);
    static void ApplySavedInterfaceScale();
    static float GetThemeDefaultApplicationScale();
    static float GetAppliedApplicationScale();
    static bool IsReducedMotionEnabled();
    static void SetReducedMotionEnabled(bool bEnabled);
    static int32 ClampContrastMode(int32 Mode);
    static int32 GetContrastMode();
    static void SetContrastMode(int32 Mode);
    static int32 ClampFeedbackMode(int32 Mode);
    static int32 GetFeedbackMode();
    static void SetFeedbackMode(int32 Mode);
    static int32 GetRemappableControlCount();
    static FName GetRemappableControlName(int32 Index);
    static FText GetRemappableControlLabel(FName ControlName);
    static FText GetLocalInputBindingLabel(FName ControlName, bool bGamepad,
        bool bAxisPairSide = false, bool bPositiveAxisSide = true);
    static bool SetLocalInputBinding(FName ControlName, bool bGamepad, const FKey& Key,
        bool bAxisPairSide = false, bool bPositiveAxisSide = true);
    static void CycleLocalInputBinding(class APlayerController* Controller, FName ControlName, bool bGamepad,
        bool bAxisPairSide = false, bool bPositiveAxisSide = true);
    static void ApplySavedInputBindings(class APlayerController* Controller);
    static void RestoreDefaultInputBindings(class APlayerController* Controller);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;

private:
    UFUNCTION()
    void ShowMainMenu();
    void ShowOptionsMenu();
    void ShowGraphicsTab();
    void ShowKeyboardAndMouseTab();
    void ShowControllerTab();
    void ShowInputTab(bool bGamepad);
    void ShowAudioTab();
    void ShowAccessibilityTab();
    void ShowPlaceholderTab(const FText& Title, const FText& Description);
    void ApplyVideoSettings();
    void UpdateVideoLabels();
    void UpdateAudioLabels();
    void UpdateSettingsLabels();
    void CommitAccessibilitySliderValues();
    void UpdateControlsLabels();
    void MarkAudioSettingsDirty();
    void FlushAudioSettingsIfDirty();
    void ApplyModalPalette(const FString* ImageOverride = nullptr);
    void ApplyHomeMenuPalette();
    void StartOptionsOpeningAnimation();
    void ResetOptionsOpeningAnimation();
    UButton* AddButton(class UVerticalBox* Parent, const FText& Label, FName Name);
    void SelectOptionsTab(int32 SelectedIndex);
    UTextBlock* AddLabel(class UVerticalBox* Parent, const FText& Label, float FontSize = 20.0f);

    UFUNCTION()
    void HandleOptionsClicked();
    UFUNCTION()
    void HandleQuitClicked();
    UFUNCTION()
    void HandleGraphicsClicked();
    UFUNCTION()
    void HandleKeyboardAndMouseClicked();
    UFUNCTION()
    void HandleControllerClicked();
    UFUNCTION()
    void HandleAudioClicked();
    UFUNCTION()
    void HandleAccessibilityClicked();
    UFUNCTION()
    void HandleResolutionSelectionChanged(FString SelectedOption, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleVSyncChanged(bool bIsChecked);
    UFUNCTION()
    void HandleWindowModeSelectionChanged(FString SelectedOption, ESelectInfo::Type SelectionType);
    UFUNCTION()
    void HandleCustomResolutionChanged(bool bIsChecked);
    UFUNCTION()
    void HandleCustomResolutionApplyClicked();
    UFUNCTION()
    void HandleViewDistanceClicked();
    UFUNCTION()
    void HandleMasterVolumeChanged(float Value);
    UFUNCTION()
    void HandleAudioMuteClicked();
    UFUNCTION()
    void HandleAmbientVolumeChanged(float Value);
    UFUNCTION()
    void HandleMusicVolumeChanged(float Value);
    UFUNCTION()
    void HandleInteractionCombatVolumeChanged(float Value);
    UFUNCTION()
    void HandleControlBindingClicked(FName ControlName, bool bGamepad, bool bAxisPairSide, bool bPositiveAxisSide);
    UFUNCTION()
    void HandleRestoreControlsClicked();
    UFUNCTION()
    void HandleTextScaleChanged(float Value);
    UFUNCTION()
    void HandleInterfaceScaleChanged(float Value);
    UFUNCTION()
    void HandleContrastChanged(float Value);
    UFUNCTION()
    void HandleReducedMotionChanged(bool bIsChecked);
    UFUNCTION()
    void HandleFeedbackSelectionChanged(FString SelectedOption, ESelectInfo::Type SelectionType);
    UFUNCTION()
    UWidget* GenerateDropdownOptionWidget(FString Option);
    UFUNCTION()
    void HandleAccessibilitySliderCaptureBegan();
    UFUNCTION()
    void HandleAccessibilitySliderCaptureEnded();

    UPROPERTY(Transient)
    TObjectPtr<class UVerticalBox> ContentBox;
    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> ResolutionDropdown;
    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> VSyncCheckBox;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> VSyncCheckMarkLabel;
    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> CustomResolutionCheckBox;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CustomResolutionCheckMarkLabel;
    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> CustomResolutionFieldsRow;
    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> CustomResolutionWidthInput;
    UPROPERTY(Transient)
    TObjectPtr<UEditableTextBox> CustomResolutionHeightInput;
    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> WindowModeDropdown;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ViewDistanceLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MasterVolumeValueLabel;
    UPROPERTY(Transient)
    TObjectPtr<USlider> MasterVolumeSlider;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AudioMuteLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AmbientVolumeValueLabel;
    UPROPERTY(Transient)
    TObjectPtr<USlider> AmbientVolumeSlider;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MusicVolumeValueLabel;
    UPROPERTY(Transient)
    TObjectPtr<USlider> MusicVolumeSlider;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> InteractionCombatVolumeValueLabel;
    UPROPERTY(Transient)
    TObjectPtr<USlider> InteractionCombatVolumeSlider;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TextScaleValueLabel;
    UPROPERTY(Transient)
    TObjectPtr<USlider> TextScaleSlider;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> InterfaceScaleValueLabel;
    UPROPERTY(Transient)
    TObjectPtr<USlider> InterfaceScaleSlider;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ContrastValueLabel;
    UPROPERTY(Transient)
    TObjectPtr<USlider> ContrastSlider;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ReducedMotionCheckMarkLabel;
    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> ReducedMotionCheckBox;
    UPROPERTY(Transient)
    TObjectPtr<UComboBoxString> FeedbackDropdown;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UKalmalaThemedButton>> OptionsTabButtons;

    UPROPERTY(Transient)
    TObjectPtr<class UBorder> BackdropBorder;
    UPROPERTY(Transient)
    TObjectPtr<class UBorder> PanelBorder;
    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanelSlot> PanelCanvasSlot;
    FVector2D LastLayoutViewportSize = FVector2D::ZeroVector;
    bool bAudioSettingsDirty = false;
    bool bUpdatingAudioControls = false;
    bool bUpdatingGraphicsControls = false;
    bool bCustomResolutionEnabled = false;
    bool bUpdatingAccessibilityControls = false;
    bool bAccessibilitySliderCaptured = false;
    double AudioSettingsLastChangedTime = 0.0;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UKalmalaControlButton>> ControlButtons;

    TArray<FIntPoint> ResolutionChoices;
    // Retained by the owner-local widget across ordinary close/reopen only.
    int32 LastOptionsTabIndex = 0;
    bool bMenuOpen = false;
    bool bOptionsOpeningAnimationActive = false;
    float OptionsOpeningElapsed = 0.0f;
};
