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
    void StartBowDraw(uint32 Sequence);
    void CancelLocalBowDraw();
    UFUNCTION(Server, Reliable) void ServerCancelBowDraw();
    void ReleaseBowDraw(uint32 Sequence, bool bCancel = false);
    UFUNCTION(Server, Reliable) void ServerBeginBowDraw(uint32 Sequence);
    UFUNCTION(Server, Reliable) void ServerReleaseBow(uint32 Sequence, bool bCancel);
    UFUNCTION(Server, Reliable) void ServerPreferArrowStack(FGuid StackOrUnitGuid);
    FGuid GetPreferredArrowStack() const { return PreferredArrowStack; }
    FName GetArrowItemIdForDisplay() const;
    bool IsBowDrawing() const;
    float GetBowDrawProgress() const;
    float GetBowDrawDuration() const;
    bool ResolveCurrentBowShotDamage(FName ArrowItemId, float DrawProgress, float& OutShotDamage) const;
    static float CalculateBowDrawProgress(float Elapsed, float DrawDuration, float ArcherySpeedMultiplier = 1.0f);
    static float GetBowCrosshairRadius(float Progress);
    bool CanDrawBow() const;
    EKalmalaCombatActionPhase GetActionPhase() const { return ActionPhase; }
    uint32 GetActionSerial() const { return ActionSerial; }
    EKalmalaCombatFeedback GetFeedback() const { return Feedback; }
    uint32 GetFeedbackSerial() const { return FeedbackSerial; }
    static constexpr float GetRecoverySeconds() { return RecoverySeconds; }
    static bool IsAttackItemAllowed(FName ItemId);
    static bool CanSwingWithoutTarget(FName ItemId, int32 Level, int32 Condition);
    static bool IsAttackRequestAllowed(bool bAuthority, bool bNewSequence, uint32 Sequence, bool bActionIdle, bool bSwingAvailable);
private:
    void CancelBowDrawFromServer();
    int32 FindArrowStack() const;
    UPROPERTY(ReplicatedUsing=OnRep_BowDrawing) bool bBowDrawing = false;
    UPROPERTY(Replicated) double BowDrawStartTime = 0;
    UPROPERTY(Replicated) FGuid PreferredArrowStack;
    UFUNCTION() void OnRep_BowDrawing();
    FGuid DrawingBowGuid;
    bool bLocalBowDrawing = false;
    double LocalBowDrawStart = 0;
    AActor* FindServerTarget() const;
    bool IsServerTargetValid(const AActor* Target) const;
    void BeginRecovery();
    void PublishFeedbackFromServer(EKalmalaCombatFeedback NewFeedback);
    static constexpr float AttackRange = 220.0f;
    static constexpr float UnarmedAttackDamage = 25.0f;
    static constexpr float WindupSeconds = 0.18f;
    static constexpr float RecoverySeconds = 0.36f;
    UPROPERTY(Replicated) EKalmalaCombatActionPhase ActionPhase = EKalmalaCombatActionPhase::Idle;
    UPROPERTY(Replicated) uint32 ActionSerial = 0;
    UPROPERTY(Replicated) EKalmalaCombatFeedback Feedback = EKalmalaCombatFeedback::None;
    UPROPERTY(Replicated) uint32 FeedbackSerial = 0;
    uint32 LastRequestSequence = 0;
    float PhaseEndTime = 0.0f;
    float LastUnavailableFeedbackTime = -1.0f;
    TWeakObjectPtr<AActor> PendingTarget;
    bool bClientCombatVerificationActionLogged = false;
    bool bClientCombatVerificationRejectionLogged = false;
};
