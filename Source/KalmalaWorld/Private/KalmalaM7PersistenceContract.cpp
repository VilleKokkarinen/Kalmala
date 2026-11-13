#include "KalmalaM7PersistenceContract.h"

namespace KalmalaM7Persistence
{
    bool IsIdentityCharacter(const TCHAR Character)
    {
        return (Character >= 'a' && Character <= 'z')
            || (Character >= 'A' && Character <= 'Z')
            || (Character >= '0' && Character <= '9')
            || Character == ':'
            || Character == '-'
            || Character == '_'
            || Character == '.';
    }
}

FKalmalaM7SaveIdentity FKalmalaM7SaveIdentity::ForWorld(const uint64 InWorldSeed)
{
    FKalmalaM7SaveIdentity Identity;
    Identity.WorldSeed = InWorldSeed;
    Identity.GeneratorRevision = CurrentGeneratorRevision;
    Identity.Scope = EKalmalaM7SaveScope::World;
    Identity.OwnerIdentity.Reset();
    return Identity;
}

FKalmalaM7SaveIdentity FKalmalaM7SaveIdentity::ForPlayer(const uint64 InWorldSeed, const FString& InOwnerIdentity)
{
    FKalmalaM7SaveIdentity Identity = ForWorld(InWorldSeed);
    Identity.Scope = EKalmalaM7SaveScope::Player;
    Identity.OwnerIdentity = InOwnerIdentity;
    return Identity;
}

bool FKalmalaM7SaveIdentity::IsValid() const
{
    if (GeneratorRevision != CurrentGeneratorRevision)
    {
        return false;
    }

    if (Scope == EKalmalaM7SaveScope::World)
    {
        return OwnerIdentity.IsEmpty();
    }

    if (Scope != EKalmalaM7SaveScope::Player || OwnerIdentity.IsEmpty() || OwnerIdentity.Len() > MaxOwnerIdentityLength)
    {
        return false;
    }

    for (const TCHAR Character : OwnerIdentity)
    {
        if (!KalmalaM7Persistence::IsIdentityCharacter(Character))
        {
            return false;
        }
    }
    return true;
}

bool FKalmalaM7SaveIdentity::Matches(const FKalmalaM7SaveIdentity& Other) const
{
    return IsValid() && Other.IsValid()
        && WorldSeed == Other.WorldSeed
        && GeneratorRevision == Other.GeneratorRevision
        && Scope == Other.Scope
        && OwnerIdentity == Other.OwnerIdentity;
}

bool FKalmalaM7SparseDelta::IsValidStableId(const FString& InStableId)
{
    if (InStableId.IsEmpty() || InStableId.Len() > MaxStableIdLength)
    {
        return false;
    }

    for (int32 Index = 0; Index < InStableId.Len(); ++Index)
    {
        const TCHAR Character = InStableId[Index];
        const bool bAlphaNumeric = (Character >= 'a' && Character <= 'z')
            || (Character >= 'A' && Character <= 'Z')
            || (Character >= '0' && Character <= '9');
        const bool bStableSeparator = Character == ':' || Character == '/' || Character == ',' || Character == '-' || Character == '_';
        if ((!bAlphaNumeric && !bStableSeparator) || (Index == 0 && !bAlphaNumeric))
        {
            return false;
        }
    }
    return true;
}

bool FKalmalaM7SparseDelta::IsValid() const
{
    return (Kind == EKalmalaM7SparseDeltaKind::ResourceDepleted
        || Kind == EKalmalaM7SparseDeltaKind::CreatureDefeated
        || Kind == EKalmalaM7SparseDeltaKind::DiscoveryClaimed
        || Kind == EKalmalaM7SparseDeltaKind::DiscoveryRewardPending)
        && IsValidStableId(StableId);
}

void UKalmalaM7PersistenceSaveGame::Initialize(const FKalmalaM7SaveIdentity& InIdentity)
{
    SchemaVersion = CurrentSchemaVersion;
    Identity = InIdentity;
    SparseDeltas.Reset();
}

