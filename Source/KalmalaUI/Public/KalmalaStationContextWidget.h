#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "KalmalaStationContextWidget.generated.h"

class AKalmalaCharacter;
class AKalmalaConstructionActor;
class APawn;
class UBorder;
class UButton;
class USizeBox;
class UTextBlock;
class UKalmalaCraftingWidget;

/** Owner-local themed shell for a server-accepted interaction with one placed station. */
UCLASS()
class KALMALAUI_API UKalmalaStationContextWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    bool OpenForStation(AKalmalaConstructionActor* Station, const FString& Section,
        UKalmalaCraftingWidget* ServiceContent);
    void Close();
    bool IsOpen() const { return bOpen; }
    bool IsTargetValid() const;
protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
private:
    UFUNCTION() void CloseClicked();
    void ApplyTheme();
    UPROPERTY(Transient) TObjectPtr<UBorder> Background;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StationTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SectionTitle;
    UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
    UPROPERTY(Transient) TObjectPtr<USizeBox> ContentBox;
    UPROPERTY(Transient) TObjectPtr<UKalmalaCraftingWidget> ServiceContent;
    TWeakObjectPtr<AKalmalaConstructionActor> ContextStation;
    TWeakObjectPtr<APawn> ContextOwnerPawn;
    FName ContextKit;
    FString ContextConstructionId;
    int32 LastTextScalePercent = INDEX_NONE;
    int32 LastContrastMode = INDEX_NONE;
    bool bOpen = false;
    bool bPreviousMoveInputIgnored = false;
    bool bPreviousLookInputIgnored = false;
    bool bPreviousCursorVisible = false;
};
