#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaCombatComponent.generated.h"

class AKalmalaWildlifeSpawn;

UENUM(BlueprintType)
enum class EKalmalaCombatActionPhase : uint8 { Idle, Windup, Recovery };

/** Owner-only result category. It deliberately contains no target identity. */
UENUM(BlueprintType)
enum class EKalmalaCombatFeedback : uint8 { None, Hit, Defeat, Unavailable };

/** Owner-pawn combat intent. Server selects target, timing, and damage. */
UCLASS(ClassGroup=(Kalmala), meta=(BlueprintSpawnableComponent))
class KALMALAGAMEPLAY_API UKalmalaCombatComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKalmalaCombatComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(Server, Reliable) void ServerRequestAttack(uint32 RequestSequence);
    EKalmalaCombatActionPhase GetActionPhase() const { return ActionPhase; }
    uint32 GetActionSerial() const { return ActionSerial; }
    EKalmalaCombatFeedback GetFeedback() const { return Feedback; }
    uint32 GetFeedbackSerial() const { return FeedbackSerial; }
    static bool IsAttackRequestAllowed(bool bAuthority, bool bNewSequence, uint32 Sequence, bool bActionIdle, bool bTargetValid);
private:
    AKalmalaWildlifeSpawn* FindServerTarget() const;
    bool IsServerTargetValid(const AKalmalaWildlifeSpawn* Target) const;
    void BeginRecovery();
    void PublishFeedbackFromServer(EKalmalaCombatFeedback NewFeedback);
    static constexpr float AttackRange = 220.0f;
    static constexpr float AttackDamage = 25.0f;
    static constexpr float WindupSeconds = 0.18f;
    static constexpr float RecoverySeconds = 0.42f;
    UPROPERTY(Replicated) EKalmalaCombatActionPhase ActionPhase = EKalmalaCombatActionPhase::Idle;
    UPROPERTY(Replicated) uint32 ActionSerial = 0;
    UPROPERTY(Replicated) EKalmalaCombatFeedback Feedback = EKalmalaCombatFeedback::None;
    UPROPERTY(Replicated) uint32 FeedbackSerial = 0;
    uint32 LastRequestSequence = 0;
    float PhaseEndTime = 0.0f;
    float LastUnavailableFeedbackTime = -1.0f;
    TWeakObjectPtr<AKalmalaWildlifeSpawn> PendingTarget;
    bool bClientCombatVerificationActionLogged = false;
    bool bClientCombatVerificationRejectionLogged = false;
};
