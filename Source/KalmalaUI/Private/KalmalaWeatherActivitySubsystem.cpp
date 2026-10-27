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

namespace
{
    constexpr float MountainWeatherFogProbeIntervalSeconds = 1.0f;
    constexpr float MountainWeatherFogMaxDensity = 0.004f;
    constexpr float MountainWeatherFogMaxOpacity = 0.20f;
    constexpr float MountainWeatherFogStartDistance = 2500.0f;
    constexpr float MountainWeatherFogFadeSpeed = 1.5f;
    constexpr float MountainWeatherFogReleaseThreshold = 0.005f;
    const FLinearColor MountainWeatherFogColor(0.32f, 0.34f, 0.41f);
}

void UKalmalaWeatherActivitySubsystem::Tick(const float DeltaTime)
{
    // The survival subsystem owns the single top-right weather/status parent.
    ReleaseController();
    UpdateMountainWeatherFog(DeltaTime);
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

    if (TargetMountainWeatherFogCoverage <= MountainWeatherFogReleaseThreshold
        && CurrentMountainWeatherFogCoverage <= MountainWeatherFogReleaseThreshold)
    {
        DestroyMountainWeatherFog();
    }
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
    LocalController = nullptr;
    Super::Deinitialize();
}