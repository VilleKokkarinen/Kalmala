#include "KalmalaPlayerStatusComponent.h"

#include "Net/UnrealNetwork.h"
#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"

const FName UKalmalaPlayerStatusComponent::WetStatusId(TEXT("State.Wet"));
const FName UKalmalaPlayerStatusComponent::WeakPoisonStatusId(TEXT("State.Poison.Weak"));
const FName UKalmalaPlayerStatusComponent::MajorPoisonStatusId(TEXT("State.Poison.Major"));
const FName UKalmalaPlayerStatusComponent::SteadyMealStatusId(TEXT("State.Food.SteadyMeal"));
const FName UKalmalaPlayerStatusComponent::CookedBoarMeatItemId(TEXT("CookedBoarMeat"));
const FName UKalmalaPlayerStatusComponent::CookedDeerMeatItemId(TEXT("CookedDeerMeat"));
const FName UKalmalaPlayerStatusComponent::HearthBrothItemId(TEXT("HearthBroth"));
const FName UKalmalaPlayerStatusComponent::LingonberryItemId(TEXT("Lingonberry"));
const FName UKalmalaPlayerStatusComponent::ForestChanterelleItemId(TEXT("ForestChanterelle"));
const FName UKalmalaPlayerStatusComponent::BerryRecoveryDraughtItemId(TEXT("BerryRecoveryDraught"));
const FName UKalmalaPlayerStatusComponent::RootStaminaDraughtItemId(TEXT("RootStaminaDraught"));
const FName UKalmalaPlayerStatusComponent::RatTailTonicItemId(TEXT("RatTailTonic"));
const FName UKalmalaPlayerStatusComponent::AntidoteItemId(TEXT("Antidote"));
const FName UKalmalaPlayerStatusComponent::PeatHeartStewItemId(TEXT("PeatHeartStew"));
const FName UKalmalaPlayerStatusComponent::BerryRecoveryStatusId(TEXT("State.Recovery.BerryDraught"));
const FName UKalmalaPlayerStatusComponent::RatTailTonicStatusId(TEXT("State.Recovery.RatTailTonic"));
const FName UKalmalaPlayerStatusComponent::DraughtCooldownStatusId(TEXT("State.Recovery.DraughtCooldown"));
const FName UKalmalaPlayerStatusComponent::PoisonResistanceStatusId(TEXT("State.Poison.Resistance"));
const FName UKalmalaPlayerStatusComponent::MeatStewItemId(TEXT("MeatStew"));
const FName UKalmalaPlayerStatusComponent::RootVegetableSoupItemId(TEXT("RootVegetableSoup"));
const FName UKalmalaPlayerStatusComponent::RoastedRootVegetablesItemId(TEXT("RoastedRootVegetables"));
const FName UKalmalaPlayerStatusComponent::DeerRootRoastItemId(TEXT("DeerRootRoast"));

FKalmalaStatusModifiers UKalmalaPlayerStatusComponent::EvaluateModifiers(const TArray<FKalmalaPlayerStatusEntry>& Entries)
{
    FKalmalaStatusModifiers Result;
    TSet<FName> Applied;
    for (const FKalmalaPlayerStatusEntry& Entry : Entries)
    {
        if (!FMath::IsFinite(Entry.RemainingSeconds) || Entry.RemainingSeconds <= 0.0f || Applied.Contains(Entry.StatusId)) continue;
        Applied.Add(Entry.StatusId);
        // Add future compiled definitions here; entries contain no numeric modifiers.
        if (Entry.StatusId == WetStatusId)
        {
            Result.Movement *= WetMovementMultiplier;
            Result.StaminaUse *= WetStaminaUseMultiplier;
        }
        else if (Entry.StatusId == SteadyMealStatusId)
        {
            Result.StaminaUse *= SteadyMealStaminaUseMultiplier;
        }
    }
    return Result;
}

