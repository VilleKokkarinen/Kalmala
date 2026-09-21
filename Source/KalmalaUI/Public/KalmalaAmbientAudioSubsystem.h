#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Tickable.h"
#include "KalmalaAmbientAudioSubsystem.generated.h"

class UAudioComponent;
class USoundWave;
class APlayerController;
class APawn;
struct FKalmalaWorldGenerationConfig;
struct FKalmalaWeatherState;

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
    void UpdateWindAmbience(float DeltaTime, const FKalmalaWeatherState& Weather);
    void UpdateRainAmbience(float DeltaTime, UWorld* World, const FKalmalaWeatherState& Weather);
    void UpdateWetStatusCue(UWorld* World, APlayerController* Controller);
    void UpdateSupportAcceptedCue(UWorld* World, APlayerController* Controller);
    float SampleVisibleWaterStrength(UWorld* World, APlayerController* Controller,
        const FKalmalaWorldGenerationConfig& Config) const;
    void UpdateWaterAmbience(float DeltaTime, UWorld* World, APlayerController* Controller,
        const FKalmalaWorldGenerationConfig& Config);
    float SampleVisibleFireStrength(UWorld* World, APlayerController* Controller) const;
    void UpdateFireAmbience(float DeltaTime, UWorld* World, APlayerController* Controller);
    void UpdateBiomeAmbience(float DeltaTime, UWorld* World, APlayerController* Controller,
        const FKalmalaWorldGenerationConfig& Config);

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> AmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> WaterAmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> FireAmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> BiomeAmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> RainAmbientAudio;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> WindBed;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> WaterBed;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> FireBed;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> BiomeBed;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> RainBed;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> WetStatusCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SupportAcceptedCue;

    float CurrentWindVolume = 0.0f;
    float TargetWindVolume = 0.0f;
    float WaterProbeTimeRemaining = 0.0f;
    float CurrentWaterVolume = 0.0f;
    float TargetWaterVolume = 0.0f;
    float FireProbeTimeRemaining = 0.0f;
    float CurrentFireVolume = 0.0f;
    float TargetFireVolume = 0.0f;
    float BiomeProbeTimeRemaining = 0.0f;
    float CurrentBiomeVolume = 0.0f;
    float TargetBiomeVolume = 0.0f;
    float CurrentBiomePitch = 1.0f;
    float TargetBiomePitch = 1.0f;
    float CurrentRainVolume = 0.0f;
    float TargetRainVolume = 0.0f;
    TWeakObjectPtr<APawn> SupportFeedbackPawn;
    uint32 LastSupportFeedbackSerial = 0;
    bool bVerificationLogged = false;
    bool bWeatherVerificationLogged = false;
    bool bWetStatusInitialized = false;
    bool bLastWetStatus = false;
    bool bWaterVerificationLogged = false;
    bool bFireVerificationLogged = false;
    bool bLastFireVisible = false;
    bool bBiomeVerificationLogged = false;
};
