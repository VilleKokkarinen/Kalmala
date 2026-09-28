#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaOceanDiscoveryCatalogue.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
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
    bool InitializeOceanServer(const FKalmalaOceanDiscoveryDescriptor& InDescriptor);
    bool InitializeM9ExplorationRewardServer(const FKalmalaM9ExplorationRewardDescriptor& InDescriptor);
    const FKalmalaWorldDiscoveryDescriptor& GetDescriptor() const { return Descriptor; }
    const FKalmalaM9ExplorationRewardDescriptor& GetM9ExplorationRewardDescriptor() const { return M9ExplorationRewardDescriptor; }
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
private:
    void BuildDiscoveryPresentation();

    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
    UPROPERTY(ReplicatedUsing = OnRep_RareDiscoverySourceId, VisibleAnywhere, Category = "Discovery")
    FName RareDiscoverySourceId = NAME_None;
    UPROPERTY(ReplicatedUsing = OnRep_OceanPresentationId, VisibleAnywhere, Category = "Discovery")
    FName OceanPresentationId = NAME_None;
    UPROPERTY(ReplicatedUsing = OnRep_M9PresentationId, VisibleAnywhere, Category = "Discovery")
    FName M9PresentationId = NAME_None;
    UPROPERTY(VisibleAnywhere, Category = "Discovery") TObjectPtr<UProceduralMeshComponent> DiscoveryMesh;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> DiscoveryMaterial;

    UFUNCTION()
    void OnRep_RareDiscoverySourceId();
    UFUNCTION()
    void OnRep_OceanPresentationId();
    UFUNCTION()
    void OnRep_M9PresentationId();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FKalmalaWorldDiscoveryDescriptor Descriptor;
    FKalmalaOceanDiscoveryDescriptor OceanDescriptor;
    FKalmalaM9ExplorationRewardDescriptor M9ExplorationRewardDescriptor;
};
