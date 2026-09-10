#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaConstructionActor.generated.h"

class UBoxComponent;

/** Session-lifetime replicated construction placeholder; persistence and piece-specific geometry follow later. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaConstructionActor : public AActor
{
    GENERATED_BODY()
public:
    AKalmalaConstructionActor();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void InitializeFromServer(FName InKit, const FString& InConstructionId);
    FName GetConstructionKit() const { return ConstructionKit; }
    const FString& GetConstructionId() const { return ConstructionId; }
private:
    UFUNCTION() void OnRep_ConstructionState();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(ReplicatedUsing=OnRep_ConstructionState) FName ConstructionKit;
    UPROPERTY(ReplicatedUsing=OnRep_ConstructionState) FString ConstructionId;
    FString LastLoggedReplicationId;
};
