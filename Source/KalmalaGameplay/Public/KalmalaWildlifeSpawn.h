#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWildlifeSpawn.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaWildlifeSpawnDefeated, const FString& /* PersistentSpawnId */);

/**
 * Minimal replicated, server-owned generated wildlife placeholder. It has no
 * combat or AI yet; it establishes the authority and persistence seam those
 * systems must use when they can defeat the spawn.
 */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaWildlifeSpawn : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaWildlifeSpawn();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    static bool IsDefeatAllowed(bool bServerAuthority, bool bAlreadyDefeated);
    bool DefeatServer();
    const FString& GetPersistentSpawnId() const { return PersistentSpawnId; }
    FKalmalaWildlifeSpawnDefeated OnDefeated;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void ApplyDefeatedState();

    UPROPERTY(ReplicatedUsing = OnRep_Defeated)
    bool bDefeated = false;

    UPROPERTY(Replicated)
    FString PersistentSpawnId;

    UFUNCTION()
    void OnRep_Defeated();
};
