#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KalmalaGameMode.generated.h"

/**
 * Server-authoritative rules for a Kalmala session.
 * Gameplay systems are added in later milestones.
 */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AKalmalaGameMode();

    virtual void BeginPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
    class APlayerStart* GeneratedPlayerStart = nullptr;
};
