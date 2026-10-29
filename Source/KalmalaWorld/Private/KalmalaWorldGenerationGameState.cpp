#include "KalmalaWorldGenerationGameState.h"

#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaM7PersistenceContract.h"

namespace
{
    bool TryParseUnsignedDecimalOption(const TCHAR* Text, uint64& OutValue)
    {
        OutValue = 0;
        if (Text == nullptr || *Text == TEXT('\0')) return false;

        uint64 Parsed = 0;
        for (const TCHAR* Character = Text; *Character != TEXT('\0'); ++Character)
        {
            if (*Character < TEXT('0') || *Character > TEXT('9')) return false;
            const uint64 Digit = static_cast<uint64>(*Character - TEXT('0'));
            if (Parsed > (MAX_uint64 - Digit) / 10ull) return false;
            Parsed = Parsed * 10ull + Digit;
        }
        OutValue = Parsed;
        return true;
    }
}

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

    const FURL& TravelUrl = GetWorld()->URL;
    const TCHAR* MenuLaunchOption = TravelUrl.GetOption(TEXT("KalmalaMenuLaunch="), nullptr);
    const TCHAR* MenuSeedOption = TravelUrl.GetOption(TEXT("KalmalaMenuSeed="), nullptr);
    const TCHAR* MenuRevisionOption = TravelUrl.GetOption(TEXT("KalmalaMenuRevision="), nullptr);
    const bool bHasMenuLaunchOption = MenuLaunchOption != nullptr
        || MenuSeedOption != nullptr || MenuRevisionOption != nullptr;
    if (bHasMenuLaunchOption)
    {
        uint64 Seed = 0;
        uint64 Revision = 0;
        const bool bSoloLaunch = MenuLaunchOption != nullptr
            && FCString::Stricmp(MenuLaunchOption, TEXT("Solo")) == 0
            && GetWorld()->GetNetMode() == NM_Standalone;
        const bool bListenHostLaunch = MenuLaunchOption != nullptr
            && FCString::Stricmp(MenuLaunchOption, TEXT("HostCoop")) == 0
            && GetWorld()->GetNetMode() == NM_ListenServer;
        const bool bValidMenuOptions = (bSoloLaunch || bListenHostLaunch)
            && TryParseUnsignedDecimalOption(MenuSeedOption, Seed)
            && TryParseUnsignedDecimalOption(MenuRevisionOption, Revision)
            && Revision <= static_cast<uint64>(MAX_int32)
            && Revision == static_cast<uint64>(FKalmalaM7SaveIdentity::ForWorld(Seed).GeneratorRevision);
        if (!bValidMenuOptions || !SelectWorldSeedFromServer(Seed))
        {
            UE_LOG(LogTemp, Error, TEXT("Rejected invalid, stale, or network-mode-mismatched server-owned menu launch identity options."));
        }
        return;
    }

    FParse::Value(FCommandLine::Get(), TEXT("WorldSeed="), WorldGenerationConfig.WorldSeed);
    ForceNetUpdate();
    LogWorldGenerationIdentity(TEXT("Server selected"));
}

bool AKalmalaWorldGenerationGameState::SelectWorldSeedFromServer(const uint64 Seed)
{
    if (!HasAuthority() || HasActorBegunPlay() || bServerWorldSeedSelectionAccepted) return false;

    WorldGenerationConfig.WorldSeed = Seed;
    bServerWorldSeedSelectionAccepted = true;
    ForceNetUpdate();
    LogWorldGenerationIdentity(TEXT("Server accepted requested seed"));
    return true;
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
    if (!IsWeatherUpdateAllowed(HasAuthority())) return;
    FKalmalaWeatherState AcceptedWeather = InWeatherState;
    AcceptedWeather.RefreshActivityLevel();
    if (!AcceptedWeather.IsValid()) return;
    WeatherState = AcceptedWeather;
    ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("Server selected weather cycle %d: Start=%.2f Duration=%.2f Precipitation=%.2f Fog=%.2f Storm=%.2f WindDirection=%d WindStrength=%.2f Activity=%d."), WeatherState.WeatherCycleIndex, WeatherState.ServerStartTimeSeconds, WeatherState.DurationSeconds, WeatherState.PrecipitationIntensity, WeatherState.FogIntensity, WeatherState.GetStormIntensity(), WeatherState.WindDirectionDegrees, WeatherState.WindStrength, static_cast<uint8>(WeatherState.ActivityLevel));
}

void AKalmalaWorldGenerationGameState::OnRep_WeatherState()
{
    UE_LOG(LogTemp, Display, TEXT("Client received weather cycle %d: Start=%.2f Duration=%.2f Precipitation=%.2f Fog=%.2f Storm=%.2f WindDirection=%d WindStrength=%.2f Activity=%d."), WeatherState.WeatherCycleIndex, WeatherState.ServerStartTimeSeconds, WeatherState.DurationSeconds, WeatherState.PrecipitationIntensity, WeatherState.FogIntensity, WeatherState.GetStormIntensity(), WeatherState.WindDirectionDegrees, WeatherState.WindStrength, static_cast<uint8>(WeatherState.ActivityLevel));
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
