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
    constexpr float MountainStormProbeIntervalSeconds = 1.0f;
    constexpr float MountainStormFogMaxDensity = 0.004f;
    constexpr float MountainStormFogMaxOpacity = 0.20f;
    constexpr float MountainStormFogStartDistance = 2500.0f;
    constexpr float MountainStormFogFadeSpeed = 1.5f;
    constexpr float MountainStormFogReleaseThreshold = 0.005f;
    const FLinearColor MountainStormFogColor(0.32f, 0.34f, 0.41f);
}

void UKalmalaWeatherActivitySubsystem::Tick(const float DeltaTime)
{
    // The survival subsystem owns the single top-right weather/status parent.
    ReleaseController();
    UpdateMountainStormVisuals(DeltaTime);
}

void UKalmalaWeatherActivitySubsystem::UpdateMountainStormVisuals(const float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr
        || World->GetNetMode() == NM_DedicatedServer)
    {
        DestroyMountainStormFog();
        return;
    }
    if (MountainStormFog != nullptr
        && (!IsValid(MountainStormFog.Get()) || MountainStormFog->GetWorld() != World))
    {
        DestroyMountainStormFog();
    }

    MountainStormProbeTimeRemaining -= FMath::Max(DeltaTime, 0.0f);
    if (MountainStormProbeTimeRemaining <= 0.0f)
    {
        TargetMountainStormCoverage = 0.0f;
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
                TargetMountainStormCoverage = FMath::Clamp(
                    FMath::Clamp(Weather.GetStormIntensity(), 0.0f, 1.0f)
                        * FMath::Clamp(MountainWeight, 0.0f, 1.0f),
                    0.0f, 1.0f);
            }
        }
        MountainStormProbeTimeRemaining = MountainStormProbeIntervalSeconds;
    }

    if (TargetMountainStormCoverage > MountainStormFogReleaseThreshold && MountainStormFog == nullptr)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.ObjectFlags |= RF_Transient;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        MountainStormFog = World->SpawnActor<AExponentialHeightFog>(
            AExponentialHeightFog::StaticClass(), FTransform::Identity, SpawnParameters);
        if (MountainStormFog != nullptr)
        {
            MountainStormFog->SetReplicates(false);
            MountainStormFog->SetActorTickEnabled(false);
        }
    }

    if (MountainStormFog == nullptr || !IsValid(MountainStormFog.Get())) return;
    UExponentialHeightFogComponent* Fog = MountainStormFog->GetComponent();
    if (Fog == nullptr)
    {
        DestroyMountainStormFog();
        return;
    }

    CurrentMountainStormCoverage = FMath::FInterpTo(CurrentMountainStormCoverage,
        TargetMountainStormCoverage, DeltaTime, MountainStormFogFadeSpeed);
    Fog->SetFogDensity(MountainStormFogMaxDensity * CurrentMountainStormCoverage);
    Fog->SetFogMaxOpacity(MountainStormFogMaxOpacity * CurrentMountainStormCoverage);
    Fog->SetStartDistance(MountainStormFogStartDistance);
    Fog->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(
        FLinearColor::White, MountainStormFogColor, CurrentMountainStormCoverage));

    if (TargetMountainStormCoverage <= MountainStormFogReleaseThreshold
        && CurrentMountainStormCoverage <= MountainStormFogReleaseThreshold)
    {
        DestroyMountainStormFog();
    }
}

void UKalmalaWeatherActivitySubsystem::DestroyMountainStormFog()
{
    if (IsValid(MountainStormFog.Get())) MountainStormFog->Destroy();
    MountainStormFog = nullptr;
    TargetMountainStormCoverage = 0.0f;
    CurrentMountainStormCoverage = 0.0f;
    MountainStormProbeTimeRemaining = 0.0f;
}

void UKalmalaWeatherActivitySubsystem::ReleaseController()
{
    if (WeatherWidget != nullptr) WeatherWidget->RemoveFromParent();
    WeatherWidget = nullptr;
}

void UKalmalaWeatherActivitySubsystem::Deinitialize()
{
    ReleaseController();
    DestroyMountainStormFog();
    LocalController = nullptr;
    Super::Deinitialize();
}
