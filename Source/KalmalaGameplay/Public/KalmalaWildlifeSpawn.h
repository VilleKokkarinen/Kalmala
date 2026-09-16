#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWildlifeSpawn.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FKalmalaWildlifeSpawnDefeated, const FString& /* PersistentSpawnId */);

enum class EKalmalaWildlifeBehaviour : uint8
{
    Idle,
    Flee,
    Investigate,
    Return
};

/**
 * Minimal replicated, server-owned generated wildlife placeholder. It has no
 * combat or AI yet; it establishes the authority and persistence seam those
 * systems must use when they can defeat the spawn.
 */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaWildlifeSpawn : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaWildlifeSpawn();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    static bool IsDefeatAllowed(bool bServerAuthority, bool bAlreadyDefeated);
    bool DefeatServer();
    bool ApplyCombatDamageFromServer(float Damage, class AKalmalaCharacter* Attacker = nullptr);
    bool IsDefeated() const { return bDefeated; }
    float GetHealth() const { return Health; }
    const FString& GetPersistentSpawnId() const { return PersistentSpawnId; }
    static bool IsBehaviourTransitionAllowed(bool bServerAuthority, bool bAlreadyDefeated, EKalmalaWildlifeBehaviour From, EKalmalaWildlifeBehaviour To);
    FKalmalaWildlifeSpawnDefeated OnDefeated;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void ApplyDefeatedState();
    void BeginServerBehaviour(EKalmalaWildlifeBehaviour NextBehaviour, float DurationSeconds, const FVector& Destination);
    void AdvanceServerBehaviour(float DeltaSeconds);
    FVector GetDeterministicOffset(float Distance) const;
    void UpdateMirelingScavenge(float DeltaSeconds);
    void BuildMirelingPresentation();

    UPROPERTY(ReplicatedUsing = OnRep_Defeated)
    bool bDefeated = false;

    UPROPERTY(Replicated)
    float Health = 100.0f;

    UPROPERTY(Replicated)
    FString PersistentSpawnId;

    UFUNCTION()
    void OnRep_Defeated();

    FVector SpawnOrigin = FVector::ZeroVector;
    FVector BehaviourDestination = FVector::ZeroVector;
    float BehaviourSecondsRemaining = 0.0f;
    EKalmalaWildlifeBehaviour Behaviour = EKalmalaWildlifeBehaviour::Idle;
    TWeakObjectPtr<class AKalmalaCharacter> LastValidatedAttacker;
    float MirelingMeleeCooldown = 0.0f;
    UPROPERTY()
    TObjectPtr<class UProceduralMeshComponent> MirelingMesh;
    bool bClientCombatVerificationDefeatLogged = false;
};
