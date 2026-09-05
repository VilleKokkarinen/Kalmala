#include "KalmalaCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "KalmalaInteractable.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

AKalmalaCharacter::AKalmalaCharacter()
{
    bReplicates = true;
    SetReplicateMovement(true);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    BaselineMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.0f;
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
}

void AKalmalaCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

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
