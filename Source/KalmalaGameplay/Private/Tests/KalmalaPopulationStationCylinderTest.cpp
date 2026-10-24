#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationStationCylinderTest,
    "Kalmala.Gameplay.Population.StationCylinder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationStationCylinderTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population station-cylinder fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population station-cylinder GameMode created"), Mode)) return false;

    const auto SpawnConstruction = [World](const FName Kit, const FString& Id,
        const FVector Location) -> AKalmalaConstructionActor*
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaConstructionActor* Construction = World->SpawnActor<AKalmalaConstructionActor>(
            Location, FRotator::ZeroRotator, SpawnParameters);
        if (IsValid(Construction) && !Kit.IsNone()) Construction->InitializeFromServer(Kit, Id);
        return Construction;
    };

    TestFalse(TEXT("No station does not suppress a finite point"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector::ZeroVector));

    AKalmalaConstructionActor* Workbench = SpawnConstruction(
        TEXT("WorkbenchKit"), TEXT("station-cylinder-workbench"), FVector::ZeroVector);
    if (!TestNotNull(TEXT("Placed Workbench created"), Workbench)) return false;
    const double Radius = FKalmalaMonsterSpawnTuning::CampSuppressionRadiusCm;
    const double HalfHeight = FKalmalaMonsterSpawnTuning::PopulationStationCylinderHalfHeightCm;
    TestTrue(TEXT("Inclusive horizontal and positive vertical boundaries suppress"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(Radius, 0.0, HalfHeight)));
    TestTrue(TEXT("Inclusive negative vertical boundary suppresses"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(0.0, 0.0, -HalfHeight)));
    TestFalse(TEXT("A point beyond the horizontal radius is outside"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(Radius + 1.0, 0.0, 0.0)));
    TestFalse(TEXT("A point beyond the vertical half-height is outside"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(0.0, 0.0, HalfHeight + 1.0)));

    AKalmalaConstructionActor* Forge = SpawnConstruction(
        TEXT("ForgeKit"), TEXT("station-cylinder-forge"), FVector(5000.0, 0.0, 0.0));
    if (!TestNotNull(TEXT("Placed Forge created"), Forge)) return false;
    TestTrue(TEXT("Forge uses the same horizontal station cylinder"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(5000.0 + Radius, 0.0, 0.0)));

    AKalmalaConstructionActor* WorkbenchRack = SpawnConstruction(
        TEXT("WorkbenchToolRackKit"), TEXT("station-cylinder-rack"), FVector(10000.0, 0.0, 0.0));
    if (!TestNotNull(TEXT("Workbench Tool Rack created"), WorkbenchRack)) return false;
    TestFalse(TEXT("Workbench attachments are not stations"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(10000.0, 0.0, 0.0)));

    AKalmalaConstructionActor* Uninitialized = SpawnConstruction(
        NAME_None, TEXT("station-cylinder-uninitialized"), FVector(15000.0, 0.0, 0.0));
    if (!TestNotNull(TEXT("Uninitialized construction actor created"), Uninitialized)) return false;
    TestFalse(TEXT("Uninitialized construction is not a placed station"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(15000.0, 0.0, 0.0)));

    AKalmalaConstructionActor* Destroyed = SpawnConstruction(
        TEXT("ForgeKit"), TEXT("station-cylinder-destroyed"), FVector(20000.0, 0.0, 0.0));
    if (!TestNotNull(TEXT("Destroyable Forge created"), Destroyed)) return false;
    Destroyed->Destroy();
    TestFalse(TEXT("Destroyed construction no longer suppresses"),
        Mode->DoesPopulationPointOverlapStationCylinder(FVector(20000.0, 0.0, 0.0)));

    const FVector Invalid(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0);
    TestTrue(TEXT("Invalid candidate input fails closed"),
        Mode->DoesPopulationPointOverlapStationCylinder(Invalid));
    return true;
}

#endif
