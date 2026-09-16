#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaDiscoveryActor.generated.h"
class USphereComponent;
class AKalmalaCharacter;

/** Relevant, generic interaction marker; the server retains its canonical descriptor. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaDiscoveryActor : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()
public:
    AKalmalaDiscoveryActor();
    void InitializeServer(const FKalmalaWorldDiscoveryDescriptor& InDescriptor);
    const FKalmalaWorldDiscoveryDescriptor& GetDescriptor() const { return Descriptor; }
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
    FKalmalaWorldDiscoveryDescriptor Descriptor;
};
