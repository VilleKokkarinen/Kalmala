#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "KalmalaCharacterMovementComponent.generated.h"

/** Sprint intent travels in saved moves; speed is always calculated locally/server-side. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaCharacterMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    UKalmalaCharacterMovementComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    float GetStamina() const { return Stamina; }
    bool IsSprintExhausted() const { return bSprintExhausted; }
    void AdvanceStaminaFromServer(float DeltaSeconds);
    bool TryConsumeStaminaFromServer(float Cost);
    static constexpr float MaximumStamina = 100.0f;
    static constexpr float SprintCostPerSecond = 10.0f;
    static constexpr float RecoveryPerSecond = 15.0f;
    static constexpr float SprintRecoveryThreshold = 20.0f;
    void SetSprintRequested(bool bRequested) { bSprintRequested = bRequested; }
    bool IsSprintRequested() const { return bSprintRequested; }
    virtual float GetMaxSpeed() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
    virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

    bool IsSwimmingInGeneratedOcean() const { return MovementMode == MOVE_Custom && CustomMovementMode == GeneratedOceanSwimmingMode; }
    static bool ShouldEnterGeneratedOcean(float WaterDepth) { return WaterDepth >= 100.0f; }
    static bool ShouldReturnToLand(float WaterDepth) { return WaterDepth < 75.0f; }

    static constexpr uint8 GeneratedOceanSwimmingMode = 1;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1.0", ClampMax = "2.0"))
    float SprintMultiplier = 1.5f;
private:
    UPROPERTY(Replicated) float Stamina = MaximumStamina;
    UPROPERTY(Replicated) bool bSprintExhausted = false;
    bool GetGeneratedOceanDepth(float& OutDepth) const;
    bool bSprintRequested = false;
};

class FSavedMove_Kalmala : public FSavedMove_Character
{
public:
    using Super = FSavedMove_Character;
    bool bSavedSprint = false;
    virtual void Clear() override;
    virtual uint8 GetCompressedFlags() const override;
    virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
    virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel,
        FNetworkPredictionData_Client_Character& ClientData) override;
    virtual void PrepMoveFor(ACharacter* Character) override;
};
