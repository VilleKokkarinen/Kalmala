#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaSurvivalStatusSubsystem.generated.h"

class APlayerController;
class AKalmalaCharacter;
class UKalmalaSurvivalStatusWidget;

/** Builds each local player's status view from that pawn's existing replicated state. */
UCLASS()
class KALMALAUI_API UKalmalaSurvivalStatusSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaSurvivalStatusSubsystem, STATGROUP_Tickables);
    }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    void ReleaseController();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaSurvivalStatusWidget> StatusWidget;

    TWeakObjectPtr<APlayerController> LocalController;
    TWeakObjectPtr<AKalmalaCharacter> FeedbackPawn;
    uint32 LastOceanTravelFeedbackSerial = 0;
    uint32 LastLoggedOceanTravelFeedbackSerial = 0;
    float OceanTravelFeedbackExpiry = 0.0f;
};
