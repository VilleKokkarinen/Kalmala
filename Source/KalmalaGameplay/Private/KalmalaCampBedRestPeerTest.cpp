#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaPlayerModelComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if !UE_BUILD_SHIPPING
namespace
{
AKalmalaConstructionActor* FindCampBedRestPeerTarget(UWorld* World)
{
    if (!World) return nullptr;
    for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
    {
        if (It->GetConstructionKit() == TEXT("CampBedKit")
            && It->GetConstructionId().StartsWith(TEXT("camp-bed-rest-peer-")))
        {
            return *It;
        }
    }
    return nullptr;
}
}

void AKalmalaCharacter::VerifyCampBedRestPeer()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampBedRestPeerOwner"))
        || HasAuthority() || !IsLocallyControlled() || !Controller || !GetWorld()
        || !PlayerModel || !Crafting || CampBedRestPeerStage >= 6)
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    const auto Fail = [this](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Error, TEXT("Camp Bed rest peer owner: Passed=0 Reason=%s"), Reason);
        CampBedRestPeerStage = 99;
    };

    AKalmalaConstructionActor* Target = FindCampBedRestPeerTarget(GetWorld());
    if (CampBedRestPeerStage == 0)
    {
        if (CampBedRestPeerStageTime <= 0.0) CampBedRestPeerStageTime = Now;
        if (Now - CampBedRestPeerStageTime > 30.0)
        {
            Fail(TEXT("the server Camp Bed target did not replicate"));
            return;
        }
        if (!Target) return;

        ConfigureInteractionCadencePeerTestView();
        FVector ViewLocation;
        FRotator ViewRotation;
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
        Controller->SetControlRotation((Target->GetActorLocation() - ViewLocation).Rotation());
        CampBedRestPeerOriginalModelTransform = PlayerModel->GetRelativeTransform();
        CampBedRestPeerInitialReceiptSerial = Crafting->GetCampBedRestReceipt().Serial;
        if (PlayerModel->IsCampBedResting())
        {
            Fail(TEXT("the remote owner's model was already resting before the request"));
            return;
        }
        CampBedRestPeerStage = 1;
        CampBedRestPeerStageTime = Now;
        return;
    }

    if (!Target)
    {
        Fail(TEXT("the Camp Bed target disappeared"));
        return;
    }

    if (CampBedRestPeerStage == 1)
    {
        if (Now - CampBedRestPeerStageTime > 20.0)
        {
            Fail(TEXT("the owner did not acquire the Camp Bed with its ordinary view trace"));
            return;
        }

        FVector ViewLocation;
        FRotator ViewRotation;
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
        Controller->SetControlRotation((Target->GetActorLocation() - ViewLocation).Rotation());
        FHitResult Hit;
        if (!GetLocalInteractionCandidate(Hit) || Hit.GetActor() != Target) return;

        RequestInteract();
        if (PlayerModel->IsCampBedResting()
            || !PlayerModel->GetRelativeTransform().Equals(CampBedRestPeerOriginalModelTransform, 0.01f))
        {
            Fail(TEXT("the model changed before the server interaction result"));
            return;
        }
        CampBedRestPeerStage = 2;
        CampBedRestPeerStageTime = Now;
        return;
    }

    const FKalmalaCampBedRestReceipt& Receipt = Crafting->GetCampBedRestReceipt();
    if (CampBedRestPeerStage == 2)
    {
        if (Now - CampBedRestPeerStageTime > 20.0)
        {
            Fail(TEXT("the rate-rejected request did not return an owner result"));
            return;
        }
        if (Receipt.Serial == CampBedRestPeerInitialReceiptSerial) return;
        if (Receipt.bAccepted || PlayerModel->IsCampBedResting()
            || !PlayerModel->GetRelativeTransform().Equals(CampBedRestPeerOriginalModelTransform, 0.01f))
        {
            Fail(TEXT("the rate-rejected result changed the local model pose"));
            return;
        }

        CampBedRestPeerInitialReceiptSerial = Receipt.Serial;
        CampBedRestPeerStage = 3;
        CampBedRestPeerStageTime = Now + 0.35;
        return;
    }

    if (CampBedRestPeerStage == 3)
    {
        if (Now < CampBedRestPeerStageTime) return;
        if (PlayerModel->IsCampBedResting()
            || !PlayerModel->GetRelativeTransform().Equals(CampBedRestPeerOriginalModelTransform, 0.01f))
        {
            Fail(TEXT("the model did not remain unchanged while waiting to retry"));
            return;
        }

        FVector ViewLocation;
        FRotator ViewRotation;
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
        Controller->SetControlRotation((Target->GetActorLocation() - ViewLocation).Rotation());
        FHitResult Hit;
        if (!GetLocalInteractionCandidate(Hit) || Hit.GetActor() != Target) return;

        RequestInteract();
        if (PlayerModel->IsCampBedResting()
            || !PlayerModel->GetRelativeTransform().Equals(CampBedRestPeerOriginalModelTransform, 0.01f))
        {
            Fail(TEXT("the model changed before the accepted server response"));
            return;
        }
        CampBedRestPeerStage = 4;
        CampBedRestPeerStageTime = Now;
        return;
    }

    if (CampBedRestPeerStage == 4)
    {
        if (Now - CampBedRestPeerStageTime > 20.0)
        {
            Fail(TEXT("the accepted server result did not reach the remote owner"));
            return;
        }
        if (Receipt.Serial == CampBedRestPeerInitialReceiptSerial) return;

        FTransform ResolvedBedTransform;
        if (!UKalmalaCraftingComponent::ResolveCampBedRestReceipt(true, Receipt,
                CampBedRestPeerInitialReceiptSerial, ResolvedBedTransform))
        {
            Fail(TEXT("the owner did not receive an accepted Camp Bed result"));
            return;
        }

        const FRotator BedYaw(0.0f, ResolvedBedTransform.Rotator().Yaw, 0.0f);
        const FVector ExpectedLocation = ResolvedBedTransform.GetLocation()
            + BedYaw.Vector() * 85.0f + FVector(0.0f, 0.0f, 35.0f);
        const FRotator ExpectedRotation(-90.0f, BedYaw.Yaw, 0.0f);
        if (!PlayerModel->IsCampBedResting()
            || FVector::Dist(PlayerModel->GetComponentLocation(), ExpectedLocation) > 2.0f
            || !PlayerModel->GetComponentRotation().Equals(ExpectedRotation, 1.0f))
        {
            Fail(TEXT("the accepted owner result did not apply the bed pose"));
            return;
        }

        CampBedRestPeerStage = 5;
        CampBedRestPeerStageTime = Now;
        return;
    }

    if (CampBedRestPeerStage == 5 && Now - CampBedRestPeerStageTime >= 0.5)
    {
        if (!PlayerModel->IsCampBedResting())
        {
            Fail(TEXT("the accepted pose did not remain while the owner was stationary and healthy"));
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("Camp Bed rest peer owner: Passed=1 RateRejectedPoseUnchanged=1 AcceptedPose=1 Stationary=1"));
        CampBedRestPeerStage = 6;
    }
}

