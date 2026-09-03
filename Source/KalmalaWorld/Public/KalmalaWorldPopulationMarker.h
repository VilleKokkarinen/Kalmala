#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationMarker.generated.h"

/** Lightweight replicated server-owned placeholder for a generated population spawn. */
UCLASS(NotBlueprintable)
class KALMALAWORLD_API AKalmalaWorldPopulationMarker : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaWorldPopulationMarker();

    void InitializeServer(const FKalmalaWorldPopulationSpawn& Spawn);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(Replicated)
    uint8 PopulationKind = static_cast<uint8>(EKalmalaWorldPopulationKind::Wildlife);

    UPROPERTY(Replicated)
    FIntPoint PopulationSpatialKey = FIntPoint::ZeroValue;

    UPROPERTY(Replicated)
    uint64 PopulationSpawnSeed = 0;
};
