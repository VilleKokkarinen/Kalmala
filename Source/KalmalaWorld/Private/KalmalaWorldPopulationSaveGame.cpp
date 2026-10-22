#include "KalmalaWorldPopulationSaveGame.h"

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
    const FString& PersistentSpawnId, const FString& PlayerIdentity)
{
    if (PersistentSpawnId.IsEmpty() || PersistentSpawnId.Len() > 128
        || PlayerIdentity.IsEmpty() || PlayerIdentity.Len() > 128) return false;

    if (const FKalmalaPendingWildlifeReward* Existing = PendingWildlifeRewards.FindByPredicate(
        [&PersistentSpawnId](const FKalmalaPendingWildlifeReward& Reward)
        { return Reward.PersistentSpawnId == PersistentSpawnId; }))
    {
        return Existing->PlayerIdentity == PlayerIdentity;
    }
    if (PendingWildlifeRewards.Num() >= MaxPendingWildlifeRewards) return false;

    FKalmalaPendingWildlifeReward& Reward = PendingWildlifeRewards.AddDefaulted_GetRef();
    Reward.PersistentSpawnId = PersistentSpawnId;
    Reward.PlayerIdentity = PlayerIdentity;
    return true;
}

bool UKalmalaWorldPopulationSaveGame::RemovePendingWildlifeReward(const FString& PersistentSpawnId)
{
    if (PersistentSpawnId.IsEmpty()) return false;
    return PendingWildlifeRewards.RemoveAll([&PersistentSpawnId](const FKalmalaPendingWildlifeReward& Reward)
        { return Reward.PersistentSpawnId == PersistentSpawnId; }) > 0;
}