bool UKalmalaM7PersistenceSaveGame::Matches(const FKalmalaM7SaveIdentity& InIdentity) const
{
    if (SchemaVersion != CurrentSchemaVersion || !Identity.Matches(InIdentity) || SparseDeltas.Num() > MaxSparseDeltas)
    {
        return false;
    }

    TSet<FString> Seen;
    TSet<FString> OceanClaimTransactionIds;
    for (const FKalmalaM7SparseDelta& Delta : SparseDeltas)
    {
        if (!Delta.IsValid()
            || (Delta.Kind == EKalmalaM7SparseDeltaKind::DiscoveryRewardPending
                && Identity.Scope != EKalmalaM7SaveScope::Player))
        {
            return false;
        }

        if (Delta.Kind == EKalmalaM7SparseDeltaKind::DiscoveryClaimed
            || Delta.Kind == EKalmalaM7SparseDeltaKind::DiscoveryRewardPending)
        {
            if (OceanClaimTransactionIds.Contains(Delta.StableId)) return false;
            OceanClaimTransactionIds.Add(Delta.StableId);
        }

        const FString Key = FString::Printf(TEXT("%d:%s"), static_cast<uint8>(Delta.Kind), *Delta.StableId);
        if (Seen.Contains(Key))
        {
            return false;
        }
        Seen.Add(Key);
    }
    return true;
}

EKalmalaM7SchemaDecision UKalmalaM7PersistenceSaveGame::EvaluateSchemaVersion(const int32 CandidateSchemaVersion)
{
    if (CandidateSchemaVersion == CurrentSchemaVersion)
    {
        return EKalmalaM7SchemaDecision::AcceptCurrent;
    }

    return EKalmalaM7SchemaDecision::Reject;
}

bool UKalmalaM7PersistenceSaveGame::AddSparseDelta(const FKalmalaM7SparseDelta& Delta)
{
    if (!Matches(Identity) || !Delta.IsValid() || SparseDeltas.Num() >= MaxSparseDeltas
        || (Delta.Kind == EKalmalaM7SparseDeltaKind::DiscoveryRewardPending
            && Identity.Scope != EKalmalaM7SaveScope::Player))
    {
        return false;
    }

    if (SparseDeltas.ContainsByPredicate([&Delta](const FKalmalaM7SparseDelta& Existing)
        {
            const bool bOceanClaimState = Delta.Kind == EKalmalaM7SparseDeltaKind::DiscoveryClaimed
                || Delta.Kind == EKalmalaM7SparseDeltaKind::DiscoveryRewardPending;
            const bool bExistingOceanClaimState = Existing.Kind == EKalmalaM7SparseDeltaKind::DiscoveryClaimed
                || Existing.Kind == EKalmalaM7SparseDeltaKind::DiscoveryRewardPending;
            return Existing.StableId == Delta.StableId
                && ((Existing.Kind == Delta.Kind) || (bOceanClaimState && bExistingOceanClaimState));
        }))
    {
        return false;
    }

    SparseDeltas.Add(Delta);
    return true;
}

bool UKalmalaM7PersistenceSaveGame::RemoveSparseDelta(
    const EKalmalaM7SparseDeltaKind Kind, const FString& StableId)
{
    if (!Matches(Identity)) return false;
    return SparseDeltas.RemoveAll([Kind, &StableId](const FKalmalaM7SparseDelta& Delta)
    {
        return Delta.Kind == Kind && Delta.StableId == StableId;
    }) == 1;
}

bool UKalmalaM7PersistenceSaveGame::HasSparseDelta(const EKalmalaM7SparseDeltaKind Kind, const FString& StableId) const
{
    return SparseDeltas.ContainsByPredicate([Kind, &StableId](const FKalmalaM7SparseDelta& Delta)
        {
            return Delta.Kind == Kind && Delta.StableId == StableId;
        });
}
