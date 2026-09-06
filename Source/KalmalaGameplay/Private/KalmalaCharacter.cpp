#include "KalmalaCharacter.h"
#include "GameFramework/PlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "KalmalaInteractable.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaPlayerModelComponent.h"
#include "Components/CapsuleComponent.h"

AKalmalaCharacter::AKalmalaCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UKalmalaCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
    bReplicates = true;
    SetReplicateMovement(true);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    BaselineMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
    GetCharacterMovement()->JumpZVelocity = 500.0f;
    GetCharacterMovement()->AirControl = 0.25f;
    JumpMaxCount = 1;

    PlayerModel = CreateDefaultSubobject<UKalmalaPlayerModelComponent>(TEXT("PlayerModel"));
    PlayerModel->SetupAttachment(RootComponent);
    PlayerModel->SetRelativeLocation(FVector(0, 0, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.0f;
    CameraBoom->TargetOffset = FVector(0, 0, 45.0f);
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

bool AKalmalaCharacter::IsExposureUpdateAllowed(const bool bServerAuthority)
{
    return bServerAuthority;
}

void AKalmalaCharacter::SetExposureStateFromServer(const FKalmalaExposureState& NewExposureState)
{
    if (!IsExposureUpdateAllowed(HasAuthority()))
    {
        return;
    }

    ExposureState.Wetness = FMath::Clamp(NewExposureState.Wetness, 0.0f, 100.0f);
    ExposureState.Warmth = FMath::Clamp(NewExposureState.Warmth, 0.0f, 100.0f);
    ExposureState.TravelSpeedMultiplier = FMath::Clamp(NewExposureState.TravelSpeedMultiplier, 0.68f, 1.0f);
    ApplyExposureTravelPenalty();
    ForceNetUpdate();
}

void AKalmalaCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaCharacter, ExposureState);
}

void AKalmalaCharacter::BeginPlay()
{
    Super::BeginPlay();
    bTraversalTelemetryEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaTraversalTest"));
    bExposureReplicationTelemetryEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaExposureReplicationTest"));
    TraversalStartLocation = GetActorLocation();
    bControlsTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaPlayerControlsTest"));
    bSwimmingTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaSwimmingTest"));
}

void AKalmalaCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    VerifyPlayerControls(DeltaSeconds);
    VerifySwimming(DeltaSeconds);

    if (IsLocallyControlled() && Controller && Controller->IsMoveInputIgnored())
    {
        StopSprint();
        StopJumping();
    }

    ConfigureTraversalTestTarget();
    if (!bTraversalTargetConfigured)
    {
        return;
    }

    const FVector2D RemainingOffset = TraversalTestTarget - FVector2D(GetActorLocation());
    if (RemainingOffset.SizeSquared() <= FMath::Square(180.0f))
    {
        if (!bTraversalArrivalLogged)
        {
            bTraversalArrivalLogged = true;
            UE_LOG(LogTemp, Display, TEXT("Traversal-test %s pawn %s reached the Shimmering Lakes target."), HasAuthority() ? TEXT("server") : TEXT("client"), *GetName());
        }
        return;
    }

    if (IsLocallyControlled())
    {
        GetCharacterMovement()->MaxWalkSpeed = 1800.0f;
        AddMovementInput(FVector(RemainingOffset.GetSafeNormal(), 0.0f), 1.0f, true);
    }
}

void AKalmalaCharacter::ConfigureTraversalTestTarget()
{
    if (!bTraversalTelemetryEnabled || bTraversalTargetConfigured || GetWorld() == nullptr)
    {
        return;
    }

    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr || !WorldGenerationState->GetWorldGenerationConfig().IsValid())
    {
        return;
    }

    const FKalmalaWorldGenerationConfig& Config = WorldGenerationState->GetWorldGenerationConfig();
    const FVector StartLocation = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation();
    const FVector2D StartPosition(StartLocation);
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Y = -12000; Y <= 12000; Y += 250)
    {
        for (int32 X = -12000; X <= 12000; X += 250)
        {
            const FVector2D Candidate = StartPosition + FVector2D(X, Y);
            if (FKalmalaShimmeringLakeSampler::IsWater(Config, Candidate))
            {
                const float DistanceSquared = FVector2D::DistSquared(StartPosition, Candidate);
                if (DistanceSquared < ClosestDistanceSquared)
                {
                    ClosestDistanceSquared = DistanceSquared;
                    TraversalTestTarget = Candidate;
                }
            }
        }
    }

    bTraversalTargetConfigured = ClosestDistanceSquared != TNumericLimits<float>::Max();
}

void AKalmalaCharacter::OnRep_ExposureState()
{
    ApplyExposureTravelPenalty();
    if (GetPlayerState() && FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampChoiceTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Camp choice client %s: Wetness=%.2f Warmth=%.2f Travel=%.2f."), *FString::FromInt(GetPlayerState()->GetPlayerId()), ExposureState.Wetness, ExposureState.Warmth, ExposureState.TravelSpeedMultiplier);
    }
    if (bExposureReplicationTelemetryEnabled)
    {
        UE_LOG(LogTemp, Display, TEXT("Exposure replication test client received state: Wetness=%.2f Warmth=%.2f TravelMultiplier=%.2f."), ExposureState.Wetness, ExposureState.Warmth, ExposureState.TravelSpeedMultiplier);
    }
}

