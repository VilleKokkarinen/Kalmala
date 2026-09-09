#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KalmalaInteractable.h"
#include "KalmalaCampfire.generated.h"

class AKalmalaCharacter;
class UPointLightComponent;
class USphereComponent;
class UProceduralMeshComponent;

/**
 * A replicated campfire whose lit state, fuel wetness, and warmth are written
 * only by the server. Paid placement and fuel consumption use private inventory.
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
    bool IsLit() const { return bIsLit; }
    float GetFuelWetness() const { return FuelWetness; }
    float GetEffectiveWarmth() const { return EffectiveWarmth; }
    static constexpr float FuelSecondsPerBundle = 60.0f;
    static constexpr float MaxFuelSeconds = 300.0f;
    float GetFuelSeconds() const { return FuelSeconds; }
    bool HasRoof() const { return bRoofProtected; }
    bool HasWindbreak() const { return bWindProtected; }
    bool CanUse(const AKalmalaCharacter* Character) const;
    bool TryRefuelFromServer(AKalmalaCharacter* Character);
    void InitializePaidFromServer(AKalmalaCharacter* Character);
    void SetSharedFromServer(bool bShared);
    FString GetStatusText() const;
    /** Trusted server weather seam, also used by the deterministic live regression. */
    void AdvanceFromServer(float DeltaSeconds, float Rain, float Wind);

private:
    void UpdateFromServerWeather(float DeltaSeconds);
    void ApplyReplicatedState();

    UPROPERTY(VisibleAnywhere, Category = "Campfire")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Campfire")
    TObjectPtr<UPointLightComponent> FireLight;

    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> HearthMesh;
    UPROPERTY(ReplicatedUsing=OnRep_CampfireState) float FuelSeconds = 0.0f;
    UPROPERTY(ReplicatedUsing=OnRep_CampfireState) bool bRoofProtected = false;
    UPROPERTY(ReplicatedUsing=OnRep_CampfireState) bool bWindProtected = false;
    UPROPERTY(ReplicatedUsing=OnRep_CampfireState) bool bSharedUse = true;

    UPROPERTY(ReplicatedUsing = OnRep_CampfireState, VisibleAnywhere, Category = "Campfire")
    bool bIsLit = false;

    UPROPERTY(ReplicatedUsing = OnRep_CampfireState, VisibleAnywhere, Category = "Campfire", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FuelWetness = 0.0f;

    UPROPERTY(ReplicatedUsing = OnRep_CampfireState, VisibleAnywhere, Category = "Campfire", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float EffectiveWarmth = 0.0f;

    UFUNCTION()
    void OnRep_CampfireState();
};
