#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaWorldDescriptor.h"
#include "KalmalaM7PersistenceContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldDescriptorTest,
    "Kalmala.UI.WorldDescriptor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldDescriptorTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig Config;
    const uint64 Seeds[] = {0ull, 10323456789ull, MAX_uint64};
    for (const uint64 Seed : Seeds)
    {
        Config.WorldSeed = Seed;
        const FKalmalaWorldDescriptor Descriptor = FKalmalaWorldDescriptorAdapter::Describe(Config);
        TestTrue(TEXT("The exact supported seed is exposed"), Descriptor.Seed == Seed
            && Descriptor.SeedAvailability == EKalmalaWorldMetadataAvailability::Available);
        TestTrue(TEXT("The revision is the current identity revision"),
            Descriptor.CurrentIdentityRevision == FKalmalaM7SaveIdentity::ForWorld(Seed).GeneratorRevision
            && Descriptor.IdentityRevisionAvailability == EKalmalaWorldMetadataAvailability::Available);
        TestTrue(TEXT("A config does not establish save existence or compatibility"),
            Descriptor.SavedWorldExistence == EKalmalaWorldMetadataAvailability::Unknown
            && Descriptor.SavedWorldCompatibility == EKalmalaWorldMetadataAvailability::Unknown);
        TestTrue(TEXT("Stored revision, display name and time are unavailable"),
            Descriptor.SavedGeneratorRevision == EKalmalaWorldMetadataAvailability::Unavailable
            && Descriptor.DisplayName == EKalmalaWorldMetadataAvailability::Unavailable
            && Descriptor.LastPlayed == EKalmalaWorldMetadataAvailability::Unavailable);
    }
    TestTrue(TEXT("Unsupported enumeration is not an empty successful list"),
        FKalmalaWorldDescriptorAdapter::WorldEnumerationAvailability() == EKalmalaWorldMetadataAvailability::Unavailable);
    return true;
}
#endif
