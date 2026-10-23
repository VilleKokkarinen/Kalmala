#pragma once
#include "CoreMinimal.h"
#include "KalmalaBiomeClassifier.h"

/** Server generation/mining rules approved on 2026-10-23. No ordinary ore co-drop. */
struct FKalmalaGemstoneContract
{
    static uint64 Mix(uint64 Value)
    {
        Value ^= Value >> 30; Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27; Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }
    static FName SelectReplacement(FName Source, EKalmalaBiome Biome, uint64 Seed)
    {
        const uint32 Roll = Mix(Seed ^ 0xA9C057A1ull) % 100;
        if (Source == TEXT("elderwood-copper-vein") && Biome == EKalmalaBiome::Elderwood)
            return Roll < 5 ? FName(TEXT("gem-quartz-rock")) : Source;
        if (Source == TEXT("lakes-tin-shore-gravel") && Biome == EKalmalaBiome::ShimmeringLakes)
            return Roll < 5 ? FName(TEXT("gem-quartz-rock")) : Roll < 7 ? FName(TEXT("gem-jade-rock")) : Source;
        if (Source == TEXT("mire-bog-iron") && Biome == EKalmalaBiome::MossyMire)
            return Roll < 3 ? FName(TEXT("gem-amethyst-rock")) : Source;
        if (Source == TEXT("mountains-slate-vein") && Biome == EKalmalaBiome::ThunderMountains)
            return Roll < 2 ? FName(TEXT("gem-tourmaline-rock")) : Source;
        if (Source == TEXT("tundra-frost-salt-deposit") && Biome == EKalmalaBiome::FreezingTundra)
            return Roll < 1 ? FName(TEXT("gem-opal-rock")) : Source;
        return Source;
    }
    static FName ItemForSource(FName Source)
    {
        if (Source == TEXT("gem-quartz-rock")) return TEXT("SmokyQuartz");
        if (Source == TEXT("gem-jade-rock")) return TEXT("GreenJade");
        if (Source == TEXT("gem-amethyst-rock")) return TEXT("Amethyst");
        if (Source == TEXT("gem-tourmaline-rock")) return TEXT("Tourmaline");
        if (Source == TEXT("gem-opal-rock")) return TEXT("StarOpal");
        return NAME_None;
    }
    static int32 MinimumPickTier(FName Source) { return Source == TEXT("gem-amethyst-rock") ? 2 : 1; }
    static bool IsGemSource(FName Source) { return !ItemForSource(Source).IsNone(); }
    /** Outcome: 0 intact, 1 shattered, 2 extracted, 3 claimed. Roll before final extraction too. */
    static bool Advance(uint8 Hits, uint8 Outcome, uint32 Roll, uint8& NextHits, uint8& NextOutcome)
    {
        if (Hits >= 3 || Outcome != 0 || Roll >= 10) return false;
        NextHits = Hits + 1;
        NextOutcome = Roll == 0 ? 1 : NextHits == 3 ? 2 : 0;
        return true;
    }
};
