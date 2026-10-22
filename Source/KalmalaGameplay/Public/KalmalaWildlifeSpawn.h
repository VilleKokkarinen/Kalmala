#pragma once

#include "CoreMinimal.h"
#include "KalmalaDamageProfile.h"
#include "GameFramework/Actor.h"
#include "KalmalaBiomeClassifier.h"
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
    Deer,
    Rat,
    Badger,
    MoonBear,
    DeathAdder,
    Viper,
    GiantSnake,
    MosquitoSwarm,
    PeatHusk,
    Wolf,
    Lynx,
    PolarBear,
    HornedGoat,
    ThunderSentinel,
    ReefMaw
};

enum class EKalmalaWildlifeAttackCue : uint8
{
    None,
    Telegraph,
    Lunge,
    Recovery
};

enum class EKalmalaRatAttackCue : uint8
{
    None,
    Telegraph,
    Recovery
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
 * Replicated, server-owned generated wildlife. Spawn selection, combat, reward
 * attribution and sparse depletion remain on the authority; clients receive
 * only the relevant actor state and attack cues.
 */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaWildlifeSpawn : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaWildlifeSpawn();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn, EKalmalaWildlifeArchetype InitialArchetype);
    static bool IsDefeatAllowed(bool bServerAuthority, bool bAlreadyDefeated);
    static bool IsDeerSinewDropSuccessful(float ServerRoll);
    bool DefeatServer();
    bool ApplyCombatDamageFromServer(const FKalmalaDamageChannels& Damage, class AKalmalaCharacter* Attacker = nullptr);
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
    static float GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype Archetype);
    static float GetArmorForArchetype(EKalmalaWildlifeArchetype Archetype);
    static float GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype Archetype);
    static FKalmalaDamageChannels GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype Archetype);
    static int32 GetDirectBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype Archetype);
    // Server-owned repeat-hit interval keeps optional Mireling pressure readable.
    static constexpr float MirelingMeleeCooldownSeconds = 1.25f;
    static constexpr float DeerCallInfluenceDurationSeconds = 180.0f;
    static constexpr float ViperTelegraphSeconds = 0.5f;
    static constexpr float ViperRecoverySeconds = 0.8f;
    static constexpr float GiantSnakeTelegraphSeconds = 1.0f;
    static constexpr float GiantSnakeRecoverySeconds = 1.2f;
    static constexpr float SnakeSpawnLeashCentimetres = 2500.0f;
    static constexpr float RatAttackTriggerCentimetres = 300.0f;
    static constexpr float RatBiteReachCentimetres = 100.0f;
    static constexpr float RatTelegraphSeconds = 0.5f;
    static constexpr float RatRecoverySeconds = 0.8f;
    static constexpr float RatSpawnLeashCentimetres = 2500.0f;
    static bool IsRatBiteInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static constexpr float BadgerAttackTriggerCentimetres = 300.0f;
    static constexpr float BadgerClawReachCentimetres = 180.0f;
    static constexpr float BadgerTelegraphSeconds = 0.5f;
    static constexpr float BadgerRecoverySeconds = 0.8f;
    static constexpr float BadgerSpawnLeashCentimetres = 2500.0f;
    static constexpr float BadgerApproachSpeedCentimetresPerSecond = 170.0f;
    static bool IsBadgerClawInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static EKalmalaWildlifeArchetype GetArchetypeForSpawnSeed(uint64 SpawnSeed);
    static bool TryGetSpawnArchetypeForBiome(uint64 SpawnSeed, EKalmalaBiome Biome, EKalmalaWildlifeArchetype& OutArchetype);
    static bool IsBoarChargeAllowed(bool bServerAuthority, bool bAlreadyDefeated, bool bAtRest, float DistanceToRestingArea);
    static bool IsBehaviourTransitionAllowed(bool bServerAuthority, bool bAlreadyDefeated, EKalmalaWildlifeBehaviour From, EKalmalaWildlifeBehaviour To);
    FKalmalaWildlifeSpawnDefeated OnDefeated;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaSeconds) override;

