#include "KalmalaSupportMagicComponent.h"
#include "KalmalaCharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
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
void UKalmalaSupportMagicComponent::ServerRequestActivateSupportEffect_Implementation(const EKalmalaSupportEffect Effect, const uint32 RequestSequence)
{
    APawn* Pawn = Cast<APawn>(GetOwner()); const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    UKalmalaCharacterMovementComponent* Movement = Pawn ? Cast<UKalmalaCharacterMovementComponent>(Pawn->GetMovementComponent()) : nullptr;
    if (!IsActivationAllowed(Pawn && Pawn->HasAuthority(), HasLearnedEffect(Effect), RequestSequence != 0 && RequestSequence > LastRequestSequence, Now >= CooldownExpiry, Movement != nullptr && Movement->GetStamina() >= ActivationCost)) return;
    if (!Movement->TryConsumeStaminaFromServer(ActivationCost)) return;
    LastRequestSequence = RequestSequence; CooldownExpiry = Now + CooldownSeconds; ActiveEffect = Effect; ActiveEffectExpiry = Now + PresentationSeconds; GetOwner()->ForceNetUpdate();
}
void UKalmalaSupportMagicComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{ Super::TickComponent(DeltaTime, TickType, ThisTickFunction); if (GetOwner() && GetOwner()->HasAuthority() && ActiveEffect != EKalmalaSupportEffect::None && GetWorld() && GetWorld()->GetTimeSeconds() >= ActiveEffectExpiry) { ActiveEffect = EKalmalaSupportEffect::None; ActiveEffectExpiry = 0.0f; GetOwner()->ForceNetUpdate(); } }
void UKalmalaSupportMagicComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME_CONDITION(UKalmalaSupportMagicComponent, LearnedMask, COND_OwnerOnly); DOREPLIFETIME(UKalmalaSupportMagicComponent, ActiveEffect); DOREPLIFETIME(UKalmalaSupportMagicComponent, ActiveEffectExpiry); }