float UKalmalaPlayerStatusComponent::CalculateStaminaCost(const float BaseCost) const
{
    if (!FMath::IsFinite(BaseCost) || BaseCost < 0.0f) return 0.0f;
    return FMath::Min(static_cast<double>(BaseCost) * GetModifiers().StaminaUse, static_cast<double>(MAX_flt));
}

UKalmalaPlayerStatusComponent::UKalmalaPlayerStatusComponent()
{
    SetIsReplicatedByDefault(true);
}

void UKalmalaPlayerStatusComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaPlayerStatusComponent, Statuses, COND_OwnerOnly);
}

bool UKalmalaPlayerStatusComponent::HasStatus(const FName StatusId) const
{
    return GetRemainingSeconds(StatusId) > 0.0f;
}

float UKalmalaPlayerStatusComponent::GetRemainingSeconds(const FName StatusId) const
{
    const FKalmalaPlayerStatusEntry* Entry = Statuses.FindByPredicate([StatusId](const FKalmalaPlayerStatusEntry& Candidate)
    {
        return Candidate.StatusId == StatusId && FMath::IsFinite(Candidate.RemainingSeconds) && Candidate.RemainingSeconds > 0.0f;
    });
    const float Maximum = StatusId == WeakPoisonStatusId ? WeakPoisonMaximumSeconds
        : StatusId == MajorPoisonStatusId ? MajorPoisonMaximumSeconds
        : StatusId == SteadyMealStatusId ? SteadyMealMaximumSeconds
        : StatusId == BerryRecoveryStatusId ? BerryRecoveryMaximumSeconds
        : StatusId == RatTailTonicStatusId ? RatTailTonicMaximumSeconds
        : StatusId == DraughtCooldownStatusId ? DraughtCooldownMaximumSeconds
        : StatusId == PoisonResistanceStatusId ? AntidoteResistanceMaximumSeconds : WetMaximumSeconds;
    return Entry ? FMath::Clamp(Entry->RemainingSeconds, 0.0f, Maximum) : 0.0f;
}

bool UKalmalaPlayerStatusComponent::IsKnownFoodItem(const FName ItemId)
{
    return ItemId == CookedBoarMeatItemId || ItemId == CookedDeerMeatItemId
        || ItemId == MeatStewItemId || ItemId == RootVegetableSoupItemId
        || ItemId == RoastedRootVegetablesItemId || ItemId == DeerRootRoastItemId
        || ItemId == HearthBrothItemId || ItemId == LingonberryItemId
        || ItemId == ForestChanterelleItemId || ItemId == TEXT("RoastedBadger")
        || ItemId == PeatHeartStewItemId;
}

bool UKalmalaPlayerStatusComponent::ApplyFood(TArray<FKalmalaPlayerStatusEntry>& Entries, const FName ItemId)
{
    if (!IsKnownFoodItem(ItemId)
        || Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == SteadyMealStatusId && FMath::IsFinite(Entry.RemainingSeconds) && Entry.RemainingSeconds > 0.0f; }))
        return false;

    Advance(Entries, 0.0f);
    FKalmalaPlayerStatusEntry& Meal = Entries.AddDefaulted_GetRef();
    Meal.StatusId = SteadyMealStatusId;
    Meal.RemainingSeconds = SteadyMealMaximumSeconds;
    return true;
}

bool UKalmalaPlayerStatusComponent::CanApplyFoodFromServer(const FName ItemId) const
{
    const AActor* Owner = GetOwner();
    return IsValid(Owner) && Owner->HasAuthority() && IsKnownFoodItem(ItemId) && !HasStatus(SteadyMealStatusId);
}

bool UKalmalaPlayerStatusComponent::ApplyFoodFromServer(const FName ItemId)
{
    if (!CanApplyFoodFromServer(ItemId) || !ApplyFood(Statuses, ItemId)) return false;
    GetOwner()->ForceNetUpdate();
    return true;
}

bool UKalmalaPlayerStatusComponent::IsKnownDraughtItem(const FName ItemId)
{
    return ItemId == BerryRecoveryDraughtItemId || ItemId == RootStaminaDraughtItemId
        || ItemId == RatTailTonicItemId || ItemId == AntidoteItemId;
}

