#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KalmalaPlayerStatusComponent.generated.h"

class AKalmalaCampfire;

/** Compiled status-definition output; never supplied by a client. */
struct FKalmalaStatusModifiers
{
    float Movement = 1.0f;
    float StaminaCostAddition = 0.0f;
};

/** Derived only from the owning inventory, never client numeric input. */
struct FKalmalaAccessoryModifiers
{
    int32 MaxHealth = 0;
    int32 MaxStamina = 0;
    float StaminaCost = 0.0f;
    float SupportSpellCost = 0.0f;
    float StaminaRegeneration = 0.0f;
    float HealingReceived = 0.0f;
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
    static const FName WeakPoisonStatusId;
    static const FName MajorPoisonStatusId;
    static const FName SteadyMealStatusId;
    static const FName CookedBoarMeatItemId;
    static const FName CookedDeerMeatItemId;
    static const FName HearthBrothItemId;
    static const FName LingonberryItemId;
    static const FName ForestChanterelleItemId;
    static const FName BerryRecoveryDraughtItemId;
    static const FName RootStaminaDraughtItemId;
    static const FName RatTailTonicItemId;
    static const FName AntidoteItemId;
    static const FName PeatHeartStewItemId;
    static const FName WolfStewItemId;
    static const FName BerryRecoveryStatusId;
    static const FName RatTailTonicStatusId;
    static const FName DraughtCooldownStatusId;
    static const FName PoisonResistanceStatusId;
    static const FName MeatStewItemId;
    static const FName RootVegetableSoupItemId;
    static const FName RoastedRootVegetablesItemId;
    static const FName DeerRootRoastItemId;
    static constexpr float WetMaximumSeconds = 120.0f;
    static constexpr float WeakPoisonMaximumSeconds = 10.0f;
    static constexpr float WeakPoisonDamagePerSecond = 1.0f;
    static constexpr float PoisonTickIntervalSeconds = 1.0f;
    static constexpr float WeakPoisonTickIntervalSeconds = PoisonTickIntervalSeconds;
    static constexpr float MajorPoisonMaximumSeconds = 10.0f;
    static constexpr float MajorPoisonDamagePerSecond = 5.0f;
    static constexpr float SteadyMealMaximumSeconds = 120.0f;
    static constexpr float BerryRecoveryMaximumSeconds = 10.0f;
    static constexpr float RatTailTonicMaximumSeconds = 10.0f;
    static constexpr float DraughtCooldownMaximumSeconds = 60.0f;
    static constexpr float AntidoteResistanceMaximumSeconds = 600.0f;
    static constexpr float AntidoteWeakPoisonMaximumSeconds = 5.0f;
    static constexpr float BerryRecoveryHealth = 20.0f;
    static constexpr float RatTailTonicHealth = 30.0f;
    static constexpr float RootStaminaDraughtStamina = 20.0f;
    static constexpr float SteadyMealStaminaCostAddition = -0.10f;
    static constexpr float UnroofedRainTriggerSeconds = 10.0f;
    // Wet remains a meaningful exposure pressure, but its movement and sprint
    // costs leave room for a player to recover or choose a short detour.
    static constexpr float WetMovementMultiplier = 0.92f;
    static constexpr float WetStaminaCostAddition = 0.15f;

    FKalmalaStatusModifiers GetModifiers() const { return EvaluateModifiers(Statuses); }
    static FKalmalaStatusModifiers EvaluateModifiers(const TArray<FKalmalaPlayerStatusEntry>& Entries);
    /** Base cost must come from an authoritative action definition, never an RPC payload. */
    float CalculateStaminaCost(float BaseCost, bool bSupportSpell = false) const;
    FKalmalaAccessoryModifiers GetAccessoryModifiers() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    bool HasStatus(FName StatusId) const;
    float GetRemainingSeconds(FName StatusId) const;
    const TArray<FKalmalaPlayerStatusEntry>& GetStatuses() const { return Statuses; }

    static bool IsKnownFoodItem(FName ItemId);
    static bool ApplyFood(TArray<FKalmalaPlayerStatusEntry>& Entries, FName ItemId);
    bool CanApplyFoodFromServer(FName ItemId) const;
    bool ApplyFoodFromServer(FName ItemId);
    static bool IsKnownDraughtItem(FName ItemId);
    static bool ApplyDraught(TArray<FKalmalaPlayerStatusEntry>& Entries, FName ItemId);
    bool CanApplyDraughtFromServer(FName ItemId) const;
    bool ApplyDraughtFromServer(FName ItemId);

    /** Server-only source-independent wet application. Reapplication clamps at the definition maximum. */
    void ApplyWetFromServer();
    /** Applies or refreshes the shared weak-poison effect without stacking its ticks. */
    bool ApplyWeakPoisonFromServer(float DurationSeconds);
    /** Applies or refreshes major poison while sharing the single poison damage clock. */
    bool ApplyMajorPoisonFromServer(float DurationSeconds);
    /** Server-only expiration tick. */
    void AdvanceFromServer(float DeltaSeconds);
    /** Validates a same-world authoritative heat source; no client RPC. */
    bool TryRemoveWetAtCampfireFromServer(const AKalmalaCampfire* Campfire);

    static void ApplyWet(TArray<FKalmalaPlayerStatusEntry>& Entries);
    static void Advance(TArray<FKalmalaPlayerStatusEntry>& Entries, float DeltaSeconds);

private:
    bool ApplyWeakPoisonFromServerInternal(float DurationSeconds, bool bIgnoreResistance);
    UFUNCTION() void OnRep_Statuses();

    UPROPERTY(ReplicatedUsing=OnRep_Statuses, VisibleAnywhere, Category = "Status")
    TArray<FKalmalaPlayerStatusEntry> Statuses;
    float PoisonSecondsUntilTick = PoisonTickIntervalSeconds;
};
