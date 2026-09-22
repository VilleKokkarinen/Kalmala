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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaSettingsSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    void BindLocalInput(APlayerController* InLocalController);
    void ReleaseController();
    void HandleSettingsMenu();
#if !UE_BUILD_SHIPPING
    void RunDeveloperSettingsVerification(float DeltaTime);
#endif

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaSettingsWidget> SettingsWidget;
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> LocalController;
    TWeakObjectPtr<UInputComponent> BoundInputComponent;
#if !UE_BUILD_SHIPPING
    bool bDeveloperSettingsVerificationStarted = false;
    bool bDeveloperSettingsVerificationCompleted = false;
    float DeveloperSettingsVerificationElapsed = 0.0f;
    int32 DeveloperSettingsVerificationStage = 0;
    bool bDeveloperScreenshotPending = false;
    int32 DeveloperSettingsScreenshotTab = -1;
    bool bDeveloperGameplayBaselineCaptured = false;
    FVector DeveloperSettingsInitialLocation = FVector::ZeroVector;
    float DeveloperSettingsInitialHealth = 100.0f;
    uint64 DeveloperSettingsInitialWorldSeed = 0;
#endif
};
