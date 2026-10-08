#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaSelectedResultWidget.generated.h"

class UBorder;
class USizeBox;
class UTextBlock;

/** Owner-local, read-only preview of the currently selected recipe/build output. */
UCLASS()
class KALMALAUI_API UKalmalaSelectedResultWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    static int32 GetPreviewIconExtent();
    static bool FindCanonicalIcon(FName OutputId, EKalmalaIcon& OutIcon, int32& OutVariant);

    void SetResult(FName OutputId, const FString& Name, const FString& Description,
        const FString& Requirements, int32 TextScalePercent, int32 ContrastMode);
    void ClearResult();

    FName GetOutputId() const { return OutputId; }
    bool HasCanonicalIcon() const { return bHasCanonicalIcon; }
    bool IsShowingResult() const;
    FString GetDescriptionText() const;
    FString GetRequirementsText() const;
    FString GetPresentationText() const;
    FVector2D GetConfiguredPreviewIconSize() const;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeOnInitialized() override;

private:
    void BuildPanel();

    UPROPERTY(Transient) TObjectPtr<UBorder> Panel;
    UPROPERTY(Transient) TObjectPtr<USizeBox> IconSize;
    UPROPERTY(Transient) TObjectPtr<UKalmalaIconWidget> Icon;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Description;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Requirements;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> PreviewStatus;

    FName OutputId = NAME_None;
    FString CurrentName;
    FString CurrentDescription;
    FString CurrentRequirements;
    bool bHasCanonicalIcon = false;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
};
