#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaSettingsWidget.generated.h"

class UTextBlock;

enum class EKalmalaAudioCategory : uint8
{
    Ambient,
    Music,
    InteractionCombat
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKalmalaControlBindingClicked, FName, ControlName, bool, bGamepad);

UCLASS()
class KALMALAUI_API UKalmalaControlButton : public UButton
{
    GENERATED_BODY()

public:
    UKalmalaControlButton(const FObjectInitializer& ObjectInitializer);

    void Configure(FName InControlName, bool bInGamepad);
    void SetDisplayText(const FText& Text);
    FName GetControlName() const { return ControlName; }
    bool IsGamepadBinding() const { return bGamepad; }

    UPROPERTY(BlueprintAssignable)
    FKalmalaControlBindingClicked OnControlBindingClicked;

private:
    UFUNCTION()
    void HandleButtonClicked();

    FName ControlName;
    bool bGamepad = false;
};

/**
 * Local-only pause and options presentation. Video settings are applied through
 * UGameUserSettings; this widget never changes replicated gameplay state.
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
    bool HasFocusableContentForVerification() const;
    bool HasFocusableControlsForVerification() const;
#endif

    static int32 ClampViewDistanceQuality(int32 Quality);
    static float ClampMasterVolume(float Volume);
    static float GetStoredMasterVolume();
    static bool IsAudioMuted();
    static void SetMasterVolume(float Volume);
    static void ToggleAudioMute();
    static void ApplySavedMasterVolume();
    static float ClampAudioCategoryVolume(float Volume);
    static float GetAudioCategoryVolume(EKalmalaAudioCategory Category);
    static void SetAudioCategoryVolume(EKalmalaAudioCategory Category, float Volume);
    static int32 ClampTextScale(int32 Percent);
    static int32 GetTextScalePercent();
    static void SetTextScalePercent(int32 Percent);
    static int32 ClampContrastMode(int32 Mode);
    static int32 GetContrastMode();
    static void SetContrastMode(int32 Mode);
    static int32 ClampFeedbackMode(int32 Mode);
    static int32 GetFeedbackMode();
    static void SetFeedbackMode(int32 Mode);
    static int32 GetRemappableControlCount();
    static FName GetRemappableControlName(int32 Index);
    static FText GetRemappableControlLabel(FName ControlName);
    static FText GetLocalInputBindingLabel(FName ControlName, bool bGamepad);
    static bool SetLocalInputBinding(FName ControlName, bool bGamepad, const FKey& Key);
    static void CycleLocalInputBinding(class APlayerController* Controller, FName ControlName, bool bGamepad);
    static void ApplySavedInputBindings(class APlayerController* Controller);
    static void RestoreDefaultInputBindings(class APlayerController* Controller);

protected:
    virtual void NativeConstruct() override;

private:
    void ShowMainMenu();
    void ShowOptionsMenu();
    void ShowVideoTab();
    void ShowAudioTab();
    void ShowControlsTab();
    void ShowSettingsTab();
    void ShowPlaceholderTab(const FText& Title, const FText& Description);
    void ApplyVideoSettings();
    void UpdateVideoLabels();
    void UpdateAudioLabels();
    void UpdateSettingsLabels();
    void UpdateControlsLabels();
    void CycleAudioCategory(EKalmalaAudioCategory Category);
    void ApplyModalPalette();
    UButton* AddButton(class UVerticalBox* Parent, const FText& Label, FName Name);
    UTextBlock* AddLabel(class UVerticalBox* Parent, const FText& Label, float FontSize = 20.0f);

    UFUNCTION()
    void HandleOptionsClicked();
    UFUNCTION()
    void HandleQuitClicked();
    UFUNCTION()
    void HandleVideoClicked();
    UFUNCTION()
    void HandleAudioClicked();
    UFUNCTION()
    void HandleControlsClicked();
    UFUNCTION()
    void HandleSettingsClicked();
    UFUNCTION()
    void HandleResolutionClicked();
    UFUNCTION()
    void HandleVSyncClicked();
    UFUNCTION()
    void HandleWindowModeClicked();
    UFUNCTION()
    void HandleViewDistanceClicked();
    UFUNCTION()
    void HandleMasterVolumeClicked();
    UFUNCTION()
    void HandleAudioMuteClicked();
    UFUNCTION()
    void HandleAmbientVolumeClicked();
    UFUNCTION()
    void HandleMusicVolumeClicked();
    UFUNCTION()
    void HandleInteractionCombatVolumeClicked();
    UFUNCTION()
    void HandleControlBindingClicked(FName ControlName, bool bGamepad);
    UFUNCTION()
    void HandleRestoreControlsClicked();
    UFUNCTION()
    void HandleTextScaleClicked();
    UFUNCTION()
    void HandleContrastClicked();
    UFUNCTION()
    void HandleFeedbackClicked();

    UPROPERTY(Transient)
    TObjectPtr<class UVerticalBox> ContentBox;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResolutionLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> VSyncLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> WindowModeLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ViewDistanceLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MasterVolumeLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AudioMuteLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AmbientVolumeLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MusicVolumeLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> InteractionCombatVolumeLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> TextScaleLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ContrastLabel;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> FeedbackLabel;

    UPROPERTY(Transient)
    TObjectPtr<class UBorder> BackdropBorder;
    UPROPERTY(Transient)
    TObjectPtr<class UBorder> PanelBorder;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UKalmalaControlButton>> ControlButtons;

    TArray<FIntPoint> ResolutionChoices;
    int32 ResolutionChoiceIndex = 0;
    bool bMenuOpen = false;
};
