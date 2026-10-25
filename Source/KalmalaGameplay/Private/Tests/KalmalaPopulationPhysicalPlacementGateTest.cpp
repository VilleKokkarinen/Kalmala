#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationPhysicalPlacementGateTest,
    "Kalmala.Gameplay.Population.PhysicalPlacementGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationPhysicalPlacementGateTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Physical-placement fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Physical-placement GameMode created"), Mode)) return false;
    FKalmalaWorldPopulationSpawn Descriptor;
    Descriptor.Kind = EKalmalaWorldPopulationKind::Wildlife;
    Descriptor.SpatialKey = FIntPoint(1, -1);
    Descriptor.SpawnSeed = 12345;
    Descriptor.Location = FVector(std::numeric_limits<double>::quiet_NaN(), 500.0, 100.0);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* ExistingCreature = World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), FVector(900.0, 900.0, 100.0),
        FRotator::ZeroRotator, SpawnParameters);
    if (!TestNotNull(TEXT("Existing creature fixture created"), ExistingCreature)) return false;
    const FVector ExistingLocation = ExistingCreature->GetActorLocation();

    TestFalse(TEXT("Invalid descriptor location fails physical placement closed"),
        Mode->DoesPopulationDescriptorPassPhysicalPlacement(Descriptor));

    const auto PassingEarlierGates = []()
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
        return Results;
    };

    FKalmalaPopulationSchedulerKeyWork KeyWork;
    KeyWork.Candidates.Add(Descriptor);
    FKalmalaPopulationSpawnAttempt& Attempt = KeyWork.SpawnAttempts.AddDefaulted_GetRef();
    Attempt.Descriptor = Descriptor;
    Attempt.GateResults = PassingEarlierGates();
    Mode->ApplyPopulationPhysicalPlacementGate(KeyWork);
    TestFalse(TEXT("Invalid descriptor is rejected by the combined physical checks"),
        KeyWork.SpawnAttempts[0].GateResults.bPhysicalPlacementPassed);
    TestEqual(TEXT("The final evaluator reports physical placement as the first remaining failure"),
        KeyWork.SpawnAttempts[0].RejectionReason,
        EKalmalaPopulationSpawnRejectionReason::PhysicalPlacement);
    TestTrue(TEXT("A physically rejected descriptor is removed from the candidate list"),
        KeyWork.Candidates.IsEmpty());

    FKalmalaPopulationSchedulerKeyWork EarlierFailure;
    EarlierFailure.Candidates.Add(Descriptor);
    FKalmalaPopulationSpawnAttempt& CappedAttempt = EarlierFailure.SpawnAttempts.AddDefaulted_GetRef();
    CappedAttempt.Descriptor = Descriptor;
    CappedAttempt.GateResults = PassingEarlierGates();
    CappedAttempt.GateResults.bCapsPassed = false;
    Mode->ApplyPopulationPhysicalPlacementGate(EarlierFailure);
    TestEqual(TEXT("Physical placement preserves the earlier Caps rejection"),
        EarlierFailure.SpawnAttempts[0].RejectionReason,
        EKalmalaPopulationSpawnRejectionReason::Caps);
    TestTrue(TEXT("Caps-rejected descriptor is not retained"), EarlierFailure.Candidates.IsEmpty());

    TestTrue(TEXT("Placement rejection does not delete existing wildlife"), IsValid(ExistingCreature));
    TestTrue(TEXT("Placement rejection does not move existing wildlife"),
        ExistingCreature->GetActorLocation().Equals(ExistingLocation));
    return true;
}

#endif
