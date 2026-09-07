#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaSettingsSubsystem.generated.h"

class APlayerController;
class UInputComponent;
class UKalmalaSettingsWidget;

/** Owns the local Escape binding and settings widget for one local player. */
UCLASS()
class KALMALAUI_API UKalmalaSettingsSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaSettingsSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    void BindLocalInput(APlayerController* InLocalController);
    void ReleaseController();
    void HandleSettingsMenu();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaSettingsWidget> SettingsWidget;
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> LocalController;
    TWeakObjectPtr<UInputComponent> BoundInputComponent;
};
