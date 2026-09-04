#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaHarvestNode.generated.h"

class USphereComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaHarvestNodeHarvested, const FString& /* PersistentSpawnId */);

/** Server-owned, one-use generated harvest node. Rewards and persistence follow in later increments. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaHarvestNode : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()

public:
    AKalmalaHarvestNode();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    static bool IsHarvestAllowed(bool bServerAuthority, bool bAlreadyHarvested, const FVector& InteractorLocation, const FVector& NodeLocation, float MaximumDistance = 250.0f);
    const FString& GetPersistentSpawnId() const { return PersistentSpawnId; }
    FKalmalaHarvestNodeHarvested OnHarvested;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void ApplyHarvestedState();

    UPROPERTY(VisibleAnywhere, Category = "Harvest")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(ReplicatedUsing = OnRep_Harvested, VisibleAnywhere, Category = "Harvest")
    bool bHarvested = false;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Harvest")
    FString PersistentSpawnId;

    UFUNCTION()
    void OnRep_Harvested();
};
