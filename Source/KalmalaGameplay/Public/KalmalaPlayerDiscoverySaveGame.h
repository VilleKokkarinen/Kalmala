#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaPlayerDiscoverySaveGame.generated.h"

/** Bounded identity-scoped discovery facts; never replicated as a catalogue. */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaPlayerDiscoverySaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    static constexpr int32 Schema = 1;
    static constexpr int32 MaxDiscoveries = 64;
    void InitializeForPlayer(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity);
    bool Matches(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity) const;
    bool HasDiscovery(const FString& Id) const;
    bool AddDiscovery(const FString& Id);
    void RemoveDiscovery(const FString& Id);
    bool HasLearnedEffect(const FString& Id) const;
    bool AddLearnedEffect(const FString& Id);
    void RemoveLearnedEffect(const FString& Id);
private:
    UPROPERTY(SaveGame) int32 SchemaVersion = Schema;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) FString PlayerIdentity;
    UPROPERTY(SaveGame) TSet<FString> DiscoveryIds;
    UPROPERTY(SaveGame) TSet<FString> LearnedEffectIds;
};
