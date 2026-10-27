#include "KalmalaWeatherActivitySubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/World.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaWeatherActivityWidget.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    constexpr float MountainWeatherFogProbeIntervalSeconds = 1.0f;
    constexpr float MountainWeatherFogMaxDensity = 0.004f;
    constexpr float MountainWeatherFogMaxOpacity = 0.20f;
    constexpr float MountainWeatherFogStartDistance = 2500.0f;
    constexpr float MountainWeatherFogFadeSpeed = 1.5f;
    constexpr float MountainWeatherFogReleaseThreshold = 0.005f;
    constexpr float WeatherMaterialResponseFadeSpeed = 0.75f;
    const FName WeatherMaterialPrecipitationParameter(TEXT("PrecipitationIntensity"));
    const FLinearColor MountainWeatherFogColor(0.32f, 0.34f, 0.41f);
}

void UKalmalaWeatherActivitySubsystem::Tick(const float DeltaTime)
{
    // The survival subsystem owns the single top-right weather/status parent.
    ReleaseController();
    UpdateMountainWeatherFog(DeltaTime);
    UpdateWeatherMaterialResponse(DeltaTime);
}

void UKalmalaWeatherActivitySubsystem::UpdateMountainWeatherFog(const float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr
        || World->GetNetMode() == NM_DedicatedServer)
    {
        DestroyMountainWeatherFog();
        return;
    }
    if (MountainWeatherFog != nullptr
        && (!IsValid(MountainWeatherFog.Get()) || MountainWeatherFog->GetWorld() != World))
    {
        DestroyMountainWeatherFog();
    }

    MountainWeatherFogProbeTimeRemaining -= FMath::Max(DeltaTime, 0.0f);
    if (MountainWeatherFogProbeTimeRemaining <= 0.0f)
    {
        TargetMountainWeatherFogCoverage = 0.0f;
        APlayerController* Controller = LocalPlayer->GetPlayerController(World);
        const AKalmalaWorldGenerationGameState* GameState =
            World->GetGameState<AKalmalaWorldGenerationGameState>();
        const APawn* Pawn = Controller != nullptr && Controller->IsLocalController()
            ? Controller->GetPawn() : nullptr;
        if (GameState != nullptr && GameState->GetWorldGenerationConfig().IsValid()
            && Pawn != nullptr && !Pawn->GetActorLocation().ContainsNaN())
        {
            const FKalmalaWeatherState& Weather = GameState->GetWeatherState();
            const double ServerTime = GameState->GetServerWorldTimeSeconds();
            const bool bWeatherIntervalActive = Weather.IsValid() && FMath::IsFinite(ServerTime)
                && ServerTime >= Weather.ServerStartTimeSeconds
                && ServerTime < Weather.ServerStartTimeSeconds + Weather.DurationSeconds;
            if (bWeatherIntervalActive)
            {
                const FVector PawnLocation = Pawn->GetActorLocation();
                const FKalmalaRegionalSample Surface = FKalmalaRegionalGeneration::Sample(
                    GameState->GetWorldGenerationConfig(), FVector2D(PawnLocation.X, PawnLocation.Y));
                const float MountainWeight = Surface.Weights[
                    static_cast<uint8>(EKalmalaBiome::ThunderMountains)];
                TargetMountainWeatherFogCoverage = FMath::Clamp(
                    FMath::Clamp(Weather.FogIntensity, 0.0f, 1.0f)
                        * FMath::Clamp(MountainWeight, 0.0f, 1.0f),
                    0.0f, 1.0f);
            }
        }
        MountainWeatherFogProbeTimeRemaining = MountainWeatherFogProbeIntervalSeconds;
    }

    if (TargetMountainWeatherFogCoverage > MountainWeatherFogReleaseThreshold && MountainWeatherFog == nullptr)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.ObjectFlags |= RF_Transient;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        MountainWeatherFog = World->SpawnActor<AExponentialHeightFog>(
            AExponentialHeightFog::StaticClass(), FTransform::Identity, SpawnParameters);
        if (MountainWeatherFog != nullptr)
        {
            MountainWeatherFog->SetReplicates(false);
            MountainWeatherFog->SetActorTickEnabled(false);
        }
    }

    if (MountainWeatherFog == nullptr || !IsValid(MountainWeatherFog.Get())) return;
    UExponentialHeightFogComponent* Fog = MountainWeatherFog->GetComponent();
    if (Fog == nullptr)
    {
        DestroyMountainWeatherFog();
        return;
    }

    CurrentMountainWeatherFogCoverage = FMath::FInterpTo(CurrentMountainWeatherFogCoverage,
        TargetMountainWeatherFogCoverage, DeltaTime, MountainWeatherFogFadeSpeed);
    Fog->SetFogDensity(MountainWeatherFogMaxDensity * CurrentMountainWeatherFogCoverage);
    Fog->SetFogMaxOpacity(MountainWeatherFogMaxOpacity * CurrentMountainWeatherFogCoverage);
    Fog->SetStartDistance(MountainWeatherFogStartDistance);
    Fog->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(
        FLinearColor::White, MountainWeatherFogColor, CurrentMountainWeatherFogCoverage));
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaBiomeBaselineCapture")))
    {
        static int32 LastLoggedFogStep = INDEX_NONE;
        const int32 FogStep = FMath::RoundToInt(CurrentMountainWeatherFogCoverage * 10.0f);
        if (FogStep != LastLoggedFogStep)
        {
            LastLoggedFogStep = FogStep;
            UE_LOG(LogTemp, Display,
                TEXT("Weather presentation fog capture sample: GameTime=%.2f TargetCoverage=%.2f CurrentCoverage=%.2f."),
                World->GetTimeSeconds(), TargetMountainWeatherFogCoverage, CurrentMountainWeatherFogCoverage);
        }
    }
