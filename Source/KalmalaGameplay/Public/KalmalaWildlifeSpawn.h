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
    Retreat,
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
    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn, EKalmalaWildlifeArchetype InitialArchetype,
        int32 WolfPackMemberIndex = 0);
    static bool IsDefeatAllowed(bool bServerAuthority, bool bAlreadyDefeated);
    static bool IsDeerSinewDropSuccessful(float ServerRoll);
    bool DefeatServer();
    bool ApplyCombatDamageFromServer(const FKalmalaDamageChannels& Damage, class AKalmalaCharacter* Attacker = nullptr);
    bool ApplyCombatDamageFromServer(float Damage, class AKalmalaCharacter* Attacker = nullptr);
    static constexpr float FangArrowWeakPoisonDurationSeconds = 5.0f;
    float GetFangArrowPoisonRemainingSeconds() const { return FangArrowPoisonRemainingSeconds; }
    bool ApplyDeerCallFromServer(const FVector& SourceLocation);
    bool IsMirelingBossRewardCandidate() const;
    bool IsDefeated() const { return bDefeated; }
    float GetHealth() const { return Health; }
    EKalmalaWildlifeArchetype GetArchetype() const { return Archetype; }
    FName GetCreatureNicheId() const { return CreatureNicheId; }
    EKalmalaWildlifeBehaviour GetBehaviour() const { return Behaviour; }
    const FString& GetPersistentSpawnId() const { return PersistentSpawnId; }
#if !UE_BUILD_SHIPPING
    int32 GetMosquitoAcceptedBiteCountForVerification() const { return MosquitoAcceptedBiteCountForVerification; }
