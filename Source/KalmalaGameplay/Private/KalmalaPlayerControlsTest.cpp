#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaPlayerModelComponent.h"
#include "Components/InputComponent.h"
#include "ProceduralMeshComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AKalmalaCharacter::VerifyPlayerControls(const float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!bControlsTestEnabled) return;
    auto* Movement = CastChecked<UKalmalaCharacterMovementComponent>(GetCharacterMovement());
    const bool bWetTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaWetStaminaTest"));
    if (bWetTest && HasAuthority()) Statuses->ApplyWetFromServer();
    if (bWetTest && !bWetStaminaReport && Movement->IsSprintRequested() && Movement->IsMovingOnGround()
        && Movement->GetStamina() < 99.0f && Statuses->GetModifiers().Movement < 1.0f)
    {
        const float Before = Movement->GetStamina();
        if (!HasAuthority()) Movement->AdvanceStaminaFromServer(0.25f);
        const bool bRejected = HasAuthority() || Before == Movement->GetStamina();
        const bool bPassed = bRejected && FMath::IsNearlyEqual(Movement->GetMaxSpeed(), Movement->MaxWalkSpeed * 1.5f * 0.9f);
        UE_LOG(LogTemp, Display, TEXT("Wet stamina: Passed=%d Authority=%d Remote=%d Stamina=%.2f Speed=%.2f Cost=%.2f ClientMutationRejected=%d"),
            bPassed, HasAuthority(), !IsLocallyControlled(), Before, Movement->GetMaxSpeed(), Statuses->CalculateStaminaCost(10.0f), bRejected);
        bWetStaminaReport = true;
    }
    if (HasAuthority())
    {
        if (!bControlsTestSprintObserved && Movement->IsSprintRequested() && Movement->IsMovingOnGround())
        {
            bControlsTestSprintObserved = true;
            UE_LOG(LogTemp, Display, TEXT("Controls server sprint: Remote=%d Speed=%.1f Base=%.1f"), !IsLocallyControlled(), Movement->GetMaxSpeed(), Movement->MaxWalkSpeed);
        }
        if (!bControlsTestJumpObserved && Movement->IsFalling() && GetVelocity().Z > 100.0f)
        {
            bControlsTestJumpObserved = true;
            UE_LOG(LogTemp, Display, TEXT("Controls server jump: Remote=%d"), !IsLocallyControlled());
        }
        if (bControlsTestSprintObserved && !bControlsTestReleaseObserved && !Movement->IsSprintRequested())
        {
            bControlsTestReleaseObserved = true;
            UE_LOG(LogTemp, Display, TEXT("Controls server release: Remote=%d"), !IsLocallyControlled());
        }
    }
    if (!IsLocallyControlled() || !InputComponent || ControlsTestStage == 4) return;
    const auto Action = [this](FName Name, EInputEvent Event)
    {
        for (int32 I = 0; I < InputComponent->GetNumActionBindings(); ++I)
        {
            FInputActionBinding& Binding = InputComponent->GetActionBinding(I);
            if (Binding.GetActionName() == Name && Binding.KeyEvent == Event) Binding.ActionDelegate.Execute(FKey());
        }
    };
    if (ControlsTestStage == 0)
    {
        if (bWetTest && Statuses->GetModifiers().Movement >= 1.0f) return;
        if (!Movement->IsMovingOnGround() || GetWorld()->GetTimeSeconds() < 2.0f) return;
        Action(TEXT("Sprint"), IE_Pressed);
        ControlsTestStage = 1;
    }
    ControlsTestElapsed += DeltaSeconds;
    if (ControlsTestElapsed < 2.0f) MoveForward(1.0f);
    if (ControlsTestStage == 1 && ControlsTestElapsed >= 0.5f)
    {
        Action(TEXT("Jump"), IE_Pressed);
        ControlsTestStage = 2;
    }
    if (ControlsTestStage == 2)
    {
        bControlsTestLocalJumpObserved |= Movement->IsFalling() && GetVelocity().Z > 100.0f;
        if (ControlsTestElapsed >= 1.0f)
        {
            Action(TEXT("Jump"), IE_Released);
            Action(TEXT("Sprint"), IE_Released);
            ControlsTestStage = 3;
        }
    }
    if (ControlsTestStage == 3 && ControlsTestElapsed >= 3.0f && Movement->IsMovingOnGround())
    {
        TArray<UProceduralMeshComponent*> Meshes;
        GetComponents(Meshes);
        const bool bCosmeticOnly = Meshes.Num() == 9 && Meshes.ContainsByPredicate([](const auto* Mesh) { return Mesh->GetNumSections() > 0; })
            && !Meshes.ContainsByPredicate([](const auto* Mesh) { return Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision; });
        const bool bPassed = bControlsTestLocalJumpObserved && !Movement->IsSprintRequested()
            && FMath::IsNearlyEqual(Movement->GetMaxSpeed(), Movement->MaxWalkSpeed * Statuses->GetModifiers().Movement)
            && PlayerModel->GetPartCount() == 9 && bCosmeticOnly;
        UE_LOG(LogTemp, Display, TEXT("Controls local result: %s Authority=%d Parts=%d Jump=%d"),
            bPassed ? TEXT("PASS") : TEXT("FAIL"), HasAuthority(), PlayerModel->GetPartCount(), bControlsTestLocalJumpObserved);
        ControlsTestStage = 4;
    }
#endif
}
