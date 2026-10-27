#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaWeatherActivitySubsystem.generated.h"

class APlayerController;
class AExponentialHeightFog;
class UMaterialParameterCollection;
class UKalmalaWeatherActivityWidget;

/** Presents local weather feedback and regional fog from replicated state. */
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
    void UpdateMountainWeatherFog(float DeltaTime);
    void UpdateWeatherMaterialResponse(float DeltaTime);
    void DestroyMountainWeatherFog();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaWeatherActivityWidget> WeatherWidget;

    UPROPERTY(Transient)
    TObjectPtr<AExponentialHeightFog> MountainWeatherFog;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialParameterCollection> WeatherMaterialCollection;

    TWeakObjectPtr<APlayerController> LocalController;
    float MountainWeatherFogProbeTimeRemaining = 0.0f;
    float TargetMountainWeatherFogCoverage = 0.0f;
    float CurrentMountainWeatherFogCoverage = 0.0f;
    float CurrentWeatherMaterialResponse = 0.0f;
};
