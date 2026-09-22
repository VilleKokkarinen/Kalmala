#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaDiscoveryActor.generated.h"
class USphereComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
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
    void BuildDiscoveryPresentation();

    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(ReplicatedUsing = OnRep_RareDiscoverySourceId, VisibleAnywhere, Category = "Discovery")
    FName RareDiscoverySourceId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category = "Discovery") TObjectPtr<UProceduralMeshComponent> DiscoveryMesh;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> DiscoveryMaterial;

    UFUNCTION()
    void OnRep_RareDiscoverySourceId();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FKalmalaWorldDiscoveryDescriptor Descriptor;
};
