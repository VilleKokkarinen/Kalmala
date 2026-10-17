#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaDamageProfile.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaHarvestNode.generated.h"

struct FKalmalaWorldGenerationConfig;
struct FKalmalaForestTreeDescriptor;

class USphereComponent;
class UMaterialInterface;
class UProceduralMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaHarvestNodeHarvested, const FString& /* PersistentSpawnId */);
DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaM9ResourceDepleted, const FString& /* StableResourceId */);

/** Server-owned one-use material grant; accepted depletion retains the stable sparse-save ID. */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaHarvestNode : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()

public:
    AKalmalaHarvestNode();

    /** Reconstructs a forest candidate on the server; no client descriptor or reward input. */
    bool InitializeForestServer(const FKalmalaWorldGenerationConfig& Config,
        const FVector2D& PatchCenter, int32 CandidateIndex);

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    void InitializeDiscoveryServer(const FString& InPersistentSpawnId, const FVector& InLocation);
    static bool IsHarvestAllowed(bool bServerAuthority, bool bAlreadyHarvested, const FVector& InteractorLocation, const FVector& NodeLocation, float MaximumDistance = 250.0f);
    const FString& GetPersistentSpawnId() const { return PersistentSpawnId; }
    FString GetM9ResourceDepletionId() const;
    FName GetGatheringSourceId() const { return GatheringSourceId; }
    FName GetGatheringPresentationId() const;
    FName GetHarvestItemId() const;
    bool IsHarvested() const { return bHarvested; }
    bool IsTreeResource() const;
    bool IsFelledTrunk() const { return TreeStage == 1; }
    bool IsTreeFalling() const { return TreeStage == 3; }
    bool IsWoodDrop() const { return TreeStage == 2; }
    static constexpr float TreeFallSeconds = 1.2f;
    /** Uses synchronized server time, so newly relevant peers resume the same fall. */
    float GetTreeFallProgress() const;
    static float SampleTreeFallProgress(double StartedAt, double ServerTime);
#if !UE_BUILD_SHIPPING
    const FVector4& GetForestDimensionsForVerification() const { return ForestDimensions; }
#endif
    bool CanReceiveAxeAttack() const { return IsTreeResource() && !bHarvested && TreeStage < 2; }
    bool ApplyAxeDamageFromServer(AKalmalaCharacter* Interactor, const FKalmalaDamageChannels& Damage);
    static bool AdvanceTreeHit(uint8 Stage, float Health, float Damage, uint8& OutStage, float& OutHealth);

    bool InteractWithToolIntentFromServer(AKalmalaCharacter* Interactor, float TraceDistance, float MaximumRange,
        FName ClientItemId, uint8 ClientAction);
    FKalmalaHarvestNodeHarvested OnHarvested;
    /** Session-only M9 depletion handoff; it deliberately does not use the M7 save callback. */
    FKalmalaM9ResourceDepleted OnM9ResourceDepleted;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    friend class AKalmalaCharacter;
    friend class AKalmalaGameMode;
    friend class UKalmalaInventoryComponent;

    /** Only the server patch owner may supply an already generated descriptor. */
    bool InitializeGeneratedForestServer(const FKalmalaForestTreeDescriptor& Tree);

    void BuildSourcePresentation();
    void ApplyHarvestedState();
    void CommitHarvestedStateFromServer();
    void SettleTreeFromServer();
    FTimerHandle TreeFallTimer;

    UPROPERTY(VisibleAnywhere, Category = "Harvest")
    TObjectPtr<USphereComponent> Collision;

    /** 0 standing, 1 settled trunk, 2 wood drop, 3 falling (rejects hits). Session-only. */
    UPROPERTY(ReplicatedUsing = OnRep_TreeStage)
    uint8 TreeStage = 0;

    UPROPERTY(Replicated)
    float TreeHealth = 100.0f;

    UPROPERTY(Replicated)
    double TreeFallStartedAt = 0.0;

    /** Server-reconstructed forest radius/height/canopy radius/height; zero selects legacy node geometry. */
    UPROPERTY(ReplicatedUsing = OnRep_TreeStage)
    FVector4 ForestDimensions = FVector4(0, 0, 0, 0);

    UFUNCTION()
    void OnRep_TreeStage();

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
