#include "KalmalaDiscoveryActor.h"
#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"

AKalmalaDiscoveryActor::AKalmalaDiscoveryActor()
{
    bReplicates = true; SetReplicateMovement(false);
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision")); Collision->InitSphereRadius(55.0f); Collision->SetCollisionProfileName(TEXT("BlockAll")); RootComponent = Collision;
}
void AKalmalaDiscoveryActor::InitializeServer(const FKalmalaWorldDiscoveryDescriptor& InDescriptor)
{ if (HasAuthority() && Descriptor.DefinitionId.IsEmpty() && !InDescriptor.DefinitionId.IsEmpty() && !InDescriptor.Location.ContainsNaN()) { Descriptor = InDescriptor; SetActorLocation(InDescriptor.Location); } }
bool AKalmalaDiscoveryActor::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{ return HasAuthority() && IsValid(Interactor) && Interactor->HasAuthority() && Interactor->GetWorld() == GetWorld() && !Descriptor.DefinitionId.IsEmpty() && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(250.0f); }
void AKalmalaDiscoveryActor::Interact_Implementation(AKalmalaCharacter* Interactor)
{ if (!CanInteract_Implementation(Interactor)) return; if (AKalmalaGameMode* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>()) Mode->ClaimDiscovery(Interactor, Descriptor); }
