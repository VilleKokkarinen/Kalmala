#include "KalmalaWorldGenerationGameState.h"

#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "KalmalaRegionalGeneration.h"

AKalmalaWorldGenerationGameState::AKalmalaWorldGenerationGameState()
{
    WorldGenerationConfig.WorldSeed = 10323456789ull;
    WorldGenerationConfig.GeneratorRevision = FKalmalaWorldGenerationConfig::CurrentGeneratorRevision;
}

void AKalmalaWorldGenerationGameState::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (!HasAuthority())
    {
        return;
    }

    FParse::Value(FCommandLine::Get(), TEXT("WorldSeed="), WorldGenerationConfig.WorldSeed);
    FParse::Value(FCommandLine::Get(), TEXT("GeneratorRevision="), WorldGenerationConfig.GeneratorRevision);

    if (!WorldGenerationConfig.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid GeneratorRevision %d; falling back to revision %d."), WorldGenerationConfig.GeneratorRevision, FKalmalaWorldGenerationConfig::CurrentGeneratorRevision);
        WorldGenerationConfig.GeneratorRevision = FKalmalaWorldGenerationConfig::CurrentGeneratorRevision;
    }

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
    UE_LOG(LogTemp, Display, TEXT("%s world-generation identity: Seed=%llu Revision=%d."), Source, WorldGenerationConfig.WorldSeed, WorldGenerationConfig.GeneratorRevision);
    if (WorldGenerationConfig.GeneratorRevision >= 3 && FParse::Param(FCommandLine::Get(), TEXT("KalmalaRegionalVerification")))
    {
        uint64 Fingerprint = 1469598103934665603ull;
        auto Mix = [&](int64 Value) { Fingerprint = (Fingerprint ^ uint64(Value)) * 1099511628211ull; };
        for (int32 Y = -4; Y <= 4; ++Y) for (int32 X = -4; X <= 4; ++X)
        {
            const auto R = FKalmalaRegionalGeneration::Sample(WorldGenerationConfig, FVector2D(X * 25000, Y * 25000));
            Mix(R.Biome); Mix(FMath::RoundToInt64(R.Height * 1000)); Mix(FMath::RoundToInt64(R.WaterLevel * 1000));
            Mix(FMath::RoundToInt64(R.RiverWeight * 100000)); Mix(FMath::RoundToInt64(R.StreamWeight * 100000));
            for (float W : R.Weights) Mix(FMath::RoundToInt64(W * 100000));
        }
        UE_LOG(LogTemp, Display, TEXT("Regional verification Seed=%llu Revision=%d Fingerprint=%llu Samples=81"), WorldGenerationConfig.WorldSeed, WorldGenerationConfig.GeneratorRevision, Fingerprint);
    }
}
