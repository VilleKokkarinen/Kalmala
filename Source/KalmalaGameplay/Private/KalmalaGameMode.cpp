#include "KalmalaGameMode.h"

#include "KalmalaCharacter.h"
#include "KalmalaWorldGenerationGameState.h"

AKalmalaGameMode::AKalmalaGameMode()
{
    bUseSeamlessTravel = true;
    DefaultPawnClass = AKalmalaCharacter::StaticClass();
    GameStateClass = AKalmalaWorldGenerationGameState::StaticClass();
}
