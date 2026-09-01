#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaInteractionTestActor.generated.h"

class USphereComponent;

/** A replicated, deliberately minimal interaction target for M1 host/client testing. */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaInteractionTestActor : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()

public:
    AKalmalaInteractionTestActor();

    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "Interaction")
    int32 GetInteractionCount() const { return InteractionCount; }

private:
    UPROPERTY(VisibleAnywhere, Category = "Interaction")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Interaction")
    int32 InteractionCount = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (ClampMin = "1.0"))
    float MaximumInteractionDistance = 250.0f;
};
