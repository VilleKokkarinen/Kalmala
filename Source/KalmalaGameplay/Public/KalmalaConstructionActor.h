#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaConstructionActor.generated.h"

class UBoxComponent;
class UProceduralMeshComponent;

/** Server-owned replicated construction. Clients derive presentation/collision from the replicated kit only. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaConstructionActor : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()
public:
    AKalmalaConstructionActor();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void InitializeFromServer(FName InKit, const FString& InConstructionId);
    FName GetConstructionKit() const { return ConstructionKit; }
    const FString& GetConstructionId() const { return ConstructionId; }
    static bool IsShelterKit(FName KitId);
    static FVector GetCollisionExtent(FName KitId);
    /** Advisory on clients; every mutation separately requires server authority. */
    bool CanUse(const AKalmalaCharacter* Character) const;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Character) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Character) override;
private:
    UFUNCTION() void OnRep_ConstructionState();
    void ApplyConstructionKit();
    void BuildPiecePresentation();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> PieceMesh;
    UPROPERTY(ReplicatedUsing=OnRep_ConstructionState) FName ConstructionKit;
    UPROPERTY(ReplicatedUsing=OnRep_ConstructionState) FString ConstructionId;
    FString LastLoggedReplicationId;
};
