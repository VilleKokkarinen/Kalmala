#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "KalmalaM7PersistenceContract.generated.h"

/** The save scope determines whether an identity may carry private player state. */
UENUM()
enum class EKalmalaM7SaveScope : uint8
{
    World,
    Player
};

/** Only generated-content facts are admitted by the first M7 persistence slice. */
UENUM()
enum class EKalmalaM7SparseDeltaKind : uint8
{
    ResourceDepleted,
    CreatureDefeated,
    DiscoveryClaimed
};

/** Explicit load policy: migration is never implicit and future data fails closed. */
UENUM()
enum class EKalmalaM7SchemaDecision : uint8
{
    AcceptCurrent,
    MigrateBeforeLoad,
    Reject
};

USTRUCT()
struct KALMALAWORLD_API FKalmalaM7SaveIdentity
{
    GENERATED_BODY()

    static constexpr int32 CurrentGeneratorRevision = 7;
    static constexpr int32 MaxOwnerIdentityLength = 128;

    UPROPERTY(SaveGame)
    uint64 WorldSeed = 0;

    UPROPERTY(SaveGame)
    int32 GeneratorRevision = CurrentGeneratorRevision;

    UPROPERTY(SaveGame)
    EKalmalaM7SaveScope Scope = EKalmalaM7SaveScope::World;

    /** Empty for world-scoped state; opaque authenticated identity for player state. */
    UPROPERTY(SaveGame)
    FString OwnerIdentity;

    static FKalmalaM7SaveIdentity ForWorld(uint64 InWorldSeed);
    static FKalmalaM7SaveIdentity ForPlayer(uint64 InWorldSeed, const FString& InOwnerIdentity);
    bool IsValid() const;
    bool Matches(const FKalmalaM7SaveIdentity& Other) const;
};

USTRUCT()
struct KALMALAWORLD_API FKalmalaM7SparseDelta
{
    GENERATED_BODY()

    static constexpr int32 MaxStableIdLength = 128;

    UPROPERTY(SaveGame)
    EKalmalaM7SparseDeltaKind Kind = EKalmalaM7SparseDeltaKind::ResourceDepleted;

    /** Server-selected stable identity; this is not a client-provided target. */
    UPROPERTY(SaveGame)
    FString StableId;

    static bool IsValidStableId(const FString& InStableId);
    bool IsValid() const;
};

/**
 * Versioned M7 gate container. It intentionally holds only bounded sparse
 * generated-content facts until progression, tools, food, and loot contracts
 * are approved in later increments.
 */
UCLASS()
class KALMALAWORLD_API UKalmalaM7PersistenceSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSchemaVersion = 1;
    static constexpr int32 MaxSparseDeltas = 256;

    void Initialize(const FKalmalaM7SaveIdentity& InIdentity);
    bool Matches(const FKalmalaM7SaveIdentity& InIdentity) const;
    static EKalmalaM7SchemaDecision EvaluateSchemaVersion(int32 CandidateSchemaVersion);
    bool AddSparseDelta(const FKalmalaM7SparseDelta& Delta);
    bool HasSparseDelta(EKalmalaM7SparseDeltaKind Kind, const FString& StableId) const;
    int32 GetSparseDeltaCount() const { return SparseDeltas.Num(); }
    const TArray<FKalmalaM7SparseDelta>& GetSparseDeltas() const { return SparseDeltas; }

private:
    UPROPERTY(SaveGame)
    int32 SchemaVersion = CurrentSchemaVersion;

    UPROPERTY(SaveGame)
    FKalmalaM7SaveIdentity Identity;

    UPROPERTY(SaveGame)
    TArray<FKalmalaM7SparseDelta> SparseDeltas;
};
