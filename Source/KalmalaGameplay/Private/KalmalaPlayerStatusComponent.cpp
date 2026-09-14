#include "KalmalaPlayerStatusComponent.h"

#include "Net/UnrealNetwork.h"

const FName UKalmalaPlayerStatusComponent::WetStatusId(TEXT("State.Wet"));

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
    return Entry ? FMath::Clamp(Entry->RemainingSeconds, 0.0f, WetMaximumSeconds) : 0.0f;
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
    bool bSawWet = false;
    for (int32 Index = Entries.Num() - 1; Index >= 0; --Index)
    {
        FKalmalaPlayerStatusEntry& Entry = Entries[Index];
        if (Entry.StatusId.IsNone() || !FMath::IsFinite(Entry.RemainingSeconds) || (Entry.StatusId == WetStatusId && bSawWet))
        {
            Entries.RemoveAt(Index);
            continue;
        }
        if (Entry.StatusId == WetStatusId)
        {
            bSawWet = true;
            Entry.RemainingSeconds = FMath::Clamp(Entry.RemainingSeconds - SafeDelta, 0.0f, WetMaximumSeconds);
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
