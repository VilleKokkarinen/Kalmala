#include "KalmalaGameMode.h"

#include "KalmalaCharacter.h"

AKalmalaGameMode::AKalmalaGameMode()
{
    bUseSeamlessTravel = true;
    DefaultPawnClass = AKalmalaCharacter::StaticClass();
}
