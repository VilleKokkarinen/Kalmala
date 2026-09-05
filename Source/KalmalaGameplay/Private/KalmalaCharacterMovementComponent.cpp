#include "KalmalaCharacterMovementComponent.h"
#include "GameFramework/Character.h"

float UKalmalaCharacterMovementComponent::GetMaxSpeed() const
{
    const float Speed = Super::GetMaxSpeed();
    return bSprintRequested && IsMovingOnGround() && CharacterOwner && !CharacterOwner->bIsCrouched
        ? Speed * FMath::Clamp(SprintMultiplier, 1.0f, 2.0f) : Speed;
}

void UKalmalaCharacterMovementComponent::UpdateFromCompressedFlags(const uint8 Flags)
{
    Super::UpdateFromCompressedFlags(Flags);
    bSprintRequested = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void FSavedMove_Kalmala::Clear()
{
    Super::Clear();
    bSavedSprint = false;
}

uint8 FSavedMove_Kalmala::GetCompressedFlags() const
{
    return Super::GetCompressedFlags() | (bSavedSprint ? FLAG_Custom_0 : 0);
}

bool FSavedMove_Kalmala::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, const float MaxDelta) const
{
    if (bSavedSprint != static_cast<const FSavedMove_Kalmala*>(NewMove.Get())->bSavedSprint) return false;
    return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_Kalmala::SetMoveFor(ACharacter* Character, const float InDeltaTime, FVector const& NewAccel,
    FNetworkPredictionData_Client_Character& ClientData)
{
    Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);
    bSavedSprint = CastChecked<UKalmalaCharacterMovementComponent>(Character->GetCharacterMovement())->IsSprintRequested();
}

void FSavedMove_Kalmala::PrepMoveFor(ACharacter* Character)
{
    Super::PrepMoveFor(Character);
    CastChecked<UKalmalaCharacterMovementComponent>(Character->GetCharacterMovement())->SetSprintRequested(bSavedSprint);
}

namespace
{
    class FPredictionData_Kalmala : public FNetworkPredictionData_Client_Character
    {
    public:
        explicit FPredictionData_Kalmala(const UCharacterMovementComponent& Movement)
            : FNetworkPredictionData_Client_Character(Movement) {}
        virtual FSavedMovePtr AllocateNewMove() override { return FSavedMovePtr(new FSavedMove_Kalmala()); }
    };
}

FNetworkPredictionData_Client* UKalmalaCharacterMovementComponent::GetPredictionData_Client() const
{
    if (!ClientPredictionData)
    {
        auto* MutableThis = const_cast<UKalmalaCharacterMovementComponent*>(this);
        MutableThis->ClientPredictionData = new FPredictionData_Kalmala(*this);
    }
    return ClientPredictionData;
}
