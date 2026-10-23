#include "KalmalaSupportMagicComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

UKalmalaSupportMagicComponent::UKalmalaSupportMagicComponent() { PrimaryComponentTick.bCanEverTick = true; SetIsReplicatedByDefault(true); }
FString UKalmalaSupportMagicComponent::CanonicalId(const EKalmalaSupportEffect Effect)
{
    switch (Effect) { case EKalmalaSupportEffect::Mending: return TEXT("Effect:mending"); case EKalmalaSupportEffect::HearthShield: return TEXT("Effect:hearth-shield"); case EKalmalaSupportEffect::BearsVigor: return TEXT("Effect:bears-vigor"); case EKalmalaSupportEffect::DeerCall: return TEXT("Effect:deer-call"); default: return FString(); }
}
FName UKalmalaSupportMagicComponent::ScrollItemId(const EKalmalaSupportEffect Effect)
{
    switch (Effect)
    {
    case EKalmalaSupportEffect::Mending: return TEXT("MendingScroll");
    case EKalmalaSupportEffect::HearthShield: return TEXT("HearthShieldScroll");
    case EKalmalaSupportEffect::BearsVigor: return TEXT("BearsVigorScroll");
    case EKalmalaSupportEffect::DeerCall: return TEXT("DeerCallScroll");
    default: return NAME_None;
    }
}
EKalmalaSupportEffect UKalmalaSupportMagicComponent::FromScrollItemId(const FName ItemId)
{
    if (ItemId == TEXT("MendingScroll")) return EKalmalaSupportEffect::Mending;
    if (ItemId == TEXT("HearthShieldScroll")) return EKalmalaSupportEffect::HearthShield;
    if (ItemId == TEXT("BearsVigorScroll")) return EKalmalaSupportEffect::BearsVigor;
    if (ItemId == TEXT("DeerCallScroll")) return EKalmalaSupportEffect::DeerCall;
    return EKalmalaSupportEffect::None;
}
EKalmalaSupportEffect UKalmalaSupportMagicComponent::FromScrollDefinition(const FString& Definition)
{ if (Definition == TEXT("mending")) return EKalmalaSupportEffect::Mending; if (Definition == TEXT("hearth-shield")) return EKalmalaSupportEffect::HearthShield; if (Definition == TEXT("bears-vigor")) return EKalmalaSupportEffect::BearsVigor; if (Definition == TEXT("deer-call")) return EKalmalaSupportEffect::DeerCall; return EKalmalaSupportEffect::None; }
bool UKalmalaSupportMagicComponent::IsKnownEffect(const EKalmalaSupportEffect Effect)
{ return Effect >= EKalmalaSupportEffect::Mending && Effect <= EKalmalaSupportEffect::DeerCall; }
bool UKalmalaSupportMagicComponent::IsNonDamagingEffect(const EKalmalaSupportEffect Effect)
{ return IsKnownEffect(Effect); }
bool UKalmalaSupportMagicComponent::LearnEffectFromServer(const EKalmalaSupportEffect Effect)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !IsKnownEffect(Effect)) return false;
    LearnedMask |= static_cast<uint8>(1u << static_cast<uint8>(Effect));
    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        if (UKalmalaInventoryComponent* Inventory = Pawn->FindComponentByClass<UKalmalaInventoryComponent>())
        {
            const FName ScrollId = ScrollItemId(Effect);
            if (!ScrollId.IsNone() && Inventory->GetQuantity(ScrollId) == 0)
            {
                // This ordinary runtime stack is rebuilt from the persisted entitlement, never saved.
                Inventory->TryGrantFromServer(ScrollId, 1);
            }
        }
    }
    GetOwner()->ForceNetUpdate();
    return true;
}
bool UKalmalaSupportMagicComponent::HasLearnedEffect(const EKalmalaSupportEffect Effect) const
{ return !CanonicalId(Effect).IsEmpty() && (LearnedMask & static_cast<uint8>(1u << static_cast<uint8>(Effect))) != 0; }
bool UKalmalaSupportMagicComponent::IsActivationAllowed(const bool bAuthority, const bool bLearned, const bool bRequestCadenceAllowed, const bool bCooldownExpired, const bool bHasStamina)
{ return bAuthority && bLearned && bRequestCadenceAllowed && bCooldownExpired && bHasStamina; }
bool UKalmalaSupportMagicComponent::IsHearthShieldActivationAllowed(const bool bBaseActivationAllowed, const bool bShieldAlreadyActive)
{ return bBaseActivationAllowed && !bShieldAlreadyActive; }
bool UKalmalaSupportMagicComponent::IsBearsVigorActivationAllowed(const bool bBaseActivationAllowed, const bool bVigorAlreadyActive)
{ return bBaseActivationAllowed && !bVigorAlreadyActive; }
bool UKalmalaSupportMagicComponent::IsDeerCallActivationAllowed(const bool bBaseActivationAllowed, const bool bHasEligibleDeer)
{ return bBaseActivationAllowed && bHasEligibleDeer; }
float UKalmalaSupportMagicComponent::CalculateHearthShieldAbsorption(const bool bServerAuthority, const bool bShieldActive,
    const float IncomingDamage, const float RemainingStrength)
{
    if (!bServerAuthority || !bShieldActive || !FMath::IsFinite(IncomingDamage) || !FMath::IsFinite(RemainingStrength)
        || IncomingDamage <= 0.0f || RemainingStrength <= 0.0f) return 0.0f;
    return FMath::Min(IncomingDamage, RemainingStrength);
}
float UKalmalaSupportMagicComponent::CalculateBearsVigorDamage(const bool bServerAuthority, const bool bVigorActive,
    const float BaseDamage, const float StrengthMultiplier)
{
    if (!bServerAuthority || !bVigorActive || !FMath::IsFinite(BaseDamage) || !FMath::IsFinite(StrengthMultiplier)
        || BaseDamage <= 0.0f || BaseDamage > 25.0f || StrengthMultiplier < 1.0f || StrengthMultiplier > BearsVigorStrength) return BaseDamage;
    return FMath::Min(35.0f, BaseDamage * StrengthMultiplier);
}

