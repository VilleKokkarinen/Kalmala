#include "KalmalaWildlifeSpawn.h"

#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"

AKalmalaWildlifeSpawn::AKalmalaWildlifeSpawn()
{
    USphereComponent* Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    Collision->InitSphereRadius(70.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    RootComponent = Collision;
    bReplicates = true;
    SetReplicateMovement(false);
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

bool AKalmalaWildlifeSpawn::ApplyCombatDamageFromServer(const float Damage)
{
    if (!HasAuthority() || bDefeated || !FMath::IsFinite(Damage) || Damage <= 0.0f || Damage > 100.0f) return false;
    Health = FMath::Clamp(Health - Damage, 0.0f, 100.0f);
    if (Health <= 0.0f) return DefeatServer();
    ForceNetUpdate();
    return true;
}

void AKalmalaWildlifeSpawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, bDefeated);
    DOREPLIFETIME(AKalmalaWildlifeSpawn, Health);
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
