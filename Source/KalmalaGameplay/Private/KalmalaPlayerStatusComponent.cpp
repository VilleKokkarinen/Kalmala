#include "KalmalaPlayerStatusComponent.h"

#include "Net/UnrealNetwork.h"
#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"

const FName UKalmalaPlayerStatusComponent::WetStatusId(TEXT("State.Wet"));
const FName UKalmalaPlayerStatusComponent::SteadyMealStatusId(TEXT("State.Food.SteadyMeal"));
const FName UKalmalaPlayerStatusComponent::CookedBoarMeatItemId(TEXT("CookedBoarMeat"));
const FName UKalmalaPlayerStatusComponent::CookedDeerMeatItemId(TEXT("CookedDeerMeat"));
const FName UKalmalaPlayerStatusComponent::HearthBrothItemId(TEXT("HearthBroth"));
const FName UKalmalaPlayerStatusComponent::LingonberryItemId(TEXT("Lingonberry"));
const FName UKalmalaPlayerStatusComponent::ForestChanterelleItemId(TEXT("ForestChanterelle"));
const FName UKalmalaPlayerStatusComponent::BerryRecoveryDraughtItemId(TEXT("BerryRecoveryDraught"));
const FName UKalmalaPlayerStatusComponent::BerryRecoveryStatusId(TEXT("State.Recovery.BerryDraught"));
const FName UKalmalaPlayerStatusComponent::DraughtCooldownStatusId(TEXT("State.Recovery.DraughtCooldown"));
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
    DOREPLIFETIME(UKalmalaPlayerStatusComponent, Statuses);
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
    const float Maximum = StatusId == SteadyMealStatusId ? SteadyMealMaximumSeconds
        : StatusId == BerryRecoveryStatusId ? BerryRecoveryMaximumSeconds
        : StatusId == DraughtCooldownStatusId ? DraughtCooldownMaximumSeconds : WetMaximumSeconds;
    return Entry ? FMath::Clamp(Entry->RemainingSeconds, 0.0f, Maximum) : 0.0f;
}

bool UKalmalaPlayerStatusComponent::IsKnownFoodItem(const FName ItemId)
{
    return ItemId == CookedBoarMeatItemId || ItemId == CookedDeerMeatItemId
        || ItemId == MeatStewItemId || ItemId == RootVegetableSoupItemId
        || ItemId == RoastedRootVegetablesItemId || ItemId == DeerRootRoastItemId
        || ItemId == HearthBrothItemId || ItemId == LingonberryItemId
        || ItemId == ForestChanterelleItemId;
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
    return ItemId == BerryRecoveryDraughtItemId;
}

bool UKalmalaPlayerStatusComponent::ApplyDraught(TArray<FKalmalaPlayerStatusEntry>& Entries, const FName ItemId)
{
    if (!IsKnownDraughtItem(ItemId)
        || Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == DraughtCooldownStatusId && FMath::IsFinite(Entry.RemainingSeconds) && Entry.RemainingSeconds > 0.0f; })
        || Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == BerryRecoveryStatusId && FMath::IsFinite(Entry.RemainingSeconds) && Entry.RemainingSeconds > 0.0f; }))
        return false;

    Advance(Entries, 0.0f);
    FKalmalaPlayerStatusEntry& Recovery = Entries.AddDefaulted_GetRef();
    Recovery.StatusId = BerryRecoveryStatusId;
    Recovery.RemainingSeconds = BerryRecoveryMaximumSeconds;
    FKalmalaPlayerStatusEntry& Cooldown = Entries.AddDefaulted_GetRef();
    Cooldown.StatusId = DraughtCooldownStatusId;
    Cooldown.RemainingSeconds = DraughtCooldownMaximumSeconds;
    return true;
}

bool UKalmalaPlayerStatusComponent::CanApplyDraughtFromServer(const FName ItemId) const
{
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
    return IsValid(Character) && Character->HasAuthority() && IsKnownDraughtItem(ItemId)
        && Character->GetHealth() > 1.0f && Character->GetHealth() < 100.0f
        && !HasStatus(DraughtCooldownStatusId) && !HasStatus(BerryRecoveryStatusId);
}

bool UKalmalaPlayerStatusComponent::ApplyDraughtFromServer(const FName ItemId)
{
    if (!CanApplyDraughtFromServer(ItemId) || !ApplyDraught(Statuses, ItemId)) return false;
    GetOwner()->ForceNetUpdate();
    return true;
}

void UKalmalaPlayerStatusComponent::ApplyWetFromServer()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    ApplyWet(Statuses);
    GetOwner()->ForceNetUpdate();
}

void UKalmalaPlayerStatusComponent::AdvanceFromServer(const float DeltaSeconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    const float SafeDelta = FMath::Max(0.0f, FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f);
    const int32 RecoveryIndex = Statuses.IndexOfByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
    {
        return Entry.StatusId == BerryRecoveryStatusId && FMath::IsFinite(Entry.RemainingSeconds) && Entry.RemainingSeconds > 0.0f;
    });
    if (RecoveryIndex != INDEX_NONE)
    {
        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwner());
        if (!Character || Character->GetHealth() <= 1.0f)
        {
            Statuses.RemoveAt(RecoveryIndex);
        }
        else
        {
            const float Elapsed = FMath::Min(SafeDelta, Statuses[RecoveryIndex].RemainingSeconds);
            if (Elapsed > 0.0f)
                Character->RestoreHealthFromDraughtFromServer(BerryRecoveryHealth * Elapsed / BerryRecoveryMaximumSeconds);
        }
    }
    Advance(Statuses, DeltaSeconds);
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
        else if (Entry.StatusId == DraughtCooldownStatusId)
        {
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, DraughtCooldownMaximumSeconds);
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
