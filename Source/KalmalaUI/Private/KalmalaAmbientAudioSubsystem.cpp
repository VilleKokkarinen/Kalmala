#include "KalmalaAmbientAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "KalmalaLakeBasin.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
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
constexpr float AmbientVolume = 0.12f;
constexpr float WaterMaximumVolume = 0.07f;
constexpr float WaterMaximumDistance = 1600.0f;
constexpr float WaterFullVolumeDistance = 300.0f;
constexpr float WaterProbeInterval = 0.75f;
constexpr float WaterFadeSpeed = 2.5f;
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
        || (WaterAmbientAudio != nullptr && (!IsValid(WaterAmbientAudio.Get()) || WaterAmbientAudio->GetWorld() != World)))
    {
        StopAmbientAudio();
        WindBed = nullptr;
        WaterBed = nullptr;
        WaterProbeTimeRemaining = 0.0f;
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
}

void UKalmalaAmbientAudioSubsystem::Deinitialize()
{
    StopAmbientAudio();
    WindBed = nullptr;
    WaterBed = nullptr;
    Super::Deinitialize();
}
