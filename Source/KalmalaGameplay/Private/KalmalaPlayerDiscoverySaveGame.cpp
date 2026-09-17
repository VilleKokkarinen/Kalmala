#include "KalmalaPlayerDiscoverySaveGame.h"
void UKalmalaPlayerDiscoverySaveGame::InitializeForPlayer(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity)
{ SchemaVersion = Schema; WorldConfig = InWorld; PlayerIdentity = InPlayerIdentity; DiscoveryIds.Reset(); LearnedEffectIds.Reset(); }
bool UKalmalaPlayerDiscoverySaveGame::Matches(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity) const
{ return SchemaVersion == Schema && WorldConfig == InWorld && !InPlayerIdentity.IsEmpty() && PlayerIdentity == InPlayerIdentity; }
bool UKalmalaPlayerDiscoverySaveGame::HasDiscovery(const FString& Id) const { return !Id.IsEmpty() && DiscoveryIds.Contains(Id); }
bool UKalmalaPlayerDiscoverySaveGame::AddDiscovery(const FString& Id)
{ if (Id.IsEmpty() || DiscoveryIds.Contains(Id) || DiscoveryIds.Num() >= MaxDiscoveries) return false; DiscoveryIds.Add(Id); return true; }
void UKalmalaPlayerDiscoverySaveGame::RemoveDiscovery(const FString& Id) { DiscoveryIds.Remove(Id); }
bool UKalmalaPlayerDiscoverySaveGame::HasLearnedEffect(const FString& Id) const { return !Id.IsEmpty() && LearnedEffectIds.Contains(Id); }
bool UKalmalaPlayerDiscoverySaveGame::AddLearnedEffect(const FString& Id)
{ if ((Id != TEXT("Effect:mending") && Id != TEXT("Effect:hearth-shield") && Id != TEXT("Effect:bears-vigor") && Id != TEXT("Effect:deer-call")) || LearnedEffectIds.Contains(Id) || LearnedEffectIds.Num() >= MaxDiscoveries) return false; LearnedEffectIds.Add(Id); return true; }
void UKalmalaPlayerDiscoverySaveGame::RemoveLearnedEffect(const FString& Id) { LearnedEffectIds.Remove(Id); }
