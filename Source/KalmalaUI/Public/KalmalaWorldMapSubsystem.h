#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaWorldMapSubsystem.generated.h"

class APlayerController;
class UInputComponent;
class UKalmalaWorldMapWidget;

/** Owns one M-toggle expanded map for each local player. */
UCLASS()
class KALMALAUI_API UKalmalaWorldMapSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaWorldMapSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }
    bool CloseMapIfOpen();

private:
    void BindLocalInput(APlayerController* InLocalController);
    void ReleaseController();
    void ToggleMap();
    void RecenterMap();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaWorldMapWidget> MapWidget;
    UPROPERTY(Transient)
    TObjectPtr<APlayerController> LocalController;
    TWeakObjectPtr<UInputComponent> BoundInputComponent;
};
