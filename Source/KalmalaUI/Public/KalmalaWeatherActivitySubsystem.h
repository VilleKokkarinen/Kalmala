#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaWeatherActivitySubsystem.generated.h"

class APlayerController;
class AExponentialHeightFog;
class UKalmalaWeatherActivityWidget;

/** Presents local weather feedback and Mountain storm atmosphere from replicated state. */
UCLASS()
class KALMALAUI_API UKalmalaWeatherActivitySubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaWeatherActivitySubsystem, STATGROUP_Tickables);
    }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    void ReleaseController();
    void UpdateMountainStormVisuals(float DeltaTime);
    void DestroyMountainStormFog();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaWeatherActivityWidget> WeatherWidget;

    UPROPERTY(Transient)
    TObjectPtr<AExponentialHeightFog> MountainStormFog;

    TWeakObjectPtr<APlayerController> LocalController;
    float MountainStormProbeTimeRemaining = 0.0f;
    float TargetMountainStormCoverage = 0.0f;
    float CurrentMountainStormCoverage = 0.0f;
};
