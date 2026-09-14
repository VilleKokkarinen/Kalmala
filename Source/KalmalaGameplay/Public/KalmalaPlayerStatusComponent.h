#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaPlayerStatusComponent.generated.h"

/** Compiled status-definition output; never supplied by a client. */
struct FKalmalaStatusModifiers
{
    float Movement = 1.0f;
    float StaminaUse = 1.0f;
};

USTRUCT(BlueprintType)
struct FKalmalaPlayerStatusEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
    FName StatusId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
    float RemainingSeconds = 0.0f;
};

/**
 * Replicated player-facing statuses. The server is the sole writer; entries
 * deliberately carry no client-selected source, multiplier, or expiry data.
 */
UCLASS(ClassGroup=(Kalmala), meta=(BlueprintSpawnableComponent))
class KALMALAGAMEPLAY_API UKalmalaPlayerStatusComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UKalmalaPlayerStatusComponent();

    static const FName WetStatusId;
    static constexpr float WetMaximumSeconds = 120.0f;
    static constexpr float UnroofedRainTriggerSeconds = 10.0f;
    static constexpr float WetMovementMultiplier = 0.90f;
    static constexpr float WetStaminaUseMultiplier = 1.25f;

    FKalmalaStatusModifiers GetModifiers() const { return EvaluateModifiers(Statuses); }
    static FKalmalaStatusModifiers EvaluateModifiers(const TArray<FKalmalaPlayerStatusEntry>& Entries);
    /** Base cost must come from an authoritative action definition, never an RPC payload. */
    float CalculateStaminaCost(float BaseCost) const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool HasStatus(FName StatusId) const;
    float GetRemainingSeconds(FName StatusId) const;
    const TArray<FKalmalaPlayerStatusEntry>& GetStatuses() const { return Statuses; }

    /** Server-only source-independent wet application. Reapplication clamps at the definition maximum. */
    void ApplyWetFromServer();
    /** Server-only expiration tick. */
    void AdvanceFromServer(float DeltaSeconds);

    static void ApplyWet(TArray<FKalmalaPlayerStatusEntry>& Entries);
    static void Advance(TArray<FKalmalaPlayerStatusEntry>& Entries, float DeltaSeconds);

private:
    UFUNCTION() void OnRep_Statuses();

    UPROPERTY(ReplicatedUsing=OnRep_Statuses, VisibleAnywhere, Category = "Status")
    TArray<FKalmalaPlayerStatusEntry> Statuses;
};
