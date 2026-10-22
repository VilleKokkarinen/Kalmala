#include "KalmalaWorldPopulationSaveGame.h"

namespace
{
    constexpr TCHAR BadgerRewardClaimPrefix[] = TEXT("badger-reward|");
    constexpr TCHAR MoonBearRewardClaimPrefix[] = TEXT("moonbear-reward|");
    constexpr TCHAR ViperRewardClaimPrefix[] = TEXT("viper-reward|");
}

void UKalmalaWorldPopulationSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig)
{
    WorldConfig = InWorldConfig;
    HarvestedSpawnIds.Reset();
    DefeatedSpawnIds.Reset();
    PendingWildlifeRewards.Reset();
}

bool UKalmalaWorldPopulationSaveGame::MatchesWorld(const FKalmalaWorldGenerationConfig& InWorldConfig) const
{
    return SchemaVersion == 1 && WorldConfig == InWorldConfig;
}

bool UKalmalaWorldPopulationSaveGame::IsHarvested(const FString& PersistentSpawnId) const
{
    return !PersistentSpawnId.IsEmpty() && HarvestedSpawnIds.Contains(PersistentSpawnId);
}

void UKalmalaWorldPopulationSaveGame::MarkHarvested(const FString& PersistentSpawnId)
{
    if (!PersistentSpawnId.IsEmpty())
    {
        HarvestedSpawnIds.Add(PersistentSpawnId);
    }
}

bool UKalmalaWorldPopulationSaveGame::IsDefeated(const FString& PersistentSpawnId) const
{
    return !PersistentSpawnId.IsEmpty() && DefeatedSpawnIds.Contains(PersistentSpawnId);
}

void UKalmalaWorldPopulationSaveGame::MarkDefeated(const FString& PersistentSpawnId)
{
    if (!PersistentSpawnId.IsEmpty())
    {
        DefeatedSpawnIds.Add(PersistentSpawnId);
    }
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingWildlifeReward(
    const FString& RewardClaimId, const FString& PlayerIdentity)
{
    if (RewardClaimId.IsEmpty() || RewardClaimId.Len() > 128
        || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    if (const FKalmalaPendingWildlifeReward* Existing = PendingWildlifeRewards.FindByPredicate(
        [&RewardClaimId](const FKalmalaPendingWildlifeReward& Reward)
        { return Reward.PersistentSpawnId == RewardClaimId; }))
    {
        return Existing->PlayerIdentity == PlayerIdentity;
    }
    if (PendingWildlifeRewards.Num() >= MaxPendingWildlifeRewards) return false;

    FKalmalaPendingWildlifeReward& Reward = PendingWildlifeRewards.AddDefaulted_GetRef();
    Reward.PersistentSpawnId = RewardClaimId;
    Reward.PlayerIdentity = PlayerIdentity;
    return true;
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingBadgerReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeBadgerPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeBadgerPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(BadgerRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(BadgerRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetBadgerSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(BadgerRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(BadgerRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingMoonBearReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeMoonBearPendingRewardClaimId(PersistentSpawnId);
    if (ClaimId.IsEmpty() || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    for (const FKalmalaPendingWildlifeReward& Existing : PendingWildlifeRewards)
    {
        FString ExistingSpawnId;
        if (TryGetMoonBearSpawnIdFromPendingRewardClaimId(Existing.PersistentSpawnId, ExistingSpawnId)
            && ExistingSpawnId == PersistentSpawnId)
        {
            return Existing.PlayerIdentity == PlayerIdentity;
        }
    }
    return TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(MoonBearRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(MoonBearRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetMoonBearSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(MoonBearRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(MoonBearRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::TryQueuePendingViperReward(
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    const FString ClaimId = MakeViperPendingRewardClaimId(PersistentSpawnId);
    return !ClaimId.IsEmpty() && TryQueuePendingWildlifeReward(ClaimId, PlayerIdentity);
}

FString UKalmalaWorldPopulationSaveGame::MakeViperPendingRewardClaimId(const FString& PersistentSpawnId)
{
    const int32 PrefixLength = UE_ARRAY_COUNT(ViperRewardClaimPrefix) - 1;
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128 - PrefixLength) return FString();
    return FString(ViperRewardClaimPrefix) + PersistentSpawnId;
}

bool UKalmalaWorldPopulationSaveGame::TryGetViperSpawnIdFromPendingRewardClaimId(
    const FString& ClaimId, FString& OutPersistentSpawnId)
{
    OutPersistentSpawnId.Reset();
    const int32 PrefixLength = UE_ARRAY_COUNT(ViperRewardClaimPrefix) - 1;
    if (!ClaimId.StartsWith(ViperRewardClaimPrefix, ESearchCase::CaseSensitive)
        || ClaimId.Len() <= PrefixLength || ClaimId.Len() > 128) return false;

    OutPersistentSpawnId = ClaimId.RightChop(PrefixLength);
    return !OutPersistentSpawnId.IsEmpty();
}

bool UKalmalaWorldPopulationSaveGame::RemovePendingWildlifeReward(const FString& RewardClaimId)
{
    if (RewardClaimId.IsEmpty()) return false;
    return PendingWildlifeRewards.RemoveAll([&RewardClaimId](const FKalmalaPendingWildlifeReward& Reward)
        { return Reward.PersistentSpawnId == RewardClaimId; }) > 0;
}
