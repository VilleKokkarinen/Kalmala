#include "KalmalaDamageProfile.h"

bool FKalmalaDamageChannels::IsValid(const float Maximum, const float Minimum) const
{
    return FMath::IsFinite(Maximum) && FMath::IsFinite(Minimum) && Maximum >= Minimum
        && FMath::IsFinite(Slash) && Slash >= Minimum && Slash <= Maximum
        && FMath::IsFinite(Stab) && Stab >= Minimum && Stab <= Maximum
        && FMath::IsFinite(Crush) && Crush >= Minimum && Crush <= Maximum
        && FMath::IsFinite(Pierce) && Pierce >= Minimum && Pierce <= Maximum
        && FMath::IsFinite(Fire) && Fire >= Minimum && Fire <= Maximum
        && FMath::IsFinite(Water) && Water >= Minimum && Water <= Maximum
        && FMath::IsFinite(Holy) && Holy >= Minimum && Holy <= Maximum
        && FMath::IsFinite(Poison) && Poison >= Minimum && Poison <= Maximum;
}

float FKalmalaDamageChannels::Total() const
{
    return Slash + Stab + Crush + Pierce + Fire + Water + Holy + Poison;
}

FKalmalaDamageChannels FKalmalaDamageChannels::Added(const FKalmalaDamageChannels& Other) const
{
    FKalmalaDamageChannels Result;
    Result.Slash = Slash + Other.Slash;
    Result.Stab = Stab + Other.Stab;
    Result.Crush = Crush + Other.Crush;
    Result.Pierce = Pierce + Other.Pierce;
    Result.Fire = Fire + Other.Fire;
    Result.Water = Water + Other.Water;
    Result.Holy = Holy + Other.Holy;
    Result.Poison = Poison + Other.Poison;
    return Result;
}

FKalmalaDamageChannels FKalmalaDamageChannels::Scaled(const float Multiplier) const
{
    FKalmalaDamageChannels Result;
    Result.Slash = Slash * Multiplier;
    Result.Stab = Stab * Multiplier;
    Result.Crush = Crush * Multiplier;
    Result.Pierce = Pierce * Multiplier;
    Result.Fire = Fire * Multiplier;
    Result.Water = Water * Multiplier;
    Result.Holy = Holy * Multiplier;
    Result.Poison = Poison * Multiplier;
    return Result;
}

FString FKalmalaDamageChannels::Describe() const
{
    TArray<FString> Parts;
    if (Slash > 0.0f) Parts.Add(FString::Printf(TEXT("Slash bonus %.0f"), Slash));
    if (Stab > 0.0f) Parts.Add(FString::Printf(TEXT("Stab bonus %.0f"), Stab));
    if (Crush > 0.0f) Parts.Add(FString::Printf(TEXT("Crush bonus %.0f"), Crush));
    if (Pierce > 0.0f) Parts.Add(FString::Printf(TEXT("Pierce bonus %.0f"), Pierce));
    if (Fire > 0.0f) Parts.Add(FString::Printf(TEXT("Fire bonus %.0f"), Fire));
    if (Water > 0.0f) Parts.Add(FString::Printf(TEXT("Water bonus %.0f"), Water));
    if (Holy > 0.0f) Parts.Add(FString::Printf(TEXT("Holy bonus %.0f"), Holy));
    if (Poison > 0.0f) Parts.Add(FString::Printf(TEXT("Poison bonus %.0f"), Poison));
    return Parts.IsEmpty() ? TEXT("No damage bonuses") : FString::Join(Parts, TEXT(" + "));
}

bool FKalmalaResistanceProfile::IsValid() const
{
    return FMath::IsFinite(Armor) && Armor >= 0.0f && Armor <= 100000.0f && Reductions.IsValid(100000.0f, -100000.0f);
}

FKalmalaResistanceProfile FKalmalaResistanceProfile::Added(const FKalmalaResistanceProfile& Other) const
{
    FKalmalaResistanceProfile Result;
    Result.Armor = Armor + Other.Armor;
    Result.Reductions = Reductions.Added(Other.Reductions);
    return Result;
}

FKalmalaResistanceProfile FKalmalaResistanceProfile::Scaled(const float Multiplier) const
{
    FKalmalaResistanceProfile Result;
    Result.Armor = Armor * Multiplier;
    Result.Reductions = Reductions.Scaled(Multiplier);
    return Result;
}

bool FKalmalaResistanceProfile::Resolve(const FKalmalaDamageChannels& Incoming, float& OutDamage, const int32 PhysicalArmor) const
{
    OutDamage = 0.0f;
    if (!IsValid() || !Incoming.IsValid() || PhysicalArmor < 0 || PhysicalArmor > 6000) return false;
    float Remaining = 0.0f;
    if (Incoming.Slash > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Slash - Reductions.Slash);
    if (Incoming.Stab > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Stab - Reductions.Stab);
    if (Incoming.Crush > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Crush - Reductions.Crush);
    if (Incoming.Pierce > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Pierce - Reductions.Pierce);
    // Wearable armor applies once to the physical subtotal; excess cannot absorb elements.
    Remaining = FMath::Max(0.0f, Remaining - PhysicalArmor);
    if (Incoming.Fire > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Fire - Reductions.Fire);
    if (Incoming.Water > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Water - Reductions.Water);
    if (Incoming.Holy > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Holy - Reductions.Holy);
    if (Incoming.Poison > 0.0f) Remaining += FMath::Max(0.0f, Incoming.Poison - Reductions.Poison);
    OutDamage = FMath::Max(0.0f, Remaining - Armor);
    return true;
}
