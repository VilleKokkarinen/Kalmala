#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWolfPackContract.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationRoofClearanceTest,
    "Kalmala.Gameplay.Population.RoofClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationRoofClearanceTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population roof-clearance fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population roof-clearance GameMode created"), Mode)) return false;

    constexpr double TraceDistanceCm =
        FKalmalaMonsterSpawnTuning::PopulationRoofClearanceTraceDistanceCm;
    const double RoofHalfHeightCm = AKalmalaConstructionActor::GetCollisionExtent(TEXT("RoofKit")).Z;
    TestTrue(TEXT("The actual RoofKit collision has a valid vertical extent"), RoofHalfHeightCm > 0.0);
    const auto TraceAt = [World](const double X, const AActor* ActorToIgnore)
    {
        const FVector Start(X, 0.0, 0.0);
        const FVector End = Start + FVector::UpVector
            * FKalmalaMonsterSpawnTuning::PopulationRoofClearanceTraceDistanceCm;
        return AKalmalaGameMode::DoesSolidConstructionRoofIntersectSegment(
            World, Start, End, ActorToIgnore);
    };
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

    TestFalse(TEXT("No construction roof leaves the upward path clear"), TraceAt(0.0, nullptr));

    AKalmalaConstructionActor* BoundaryRoof = SpawnConstruction(
        TEXT("RoofKit"), TEXT("roof-boundary"), FVector(1000.0, 0.0, TraceDistanceCm + RoofHalfHeightCm));
    if (!TestNotNull(TEXT("Boundary RoofKit actor created"), BoundaryRoof)) return false;
    TestTrue(TEXT("A solid RoofKit beginning exactly at the 24 m endpoint rejects"), TraceAt(1000.0, nullptr));

    AKalmalaConstructionActor* BeyondRoof = SpawnConstruction(
        TEXT("RoofKit"), TEXT("roof-beyond"), FVector(1500.0, 0.0, TraceDistanceCm + RoofHalfHeightCm + 4.0));
    if (!TestNotNull(TEXT("Beyond-boundary RoofKit actor created"), BeyondRoof)) return false;
    TestFalse(TEXT("A roof wholly above the 24 m trace does not intersect"), TraceAt(1500.0, nullptr));

    AKalmalaConstructionActor* Wall = SpawnConstruction(
        TEXT("WallKit"), TEXT("wall-only"), FVector(2000.0, 0.0, 1200.0));
    if (!TestNotNull(TEXT("Non-roof construction actor created"), Wall)) return false;
    Wall->Tags.AddUnique(TEXT("KalmalaShelterRoof"));
    TestFalse(TEXT("A solid non-roof construction piece does not reject"), TraceAt(2000.0, nullptr));

    AActor* FoliageBlocker = World->SpawnActor<AActor>(FVector(2500.0, 0.0, 1200.0),
        FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Foliage/canopy blocker actor created"), FoliageBlocker)) return false;
    FoliageBlocker->Tags.AddUnique(TEXT("KalmalaShelterRoof"));
    UBoxComponent* FoliageCollision = NewObject<UBoxComponent>(FoliageBlocker, TEXT("FoliageBlocker"));
    if (!TestNotNull(TEXT("Foliage/canopy blocker component created"), FoliageCollision)) return false;
    FoliageBlocker->SetRootComponent(FoliageCollision);
    FoliageCollision->SetBoxExtent(FVector(30.0, 30.0, 30.0));
    FoliageCollision->SetCollisionProfileName(TEXT("BlockAll"));
    FoliageCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    FoliageCollision->RegisterComponent();
    TestFalse(TEXT("Foliage or canopy without RoofKit identity is ignored"), TraceAt(2500.0, nullptr));

    AKalmalaConstructionActor* RoofAboveFoliage = SpawnConstruction(
        TEXT("RoofKit"), TEXT("roof-above-foliage"),
        FVector(2500.0, 0.0, TraceDistanceCm + RoofHalfHeightCm));
    if (!TestNotNull(TEXT("RoofKit above foliage created"), RoofAboveFoliage)) return false;
    TestTrue(TEXT("An intervening foliage hit cannot mask RoofKit above it"), TraceAt(2500.0, nullptr));

    AActor* Cosmetic = World->SpawnActor<AActor>(FVector(3000.0, 0.0, 1200.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Cosmetic fixture actor created"), Cosmetic)) return false;
    UBoxComponent* CosmeticCollision = NewObject<UBoxComponent>(Cosmetic, TEXT("NonSolidCosmetic"));
    if (!TestNotNull(TEXT("Cosmetic fixture component created"), CosmeticCollision)) return false;
    Cosmetic->SetRootComponent(CosmeticCollision);
    CosmeticCollision->SetBoxExtent(FVector(30.0, 30.0, 30.0));
    CosmeticCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CosmeticCollision->RegisterComponent();
    TestFalse(TEXT("A non-solid cosmetic is ignored"), TraceAt(3000.0, nullptr));

    AKalmalaConstructionActor* UninitializedConstruction = SpawnConstruction(
        NAME_None, TEXT("uninitialized"), FVector(3500.0, 0.0, 1200.0));
    if (!TestNotNull(TEXT("Uninitialized construction actor created"), UninitializedConstruction)) return false;
    TestFalse(TEXT("Construction without an accepted kit state is ignored"), TraceAt(3500.0, nullptr));

    AKalmalaConstructionActor* IgnoredRoof = SpawnConstruction(
        TEXT("RoofKit"), TEXT("roof-ignored"), FVector(4000.0, 0.0, TraceDistanceCm + RoofHalfHeightCm));
    if (!TestNotNull(TEXT("Ignored RoofKit actor created"), IgnoredRoof)) return false;
    TestFalse(TEXT("The proposed actor can be ignored explicitly"), TraceAt(4000.0, IgnoredRoof));

    AKalmalaConstructionActor* DisabledRoof = SpawnConstruction(
        TEXT("RoofKit"), TEXT("roof-disabled"), FVector(4500.0, 0.0, 1200.0));
    if (!TestNotNull(TEXT("Disabled RoofKit actor created"), DisabledRoof)) return false;
    UBoxComponent* DisabledRoofCollision = Cast<UBoxComponent>(DisabledRoof->GetRootComponent());
    if (!TestNotNull(TEXT("Disabled RoofKit root box exists"), DisabledRoofCollision)) return false;
    DisabledRoofCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestFalse(TEXT("A RoofKit without active query collision is not a solid roof"), TraceAt(4500.0, nullptr));

    FVector InvalidStart(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0);
    TestTrue(TEXT("A nonfinite trace fails closed"),
        AKalmalaGameMode::DoesSolidConstructionRoofIntersectSegment(
            World, InvalidStart, FVector(0.0, 0.0, TraceDistanceCm)));
    TestTrue(TEXT("A descending trace fails closed"),
        AKalmalaGameMode::DoesSolidConstructionRoofIntersectSegment(
            World, FVector::ZeroVector, FVector(0.0, 0.0, -1.0)));
    TestTrue(TEXT("A trace longer than the fixed 24 m contract fails closed"),
        AKalmalaGameMode::DoesSolidConstructionRoofIntersectSegment(
            World, FVector::ZeroVector, FVector::UpVector * (TraceDistanceCm + 1.0)));
    TestTrue(TEXT("A null world fails closed"),
        AKalmalaGameMode::DoesSolidConstructionRoofIntersectSegment(
            nullptr, FVector::ZeroVector, FVector::UpVector * TraceDistanceCm));

    TArray<FKalmalaWildlifeBodyCollisionShape> SnakeShapes;
    TestTrue(TEXT("Giant Snake exposes its actual S22a profile"),
        AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            EKalmalaWildlifeArchetype::GiantSnake, SnakeShapes));
    FVector SnakeBodyTop;
    TestTrue(TEXT("Giant Snake body top uses its complete active profile"),
        AKalmalaGameMode::TryGetPopulationBodyTop(
            SnakeShapes, FVector(5000.0, 0.0, 100.0), FRotator::ZeroRotator, SnakeBodyTop));
    double ExpectedSnakeTop = -TNumericLimits<double>::Max();
    for (const FKalmalaWildlifeBodyCollisionShape& Shape : SnakeShapes)
    {
        double ShapeTop = Shape.RadiusCentimetres;
        if (Shape.Type == EKalmalaWildlifeBodyShapeType::Capsule)
        {
            const FVector Axis = Shape.RelativeRotation.Quaternion().RotateVector(FVector::UpVector);
            ShapeTop = Shape.RelativeLocation.Z + Shape.RadiusCentimetres
                + (Shape.HalfHeightCentimetres - Shape.RadiusCentimetres) * FMath::Abs(Axis.Z);
        }
        ExpectedSnakeTop = FMath::Max(ExpectedSnakeTop, 100.0 + ShapeTop);
    }
    TestTrue(TEXT("Snake roof origin is above every active profile component"),
        FMath::IsNearlyEqual(SnakeBodyTop.Z, ExpectedSnakeTop));

    TArray<FKalmalaWildlifeBodyCollisionShape> WolfShapes;
    TestTrue(TEXT("Wolf exposes its actual S22a profile"),
        AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            EKalmalaWildlifeArchetype::Wolf, WolfShapes));
    for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
    {
        FVector WolfBodyTop;
        const FVector MemberLocation(6000.0 + MemberIndex * 500.0, 0.0, 200.0);
        TestTrue(TEXT("Each Wolf member profile has a valid body top"),
            AKalmalaGameMode::TryGetPopulationBodyTop(
                WolfShapes, MemberLocation, FRotator::ZeroRotator, WolfBodyTop));
        TestTrue(TEXT("Each Wolf trace starts at its own top Z"),
            WolfBodyTop.Z > MemberLocation.Z && FMath::IsNearlyEqual(WolfBodyTop.X, MemberLocation.X));
    }

    FKalmalaWorldPopulationSpawn InvalidDescriptor;
    TestFalse(TEXT("The descriptor roof predicate rejects invalid/unconfigured inputs"),
        Mode->DoesPopulationDescriptorPassRoofClearance(InvalidDescriptor));
    FKalmalaPopulationSpawnGateResults GateResults;
    GateResults.bSchedulerBudgetAvailable = true;
    GateResults.bSeededAdmissionRollPassed = true;
    GateResults.bPlayerDistancePassed = true;
    GateResults.bSpeciesHabitatPassed = true;
    GateResults.bTimePassed = true;
    GateResults.bWeatherPassed = true;
    GateResults.bProgressionPassed = true;
    GateResults.bCapsPassed = true;
    TestEqual(TEXT("S22b does not wire the final physical-placement gate"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(GateResults),
        EKalmalaPopulationSpawnRejectionReason::PhysicalPlacement);
    return true;
}

#endif
