#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaEnvironmentalSpawnerPolicy.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaEnvironmentalSpawnerPolicyTest,
    "Kalmala.Gameplay.Population.EnvironmentalSpawnerPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaEnvironmentalSpawnerPolicyTest::RunTest(const FString& Parameters)
{
    using Policy = FKalmalaEnvironmentalSpawnerPolicy;
    const FString SourceId = TEXT("configured-source-a");

    TestEqual(TEXT("A source has exactly two finite slots"), Policy::SourceSlotCount, 2);
    TestTrue(TEXT("Both finite source slot indices are valid"),
        Policy::IsSourceSlotIndexValid(0) && Policy::IsSourceSlotIndexValid(1));
    TestFalse(TEXT("No third source slot can be configured"),
        Policy::IsSourceSlotIndexValid(2));
    TestEqual(TEXT("Local checks use the fixed eight-second interval"), Policy::CheckIntervalSeconds, 8.0);
    TestEqual(TEXT("Local living cap is two"), Policy::MaximumLivingCreaturesPerSource, 2);
    TestEqual(TEXT("The local living radius is 24 m"), Policy::LocalLivingRadiusCentimetres, 2400.0);
    TestEqual(TEXT("A check examines at most four candidates"), Policy::MaximumCandidatesPerCheck, 4);
    TestEqual(TEXT("Fixed candidates stay within 12 m"), Policy::CandidateRadiusCentimetres, 1200.0);
    TestTrue(TEXT("The inclusive 12 m candidate boundary passes"),
        Policy::IsCandidateWithinSourceRadius(FVector::ZeroVector, FVector(0.0, 0.0, 1200.0)));
    TestFalse(TEXT("A candidate outside 12 m is rejected"),
        Policy::IsCandidateWithinSourceRadius(FVector::ZeroVector, FVector(1201.0, 0.0, 0.0)));
    TestFalse(TEXT("A candidate beyond 12 m in three dimensions is rejected"),
        Policy::IsCandidateWithinSourceRadius(FVector::ZeroVector, FVector(0.0, 0.0, 1201.0)));
    TestEqual(TEXT("A check creates at most one creature"), Policy::MaximumCreationsPerCheck, 1);

    TestFalse(TEXT("An unconfigured source is disabled even with an ID"),
        Policy::IsCheckDue(true, false, SourceId, 0, 8.0, 8.0));
    TestFalse(TEXT("No source can use a slot outside the two finite indices"),
        Policy::IsCheckDue(true, true, SourceId, 2, 8.0, 8.0));
    TestFalse(TEXT("An empty source identity is never configured"),
        Policy::IsCheckDue(true, true, TEXT("  "), 0, 8.0, 8.0));
    TestFalse(TEXT("Clients cannot schedule environmental checks"),
        Policy::IsCheckDue(false, true, SourceId, 0, 8.0, 8.0));
    TestFalse(TEXT("A source check is not due before its timestamp"),
        Policy::IsCheckDue(true, true, SourceId, 0, 7.999, 8.0));
    TestTrue(TEXT("A source check is due at its timestamp"),
        Policy::IsCheckDue(true, true, SourceId, 0, 8.0, 8.0));

    TestFalse(TEXT("An unconfigured source cannot admit a creature"),
        Policy::CanAdmitLivingCreature(true, false, SourceId, 0, 0));
    TestFalse(TEXT("An unconfigured source cannot create a creature"),
        Policy::CanCreateMoreThisCheck(true, false, SourceId, 0, 0));

    double NextDueSeconds = 0.0;
    TestTrue(TEXT("A configured source can service one overdue check"),
        Policy::IsCheckDue(true, true, SourceId, 0, 25.0, 8.0));
    TestTrue(TEXT("The next due time is based on actual service time after a hitch"),
        Policy::TryGetNextDueTimeAfterService(25.0, NextDueSeconds));
    TestEqual(TEXT("Missed intervals are skipped instead of replayed"), NextDueSeconds, 33.0);
    TestFalse(TEXT("The serviced source cannot catch up again immediately"),
        Policy::IsCheckDue(true, true, SourceId, 0, 25.0, NextDueSeconds));
    TestFalse(TEXT("A non-finite service time cannot schedule a check"),
        Policy::TryGetNextDueTimeAfterService(std::numeric_limits<double>::quiet_NaN(), NextDueSeconds));
    TestFalse(TEXT("Failed due-time calculation cannot look immediately due"),
        Policy::IsCheckDue(true, true, SourceId, 0, 25.0, NextDueSeconds));

    TestEqual(TEXT("Candidate work respects the four-candidate source budget"),
        Policy::GetCandidateBudgetForCheck(8), 4);
    TestEqual(TEXT("Candidate work also respects remaining shared budget"),
        Policy::GetCandidateBudgetForCheck(3), 3);
    TestEqual(TEXT("An exhausted shared budget yields no candidates"),
        Policy::GetCandidateBudgetForCheck(0), 0);
    TestTrue(TEXT("A configured source below its local cap may admit one"),
        Policy::CanAdmitLivingCreature(true, true, SourceId, 0, 1));
    TestFalse(TEXT("The local cap rejects a third living creature"),
        Policy::CanAdmitLivingCreature(true, true, SourceId, 0, 2));
    TestTrue(TEXT("Only one creation is allowed in a check"),
        Policy::CanCreateMoreThisCheck(true, true, SourceId, 0, 0));
    TestFalse(TEXT("A second creation in a check is rejected"),
        Policy::CanCreateMoreThisCheck(true, true, SourceId, 0, 1));

    const TArray<FKalmalaEnvironmentalSpawnerCreatureSample> Samples = {
        { SourceId, FVector::ZeroVector, true },
        { SourceId, FVector(2400.0, 0.0, 9000.0), true },
        { SourceId, FVector(2401.0, 0.0, 0.0), true },
        { TEXT("other-source"), FVector::ZeroVector, true },
        { SourceId, FVector::ZeroVector, false }
    };
    int32 LivingCount = -1;
    TestTrue(TEXT("The server can count finite same-source living samples"),
        Policy::TryCountLivingOwnedWithinRadius(true, true, SourceId, 0, FVector::ZeroVector,
            Samples, LivingCount));
    TestEqual(TEXT("The inclusive 24 m boundary counts, while dead, foreign and outside samples do not"),
        LivingCount, 2);

    TArray<FKalmalaEnvironmentalSpawnerCreatureSample> InvalidSamples = {
        { SourceId, FVector(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0), true }
    };
    const TArray<FKalmalaEnvironmentalSpawnerCreatureSample> InvalidHeightSamples = {
        { SourceId, FVector(0.0, 0.0, std::numeric_limits<double>::quiet_NaN()), true }
    };
    LivingCount = -1;
    TestFalse(TEXT("Invalid owned living positions fail closed"),
        Policy::TryCountLivingOwnedWithinRadius(true, true, SourceId, 0, FVector::ZeroVector,
            InvalidSamples, LivingCount));
    TestEqual(TEXT("Failed local counts cannot expose an admission count"), LivingCount, -1);

    LivingCount = -1;
    TestFalse(TEXT("Invalid owned living height also fails closed"),
        Policy::TryCountLivingOwnedWithinRadius(true, true, SourceId, 0, FVector::ZeroVector,
            InvalidHeightSamples, LivingCount));
    TestEqual(TEXT("Invalid heights cannot expose an admission count"), LivingCount, -1);

    return true;
}

#endif
