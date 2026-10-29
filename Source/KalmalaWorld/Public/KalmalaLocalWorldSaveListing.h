#pragma once

#include "CoreMinimal.h"

enum class EKalmalaLocalWorldSaveKind : uint8
{
    Population,
    Construction,
    Storage,
    OceanTravel
};

enum class EKalmalaLocalWorldSaveListingAvailability : uint8
{
    Supported,
    Unsupported
};

/** Generator-revision evidence from recognized save names; it says nothing about save payload compatibility. */
enum class EKalmalaNamedSaveRevisionStatus : uint8
{
    Unknown,
    Current,
    Different,
    Conflicting
};

/** Next menu action for a seed; NoKnownRecord is not proof that the seed has no saves. */
enum class EKalmalaWorldSeedReuseDecision : uint8
{
    NoKnownRecord,
    ConfirmExisting,
    ExistingSeedNeedsReview
};

/** A seed recovered from a recognized world-scoped save slot name. */
struct KALMALAWORLD_API FKalmalaLocalWorldSaveRecord
{
    uint64 Seed = 0;
    bool bHasPopulation = false;
    bool bHasConstruction = false;
    bool bHasStorage = false;
    bool bHasOceanTravel = false;
    bool bHasStoredGeneratorRevision = false;
    bool bHasConflictingGeneratorRevisions = false;
    int32 StoredGeneratorRevision = 0;

    FString GetDisplayLabel() const;
    EKalmalaNamedSaveRevisionStatus GetNamedSaveRevisionStatus(int32 CurrentRevision) const;
};

/** Read-only enumeration of known world-owned slots; it does not load or mutate saves. */
class KALMALAWORLD_API FKalmalaLocalWorldSaveListing
{
public:
    static EKalmalaLocalWorldSaveListingAvailability Enumerate(TArray<FKalmalaLocalWorldSaveRecord>& OutRecords);
    static EKalmalaWorldSeedReuseDecision ResolveSeedReuse(
        EKalmalaLocalWorldSaveListingAvailability Availability,
        const TArray<FKalmalaLocalWorldSaveRecord>& Records, uint64 Seed, int32 CurrentRevision);
    static bool ParseSlotName(const FString& SlotName, EKalmalaLocalWorldSaveKind& OutKind,
        uint64& OutSeed, bool& bOutHasGeneratorRevision, int32& OutGeneratorRevision);
};
