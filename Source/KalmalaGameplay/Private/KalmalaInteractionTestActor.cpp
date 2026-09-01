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
    return HasAuthority() && IsValid(Interactor)
        && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation())
            <= FMath::Square(MaximumInteractionDistance);
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
