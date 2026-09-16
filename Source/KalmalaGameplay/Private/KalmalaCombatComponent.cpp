#include "KalmalaCombatComponent.h"
#include "KalmalaCombatIntentContract.h"
#include "KalmalaWildlifeSpawn.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UKalmalaCombatComponent::UKalmalaCombatComponent() { PrimaryComponentTick.bCanEverTick = true; SetIsReplicatedByDefault(true); }
bool UKalmalaCombatComponent::IsAttackRequestAllowed(const bool bAuthority, const bool bNewSequence, const uint32 Sequence, const bool bActionIdle, const bool bTargetValid)
{ return FKalmalaCombatIntentContract::IsAttackMutationAllowed(bAuthority, true, true, bNewSequence, Sequence, true, bActionIdle, bTargetValid, bTargetValid); }
void UKalmalaCombatComponent::ServerRequestAttack_Implementation(const uint32 RequestSequence)
{
    APawn* Pawn = Cast<APawn>(GetOwner()); AKalmalaWildlifeSpawn* Target = FindServerTarget();
    const bool bAllowed = Pawn != nullptr && IsAttackRequestAllowed(Pawn->HasAuthority(), RequestSequence != 0 && RequestSequence > LastRequestSequence, RequestSequence, ActionPhase == EKalmalaCombatActionPhase::Idle, IsServerTargetValid(Target));
    if (!bAllowed)
    {
        if (Pawn != nullptr && Pawn->HasAuthority() && GetWorld() != nullptr && GetWorld()->GetTimeSeconds() >= LastUnavailableFeedbackTime + 0.25f)
        {
            LastUnavailableFeedbackTime = GetWorld()->GetTimeSeconds();
            PublishFeedbackFromServer(EKalmalaCombatFeedback::Unavailable);
        }
        return;
    }
    LastRequestSequence = RequestSequence; ++ActionSerial; PendingTarget = Target; ActionPhase = EKalmalaCombatActionPhase::Windup; PhaseEndTime = GetWorld()->GetTimeSeconds() + WindupSeconds; GetOwner()->ForceNetUpdate();
}
void UKalmalaCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!GetOwner()->HasAuthority() || ActionPhase == EKalmalaCombatActionPhase::Idle || GetWorld() == nullptr || GetWorld()->GetTimeSeconds() < PhaseEndTime) return;
    if (ActionPhase == EKalmalaCombatActionPhase::Windup)
    {
        if (AKalmalaWildlifeSpawn* Target = PendingTarget.Get(); IsServerTargetValid(Target))
        {
            const bool bDefeated = Target->GetHealth() <= AttackDamage;
            if (Target->ApplyCombatDamageFromServer(AttackDamage)) PublishFeedbackFromServer(bDefeated ? EKalmalaCombatFeedback::Defeat : EKalmalaCombatFeedback::Hit);
        }
        else PublishFeedbackFromServer(EKalmalaCombatFeedback::Unavailable);
        BeginRecovery();
        return;
    }
    ActionPhase = EKalmalaCombatActionPhase::Idle; PendingTarget.Reset(); GetOwner()->ForceNetUpdate();
}
AKalmalaWildlifeSpawn* UKalmalaCombatComponent::FindServerTarget() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner()); if (Pawn == nullptr || GetWorld() == nullptr) return nullptr;
    const FVector Start = Pawn->GetActorLocation() + FVector(0, 0, 55); FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(KalmalaCombatTarget), false, Pawn);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Pawn->GetActorForwardVector() * AttackRange, ECC_Visibility, Params) ? Cast<AKalmalaWildlifeSpawn>(Hit.GetActor()) : nullptr;
}
bool UKalmalaCombatComponent::IsServerTargetValid(const AKalmalaWildlifeSpawn* Target) const
{ const APawn* Pawn = Cast<APawn>(GetOwner()); return Pawn != nullptr && Target != nullptr && !Target->IsDefeated() && FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(AttackRange); }
void UKalmalaCombatComponent::BeginRecovery() { ActionPhase = EKalmalaCombatActionPhase::Recovery; PhaseEndTime = GetWorld()->GetTimeSeconds() + RecoverySeconds; GetOwner()->ForceNetUpdate(); }
void UKalmalaCombatComponent::PublishFeedbackFromServer(const EKalmalaCombatFeedback NewFeedback)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    Feedback = NewFeedback;
    ++FeedbackSerial;
    GetOwner()->ForceNetUpdate();
}
void UKalmalaCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UKalmalaCombatComponent, ActionPhase);
    DOREPLIFETIME(UKalmalaCombatComponent, ActionSerial);
    DOREPLIFETIME_CONDITION(UKalmalaCombatComponent, Feedback, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCombatComponent, FeedbackSerial, COND_OwnerOnly);
}
