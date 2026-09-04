#include "KalmalaWildlifeSpawn.h"

#include "Net/UnrealNetwork.h"

AKalmalaWildlifeSpawn::AKalmalaWildlifeSpawn()
{
    bReplicates = true;
    SetReplicateMovement(false);
    SetActorEnableCollision(false);
}

void AKalmalaWildlifeSpawn::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    if (HasAuthority())
    {
        SetActorLocation(Spawn.Location);
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
    }
}

bool AKalmalaWildlifeSpawn::IsDefeatAllowed(const bool bServerAuthority, const bool bAlreadyDefeated)
{
    return bServerAuthority && !bAlreadyDefeated;
}

bool AKalmalaWildlifeSpawn::DefeatServer()
{
    if (!IsDefeatAllowed(HasAuthority(), bDefeated))
    {
        return false;
    }

    bDefeated = true;
    ApplyDefeatedState();
    OnDefeated.Broadcast(PersistentSpawnId);
    ForceNetUpdate();
    return true;
}

void AKalmalaWildlifeSpawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, bDefeated);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, PersistentSpawnId);
}

void AKalmalaWildlifeSpawn::OnRep_Defeated()
{
    ApplyDefeatedState();
}

void AKalmalaWildlifeSpawn::ApplyDefeatedState()
{
    SetActorHiddenInGame(bDefeated);
}
