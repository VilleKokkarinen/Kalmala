#include "KalmalaAmbientAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaLakeBasin.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldFieldSampler.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Sound/SoundWave.h"

namespace
{
constexpr TCHAR WindBedAssetPath[] = TEXT("/Game/Kalmala/Audio/WindBed.WindBed");
constexpr TCHAR WaterBedAssetPath[] = TEXT("/Game/Kalmala/Audio/WaterBed.WaterBed");
constexpr TCHAR FireBedAssetPath[] = TEXT("/Game/Kalmala/Audio/FireBed.FireBed");
constexpr TCHAR BiomeBedAssetPath[] = TEXT("/Game/Kalmala/Audio/BiomeBed.BiomeBed");
constexpr TCHAR RainBedAssetPath[] = TEXT("/Game/Kalmala/Audio/RainBed.RainBed");
constexpr TCHAR WetStatusCueAssetPath[] = TEXT("/Game/Kalmala/Audio/WetStatusCue.WetStatusCue");
constexpr TCHAR SupportAcceptedCueAssetPath[] = TEXT("/Game/Kalmala/Audio/SupportAcceptedCue.SupportAcceptedCue");
constexpr TCHAR SupportMendingCueAssetPath[] = TEXT("/Game/Kalmala/Audio/SupportMendingCue.SupportMendingCue");
constexpr TCHAR SupportHearthShieldCueAssetPath[] = TEXT("/Game/Kalmala/Audio/SupportHearthShieldCue.SupportHearthShieldCue");
constexpr TCHAR SupportBearsVigorCueAssetPath[] = TEXT("/Game/Kalmala/Audio/SupportBearsVigorCue.SupportBearsVigorCue");
constexpr TCHAR SupportDeerCallCueAssetPath[] = TEXT("/Game/Kalmala/Audio/SupportDeerCallCue.SupportDeerCallCue");
constexpr TCHAR SupportHearthShieldExpiryCueAssetPath[] = TEXT("/Game/Kalmala/Audio/SupportHearthShieldExpiryCue.SupportHearthShieldExpiryCue");
constexpr TCHAR SupportBearsVigorExpiryCueAssetPath[] = TEXT("/Game/Kalmala/Audio/SupportBearsVigorExpiryCue.SupportBearsVigorExpiryCue");
constexpr TCHAR CombatResultCueAssetPath[] = TEXT("/Game/Kalmala/Audio/CombatResultCue.CombatResultCue");
constexpr TCHAR DiscoveryAcknowledgedCueAssetPath[] = TEXT("/Game/Kalmala/Audio/DiscoveryAcknowledgedCue.DiscoveryAcknowledgedCue");
constexpr TCHAR InteractionAcceptedCueAssetPath[] = TEXT("/Game/Kalmala/Audio/InteractionAcceptedCue.InteractionAcceptedCue");
constexpr TCHAR InteractionRejectedCueAssetPath[] = TEXT("/Game/Kalmala/Audio/InteractionRejectedCue.InteractionRejectedCue");
constexpr float QuietWindVolume = 0.025f;
constexpr float StrongWindVolume = 0.10f;
constexpr float WindFadeSpeed = 1.4f;
constexpr float RainMinimumIntensity = 0.01f;
constexpr float RainMaximumVolume = 0.065f;
constexpr float RainFadeSpeed = 1.8f;
constexpr float WetStatusCueVolume = 0.16f;
constexpr float SupportEffectCueVolume = 0.14f;
constexpr float SupportEffectExpiryCueVolume = 0.10f;
constexpr float CombatResultCueVolume = 0.16f;
constexpr float DiscoveryAcknowledgedCueVolume = 0.14f;
constexpr float InteractionAcceptedCueVolume = 0.14f;
constexpr float InteractionRejectedCueVolume = 0.12f;
constexpr float InteractionCueMinimumInterval = 0.35f;
constexpr float WaterMaximumVolume = 0.07f;
constexpr float WaterMaximumDistance = 1600.0f;
constexpr float WaterFullVolumeDistance = 300.0f;
constexpr float WaterProbeInterval = 0.75f;
constexpr float WaterFadeSpeed = 2.5f;
constexpr float FireMaximumVolume = 0.075f;
constexpr float FireMaximumDistance = 1400.0f;
constexpr float FireFullVolumeDistance = 275.0f;
constexpr float FireProbeInterval = 0.75f;
constexpr float FireFadeSpeed = 2.5f;
constexpr float BiomeProbeInterval = 0.75f;
constexpr float BiomeFadeSpeed = 1.6f;

struct FBiomeAmbienceProfile
{
    float Volume = 0.0f;
    float Pitch = 1.0f;
};

const TCHAR* GetBiomeName(const EKalmalaBiome Biome)
{
    switch (Biome)
    {
    case EKalmalaBiome::Meadows: return TEXT("Meadows");
    case EKalmalaBiome::ShimmeringLakes: return TEXT("ShimmeringLakes");
    case EKalmalaBiome::Elderwood: return TEXT("Elderwood");
    case EKalmalaBiome::MossyMire: return TEXT("MossyMire");
    case EKalmalaBiome::FreezingTundra: return TEXT("FreezingTundra");
    case EKalmalaBiome::ThunderMountains: return TEXT("ThunderMountains");
    case EKalmalaBiome::Ocean: return TEXT("Ocean");
    default: return TEXT("Unknown");
    }
}

FBiomeAmbienceProfile GetBiomeAmbienceProfile(const EKalmalaBiome Biome)
{
    // One quiet original texture changes gently with the sampled local biome;
    // the existing water/fire beds retain their more specific context cues.
    switch (Biome)
    {
    case EKalmalaBiome::Meadows: return {0.030f, 1.00f};
    case EKalmalaBiome::ShimmeringLakes: return {0.025f, 1.04f};
    case EKalmalaBiome::Elderwood: return {0.036f, 1.10f};
    case EKalmalaBiome::MossyMire: return {0.030f, 0.94f};
    case EKalmalaBiome::FreezingTundra: return {0.022f, 0.86f};
    case EKalmalaBiome::ThunderMountains: return {0.027f, 0.91f};
    case EKalmalaBiome::Ocean: return {0.020f, 0.97f};
    default: return {};
    }
}
}

void UKalmalaAmbientAudioSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    InteractionCueCooldownRemaining = FMath::Max(0.0f, InteractionCueCooldownRemaining - DeltaTime);
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr)
    {
        StopAmbientAudio();
        return;
    }

    if ((AmbientAudio != nullptr && (!IsValid(AmbientAudio.Get()) || AmbientAudio->GetWorld() != World))
        || (WaterAmbientAudio != nullptr && (!IsValid(WaterAmbientAudio.Get()) || WaterAmbientAudio->GetWorld() != World))
        || (FireAmbientAudio != nullptr && (!IsValid(FireAmbientAudio.Get()) || FireAmbientAudio->GetWorld() != World))
        || (BiomeAmbientAudio != nullptr && (!IsValid(BiomeAmbientAudio.Get()) || BiomeAmbientAudio->GetWorld() != World))
        || (RainAmbientAudio != nullptr && (!IsValid(RainAmbientAudio.Get()) || RainAmbientAudio->GetWorld() != World)))
    {
        StopAmbientAudio();
        WindBed = nullptr;
        WaterBed = nullptr;
        FireBed = nullptr;
        BiomeBed = nullptr;
        RainBed = nullptr;
        WetStatusCue = nullptr;
        SupportAcceptedCue = nullptr;
        SupportMendingCue = nullptr;
        SupportHearthShieldCue = nullptr;
        SupportBearsVigorCue = nullptr;
        SupportDeerCallCue = nullptr;
        SupportHearthShieldExpiryCue = nullptr;
        SupportBearsVigorExpiryCue = nullptr;
        CombatResultCue = nullptr;
        DiscoveryAcknowledgedCue = nullptr;
        InteractionAcceptedCue = nullptr;
        InteractionRejectedCue = nullptr;
        WaterProbeTimeRemaining = 0.0f;
        FireProbeTimeRemaining = 0.0f;
        BiomeProbeTimeRemaining = 0.0f;
    }

    const AKalmalaWorldGenerationGameState* GameState = Cast<AKalmalaWorldGenerationGameState>(World->GetGameState());
    APlayerController* Controller = LocalPlayer->GetPlayerController(World);
    const bool bInNormalLocalPlay = Controller != nullptr
        && Controller->IsLocalController()
        && Controller->GetPawn() != nullptr
        && GameState != nullptr;
    if (!bInNormalLocalPlay)
    {
        StopAmbientAudio();
        return;
    }

    if (AmbientAudio == nullptr && WindBed == nullptr)
    {
        WindBed = LoadObject<USoundWave>(nullptr, WindBedAssetPath);
    }
    if (AmbientAudio == nullptr && WindBed != nullptr)
    {
        WindBed->bLooping = true;
        AmbientAudio = UGameplayStatics::CreateSound2D(World, WindBed, 0.0f, 1.0f, 0.0f, nullptr, false, false);
        if (AmbientAudio != nullptr)
        {
            AmbientAudio->Play();
#if !UE_BUILD_SHIPPING
            if (!bVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
            {
                bVerificationLogged = true;
                UE_LOG(LogTemp, Display, TEXT("Ambient audio runtime: ComponentCreated=1 Local=1 Asset=WindBed Looping=%d"), WindBed->bLooping ? 1 : 0);
            }
#endif
        }
    }

    if (GameState != nullptr)
    {
        const FKalmalaWeatherState& Weather = GameState->GetWeatherState();
        UpdateWindAmbience(DeltaTime, Weather);
        UpdateRainAmbience(DeltaTime, World, Weather);
        UpdateWetStatusCue(World, Controller);
        UpdateSupportEffectCues(World, Controller);
        UpdateCombatResultCue(World, Controller);
        UpdateDiscoveryAcknowledgementCue(World, Controller);
        UpdateInteractionResultCue(World, Controller);
        UpdateGatheringResultCue(World, Controller);
        UpdateWaterAmbience(DeltaTime, World, Controller, GameState->GetWorldGenerationConfig());
        UpdateFireAmbience(DeltaTime, World, Controller);
        UpdateBiomeAmbience(DeltaTime, World, Controller, GameState->GetWorldGenerationConfig());
    }
}

void UKalmalaAmbientAudioSubsystem::UpdateWindAmbience(const float DeltaTime, const FKalmalaWeatherState& Weather)
{
    const float WindStrength = FMath::IsFinite(Weather.WindStrength)
        ? FMath::Clamp(Weather.WindStrength, 0.0f, 1.0f)
        : 0.0f;
    TargetWindVolume = FMath::Lerp(QuietWindVolume, StrongWindVolume, WindStrength);
    if (IsValid(AmbientAudio.Get()))
    {
        CurrentWindVolume = FMath::FInterpTo(CurrentWindVolume, TargetWindVolume, DeltaTime, WindFadeSpeed);
        AmbientAudio->SetVolumeMultiplier(CurrentWindVolume);
    }
}

