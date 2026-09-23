#include "KalmalaSurvivalStatusSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaCharacter.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaWeatherActivityWidget.h"
#include "KalmalaWorldGenerationGameState.h"

void UKalmalaSurvivalStatusSubsystem::Tick(float)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr) return;

    APlayerController* FoundController = LocalPlayer->GetPlayerController(World);
    if (LocalController.Get() != FoundController)
    {
        ReleaseController();
        LocalController = FoundController;
    }
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;

    if (StatusWidget == nullptr)
    {
        StatusWidget = CreateWidget<UKalmalaSurvivalStatusWidget>(FoundController,
            UKalmalaSurvivalStatusWidget::StaticClass());
        if (StatusWidget == nullptr) return;
        StatusWidget->ConfigureViewportPlacement();
        StatusWidget->AddToPlayerScreen(54);
    }

    FKalmalaSurvivalStatusSnapshot Snapshot;
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(FoundController->GetPawn());
    Snapshot.bHasCharacter = Character != nullptr;
    if (Character != nullptr)
    {
        Snapshot.Exposure = Character->GetExposureState();
        if (const UKalmalaPlayerStatusComponent* Status = Character->GetStatusComponent())
        {
            Snapshot.Statuses = Status->GetStatuses();
        }
        if (const UKalmalaSupportMagicComponent* Support = Character->GetSupportMagicComponent())
        {
            Snapshot.ActiveSupportEffect = Support->GetActiveEffect();
            Snapshot.ActiveSupportEffectExpiry = Support->GetActiveEffectExpiry();
            Snapshot.HearthShieldStrength = Support->GetHearthShieldStrength();
            Snapshot.BearsVigorStrengthMultiplier = Support->GetBearsVigorStrengthMultiplier();
        }
    }

    if (const AKalmalaWorldGenerationGameState* GameState = World->GetGameState<AKalmalaWorldGenerationGameState>())
    {
        Snapshot.ServerTimeSeconds = GameState->GetServerWorldTimeSeconds();
        Snapshot.Weather = GameState->GetWeatherState();
        Snapshot.bHasWeatherState = Snapshot.Weather.IsValid();
    }
    else
    {
        Snapshot.ServerTimeSeconds = World->GetTimeSeconds();
    }

    StatusWidget->SetSnapshot(Snapshot,
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    StatusWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaSurvivalStatusSubsystem::ReleaseController()
{
    if (StatusWidget != nullptr) StatusWidget->RemoveFromParent();
    StatusWidget = nullptr;
}

void UKalmalaSurvivalStatusSubsystem::Deinitialize()
{
    ReleaseController();
    LocalController = nullptr;
    Super::Deinitialize();
}