#endif

    if (TargetMountainWeatherFogCoverage <= MountainWeatherFogReleaseThreshold
        && CurrentMountainWeatherFogCoverage <= MountainWeatherFogReleaseThreshold)
    {
        DestroyMountainWeatherFog();
    }
}

void UKalmalaWeatherActivitySubsystem::UpdateWeatherMaterialResponse(const float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr
        || World->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (WeatherMaterialCollection == nullptr)
    {
        WeatherMaterialCollection = LoadObject<UMaterialParameterCollection>(nullptr,
            TEXT("/Game/Kalmala/World/Materials/MPC_KalmalaWeather.MPC_KalmalaWeather"));
    }
    if (WeatherMaterialCollection == nullptr) return;

    UMaterialParameterCollectionInstance* Parameters =
        World->GetParameterCollectionInstance(WeatherMaterialCollection);
    if (Parameters == nullptr) return;

    float TargetResponse = 0.0f;
    bool bWeatherIntervalActive = false;
    const AKalmalaWorldGenerationGameState* GameState =
        World->GetGameState<AKalmalaWorldGenerationGameState>();
    if (GameState != nullptr)
    {
        const FKalmalaWeatherState& Weather = GameState->GetWeatherState();
        const double ServerTime = GameState->GetServerWorldTimeSeconds();
        bWeatherIntervalActive = Weather.IsValid() && FMath::IsFinite(ServerTime)
            && ServerTime >= Weather.ServerStartTimeSeconds
            && ServerTime < Weather.ServerStartTimeSeconds + Weather.DurationSeconds;
        if (bWeatherIntervalActive && FMath::IsFinite(Weather.PrecipitationIntensity))
        {
            TargetResponse = FMath::Clamp(Weather.PrecipitationIntensity, 0.0f, 1.0f);
        }
    }

    CurrentWeatherMaterialResponse = FMath::FInterpTo(CurrentWeatherMaterialResponse, TargetResponse,
        FMath::Max(DeltaTime, 0.0f), WeatherMaterialResponseFadeSpeed);
    Parameters->SetScalarParameterValue(WeatherMaterialPrecipitationParameter, CurrentWeatherMaterialResponse);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaBiomeBaselineCapture")))
    {
        static int32 LastLoggedResponseStep = INDEX_NONE;
        const int32 ResponseStep = FMath::RoundToInt(CurrentWeatherMaterialResponse * 10.0f);
        if (ResponseStep != LastLoggedResponseStep)
        {
            LastLoggedResponseStep = ResponseStep;
            UE_LOG(LogTemp, Display,
                TEXT("Weather presentation capture sample: GameTime=%.2f IntervalActive=%d TargetPrecipitation=%.2f MaterialResponse=%.2f."),
                World->GetTimeSeconds(), bWeatherIntervalActive ? 1 : 0, TargetResponse,
                CurrentWeatherMaterialResponse);
        }
    }
#endif
}

void UKalmalaWeatherActivitySubsystem::DestroyMountainWeatherFog()
{
    if (IsValid(MountainWeatherFog.Get())) MountainWeatherFog->Destroy();
    MountainWeatherFog = nullptr;
    TargetMountainWeatherFogCoverage = 0.0f;
    CurrentMountainWeatherFogCoverage = 0.0f;
    MountainWeatherFogProbeTimeRemaining = 0.0f;
}

void UKalmalaWeatherActivitySubsystem::ReleaseController()
{
    if (WeatherWidget != nullptr) WeatherWidget->RemoveFromParent();
    WeatherWidget = nullptr;
}

void UKalmalaWeatherActivitySubsystem::Deinitialize()
{
    ReleaseController();
    DestroyMountainWeatherFog();
    UWorld* World = GetWorld();
    if (World != nullptr && WeatherMaterialCollection != nullptr)
    {
        if (UMaterialParameterCollectionInstance* Parameters =
            World->GetParameterCollectionInstance(WeatherMaterialCollection))
        {
            Parameters->SetScalarParameterValue(WeatherMaterialPrecipitationParameter, 0.0f);
        }
    }
    CurrentWeatherMaterialResponse = 0.0f;
    LocalController = nullptr;
    Super::Deinitialize();
}