#include "KalmalaHarvestNode.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "Net/UnrealNetwork.h"

AKalmalaHarvestNode::AKalmalaHarvestNode()
{
    bReplicates = true;
    SetReplicateMovement(false);
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(50.0f);
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    RootComponent = Collision;
}

void AKalmalaHarvestNode::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    if (HasAuthority())
    {
        SetActorLocation(Spawn.Location);
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
    }
}

bool AKalmalaHarvestNode::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    return IsValid(Interactor) && IsHarvestAllowed(HasAuthority(), bHarvested, Interactor->GetActorLocation(), GetActorLocation());
}

bool AKalmalaHarvestNode::IsHarvestAllowed(const bool bServerAuthority, const bool bAlreadyHarvested, const FVector& InteractorLocation, const FVector& NodeLocation, const float MaximumDistance)
{
    return bServerAuthority && !bAlreadyHarvested && MaximumDistance > 0.0f
        && FVector::DistSquared(InteractorLocation, NodeLocation) <= FMath::Square(MaximumDistance);
}

void AKalmalaHarvestNode::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    if (CanInteract_Implementation(Interactor))
    {
        bHarvested = true;
        ApplyHarvestedState();
        OnHarvested.Broadcast(PersistentSpawnId);
        ForceNetUpdate();
    }
}

void AKalmalaHarvestNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaHarvestNode, bHarvested);
    DOREPLIFETIME(AKalmalaHarvestNode, PersistentSpawnId);
}

void AKalmalaHarvestNode::OnRep_Harvested()
{
    ApplyHarvestedState();
}

void AKalmalaHarvestNode::ApplyHarvestedState()
{
    SetActorHiddenInGame(bHarvested);
    Collision->SetCollisionEnabled(bHarvested ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
}
