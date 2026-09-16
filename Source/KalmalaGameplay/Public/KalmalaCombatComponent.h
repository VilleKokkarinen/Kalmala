#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaCombatComponent.generated.h"

class AKalmalaWildlifeSpawn;

UENUM(BlueprintType)
enum class EKalmalaCombatActionPhase : uint8 { Idle, Windup, Recovery };

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
    static bool IsAttackRequestAllowed(bool bAuthority, bool bNewSequence, uint32 Sequence, bool bActionIdle, bool bTargetValid);
private:
    AKalmalaWildlifeSpawn* FindServerTarget() const;
    bool IsServerTargetValid(const AKalmalaWildlifeSpawn* Target) const;
    void BeginRecovery();
    static constexpr float AttackRange = 220.0f;
    static constexpr float AttackDamage = 25.0f;
    static constexpr float WindupSeconds = 0.18f;
    static constexpr float RecoverySeconds = 0.42f;
    UPROPERTY(Replicated) EKalmalaCombatActionPhase ActionPhase = EKalmalaCombatActionPhase::Idle;
    UPROPERTY(Replicated) uint32 ActionSerial = 0;
    uint32 LastRequestSequence = 0;
    float PhaseEndTime = 0.0f;
    TWeakObjectPtr<AKalmalaWildlifeSpawn> PendingTarget;
};
