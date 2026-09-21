#include "KalmalaAmbientAudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/LocalPlayer.h"
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
constexpr float AmbientVolume = 0.12f;
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

    if (AmbientAudio != nullptr && (!IsValid(AmbientAudio.Get()) || AmbientAudio->GetWorld() != World))
    {
        StopAmbientAudio();
        WindBed = nullptr;
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

    if (AmbientAudio != nullptr)
    {
        return;
    }

    if (WindBed == nullptr)
    {
        WindBed = LoadObject<USoundWave>(nullptr, WindBedAssetPath);
    }
    if (WindBed == nullptr)
    {
        return;
    }

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
}

void UKalmalaAmbientAudioSubsystem::Deinitialize()
{
    StopAmbientAudio();
    WindBed = nullptr;
    Super::Deinitialize();
}
