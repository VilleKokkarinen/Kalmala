#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaTutorialSubsystem.generated.h"

class AActor;
class APawn;
class APlayerController;
class UBorder;
class UInputComponent;
class USizeBox;
class UTextBlock;

UENUM()
enum class EKalmalaTutorialBeat : uint8
{
    None,
    Arrive,
    Interact,
    Gather,
    Prepare,
    Weather,
    Explore,
    OptionalEncounter,
    Discovery,
    SupportMagic,
    Return
};

UCLASS()
class KALMALAUI_API UKalmalaTutorialPromptWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetPrompt(EKalmalaTutorialBeat Beat, const FString& Title, const FString& Body, const FString& Controls);
    void SetPromptVisible(bool bVisible);
    void SetCardSize(float Width, float Height);

protected:
    virtual void NativeOnInitialized() override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> BodyText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ControlsText;
    UPROPERTY(Transient) TObjectPtr<UBorder> CardBorder;
    UPROPERTY(Transient) TObjectPtr<USizeBox> CardSizeBox;
    EKalmalaTutorialBeat DisplayedBeat = EKalmalaTutorialBeat::None;
};

/** Contextual prompts for this local player; it reads only that player's view and replicated pawn state. */
UCLASS()
class KALMALAUI_API UKalmalaTutorialSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaTutorialSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    void BindLocalInput(APlayerController* InController);
    void ReleaseController();
    void UpdateVisibleFocus(APawn* Pawn);
    EKalmalaTutorialBeat FindAvailableBeat(APawn* Pawn) const;
    bool IsBeatContextActive(EKalmalaTutorialBeat Beat, APawn* Pawn) const;
    bool HasLearnedSupportEffect(const class UKalmalaSupportMagicComponent* Support) const;
    void ShowBeat(EKalmalaTutorialBeat Beat, bool bMarkSeen = true);
    void HideCurrentBeat();
    void DismissPrompt();
    void RevisitPrompt();
    void NoteAttackIntent();
    FString BuildBody(EKalmalaTutorialBeat Beat) const;
    FString BuildControls() const;

    UPROPERTY(Transient) TObjectPtr<APlayerController> LocalController;
    UPROPERTY(Transient) TObjectPtr<UKalmalaTutorialPromptWidget> PromptWidget;
    TWeakObjectPtr<UInputComponent> BoundInputComponent;
    TWeakObjectPtr<APawn> TrackedPawn;
    TWeakObjectPtr<AActor> VisibleFocusActor;
    TSet<EKalmalaTutorialBeat> SeenBeats;
    FVector InitialPawnLocation = FVector::ZeroVector;
    bool bHasInitialPawnLocation = false;
    FIntPoint LastViewportSize = FIntPoint::ZeroValue;
    float LastViewportScale = 0.0f;
    EKalmalaTutorialBeat CurrentBeat = EKalmalaTutorialBeat::None;
    EKalmalaTutorialBeat LastBeat = EKalmalaTutorialBeat::None;
    float CurrentBeatSecondsRemaining = 0.0f;
    float FocusRefreshSecondsRemaining = 0.0f;
    float AttackIntentSecondsRemaining = 0.0f;
};
