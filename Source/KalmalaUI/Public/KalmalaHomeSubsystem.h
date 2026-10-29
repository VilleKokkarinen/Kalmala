#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaHomeSubsystem.generated.h"

class APlayerController;
class UKalmalaHomeWidget;

/** Shows Home only for the dedicated menu controller and active Home state. */
UCLASS()
class KALMALAUI_API UKalmalaHomeSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UKalmalaHomeWidget* GetHomeWidget() const { return HomeWidget; }
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaHomeSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
private:
    void Release();
    UPROPERTY(Transient) TObjectPtr<UKalmalaHomeWidget> HomeWidget;
    UPROPERTY(Transient) TObjectPtr<APlayerController> HomeController;
};
