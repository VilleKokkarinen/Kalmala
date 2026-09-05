#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaMinimapSubsystem.generated.h"

class UKalmalaMinimapWidget;
class APlayerController;
class UInputComponent;

/** Creates the local-only minimap after a local player controller is available. */
UCLASS()
class KALMALAUI_API UKalmalaMinimapSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaMinimapSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    void BindLocalInput(APlayerController* LocalController);
    void HandleMinimapZoom(float WheelDelta);
    void ReleaseController();
    void VerifyLocalInput();

    UPROPERTY(Transient)
    TObjectPtr<UKalmalaMinimapWidget> MinimapWidget;

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> LocalController;

    TWeakObjectPtr<UInputComponent> BoundInputComponent;
    float SessionZoom = 5000.0f;
    bool bVerifiedInput = false;
};
