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
    }
}

bool AKalmalaHarvestNode::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    return HasAuthority() && !bHarvested && IsValid(Interactor)
        && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(250.0f);
}

void AKalmalaHarvestNode::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    if (CanInteract_Implementation(Interactor))
    {
        bHarvested = true;
        ApplyHarvestedState();
        ForceNetUpdate();
    }
}

void AKalmalaHarvestNode::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaHarvestNode, bHarvested);
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
