#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaHarvestNode.generated.h"

class USphereComponent;
class UMaterialInterface;
class UProceduralMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaHarvestNodeHarvested, const FString& /* PersistentSpawnId */);

/** Server-owned one-use material grant; accepted depletion retains the stable sparse-save ID. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaHarvestNode : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()

public:
    AKalmalaHarvestNode();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    void InitializeDiscoveryServer(const FString& InPersistentSpawnId, const FVector& InLocation);
    static bool IsHarvestAllowed(bool bServerAuthority, bool bAlreadyHarvested, const FVector& InteractorLocation, const FVector& NodeLocation, float MaximumDistance = 250.0f);
    const FString& GetPersistentSpawnId() const { return PersistentSpawnId; }
    FName GetGatheringSourceId() const { return GatheringSourceId; }
    FName GetGatheringPresentationId() const;
    FName GetHarvestItemId() const;
    FKalmalaHarvestNodeHarvested OnHarvested;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void BuildSourcePresentation();
    void ApplyHarvestedState();

    UPROPERTY(VisibleAnywhere, Category = "Harvest")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(ReplicatedUsing = OnRep_Harvested, VisibleAnywhere, Category = "Harvest")
    bool bHarvested = false;

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Harvest")
    FString PersistentSpawnId;

    /** Relevant peers may present the server-selected biome source; reward selection remains server-side. */
    UPROPERTY(ReplicatedUsing = OnRep_GatheringSourceId, VisibleAnywhere, Category = "Harvest")
    FName GatheringSourceId = NAME_None;

    UPROPERTY(VisibleAnywhere, Category = "Harvest")
    TObjectPtr<UProceduralMeshComponent> SourceMesh;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> BarkMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> RockMaterial;

    UFUNCTION()
    void OnRep_Harvested();

    UFUNCTION()
    void OnRep_GatheringSourceId();
};
