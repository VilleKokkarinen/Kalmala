#include "KalmalaWorldPopulationMarker.h"

#include "Net/UnrealNetwork.h"

AKalmalaWorldPopulationMarker::AKalmalaWorldPopulationMarker()
{
    bReplicates = true;
    SetReplicateMovement(false);
    SetActorEnableCollision(false);
}

void AKalmalaWorldPopulationMarker::InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn)
{
    if (!HasAuthority())
    {
        return;
    }

    PopulationKind = static_cast<uint8>(Spawn.Kind);
    PopulationSpatialKey = Spawn.SpatialKey;
    PopulationSpawnSeed = Spawn.SpawnSeed;
    SetActorLocation(Spawn.Location);
}

void AKalmalaWorldPopulationMarker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaWorldPopulationMarker, PopulationKind);
    DOREPLIFETIME(AKalmalaWorldPopulationMarker, PopulationSpatialKey);
    DOREPLIFETIME(AKalmalaWorldPopulationMarker, PopulationSpawnSeed);
}
