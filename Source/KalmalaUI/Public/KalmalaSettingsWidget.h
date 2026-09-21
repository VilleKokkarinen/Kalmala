#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaSettingsWidget.generated.h"

class UButton;
class UTextBlock;

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

    static int32 ClampViewDistanceQuality(int32 Quality);
    static float ClampMasterVolume(float Volume);
    static float GetStoredMasterVolume();
    static bool IsAudioMuted();
    static void SetMasterVolume(float Volume);
    static void ToggleAudioMute();
    static void ApplySavedMasterVolume();

protected:
    virtual void NativeConstruct() override;

private:
    void ShowMainMenu();
    void ShowOptionsMenu();
    void ShowVideoTab();
    void ShowAudioTab();
    void ShowPlaceholderTab(const FText& Title, const FText& Description);
    void ApplyVideoSettings();
    void UpdateVideoLabels();
    void UpdateAudioLabels();
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

    TArray<FIntPoint> ResolutionChoices;
    int32 ResolutionChoiceIndex = 0;
    bool bMenuOpen = false;
};
