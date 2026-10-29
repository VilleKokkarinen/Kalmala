#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWorldListWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldListAvailabilityTest, "Kalmala.UI.WorldList.Availability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldListAvailabilityTest::RunTest(const FString& Parameters)
{
    const FString Unsupported(UKalmalaWorldListWidget::EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability::Unavailable));
    const FString Unknown(UKalmalaWorldListWidget::EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability::Unknown));
    const FString Available(UKalmalaWorldListWidget::EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability::Available));
    TestTrue(TEXT("Unsupported listing is explicitly reported"), Unsupported.Contains(TEXT("not supported")));
    TestTrue(TEXT("Unknown availability is not presented as empty"), Unknown.Contains(TEXT("could not be determined")));
    TestFalse(TEXT("Available metadata alone does not prove an empty world list"), Available.Contains(TEXT("No worlds")));
    TestTrue(TEXT("The descriptor adapter does not claim project-level enumeration support"),
        FKalmalaWorldDescriptorAdapter::WorldEnumerationAvailability() == EKalmalaWorldMetadataAvailability::Unavailable);
    const FString ReusePrompt = UKalmalaWorldListWidget::ReuseConfirmationMessage(42, 7);
    TestTrue(TEXT("Reuse confirmation names the exact seed and revision"),
        ReusePrompt.Contains(TEXT("42")) && ReusePrompt.Contains(TEXT("revision 7")));
    TestTrue(TEXT("Reuse confirmation identifies evidence limits and the read-only result"),
        ReusePrompt.Contains(TEXT("not been validated")) && ReusePrompt.Contains(TEXT("will not change saves")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldSaveSlotNameTest, "Kalmala.UI.WorldList.SaveSlotNames",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldSaveSlotNameTest::RunTest(const FString& Parameters)
{
    EKalmalaLocalWorldSaveKind Kind = EKalmalaLocalWorldSaveKind::Population;
    uint64 Seed = 0;
    bool bHasRevision = false;
    int32 Revision = 0;
    TestTrue(TEXT("Zero-seed population slot is recognized"),
        FKalmalaLocalWorldSaveListing::ParseSlotName(TEXT("KalmalaPopulationDeltas_0"), Kind, Seed, bHasRevision, Revision));
    TestTrue(TEXT("Population kind is preserved"), Kind == EKalmalaLocalWorldSaveKind::Population);
    TestEqual(TEXT("Zero seed is preserved"), Seed, static_cast<uint64>(0));
    TestFalse(TEXT("Population slot has no stored revision"), bHasRevision);

    TestTrue(TEXT("Maximum uint64 storage seed is recognized"),
        FKalmalaLocalWorldSaveListing::ParseSlotName(TEXT("KalmalaStorage_18446744073709551615"), Kind, Seed, bHasRevision, Revision));
    TestEqual(TEXT("Maximum uint64 seed is preserved"), Seed, MAX_uint64);

    TestTrue(TEXT("Ocean travel slot exposes its revision"),
        FKalmalaLocalWorldSaveListing::ParseSlotName(TEXT("KalmalaOceanTravelWorld_42_7"), Kind, Seed, bHasRevision, Revision));
    TestTrue(TEXT("Ocean-travel kind is preserved"), Kind == EKalmalaLocalWorldSaveKind::OceanTravel);
    TestTrue(TEXT("Ocean slot revision is marked present"), bHasRevision);
    TestEqual(TEXT("Ocean slot revision is preserved"), Revision, 7);

    FKalmalaLocalWorldSaveRecord RevisionRecord;
    TestTrue(TEXT("A recognized slot without revision evidence remains unknown"),
        RevisionRecord.GetNamedSaveRevisionStatus(7) == EKalmalaNamedSaveRevisionStatus::Unknown);
    RevisionRecord.bHasStoredGeneratorRevision = true;
    RevisionRecord.StoredGeneratorRevision = 7;
    TestTrue(TEXT("A listed current revision is identified"),
        RevisionRecord.GetNamedSaveRevisionStatus(7) == EKalmalaNamedSaveRevisionStatus::Current);
    TestTrue(TEXT("A listed different revision is not called current"),
        RevisionRecord.GetNamedSaveRevisionStatus(8) == EKalmalaNamedSaveRevisionStatus::Different);
    RevisionRecord.bHasStoredGeneratorRevision = false;
    RevisionRecord.bHasConflictingGeneratorRevisions = true;
    TestTrue(TEXT("Conflicting listed revisions remain explicit"),
        RevisionRecord.GetNamedSaveRevisionStatus(7) == EKalmalaNamedSaveRevisionStatus::Conflicting);

    TArray<FKalmalaLocalWorldSaveRecord> Records;
    TestTrue(TEXT("No matching slot is only reported as no known record"),
        FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            EKalmalaLocalWorldSaveListingAvailability::Supported, Records, 42, 7)
            == EKalmalaWorldSeedReuseDecision::NoKnownRecord);
    TestTrue(TEXT("Unsupported enumeration must not imply a seed is unused"),
        FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            EKalmalaLocalWorldSaveListingAvailability::Unsupported, Records, 42, 7)
            == EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview);

    FKalmalaLocalWorldSaveRecord Existing;
    Existing.Seed = 42;
    Existing.bHasOceanTravel = true;
    Existing.bHasStoredGeneratorRevision = true;
    Existing.StoredGeneratorRevision = 7;
    Records.Add(Existing);
    TestTrue(TEXT("An exact seed and revision requires explicit confirmation"),
        FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            EKalmalaLocalWorldSaveListingAvailability::Supported, Records, 42, 7)
            == EKalmalaWorldSeedReuseDecision::ConfirmExisting);
    TestTrue(TEXT("A different seed does not match the listed record"),
        FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            EKalmalaLocalWorldSaveListingAvailability::Supported, Records, 43, 7)
            == EKalmalaWorldSeedReuseDecision::NoKnownRecord);

    Existing.Seed = 43;
    Existing.bHasStoredGeneratorRevision = false;
    Existing.bHasPopulation = true;
    Records.Add(Existing);
    TestTrue(TEXT("A seed without revision evidence requires review"),
        FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            EKalmalaLocalWorldSaveListingAvailability::Supported, Records, 43, 7)
            == EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview);

    Existing.Seed = 42;
    Records.Add(Existing);
    TestTrue(TEXT("An exact match plus uncertain same-seed evidence requires review"),
        FKalmalaLocalWorldSaveListing::ResolveSeedReuse(
            EKalmalaLocalWorldSaveListingAvailability::Supported, Records, 42, 7)
            == EKalmalaWorldSeedReuseDecision::ExistingSeedNeedsReview);

    TestFalse(TEXT("Trailing seed text is rejected"),
        FKalmalaLocalWorldSaveListing::ParseSlotName(TEXT("KalmalaConstruction_42x"), Kind, Seed, bHasRevision, Revision));
    TestFalse(TEXT("Seed overflow is rejected"),
        FKalmalaLocalWorldSaveListing::ParseSlotName(TEXT("KalmalaStorage_18446744073709551616"), Kind, Seed, bHasRevision, Revision));
    TestFalse(TEXT("Negative revision is rejected"),
        FKalmalaLocalWorldSaveListing::ParseSlotName(TEXT("KalmalaOceanTravelWorld_42_-1"), Kind, Seed, bHasRevision, Revision));
    TestFalse(TEXT("Player-scoped slots are not treated as worlds"),
        FKalmalaLocalWorldSaveListing::ParseSlotName(TEXT("KalmalaPlayerDiscoveries_42_12345678"), Kind, Seed, bHasRevision, Revision));
    return true;
}
#endif
