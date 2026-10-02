#include "KalmalaWeatherActivitySubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaWeatherActivityWidget.h"
#include "KalmalaWorldGenerationGameState.h"

void UKalmalaWeatherActivitySubsystem::Tick(float)
{
    // The survival subsystem owns the single top-right weather/status parent.
    ReleaseController();
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
