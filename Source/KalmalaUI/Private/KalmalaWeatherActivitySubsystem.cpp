#include "KalmalaWeatherActivitySubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaWeatherActivityWidget.h"
#include "KalmalaWorldGenerationGameState.h"

void UKalmalaWeatherActivitySubsystem::Tick(float)
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

    const AKalmalaWorldGenerationGameState* GameState = World->GetGameState<AKalmalaWorldGenerationGameState>();
    if (GameState == nullptr || !GameState->GetWeatherState().IsValid())
    {
        if (WeatherWidget != nullptr) WeatherWidget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (WeatherWidget == nullptr)
    {
        WeatherWidget = CreateWidget<UKalmalaWeatherActivityWidget>(FoundController,
            UKalmalaWeatherActivityWidget::StaticClass());
        if (WeatherWidget == nullptr) return;
        WeatherWidget->ConfigureViewportPlacement();
        WeatherWidget->AddToPlayerScreen(55);
    }

    WeatherWidget->SetWeatherActivity(GameState->GetWeatherState().ActivityLevel,
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    WeatherWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaWeatherActivitySubsystem::ReleaseController()
{
    if (WeatherWidget != nullptr) WeatherWidget->RemoveFromParent();
    WeatherWidget = nullptr;
}

void UKalmalaWeatherActivitySubsystem::Deinitialize()
{
    ReleaseController();
    LocalController = nullptr;
    Super::Deinitialize();
}
