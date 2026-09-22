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

UENUM()
enum class EKalmalaWildlifeArchetype : uint8
{
    Mireling,
    Boar,
    Deer
};

/**
 * Local presentation and movement profile derived from the server-selected
 * biome niche. It is never client-authored or replicated as a second identity.
 */
enum class EKalmalaWildlifeEcology : uint8
{
    Generalist,
    OpenGrazer,
    ShoreForager,
    CanopyBrowser,
    HummockScavenger,
    WindGrazer,
    RidgeForager
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
    bool ApplyDeerCallFromServer(const FVector& SourceLocation);
    bool IsMirelingBossRewardCandidate() const;
    bool IsDefeated() const { return bDefeated; }
    float GetHealth() const { return Health; }
    EKalmalaWildlifeArchetype GetArchetype() const { return Archetype; }
    FName GetCreatureNicheId() const { return CreatureNicheId; }
    EKalmalaWildlifeBehaviour GetBehaviour() const { return Behaviour; }
    const FString& GetPersistentSpawnId() const { return PersistentSpawnId; }
    static EKalmalaWildlifeEcology GetEcologyForNiche(FName CreatureNicheId);
    static float GetEcologicalFleeDistance(FName CreatureNicheId);
    // Server-owned repeat-hit interval keeps optional Mireling pressure readable.
    static constexpr float MirelingMeleeCooldownSeconds = 1.25f;
    static EKalmalaWildlifeArchetype GetArchetypeForSpawnSeed(uint64 SpawnSeed);
    static bool IsBoarChargeAllowed(bool bServerAuthority, bool bAlreadyDefeated, bool bAtRest, float DistanceToRestingArea);
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
    void UpdateBoarTerritory(float DeltaSeconds);
    void AlertNearbyDeerFromServer();
    void BuildArchetypePresentation();
    void GrantDefeatReward();

    UPROPERTY(ReplicatedUsing = OnRep_Defeated)
    bool bDefeated = false;

    UPROPERTY(Replicated)
    float Health = 100.0f;

    UPROPERTY(Replicated)
    FString PersistentSpawnId;

    UPROPERTY(ReplicatedUsing = OnRep_Archetype)
    EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;

    /** Server-selected ecological niche; archetype combat behaviour remains separately bounded. */
    UPROPERTY(ReplicatedUsing = OnRep_CreatureNicheId)
    FName CreatureNicheId = NAME_None;

    UFUNCTION()
    void OnRep_Defeated();

    UFUNCTION()
    void OnRep_Archetype();

    UFUNCTION()
    void OnRep_CreatureNicheId();

    FVector SpawnOrigin = FVector::ZeroVector;
    FVector BehaviourDestination = FVector::ZeroVector;
    float BehaviourSecondsRemaining = 0.0f;
    EKalmalaWildlifeBehaviour Behaviour = EKalmalaWildlifeBehaviour::Idle;
    TWeakObjectPtr<class AKalmalaCharacter> LastValidatedAttacker;
    TWeakObjectPtr<class AKalmalaCharacter> BoarChargeTarget;
    float MirelingMeleeCooldown = 0.0f;
    float BoarChargeSecondsRemaining = 0.0f;
    float BoarMeleeCooldown = 0.0f;
    UPROPERTY()
    TObjectPtr<class UProceduralMeshComponent> MirelingMesh;
    bool bClientCombatVerificationDefeatLogged = false;
};