void AKalmalaCharacter::ApplyExposureTravelPenalty()
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = BaselineMaxWalkSpeed * ExposureState.TravelSpeedMultiplier;
    }
}

void AKalmalaCharacter::OnRep_ReplicatedMovement()
{
    Super::OnRep_ReplicatedMovement();

    if (bTraversalTelemetryEnabled && !HasAuthority() && !bTraversalMovementLogged
        && FVector::DistSquared2D(TraversalStartLocation, GetActorLocation()) >= FMath::Square(3000.0f))
    {
        bTraversalMovementLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Traversal-test client observed replicated pawn movement of at least 3,000 units."));
    }
}

void AKalmalaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AKalmalaCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AKalmalaCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AKalmalaCharacter::RequestInteract);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AKalmalaCharacter::StartSprint);
    PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AKalmalaCharacter::StopSprint);
}

void AKalmalaCharacter::ConfigureSwimmingTestTarget()
{
    if (!bSwimmingTestEnabled || bSwimmingTargetConfigured || GetWorld() == nullptr) return;
    const AKalmalaWorldGenerationGameState* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (State == nullptr || !State->GetWorldGenerationConfig().IsValid()) return;
    const FVector2D Start(GetActorLocation());
    SwimmingTestStart = Start;
    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Y = -12000; Y <= 12000; Y += 250)
    {
        for (int32 X = -12000; X <= 12000; X += 250)
        {
            const FVector2D Candidate = Start + FVector2D(X, Y);
            if (FKalmalaOceanSampler::Sample(State->GetWorldGenerationConfig(), Candidate).WaterDepth >= 150.0f)
            {
                const float DistanceSquared = FVector2D::DistSquared(Start, Candidate);
                if (DistanceSquared < ClosestDistanceSquared) { ClosestDistanceSquared = DistanceSquared; SwimmingTestTarget = Candidate; }
            }
        }
    }
    bSwimmingTargetConfigured = ClosestDistanceSquared != TNumericLimits<float>::Max();
}

void AKalmalaCharacter::VerifySwimming(const float DeltaSeconds)
{
    ConfigureSwimmingTestTarget();
    if (!bSwimmingTargetConfigured) return;
    if (IsLocallyControlled() && !bSwimmingReturnLogged)
    {
        const FVector2D Goal = bSwimmingEntryLogged ? SwimmingTestStart : SwimmingTestTarget;
        const FVector2D Remaining = Goal - FVector2D(GetActorLocation());
        if (Remaining.SizeSquared() > FMath::Square(100.0f))
        {
            AddMovementInput(FVector(Remaining.GetSafeNormal(), 0.0f), 1.0f, true);
        }
    }
    if (Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->IsSwimmingInGeneratedOcean() && !bSwimmingEntryLogged)
    {
        bSwimmingEntryLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Swimming test %s entered generated ocean. Authority=%d."), IsLocallyControlled() ? TEXT("owner") : TEXT("replica"), HasAuthority() ? 1 : 0);
    }
    if (bSwimmingEntryLogged && !Cast<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->IsSwimmingInGeneratedOcean()
        && FVector2D::DistSquared(FVector2D(GetActorLocation()), SwimmingTestStart) <= FMath::Square(150.0f) && !bSwimmingReturnLogged)
    {
        bSwimmingReturnLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Swimming test %s returned to land. Authority=%d."), IsLocallyControlled() ? TEXT("owner") : TEXT("replica"), HasAuthority() ? 1 : 0);
    }
}

void AKalmalaCharacter::StartSprint()
{
    if (IsLocallyControlled() && Controller && !Controller->IsMoveInputIgnored())
    {
        CastChecked<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->SetSprintRequested(true);
    }
}

void AKalmalaCharacter::StopSprint()
{
    CastChecked<UKalmalaCharacterMovementComponent>(GetCharacterMovement())->SetSprintRequested(false);
}

void AKalmalaCharacter::MoveForward(const float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AKalmalaCharacter::MoveRight(const float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        const FRotator ControlRotation = Controller->GetControlRotation();
        const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
    }
}

void AKalmalaCharacter::RequestInteract()
{
    if (IsLocallyControlled())
    {
        ServerRequestInteract();
    }
}

void AKalmalaCharacter::ServerRequestInteract_Implementation()
{
    if (Controller == nullptr || GetWorld() == nullptr)
    {
        return;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(KalmalaInteraction), false, this);
    FHitResult Hit;
    const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * InteractionRange;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
    {
        return;
    }

    AActor* Target = Hit.GetActor();
    if (IsValid(Target) && Target->Implements<UKalmalaInteractable>()
        && IKalmalaInteractable::Execute_CanInteract(Target, this))
    {
        IKalmalaInteractable::Execute_Interact(Target, this);
    }
}
