#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "KalmalaMinimapSubsystem.generated.h"

class UKalmalaMinimapWidget;

/** Creates the local-only minimap after a local player controller is available. */
UCLASS()
class KALMALAUI_API UKalmalaMinimapSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaMinimapSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    UPROPERTY(Transient)
    TObjectPtr<UKalmalaMinimapWidget> MinimapWidget;
};
