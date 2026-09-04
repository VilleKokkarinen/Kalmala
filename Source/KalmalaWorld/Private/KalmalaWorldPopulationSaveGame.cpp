#include "KalmalaWorldPopulationSaveGame.h"

void UKalmalaWorldPopulationSaveGame::InitializeForWorld(const FKalmalaWorldGenerationConfig& InWorldConfig)
{
    WorldConfig = InWorldConfig;
    HarvestedSpawnIds.Reset();
    DefeatedSpawnIds.Reset();
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