void UKalmalaAmbientAudioSubsystem::UpdateRainAmbience(const float DeltaTime, UWorld* World,
    const FKalmalaWeatherState& Weather)
{
    const float WindStrength = FMath::IsFinite(Weather.WindStrength)
        ? FMath::Clamp(Weather.WindStrength, 0.0f, 1.0f)
        : 0.0f;
    const float Precipitation = FMath::IsFinite(Weather.PrecipitationIntensity)
        ? FMath::Clamp(Weather.PrecipitationIntensity, 0.0f, 1.0f)
        : 0.0f;
    TargetRainVolume = Precipitation >= RainMinimumIntensity
        ? RainMaximumVolume * FMath::Lerp(0.20f, 1.0f, Precipitation)
        : 0.0f;

    if (TargetRainVolume > 0.0f && RainBed == nullptr)
    {
        RainBed = LoadObject<USoundWave>(nullptr, RainBedAssetPath);
    }
    if (TargetRainVolume > 0.0f && RainAmbientAudio == nullptr && RainBed != nullptr)
    {
        RainBed->bLooping = true;
        RainAmbientAudio = UGameplayStatics::CreateSound2D(World, RainBed, 0.0f, 1.0f, 0.0f, nullptr, false, false);
        if (RainAmbientAudio != nullptr)
        {
            RainAmbientAudio->Play();
        }
    }

    if (IsValid(RainAmbientAudio.Get()))
    {
        CurrentRainVolume = FMath::FInterpTo(CurrentRainVolume, TargetRainVolume, DeltaTime, RainFadeSpeed);
        RainAmbientAudio->SetVolumeMultiplier(CurrentRainVolume);
        if (TargetRainVolume <= 0.0f && CurrentRainVolume <= 0.003f)
        {
            RainAmbientAudio->Stop();
            RainAmbientAudio->DestroyComponent();
            RainAmbientAudio = nullptr;
            CurrentRainVolume = 0.0f;
        }
    }

#if !UE_BUILD_SHIPPING
    if (!bWeatherVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
    {
        bWeatherVerificationLogged = true;
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio weather context: Local=1 Precipitation=%.2f WindStrength=%.2f RainComponentCreated=%d Asset=%s"),
            Precipitation, WindStrength,
            IsValid(RainAmbientAudio.Get()) ? 1 : 0,
            IsValid(RainAmbientAudio.Get()) ? TEXT("RainBed") : TEXT("None"));
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::UpdateWetStatusCue(UWorld* World, APlayerController* Controller)
{
    const AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
    const UKalmalaPlayerStatusComponent* Statuses = Character != nullptr ? Character->GetStatusComponent() : nullptr;
    const bool bWetStatusActive = Statuses != nullptr && Statuses->HasStatus(UKalmalaPlayerStatusComponent::WetStatusId);
    const bool bPlayCue = bWetStatusActive && (!bWetStatusInitialized || !bLastWetStatus);
    bWetStatusInitialized = true;
    bLastWetStatus = bWetStatusActive;
    if (!bPlayCue)
    {
        return;
    }

    if (WetStatusCue == nullptr)
    {
        WetStatusCue = LoadObject<USoundWave>(nullptr, WetStatusCueAssetPath);
    }
    const bool bCueSubmitted = World != nullptr && WetStatusCue != nullptr;
    if (bCueSubmitted)
    {
        UGameplayStatics::PlaySound2D(World, WetStatusCue, WetStatusCueVolume, 1.0f, 0.0f, nullptr, nullptr, false);
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
    {
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio exposure context: Local=1 Wet=%d CueSubmitted=%d Asset=%s"),
            bWetStatusActive ? 1 : 0, bCueSubmitted ? 1 : 0, bCueSubmitted ? TEXT("WetStatusCue") : TEXT("None"));
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::UpdateSupportEffectCues(UWorld* World, APlayerController* Controller)
{
    AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
    if (Character == nullptr)
    {
        SupportFeedbackPawn = nullptr;
        LastSupportFeedbackSerial = 0;
        bSupportEffectStateInitialized = false;
        return;
    }

    if (SupportFeedbackPawn.Get() != Character)
    {
        SupportFeedbackPawn = Character;
        LastSupportFeedbackSerial = 0;
        bSupportEffectStateInitialized = false;
    }

    const UKalmalaSupportMagicComponent* Support = Character->GetSupportMagicComponent();
    if (Support == nullptr)
    {
        return;
    }

    const bool bHearthShieldActive = Support->GetHearthShieldExpiry() > 0.0f
        && Support->GetHearthShieldStrength() > 0.0f;
    const bool bBearsVigorActive = Support->GetBearsVigorExpiry() > 0.0f
        && Support->GetBearsVigorStrengthMultiplier() > 1.0f;
    if (!bSupportEffectStateInitialized)
    {
        bLastHearthShieldActive = bHearthShieldActive;
        bLastBearsVigorActive = bBearsVigorActive;
        bSupportEffectStateInitialized = true;
    }
    else
    {
        const auto SubmitExpiryCue = [this, World](TObjectPtr<USoundWave>& Cue,
            const TCHAR* AssetPath, const TCHAR* AssetName)
        {
            if (Cue == nullptr)
            {
                Cue = LoadObject<USoundWave>(nullptr, AssetPath);
            }
            const bool bSubmitted = World != nullptr && Cue != nullptr;
            if (bSubmitted)
            {
                UGameplayStatics::PlaySound2D(World, Cue, SupportEffectExpiryCueVolume,
                    1.0f, 0.0f, nullptr, nullptr, false);
            }
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
            {
                UE_LOG(LogTemp, Display,
                    TEXT("Ambient audio support expiry: Local=1 CueSubmitted=%d Asset=%s"),
                    bSubmitted ? 1 : 0, bSubmitted ? AssetName : TEXT("None"));
            }
#endif
        };

        if (bLastHearthShieldActive && !bHearthShieldActive)
        {
            SubmitExpiryCue(SupportHearthShieldExpiryCue, SupportHearthShieldExpiryCueAssetPath,
                TEXT("SupportHearthShieldExpiryCue"));
        }
        if (bLastBearsVigorActive && !bBearsVigorActive)
        {
            SubmitExpiryCue(SupportBearsVigorExpiryCue, SupportBearsVigorExpiryCueAssetPath,
                TEXT("SupportBearsVigorExpiryCue"));
        }
        bLastHearthShieldActive = bHearthShieldActive;
        bLastBearsVigorActive = bBearsVigorActive;
    }

    const uint32 FeedbackSerial = Support->GetFeedbackSerial();
    if (FeedbackSerial == 0 || FeedbackSerial == LastSupportFeedbackSerial)
    {
        return;
    }
    LastSupportFeedbackSerial = FeedbackSerial;

    const bool bAccepted = Support->GetFeedback() == EKalmalaSupportFeedback::Accepted;
    bool bCueSubmitted = false;
    const TCHAR* EffectName = TEXT("None");
    const TCHAR* AssetName = TEXT("None");
    if (bAccepted)
    {
        USoundWave* Cue = nullptr;
        const TCHAR* AssetPath = SupportAcceptedCueAssetPath;
        switch (Support->GetActiveEffect())
        {
        case EKalmalaSupportEffect::Mending:
            AssetPath = SupportMendingCueAssetPath;
            if (SupportMendingCue == nullptr) SupportMendingCue = LoadObject<USoundWave>(nullptr, AssetPath);
            Cue = SupportMendingCue;
            EffectName = TEXT("Mending");
            AssetName = TEXT("SupportMendingCue");
            break;
        case EKalmalaSupportEffect::HearthShield:
            AssetPath = SupportHearthShieldCueAssetPath;
            if (SupportHearthShieldCue == nullptr) SupportHearthShieldCue = LoadObject<USoundWave>(nullptr, AssetPath);
            Cue = SupportHearthShieldCue;
            EffectName = TEXT("HearthShield");
            AssetName = TEXT("SupportHearthShieldCue");
            break;
        case EKalmalaSupportEffect::BearsVigor:
            AssetPath = SupportBearsVigorCueAssetPath;
            if (SupportBearsVigorCue == nullptr) SupportBearsVigorCue = LoadObject<USoundWave>(nullptr, AssetPath);
            Cue = SupportBearsVigorCue;
            EffectName = TEXT("BearsVigor");
            AssetName = TEXT("SupportBearsVigorCue");
            break;
        case EKalmalaSupportEffect::DeerCall:
            AssetPath = SupportDeerCallCueAssetPath;
            if (SupportDeerCallCue == nullptr) SupportDeerCallCue = LoadObject<USoundWave>(nullptr, AssetPath);
            Cue = SupportDeerCallCue;
            EffectName = TEXT("DeerCall");
            AssetName = TEXT("SupportDeerCallCue");
            break;
        default:
            break;
        }

        if (Cue == nullptr)
        {
            if (SupportAcceptedCue == nullptr)
            {
                SupportAcceptedCue = LoadObject<USoundWave>(nullptr, SupportAcceptedCueAssetPath);
            }
            Cue = SupportAcceptedCue;
            AssetName = TEXT("SupportAcceptedCue");
        }
        bCueSubmitted = World != nullptr && Cue != nullptr;
        if (bCueSubmitted)
        {
            UGameplayStatics::PlaySound2D(World, Cue, SupportEffectCueVolume,
                1.0f, 0.0f, nullptr, nullptr, false);
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")) && bAccepted)
    {
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio support activation: Local=1 Feedback=Accepted Serial=%u Effect=%s CueSubmitted=%d Asset=%s"),
            FeedbackSerial, EffectName,
            bCueSubmitted ? 1 : 0, bCueSubmitted ? AssetName : TEXT("None"));
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::UpdateCombatResultCue(UWorld* World, APlayerController* Controller)
{
    AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
    if (Character == nullptr)
    {
        CombatFeedbackPawn = nullptr;
        LastCombatFeedbackSerial = 0;
        return;
    }

    if (CombatFeedbackPawn.Get() != Character)
    {
        CombatFeedbackPawn = Character;
        LastCombatFeedbackSerial = 0;
    }

    const UKalmalaCombatComponent* Combat = Character->GetCombatComponent();
    if (Combat == nullptr)
    {
        return;
    }

    const uint32 FeedbackSerial = Combat->GetFeedbackSerial();
    if (FeedbackSerial == 0 || FeedbackSerial == LastCombatFeedbackSerial)
    {
        return;
    }
    LastCombatFeedbackSerial = FeedbackSerial;

    const EKalmalaCombatFeedback Feedback = Combat->GetFeedback();
    const bool bConfirmedResult = Feedback == EKalmalaCombatFeedback::Hit
        || Feedback == EKalmalaCombatFeedback::Defeat;
    bool bCueSubmitted = false;
    if (bConfirmedResult)
    {
        if (CombatResultCue == nullptr)
        {
            CombatResultCue = LoadObject<USoundWave>(nullptr, CombatResultCueAssetPath);
        }
        bCueSubmitted = World != nullptr && CombatResultCue != nullptr;
        if (bCueSubmitted)
        {
            UGameplayStatics::PlaySound2D(World, CombatResultCue, CombatResultCueVolume,
                1.0f, 0.0f, nullptr, nullptr, false);
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest")))
    {
        const TCHAR* FeedbackName = Feedback == EKalmalaCombatFeedback::Hit ? TEXT("Hit")
            : Feedback == EKalmalaCombatFeedback::Defeat ? TEXT("Defeat")
            : Feedback == EKalmalaCombatFeedback::Unavailable ? TEXT("Unavailable") : TEXT("None");
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio combat context: Local=1 Feedback=%s Serial=%u CueSubmitted=%d Asset=%s"),
            FeedbackName, FeedbackSerial, bCueSubmitted ? 1 : 0,
            bCueSubmitted ? TEXT("CombatResultCue") : TEXT("None"));
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::UpdateDiscoveryAcknowledgementCue(UWorld* World, APlayerController* Controller)
{
    AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
    if (Character == nullptr)
    {
        DiscoveryFeedbackPawn = nullptr;
        LastDiscoveryFeedbackSerial = 0;
        return;
    }

    if (DiscoveryFeedbackPawn.Get() != Character)
    {
        DiscoveryFeedbackPawn = Character;
        LastDiscoveryFeedbackSerial = 0;
    }

    const UKalmalaDiscoveryProgressComponent* Discovery = Character->GetDiscoveryProgressComponent();
    if (Discovery == nullptr)
    {
        return;
    }

    const uint32 FeedbackSerial = Discovery->GetFeedbackSerial();
    if (FeedbackSerial == 0 || FeedbackSerial == LastDiscoveryFeedbackSerial)
    {
        return;
    }
    LastDiscoveryFeedbackSerial = FeedbackSerial;

    const EKalmalaDiscoveryFeedback Feedback = Discovery->GetFeedback();
    const bool bAcceptedDiscovery = Feedback == EKalmalaDiscoveryFeedback::LandmarkFound
        || Feedback == EKalmalaDiscoveryFeedback::ScrollFound;
    bool bCueSubmitted = false;
    if (bAcceptedDiscovery)
    {
        if (DiscoveryAcknowledgedCue == nullptr)
        {
            DiscoveryAcknowledgedCue = LoadObject<USoundWave>(nullptr, DiscoveryAcknowledgedCueAssetPath);
        }
        bCueSubmitted = World != nullptr && IsValid(DiscoveryAcknowledgedCue.Get());
        if (bCueSubmitted)
        {
            UGameplayStatics::PlaySound2D(World, DiscoveryAcknowledgedCue.Get(),
                DiscoveryAcknowledgedCueVolume, 1.0f, 0.0f, nullptr, nullptr, false);
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaDiscoveryPeerTest")))
    {
        const TCHAR* FeedbackName = Feedback == EKalmalaDiscoveryFeedback::LandmarkFound ? TEXT("LandmarkFound")
            : Feedback == EKalmalaDiscoveryFeedback::ScrollFound ? TEXT("ScrollFound")
            : Feedback == EKalmalaDiscoveryFeedback::AlreadyFound ? TEXT("AlreadyFound")
            : Feedback == EKalmalaDiscoveryFeedback::Unavailable ? TEXT("Unavailable") : TEXT("None");
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio discovery result: Local=1 Feedback=%s Serial=%u CueSubmitted=%d Asset=%s"),
            FeedbackName,
            FeedbackSerial, bCueSubmitted ? 1 : 0, bCueSubmitted ? TEXT("DiscoveryAcknowledgedCue") : TEXT("None"));
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::UpdateInteractionResultCue(UWorld* World, APlayerController* Controller)
{
    AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
    if (Character == nullptr)
    {
        InteractionFeedbackPawn = nullptr;
        LastInteractionFeedbackSerial = 0;
        return;
    }

    if (InteractionFeedbackPawn.Get() != Character)
    {
        InteractionFeedbackPawn = Character;
        LastInteractionFeedbackSerial = 0;
    }

    const UKalmalaCraftingComponent* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
    if (Crafting == nullptr)
    {
        return;
    }

    const uint32 ResultSerial = Crafting->GetResultSerial();
    if (ResultSerial == 0 || ResultSerial == LastInteractionFeedbackSerial)
    {
        return;
    }
    LastInteractionFeedbackSerial = ResultSerial;

    const bool bAccepted = Crafting->WasLastResultAccepted();
    USoundWave* Cue = bAccepted ? InteractionAcceptedCue.Get() : InteractionRejectedCue.Get();
    const TCHAR* AssetName = bAccepted ? TEXT("InteractionAcceptedCue") : TEXT("InteractionRejectedCue");
    bool bCueSubmitted = false;
    if (InteractionCueCooldownRemaining <= 0.0f)
    {
        if (Cue == nullptr)
        {
            if (bAccepted)
            {
                InteractionAcceptedCue = LoadObject<USoundWave>(nullptr, InteractionAcceptedCueAssetPath);
                Cue = InteractionAcceptedCue;
            }
            else
            {
                InteractionRejectedCue = LoadObject<USoundWave>(nullptr, InteractionRejectedCueAssetPath);
                Cue = InteractionRejectedCue;
            }
        }
        bCueSubmitted = World != nullptr && IsValid(Cue);
        if (bCueSubmitted)
        {
            UGameplayStatics::PlaySound2D(World, Cue,
                bAccepted ? InteractionAcceptedCueVolume : InteractionRejectedCueVolume,
                1.0f, 0.0f, nullptr, nullptr, false);
            InteractionCueCooldownRemaining = InteractionCueMinimumInterval;
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
    {
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio interaction result: Local=1 Feedback=%s Serial=%u CueSubmitted=%d Asset=%s"),
            bAccepted ? TEXT("Accepted") : TEXT("Unavailable"), ResultSerial, bCueSubmitted ? 1 : 0,
            bCueSubmitted ? AssetName : TEXT("None"));
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::UpdateGatheringResultCue(UWorld* World, APlayerController* Controller)
{
    AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
    if (Character == nullptr)
    {
        GatheringFeedbackPawn = nullptr;
        LastGatheringQuantities.Reset();
        bGatheringInventoryInitialized = false;
        return;
    }

    if (GatheringFeedbackPawn.Get() != Character)
    {
        GatheringFeedbackPawn = Character;
        LastGatheringQuantities.Reset();
        bGatheringInventoryInitialized = false;
    }

    const UKalmalaInventoryComponent* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    if (Inventory == nullptr)
    {
        LastGatheringQuantities.Reset();
        bGatheringInventoryInitialized = false;
        return;
    }

    TMap<FName, int32> CurrentQuantities;
    for (const FKalmalaInventoryStack& Stack : Inventory->GetStacks())
    {
        if (!Stack.ItemId.IsNone() && Stack.Quantity > 0)
        {
            CurrentQuantities.Add(Stack.ItemId, Stack.Quantity);
        }
    }

    if (!bGatheringInventoryInitialized)
    {
        LastGatheringQuantities = MoveTemp(CurrentQuantities);
        bGatheringInventoryInitialized = true;
        return;
    }

    bool bInventoryIncreased = false;
    for (const TPair<FName, int32>& Current : CurrentQuantities)
    {
        const int32 PreviousQuantity = LastGatheringQuantities.FindRef(Current.Key);
        if (Current.Value > PreviousQuantity)
        {
            bInventoryIncreased = true;
            break;
        }
    }
    LastGatheringQuantities = MoveTemp(CurrentQuantities);
    if (!bInventoryIncreased)
    {
        return;
    }

    bool bCueSubmitted = false;
    if (InteractionCueCooldownRemaining <= 0.0f)
    {
        if (InteractionAcceptedCue == nullptr)
        {
            InteractionAcceptedCue = LoadObject<USoundWave>(nullptr, InteractionAcceptedCueAssetPath);
        }
        bCueSubmitted = World != nullptr && InteractionAcceptedCue != nullptr;
        if (bCueSubmitted)
        {
            UGameplayStatics::PlaySound2D(World, InteractionAcceptedCue.Get(), InteractionAcceptedCueVolume,
                1.0f, 0.0f, nullptr, nullptr, false);
            InteractionCueCooldownRemaining = InteractionCueMinimumInterval;
        }
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
    {
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio gathering result: Local=1 InventoryIncrease=1 CueSubmitted=%d Asset=%s"),
            bCueSubmitted ? 1 : 0, bCueSubmitted ? TEXT("InteractionAcceptedCue") : TEXT("None"));
    }
#endif
}

float UKalmalaAmbientAudioSubsystem::SampleVisibleWaterStrength(UWorld* World, APlayerController* Controller,
    const FKalmalaWorldGenerationConfig& Config) const
{
    if (World == nullptr || Controller == nullptr || Controller->GetPawn() == nullptr)
    {
        return 0.0f;
    }

    APawn* Pawn = Controller->GetPawn();
    FVector ViewOrigin;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewOrigin, ViewRotation);
    const FVector2D ListenerPosition(Pawn->GetActorLocation());

    TArray<FVector2D, TInlineAllocator<17>> ProbeOffsets;
    ProbeOffsets.Add(FVector2D::ZeroVector);
    for (const float Radius : {750.0f, 1500.0f})
    {
        for (int32 Direction = 0; Direction < 8; ++Direction)
        {
            const float Angle = FMath::DegreesToRadians(Direction * 45.0f);
            ProbeOffsets.Add(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
        }
    }

    FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(KalmalaAmbientWaterVisibility), true, Pawn);
    TraceParams.AddIgnoredActor(Pawn);
    float BestDistanceSquared = FMath::Square(WaterMaximumDistance);

    for (const FVector2D& Offset : ProbeOffsets)
    {
        const FVector2D Candidate2D = ListenerPosition + Offset;
        const float DistanceSquared = FVector2D::DistSquared(ListenerPosition, Candidate2D);
        if (DistanceSquared > BestDistanceSquared)
        {
            continue;
        }

        float WaterSurfaceHeight = 0.0f;
        const FKalmalaOceanSample OceanSample = FKalmalaOceanSampler::Sample(Config, Candidate2D);
        if (!OceanSample.IsWater())
        {
            if (!FKalmalaLakeBasin::IsVisibleWater(Config, Candidate2D))
            {
                continue;
            }
            WaterSurfaceHeight = FKalmalaShimmeringLakeSampler::WaterSurfaceWorldHeight;
        }

        const FVector Candidate(Candidate2D, WaterSurfaceHeight);
        FHitResult VisibilityHit;
        if (World->LineTraceSingleByChannel(VisibilityHit, ViewOrigin, Candidate, ECC_Visibility, TraceParams))
        {
            continue;
        }

        BestDistanceSquared = DistanceSquared;
    }

    if (BestDistanceSquared >= FMath::Square(WaterMaximumDistance))
    {
        return 0.0f;
    }

    const float Distance = FMath::Sqrt(BestDistanceSquared);
    const float Proximity = FMath::Clamp(
        (WaterMaximumDistance - Distance) / (WaterMaximumDistance - WaterFullVolumeDistance), 0.0f, 1.0f);
    return WaterMaximumVolume * Proximity;
}

void UKalmalaAmbientAudioSubsystem::UpdateWaterAmbience(const float DeltaTime, UWorld* World,
    APlayerController* Controller, const FKalmalaWorldGenerationConfig& Config)
{
    WaterProbeTimeRemaining -= DeltaTime;
    bool bProbedThisFrame = false;
    if (WaterProbeTimeRemaining <= 0.0f)
    {
        TargetWaterVolume = SampleVisibleWaterStrength(World, Controller, Config);
        WaterProbeTimeRemaining = WaterProbeInterval;
        bProbedThisFrame = true;
    }

    if (bProbedThisFrame && TargetWaterVolume > 0.0f && WaterBed == nullptr)
    {
        WaterBed = LoadObject<USoundWave>(nullptr, WaterBedAssetPath);
    }
    if (TargetWaterVolume > 0.0f && WaterAmbientAudio == nullptr && WaterBed != nullptr)
    {
        WaterBed->bLooping = true;
        WaterAmbientAudio = UGameplayStatics::CreateSound2D(World, WaterBed, 0.0f, 1.0f, 0.0f, nullptr, false, false);
        if (WaterAmbientAudio != nullptr)
        {
            WaterAmbientAudio->Play();
        }
    }

    if (IsValid(WaterAmbientAudio.Get()))
    {
        CurrentWaterVolume = FMath::FInterpTo(CurrentWaterVolume, TargetWaterVolume, DeltaTime, WaterFadeSpeed);
        WaterAmbientAudio->SetVolumeMultiplier(CurrentWaterVolume);
        if (TargetWaterVolume <= 0.0f && CurrentWaterVolume <= 0.003f)
        {
            WaterAmbientAudio->Stop();
            WaterAmbientAudio->DestroyComponent();
            WaterAmbientAudio = nullptr;
            CurrentWaterVolume = 0.0f;
        }
    }

#if !UE_BUILD_SHIPPING
    if (!bWaterVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest"))
        && bProbedThisFrame)
    {
        bWaterVerificationLogged = true;
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio water context: Probed=1 Local=1 Visible=%d ComponentCreated=%d Asset=%s"),
            TargetWaterVolume > 0.0f ? 1 : 0, IsValid(WaterAmbientAudio.Get()) ? 1 : 0,
            IsValid(WaterAmbientAudio.Get()) ? TEXT("WaterBed") : TEXT("None"));
    }
#endif
}

float UKalmalaAmbientAudioSubsystem::SampleVisibleFireStrength(UWorld* World, APlayerController* Controller) const
{
    if (World == nullptr || Controller == nullptr || Controller->GetPawn() == nullptr)
    {
        return 0.0f;
    }

    APawn* Pawn = Controller->GetPawn();
    FVector ViewOrigin;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewOrigin, ViewRotation);
    const FVector ListenerPosition = Pawn->GetActorLocation();
    float BestDistanceSquared = FMath::Square(FireMaximumDistance);

    FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(KalmalaAmbientFireVisibility), true, Pawn);
    TraceParams.AddIgnoredActor(Pawn);
    for (TActorIterator<AKalmalaCampfire> Iterator(World); Iterator; ++Iterator)
    {
        const AKalmalaCampfire* Hearth = *Iterator;
        if (!IsValid(Hearth) || !Hearth->GetIsReplicated())
        {
            continue;
        }
        if (!Hearth->IsLit() || Hearth->GetActorLocation().ContainsNaN())
        {
            continue;
        }

        const float DistanceSquared = FVector::DistSquared(ListenerPosition, Hearth->GetActorLocation());
        if (DistanceSquared > BestDistanceSquared)
        {
            continue;
        }

        // Aim into the collision sphere around the visible stone ring. A trace hit on
        // the hearth itself is clear; any intervening actor or terrain hides it.
        const FVector HearthFocus = Hearth->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
        FHitResult VisibilityHit;
        if (World->LineTraceSingleByChannel(VisibilityHit, ViewOrigin, HearthFocus, ECC_Visibility, TraceParams)
            && VisibilityHit.GetActor() != Hearth)
        {
            continue;
        }

        BestDistanceSquared = DistanceSquared;
    }

    if (BestDistanceSquared >= FMath::Square(FireMaximumDistance))
    {
        return 0.0f;
    }

    const float Distance = FMath::Sqrt(BestDistanceSquared);
    const float Proximity = FMath::Clamp(
        (FireMaximumDistance - Distance) / (FireMaximumDistance - FireFullVolumeDistance), 0.0f, 1.0f);
    return FireMaximumVolume * Proximity;
}

void UKalmalaAmbientAudioSubsystem::UpdateFireAmbience(const float DeltaTime, UWorld* World,
    APlayerController* Controller)
{
    FireProbeTimeRemaining -= DeltaTime;
    bool bProbedThisFrame = false;
    if (FireProbeTimeRemaining <= 0.0f)
    {
        TargetFireVolume = SampleVisibleFireStrength(World, Controller);
        FireProbeTimeRemaining = FireProbeInterval;
        bProbedThisFrame = true;
    }

    if (bProbedThisFrame && TargetFireVolume > 0.0f && FireBed == nullptr)
    {
        FireBed = LoadObject<USoundWave>(nullptr, FireBedAssetPath);
    }
    if (TargetFireVolume > 0.0f && FireAmbientAudio == nullptr && FireBed != nullptr)
    {
        FireBed->bLooping = true;
        FireAmbientAudio = UGameplayStatics::CreateSound2D(World, FireBed, 0.0f, 1.0f, 0.0f, nullptr, false, false);
        if (FireAmbientAudio != nullptr)
        {
            FireAmbientAudio->Play();
        }
    }

    if (IsValid(FireAmbientAudio.Get()))
    {
        CurrentFireVolume = FMath::FInterpTo(CurrentFireVolume, TargetFireVolume, DeltaTime, FireFadeSpeed);
        FireAmbientAudio->SetVolumeMultiplier(CurrentFireVolume);
        if (TargetFireVolume <= 0.0f && CurrentFireVolume <= 0.003f)
        {
            FireAmbientAudio->Stop();
            FireAmbientAudio->DestroyComponent();
            FireAmbientAudio = nullptr;
            CurrentFireVolume = 0.0f;
        }
    }

#if !UE_BUILD_SHIPPING
    if (bProbedThisFrame && FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
    {
        const bool bVisible = TargetFireVolume > 0.0f;
        if (!bFireVerificationLogged || bVisible != bLastFireVisible)
        {
            bFireVerificationLogged = true;
            bLastFireVisible = bVisible;
            UE_LOG(LogTemp, Display,
                TEXT("Ambient audio fire context: Probed=1 Local=1 Visible=%d ComponentCreated=%d Asset=%s"),
                bVisible ? 1 : 0, IsValid(FireAmbientAudio.Get()) ? 1 : 0,
                IsValid(FireAmbientAudio.Get()) ? TEXT("FireBed") : TEXT("None"));
        }
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::UpdateBiomeAmbience(const float DeltaTime, UWorld* World,
    APlayerController* Controller, const FKalmalaWorldGenerationConfig& Config)
{
    BiomeProbeTimeRemaining -= DeltaTime;
    bool bProbedThisFrame = false;
    EKalmalaBiome SampledBiome = EKalmalaBiome::Meadows;
    if (BiomeProbeTimeRemaining <= 0.0f)
    {
        APawn* Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
        if (Pawn != nullptr && !Pawn->GetActorLocation().ContainsNaN())
        {
            const FVector PawnLocation = Pawn->GetActorLocation();
            const FKalmalaWorldFieldSample Field = FKalmalaWorldFieldSampler::Sample(
                Config, FVector2D(PawnLocation.X, PawnLocation.Y));
            SampledBiome = FKalmalaBiomeClassifier::Classify(Field);
            const FBiomeAmbienceProfile Profile = GetBiomeAmbienceProfile(SampledBiome);
            TargetBiomeVolume = Profile.Volume;
            TargetBiomePitch = Profile.Pitch;
        }
        else
        {
            TargetBiomeVolume = 0.0f;
            TargetBiomePitch = 1.0f;
        }
        BiomeProbeTimeRemaining = BiomeProbeInterval;
        bProbedThisFrame = true;
    }

    if (bProbedThisFrame && TargetBiomeVolume > 0.0f && BiomeBed == nullptr)
    {
        BiomeBed = LoadObject<USoundWave>(nullptr, BiomeBedAssetPath);
    }
    if (TargetBiomeVolume > 0.0f && BiomeAmbientAudio == nullptr && BiomeBed != nullptr)
    {
        BiomeBed->bLooping = true;
        BiomeAmbientAudio = UGameplayStatics::CreateSound2D(World, BiomeBed, 0.0f, CurrentBiomePitch,
            0.0f, nullptr, false, false);
        if (BiomeAmbientAudio != nullptr)
        {
            BiomeAmbientAudio->Play();
        }
    }

    if (IsValid(BiomeAmbientAudio.Get()))
    {
        CurrentBiomeVolume = FMath::FInterpTo(CurrentBiomeVolume, TargetBiomeVolume, DeltaTime, BiomeFadeSpeed);
        CurrentBiomePitch = FMath::FInterpTo(CurrentBiomePitch, TargetBiomePitch, DeltaTime, BiomeFadeSpeed);
        BiomeAmbientAudio->SetVolumeMultiplier(CurrentBiomeVolume);
        BiomeAmbientAudio->SetPitchMultiplier(CurrentBiomePitch);
        if (TargetBiomeVolume <= 0.0f && CurrentBiomeVolume <= 0.003f)
        {
            BiomeAmbientAudio->Stop();
            BiomeAmbientAudio->DestroyComponent();
            BiomeAmbientAudio = nullptr;
            CurrentBiomeVolume = 0.0f;
            CurrentBiomePitch = 1.0f;
        }
    }

#if !UE_BUILD_SHIPPING
    if (!bBiomeVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest"))
        && bProbedThisFrame)
    {
        bBiomeVerificationLogged = true;
        UE_LOG(LogTemp, Display,
            TEXT("Ambient audio biome context: Probed=1 Local=1 Biome=%s ComponentCreated=%d Asset=%s Pitch=%.2f"),
            GetBiomeName(SampledBiome), IsValid(BiomeAmbientAudio.Get()) ? 1 : 0,
            IsValid(BiomeAmbientAudio.Get()) ? TEXT("BiomeBed") : TEXT("None"), TargetBiomePitch);
    }
#endif
}

void UKalmalaAmbientAudioSubsystem::StopAmbientAudio()
{
    if (IsValid(AmbientAudio.Get()))
    {
        if (AmbientAudio->IsPlaying())
        {
            AmbientAudio->Stop();
        }
        AmbientAudio->DestroyComponent();
    }
    AmbientAudio = nullptr;
    CurrentWindVolume = 0.0f;
    TargetWindVolume = 0.0f;

    if (IsValid(WaterAmbientAudio.Get()))
    {
        if (WaterAmbientAudio->IsPlaying())
        {
            WaterAmbientAudio->Stop();
        }
        WaterAmbientAudio->DestroyComponent();
    }
    WaterAmbientAudio = nullptr;
    CurrentWaterVolume = 0.0f;
    TargetWaterVolume = 0.0f;

    if (IsValid(FireAmbientAudio.Get()))
    {
        if (FireAmbientAudio->IsPlaying())
        {
            FireAmbientAudio->Stop();
        }
        FireAmbientAudio->DestroyComponent();
    }
    FireAmbientAudio = nullptr;
    CurrentFireVolume = 0.0f;
    TargetFireVolume = 0.0f;
    FireProbeTimeRemaining = 0.0f;

    if (IsValid(BiomeAmbientAudio.Get()))
    {
        if (BiomeAmbientAudio->IsPlaying())
        {
            BiomeAmbientAudio->Stop();
        }
        BiomeAmbientAudio->DestroyComponent();
    }
    BiomeAmbientAudio = nullptr;
    CurrentBiomeVolume = 0.0f;
    TargetBiomeVolume = 0.0f;
    CurrentBiomePitch = 1.0f;
    TargetBiomePitch = 1.0f;
    BiomeProbeTimeRemaining = 0.0f;
    bBiomeVerificationLogged = false;

    if (IsValid(RainAmbientAudio.Get()))
    {
        if (RainAmbientAudio->IsPlaying())
        {
            RainAmbientAudio->Stop();
        }
        RainAmbientAudio->DestroyComponent();
    }
    RainAmbientAudio = nullptr;
    CurrentRainVolume = 0.0f;
    TargetRainVolume = 0.0f;
    bWeatherVerificationLogged = false;
    bWetStatusInitialized = false;
    bLastWetStatus = false;
    SupportFeedbackPawn = nullptr;
    LastSupportFeedbackSerial = 0;
    DiscoveryFeedbackPawn = nullptr;
    LastDiscoveryFeedbackSerial = 0;
    InteractionFeedbackPawn = nullptr;
    LastInteractionFeedbackSerial = 0;
    GatheringFeedbackPawn = nullptr;
    LastGatheringQuantities.Reset();
    bGatheringInventoryInitialized = false;
    InteractionCueCooldownRemaining = 0.0f;
}

void UKalmalaAmbientAudioSubsystem::Deinitialize()
{
    StopAmbientAudio();
    WindBed = nullptr;
    WaterBed = nullptr;
    FireBed = nullptr;
    BiomeBed = nullptr;
    RainBed = nullptr;
    WetStatusCue = nullptr;
    SupportAcceptedCue = nullptr;
    CombatResultCue = nullptr;
    DiscoveryAcknowledgedCue = nullptr;
    InteractionAcceptedCue = nullptr;
    InteractionRejectedCue = nullptr;
    Super::Deinitialize();
}
