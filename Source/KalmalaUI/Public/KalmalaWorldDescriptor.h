#pragma once

#include "CoreMinimal.h"
#include "KalmalaWorldGenerationConfig.h"

// A configuration describes a candidate/current session, not an enumerated save.
enum class EKalmalaWorldMetadataAvailability : uint8
{
    Available,
    Unavailable,
    Unknown
};

struct KALMALAUI_API FKalmalaWorldDescriptor
{
    uint64 Seed = 0;
    // Revision used for a new M7 world identity; it is not read from a save.
    int32 CurrentIdentityRevision = 0;
    EKalmalaWorldMetadataAvailability SeedAvailability = EKalmalaWorldMetadataAvailability::Unavailable;
    EKalmalaWorldMetadataAvailability IdentityRevisionAvailability = EKalmalaWorldMetadataAvailability::Unavailable;
    EKalmalaWorldMetadataAvailability SavedWorldExistence = EKalmalaWorldMetadataAvailability::Unknown;
    EKalmalaWorldMetadataAvailability SavedWorldCompatibility = EKalmalaWorldMetadataAvailability::Unknown;
    EKalmalaWorldMetadataAvailability SavedGeneratorRevision = EKalmalaWorldMetadataAvailability::Unavailable;
    EKalmalaWorldMetadataAvailability DisplayName = EKalmalaWorldMetadataAvailability::Unavailable;
    EKalmalaWorldMetadataAvailability LastPlayed = EKalmalaWorldMetadataAvailability::Unavailable;
};

/** Read-only view of supplied runtime configuration; no save, travel, or session operations. */
class KALMALAUI_API FKalmalaWorldDescriptorAdapter
{
public:
    static FKalmalaWorldDescriptor Describe(const FKalmalaWorldGenerationConfig& Config);
    // Unsupported is distinct from a successful enumeration returning no worlds.
    static EKalmalaWorldMetadataAvailability WorldEnumerationAvailability();
};
