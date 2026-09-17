#include "KalmalaSupportMagicComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

UKalmalaSupportMagicComponent::UKalmalaSupportMagicComponent() { PrimaryComponentTick.bCanEverTick = true; SetIsReplicatedByDefault(true); }
FString UKalmalaSupportMagicComponent::CanonicalId(const EKalmalaSupportEffect Effect)
{
    switch (Effect) { case EKalmalaSupportEffect::Mending: return TEXT("Effect:mending"); case EKalmalaSupportEffect::HearthShield: return TEXT("Effect:hearth-shield"); case EKalmalaSupportEffect::BearsVigor: return TEXT("Effect:bears-vigor"); case EKalmalaSupportEffect::DeerCall: return TEXT("Effect:deer-call"); default: return FString(); }
}
EKalmalaSupportEffect UKalmalaSupportMagicComponent::FromScrollDefinition(const FString& Definition)
{ if (Definition == TEXT("mending")) return EKalmalaSupportEffect::Mending; if (Definition == TEXT("hearth-shield")) return EKalmalaSupportEffect::HearthShield; if (Definition == TEXT("bears-vigor")) return EKalmalaSupportEffect::BearsVigor; if (Definition == TEXT("deer-call")) return EKalmalaSupportEffect::DeerCall; return EKalmalaSupportEffect::None; }
bool UKalmalaSupportMagicComponent::LearnEffectFromServer(const EKalmalaSupportEffect Effect)
{ if (!GetOwner() || !GetOwner()->HasAuthority() || CanonicalId(Effect).IsEmpty()) return false; LearnedMask |= static_cast<uint8>(1u << static_cast<uint8>(Effect)); GetOwner()->ForceNetUpdate(); return true; }
bool UKalmalaSupportMagicComponent::HasLearnedEffect(const EKalmalaSupportEffect Effect) const
{ return !CanonicalId(Effect).IsEmpty() && (LearnedMask & static_cast<uint8>(1u << static_cast<uint8>(Effect))) != 0; }
bool UKalmalaSupportMagicComponent::IsActivationAllowed(const bool bAuthority, const bool bLearned, const bool bNewSequence, const bool bCooldownExpired, const bool bHasStamina)
{ return bAuthority && bLearned && bNewSequence && bCooldownExpired && bHasStamina; }
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
        Target->GetHealth() > 1.0f, Target->GetHealth() < 100.0f, MendingHealAmount) ? Target : nullptr;
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
void UKalmalaSupportMagicComponent::ServerRequestActivateSupportEffect_Implementation(const EKalmalaSupportEffect Effect, const uint32 RequestSequence)
{
    APawn* Pawn = Cast<APawn>(GetOwner()); const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    UKalmalaCharacterMovementComponent* Movement = Pawn ? Cast<UKalmalaCharacterMovementComponent>(Pawn->GetMovementComponent()) : nullptr;
    const bool bBaseActivationAllowed = IsActivationAllowed(Pawn && Pawn->HasAuthority(), HasLearnedEffect(Effect), RequestSequence != 0 && RequestSequence > LastRequestSequence, Now >= CooldownExpiry, Movement != nullptr && Movement->GetStamina() >= ActivationCost);
    const bool bShieldAlreadyActive = HearthShieldExpiry > Now && HearthShieldStrength > 0.0f;
    const bool bVigorAlreadyActive = BearsVigorExpiry > Now && BearsVigorStrengthMultiplier > 1.0f;
    const bool bEffectActivationAllowed = Effect == EKalmalaSupportEffect::HearthShield ? IsHearthShieldActivationAllowed(bBaseActivationAllowed, bShieldAlreadyActive)
        : Effect == EKalmalaSupportEffect::BearsVigor ? IsBearsVigorActivationAllowed(bBaseActivationAllowed, bVigorAlreadyActive) : bBaseActivationAllowed;
    if (!bEffectActivationAllowed) return;
    // Mending has no client target payload: the server forward trace finds one eligible allied pawn before the transaction charges stamina.
    AKalmalaCharacter* MendingTarget = Effect == EKalmalaSupportEffect::Mending ? ResolveMendingTargetFromServer(Pawn) : nullptr;
    if (Effect == EKalmalaSupportEffect::Mending && MendingTarget == nullptr) return;
    if (Effect == EKalmalaSupportEffect::DeerCall && !InfluenceNearbyDeerFromServer(Pawn)) return;
    if (!Movement->TryConsumeStaminaFromServer(ActivationCost)) return;
    if (MendingTarget != nullptr && !MendingTarget->ReceiveMendingFromServer(CastChecked<AKalmalaCharacter>(Pawn), MendingHealAmount)) return;
    LastRequestSequence = RequestSequence; CooldownExpiry = Now + CooldownSeconds; ActiveEffect = Effect;
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
        if (!Movement->SetBearsVigorFromServer(true)) return;
        ActiveEffectExpiry = BearsVigorExpiry;
    }
    else ActiveEffectExpiry = Now + PresentationSeconds;
    GetOwner()->ForceNetUpdate();
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
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME_CONDITION(UKalmalaSupportMagicComponent, LearnedMask, COND_OwnerOnly); DOREPLIFETIME(UKalmalaSupportMagicComponent, ActiveEffect); DOREPLIFETIME(UKalmalaSupportMagicComponent, ActiveEffectExpiry); DOREPLIFETIME(UKalmalaSupportMagicComponent, HearthShieldStrength); DOREPLIFETIME(UKalmalaSupportMagicComponent, HearthShieldExpiry); DOREPLIFETIME(UKalmalaSupportMagicComponent, BearsVigorExpiry); DOREPLIFETIME(UKalmalaSupportMagicComponent, BearsVigorStrengthMultiplier); }
