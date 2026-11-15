#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaPlayerModelComponent.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "ProceduralMeshComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

void AKalmalaCharacter::VerifyPlayerControls(const float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!bControlsTestEnabled) return;
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldBoundaryTest")))
    {
        VerifyWorldBoundaryMovement(DeltaSeconds);
        return;
    }
    auto* Movement = CastChecked<UKalmalaCharacterMovementComponent>(GetCharacterMovement());
    const bool bWetTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaWetStaminaTest"));
    if (bWetTest && HasAuthority()) Statuses->ApplyWetFromServer();
    if (bWetTest && !bWetStaminaReport && Movement->IsSprintRequested() && Movement->IsMovingOnGround()
        && Movement->GetStamina() < 99.0f && Statuses->GetModifiers().Movement < 1.0f)
    {
        const float Before = Movement->GetStamina();
        if (!HasAuthority()) Movement->AdvanceStaminaFromServer(0.25f);
        const bool bRejected = HasAuthority() || Before == Movement->GetStamina();
        const bool bPassed = bRejected && FMath::IsNearlyEqual(Movement->GetMaxSpeed(), Movement->MaxWalkSpeed * 1.5f * UKalmalaPlayerStatusComponent::WetMovementMultiplier);
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
    const auto RequestMovementLightingScreenshot = [](const TCHAR* ArgumentName, const TCHAR* Phase)
    {
        FString ScreenshotPath;
        if (FParse::Value(FCommandLine::Get(), ArgumentName, ScreenshotPath) && !ScreenshotPath.IsEmpty())
        {
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, false, false);
            UE_LOG(LogTemp, Display, TEXT("Lighting movement capture requested: Phase=%s Output=%s."), Phase, *ScreenshotPath);
        }
    };
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
        RequestMovementLightingScreenshot(TEXT("KalmalaMovementLightingStart="), TEXT("Start"));
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
        // Nine body parts plus a fixed three-part, non-colliding held-tool pool.
        constexpr int32 ExpectedModelPartCount = 12;
        const bool bCosmeticOnly = Meshes.Num() == ExpectedModelPartCount && Meshes.ContainsByPredicate([](const auto* Mesh) { return Mesh->GetNumSections() > 0; })
            && !Meshes.ContainsByPredicate([](const auto* Mesh) { return Mesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision; });
        const bool bPassed = bControlsTestLocalJumpObserved && !Movement->IsSprintRequested()
            && FMath::IsNearlyEqual(Movement->GetMaxSpeed(), Movement->MaxWalkSpeed * Statuses->GetModifiers().Movement)
            && PlayerModel->GetPartCount() == ExpectedModelPartCount && bCosmeticOnly;
        UE_LOG(LogTemp, Display, TEXT("Controls local result: %s Authority=%d Parts=%d Jump=%d"),
            bPassed ? TEXT("PASS") : TEXT("FAIL"), HasAuthority(), PlayerModel->GetPartCount(), bControlsTestLocalJumpObserved);
        RequestMovementLightingScreenshot(TEXT("KalmalaMovementLightingEnd="), TEXT("End"));
        ControlsTestStage = 4;
    }
#endif
}

