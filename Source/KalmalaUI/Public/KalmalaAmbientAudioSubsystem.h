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

/** Plays project-owned ambience and accepted-result cues for this local player only. */
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
    void UpdateSupportEffectCues(UWorld* World, APlayerController* Controller);
    void UpdateCombatResultCue(UWorld* World, APlayerController* Controller);
    void UpdateDiscoveryAcknowledgementCue(UWorld* World, APlayerController* Controller);
    void UpdateInteractionResultCue(UWorld* World, APlayerController* Controller);
    void UpdateGatheringResultCue(UWorld* World, APlayerController* Controller);
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

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SupportMendingCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SupportHearthShieldCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SupportBearsVigorCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SupportDeerCallCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SupportHearthShieldExpiryCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> SupportBearsVigorExpiryCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> CombatResultCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> DiscoveryAcknowledgedCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> InteractionAcceptedCue;

    UPROPERTY(Transient)
    TObjectPtr<USoundWave> InteractionRejectedCue;

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
    bool bSupportEffectStateInitialized = false;
    bool bLastHearthShieldActive = false;
    bool bLastBearsVigorActive = false;
    TWeakObjectPtr<APawn> CombatFeedbackPawn;
    uint32 LastCombatFeedbackSerial = 0;
    TWeakObjectPtr<APawn> DiscoveryFeedbackPawn;
    uint32 LastDiscoveryFeedbackSerial = 0;
    TWeakObjectPtr<APawn> InteractionFeedbackPawn;
    uint32 LastInteractionFeedbackSerial = 0;
    TWeakObjectPtr<APawn> GatheringFeedbackPawn;
    TMap<FName, int32> LastGatheringQuantities;
    float InteractionCueCooldownRemaining = 0.0f;
    bool bGatheringInventoryInitialized = false;
    bool bVerificationLogged = false;
    bool bWeatherVerificationLogged = false;
    bool bWetStatusInitialized = false;
    bool bLastWetStatus = false;
    bool bWaterVerificationLogged = false;
    bool bFireVerificationLogged = false;
    bool bLastFireVisible = false;
    bool bBiomeVerificationLogged = false;
};
