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

    Results = AllGatesPass();
    Results.bSchedulerBudgetAvailable = false;
    TestEqual(TEXT("Scheduler budget has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::SchedulerBudget);
    Results = AllGatesPass();
    Results.bSeededAdmissionRollPassed = false;
    TestEqual(TEXT("Seeded admission has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::SeededAdmissionRoll);
    Results = AllGatesPass();
    Results.bPlayerDistancePassed = false;
    TestEqual(TEXT("Player distance has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::PlayerDistance);
    Results = AllGatesPass();
    Results.bSpeciesHabitatPassed = false;
    TestEqual(TEXT("Species habitat has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::SpeciesHabitat);
    Results = AllGatesPass();
    Results.bTimePassed = false;
    TestEqual(TEXT("Time has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Time);
    Results = AllGatesPass();
    Results.bWeatherPassed = false;
    TestEqual(TEXT("Weather has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Weather);
    Results = AllGatesPass();
    Results.bProgressionPassed = false;
    TestEqual(TEXT("Progression has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Progression);
    Results = AllGatesPass();
    Results.bCapsPassed = false;
    TestEqual(TEXT("Caps have their own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::Caps);
    Results = AllGatesPass();
    Results.bPhysicalPlacementPassed = false;
    TestEqual(TEXT("Physical placement has its own rejection reason"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(Results),
        EKalmalaPopulationSpawnRejectionReason::PhysicalPlacement);

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

    const EKalmalaWildlifeArchetype UnknownArchetype = static_cast<EKalmalaWildlifeArchetype>(255);
    TestFalse(TEXT("An unknown archetype fails the time gate closed"),
        AKalmalaGameMode::IsPopulationTimeGateAllowed(UnknownArchetype));
    TestFalse(TEXT("An unknown archetype fails the weather gate closed"),
        AKalmalaGameMode::IsPopulationWeatherGateAllowed(UnknownArchetype));
    TestFalse(TEXT("The fixture seam can force a known species time rejection"),
        AKalmalaGameMode::IsPopulationTimeGateAllowedForTests(EKalmalaWildlifeArchetype::Deer, true));
    TestFalse(TEXT("The fixture seam cannot allow an unknown archetype"),
        AKalmalaGameMode::IsPopulationTimeGateAllowedForTests(UnknownArchetype, true));
    TestFalse(TEXT("The fixture seam can force a known species weather rejection"),
        AKalmalaGameMode::IsPopulationWeatherGateAllowedForTests(EKalmalaWildlifeArchetype::Deer, true));
    TestFalse(TEXT("The weather fixture seam cannot allow an unknown archetype"),
        AKalmalaGameMode::IsPopulationWeatherGateAllowedForTests(UnknownArchetype, true));
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
    return true;
}

#endif
