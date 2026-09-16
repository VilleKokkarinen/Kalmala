#include "KalmalaPlayerDiscoverySaveGame.h"
void UKalmalaPlayerDiscoverySaveGame::InitializeForPlayer(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity)
{ SchemaVersion = Schema; WorldConfig = InWorld; PlayerIdentity = InPlayerIdentity; DiscoveryIds.Reset(); }
bool UKalmalaPlayerDiscoverySaveGame::Matches(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity) const
{ return SchemaVersion == Schema && WorldConfig == InWorld && !InPlayerIdentity.IsEmpty() && PlayerIdentity == InPlayerIdentity; }
bool UKalmalaPlayerDiscoverySaveGame::HasDiscovery(const FString& Id) const { return !Id.IsEmpty() && DiscoveryIds.Contains(Id); }
bool UKalmalaPlayerDiscoverySaveGame::AddDiscovery(const FString& Id)
{ if (Id.IsEmpty() || DiscoveryIds.Contains(Id) || DiscoveryIds.Num() >= MaxDiscoveries) return false; DiscoveryIds.Add(Id); return true; }
void UKalmalaPlayerDiscoverySaveGame::RemoveDiscovery(const FString& Id) { DiscoveryIds.Remove(Id); }
