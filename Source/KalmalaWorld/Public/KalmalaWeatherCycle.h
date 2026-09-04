#pragma once

#include "CoreMinimal.h"
#include "KalmalaWeatherState.h"
#include "KalmalaWorldGenerationConfig.h"

/** Pure deterministic selection of one weather interval from immutable world identity. */
struct KALMALAWORLD_API FKalmalaWeatherCycle
{
    static FKalmalaWeatherState DeriveState(const FKalmalaWorldGenerationConfig& Config, const int32 WeatherCycleIndex, const float ServerStartTimeSeconds)
    {
        check(Config.IsValid());
        check(WeatherCycleIndex >= 0);

        const uint64 Seed = DeriveWeatherSeed(Config, WeatherCycleIndex);
        FRandomStream RandomStream(static_cast<int32>(Seed ^ (Seed >> 32)));

        FKalmalaWeatherState State;
        State.WeatherCycleIndex = WeatherCycleIndex;
        State.ServerStartTimeSeconds = ServerStartTimeSeconds;
        State.DurationSeconds = RandomStream.FRandRange(120.0f, 240.0f);
        State.WindDirectionDegrees = 45 * RandomStream.RandRange(0, 7);

        const float Outcome = RandomStream.FRand();
        if (Outcome < 0.45f)
        {
            State.PrecipitationIntensity = 0.0f;
            State.WindStrength = RandomStream.FRandRange(0.05f, 0.35f);
        }
        else if (Outcome < 0.75f)
        {
            State.PrecipitationIntensity = RandomStream.FRandRange(0.15f, 0.55f);
            State.WindStrength = RandomStream.FRandRange(0.15f, 0.60f);
        }
        else
        {
            State.PrecipitationIntensity = RandomStream.FRandRange(0.60f, 1.0f);
            State.WindStrength = RandomStream.FRandRange(0.40f, 1.0f);
        }
        return State;
    }

private:
    static uint64 DeriveWeatherSeed(const FKalmalaWorldGenerationConfig& Config, const int32 WeatherCycleIndex)
    {
        uint64 Value = Config.WorldSeed ^ (static_cast<uint64>(Config.GeneratorRevision) << 32)
            ^ 0xC6A4A7935BD1E995ull ^ (0x9E3779B97F4A7C15ull * (static_cast<uint64>(WeatherCycleIndex) + 1ull));
        Value ^= Value >> 30;
        Value *= 0xBF58476D1CE4E5B9ull;
        Value ^= Value >> 27;
        Value *= 0x94D049BB133111EBull;
        return Value ^ (Value >> 31);
    }
};
