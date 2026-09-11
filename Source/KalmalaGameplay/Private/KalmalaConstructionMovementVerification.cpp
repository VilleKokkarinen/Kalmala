#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AKalmalaCharacter::VerifyConstructionMovement(const float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaConstructionMovementTest"))
        || bConstructionMovementFinished || (!HasAuthority() && !IsLocallyControlled())
        || !GetPlayerState() || !GetWorld()->GetGameState()
        || GetWorld()->GetGameState()->PlayerArray.Num() < 2) return;
    const FString Prefix = FString::Printf(TEXT("MovementFixture-%d-"), GetPlayerState()->GetPlayerId());
    if (HasAuthority() && !bConstructionMovementSpawned)
    {
        bConstructionMovementSpawned = true;
        // Isolated elevated test solids, never registered or saved as a player camp.
        const FVector Origin = GetActorLocation() + FVector(GetWorld()->GetGameState()->PlayerArray.IndexOfByKey(GetPlayerState()) * 500.0, 0, 2000);
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        for (const FName Kit : { FName(TEXT("FloorKit")), FName(TEXT("WallKit")), FName(TEXT("RoofKit")) })
        {
            const FVector Offset = Kit == TEXT("FloorKit") ? FVector::ZeroVector
                : Kit == TEXT("WallKit") ? FVector(0, 90, 122) : FVector(0, 0, 250);
            auto* Piece = GetWorld()->SpawnActor<AKalmalaConstructionActor>(Origin + Offset, FRotator::ZeroRotator, Params);
            if (!Piece)
            {
                UE_LOG(LogTemp, Error, TEXT("Construction movement: Passed=0 Spawn failed"));
                bConstructionMovementFinished = true;
                return;
            }
            Piece->InitializeFromServer(Kit, Prefix + Kit.ToString());
            Piece->ForceNetUpdate();
        }
        SetActorLocation(Origin + FVector(0, -70, 14 + GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), false, nullptr, ETeleportType::TeleportPhysics);
        GetCharacterMovement()->StopMovementImmediately();
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        ForceNetUpdate();
    }
    AKalmalaConstructionActor* Floor = nullptr;
    AKalmalaConstructionActor* Wall = nullptr;
    AKalmalaConstructionActor* Roof = nullptr;
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
    {
        if (It->GetConstructionId() == Prefix + TEXT("FloorKit")) Floor = *It;
        if (It->GetConstructionId() == Prefix + TEXT("WallKit")) Wall = *It;
        if (It->GetConstructionId() == Prefix + TEXT("RoofKit")) Roof = *It;
    }
    if (!Floor || !Wall || !Roof || FMath::Abs(GetActorLocation().Z - Floor->GetActorLocation().Z) > 200) return;
    if (ConstructionMovementElapsed == 0) ConstructionMovementStartY = GetActorLocation().Y;
    ConstructionMovementElapsed += DeltaSeconds;
    if (ConstructionMovementElapsed < 3) return; // Allow replicated bases and teleport to settle.
    // Observe the remote jump throughout the movement phase: peer frame clocks
    // need not reach the local jump trigger at the same wall-clock time.
    const float Z = GetActorLocation().Z - Floor->GetActorLocation().Z;
    ConstructionRoofPeakZ = FMath::Max(ConstructionRoofPeakZ, Z);
    bConstructionRoofAirborne |= GetCharacterMovement()->IsFalling();
    if (IsLocallyControlled() && ConstructionMovementElapsed < 7) AddMovementInput(FVector::YAxisVector, 1.0f, true);
    if (ConstructionMovementElapsed < 7) return;
    const float Y = GetActorLocation().Y - Floor->GetActorLocation().Y;
    const float ExpectedY = 90 - 12 - GetCapsuleComponent()->GetScaledCapsuleRadius();
    const float Travel = GetActorLocation().Y - ConstructionMovementStartY;
    const auto* Base = Cast<UPrimitiveComponent>(GetMovementBaseObject());
    const bool bOnFloor = Base && Base->GetOwner() == Floor && GetCharacterMovement()->IsMovingOnGround();
    if (!bConstructionWallLogged)
    {
        const bool bPassed = bOnFloor && Travel > 50 && FMath::Abs(Y - ExpectedY) < 3;
        UE_LOG(LogTemp, Display, TEXT("Construction movement: Passed=%d Authority=%d Local=%d Player=%d Floor=%s Wall=%s Grounded=%d Travel=%.2f Y=%.2f Expected=%.2f"),
            bPassed, HasAuthority(), IsLocallyControlled(), GetPlayerState()->GetPlayerId(),
            *Floor->GetConstructionId(), *Wall->GetConstructionId(), bOnFloor, Travel, Y, ExpectedY);
        bConstructionWallLogged = true;
    }
    if (ConstructionMovementElapsed < 8) return;
    if (IsLocallyControlled() && !bConstructionRoofJumpRequested)
    {
        Jump();
        bConstructionRoofJumpRequested = true;
    }
    if (IsLocallyControlled() && ConstructionMovementElapsed > 8.2f) StopJumping();
    if (ConstructionMovementElapsed < 11) return;
    const float CeilingZ = Roof->GetActorLocation().Z - Floor->GetActorLocation().Z
        - AKalmalaConstructionActor::GetCollisionExtent(TEXT("RoofKit")).Z
        - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const bool bRoofPassed = bOnFloor && bConstructionRoofAirborne
        && ConstructionRoofPeakZ >= CeilingZ - 10 && ConstructionRoofPeakZ <= CeilingZ + 1;
    UE_LOG(LogTemp, Display, TEXT("Construction roof: Passed=%d Authority=%d Local=%d Player=%d Roof=%s Airborne=%d Landed=%d Peak=%.2f Ceiling=%.2f"),
        bRoofPassed, HasAuthority(), IsLocallyControlled(), GetPlayerState()->GetPlayerId(),
        *Roof->GetConstructionId(), bConstructionRoofAirborne, bOnFloor, ConstructionRoofPeakZ, CeilingZ);
    bConstructionMovementFinished = true;
#endif
}
