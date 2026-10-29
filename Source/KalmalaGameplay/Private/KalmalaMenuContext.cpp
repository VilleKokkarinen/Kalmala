#include "KalmalaMenuContext.h"

void AKalmalaMenuPlayerController::SpawnDefaultHUD() {}
ASpectatorPawn* AKalmalaMenuPlayerController::SpawnSpectatorPawn() { return nullptr; }

AKalmalaMenuGameMode::AKalmalaMenuGameMode()
{
    PlayerControllerClass = AKalmalaMenuPlayerController::StaticClass();
    DefaultPawnClass = nullptr;
    SpectatorClass = nullptr;
    HUDClass = nullptr;
}

void AKalmalaMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) {}
void AKalmalaMenuGameMode::RestartPlayer(AController* NewPlayer) {}
APawn* AKalmalaMenuGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) { return nullptr; }
APawn* AKalmalaMenuGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) { return nullptr; }
