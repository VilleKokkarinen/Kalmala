#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaHarvestNode.generated.h"

class USphereComponent;

/** Server-owned, one-use generated harvest node. Rewards and persistence follow in later increments. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaHarvestNode : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()

public:
    AKalmalaHarvestNode();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void ApplyHarvestedState();

    UPROPERTY(VisibleAnywhere, Category = "Harvest")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(ReplicatedUsing = OnRep_Harvested, VisibleAnywhere, Category = "Harvest")
    bool bHarvested = false;

    UFUNCTION()
    void OnRep_Harvested();
};
