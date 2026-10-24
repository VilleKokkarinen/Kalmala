#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationSchedulerCadenceTest,
    "Kalmala.Gameplay.Population.SchedulerCadence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationSchedulerCadenceTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population cadence fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population cadence fixture GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig.WorldSeed = 1;

    const FIntPoint FirstKey(0, 0);
    const FIntPoint SecondKey(1, 0);
    Mode->ActivePopulationSpatialKeys.Add(SecondKey);
    Mode->ActivePopulationSpatialKeys.Add(FirstKey);

    APlayerController* Controller = World->SpawnActor<APlayerController>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    FActorSpawnParameters PawnParameters;
    PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaCharacter* Character = World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
        FVector(3200.0, 0.0, 0.0), FRotator::ZeroRotator, PawnParameters);
    if (PlayerState != nullptr) PlayerState->SetPlayerId(1);
    if (Controller != nullptr && PlayerState != nullptr) Controller->SetPlayerState(PlayerState);
    if (Controller != nullptr && Character != nullptr)
    {
        Controller->Possess(Character);
        World->AddController(Controller);
    }
    if (!TestTrue(TEXT("Living player fixture is fully possessed"), IsValid(Controller)
        && IsValid(PlayerState) && IsValid(Character) && Character->GetController() == Controller))
    {
        return false;
    }

    const double StartTime = 100.0;
    Mode->UpdatePopulationKeySchedule(StartTime);
    TestTrue(TEXT("Only nearby materialized keys enter the active schedule"),
        Mode->ActivePopulationSchedulerKeys.Num() == 2
        && Mode->ActivePopulationSchedulerKeys.Contains(FirstKey)
        && Mode->ActivePopulationSchedulerKeys.Contains(SecondKey));

    const uint64 FirstPhase = FKalmalaWorldPopulationLayout::DeriveSpatialSeed(
        Mode->WorldGenerationConfig, FirstKey, EKalmalaWorldPopulationKind::Wildlife)
        % FKalmalaMonsterSpawnTuning::PopulationKeyPhaseCount;
    const uint64 SecondPhase = FKalmalaWorldPopulationLayout::DeriveSpatialSeed(
        Mode->WorldGenerationConfig, SecondKey, EKalmalaWorldPopulationKind::Wildlife)
        % FKalmalaMonsterSpawnTuning::PopulationKeyPhaseCount;
    TestNotEqual(TEXT("Fixture keys receive distinct stable phases"), FirstPhase, SecondPhase);
    const double FirstDue = StartTime + FirstPhase * FKalmalaMonsterSpawnTuning::PopulationKeyPhaseStepSeconds;
    const double SecondDue = StartTime + SecondPhase * FKalmalaMonsterSpawnTuning::PopulationKeyPhaseStepSeconds;
    TestTrue(TEXT("First due time uses its deterministic spatial-seed phase"),
        FMath::IsNearlyEqual(Mode->PopulationKeyNextDueTimes.FindRef(FirstKey), FirstDue));
    TestTrue(TEXT("Second due time uses its deterministic spatial-seed phase"),
        FMath::IsNearlyEqual(Mode->PopulationKeyNextDueTimes.FindRef(SecondKey), SecondDue));
    TestTrue(TEXT("No key is due before its assigned phase"),
        Mode->GetDuePopulationSpatialKeys(StartTime).IsEmpty());

    const FIntPoint EarlierKey = FirstDue < SecondDue ? FirstKey : SecondKey;
    const double EarlierDue = FMath::Min(FirstDue, SecondDue);
    const TArray<FIntPoint> FirstDueKeys = Mode->GetDuePopulationSpatialKeys(EarlierDue);
    TestTrue(TEXT("The earlier staggered key is the only due key"),
        FirstDueKeys.Num() == 1 && FirstDueKeys[0] == EarlierKey);
    TestTrue(TEXT("Servicing a due key advances it by exactly five seconds"),
        Mode->MarkPopulationSpatialKeyServiced(EarlierKey, EarlierDue)
        && FMath::IsNearlyEqual(Mode->PopulationKeyNextDueTimes.FindRef(EarlierKey),
            EarlierDue + FKalmalaMonsterSpawnTuning::PopulationKeyCheckIntervalSeconds));
    TestFalse(TEXT("The same key cannot be serviced twice for one due time"),
        Mode->MarkPopulationSpatialKeyServiced(EarlierKey, EarlierDue));

    const FIntPoint LaterKey = FirstDue < SecondDue ? SecondKey : FirstKey;
    const double LaterDue = FMath::Max(FirstDue, SecondDue);
    TestTrue(TEXT("The later staggered key becomes due independently"),
        Mode->GetDuePopulationSpatialKeys(LaterDue).Contains(LaterKey));
    TestTrue(TEXT("The later key advances after service"), Mode->MarkPopulationSpatialKeyServiced(LaterKey, LaterDue));

    const double DelayedServiceTime = 120.0;
    TArray<FIntPoint> DelayedDueKeys = Mode->GetDuePopulationSpatialKeys(DelayedServiceTime);
    TestTrue(TEXT("A long stall exposes each overdue active key once in stable order"),
        DelayedDueKeys.Num() == 2 && DelayedDueKeys[0] == FirstKey && DelayedDueKeys[1] == SecondKey);
    for (const FIntPoint& Key : DelayedDueKeys)
    {
        TestTrue(TEXT("Overdue service succeeds once"), Mode->MarkPopulationSpatialKeyServiced(Key, DelayedServiceTime));
    }
    TestTrue(TEXT("Missed intervals do not produce catch-up retries"),
        Mode->GetDuePopulationSpatialKeys(DelayedServiceTime).IsEmpty());

    const double InactiveTime = DelayedServiceTime + 1.0;
    Character->SetActorLocation(FVector(30000.0, 30000.0, 0.0));
    Mode->UpdatePopulationKeySchedule(InactiveTime);
    TestTrue(TEXT("Inactive keys are removed from the active due set"),
        Mode->ActivePopulationSchedulerKeys.IsEmpty());
    TestTrue(TEXT("Inactive scheduler due entries are removed"), Mode->PopulationKeyNextDueTimes.IsEmpty());

    const double ReactivationTime = DelayedServiceTime
        + FKalmalaMonsterSpawnTuning::PopulationKeyCheckIntervalSeconds + 1.0;
    Character->SetActorLocation(FVector(3200.0, 0.0, 0.0));
    Mode->UpdatePopulationKeySchedule(ReactivationTime);
    const double ResumedFirstDue = ReactivationTime
        + FirstPhase * FKalmalaMonsterSpawnTuning::PopulationKeyPhaseStepSeconds;
    const double ResumedSecondDue = ReactivationTime
        + SecondPhase * FKalmalaMonsterSpawnTuning::PopulationKeyPhaseStepSeconds;
    TestTrue(TEXT("Reactivation creates fresh due entries from deterministic phases"),
        FMath::IsNearlyEqual(Mode->PopulationKeyNextDueTimes.FindRef(FirstKey), ResumedFirstDue)
        && FMath::IsNearlyEqual(Mode->PopulationKeyNextDueTimes.FindRef(SecondKey), ResumedSecondDue));
    TestTrue(TEXT("A missed old interval does not trigger catch-up on reactivation"),
        Mode->GetDuePopulationSpatialKeys(ReactivationTime).IsEmpty());
    const FIntPoint ResumedEarlierKey = ResumedFirstDue < ResumedSecondDue ? FirstKey : SecondKey;
    const double ResumedEarlierDue = FMath::Min(ResumedFirstDue, ResumedSecondDue);
    const TArray<FIntPoint> ResumedDueKeys = Mode->GetDuePopulationSpatialKeys(ResumedEarlierDue);
    TestTrue(TEXT("Reactivated keys retain their stable staggered order"),
        ResumedDueKeys.Num() == 1 && ResumedDueKeys[0] == ResumedEarlierKey);
    return true;
}

#endif