bool UKalmalaPlayerStatusComponent::ApplyDraught(TArray<FKalmalaPlayerStatusEntry>& Entries, const FName ItemId)
{
    if (!IsKnownDraughtItem(ItemId)
        || Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == DraughtCooldownStatusId && FMath::IsFinite(Entry.RemainingSeconds) && Entry.RemainingSeconds > 0.0f; })
        || Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return (Entry.StatusId == BerryRecoveryStatusId || Entry.StatusId == RatTailTonicStatusId)
                && FMath::IsFinite(Entry.RemainingSeconds) && Entry.RemainingSeconds > 0.0f; })
        || (ItemId == AntidoteItemId && Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == PoisonResistanceStatusId && FMath::IsFinite(Entry.RemainingSeconds)
                && Entry.RemainingSeconds > 0.0f; })))
        return false;

    Advance(Entries, 0.0f);
    if (ItemId == AntidoteItemId)
    {
        const FKalmalaPlayerStatusEntry* MajorPoison = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        {
            return Entry.StatusId == MajorPoisonStatusId && FMath::IsFinite(Entry.RemainingSeconds)
                && Entry.RemainingSeconds > 0.0f;
        });
        const float ConvertedWeakDuration = MajorPoison
            ? FMath::Min(MajorPoison->RemainingSeconds, AntidoteWeakPoisonMaximumSeconds) : 0.0f;
        Entries.RemoveAll([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == WeakPoisonStatusId || Entry.StatusId == MajorPoisonStatusId; });
        if (ConvertedWeakDuration > 0.0f)
        {
            FKalmalaPlayerStatusEntry& WeakPoison = Entries.AddDefaulted_GetRef();
            WeakPoison.StatusId = WeakPoisonStatusId;
            WeakPoison.RemainingSeconds = ConvertedWeakDuration;
        }
        FKalmalaPlayerStatusEntry& Resistance = Entries.AddDefaulted_GetRef();
        Resistance.StatusId = PoisonResistanceStatusId;
        Resistance.RemainingSeconds = AntidoteResistanceMaximumSeconds;
    }
    else if (ItemId == BerryRecoveryDraughtItemId || ItemId == RatTailTonicItemId)
    {
        FKalmalaPlayerStatusEntry& Recovery = Entries.AddDefaulted_GetRef();
        const bool bRatTailTonic = ItemId == RatTailTonicItemId;
        Recovery.StatusId = bRatTailTonic ? RatTailTonicStatusId : BerryRecoveryStatusId;
        Recovery.RemainingSeconds = bRatTailTonic ? RatTailTonicMaximumSeconds : BerryRecoveryMaximumSeconds;
    }
    FKalmalaPlayerStatusEntry& Cooldown = Entries.AddDefaulted_GetRef();
    Cooldown.StatusId = DraughtCooldownStatusId;
    Cooldown.RemainingSeconds = DraughtCooldownMaximumSeconds;
    return true;
}

bool UKalmalaPlayerStatusComponent::CanApplyDraughtFromServer(const FName ItemId) const
{
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!IsValid(Character) || !Character->HasAuthority() || !IsKnownDraughtItem(ItemId)
        || !FMath::IsFinite(Character->GetHealth()) || Character->GetHealth() <= 1.0f
        || HasStatus(DraughtCooldownStatusId) || HasStatus(BerryRecoveryStatusId)
        || HasStatus(RatTailTonicStatusId))
        return false;
    if (ItemId == AntidoteItemId) return !HasStatus(PoisonResistanceStatusId);
    if (ItemId == BerryRecoveryDraughtItemId || ItemId == RatTailTonicItemId)
        return Character->GetHealth() < 100.0f;

    const UKalmalaCharacterMovementComponent* Movement = Cast<UKalmalaCharacterMovementComponent>(Character->GetCharacterMovement());
    return Movement && FMath::IsFinite(Movement->GetStamina())
        && FMath::IsFinite(Movement->GetMaximumStamina())
        && Movement->GetMaximumStamina() > 0.0f
        && Movement->GetStamina() < Movement->GetMaximumStamina();
}

