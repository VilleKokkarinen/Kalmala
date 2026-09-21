#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaAmbientAudioSubsystem.generated.h"

class UAudioComponent;
class USoundWave;

/** Starts a quiet, project-owned wilderness bed for this local player only. */
UCLASS()
class KALMALAUI_API UKalmalaAmbientAudioSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual void Deinitialize() override;
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UKalmalaAmbientAudioSubsystem, STATGROUP_Tickables); }
    virtual bool IsTickable() const override { return !IsTemplate(); }

private:
    void StopAmbientAudio();

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> WindBed;

    bool bVerificationLogged = false;
};
