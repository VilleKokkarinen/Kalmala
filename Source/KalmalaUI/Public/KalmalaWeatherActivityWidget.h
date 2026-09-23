#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaWeatherActivityWidget.generated.h"

class UBorder;
class UTextBlock;
enum class EKalmalaWeatherActivityLevel : uint8;

/** Local-only, colour-independent label for the replicated server weather tier. */
UCLASS()
class KALMALAUI_API UKalmalaWeatherActivityWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void ConfigureViewportPlacement();
    void SetWeatherActivity(EKalmalaWeatherActivityLevel ActivityLevel, int32 TextScalePercent, int32 ContrastMode);

    static FString BuildActivityLabel(EKalmalaWeatherActivityLevel ActivityLevel);

protected:
    virtual void NativeOnInitialized() override;

private:
    void ApplyAccessibilityPresentation(int32 TextScalePercent, int32 ContrastMode);

    UPROPERTY(Transient)
    TObjectPtr<UBorder> Background;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> HeadingText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ActivityText;

    uint8 LastActivityLevel = 0xff;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};
