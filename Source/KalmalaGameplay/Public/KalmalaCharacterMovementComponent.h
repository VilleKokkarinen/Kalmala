#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "KalmalaCharacterMovementComponent.generated.h"

/** Sprint intent travels in saved moves; speed is always calculated locally/server-side. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaCharacterMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()
public:
    void SetSprintRequested(bool bRequested) { bSprintRequested = bRequested; }
    bool IsSprintRequested() const { return bSprintRequested; }
    virtual float GetMaxSpeed() const override;
    virtual void UpdateFromCompressedFlags(uint8 Flags) override;
    virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

    UPROPERTY(EditDefaultsOnly, Category = "Movement", meta = (ClampMin = "1.0", ClampMax = "2.0"))
    float SprintMultiplier = 1.5f;
private:
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
