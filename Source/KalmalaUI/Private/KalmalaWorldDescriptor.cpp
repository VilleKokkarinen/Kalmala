#include "KalmalaWorldDescriptor.h"
#include "KalmalaM7PersistenceContract.h"

FKalmalaWorldDescriptor FKalmalaWorldDescriptorAdapter::Describe(const FKalmalaWorldGenerationConfig& Config)
{
    FKalmalaWorldDescriptor Result;
    if (Config.IsValid())
    {
        Result.Seed = Config.WorldSeed;
        Result.CurrentIdentityRevision = FKalmalaM7SaveIdentity::ForWorld(Config.WorldSeed).GeneratorRevision;
        Result.SeedAvailability = EKalmalaWorldMetadataAvailability::Available;
        Result.IdentityRevisionAvailability = EKalmalaWorldMetadataAvailability::Available;
    }
    return Result;
}

FKalmalaWorldSaveInspection FKalmalaWorldDescriptorAdapter::InspectKnownSeed(const uint64 Seed)
{
    return FKalmalaWorldSaveInspector::Inspect(Seed);
}

EKalmalaWorldMetadataAvailability FKalmalaWorldDescriptorAdapter::WorldEnumerationAvailability()
{
    return EKalmalaWorldMetadataAvailability::Unavailable;
}
