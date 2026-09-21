#include "KalmalaAmbientAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "KalmalaCampfire.h"
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
constexpr float AmbientVolume = 0.12f;
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
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr)
    {
        StopAmbientAudio();
        return;
    }

    if ((AmbientAudio != nullptr && (!IsValid(AmbientAudio.Get()) || AmbientAudio->GetWorld() != World))
        || (WaterAmbientAudio != nullptr && (!IsValid(WaterAmbientAudio.Get()) || WaterAmbientAudio->GetWorld() != World))
        || (FireAmbientAudio != nullptr && (!IsValid(FireAmbientAudio.Get()) || FireAmbientAudio->GetWorld() != World))
        || (BiomeAmbientAudio != nullptr && (!IsValid(BiomeAmbientAudio.Get()) || BiomeAmbientAudio->GetWorld() != World)))
    {
        StopAmbientAudio();
        WindBed = nullptr;
        WaterBed = nullptr;
        FireBed = nullptr;
        BiomeBed = nullptr;
        WaterProbeTimeRemaining = 0.0f;
        FireProbeTimeRemaining = 0.0f;
        BiomeProbeTimeRemaining = 0.0f;
    }

    APlayerController* Controller = LocalPlayer->GetPlayerController(World);
    const bool bInNormalLocalPlay = Controller != nullptr
        && Controller->IsLocalController()
        && Controller->GetPawn() != nullptr
        && Cast<AKalmalaWorldGenerationGameState>(World->GetGameState()) != nullptr;
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
        AmbientAudio = UGameplayStatics::CreateSound2D(World, WindBed, AmbientVolume, 1.0f, 0.0f, nullptr, false, false);
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

    const AKalmalaWorldGenerationGameState* GameState = Cast<AKalmalaWorldGenerationGameState>(World->GetGameState());
    if (GameState != nullptr)
    {
        UpdateWaterAmbience(DeltaTime, World, Controller, GameState->GetWorldGenerationConfig());
        UpdateFireAmbience(DeltaTime, World, Controller);
        UpdateBiomeAmbience(DeltaTime, World, Controller, GameState->GetWorldGenerationConfig());
    }
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
}

void UKalmalaAmbientAudioSubsystem::Deinitialize()
{
    StopAmbientAudio();
    WindBed = nullptr;
    WaterBed = nullptr;
    FireBed = nullptr;
    BiomeBed = nullptr;
    Super::Deinitialize();
}
