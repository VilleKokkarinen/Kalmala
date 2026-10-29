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

/** A seed recovered from a recognized world-scoped save slot name. */
struct KALMALAUI_API FKalmalaLocalWorldSaveRecord
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
};

/** Read-only enumeration of known world-owned slots; it does not load or mutate saves. */
class KALMALAUI_API FKalmalaLocalWorldSaveListing
{
public:
    static EKalmalaLocalWorldSaveListingAvailability Enumerate(TArray<FKalmalaLocalWorldSaveRecord>& OutRecords);
    static bool ParseSlotName(const FString& SlotName, EKalmalaLocalWorldSaveKind& OutKind,
        uint64& OutSeed, bool& bOutHasGeneratorRevision, int32& OutGeneratorRevision);
};
