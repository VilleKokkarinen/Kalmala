#pragma once

#include "CoreMinimal.h"
#include "KalmalaDamageProfile.generated.h"

/** Additive damage bonuses. Melee stab and ranged pierce are distinct channels. */
USTRUCT(BlueprintType)
struct KALMALAGAMEPLAY_API FKalmalaDamageChannels
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Slash = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Stab = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Crush = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Pierce = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Fire = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Water = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Holy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
    float Poison = 0.0f;

    bool IsValid(float Maximum = 100000.0f, float Minimum = 0.0f) const;
    float Total() const;
    FKalmalaDamageChannels Added(const FKalmalaDamageChannels& Other) const;
    FKalmalaDamageChannels Scaled(float Multiplier) const;
    FString Describe() const;
};

USTRUCT(BlueprintType)
struct KALMALAGAMEPLAY_API FKalmalaResistanceProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resistance")
    float Armor = 0.0f;

    /** Negative channel reductions are flat weaknesses; inactive channels add no damage. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resistance")
    FKalmalaDamageChannels Reductions;

    bool IsValid() const;
    FKalmalaResistanceProfile Added(const FKalmalaResistanceProfile& Other) const;
    FKalmalaResistanceProfile Scaled(float Multiplier) const;
    /** Optional player armor reduces only the physical subtotal once, before existing entity armor. */
    bool Resolve(const FKalmalaDamageChannels& Incoming, float& OutDamage, int32 PhysicalArmor = 0) const;
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaWeaponAttackData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName AttackId;

    UPROPERTY(EditAnywhere)
    FKalmalaDamageChannels DamageBonuses;
};
