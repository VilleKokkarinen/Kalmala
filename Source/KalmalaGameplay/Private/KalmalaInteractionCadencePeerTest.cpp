#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaInteractionTestActor.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if !UE_BUILD_SHIPPING
void AKalmalaCharacter::ConfigureInteractionCadencePeerTestView()
{
    if (CameraBoom == nullptr) return;
    CameraBoom->TargetArmLength = 0.0f;
    CameraBoom->TargetOffset = FVector(0.0f, 0.0f, 45.0f);
    CameraBoom->bEnableCameraLag = false;
    CameraBoom->bEnableCameraRotationLag = false;
}

void AKalmalaCharacter::ResetInteractionCadencePeerTestCounters()
{
    InteractionCadencePeerRequestCount = 0;
    InteractionCadencePeerTraceCount = 0;
    InteractionCadencePeerFirstRequestTime = -1.0;
    InteractionCadencePeerFirstTraceTime = -1.0;
}

void AKalmalaCharacter::ResetInteractionRequestCadenceForVerification()
{
    LastServerInteractionRequestTime = -1.0;
}

void AKalmalaCharacter::VerifyInteractionCadencePeer()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaInteractionCadencePeerOwner"))
        || HasAuthority() || !IsLocallyControlled() || Controller == nullptr || GetWorld() == nullptr
        || InteractionCadencePeerStage >= 4)
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    const auto Fail = [this](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Error, TEXT("Interaction cadence peer owner: Passed=0 Reason=%s"), Reason);
        InteractionCadencePeerStage = 4;
    };

    AKalmalaInteractionTestActor* Target = nullptr;
    for (TActorIterator<AKalmalaInteractionTestActor> It(GetWorld()); It; ++It)
    {
        if (It->IsInteractionCadencePeerTarget())
        {
            Target = *It;
            break;
        }
    }

    if (InteractionCadencePeerStage == 0)
    {
        if (InteractionCadencePeerStageTime <= 0.0)
        {
            InteractionCadencePeerStageTime = Now;
        }
        if (Now - InteractionCadencePeerStageTime > 30.0)
        {
            Fail(TEXT("the server test target did not replicate"));
            return;
        }
        if (Target == nullptr) return;

        ConfigureInteractionCadencePeerTestView();
        FVector ViewLocation;
        FRotator ViewRotation;
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
        (void)ViewRotation;
        Controller->SetControlRotation((Target->GetActorLocation() - ViewLocation).Rotation());
        InteractionCadencePeerStage = 1;
        InteractionCadencePeerStageTime = Now;
        return;
    }

    if (Target == nullptr)
    {
        Fail(TEXT("the replicated interaction target disappeared"));
        return;
    }

    if (InteractionCadencePeerStage == 1)
    {
        if (Now - InteractionCadencePeerStageTime > 15.0)
        {
            Fail(TEXT("the remote owner's ordinary view trace did not find the test target"));
            return;
        }

        FVector ViewLocation;
        FRotator ViewRotation;
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
        (void)ViewRotation;
        Controller->SetControlRotation((Target->GetActorLocation() - ViewLocation).Rotation());
        FHitResult Hit;
        if (!GetLocalInteractionCandidate(Hit) || Hit.GetActor() != Target) return;

        RequestInteract();
        for (int32 Repeat = 0; Repeat < 7; ++Repeat)
        {
            ServerRequestInteract(NAME_None, 0);
        }
        InteractionCadencePeerStage = 2;
        InteractionCadencePeerStageTime = Now;
        return;
    }

    if (InteractionCadencePeerStage == 2)
    {
        if (Target->GetInteractionCount() > 1)
        {
            Fail(TEXT("the rapid burst dispatched more than once"));
            return;
        }
        if (Now - InteractionCadencePeerStageTime > 15.0)
        {
            Fail(TEXT("the first remote interaction did not dispatch"));
            return;
        }
        if (Target->GetInteractionCount() == 1 && Now - InteractionCadencePeerStageTime >= 0.5)
        {
            RequestInteract();
            InteractionCadencePeerStage = 3;
            InteractionCadencePeerStageTime = Now;
        }
        return;
    }

    if (Target->GetInteractionCount() > 2)
    {
        Fail(TEXT("the paced retry dispatched more than once"));
        return;
    }
    if (Target->GetInteractionCount() == 2)
    {
        UE_LOG(LogTemp, Display,
            TEXT("Interaction cadence peer owner: Passed=1 FirstDispatch=1 BurstBounded=1 PacedDispatch=1"));
        InteractionCadencePeerStage = 4;
        return;
    }
    if (Now - InteractionCadencePeerStageTime > 15.0)
    {
        Fail(TEXT("the normally paced retry did not dispatch"));
    }
}

