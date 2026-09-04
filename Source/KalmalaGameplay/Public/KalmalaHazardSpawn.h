#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaHazardSpawn.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaHazardSpawnDefeated, const FString& /* PersistentSpawnId */);

/** Minimal replicated, server-owned generated hazard placeholder with sparse defeat persistence. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaHazardSpawn : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaHazardSpawn();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    static bool IsDefeatAllowed(bool bServerAuthority, bool bAlreadyDefeated);
    bool DefeatServer();
    FKalmalaHazardSpawnDefeated OnDefeated;
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