#endif
    static EKalmalaWildlifeEcology GetEcologyForNiche(FName CreatureNicheId);
    static float GetEcologicalFleeDistance(FName CreatureNicheId);
    static float GetMaximumHealthForArchetype(EKalmalaWildlifeArchetype Archetype);
    static float GetArmorForArchetype(EKalmalaWildlifeArchetype Archetype);
    static float GetBaseAttackDamageForArchetype(EKalmalaWildlifeArchetype Archetype);
    static FKalmalaDamageChannels GetBaseAttackChannelsForArchetype(EKalmalaWildlifeArchetype Archetype);
    static int32 GetBoneFragmentRewardQuantity(EKalmalaWildlifeArchetype Archetype);
    static int32 GetStoneRewardQuantityForArchetype(EKalmalaWildlifeArchetype Archetype);
    static constexpr float HardEnemyStartExclusionCentimetres = 15000.0f;
    static bool IsHardEnemySpawnAllowedByStart(bool bHardEnemy, bool bHasGeneratedPlayerStart,
        float DistanceCentimetres);
    // Server-owned repeat-hit interval keeps optional Mireling pressure readable.
    static constexpr float MirelingMeleeCooldownSeconds = 1.25f;
    static constexpr float DeerCallInfluenceDurationSeconds = 180.0f;
    static constexpr float ViperTelegraphSeconds = 0.5f;
    static constexpr float ViperRecoverySeconds = 0.8f;
    static constexpr float ViperWeakPoisonDurationSeconds = 5.0f;
    static constexpr float DeathAdderMajorPoisonDurationSeconds = 10.0f;
    static constexpr float MosquitoWeakPoisonDurationSeconds = 10.0f;
    static constexpr float MosquitoBiteDamagePerSecond = 25.0f;
    static constexpr float MosquitoBiteIntervalSeconds = 0.2f;
    static constexpr float MosquitoBiteReachCentimetres = 100.0f;
    static constexpr float MosquitoSpawnLeashCentimetres = 2500.0f;
    static constexpr float MosquitoApproachSeconds = 0.5f;
    static constexpr float MosquitoFlightHeightCentimetres = 150.0f;
    static constexpr float MosquitoFlightAmplitudeCentimetres = 95.0f;
    static constexpr float MosquitoFlightSpeedCentimetresPerSecond = 440.0f;
    static constexpr float MosquitoTorchAvoidanceCentimetres = 300.0f;
    static constexpr float MosquitoCampfireAvoidanceCentimetres = 500.0f;
    static constexpr float SnakeBiteReachCentimetres = 180.0f;
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
    static constexpr float WolfAttackTriggerCentimetres = 300.0f;
    static constexpr float WolfBiteReachCentimetres = 180.0f;
    static constexpr float WolfTelegraphSeconds = 0.5f;
    static constexpr float WolfLungeSeconds = 0.12f;
    static constexpr float WolfRecoverySeconds = 0.8f;
    static constexpr float WolfSpawnLeashCentimetres = 2500.0f;
    static constexpr float WolfApproachSpeedCentimetresPerSecond = 170.0f;
    static bool IsWolfBiteInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static constexpr float LynxAttackTriggerCentimetres = 500.0f;
    static constexpr float LynxLeapImpactReachCentimetres = 350.0f;
    static constexpr float LynxTelegraphSeconds = 0.5f;
    static constexpr float LynxLeapSeconds = 0.24f;
    static constexpr float LynxRecoverySeconds = 0.8f;
    static constexpr float LynxMissRecoverySeconds = 1.2f;
    static constexpr float LynxSpawnLeashCentimetres = 2500.0f;
    static constexpr float LynxApproachSpeedCentimetresPerSecond = 170.0f;
    static constexpr float LynxLeapSpeedCentimetresPerSecond = 1400.0f;
    static constexpr float LynxLeapLandingDistanceCentimetres = 110.0f;
    static bool IsLynxLeapInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static constexpr float PolarBearAttackTriggerCentimetres = 850.0f;
    static constexpr float PolarBearApproachSpeedCentimetresPerSecond = 170.0f;
    static constexpr float PolarBearChargeImpactReachCentimetres = 220.0f;
    static constexpr float PolarBearTelegraphSeconds = 1.0f;
    static constexpr float PolarBearChargeSeconds = 0.48f;
    static constexpr float PolarBearMaximumChargeCentimetres = 620.0f;
    static constexpr float PolarBearChargeSpeedCentimetresPerSecond = 1400.0f;
    static constexpr float PolarBearRetreatSeconds = 0.25f;
    static constexpr float PolarBearRetreatDistanceCentimetres = 220.0f;
    static constexpr float PolarBearRetreatSpeedCentimetresPerSecond = 880.0f;
    static constexpr float PolarBearRecoverySeconds = 1.2f;
    static constexpr float PolarBearSpawnLeashCentimetres = 2500.0f;
    static bool IsPolarBearChargeInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static constexpr float MoonBearAttackTriggerCentimetres = 300.0f;
    static constexpr float MoonBearPawReachCentimetres = 220.0f;
    static constexpr float MoonBearTelegraphSeconds = 1.0f;
    static constexpr float MoonBearRecoverySeconds = 1.2f;
    static constexpr float MoonBearSpawnLeashCentimetres = 2500.0f;
    static bool IsMoonBearPawInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static constexpr float HornedGoatAttackTriggerCentimetres = 300.0f;
    static constexpr float HornedGoatHornReachCentimetres = 180.0f;
    static constexpr float HornedGoatTelegraphSeconds = 0.5f;
    static constexpr float HornedGoatApproachSpeedCentimetresPerSecond = 170.0f;
    static constexpr float HornedGoatChargeSpeedCentimetresPerSecond = 300.0f;
    static constexpr float HornedGoatRecoverySeconds = 0.8f;
    static constexpr float HornedGoatSpawnLeashCentimetres = 2500.0f;
    static constexpr float HornedGoatPushCentimetres = 250.0f;
    static constexpr float HornedGoatStaggerSeconds = 0.75f;
    static bool IsHornedGoatHornInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static constexpr float ThunderSentinelAttackTriggerCentimetres = 420.0f;
    static constexpr float ThunderSentinelSlamReachCentimetres = 300.0f;
    static constexpr float ThunderSentinelStepDistanceCentimetres = 120.0f;
    static constexpr float ThunderSentinelStepSpeedCentimetresPerSecond = 120.0f;
    static constexpr float ThunderSentinelTelegraphSeconds = 1.0f;
    static constexpr float ThunderSentinelSlamCueSeconds = 0.16f;
    static constexpr float ThunderSentinelRecoverySeconds = 1.2f;
    static constexpr float ThunderSentinelSpawnLeashCentimetres = 2500.0f;
    static constexpr int32 ThunderSentinelStoneRewardQuantity = 3;
    static bool IsThunderSentinelSlamInReach(bool bServerAuthority, bool bValidTarget,
        float HorizontalDistanceCentimetres);
    static constexpr float ReefMawMinimumOceanDepthCentimetres = 150.0f;
    static constexpr float ReefMawDetectionCentimetres = 1500.0f;
    static constexpr float ReefMawAttackTriggerCentimetres = 500.0f;
    static constexpr float ReefMawBiteReachCentimetres = 180.0f;
    static constexpr float ReefMawTelegraphSeconds = 1.0f;
    static constexpr float ReefMawBiteCueSeconds = 0.16f;
    static constexpr float ReefMawRecoverySeconds = 1.2f;
    static constexpr float ReefMawApproachSpeedCentimetresPerSecond = 320.0f;
    static constexpr float ReefMawSpawnDepthCentimetres = 120.0f;
    static constexpr float ReefMawSpawnLeashCentimetres = 2500.0f;
    static bool IsReefMawSpawnHabitatAllowed(EKalmalaBiome Biome, bool bLakeWater, float WaterDepthCentimetres);
    static bool IsReefMawBiteInReach(bool bServerAuthority, bool bValidTarget, bool bTargetSwimming,
        float DistanceCentimetres);
    static constexpr float PeatHuskAttackTriggerCentimetres = 500.0f;
    static constexpr float PeatHuskSweepReachCentimetres = 260.0f;
    static constexpr float PeatHuskSweepMinimumForwardDot = 0.5f;
    static constexpr float PeatHuskTelegraphSeconds = 1.0f;
    static constexpr float PeatHuskRecoverySeconds = 1.2f;
    static constexpr float PeatHuskSpawnLeashCentimetres = 2500.0f;
    static constexpr float PeatHuskApproachSpeedCentimetresPerSecond = 100.0f;
    static bool IsPeatHuskSweepInReach(bool bServerAuthority, bool bValidTarget,
        float DistanceCentimetres, float ForwardDot);
    static bool IsMosquitoBiteInReach(bool bServerAuthority, bool bValidTarget, float DistanceCentimetres);
    static bool IsMosquitoFireRepelling(bool bServerAuthority, bool bActiveFlame, float DistanceCentimetres, float RadiusCentimetres);
    static EKalmalaWildlifeArchetype GetArchetypeForSpawnSeed(uint64 SpawnSeed);
    static bool TryGetSpawnArchetypeForBiome(uint64 SpawnSeed, EKalmalaBiome Biome, EKalmalaWildlifeArchetype& OutArchetype);
    static bool IsBoarChargeAllowed(bool bServerAuthority, bool bAlreadyDefeated, bool bAtRest, float DistanceToRestingArea);
    static bool IsBehaviourTransitionAllowed(bool bServerAuthority, bool bAlreadyDefeated, EKalmalaWildlifeBehaviour From, EKalmalaWildlifeBehaviour To);
    static bool IsMossyMireSpawnHabitatAllowed(EKalmalaBiome Biome, bool bIsWater);
    static bool IsThunderMountainsSpawnHabitatAllowed(EKalmalaBiome Biome, bool bIsWater, bool bTraversable);
    FKalmalaWildlifeSpawnDefeated OnDefeated;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Tick(float DeltaSeconds) override;

