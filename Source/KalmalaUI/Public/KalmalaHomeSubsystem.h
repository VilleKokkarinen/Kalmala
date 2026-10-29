#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaHomeSubsystem.generated.h"

class APlayerController;
class UKalmalaHomeWidget;
class UKalmalaPlayerStepWidget;
class UKalmalaWorldListWidget;
enum class EKalmalaHomeAction : uint8;

/** Shows Home only for the dedicated menu controller and active Home state. */
UCLASS()
class KALMALAUI_API UKalmalaHomeSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UKalmalaHomeWidget* GetHomeWidget() const { return HomeWidget; }
    UKalmalaPlayerStepWidget* GetPlayerStep() const { return PlayerStep; }
    void ReturnFromOptions();
    void AbandonOptions();
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaHomeSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
private:
    void Release();
    void HandleHomeAction(EKalmalaHomeAction Action);
    void HandlePlayerBack();
    void HandlePlayerContinue();
    void HandleWorldListBack();
    void ReleasePlayerStep();
    void ReleaseWorldList();
    UPROPERTY(Transient) TObjectPtr<UKalmalaHomeWidget> HomeWidget;
    UPROPERTY(Transient) TObjectPtr<UKalmalaPlayerStepWidget> PlayerStep;
    UPROPERTY(Transient) TObjectPtr<UKalmalaWorldListWidget> WorldList;
    UPROPERTY(Transient) TObjectPtr<APlayerController> HomeController;
};
