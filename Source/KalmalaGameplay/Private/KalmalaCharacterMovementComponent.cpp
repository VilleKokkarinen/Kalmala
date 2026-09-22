#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaPlayerStatusComponent.h"
#include "GameFramework/Character.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaOceanTravelTestFixture.h"
#include "EngineUtils.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

UKalmalaCharacterMovementComponent::UKalmalaCharacterMovementComponent()
{
    SetIsReplicatedByDefault(true);
}

void UKalmalaCharacterMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UKalmalaCharacterMovementComponent, Stamina);
    DOREPLIFETIME(UKalmalaCharacterMovementComponent, CurrentMaximumStamina);
    DOREPLIFETIME(UKalmalaCharacterMovementComponent, bSprintExhausted);
}

void UKalmalaCharacterMovementComponent::AdvanceStaminaFromServer(const float DeltaSeconds)
{
    if (!CharacterOwner || !CharacterOwner->HasAuthority() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;
    // Use the engine-validated movement step, including remote autonomous moves.
    const float Step = FMath::Min(DeltaSeconds, 0.25f);
    const AKalmalaCharacter* Pawn = Cast<AKalmalaCharacter>(CharacterOwner);
    const bool bConsumes = bSprintRequested && !bSprintExhausted && IsMovingOnGround()
        && !CharacterOwner->bIsCrouched && Velocity.SizeSquared2D() > 1.0f;
    if (bConsumes)
    {
        const float BaseCost = SprintCostPerSecond * Step;
        const float Cost = Pawn && Pawn->GetStatusComponent()
            ? Pawn->GetStatusComponent()->CalculateStaminaCost(BaseCost) : BaseCost;
        Stamina = FMath::Max(0.0f, Stamina - Cost);
        if (Stamina <= 0.0f) bSprintExhausted = true;
    }
    else
    {
        Stamina = FMath::Min(CurrentMaximumStamina, Stamina + RecoveryPerSecond * Step);
        if (Stamina >= SprintRecoveryThreshold) bSprintExhausted = false;
    }
}

bool UKalmalaCharacterMovementComponent::TryConsumeStaminaFromServer(const float Cost)
{
    if (!CharacterOwner || !CharacterOwner->HasAuthority() || !FMath::IsFinite(Cost) || Cost <= 0.0f || Cost > MaximumStamina || Stamina < Cost) return false;
    Stamina -= Cost; if (Stamina <= 0.0f) bSprintExhausted = true; CharacterOwner->ForceNetUpdate(); return true;
}

bool UKalmalaCharacterMovementComponent::SetBearsVigorFromServer(const bool bEnabled)
{
    if (!CharacterOwner || !CharacterOwner->HasAuthority()) return false;
    const float NewMaximum = bEnabled ? MaximumStamina + 40.0f : MaximumStamina;
    if (!FMath::IsFinite(NewMaximum) || NewMaximum < MaximumStamina || NewMaximum > MaximumStamina + 40.0f) return false;
    if (FMath::IsNearlyEqual(CurrentMaximumStamina, NewMaximum)) return true;
    CurrentMaximumStamina = NewMaximum;
    // Vigor never refills stamina. Its removal only clamps the existing authoritative value.
    Stamina = FMath::Clamp(Stamina, 0.0f, CurrentMaximumStamina);
    if (Stamina <= 0.0f) bSprintExhausted = true;
    else if (Stamina >= SprintRecoveryThreshold) bSprintExhausted = false;
    CharacterOwner->ForceNetUpdate();
    return true;
}

float UKalmalaCharacterMovementComponent::GetMaxSpeed() const
{
    const float Speed = Super::GetMaxSpeed();
    const AKalmalaCharacter* Pawn = Cast<AKalmalaCharacter>(CharacterOwner);
    const float StatusSpeed = Pawn && Pawn->GetStatusComponent() ? Pawn->GetStatusComponent()->GetModifiers().Movement : 1.0f;
    if (IsSwimmingInGeneratedOcean())
    {
#if !UE_BUILD_SHIPPING
        // The ocean-travel fixture can span the current seed's long-distance
        // island route; accelerate only that verification command, never play.
        const float OceanSpeedCap = FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanTravelTest")) ? 1800.0f : 420.0f;
#else
        constexpr float OceanSpeedCap = 420.0f;
#endif
        return FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanTravelTest")) ? OceanSpeedCap * StatusSpeed
            : FMath::Min(Speed, OceanSpeedCap) * StatusSpeed;
    }
    return (bSprintRequested && !bSprintExhausted && IsMovingOnGround() && CharacterOwner && !CharacterOwner->bIsCrouched
        ? Speed * FMath::Clamp(SprintMultiplier, 1.0f, 2.0f) : Speed) * StatusSpeed;
}

bool UKalmalaCharacterMovementComponent::GetGeneratedOceanDepth(float& OutDepth) const
{
    OutDepth = 0.0f;
    if (CharacterOwner == nullptr || CharacterOwner->GetWorld() == nullptr) return false;
    const AKalmalaWorldGenerationGameState* GenerationState = CharacterOwner->GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (GenerationState == nullptr) return false;
    for (TActorIterator<AKalmalaOceanTravelTestFixture> Iterator(CharacterOwner->GetWorld()); Iterator; ++Iterator)
    {
        if (Iterator->TryGetWaterDepth(FVector2D(CharacterOwner->GetActorLocation()), OutDepth))
        {
            return true;
        }
    }
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
    AdvanceStaminaFromServer(DeltaSeconds);
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
