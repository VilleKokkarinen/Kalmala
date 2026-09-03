#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "KalmalaWorldGenerationConfig.h"
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

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    const FKalmalaWorldGenerationConfig& GetWorldGenerationConfig() const { return WorldGenerationConfig; }

private:
    UPROPERTY(ReplicatedUsing = OnRep_WorldGenerationConfig, VisibleAnywhere, Category = "World Generation")
    FKalmalaWorldGenerationConfig WorldGenerationConfig;

    UFUNCTION()
    void OnRep_WorldGenerationConfig();

    void LogWorldGenerationIdentity(const TCHAR* Source) const;
};
