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

    bool IsValid(float Maximum = 100000.0f) const;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resistance")
    FKalmalaDamageChannels Reductions;

    bool IsValid() const;
    /** Per-channel flat reductions, then general armor once per hit. Invalid profiles fail closed. */
    bool Resolve(const FKalmalaDamageChannels& Incoming, float& OutDamage) const;
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