private:
    friend class AKalmalaArrowProjectile;
    bool ApplyArrowDamageFromServer(const FKalmalaDamageChannels& Damage, AKalmalaCharacter* Attacker, FName ArrowItemId);
    bool ApplyValidatedDamageFromServer(const FKalmalaDamageChannels& Damage, AKalmalaCharacter* Attacker, bool bMelee);
    void ApplyFangArrowWeakPoisonFromServer();
    void AdvanceFangArrowWeakPoisonFromServer(float DeltaSeconds);

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
    void UpdateWolfAttack(float DeltaSeconds);
    void BeginWolfAttack(class AKalmalaCharacter* Target);
    void SetWolfAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsWolfAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyWolfBite(class AKalmalaCharacter* Target);
    bool IsWolfPositionTraversable(const FVector& Position) const;
    bool CanWolfMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdateLynxAttack(float DeltaSeconds);
    void BeginLynxAttack(class AKalmalaCharacter* Target);
    void SetLynxAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsLynxAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyLynxLeap(class AKalmalaCharacter* Target);
    bool IsLynxPositionTraversable(const FVector& Position) const;
    bool CanLynxMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdatePolarBearAttack(float DeltaSeconds);
    void BeginPolarBearAttack(class AKalmalaCharacter* Target);
    void SetPolarBearAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsPolarBearAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyPolarBearCharge(class AKalmalaCharacter* Target);
    bool IsPolarBearPositionTraversable(const FVector& Position) const;
    bool CanPolarBearMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdateMoonBearAttack(float DeltaSeconds);
    void BeginMoonBearAttack(class AKalmalaCharacter* Target);
    void SetMoonBearAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsMoonBearAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyMoonBearPaw(class AKalmalaCharacter* Target);
    void UpdateHornedGoatAttack(float DeltaSeconds);
    void BeginHornedGoatAttack(class AKalmalaCharacter* Target);
    void SetHornedGoatAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsHornedGoatAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyHornedGoatCharge(class AKalmalaCharacter* Target);
    bool IsHornedGoatPositionTraversable(const FVector& Position) const;
    bool CanHornedGoatMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdateThunderSentinelAttack(float DeltaSeconds);
    void BeginThunderSentinelAttack(class AKalmalaCharacter* Target);
    void SetThunderSentinelAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsThunderSentinelAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyThunderSentinelSlam(class AKalmalaCharacter* Target);
    bool IsThunderSentinelPositionTraversable(const FVector& Position) const;
    bool CanThunderSentinelMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdateReefMawAttack(float DeltaSeconds);
    void BeginReefMawAttack(class AKalmalaCharacter* Target);
    void SetReefMawAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsReefMawAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyReefMawBite(class AKalmalaCharacter* Target);
    bool IsReefMawPositionNavigable(const FVector& Position) const;
    bool CanReefMawSwimAlong(const FVector& Start, const FVector& End) const;
    void UpdatePeatHuskAttack(float DeltaSeconds);
    void BeginPeatHuskAttack(class AKalmalaCharacter* Target);
    void SetPeatHuskAttackCue(EKalmalaWildlifeAttackCue NextCue);
    bool IsPeatHuskAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplyPeatHuskSweep(class AKalmalaCharacter* Target);
    bool IsPeatHuskPositionDry(const FVector& Position) const;
    bool CanPeatHuskMoveAlong(const FVector& Start, const FVector& End) const;
    void UpdateSnakeAmbush(float DeltaSeconds);
    void BeginSnakeAttack(class AKalmalaCharacter* Target);
    void SetSnakeAttackCue(EKalmalaWildlifeAttackCue NextCue);
    void PlaySnakeHiss();
    bool IsSnakePositionDry(const FVector& Position) const;
    bool CanSnakeMoveAlong(const FVector& Start, const FVector& End) const;
    bool IsSnakeAttackTargetValid(const class AKalmalaCharacter* Target) const;
    bool TryApplySnakeBite(class AKalmalaCharacter* Target);
    void UpdateMosquitoSwarm(float DeltaSeconds);
    bool IsMosquitoTargetValid(const class AKalmalaCharacter* Target) const;
    bool IsMosquitoTargetReservedByOther(const class AKalmalaCharacter* Target) const;
    bool IsMosquitoPositionValid(const FVector& Position, float& OutGroundHeight) const;
    bool CanMosquitoFlyAlong(const FVector& Start, const FVector& End, const AActor* Target = nullptr) const;
    bool IsMosquitoRepelledByFire(const FVector& Position, const class AKalmalaCharacter* Target,
        FVector& OutFireLocation, float& OutAvoidanceRadius) const;
    bool TryApplyMosquitoBite(class AKalmalaCharacter* Target);
    void PlayMosquitoBuzz();
    void StopMosquitoBuzz();
    void UpdateMosquitoPresentation(float DeltaSeconds);
    void AlertNearbyDeerFromServer();
    void BuildArchetypePresentation();
    void GrantDefeatReward();

    UPROPERTY(ReplicatedUsing = OnRep_Defeated)
    bool bDefeated = false;

    UPROPERTY(Replicated)
    float Health = 75.0f;

    float FangArrowPoisonRemainingSeconds = 0.0f;
    float FangArrowPoisonSecondsUntilTick = 1.0f;

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

    UPROPERTY(ReplicatedUsing = OnRep_WolfAttackCue)
    uint8 WolfAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_LynxAttackCue)
    uint8 LynxAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_PolarBearAttackCue)
    uint8 PolarBearAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_MoonBearAttackCue)
    uint8 MoonBearAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_HornedGoatAttackCue)
    uint8 HornedGoatAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_ThunderSentinelAttackCue)
    uint8 ThunderSentinelAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_ReefMawAttackCue)
    uint8 ReefMawAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

    UPROPERTY(ReplicatedUsing = OnRep_PeatHuskAttackCue)
    uint8 PeatHuskAttackCue = static_cast<uint8>(EKalmalaWildlifeAttackCue::None);

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

    UFUNCTION()
    void OnRep_WolfAttackCue();

    UFUNCTION()
    void OnRep_LynxAttackCue();

    UFUNCTION()
    void OnRep_PolarBearAttackCue();

    UFUNCTION()
    void OnRep_MoonBearAttackCue();

    UFUNCTION()
    void OnRep_HornedGoatAttackCue();

    UFUNCTION()
    void OnRep_ThunderSentinelAttackCue();

    UFUNCTION()
    void OnRep_ReefMawAttackCue();

    UFUNCTION()
    void OnRep_PeatHuskAttackCue();

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
    float WolfAttackSecondsRemaining = 0.0f;
    float WolfLungeDistanceRemaining = 0.0f;
    FVector WolfLungeDirection = FVector::ForwardVector;
    TWeakObjectPtr<class AKalmalaCharacter> WolfAttackTarget;
    float LynxAttackSecondsRemaining = 0.0f;
    float LynxLeapDistanceRemaining = 0.0f;
    FVector LynxLeapDirection = FVector::ForwardVector;
    TWeakObjectPtr<class AKalmalaCharacter> LynxAttackTarget;
    float PolarBearAttackSecondsRemaining = 0.0f;
    float PolarBearChargeDistanceRemaining = 0.0f;
    float PolarBearRetreatDistanceRemaining = 0.0f;
    FVector PolarBearChargeDirection = FVector::ForwardVector;
    FVector PolarBearRetreatDirection = -FVector::ForwardVector;
    TWeakObjectPtr<class AKalmalaCharacter> PolarBearAttackTarget;
    bool bPolarBearChargeHitApplied = false;
    float MoonBearAttackSecondsRemaining = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> MoonBearAttackTarget;
    float HornedGoatAttackSecondsRemaining = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> HornedGoatAttackTarget;
    float ThunderSentinelAttackSecondsRemaining = 0.0f;
    float ThunderSentinelStepDistanceRemaining = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> ThunderSentinelAttackTarget;
    bool bThunderSentinelSlamHitApplied = false;
    float ReefMawAttackSecondsRemaining = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> ReefMawAttackTarget;
    bool bReefMawBiteHitApplied = false;
    float PeatHuskAttackSecondsRemaining = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> PeatHuskAttackTarget;
    TArray<uint8> SnakeHissAudioData;
    TWeakObjectPtr<class AKalmalaCharacter> MosquitoTarget;
    float MosquitoApproachSecondsRemaining = 0.0f;
    float MosquitoBiteSecondsRemaining = 0.0f;
    float MosquitoFlightElapsedSeconds = 0.0f;
#if !UE_BUILD_SHIPPING
    int32 MosquitoAcceptedBiteCountForVerification = 0;
#endif
    FVector MosquitoCurrentFireLocation = FVector::ZeroVector;
    TArray<uint8> MosquitoBuzzAudioData;
    UPROPERTY()
    TObjectPtr<class UProceduralMeshComponent> MirelingMesh;
    UPROPERTY()
    TObjectPtr<class UCapsuleComponent> SnakeBodyCollision;
    UPROPERTY(Transient)
    TObjectPtr<class USoundWaveProcedural> SnakeHissSound;
    UPROPERTY(Transient)
    TObjectPtr<class USoundWaveProcedural> MosquitoBuzzSound;
    UPROPERTY(Transient)
    TObjectPtr<class USoundAttenuation> MosquitoBuzzAttenuation;
    UPROPERTY(Transient)
    TObjectPtr<class UAudioComponent> MosquitoBuzzComponent;
    bool bClientCombatVerificationDefeatLogged = false;
};