bool UKalmalaPlayerStatusComponent::ApplyDraughtFromServer(const FName ItemId)
{
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!CanApplyDraughtFromServer(ItemId)) return false;

    const bool bHadPoison = HasStatus(WeakPoisonStatusId) || HasStatus(MajorPoisonStatusId);
    TArray<FKalmalaPlayerStatusEntry> CandidateStatuses = Statuses;
    if (!ApplyDraught(CandidateStatuses, ItemId)) return false;
    if (ItemId == RootStaminaDraughtItemId)
    {
        auto* Movement = Character
            ? Cast<UKalmalaCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;
        if (!Movement || !Movement->RestoreStaminaFromDraughtFromServer(RootStaminaDraughtStamina)) return false;
    }
    Statuses = MoveTemp(CandidateStatuses);
    if (ItemId == AntidoteItemId && bHadPoison
        && !HasStatus(WeakPoisonStatusId) && !HasStatus(MajorPoisonStatusId))
    {
        PoisonSecondsUntilTick = PoisonTickIntervalSeconds;
    }
    GetOwner()->ForceNetUpdate();
    return true;
}

void UKalmalaPlayerStatusComponent::ApplyWetFromServer()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    ApplyWet(Statuses);
    GetOwner()->ForceNetUpdate();
}

bool UKalmalaPlayerStatusComponent::ApplyWeakPoisonFromServer(const float DurationSeconds)
{
    return ApplyWeakPoisonFromServerInternal(DurationSeconds, false);
}

bool UKalmalaPlayerStatusComponent::ApplyWeakPoisonFromServerInternal(const float DurationSeconds,
    const bool bIgnoreResistance)
{
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!IsValid(Character) || !Character->HasAuthority() || Character->GetHealth() <= 1.0f
        || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.0f
        || DurationSeconds > WeakPoisonMaximumSeconds)
    {
        return false;
    }
    if (!bIgnoreResistance && HasStatus(PoisonResistanceStatusId)) return false;

    const bool bPoisonWasActive = HasStatus(WeakPoisonStatusId) || HasStatus(MajorPoisonStatusId);
    Advance(Statuses, 0.0f);
    FKalmalaPlayerStatusEntry* Poison = Statuses.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
    {
        return Entry.StatusId == WeakPoisonStatusId && FMath::IsFinite(Entry.RemainingSeconds)
            && Entry.RemainingSeconds > 0.0f;
    });
    if (Poison != nullptr)
    {
        Poison->RemainingSeconds = FMath::Max(Poison->RemainingSeconds, DurationSeconds);
    }
    else
    {
        Poison = &Statuses.AddDefaulted_GetRef();
        Poison->StatusId = WeakPoisonStatusId;
        Poison->RemainingSeconds = DurationSeconds;
        if (!bPoisonWasActive) PoisonSecondsUntilTick = PoisonTickIntervalSeconds;
    }
    GetOwner()->ForceNetUpdate();
    return true;
}

bool UKalmalaPlayerStatusComponent::ApplyMajorPoisonFromServer(const float DurationSeconds)
{
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!IsValid(Character) || !Character->HasAuthority() || Character->GetHealth() <= 1.0f
        || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.0f
        || DurationSeconds > MajorPoisonMaximumSeconds)
    {
        return false;
    }
    if (HasStatus(PoisonResistanceStatusId))
    {
        return ApplyWeakPoisonFromServerInternal(
            FMath::Min(DurationSeconds, AntidoteWeakPoisonMaximumSeconds), true);
    }

    const bool bPoisonWasActive = HasStatus(WeakPoisonStatusId) || HasStatus(MajorPoisonStatusId);
    Advance(Statuses, 0.0f);
    FKalmalaPlayerStatusEntry* Poison = Statuses.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
    {
        return Entry.StatusId == MajorPoisonStatusId && FMath::IsFinite(Entry.RemainingSeconds)
            && Entry.RemainingSeconds > 0.0f;
    });
    if (Poison != nullptr)
    {
        Poison->RemainingSeconds = FMath::Max(Poison->RemainingSeconds, DurationSeconds);
    }
    else
    {
        Poison = &Statuses.AddDefaulted_GetRef();
        Poison->StatusId = MajorPoisonStatusId;
        Poison->RemainingSeconds = DurationSeconds;
        if (!bPoisonWasActive) PoisonSecondsUntilTick = PoisonTickIntervalSeconds;
    }
    GetOwner()->ForceNetUpdate();
    return true;
}

