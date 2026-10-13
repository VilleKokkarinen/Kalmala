#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaDamageProfile.h"
#include "KalmalaResistanceComponent.generated.h"

/** Authored entity defenses; no client setter or damage RPC. All entity channels default to zero. */
UCLASS(ClassGroup=(Kalmala), meta=(BlueprintSpawnableComponent))
class KALMALAGAMEPLAY_API UKalmalaResistanceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKalmalaResistanceComponent();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resistance")
    FKalmalaResistanceProfile Profile;

    bool ResolveFromServer(const FKalmalaDamageChannels& Incoming, float& OutDamage) const;
};