void AKalmalaGameMode::DriveCampBedRestPeerTest()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampBedRestPeerTest"))
        || !HasAuthority() || !GetWorld() || CampBedRestPeerTestStage >= 99)
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    const auto Fail = [this](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Error, TEXT("Camp Bed rest peer server: Passed=0 Reason=%s"), Reason);
        CampBedRestPeerTestStage = 99;
    };

    if (CampBedRestPeerTestStage == 0)
    {
        if (CampBedRestPeerTestStageTime <= 0.0) CampBedRestPeerTestStageTime = Now;
        if (Now - CampBedRestPeerTestStageTime > 30.0)
        {
            Fail(TEXT("a listen host and remote owner did not join"));
            return;
        }

        AKalmalaCharacter* ListenHost = nullptr;
        AKalmalaCharacter* RemoteOwner = nullptr;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PlayerController = It->Get();
            AKalmalaCharacter* Candidate = PlayerController ? Cast<AKalmalaCharacter>(PlayerController->GetPawn()) : nullptr;
            if (!Candidate || !Candidate->GetPlayerState()) continue;
            if (PlayerController->IsLocalController()) ListenHost = Candidate;
            else if (!RemoteOwner || Candidate->GetPlayerState()->GetPlayerId() < RemoteOwner->GetPlayerState()->GetPlayerId())
                RemoteOwner = Candidate;
        }
        if (!ListenHost || !RemoteOwner) return;

        UKalmalaPlayerModelComponent* HostModel = ListenHost->FindComponentByClass<UKalmalaPlayerModelComponent>();
        UKalmalaPlayerModelComponent* RemoteModel = RemoteOwner->FindComponentByClass<UKalmalaPlayerModelComponent>();
        UKalmalaCraftingComponent* RemoteCrafting = RemoteOwner->FindComponentByClass<UKalmalaCraftingComponent>();
        APlayerController* OwnerController = Cast<APlayerController>(RemoteOwner->GetController());
        UCharacterMovementComponent* Movement = RemoteOwner->GetCharacterMovement();
        if (!HostModel || !RemoteModel || !RemoteCrafting || !OwnerController || OwnerController->IsLocalController()
            || !Movement || HostModel->IsCampBedResting() || RemoteModel->IsCampBedResting())
        {
            Fail(TEXT("the listen host, remote owner, or initial model state was invalid"));
            return;
        }

        Movement->StopMovementImmediately();
        Movement->SetMovementMode(MOVE_Flying);
        RemoteOwner->SetActorLocationAndRotation(FVector(0.0f, 0.0f, 10000.0f), FRotator::ZeroRotator,
            false, nullptr, ETeleportType::TeleportPhysics);
        RemoteOwner->ConfigureInteractionCadencePeerTestView();
        OwnerController->SetControlRotation(FRotator::ZeroRotator);

        const FVector BedLocation = RemoteOwner->GetPawnViewLocation() + FVector(180.0f, 0.0f, 0.0f);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaConstructionActor* Bed = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
            AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, BedLocation), SpawnParameters);
        if (!Bed)
        {
            Fail(TEXT("the isolated Camp Bed could not be spawned"));
            return;
        }

        const FString ConstructionId = FString::Printf(TEXT("camp-bed-rest-peer-%d"), RemoteOwner->GetPlayerState()->GetPlayerId());
        Bed->InitializeFromServer(TEXT("CampBedKit"), ConstructionId);
        Bed->ForceNetUpdate();
        RemoteCrafting->SetNextRequestTimeForVerification(Now + 30.0);
        CampBedRestPeerTestOwner = RemoteOwner;
        CampBedRestPeerTestTarget = Bed;
        CampBedRestPeerTestRejectedReceiptSerial = RemoteCrafting->GetCampBedRestReceipt().Serial;
        CampBedRestPeerTestStage = 1;
        CampBedRestPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* RemoteOwner = CampBedRestPeerTestOwner.Get();
    AKalmalaConstructionActor* Bed = CampBedRestPeerTestTarget.Get();
    if (!RemoteOwner || !Bed)
    {
        Fail(TEXT("the remote owner or isolated Camp Bed disappeared"));
        return;
    }
    if (Now - CampBedRestPeerTestStageTime > 45.0)
    {
        Fail(TEXT("the remote interaction result did not complete before timeout"));
        return;
    }

    UKalmalaCraftingComponent* RemoteCrafting = RemoteOwner->FindComponentByClass<UKalmalaCraftingComponent>();
    UKalmalaPlayerModelComponent* RemoteModel = RemoteOwner->FindComponentByClass<UKalmalaPlayerModelComponent>();
    if (!RemoteCrafting || !RemoteModel)
    {
        Fail(TEXT("the remote owner's crafting or model component disappeared"));
        return;
    }

    const FKalmalaCampBedRestReceipt& Receipt = RemoteCrafting->GetCampBedRestReceipt();
    if (CampBedRestPeerTestStage == 1)
    {
        if (Receipt.Serial == CampBedRestPeerTestRejectedReceiptSerial) return;
        if (Receipt.bAccepted || RemoteCrafting->WasLastResultAccepted()
            || RemoteCrafting->GetLastResult() != TEXT("Please wait before the next action")
            || RemoteModel->IsCampBedResting())
        {
            Fail(TEXT("the first request was not rate-rejected without a server-side pose"));
            return;
        }

        CampBedRestPeerTestRejectedReceiptSerial = Receipt.Serial;
        RemoteCrafting->SetNextRequestTimeForVerification(0.0);
        CampBedRestPeerTestStage = 2;
        CampBedRestPeerTestStageTime = Now;
        return;
    }

    if (Receipt.Serial == CampBedRestPeerTestRejectedReceiptSerial) return;
    UKalmalaPlayerModelComponent* HostModel = nullptr;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (!PlayerController || !PlayerController->IsLocalController()) continue;
        if (AKalmalaCharacter* ListenHost = Cast<AKalmalaCharacter>(PlayerController->GetPawn()))
            HostModel = ListenHost->FindComponentByClass<UKalmalaPlayerModelComponent>();
        break;
    }
    if (!Receipt.bAccepted || !RemoteCrafting->WasLastResultAccepted()
        || !RemoteCrafting->GetLastResult().StartsWith(TEXT("Resting briefly"))
        || RemoteModel->IsCampBedResting() || !HostModel || HostModel->IsCampBedResting())
    {
        Fail(TEXT("the accepted result or listen-host presentation boundary was incorrect"));
        return;
    }

    UE_LOG(LogTemp, Display,
        TEXT("Camp Bed rest peer server: Passed=1 RateRejected=1 Accepted=1 ListenHostUnposed=1 RemoteAuthorityUnposed=1"));
    CampBedRestPeerTestStage = 99;
}
}
#endif
