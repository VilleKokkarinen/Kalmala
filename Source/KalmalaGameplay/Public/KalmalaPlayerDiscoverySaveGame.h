#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaToolLifecycleContract.h"
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
    friend class UKalmalaPlayerDiscoverySaveGameV2;
    friend class FKalmalaPlayerDiscoverySaveGameV2Test;

    UPROPERTY(SaveGame) int32 SchemaVersion = Schema;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) FString PlayerIdentity;
    UPROPERTY(SaveGame) TSet<FString> DiscoveryIds;
    UPROPERTY(SaveGame) TSet<FString> LearnedEffectIds;
};

USTRUCT()
struct KALMALAGAMEPLAY_API FKalmalaPlayerToolSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame) FName ToolId = NAME_None;
    UPROPERTY(SaveGame) int32 ToolLevel = 1;
    UPROPERTY(SaveGame) int32 Condition = 0;
};

/**
 * Isolated schema-2 candidate for player discoveries and carried tools.
 * Normal player slots stay on schema 1 until the complete M9 gate passes.
 */
UCLASS()
class KALMALAGAMEPLAY_API UKalmalaPlayerDiscoverySaveGameV2 : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 SchemaVersionValue = 2;
    static constexpr int32 MaxLegacyDiscoveries = UKalmalaPlayerDiscoverySaveGame::MaxDiscoveries;
    static constexpr int32 MaxM9Claims = 64;
    static constexpr int32 MaxCombinedDiscoveries = MaxLegacyDiscoveries + MaxM9Claims;
    static constexpr int32 MaxLearnedEffects = 4;
    static constexpr int32 MaxCarriedTools = FKalmalaToolLifecycleContract::MaxCarriedToolRecords;

    void InitializeForPlayer(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity);
    bool MatchesPlayer(const FKalmalaWorldGenerationConfig& InWorld, const FString& InPlayerIdentity) const;
    bool AddDiscovery(const FString& Id);
    bool AddM9Claim(const FString& Id);
    bool AddLearnedEffect(const FString& Id);
    bool AddToolRecord(const FKalmalaPlayerToolSaveRecord& Record);
    bool HasDiscovery(const FString& Id) const;
    bool HasLearnedEffect(const FString& Id) const;

    const TSet<FString>& GetDiscoveryIds() const { return DiscoveryIds; }
    const TArray<FString>& GetM9ClaimIds() const { return M9ClaimIds; }
    const TSet<FString>& GetLearnedEffectIds() const { return LearnedEffectIds; }
    const TArray<FKalmalaPlayerToolSaveRecord>& GetToolRecords() const { return ToolRecords; }

    static bool IsValidLegacyDiscoveryId(const FString& Id);
    static bool IsValidM9ClaimId(const FString& Id);
    static bool IsValidToolRecord(const FKalmalaPlayerToolSaveRecord& Record);
    static bool TryMigrateSchema1(
        const UKalmalaPlayerDiscoverySaveGame* Legacy,
        const FKalmalaWorldGenerationConfig& RequestedWorld,
        const FString& AuthenticatedPlayerIdentity,
        UObject* Outer,
        UKalmalaPlayerDiscoverySaveGameV2*& OutMigrated);

private:
    friend class FKalmalaPlayerDiscoverySaveGameV2Test;

    UPROPERTY(SaveGame) int32 SchemaVersion = SchemaVersionValue;
    UPROPERTY(SaveGame) FKalmalaWorldGenerationConfig WorldConfig;
    UPROPERTY(SaveGame) FKalmalaM7SaveIdentity Identity;
    UPROPERTY(SaveGame) FString PlayerIdentity;
    UPROPERTY(SaveGame) TSet<FString> DiscoveryIds;
    UPROPERTY(SaveGame) TArray<FString> M9ClaimIds;
    UPROPERTY(SaveGame) TSet<FString> LearnedEffectIds;
    UPROPERTY(SaveGame) TArray<FKalmalaPlayerToolSaveRecord> ToolRecords;
};
