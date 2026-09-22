#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaAccessibilityFeedbackSubsystem.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class KALMALAUI_API UKalmalaAccessibilityFeedbackWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetFeedbackText(const FString& Text);

protected:
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> Background;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> FeedbackText;
};

/** Local owner-only text/shape feedback for gameplay states that may otherwise use colour. */
UCLASS()
class KALMALAUI_API UKalmalaAccessibilityFeedbackSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaAccessibilityFeedbackSubsystem, STATGROUP_Tickables);
    }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    FString BuildFeedbackText(class APawn* Pawn) const;
    void ReleaseWidget();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaAccessibilityFeedbackWidget> Widget;

    TWeakObjectPtr<class APlayerController> Controller;
};
