#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaGeneratedTerrainTile.generated.h"

class UBoxComponent;

/** A replicated collision tile with a stable default component for movement bases. */
UCLASS(NotPlaceable)
class KALMALAWORLD_API AKalmalaGeneratedTerrainTile : public AActor
{
    GENERATED_BODY()

public:
    AKalmalaGeneratedTerrainTile();
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> Collision;
};
