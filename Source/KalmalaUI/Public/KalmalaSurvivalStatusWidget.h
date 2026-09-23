#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaCharacter.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaWeatherState.h"
#include "KalmalaSurvivalStatusWidget.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;

/** Snapshot of already replicated owner-visible state; it carries no gameplay authority. */
struct KALMALAUI_API FKalmalaSurvivalStatusSnapshot
{
    bool bHasCharacter = false;
    TArray<FKalmalaPlayerStatusEntry> Statuses;
    FKalmalaExposureState Exposure;
    bool bHasWeatherState = false;
    FKalmalaWeatherState Weather;
    float ServerTimeSeconds = 0.0f;
    EKalmalaSupportEffect ActiveSupportEffect = EKalmalaSupportEffect::None;
    float ActiveSupportEffectExpiry = 0.0f;
    float HearthShieldStrength = 0.0f;
    float BearsVigorStrengthMultiplier = 1.0f;
};

/** Persistent, read-only local presentation of replicated survival status. */
UCLASS()
class KALMALAUI_API UKalmalaSurvivalStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void ConfigureViewportPlacement();
    void SetSnapshot(const FKalmalaSurvivalStatusSnapshot& Snapshot, int32 TextScalePercent, int32 ContrastMode);

    static FString BuildStatusText(const FKalmalaSurvivalStatusSnapshot& Snapshot);

protected:
    virtual void NativeOnInitialized() override;

private:
    void ApplyAccessibilityPresentation(int32 TextScalePercent, int32 ContrastMode);

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Background;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> ContentWidth;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeadingText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;

    FString LastStatusText;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};
