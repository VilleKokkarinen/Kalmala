#include "KalmalaDamageProfile.h"

bool FKalmalaDamageChannels::IsValid(const float Maximum) const
{
    return FMath::IsFinite(Maximum) && Maximum >= 0.0f
        && FMath::IsFinite(Slash) && Slash >= 0.0f && Slash <= Maximum
        && FMath::IsFinite(Stab) && Stab >= 0.0f && Stab <= Maximum
        && FMath::IsFinite(Crush) && Crush >= 0.0f && Crush <= Maximum
        && FMath::IsFinite(Pierce) && Pierce >= 0.0f && Pierce <= Maximum
        && FMath::IsFinite(Fire) && Fire >= 0.0f && Fire <= Maximum
        && FMath::IsFinite(Water) && Water >= 0.0f && Water <= Maximum
        && FMath::IsFinite(Holy) && Holy >= 0.0f && Holy <= Maximum
        && FMath::IsFinite(Poison) && Poison >= 0.0f && Poison <= Maximum;
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
    return FMath::IsFinite(Armor) && Armor >= 0.0f && Armor <= 100000.0f && Reductions.IsValid();
}

bool FKalmalaResistanceProfile::Resolve(const FKalmalaDamageChannels& Incoming, float& OutDamage) const
{
    OutDamage = 0.0f;
    if (!IsValid() || !Incoming.IsValid()) return false;
    float Remaining = 0.0f;
    Remaining += FMath::Max(0.0f, Incoming.Slash - Reductions.Slash);
    Remaining += FMath::Max(0.0f, Incoming.Stab - Reductions.Stab);
    Remaining += FMath::Max(0.0f, Incoming.Crush - Reductions.Crush);
    Remaining += FMath::Max(0.0f, Incoming.Pierce - Reductions.Pierce);
    Remaining += FMath::Max(0.0f, Incoming.Fire - Reductions.Fire);
    Remaining += FMath::Max(0.0f, Incoming.Water - Reductions.Water);
    Remaining += FMath::Max(0.0f, Incoming.Holy - Reductions.Holy);
    Remaining += FMath::Max(0.0f, Incoming.Poison - Reductions.Poison);
    OutDamage = FMath::Max(0.0f, Remaining - Armor);
    return true;
}
