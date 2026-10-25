#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationSpawnGateEvaluatorTest,
    "Kalmala.Gameplay.Population.SpawnGateEvaluator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationSpawnGateEvaluatorTest::RunTest(const FString& Parameters)
{
    const auto AllGatesPass = []()
    {
        FKalmalaPopulationSpawnGateResults Results;
        Results.bSchedulerBudgetAvailable = true;
        Results.bSeededAdmissionRollPassed = true;
        Results.bPlayerDistancePassed = true;
        Results.bSpeciesHabitatPassed = true;
        Results.bTimePassed = true;
        Results.bWeatherPassed = true;
        Results.bProgressionPassed = true;
        Results.bCapsPassed = true;
        Results.bPhysicalPlacementPassed = true;
        return Results;
    };

    TestEqual(TEXT("Default unimplemented gates fail safely at the first gate"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(FKalmalaPopulationSpawnGateResults()),
        EKalmalaPopulationSpawnRejectionReason::SchedulerBudget);

    FKalmalaPopulationSpawnGateResults Results = AllGatesPass();
    TestEqual(TEXT("All nine passing gates produce no rejection"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::None);

    const EKalmalaPopulationSpawnRejectionReason OrderedReasons[] = {
        EKalmalaPopulationSpawnRejectionReason::SchedulerBudget,
        EKalmalaPopulationSpawnRejectionReason::SeededAdmissionRoll,
        EKalmalaPopulationSpawnRejectionReason::PlayerDistance,
        EKalmalaPopulationSpawnRejectionReason::SpeciesHabitat,
        EKalmalaPopulationSpawnRejectionReason::Time,
        EKalmalaPopulationSpawnRejectionReason::Weather,
        EKalmalaPopulationSpawnRejectionReason::Progression,
        EKalmalaPopulationSpawnRejectionReason::Caps,
        EKalmalaPopulationSpawnRejectionReason::PhysicalPlacement
    };
    const auto SetGatePassed = [](FKalmalaPopulationSpawnGateResults& GateResults,
        const int32 GateIndex)
    {
        switch (GateIndex)
        {
        case 0: GateResults.bSchedulerBudgetAvailable = true; break;
        case 1: GateResults.bSeededAdmissionRollPassed = true; break;
        case 2: GateResults.bPlayerDistancePassed = true; break;
        case 3: GateResults.bSpeciesHabitatPassed = true; break;
        case 4: GateResults.bTimePassed = true; break;
        case 5: GateResults.bWeatherPassed = true; break;
        case 6: GateResults.bProgressionPassed = true; break;
        case 7: GateResults.bCapsPassed = true; break;
        case 8: GateResults.bPhysicalPlacementPassed = true; break;
        default: break;
        }
    };
    for (int32 FailingGateIndex = 0; FailingGateIndex < UE_ARRAY_COUNT(OrderedReasons);
        ++FailingGateIndex)
    {
        Results = FKalmalaPopulationSpawnGateResults();
        for (int32 EarlierGateIndex = 0; EarlierGateIndex < FailingGateIndex; ++EarlierGateIndex)
        {
            SetGatePassed(Results, EarlierGateIndex);
        }
        TestEqual(FString::Printf(TEXT("Gate %d returns its reason before later false gates"),
                FailingGateIndex),
            AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
            OrderedReasons[FailingGateIndex]);
    }

    Results = AllGatesPass();
    Results.bSchedulerBudgetAvailable = false;
    Results.bPlayerDistancePassed = false;
    TestEqual(TEXT("Evaluator returns only the earliest failing gate"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::SchedulerBudget);

    const EKalmalaWildlifeArchetype CurrentSpecies[] = {
        EKalmalaWildlifeArchetype::Mireling,
        EKalmalaWildlifeArchetype::Boar,
        EKalmalaWildlifeArchetype::Deer,
        EKalmalaWildlifeArchetype::Rat,
        EKalmalaWildlifeArchetype::Badger,
        EKalmalaWildlifeArchetype::MoonBear,
        EKalmalaWildlifeArchetype::DeathAdder,
        EKalmalaWildlifeArchetype::Viper,
        EKalmalaWildlifeArchetype::GiantSnake,
        EKalmalaWildlifeArchetype::MosquitoSwarm,
        EKalmalaWildlifeArchetype::PeatHusk,
        EKalmalaWildlifeArchetype::Wolf,
        EKalmalaWildlifeArchetype::Lynx,
        EKalmalaWildlifeArchetype::PolarBear,
        EKalmalaWildlifeArchetype::HornedGoat,
        EKalmalaWildlifeArchetype::ThunderSentinel,
        EKalmalaWildlifeArchetype::ReefMaw
    };
    bool bEveryCurrentSpeciesPassesTime = true;
    for (const EKalmalaWildlifeArchetype Archetype : CurrentSpecies)
    {
        bEveryCurrentSpeciesPassesTime &= AKalmalaGameMode::IsPopulationTimeGateAllowed(Archetype);
    }
    TestTrue(TEXT("The time gate passes every current wildlife species"), bEveryCurrentSpeciesPassesTime);

    bool bEveryCurrentSpeciesPassesWeather = true;
    for (const EKalmalaWildlifeArchetype Archetype : CurrentSpecies)
    {
        bEveryCurrentSpeciesPassesWeather &= AKalmalaGameMode::IsPopulationWeatherGateAllowed(Archetype);
    }
    TestTrue(TEXT("The weather gate passes every current wildlife species"),
        bEveryCurrentSpeciesPassesWeather);

    bool bEveryCurrentSpeciesPassesProgression = true;
    for (const EKalmalaWildlifeArchetype Archetype : CurrentSpecies)
    {
        bEveryCurrentSpeciesPassesProgression &=
            AKalmalaGameMode::IsPopulationProgressionGateAllowed(Archetype);
    }
    TestTrue(TEXT("The progression gate passes every current wildlife species"),
        bEveryCurrentSpeciesPassesProgression);

    const EKalmalaWildlifeArchetype UnknownArchetype = static_cast<EKalmalaWildlifeArchetype>(255);
    TestFalse(TEXT("An unknown archetype fails the time gate closed"),
        AKalmalaGameMode::IsPopulationTimeGateAllowed(UnknownArchetype));
    TestFalse(TEXT("An unknown archetype fails the weather gate closed"),
        AKalmalaGameMode::IsPopulationWeatherGateAllowed(UnknownArchetype));
    TestFalse(TEXT("An unknown archetype fails the progression gate closed"),
        AKalmalaGameMode::IsPopulationProgressionGateAllowed(UnknownArchetype));
    TestFalse(TEXT("The fixture seam can force a known species time rejection"),
        AKalmalaGameMode::IsPopulationTimeGateAllowedForTests(EKalmalaWildlifeArchetype::Deer, true));
    TestFalse(TEXT("The fixture seam cannot allow an unknown archetype"),
        AKalmalaGameMode::IsPopulationTimeGateAllowedForTests(UnknownArchetype, true));
    TestFalse(TEXT("The fixture seam can force a known species weather rejection"),
        AKalmalaGameMode::IsPopulationWeatherGateAllowedForTests(EKalmalaWildlifeArchetype::Deer, true));
    TestFalse(TEXT("The weather fixture seam cannot allow an unknown archetype"),
        AKalmalaGameMode::IsPopulationWeatherGateAllowedForTests(UnknownArchetype, true));
    TestFalse(TEXT("The fixture seam can force a known species progression rejection"),
        AKalmalaGameMode::IsPopulationProgressionGateAllowedForTests(
            EKalmalaWildlifeArchetype::Deer, true));
    TestFalse(TEXT("The progression fixture seam cannot allow an unknown archetype"),
        AKalmalaGameMode::IsPopulationProgressionGateAllowedForTests(UnknownArchetype, true));
    Results = AllGatesPass();
    Results.bTimePassed = AKalmalaGameMode::IsPopulationTimeGateAllowedForTests(
        EKalmalaWildlifeArchetype::Deer, true);
    TestEqual(TEXT("A forced time-policy rejection reaches the ordered evaluator"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Time);
    Results = AllGatesPass();
    Results.bWeatherPassed = AKalmalaGameMode::IsPopulationWeatherGateAllowedForTests(
        EKalmalaWildlifeArchetype::Deer, true);
    TestEqual(TEXT("A forced weather-policy rejection reaches the ordered evaluator"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Weather);
    Results.bTimePassed = false;
    TestEqual(TEXT("An earlier time rejection takes precedence over weather"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Time);
    Results = AllGatesPass();
    Results.bProgressionPassed = AKalmalaGameMode::IsPopulationProgressionGateAllowedForTests(
        EKalmalaWildlifeArchetype::Deer, true);
    TestEqual(TEXT("A forced progression-policy rejection reaches the ordered evaluator"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Progression);
    Results.bWeatherPassed = false;
    TestEqual(TEXT("An earlier weather rejection takes precedence over progression"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Weather);
    return true;
}

#endif
