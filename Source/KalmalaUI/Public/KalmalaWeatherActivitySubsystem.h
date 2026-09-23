#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaWeatherActivitySubsystem.generated.h"

class APlayerController;
class UKalmalaWeatherActivityWidget;

/** Presents only the owning local player's view of the replicated weather activity tier. */
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

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaWeatherActivityWidget> WeatherWidget;

    TWeakObjectPtr<APlayerController> LocalController;
};