float UKalmalaSupportMagicComponent::AbsorbHearthShieldDamageFromServer(const float IncomingDamage)
{
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const bool bShieldActive = GetOwner() && GetOwner()->HasAuthority() && HearthShieldExpiry > Now && HearthShieldStrength > 0.0f;
    const float Absorbed = CalculateHearthShieldAbsorption(GetOwner() && GetOwner()->HasAuthority(), bShieldActive, IncomingDamage, HearthShieldStrength);
    if (Absorbed <= 0.0f) return 0.0f;
    HearthShieldStrength = FMath::Max(0.0f, HearthShieldStrength - Absorbed);
    if (HearthShieldStrength <= 0.0f)
    {
        HearthShieldExpiry = 0.0f;
        if (ActiveEffect == EKalmalaSupportEffect::HearthShield)
        {
            ActiveEffect = EKalmalaSupportEffect::None;
            ActiveEffectExpiry = 0.0f;
        }
    }
    GetOwner()->ForceNetUpdate();
    return Absorbed;
}

AKalmalaCharacter* UKalmalaSupportMagicComponent::ResolveMendingTargetFromServer(APawn* Caster) const
{
    if (!Caster || !Caster->HasAuthority() || !GetWorld()) return nullptr;
    const FVector Start = Caster->GetPawnViewLocation();
    const FVector End = Start + Caster->GetActorForwardVector().GetSafeNormal() * MendingRange;
    FCollisionQueryParams PawnQuery(SCENE_QUERY_STAT(KalmalaMendingTarget), false, Caster);
    FHitResult PawnHit;
    if (!GetWorld()->LineTraceSingleByChannel(PawnHit, Start, End, ECC_Pawn, PawnQuery)) return nullptr;
    AKalmalaCharacter* Target = Cast<AKalmalaCharacter>(PawnHit.GetActor());
    AKalmalaCharacter* Source = Cast<AKalmalaCharacter>(Caster);
    if (!Target || !Source || Target == Source) return nullptr;
    FCollisionQueryParams VisibilityQuery(SCENE_QUERY_STAT(KalmalaMendingSight), false, Caster);
    VisibilityQuery.AddIgnoredActor(Target);
    if (GetWorld()->LineTraceTestByChannel(Start, Target->GetActorLocation(), ECC_Visibility, VisibilityQuery)) return nullptr;
    const bool bInRange = FVector::DistSquared(Source->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(MendingRange);
    return AKalmalaCharacter::IsMendingReceiveAllowed(true, true, Source->GetWorld() == Target->GetWorld(), bInRange,
        Target->GetHealth() > 1.0f, Target->GetHealth() < Target->GetMaximumHealth(), MendingHealAmount) ? Target : nullptr;
}
bool UKalmalaSupportMagicComponent::InfluenceNearbyDeerFromServer(APawn* Caster) const
{
    if (!Caster || !Caster->HasAuthority() || !GetWorld()) return false;
    constexpr float DeerCallRange = 900.0f;
    constexpr int32 DeerCallBudget = 3;
    int32 Influenced = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It && Influenced < DeerCallBudget; ++It)
    {
        AKalmalaWildlifeSpawn* Deer = *It;
        if (!IsValid(Deer) || Deer->GetArchetype() != EKalmalaWildlifeArchetype::Deer
            || Deer->IsDefeated() || Deer->GetBehaviour() != EKalmalaWildlifeBehaviour::Idle
            || FVector::DistSquared2D(Deer->GetActorLocation(), Caster->GetActorLocation()) > FMath::Square(DeerCallRange)) continue;
        if (Deer->ApplyDeerCallFromServer(Caster->GetActorLocation())) ++Influenced;
    }
    return Influenced > 0;
}

