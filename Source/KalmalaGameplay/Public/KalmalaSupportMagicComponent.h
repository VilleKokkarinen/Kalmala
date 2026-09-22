#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaSupportMagicComponent.generated.h"

UENUM(BlueprintType)
enum class EKalmalaSupportEffect : uint8 { None, Mending, HearthShield, BearsVigor, DeerCall };

UENUM(BlueprintType)
enum class EKalmalaSupportFeedback : uint8 { None, Accepted, Unavailable };

/** Server-owned learned effects and their bounded, relevant presentation state. */
UCLASS(ClassGroup=(Kalmala), meta=(BlueprintSpawnableComponent))
class KALMALAGAMEPLAY_API UKalmalaSupportMagicComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UKalmalaSupportMagicComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    UFUNCTION(Server, Reliable) void ServerRequestActivateSupportEffect(EKalmalaSupportEffect Effect, uint32 RequestSequence);
    bool LearnEffectFromServer(EKalmalaSupportEffect Effect);
    bool HasLearnedEffect(EKalmalaSupportEffect Effect) const;
    EKalmalaSupportEffect GetActiveEffect() const { return ActiveEffect; }
    float GetActiveEffectExpiry() const { return ActiveEffectExpiry; }
    float GetCooldownExpiry() const { return CooldownExpiry; }
    EKalmalaSupportFeedback GetFeedback() const { return Feedback; }
    uint32 GetFeedbackSerial() const { return FeedbackSerial; }
    float GetHearthShieldStrength() const { return HearthShieldStrength; }
    float GetHearthShieldExpiry() const { return HearthShieldExpiry; }
    float GetBearsVigorExpiry() const { return BearsVigorExpiry; }
    float GetBearsVigorStrengthMultiplier() const { return BearsVigorStrengthMultiplier; }
    float AbsorbHearthShieldDamageFromServer(float IncomingDamage);
    static FString CanonicalId(EKalmalaSupportEffect Effect);
    static EKalmalaSupportEffect FromScrollDefinition(const FString& Definition);
    static bool IsKnownEffect(EKalmalaSupportEffect Effect);
    static bool IsNonDamagingEffect(EKalmalaSupportEffect Effect);
    static bool IsActivationAllowed(bool bAuthority, bool bLearned, bool bNewSequence, bool bCooldownExpired, bool bHasStamina);
    static bool IsHearthShieldActivationAllowed(bool bBaseActivationAllowed, bool bShieldAlreadyActive);
    static bool IsBearsVigorActivationAllowed(bool bBaseActivationAllowed, bool bVigorAlreadyActive);
    static bool IsDeerCallActivationAllowed(bool bBaseActivationAllowed, bool bHasEligibleDeer);
    static float CalculateHearthShieldAbsorption(bool bServerAuthority, bool bShieldActive, float IncomingDamage, float RemainingStrength);
    static float CalculateBearsVigorDamage(bool bServerAuthority, bool bVigorActive, float BaseDamage, float StrengthMultiplier);
    // Shared base cost used by every learned effect before server-owned status modifiers.
    static constexpr float ActivationCost = 18.0f;
private:
    static constexpr float CooldownSeconds = 4.0f;
    static constexpr float PresentationSeconds = 2.0f;
    static constexpr float MendingRange = 350.0f;
    static constexpr float MendingHealAmount = 30.0f;
    static constexpr float HearthShieldDuration = 8.0f;
    static constexpr float HearthShieldAbsorption = 40.0f;
    static constexpr float BearsVigorDuration = 8.0f;
    static constexpr float BearsVigorStrength = 1.4f;
    class AKalmalaCharacter* ResolveMendingTargetFromServer(class APawn* Caster) const;
    bool InfluenceNearbyDeerFromServer(class APawn* Caster) const;
    void SetFeedbackFromServer(EKalmalaSupportFeedback NewFeedback, float ServerNow);
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") uint8 LearnedMask = 0;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") EKalmalaSupportEffect ActiveEffect = EKalmalaSupportEffect::None;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") float ActiveEffectExpiry = 0.0f;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") float HearthShieldStrength = 0.0f;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") float HearthShieldExpiry = 0.0f;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") float BearsVigorExpiry = 0.0f;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") float BearsVigorStrengthMultiplier = 1.0f;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") float CooldownExpiry = 0.0f;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") EKalmalaSupportFeedback Feedback = EKalmalaSupportFeedback::None;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") uint32 FeedbackSerial = 0;
    uint32 LastRequestSequence = 0;
    float NextUnavailableFeedbackTime = 0.0f;
};