void UKalmalaPlayerStatusComponent::AdvanceFromServer(const float DeltaSeconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    const float SafeDelta = FMath::Max(0.0f, FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f);
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
    if (!Character || Character->GetHealth() <= 1.0f)
    {
        Statuses.RemoveAll([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == PoisonResistanceStatusId; });
    }
    const float WeakPoisonRemaining = GetRemainingSeconds(WeakPoisonStatusId);
    const float MajorPoisonRemaining = GetRemainingSeconds(MajorPoisonStatusId);
    if (WeakPoisonRemaining > 0.0f || MajorPoisonRemaining > 0.0f)
    {
        if (!Character || Character->GetHealth() <= 1.0f)
        {
            Statuses.RemoveAll([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == WeakPoisonStatusId || Entry.StatusId == MajorPoisonStatusId; });
            PoisonSecondsUntilTick = PoisonTickIntervalSeconds;
        }
        else if (SafeDelta > 0.0f)
        {
            const float TimeToTick = FMath::Clamp(PoisonSecondsUntilTick, 0.0f, PoisonTickIntervalSeconds);
            const bool bTickDue = TimeToTick <= SafeDelta + KINDA_SMALL_NUMBER;
            if (bTickDue)
            {
                const bool bMajorActiveAtTick = MajorPoisonRemaining + KINDA_SMALL_NUMBER >= TimeToTick;
                const bool bWeakActiveAtTick = WeakPoisonRemaining + KINDA_SMALL_NUMBER >= TimeToTick;
                if (bMajorActiveAtTick)
                    Character->ApplyPoisonDamageFromStatusFromServer(MajorPoisonDamagePerSecond);
                else if (bWeakActiveAtTick)
                    Character->ApplyPoisonDamageFromStatusFromServer(WeakPoisonDamagePerSecond);
                // Apply at most one tick in a server update. Excess elapsed time is not replayed
                // as a burst after a hitch.
                PoisonSecondsUntilTick = PoisonTickIntervalSeconds;
            }
            else
            {
                PoisonSecondsUntilTick = FMath::Max(0.0f, TimeToTick - SafeDelta);
            }
        }
    }
    const int32 RecoveryIndex = Statuses.IndexOfByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
    {
        return (Entry.StatusId == BerryRecoveryStatusId || Entry.StatusId == RatTailTonicStatusId)
            && FMath::IsFinite(Entry.RemainingSeconds) && Entry.RemainingSeconds > 0.0f;
    });
    if (RecoveryIndex != INDEX_NONE)
    {
        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
        if (!Character || Character->GetHealth() <= 1.0f)
        {
            Statuses.RemoveAll([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == BerryRecoveryStatusId || Entry.StatusId == RatTailTonicStatusId; });
        }
        else if (Character->GetHealth() >= 100.0f)
        {
            Statuses.RemoveAll([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == BerryRecoveryStatusId || Entry.StatusId == RatTailTonicStatusId; });
        }
        else
        {
            const bool bRatTailTonic = Statuses[RecoveryIndex].StatusId == RatTailTonicStatusId;
            const float Elapsed = FMath::Min(SafeDelta, Statuses[RecoveryIndex].RemainingSeconds);
            if (Elapsed > 0.0f)
            {
                const float TotalHealing = bRatTailTonic ? RatTailTonicHealth : BerryRecoveryHealth;
                const float HealingDuration = bRatTailTonic ? RatTailTonicMaximumSeconds : BerryRecoveryMaximumSeconds;
                Character->RestoreHealthFromDraughtFromServer(TotalHealing * Elapsed / HealingDuration);
                if (Character->GetHealth() >= 100.0f)
                    Statuses.RemoveAll([](const FKalmalaPlayerStatusEntry& Entry)
                    { return Entry.StatusId == BerryRecoveryStatusId || Entry.StatusId == RatTailTonicStatusId; });
            }
        }
    }
    Advance(Statuses, DeltaSeconds);
    if (!HasStatus(WeakPoisonStatusId) && !HasStatus(MajorPoisonStatusId))
        PoisonSecondsUntilTick = PoisonTickIntervalSeconds;
    GetOwner()->ForceNetUpdate();
}

