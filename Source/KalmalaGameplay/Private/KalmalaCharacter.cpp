#include "KalmalaCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "KalmalaInteractable.h"

AKalmalaCharacter::AKalmalaCharacter()
{
    bReplicates = true;
    SetReplicateMovement(true);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
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
