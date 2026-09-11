#include "KalmalaCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"

float UKalmalaCharacterMovementComponent::GetMaxSpeed() const
{
    const float Speed = Super::GetMaxSpeed();
    if (IsSwimmingInGeneratedOcean()) return FMath::Min(Speed, 420.0f);
    return bSprintRequested && IsMovingOnGround() && CharacterOwner && !CharacterOwner->bIsCrouched
        ? Speed * FMath::Clamp(SprintMultiplier, 1.0f, 2.0f) : Speed;
}

bool UKalmalaCharacterMovementComponent::GetGeneratedOceanDepth(float& OutDepth) const
{
    OutDepth = 0.0f;
    if (CharacterOwner == nullptr || CharacterOwner->GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* GenerationState = CharacterOwner->GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (GenerationState == nullptr) return false;
    const FKalmalaOceanSample Sample = FKalmalaOceanSampler::Sample(GenerationState->GetWorldGenerationConfig(), FVector2D(CharacterOwner->GetActorLocation()));
    if (!Sample.bIsValid) return false;
    OutDepth = Sample.WaterDepth;
    return true;
}

void UKalmalaCharacterMovementComponent::UpdateCharacterStateBeforeMovement(const float DeltaSeconds)
{
    Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
    float WaterDepth = 0.0f;
    const bool bHasDeepWater = GetGeneratedOceanDepth(WaterDepth) && ShouldEnterGeneratedOcean(WaterDepth);
    if (IsSwimmingInGeneratedOcean())
    {
        // Hysteresis prevents shore triangles from rapidly toggling the replicated movement mode.
        if (ShouldReturnToLand(WaterDepth)) SetMovementMode(MOVE_Walking);
    }
    else if (bHasDeepWater)
    {
        SetMovementMode(MOVE_Custom, GeneratedOceanSwimmingMode);
    }
}

void UKalmalaCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
    Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
    if (!CharacterOwner || !UpdatedComponent || CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy) return;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !FKalmalaWorldBounds::IsBounded(State->GetWorldGenerationConfig())) return;
    const FVector Location = UpdatedComponent->GetComponentLocation();
    const FVector2D Bounded = FKalmalaWorldBounds::Constrain(State->GetWorldGenerationConfig(), FVector2D(Location),
        CharacterOwner->GetSimpleCollisionRadius() + 2.0);
    if (!Bounded.Equals(FVector2D(Location), 0.001))
    {
        UpdatedComponent->SetWorldLocation(FVector(Bounded, Location.Z), false, nullptr, ETeleportType::TeleportPhysics);
        const FVector Outward(Bounded.GetSafeNormal(), 0);
        Velocity -= Outward * FMath::Max(0.0, FVector::DotProduct(Velocity, Outward));
    }
}

void UKalmalaCharacterMovementComponent::PhysCustom(const float DeltaTime, const int32 Iterations)
{
    if (!IsSwimmingInGeneratedOcean())
    {
        Super::PhysCustom(DeltaTime, Iterations);
        return;
    }

    float WaterDepth = 0.0f;
    if (!GetGeneratedOceanDepth(WaterDepth) || ShouldReturnToLand(WaterDepth))
    {
        SetMovementMode(MOVE_Walking);
        StartNewPhysics(DeltaTime, Iterations);
        return;
    }

    // The visible sea is at zero. Keep the capsule centre just above it while
    // retaining normal swept collision against the shared terrain mesh.
    const float SurfaceCentreZ = CharacterOwner->GetSimpleCollisionHalfHeight() + 8.0f;
    const float VerticalError = SurfaceCentreZ - CharacterOwner->GetActorLocation().Z;
    Velocity.Z = FMath::Clamp(VerticalError * 6.0f, -300.0f, 300.0f);
    PhysFlying(DeltaTime, Iterations);
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
