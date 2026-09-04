#include "KalmalaHazardSpawn.h"

#include "Net/UnrealNetwork.h"

AKalmalaHazardSpawn::AKalmalaHazardSpawn()
{
    bReplicates = true;
    SetReplicateMovement(false);
    SetActorEnableCollision(false);
}

void AKalmalaHazardSpawn::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    if (HasAuthority())
    {
        SetActorLocation(Spawn.Location);
        PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
    }
}

bool AKalmalaHazardSpawn::IsDefeatAllowed(const bool bServerAuthority, const bool bAlreadyDefeated)
{
    return bServerAuthority && !bAlreadyDefeated;
}

bool AKalmalaHazardSpawn::DefeatServer()
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

void AKalmalaHazardSpawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaHazardSpawn, bDefeated);
    DOREPLIFETIME(AKalmalaHazardSpawn, PersistentSpawnId);
}

void AKalmalaHazardSpawn::OnRep_Defeated()
{
    ApplyDefeatedState();
}

void AKalmalaHazardSpawn::ApplyDefeatedState()
{
    SetActorHiddenInGame(bDefeated);
}