bool UKalmalaPlayerStatusComponent::TryRemoveWetAtCampfireFromServer(const AKalmalaCampfire* Campfire)
{
    const AActor* Owner = GetOwner();
    if (!IsValid(Owner) || !Owner->HasAuthority() || !IsValid(Campfire) || !Campfire->HasAuthority()
        || Campfire->GetWorld() != Owner->GetWorld() || !Campfire->IsLit()
        || Owner->GetActorLocation().ContainsNaN() || Campfire->GetActorLocation().ContainsNaN()) return false;
    const float Heat = Campfire->GetWarmthContributionAt(Owner->GetActorLocation());
    if (!FMath::IsFinite(Heat) || Heat <= 0.0f) return false;
    const int32 Removed = Statuses.RemoveAll([](const FKalmalaPlayerStatusEntry& Entry) { return Entry.StatusId == WetStatusId; });
    if (Removed > 0) GetOwner()->ForceNetUpdate();
    return Removed > 0;
}

void UKalmalaPlayerStatusComponent::ApplyWet(TArray<FKalmalaPlayerStatusEntry>& Entries)
{
    FKalmalaPlayerStatusEntry* Wet = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry) { return Entry.StatusId == WetStatusId; });
    if (Wet == nullptr)
    {
        Wet = &Entries.AddDefaulted_GetRef();
        Wet->StatusId = WetStatusId;
    }
    Wet->RemainingSeconds = WetMaximumSeconds;
    Advance(Entries, 0.0f);
}

void UKalmalaPlayerStatusComponent::Advance(TArray<FKalmalaPlayerStatusEntry>& Entries, const float DeltaSeconds)
{
    const float SafeDelta = FMath::Max(0.0f, FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f);
    TSet<FName> Seen;
    for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
    {
        FKalmalaPlayerStatusEntry& Entry = Entries[Index];
        if (Entry.StatusId.IsNone() || !FMath::IsFinite(Entry.RemainingSeconds) || Seen.Contains(Entry.StatusId))
        {
            Entries.RemoveAt(Index);
            continue;
        }
        Seen.Add(Entry.StatusId);
        if (Entry.StatusId == WetStatusId)
        {
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, WetMaximumSeconds);
        }
        else if (Entry.StatusId == SteadyMealStatusId)
        {
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, SteadyMealMaximumSeconds);
        }
        else if (Entry.StatusId == BerryRecoveryStatusId)
        {
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, BerryRecoveryMaximumSeconds);
        }
        else if (Entry.StatusId == RatTailTonicStatusId)
        {
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, RatTailTonicMaximumSeconds);
        }
        else if (Entry.StatusId == DraughtCooldownStatusId)
        {
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, DraughtCooldownMaximumSeconds);
        }
        else if (Entry.StatusId == PoisonResistanceStatusId)
        {
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, AntidoteResistanceMaximumSeconds);
        }
        else
        {
            Entry.RemainingSeconds = FMath::Max(0.0f, Entry.RemainingSeconds - SafeDelta);
        }
        if (Entry.RemainingSeconds <= 0.0f) Entries.RemoveAt(Index);
    }
}

void UKalmalaPlayerStatusComponent::OnRep_Statuses()
{
    // Replication is presentation-only on clients. Server validation happens
    // before state is published and clients never sanitize or author entries.
}
