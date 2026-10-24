#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationSchedulerBudgetTest,
    "Kalmala.Gameplay.Population.SchedulerWakeBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationSchedulerBudgetTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population scheduler fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population scheduler fixture GameMode created"), Mode)) return false;

    for (int32 Index = 0; Index < 6; ++Index)
    {
        const FIntPoint Key(Index, 0);
        Mode->ActivePopulationSchedulerKeys.Add(Key);
        Mode->PopulationKeyNextDueTimes.Add(Key, 1.0);
    }
    Mode->NextPopulationSchedulerWakeTime = 0.5;

    Mode->UpdatePopulationSchedulerWake(1.0);
    TestTrue(TEXT("One wake selects no more than four due keys"),
        Mode->PopulationSchedulerWakeWork.Num() == FKalmalaMonsterSpawnTuning::MaxPopulationKeysPerWake);
    int32 TotalCandidateBudget = 0;
    for (int32 Index = 0; Index < Mode->PopulationSchedulerWakeWork.Num(); ++Index)
    {
        const FKalmalaPopulationSchedulerKeyWork& Work = Mode->PopulationSchedulerWakeWork[Index];
        TestTrue(TEXT("Each key receives no more than four candidate slots"),
            Work.CandidateBudget <= FKalmalaMonsterSpawnTuning::MaxPopulationCandidatesPerKey);
        TestTrue(TEXT("Sorted due keys are serviced first"), Work.SpatialKey == FIntPoint(Index, 0));
        TotalCandidateBudget += Work.CandidateBudget;
    }
    TestTrue(TEXT("Total candidate budget is capped at sixteen per wake"),
        TotalCandidateBudget <= FKalmalaMonsterSpawnTuning::MaxPopulationCandidatesPerWake);
    TestTrue(TEXT("Four selected keys reserve exactly sixteen candidate slots"), TotalCandidateBudget == 16);
    TestTrue(TEXT("Serviced keys advance while remaining due keys wait for another wake"),
        Mode->PopulationKeyNextDueTimes.FindRef(FIntPoint(0, 0)) == 6.0
        && Mode->PopulationKeyNextDueTimes.FindRef(FIntPoint(4, 0)) == 1.0);
    TestTrue(TEXT("Next wake is scheduled a quarter second after service"),
        FMath::IsNearlyEqual(Mode->NextPopulationSchedulerWakeTime, 1.25));

    Mode->UpdatePopulationSchedulerWake(1.0);
    TestTrue(TEXT("Repeated calls before the next wake do no work"),
        Mode->PopulationSchedulerWakeWork.IsEmpty()
        && FMath::IsNearlyEqual(Mode->NextPopulationSchedulerWakeTime, 1.25));

    Mode->UpdatePopulationSchedulerWake(5.0);
    TestTrue(TEXT("A missed series of wakes produces only one bounded wake"),
        Mode->PopulationSchedulerWakeWork.Num() == 2
        && Mode->PopulationSchedulerWakeWork[0].SpatialKey == FIntPoint(4, 0)
        && Mode->PopulationSchedulerWakeWork[1].SpatialKey == FIntPoint(5, 0)
        && FMath::IsNearlyEqual(Mode->NextPopulationSchedulerWakeTime, 5.25));
    const double NextWakeAfterHitch = Mode->NextPopulationSchedulerWakeTime;
    Mode->UpdatePopulationSchedulerWake(5.0);
    TestTrue(TEXT("A hitch does not trigger immediate catch-up wakes"),
        Mode->PopulationSchedulerWakeWork.IsEmpty()
        && FMath::IsNearlyEqual(Mode->NextPopulationSchedulerWakeTime, NextWakeAfterHitch));
    return true;
}

#endif