void AKalmalaCharacter::VerifyWorldBoundaryMovement(const float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    UWorld* World = GetWorld();
    AKalmalaWorldGenerationGameState* State = World != nullptr
        ? World->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    APlayerState* PlayerState = GetPlayerState();
    UKalmalaCharacterMovementComponent* Movement = Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement());
    if (World == nullptr || State == nullptr || PlayerState == nullptr || Movement == nullptr
        || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.0f) return;

    const FString PlayerName = PlayerState->GetPlayerName();
    const bool bBoundaryOwner = PlayerName == TEXT("KalmalaBoundaryOwner");
    const bool bBoundaryObserver = PlayerName == TEXT("KalmalaBoundaryObserver");
    if (!bBoundaryOwner && !bBoundaryObserver) return;

    const double Limit = FKalmalaWorldBounds::Radius - GetSimpleCollisionRadius() - 2.0;
    const auto CaptureBoundaryContact = [this, Movement, Limit]()
    {
        if (bWorldBoundaryContactObserved) return;
        const FVector Location = GetActorLocation();
        const FVector2D Position(Location);
        if (Position.Size() < Limit - 1.0) return;

        const FVector2D Radial = Position.GetSafeNormal();
        const FVector2D Tangent(-Radial.Y, Radial.X);
        const FVector2D HorizontalVelocity(Movement->Velocity);
        WorldBoundaryContactLocation = Location;
        WorldBoundaryContactRadius = Position.Size();
        WorldBoundaryContactRadialVelocity = FVector2D::DotProduct(HorizontalVelocity, Radial);
        WorldBoundaryContactTangentialVelocity = FVector2D::DotProduct(HorizontalVelocity, Tangent);
        WorldBoundaryContactVerticalVelocity = Movement->Velocity.Z;
        bWorldBoundaryContactObserved = true;
        UE_LOG(LogTemp, Display,
            TEXT("World boundary contact: Role=%s X=%.1f Y=%.1f Z=%.1f Radius=%.1f RadialVelocity=%.1f TangentialVelocity=%.1f VerticalVelocity=%.1f"),
            HasAuthority() ? TEXT("Server") : TEXT("Owner"), Location.X, Location.Y, Location.Z,
            WorldBoundaryContactRadius, WorldBoundaryContactRadialVelocity,
            WorldBoundaryContactTangentialVelocity, WorldBoundaryContactVerticalVelocity);
    };

    if (HasAuthority() && bBoundaryOwner && WorldBoundaryTestStage == 0)
    {
        bool bObserverReady = false;
        for (TActorIterator<AKalmalaCharacter> It(World); It; ++It)
        {
            const APlayerState* OtherPlayerState = It->GetPlayerState();
            if (*It != this && OtherPlayerState != nullptr
                && OtherPlayerState->GetPlayerName() == TEXT("KalmalaBoundaryObserver"))
            {
                bObserverReady = true;
                break;
            }
        }
        if (!bObserverReady) return;

        const FKalmalaWorldGenerationConfig Config = State->GetWorldGenerationConfig();
        FVector2D Outward(1.0, 0.0);
        float SurfaceHeight = -TNumericLimits<float>::Max();
        for (int32 Index = 0; Index < 72; ++Index)
        {
            const double Angle = 2.0 * PI * static_cast<double>(Index) / 72.0;
            const FVector2D CandidateDirection(FMath::Cos(Angle), FMath::Sin(Angle));
            const FVector2D CandidatePosition = CandidateDirection * (Limit - 5.0);
            const float CandidateHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, CandidatePosition);
            if (CandidateHeight > SurfaceHeight)
            {
                SurfaceHeight = CandidateHeight;
                Outward = CandidateDirection;
            }
        }

        const FVector2D StagePosition = Outward * (Limit - 5.0);
        const float StageHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, StagePosition);
        const FRotator Facing(0.0f, FMath::RadiansToDegrees(FMath::Atan2(Outward.Y, Outward.X)), 0.0f);
        SetActorLocation(FVector(StagePosition, StageHeight + GetSimpleCollisionHalfHeight() + 5000.0f),
            false, nullptr, ETeleportType::TeleportPhysics);
        SetActorRotation(Facing);
        if (AController* Controller = GetController()) Controller->SetControlRotation(Facing);
        Movement->StopMovementImmediately();
        Movement->SetMovementMode(MOVE_Falling);
        WorldBoundaryTestStartedAt = World->GetTimeSeconds();
        WorldBoundaryTestStage = 1;
        ForceNetUpdate();
        UE_LOG(LogTemp, Display,
            TEXT("World boundary fixture staged owner: X=%.1f Y=%.1f Z=%.1f Limit=%.1f"),
            StagePosition.X, StagePosition.Y, GetActorLocation().Z, Limit);
        return;
    }

    if (HasAuthority() && bBoundaryOwner && WorldBoundaryTestStage == 1)
    {
        CaptureBoundaryContact();
        if (!bWorldBoundaryResultReported && World->GetTimeSeconds() - WorldBoundaryTestStartedAt >= 2.0)
        {
            const FVector Location = GetActorLocation();
            const double Radius = FVector2D(Location).Size();
            const bool bContactPassed = bWorldBoundaryContactObserved
                && WorldBoundaryContactRadius <= Limit + 1.0
                && FMath::Abs(WorldBoundaryContactRadialVelocity) <= 1.0
                && FMath::Abs(WorldBoundaryContactTangentialVelocity) > 1.0
                && FMath::Abs(WorldBoundaryContactVerticalVelocity) > 1.0;
            const bool bPassed = bContactPassed && Radius <= Limit + 1.0;
            UE_LOG(LogTemp, Display,
                TEXT("World boundary result: Role=Server Passed=%d X=%.1f Y=%.1f Z=%.1f Radius=%.1f Limit=%.1f ContactX=%.1f ContactY=%.1f ContactZ=%.1f ContactRadial=%.1f ContactTangential=%.1f ContactVertical=%.1f"),
                bPassed ? 1 : 0, Location.X, Location.Y, Location.Z, Radius, Limit,
                WorldBoundaryContactLocation.X, WorldBoundaryContactLocation.Y, WorldBoundaryContactLocation.Z,
                WorldBoundaryContactRadialVelocity, WorldBoundaryContactTangentialVelocity,
                WorldBoundaryContactVerticalVelocity);
            bWorldBoundaryResultReported = true;
            WorldBoundaryTestStage = 3;
        }
        return;
    }

    if (!HasAuthority() && IsLocallyControlled() && bBoundaryOwner)
    {
        if (WorldBoundaryTestStage == 0)
        {
            if (FVector2D(GetActorLocation()).Size() < Limit - 1000.0) return;
            const FVector2D Outward = FVector2D(GetActorLocation()).GetSafeNormal();
            const FRotator Facing(0.0f,
                FMath::RadiansToDegrees(FMath::Atan2(Outward.Y, Outward.X)), 0.0f);
            if (AController* Controller = GetController()) Controller->SetControlRotation(Facing);
            WorldBoundaryTestStartedAt = World->GetTimeSeconds();
            WorldBoundaryTestStage = 1;
        }

        if (WorldBoundaryTestStage == 1)
        {
            MoveForward(1.0f);
            MoveRight(1.0f);
            CaptureBoundaryContact();
            if (World->GetTimeSeconds() - WorldBoundaryTestStartedAt >= 1.0)
            {
                MoveForward(0.0f);
                MoveRight(0.0f);
                WorldBoundaryTestStage = 2;
            }
        }

        if (WorldBoundaryTestStage == 2 && !bWorldBoundaryResultReported
            && World->GetTimeSeconds() - WorldBoundaryTestStartedAt >= 2.0)
        {
            const FVector Location = GetActorLocation();
            const double Radius = FVector2D(Location).Size();
            const bool bContactPassed = bWorldBoundaryContactObserved
                && WorldBoundaryContactRadius <= Limit + 1.0
                && FMath::Abs(WorldBoundaryContactRadialVelocity) <= 1.0
                && FMath::Abs(WorldBoundaryContactTangentialVelocity) > 1.0
                && FMath::Abs(WorldBoundaryContactVerticalVelocity) > 1.0;
            const bool bPassed = bContactPassed && Radius <= Limit + 1.0;
            UE_LOG(LogTemp, Display,
                TEXT("World boundary result: Role=Owner Passed=%d X=%.1f Y=%.1f Z=%.1f Radius=%.1f Limit=%.1f ContactX=%.1f ContactY=%.1f ContactZ=%.1f ContactRadial=%.1f ContactTangential=%.1f ContactVertical=%.1f"),
                bPassed ? 1 : 0, Location.X, Location.Y, Location.Z, Radius, Limit,
                WorldBoundaryContactLocation.X, WorldBoundaryContactLocation.Y, WorldBoundaryContactLocation.Z,
                WorldBoundaryContactRadialVelocity, WorldBoundaryContactTangentialVelocity,
                WorldBoundaryContactVerticalVelocity);
            bWorldBoundaryResultReported = true;
            WorldBoundaryTestStage = 3;
        }
        return;
    }

    if (!HasAuthority() && IsLocallyControlled() && bBoundaryObserver)
    {
        AKalmalaCharacter* OwnerCharacter = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(World); It; ++It)
        {
            const APlayerState* OtherPlayerState = It->GetPlayerState();
            if (*It != this && OtherPlayerState != nullptr
                && OtherPlayerState->GetPlayerName() == TEXT("KalmalaBoundaryOwner"))
            {
                OwnerCharacter = *It;
                break;
            }
        }
        if (OwnerCharacter == nullptr) return;

        if (WorldBoundaryTestStage == 0)
        {
            if (FVector2D(OwnerCharacter->GetActorLocation()).Size() < Limit - 1000.0) return;
            WorldBoundaryTestStartedAt = World->GetTimeSeconds();
            WorldBoundaryTestStage = 1;
        }

        if (WorldBoundaryTestStage == 1 && !bWorldBoundaryResultReported
            && World->GetTimeSeconds() - WorldBoundaryTestStartedAt >= 2.0)
        {
            const FVector Location = OwnerCharacter->GetActorLocation();
            const double Radius = FVector2D(Location).Size();
            const bool bPassed = Radius <= Limit + 5.0;
            UE_LOG(LogTemp, Display,
                TEXT("World boundary result: Role=Observer Passed=%d X=%.1f Y=%.1f Z=%.1f Radius=%.1f Limit=%.1f"),
                bPassed ? 1 : 0, Location.X, Location.Y, Location.Z, Radius, Limit);
            bWorldBoundaryResultReported = true;
            WorldBoundaryTestStage = 3;
        }
    }
#endif
}
