#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaConstructionActor.generated.h"

class UBoxComponent;
class UProceduralMeshComponent;
class USceneComponent;

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
        const FKalmalaSmelterState& InSmelter = FKalmalaSmelterState(),
        const FKalmalaGardenPlotState& InGardenPlot = FKalmalaGardenPlotState(),
        bool bInDoorOpen = false);
    FName GetConstructionKit() const { return ConstructionKit; }
    const FString& GetConstructionId() const { return ConstructionId; }
    float GetHealth() const { return Health; }
    const FKalmalaCharcoalKilnState& GetCharcoalKilnState() const { return CharcoalKiln; }
    bool ApplyCharcoalKilnStateFromServer(const FKalmalaCharcoalKilnState& State);
    const FKalmalaSmelterState& GetSmelterState() const { return Smelter; }
    bool ApplySmelterStateFromServer(const FKalmalaSmelterState& State);
    const FKalmalaGardenPlotState& GetGardenPlotState() const { return GardenPlot; }
    bool ApplyGardenPlotStateFromServer(const FKalmalaGardenPlotState& State);
    bool IsDoorOpen() const { return bDoorOpen; }
    bool ReplaceWallWithDoorFromServer();
    bool ToggleDoorFromServer(AKalmalaCharacter* Character, FString& Reason);
    bool HasUsablePlantingTrowel(const AKalmalaCharacter* Character) const;
    /** Trusted server weather input; roof eligibility is sampled internally. No RPC. */
    void AdvanceRainWearFromServer(float DeltaSeconds, float Precipitation);
    static constexpr float MaximumHealth = 100.0f;
    static constexpr float RainHealthFloor = 50.0f;
    static constexpr float RainWearPerSecond = 0.10f;
    static bool IsShelterKit(FName KitId);
    static bool IsCraftingStationKit(FName KitId);
    static bool IsStorageKit(FName KitId);
    static bool IsStructuralEndpointKit(FName KitId);
    static void GetStructuralEndpoints(const AKalmalaConstructionActor* Construction, TArray<FVector>& OutEndpoints);
    static FVector GetCollisionExtent(FName KitId);
    /** Advisory on clients; every mutation separately requires server authority. */
    bool CanUse(const AKalmalaCharacter* Character) const;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Character) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Character) override;
private:
    UFUNCTION() void OnRep_ConstructionState();
    UFUNCTION() void OnRep_DoorState();
    void ApplyConstructionKit();
    void ApplyDoorState();
    bool IsPawnInDoorway() const;
    void BuildPiecePresentation();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> PieceMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> DoorPivot;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> DoorLeafMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> DoorLeftFrameCollision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> DoorRightFrameCollision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> DoorHeaderCollision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> DoorLeafCollision;
    UPROPERTY(VisibleAnywhere) TArray<TObjectPtr<UBoxComponent>> StairsStepCollisions;
    UPROPERTY(ReplicatedUsing=OnRep_ConstructionState) FName ConstructionKit;
    UPROPERTY(ReplicatedUsing=OnRep_ConstructionState) FString ConstructionId;
    UPROPERTY(Replicated) float Health = MaximumHealth;
    UPROPERTY(Replicated) FKalmalaCharcoalKilnState CharcoalKiln;
    UPROPERTY(Replicated) FKalmalaSmelterState Smelter;
    UPROPERTY(Replicated) FKalmalaGardenPlotState GardenPlot;
    UPROPERTY(ReplicatedUsing=OnRep_DoorState) bool bDoorOpen = false;
    FString LastLoggedReplicationId;
};
