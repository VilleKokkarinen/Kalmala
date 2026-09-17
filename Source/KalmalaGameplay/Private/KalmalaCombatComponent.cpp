#include "KalmalaCombatComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaCombatIntentContract.h"
#include "KalmalaWildlifeSpawn.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
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
    const bool bCombatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest"));
    const bool bMirelingPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest"));
    const bool bBoarPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest"));
    const bool bDeerPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest"));
    if ((bCombatPeerTest || bMirelingPeerTest || bBoarPeerTest || bDeerPeerTest) && GetOwner() != nullptr && !GetOwner()->HasAuthority())
    {
        const TCHAR* VerificationName = bBoarPeerTest ? TEXT("Boar") : (bDeerPeerTest ? TEXT("Deer") : (bMirelingPeerTest ? TEXT("Mireling") : TEXT("Combat")));
        const APawn* OwnerPawn = Cast<APawn>(GetOwner());
        if (!bClientCombatVerificationActionLogged && ActionSerial >= 4)
        {
            bClientCombatVerificationActionLogged = true;
            UE_LOG(LogTemp, Display, TEXT("%s verification client observed shared action serial=%u."), VerificationName, ActionSerial);
        }
        if (!bClientCombatVerificationRejectionLogged && OwnerPawn != nullptr && OwnerPawn->IsLocallyControlled() && FeedbackSerial > 0 && Feedback == EKalmalaCombatFeedback::Unavailable)
        {
            bClientCombatVerificationRejectionLogged = true;
            UE_LOG(LogTemp, Display, TEXT("%s verification client rejected invalid owned attack without target data."), VerificationName);
        }
    }
    if (!GetOwner()->HasAuthority() || ActionPhase == EKalmalaCombatActionPhase::Idle || GetWorld() == nullptr || GetWorld()->GetTimeSeconds() < PhaseEndTime) return;
    if (ActionPhase == EKalmalaCombatActionPhase::Windup)
    {
        if (AKalmalaWildlifeSpawn* Target = PendingTarget.Get(); IsServerTargetValid(Target))
        {
            const AKalmalaCharacter* OwnerCharacter = Cast<AKalmalaCharacter>(GetOwner());
            const auto* Support = OwnerCharacter ? OwnerCharacter->GetSupportMagicComponent() : nullptr;
            const float Now = GetWorld()->GetTimeSeconds();
            const float Damage = UKalmalaSupportMagicComponent::CalculateBearsVigorDamage(true,
                Support && Support->GetBearsVigorExpiry() > Now, AttackDamage, Support ? Support->GetBearsVigorStrengthMultiplier() : 1.0f);
            const bool bDefeated = Target->GetHealth() <= Damage;
            const bool bApplied = Target->ApplyCombatDamageFromServer(Damage, Cast<AKalmalaCharacter>(GetOwner()));
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")))
            {
                UE_LOG(LogTemp, Display, TEXT("Boar verification attack execution: Applied=%d Distance=%.1f Health=%.1f."), bApplied, FVector::Dist(Cast<APawn>(GetOwner())->GetActorLocation(), Target->GetActorLocation()), Target->GetHealth());
            }
#endif
            if (bApplied) PublishFeedbackFromServer(bDefeated ? EKalmalaCombatFeedback::Defeat : EKalmalaCombatFeedback::Hit);
        }
        else
        {
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest")))
            {
                const AKalmalaWildlifeSpawn* Pending = PendingTarget.Get();
                const APawn* OwnerPawn = Cast<APawn>(GetOwner());
                UE_LOG(LogTemp, Display, TEXT("Boar verification attack execution: target valid=%d distance=%.1f."), Pending != nullptr, Pending != nullptr && OwnerPawn != nullptr ? FVector::Dist(OwnerPawn->GetActorLocation(), Pending->GetActorLocation()) : -1.0f);
            }
#endif
            PublishFeedbackFromServer(EKalmalaCombatFeedback::Unavailable);
        }
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
