#include "KalmalaWorldGenerationGameState.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "KalmalaRegionalGeneration.h"

AKalmalaWorldGenerationGameState::AKalmalaWorldGenerationGameState()
{
    WorldGenerationConfig.WorldSeed = 10323456789ull;
}

void AKalmalaWorldGenerationGameState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (!HasAuthority())
    {
        return;
    }

    FParse::Value(FCommandLine::Get(), TEXT("WorldSeed="), WorldGenerationConfig.WorldSeed);
    ForceNetUpdate();
    LogWorldGenerationIdentity(TEXT("Server selected"));
}

void AKalmalaWorldGenerationGameState::OnRep_WorldGenerationConfig()
{
    LogWorldGenerationIdentity(TEXT("Client received"));
}

bool AKalmalaWorldGenerationGameState::IsWeatherUpdateAllowed(const bool bServerAuthority)
{
    return bServerAuthority;
}

void AKalmalaWorldGenerationGameState::SetWeatherStateFromServer(const FKalmalaWeatherState& InWeatherState)
{
    check(IsWeatherUpdateAllowed(HasAuthority()));
    check(InWeatherState.IsValid());
    WeatherState = InWeatherState;
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("Server selected weather cycle %d: Start=%.2f Duration=%.2f Precipitation=%.2f WindDirection=%d WindStrength=%.2f."), WeatherState.WeatherCycleIndex, WeatherState.ServerStartTimeSeconds, WeatherState.DurationSeconds, WeatherState.PrecipitationIntensity, WeatherState.WindDirectionDegrees, WeatherState.WindStrength);
}

void AKalmalaWorldGenerationGameState::OnRep_WeatherState()
{
    UE_LOG(LogTemp, Display, TEXT("Client received weather cycle %d: Start=%.2f Duration=%.2f Precipitation=%.2f WindDirection=%d WindStrength=%.2f."), WeatherState.WeatherCycleIndex, WeatherState.ServerStartTimeSeconds, WeatherState.DurationSeconds, WeatherState.PrecipitationIntensity, WeatherState.WindDirectionDegrees, WeatherState.WindStrength);
}

void AKalmalaWorldGenerationGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AKalmalaWorldGenerationGameState, WorldGenerationConfig);
    DOREPLIFETIME(AKalmalaWorldGenerationGameState, WeatherState);
}

void AKalmalaWorldGenerationGameState::LogWorldGenerationIdentity(const TCHAR* Source) const
{
    UE_LOG(LogTemp, Display, TEXT("%s world-generation identity: Seed=%llu."), Source, WorldGenerationConfig.WorldSeed);
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaRegionalVerification")))
    {
        uint64 Fingerprint = 1469598103934665603ull;
        auto Mix = [&](int64 Value) { Fingerprint = (Fingerprint ^ uint64(Value)) * 1099511628211ull; };
        for (int32 Y = -4; Y <= 4; ++Y) for (int32 X = -4; X <= 4; ++X)
        {
            // Compare outer biomes as well as the protected centre across 20 km.
            const double Step = 250000.0;
            const auto R = FKalmalaRegionalGeneration::Sample(WorldGenerationConfig, FVector2D(X * Step, Y * Step));
            Mix(R.Biome); Mix(FMath::RoundToInt64(R.Height * 1000)); Mix(FMath::RoundToInt64(R.WaterLevel * 1000));
            Mix(FMath::RoundToInt64(R.RiverWeight * 100000)); Mix(FMath::RoundToInt64(R.StreamWeight * 100000));
            for (float W : R.Weights) Mix(FMath::RoundToInt64(W * 100000));
        }
        UE_LOG(LogTemp, Display, TEXT("Regional verification Seed=%llu Fingerprint=%llu Samples=81"), WorldGenerationConfig.WorldSeed, Fingerprint);
    }
}