private:
    friend class AKalmalaArrowProjectile;
    bool ApplyArrowDamageFromServer(const FKalmalaDamageChannels& Damage, AKalmalaCharacter* Attacker);
    bool ApplyValidatedDamageFromServer(const FKalmalaDamageChannels& Damage, AKalmalaCharacter* Attacker, bool bMelee);

    void ApplyDefeatedState();
    void BeginServerBehaviour(EKalmalaWildlifeBehaviour NextBehaviour, float DurationSeconds, const FVector& Destination,
        bool bHoldDurationAtDestination = false);
    void AdvanceServerBehaviour(float DeltaSeconds);
    FVector GetDeterministicOffset(float Distance) const;
    void UpdateMirelingScavenge(float DeltaSeconds);
    void UpdateBoarTerritory(float DeltaSeconds);
    void UpdateRatAttack(float DeltaSeconds);
    void BeginRatAttack(class AKalmalaCharacter* Target);
    void SetRatAttackCue(EKalmalaRatAttackCue NextCue);
    bool IsRatAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyRatBite(class AKalmalaCharacter* Target);
    bool IsRatPositionDry(const FVector& Position) const;
    bool CanRatMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdateBadgerAttack(float DeltaSeconds);
    void BeginBadgerAttack(class AKalmalaCharacter* Target);
    void SetBadgerAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsBadgerAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyBadgerClaw(class AKalmalaCharacter* Target);
    bool IsBadgerPositionDry(const FVector& Position) const;
    bool CanBadgerMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdateSnakeAmbush(float DeltaSeconds);
    void BeginSnakeAttack(class AKalmalaCharacter* Target);
    void SetSnakeAttackCue(EKalmalaWildlifeAttackCue NextCue);
    void PlaySnakeHiss();
    bool IsSnakePositionDry(const FVector& Position) const;
    bool CanSnakeMoveAlong(const FVector& Start, const FVector& End) const;
    bool IsSnakeAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplySnakeBite(class AKalmalaCharacter* Target);
    void AlertNearbyDeerFromServer();
    void BuildArchetypePresentation();
    void GrantDefeatReward();

    UPROPERTY(ReplicatedUsing = OnRep_Defeated)
    bool bDefeated = false;

    UPROPERTY(Replicated)
    float Health = 75.0f;

    UPROPERTY(Replicated)
    FString PersistentSpawnId;

    UPROPERTY(ReplicatedUsing = OnRep_Archetype)
    EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;

    /** Server-selected ecological niche; archetype combat behaviour remains separately bounded. */
    UPROPERTY(ReplicatedUsing = OnRep_CreatureNicheId)
    FName CreatureNicheId = NAME_None;

    UPROPERTY(ReplicatedUsing = OnRep_SnakeAttackCue)
    uint8 SnakeAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_RatAttackCue)
    uint8 RatAttackCue = static_cast<uint8>(EKalmalaRatAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_BadgerAttackCue)
    uint8 BadgerAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UFUNCTION()
    void OnRep_Defeated();

    UFUNCTION()
    void OnRep_Archetype();

    UFUNCTION()
    void OnRep_CreatureNicheId();

    UFUNCTION()
    void OnRep_SnakeAttackCue();

    UFUNCTION()
    void OnRep_RatAttackCue();

    UFUNCTION()
    void OnRep_BadgerAttackCue();

    FVector SpawnOrigin = FVector::ZeroVector;
    FVector BehaviourDestination = FVector::ZeroVector;
    float BehaviourSecondsRemaining = 0.0f;
    bool bHoldBehaviourDurationAtDestination = false;
    bool bDefeatRewardProcessed = false;
    EKalmalaWildlifeBehaviour Behaviour = EKalmalaWildlifeBehaviour::Idle;
    TWeakObjectPtr<class AKalmalaCharacter> LastValidatedAttacker;
    TWeakObjectPtr<class AKalmalaCharacter> BoarChargeTarget;
    float MirelingMeleeCooldown = 0.0f;
    float BoarChargeSecondsRemaining = 0.0f;
    float BoarMeleeCooldown = 0.0f;
    float SnakeAttackSecondsRemaining = 0.0f;
    float SnakeLungeDistanceRemaining = 0.0f;
    FVector SnakeLungeDirection = FVector::ForwardVector;
    TWeakObjectPtr<class AKalmalaCharacter> SnakeAttackTarget;
    float RatAttackSecondsRemaining = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> RatAttackTarget;
    float BadgerAttackSecondsRemaining = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> BadgerAttackTarget;
    TArray<uint8> SnakeHissAudioData;
    UPROPERTY()
    TObjectPtr<class UProceduralMeshComponent> MirelingMesh;
    UPROPERTY()
    TObjectPtr<class UCapsuleComponent> SnakeBodyCollision;
    UPROPERTY(Transient)
    TObjectPtr<class USoundWaveProcedural> SnakeHissSound;
    bool bClientCombatVerificationDefeatLogged = false;
};
