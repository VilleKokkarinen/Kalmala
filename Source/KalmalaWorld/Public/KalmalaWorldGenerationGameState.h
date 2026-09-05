#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWeatherState.h"
#include "KalmalaWorldGenerationGameState.generated.h"

/**
 * Replicates the immutable, server-selected world identity to every connected
 * player. Clients consume this value for deterministic cosmetic generation and
 * never select or mutate it.
 */
UCLASS()
class KALMALAWORLD_API AKalmalaWorldGenerationGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AKalmalaWorldGenerationGameState();

    virtual void PostInitializeComponents() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    const FKalmalaWorldGenerationConfig& GetWorldGenerationConfig() const { return WorldGenerationConfig; }
    const FKalmalaWeatherState& GetWeatherState() const { return WeatherState; }
    static bool IsWeatherUpdateAllowed(bool bServerAuthority);

    /** Called by the authoritative GameMode after deterministic weather selection. */
    void SetWeatherStateFromServer(const FKalmalaWeatherState& InWeatherState);

private:
    UPROPERTY(ReplicatedUsing = OnRep_WorldGenerationConfig, VisibleAnywhere, Category = "World Generation")
    FKalmalaWorldGenerationConfig WorldGenerationConfig;

    UPROPERTY(ReplicatedUsing = OnRep_WeatherState, VisibleAnywhere, Category = "Weather")
    FKalmalaWeatherState WeatherState;

    UFUNCTION()
    void OnRep_WorldGenerationConfig();

    UFUNCTION()
    void OnRep_WeatherState();

    void LogWorldGenerationIdentity(const TCHAR* Source) const;
};