void AKalmalaGameMode::DriveInteractionCadencePeerTest()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaInteractionCadencePeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || InteractionCadencePeerTestStage >= 99)
    {
        return;
    }

    const auto Fail = [this](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Error, TEXT("Interaction cadence peer verification FAILED: %s"), Reason);
        InteractionCadencePeerTestStage = 99;
    };

    const double Now = GetWorld()->GetTimeSeconds();
    if (InteractionCadencePeerTestStage == 0)
    {
        AKalmalaCharacter* ListenHost = nullptr;
        AKalmalaCharacter* RemoteOwner = nullptr;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PlayerController = It->Get();
            AKalmalaCharacter* Candidate = PlayerController != nullptr
                ? Cast<AKalmalaCharacter>(PlayerController->GetPawn()) : nullptr;
            if (Candidate == nullptr || Candidate->GetPlayerState() == nullptr) continue;
            if (PlayerController->IsLocalController())
            {
                ListenHost = Candidate;
            }
            else if (RemoteOwner == nullptr
                || Candidate->GetPlayerState()->GetPlayerId() < RemoteOwner->GetPlayerState()->GetPlayerId())
            {
                RemoteOwner = Candidate;
            }
        }
        if (ListenHost == nullptr || RemoteOwner == nullptr) return;
        APlayerController* OwnerController = Cast<APlayerController>(RemoteOwner->GetController());
        UCharacterMovementComponent* Movement = RemoteOwner->GetCharacterMovement();
        if (OwnerController == nullptr || OwnerController->IsLocalController() || Movement == nullptr)
        {
            Fail(TEXT("the remote owner controller or movement component was unavailable"));
            return;
        }

        Movement->StopMovementImmediately();
        Movement->SetMovementMode(MOVE_Flying);
        RemoteOwner->SetActorLocation(FVector(0.0f, 0.0f, 10000.0f), false, nullptr, ETeleportType::TeleportPhysics);
        RemoteOwner->SetActorRotation(FRotator::ZeroRotator);
        OwnerController->SetControlRotation(FRotator::ZeroRotator);
        RemoteOwner->ConfigureInteractionCadencePeerTestView();
        RemoteOwner->ResetInteractionCadencePeerTestCounters();

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaInteractionTestActor* Target = GetWorld()->SpawnActor<AKalmalaInteractionTestActor>(
            AKalmalaInteractionTestActor::StaticClass(), RemoteOwner->GetActorLocation() + FVector(180.0f, 0.0f, 70.0f),
            FRotator::ZeroRotator, SpawnParameters);
        if (Target == nullptr)
        {
            Fail(TEXT("the interaction test target could not be spawned"));
            return;
        }
        Target->MarkAsInteractionCadencePeerTargetFromServer();
        RemoteOwner->ForceNetUpdate();
        InteractionCadencePeerTestOwner = RemoteOwner;
        InteractionCadencePeerTestTarget = Target;
        InteractionCadencePeerTestStage = 1;
        InteractionCadencePeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* RemoteOwner = InteractionCadencePeerTestOwner.Get();
    AKalmalaInteractionTestActor* Target = InteractionCadencePeerTestTarget.Get();
    if (RemoteOwner == nullptr || Target == nullptr)
    {
        Fail(TEXT("the owner or interaction target disappeared"));
        return;
    }
    if (Now - InteractionCadencePeerTestStageTime > 30.0)
    {
        Fail(TEXT("remote interaction requests did not complete before timeout"));
        return;
    }

    if (InteractionCadencePeerTestStage == 1)
    {
        const int32 Received = RemoteOwner->GetInteractionCadencePeerRequestCount();
        if (Received > 8)
        {
            Fail(TEXT("the burst included an unexpected extra request"));
            return;
        }
        if (Received < 8 || RemoteOwner->GetInteractionCadencePeerFirstRequestTime() < 0.0
            || Now - RemoteOwner->GetInteractionCadencePeerFirstRequestTime() < 0.15)
        {
            return;
        }

        const double FirstRequestTime = RemoteOwner->GetInteractionCadencePeerFirstRequestTime();
        const double FirstTraceTime = RemoteOwner->GetInteractionCadencePeerFirstTraceTime();
        const bool bFirstImmediate = FirstTraceTime >= FirstRequestTime
            && FirstTraceTime - FirstRequestTime <= 0.001;
        if (RemoteOwner->GetInteractionCadencePeerTraceCount() != 1
            || Target->GetInteractionCount() != 1 || !bFirstImmediate)
        {
            Fail(TEXT("the first request was delayed or the burst produced multiple traces or dispatches"));
            return;
        }
        InteractionCadencePeerTestStage = 2;
        InteractionCadencePeerTestStageTime = Now;
        return;
    }

    const int32 Received = RemoteOwner->GetInteractionCadencePeerRequestCount();
    const int32 Traces = RemoteOwner->GetInteractionCadencePeerTraceCount();
    const int32 Dispatches = Target->GetInteractionCount();
    if (Received > 9 || Traces > 2 || Dispatches > 2)
    {
        Fail(TEXT("an extra interaction trace or dispatch passed the cadence gate"));
        return;
    }
    if (Received == 9 && Traces == 2 && Dispatches == 2)
    {
        UE_LOG(LogTemp, Display,
            TEXT("Interaction cadence peer server: Passed=1 RemoteOwner=1 BurstRequests=8 CadencePassedTraces=1 FirstImmediate=1 FirstDispatch=1 PacedRequest=1 BoundedDispatch=2"));
        InteractionCadencePeerTestStage = 99;
    }
}
#endif
