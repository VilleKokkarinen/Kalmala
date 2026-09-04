#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaCampfire.generated.h"

class AKalmalaCharacter;
class UPointLightComponent;
class USphereComponent;

/**
 * A replicated campfire whose lit state, fuel wetness, and warmth are written
 * only by the server. Construction and inventory will create/stock it later.
 */
UCLASS(NotBlueprintable)
class KALMALAGAMEPLAY_API AKalmalaCampfire : public AActor, public IKalmalaInteractable
{
    GENERATED_BODY()

public:
    AKalmalaCampfire();

    virtual void Tick(float DeltaSeconds) override;
    virtual bool CanInteract_Implementation(AKalmalaCharacter* Interactor) const override;
    virtual void Interact_Implementation(AKalmalaCharacter* Interactor) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    static bool IsLightingAllowed(bool bServerAuthority, float FuelWetness);
    float GetWarmthContributionAt(const FVector& Location) const;

private:
    void UpdateFromServerWeather(float DeltaSeconds);
    void ApplyReplicatedState();

    UPROPERTY(VisibleAnywhere, Category = "Campfire")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Campfire")
    TObjectPtr<UPointLightComponent> FireLight;

    UPROPERTY(ReplicatedUsing = OnRep_CampfireState, VisibleAnywhere, Category = "Campfire")
    bool bIsLit = false;

    UPROPERTY(ReplicatedUsing = OnRep_CampfireState, VisibleAnywhere, Category = "Campfire", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FuelWetness = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_CampfireState, VisibleAnywhere, Category = "Campfire", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EffectiveWarmth = 0.0f;

    UFUNCTION()
    void OnRep_CampfireState();
};