void UKalmalaSupportMagicComponent::SetFeedbackFromServer(const EKalmalaSupportFeedback NewFeedback, const float ServerNow)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !FMath::IsFinite(ServerNow)
        || NewFeedback == EKalmalaSupportFeedback::None) return;
    if (NewFeedback == EKalmalaSupportFeedback::Unavailable)
    {
        if (ServerNow < NextUnavailableFeedbackTime) return;
        NextUnavailableFeedbackTime = ServerNow + 0.5f;
    }
    else
    {
        NextUnavailableFeedbackTime = 0.0f;
    }
    Feedback = NewFeedback;
    FeedbackSerial = FeedbackSerial == TNumericLimits<uint32>::Max() ? 1 : FeedbackSerial + 1;
    GetOwner()->ForceNetUpdate();
}

bool UKalmalaSupportMagicComponent::ActivateSupportEffectFromHotbarServer(const EKalmalaSupportEffect Effect)
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    UWorld* World = GetWorld();
    if (!Pawn || !Pawn->HasAuthority() || !World || !IsKnownEffect(Effect)) return false;
    const float Now = World->GetTimeSeconds();
    UKalmalaCharacterMovementComponent* Movement = Cast<UKalmalaCharacterMovementComponent>(Pawn->GetMovementComponent());
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Pawn);
    const UKalmalaPlayerStatusComponent* Status = Character ? Character->GetStatusComponent() : nullptr;
    const float EffectiveActivationCost = Status ? Status->CalculateStaminaCost(ActivationCost, true) : ActivationCost;
    const bool bHasStamina = Movement != nullptr && FMath::IsFinite(EffectiveActivationCost)
        && EffectiveActivationCost > 0.0f && Movement->GetStamina() >= EffectiveActivationCost;
    const bool bBaseActivationAllowed = IsActivationAllowed(Pawn->HasAuthority(), HasLearnedEffect(Effect),
        true, Now >= CooldownExpiry, bHasStamina);
    const bool bOngoingEffectActive = ActiveEffect != EKalmalaSupportEffect::None
        && ActiveEffect != EKalmalaSupportEffect::Mending && ActiveEffectExpiry > Now;
    const bool bShieldAlreadyActive = HearthShieldExpiry > Now && HearthShieldStrength > 0.0f;
    const bool bVigorAlreadyActive = BearsVigorExpiry > Now && BearsVigorStrengthMultiplier > 1.0f;
    const bool bEffectActivationAllowed = !bOngoingEffectActive
        && (Effect == EKalmalaSupportEffect::HearthShield ? IsHearthShieldActivationAllowed(bBaseActivationAllowed, bShieldAlreadyActive)
        : Effect == EKalmalaSupportEffect::BearsVigor ? IsBearsVigorActivationAllowed(bBaseActivationAllowed, bVigorAlreadyActive)
        : bBaseActivationAllowed);
    if (!bEffectActivationAllowed) { SetFeedbackFromServer(EKalmalaSupportFeedback::Unavailable, Now); return false; }

    // Mending has no client target payload: the server forward trace finds one eligible allied pawn before charging.
    AKalmalaCharacter* MendingTarget = Effect == EKalmalaSupportEffect::Mending ? ResolveMendingTargetFromServer(Pawn) : nullptr;
    if (Effect == EKalmalaSupportEffect::Mending && MendingTarget == nullptr)
    {
        SetFeedbackFromServer(EKalmalaSupportFeedback::Unavailable, Now);
        return false;
    }
    // Deer selection and influence remain bounded and server-owned; validate it before charging.
    if (Effect == EKalmalaSupportEffect::DeerCall && !InfluenceNearbyDeerFromServer(Pawn))
    {
        SetFeedbackFromServer(EKalmalaSupportFeedback::Unavailable, Now);
        return false;
    }
    if (!Movement->TryConsumeStaminaFromServer(EffectiveActivationCost))
    {
        SetFeedbackFromServer(EKalmalaSupportFeedback::Unavailable, Now);
        return false;
    }
    if (MendingTarget != nullptr && !MendingTarget->ReceiveMendingFromServer(CastChecked<AKalmalaCharacter>(Pawn), MendingHealAmount))
    {
        SetFeedbackFromServer(EKalmalaSupportFeedback::Unavailable, Now);
        return false;
    }

    CooldownExpiry = Now + CooldownSeconds;
    ActiveEffect = Effect;
    if (Effect == EKalmalaSupportEffect::HearthShield)
    {
        HearthShieldStrength = HearthShieldAbsorption;
        HearthShieldExpiry = Now + HearthShieldDuration;
        ActiveEffectExpiry = HearthShieldExpiry;
    }
    else if (Effect == EKalmalaSupportEffect::BearsVigor)
    {
        BearsVigorStrengthMultiplier = BearsVigorStrength;
        BearsVigorExpiry = Now + BearsVigorDuration;
        if (!Movement->SetBearsVigorFromServer(true))
        {
            BearsVigorStrengthMultiplier = 1.0f;
            BearsVigorExpiry = 0.0f;
            CooldownExpiry = 0.0f;
            ActiveEffect = EKalmalaSupportEffect::None;
            ActiveEffectExpiry = 0.0f;
            SetFeedbackFromServer(EKalmalaSupportFeedback::Unavailable, Now);
            return false;
        }
        ActiveEffectExpiry = BearsVigorExpiry;
    }
    else if (Effect == EKalmalaSupportEffect::DeerCall)
    {
        ActiveEffectExpiry = Now + DeerCallDurationSeconds;
    }
    else
    {
        // Mending is an instantaneous transaction, not an ongoing active effect.
        ActiveEffect = EKalmalaSupportEffect::None;
        ActiveEffectExpiry = 0.0f;
    }
    SetFeedbackFromServer(EKalmalaSupportFeedback::Accepted, Now);
    GetOwner()->ForceNetUpdate();
    return true;
}
void UKalmalaSupportMagicComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld()) return;
    const float Now = GetWorld()->GetTimeSeconds();
    if (HearthShieldExpiry > 0.0f && Now >= HearthShieldExpiry)
    {
        HearthShieldStrength = 0.0f;
        HearthShieldExpiry = 0.0f;
    }
    if (BearsVigorExpiry > 0.0f && Now >= BearsVigorExpiry)
    {
        if (APawn* Pawn = Cast<APawn>(GetOwner()))
        {
            if (UKalmalaCharacterMovementComponent* Movement = Cast<UKalmalaCharacterMovementComponent>(Pawn->GetMovementComponent())) Movement->SetBearsVigorFromServer(false);
        }
        BearsVigorStrengthMultiplier = 1.0f;
        BearsVigorExpiry = 0.0f;
    }
    if (ActiveEffect != EKalmalaSupportEffect::None && Now >= ActiveEffectExpiry)
    {
        ActiveEffect = EKalmalaSupportEffect::None;
        ActiveEffectExpiry = 0.0f;
        GetOwner()->ForceNetUpdate();
    }
}
void UKalmalaSupportMagicComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME_CONDITION(UKalmalaSupportMagicComponent, LearnedMask, COND_OwnerOnly); DOREPLIFETIME(UKalmalaSupportMagicComponent, ActiveEffect); DOREPLIFETIME(UKalmalaSupportMagicComponent, ActiveEffectExpiry); DOREPLIFETIME(UKalmalaSupportMagicComponent, HearthShieldStrength); DOREPLIFETIME(UKalmalaSupportMagicComponent, HearthShieldExpiry); DOREPLIFETIME(UKalmalaSupportMagicComponent, BearsVigorExpiry); DOREPLIFETIME(UKalmalaSupportMagicComponent, BearsVigorStrengthMultiplier); DOREPLIFETIME_CONDITION(UKalmalaSupportMagicComponent, CooldownExpiry, COND_OwnerOnly); DOREPLIFETIME_CONDITION(UKalmalaSupportMagicComponent, Feedback, COND_OwnerOnly); DOREPLIFETIME_CONDITION(UKalmalaSupportMagicComponent, FeedbackSerial, COND_OwnerOnly); }
