#include "KalmalaPlayerStatusComponent.h"

#include "Net/UnrealNetwork.h"
#include "KalmalaCampfire.h"

const FName UKalmalaPlayerStatusComponent::WetStatusId(TEXT("State.Wet"));
const FName UKalmalaPlayerStatusComponent::SteadyMealStatusId(TEXT("State.Food.SteadyMeal"));
const FName UKalmalaPlayerStatusComponent::RoastedFieldMeatItemId(TEXT("RoastedFieldMeat"));
const FName UKalmalaPlayerStatusComponent::HearthBrothItemId(TEXT("HearthBroth"));
const FName UKalmalaPlayerStatusComponent::SmokedFieldMeatItemId(TEXT("SmokedFieldMeat"));
const FName UKalmalaPlayerStatusComponent::DriedFieldMeatItemId(TEXT("DriedFieldMeat"));

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
    const float Maximum = StatusId == SteadyMealStatusId ? SteadyMealMaximumSeconds : WetMaximumSeconds;
    return Entry ? FMath::Clamp(Entry->RemainingSeconds, 0.0f, Maximum) : 0.0f;
}

bool UKalmalaPlayerStatusComponent::IsKnownFoodItem(const FName ItemId)
{
    return ItemId == RoastedFieldMeatItemId || ItemId == HearthBrothItemId
        || ItemId == SmokedFieldMeatItemId || ItemId == DriedFieldMeatItemId;
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

void UKalmalaPlayerStatusComponent::ApplyWetFromServer()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    ApplyWet(Statuses);
    GetOwner()->ForceNetUpdate();
}

void UKalmalaPlayerStatusComponent::AdvanceFromServer(const float DeltaSeconds)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
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
