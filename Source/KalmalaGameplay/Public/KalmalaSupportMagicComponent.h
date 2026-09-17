#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaSupportMagicComponent.generated.h"

UENUM(BlueprintType)
enum class EKalmalaSupportEffect : uint8 { None, Mending, HearthShield, BearsVigor, DeerCall };

/** Server-owned learned effects. Active state is presentation only until each effect increment supplies its result. */
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
    static FString CanonicalId(EKalmalaSupportEffect Effect);
    static EKalmalaSupportEffect FromScrollDefinition(const FString& Definition);
    static bool IsActivationAllowed(bool bAuthority, bool bLearned, bool bNewSequence, bool bCooldownExpired, bool bHasStamina);
private:
    static constexpr float ActivationCost = 20.0f;
    static constexpr float CooldownSeconds = 4.0f;
    static constexpr float PresentationSeconds = 2.0f;
    static constexpr float MendingRange = 350.0f;
    static constexpr float MendingHealAmount = 30.0f;
    class AKalmalaCharacter* ResolveMendingTargetFromServer(class APawn* Caster) const;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") uint8 LearnedMask = 0;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") EKalmalaSupportEffect ActiveEffect = EKalmalaSupportEffect::None;
    UPROPERTY(Replicated, VisibleAnywhere, Category="Support") float ActiveEffectExpiry = 0.0f;
    uint32 LastRequestSequence = 0;
    float CooldownExpiry = 0.0f;
};
