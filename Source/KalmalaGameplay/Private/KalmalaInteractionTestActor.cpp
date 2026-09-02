#include "KalmalaInteractionTestActor.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "Net/UnrealNetwork.h"

AKalmalaInteractionTestActor::AKalmalaInteractionTestActor()
{
    bReplicates = true;
    SetReplicateMovement(false);

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(50.0f);
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    RootComponent = Collision;
}

bool AKalmalaInteractionTestActor::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    return IsValid(Interactor)
        && IsInteractionAllowed(HasAuthority(), Interactor->GetActorLocation(), GetActorLocation(), MaximumInteractionDistance);
}

bool AKalmalaInteractionTestActor::IsInteractionAllowed(const bool bServerAuthority, const FVector& InteractorLocation, const FVector& TargetLocation, const float MaximumDistance)
{
    return bServerAuthority && MaximumDistance > 0.0f
        && FVector::DistSquared(InteractorLocation, TargetLocation) <= FMath::Square(MaximumDistance);
}

void AKalmalaInteractionTestActor::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    if (CanInteract_Implementation(Interactor))
    {
        ++InteractionCount;
        ForceNetUpdate();
    }
}

void AKalmalaInteractionTestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaInteractionTestActor, InteractionCount);
}
