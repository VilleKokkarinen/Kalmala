#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWorldBounds.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationLandSurfaceTest,
    "Kalmala.Gameplay.Population.LandSurface",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationLandSurfaceTest::RunTest(const FString& Parameters)
{
    const double MaximumSlopeRadians = FMath::DegreesToRadians(
        static_cast<double>(FKalmalaMonsterSpawnTuning::MaximumLandSlopeDegrees));
    const FVector AtSlopeLimit(
        FMath::Sin(MaximumSlopeRadians), 0.0, FMath::Cos(MaximumSlopeRadians));
    const FVector OverSlopeLimit(
        FMath::Sin(FMath::DegreesToRadians(32.1)), 0.0,
        FMath::Cos(FMath::DegreesToRadians(32.1)));

    TestTrue(TEXT("Dry generated ground at the inclusive 32 degree slope limit passes"),
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(
            true, true, true, AtSlopeLimit));
    TestFalse(TEXT("A slope above 32 degrees fails"),
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(
            true, true, true, OverSlopeLimit));
    TestFalse(TEXT("Ocean or lake water fails the dry-ground requirement"),
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(
            true, true, false, FVector::UpVector));
    TestFalse(TEXT("A surface outside world bounds fails"),
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(
            false, true, true, FVector::UpVector));
    TestFalse(TEXT("A missing or invalid generated-ground sample fails"),
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(
            true, false, true, FVector::UpVector));

    const TArray<bool> AllWolfMemberSurfacesAllowed = {
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(true, true, true, FVector::UpVector),
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(true, true, true, AtSlopeLimit),
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(true, true, true, FVector::UpVector)
    };
    TestTrue(TEXT("A Wolf pack passes only after the center and all three member surfaces pass"),
        AKalmalaGameMode::AreAllPopulationLandSurfacesAllowed(AllWolfMemberSurfacesAllowed));
    const TArray<bool> OneWolfMemberTooSteep = {
        AllWolfMemberSurfacesAllowed[0], AllWolfMemberSurfacesAllowed[1],
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(true, true, true, OverSlopeLimit),
        AllWolfMemberSurfacesAllowed[2]
    };
    TestFalse(TEXT("One Wolf member above 32 degrees rejects the whole pack's land check"),
        AKalmalaGameMode::AreAllPopulationLandSurfacesAllowed(OneWolfMemberTooSteep));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population land-surface fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population land-surface fixture GameMode created"), Mode)) return false;

    FKalmalaWorldPopulationSpawn OutsideWorld;
    OutsideWorld.Location = FVector(FKalmalaWorldBounds::Radius + 1.0, 0.0, 0.0);
    TestFalse(TEXT("The server predicate rejects descriptors outside generated world bounds"),
        Mode->DoesPopulationDescriptorPassLandSurface(OutsideWorld));

    FKalmalaPopulationSpawnGateResults PlacementResults;
    PlacementResults.bSchedulerBudgetAvailable = true;
    PlacementResults.bSeededAdmissionRollPassed = true;
    PlacementResults.bPlayerDistancePassed = true;
    PlacementResults.bSpeciesHabitatPassed = true;
    PlacementResults.bTimePassed = true;
    PlacementResults.bWeatherPassed = true;
    PlacementResults.bProgressionPassed = true;
    PlacementResults.bCapsPassed = true;
    TestEqual(TEXT("S20 leaves final placement fail-closed until S25 wires all checks"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(PlacementResults),
        EKalmalaPopulationSpawnRejectionReason::PhysicalPlacement);

    return true;
}

#endif
