#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaAmbientAudioSubsystem.generated.h"

class UAudioComponent;
class USoundWave;
class APlayerController;
struct FKalmalaWorldGenerationConfig;

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
    float SampleVisibleWaterStrength(UWorld* World, APlayerController* Controller,
        const FKalmalaWorldGenerationConfig& Config) const;
    void UpdateWaterAmbience(float DeltaTime, UWorld* World, APlayerController* Controller,
        const FKalmalaWorldGenerationConfig& Config);

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> WaterAmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> WindBed;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> WaterBed;

    float WaterProbeTimeRemaining = 0.0f;
    float CurrentWaterVolume = 0.0f;
    float TargetWaterVolume = 0.0f;
    bool bVerificationLogged = false;
    bool bWaterVerificationLogged = false;
};
