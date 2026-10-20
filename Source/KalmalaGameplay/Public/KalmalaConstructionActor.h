#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaConstructionSaveGame.h"
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
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    void InitializeFromServer(FName InKit, const FString& InConstructionId,
        const FKalmalaCharcoalKilnState& InCharcoalKiln = FKalmalaCharcoalKilnState(),
        const FKalmalaSmelterState& InSmelter = FKalmalaSmelterState());
    FName GetConstructionKit() const { return ConstructionKit; }
    const FString& GetConstructionId() const { return ConstructionId; }
    float GetHealth() const { return Health; }
    const FKalmalaCharcoalKilnState& GetCharcoalKilnState() const { return CharcoalKiln; }
    bool ApplyCharcoalKilnStateFromServer(const FKalmalaCharcoalKilnState& State);
    const FKalmalaSmelterState& GetSmelterState() const { return Smelter; }
    bool ApplySmelterStateFromServer(const FKalmalaSmelterState& State);
    /** Trusted server weather input; roof eligibility is sampled internally. No RPC. */
    void AdvanceRainWearFromServer(float DeltaSeconds, float Precipitation);
    static constexpr float MaximumHealth = 100.0f;
    static constexpr float RainHealthFloor = 50.0f;
    static constexpr float RainWearPerSecond = 0.10f;
    static bool IsShelterKit(FName KitId);
    static bool IsCraftingStationKit(FName KitId);
    static bool IsStorageKit(FName KitId);
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
    UPROPERTY(Replicated) float Health = MaximumHealth;
    UPROPERTY(Replicated) FKalmalaCharcoalKilnState CharcoalKiln;
    UPROPERTY(Replicated) FKalmalaSmelterState Smelter;
    FString LastLoggedReplicationId;
};
