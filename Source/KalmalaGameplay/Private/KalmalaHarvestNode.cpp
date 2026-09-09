#include "KalmalaHarvestNode.h"

#include "Components/SphereComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "Misc/Crc.h"
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
    if (HasAuthority() && PersistentSpawnId.IsEmpty() && !bHarvested
        && Spawn.Kind == EKalmalaWorldPopulationKind::HarvestNode && !Spawn.Location.ContainsNaN())
    {
        SetActorLocation(Spawn.Location);
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
    }
}

void AKalmalaHarvestNode::InitializeDiscoveryServer(const FString& InPersistentSpawnId, const FVector& InLocation)
{
    if (HasAuthority() && PersistentSpawnId.IsEmpty() && !bHarvested
        && !InPersistentSpawnId.IsEmpty() && !InLocation.ContainsNaN())
    {
        SetActorLocation(InLocation);
        PersistentSpawnId = InPersistentSpawnId;
    }
}

bool AKalmalaHarvestNode::CanInteract_Implementation(AKalmalaCharacter* Interactor) const
{
    return IsValid(Interactor) && Interactor->HasAuthority() && Interactor->GetWorld() == GetWorld()
        && !PersistentSpawnId.IsEmpty()
        && IsHarvestAllowed(HasAuthority(), bHarvested, Interactor->GetActorLocation(), GetActorLocation());
}

bool AKalmalaHarvestNode::IsHarvestAllowed(const bool bServerAuthority, const bool bAlreadyHarvested, const FVector& InteractorLocation, const FVector& NodeLocation, const float MaximumDistance)
{
    return bServerAuthority && !bAlreadyHarvested && FMath::IsFinite(MaximumDistance) && MaximumDistance > 0.0f
        && !InteractorLocation.ContainsNaN() && !NodeLocation.ContainsNaN()
        && FVector::DistSquared(InteractorLocation, NodeLocation) <= FMath::Square(MaximumDistance);
}

void AKalmalaHarvestNode::Interact_Implementation(AKalmalaCharacter* Interactor)
{
    // No client-selected reward or quantity. Commit depletion only after the complete grant succeeds.
    if (!CanInteract_Implementation(Interactor)) return;
    auto* Inventory = Interactor->FindComponentByClass<UKalmalaInventoryComponent>();
    if (IsValid(Inventory) && Inventory->TryGrantFromServer(GetHarvestItemId(), 1))
    {
        bHarvested = true;
        ApplyHarvestedState();
        OnHarvested.Broadcast(PersistentSpawnId);
        ForceNetUpdate();
    }
}

FName AKalmalaHarvestNode::GetHarvestItemId() const
{
    if (PersistentSpawnId.IsEmpty()) return NAME_None;
    static const FName Materials[] = { TEXT("Wood"), TEXT("Stone"), TEXT("Fibre") };
    return Materials[FCrc::StrCrc32(*PersistentSpawnId) % UE_ARRAY_COUNT(Materials)];
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
