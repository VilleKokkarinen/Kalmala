#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldBounds.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationBodyClearanceTest,
    "Kalmala.Gameplay.Population.BodyClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationBodyClearanceTest::RunTest(const FString& Parameters)
{
    const EKalmalaWildlifeArchetype CurrentArchetypes[] = {
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
    for (const EKalmalaWildlifeArchetype Archetype : CurrentArchetypes)
    {
        TArray<FKalmalaWildlifeBodyCollisionShape> BodyShapes;
        const bool bHasProfile = AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            Archetype, BodyShapes);
        TestTrue(TEXT("Every current archetype has an explicit body collision profile"), bHasProfile);
        if (!bHasProfile || BodyShapes.IsEmpty()) continue;
        TestTrue(TEXT("Every current archetype has its active root sphere profile"),
            BodyShapes[0].Type == EKalmalaWildlifeBodyShapeType::Sphere
                && FMath::IsFinite(BodyShapes[0].RadiusCentimetres)
                && BodyShapes[0].RadiusCentimetres > 0.0f);
    }
    TArray<FKalmalaWildlifeBodyCollisionShape> UnsupportedShapes;
    TestFalse(TEXT("Unknown archetypes have no guessed fallback body profile"),
        AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            static_cast<EKalmalaWildlifeArchetype>(255), UnsupportedShapes));
    TestTrue(TEXT("An unknown archetype leaves its profile empty"), UnsupportedShapes.IsEmpty());

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population body-clearance fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    const EKalmalaWildlifeArchetype ComponentCheckArchetypes[] = {
        EKalmalaWildlifeArchetype::Boar,
        EKalmalaWildlifeArchetype::Rat,
        EKalmalaWildlifeArchetype::MosquitoSwarm,
        EKalmalaWildlifeArchetype::ThunderSentinel,
        EKalmalaWildlifeArchetype::Viper,
        EKalmalaWildlifeArchetype::GiantSnake,
        EKalmalaWildlifeArchetype::DeathAdder
    };
    int32 ComponentIndex = 0;
    for (const EKalmalaWildlifeArchetype Archetype : ComponentCheckArchetypes)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const FVector ComponentLocation(20000.0 + ComponentIndex++ * 500.0, 20000.0, 0.0);
        AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
            ComponentLocation, FRotator::ZeroRotator, SpawnParameters);
        if (!TestNotNull(TEXT("Wildlife collision fixture actor created"), Creature)) return false;
        FKalmalaWorldPopulationSpawn Descriptor;
        Descriptor.Location = ComponentLocation;
        Creature->InitializeServer(Descriptor, Archetype);

        TArray<FKalmalaWildlifeBodyCollisionShape> BodyShapes;
        const bool bHasProfile = AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            Archetype, BodyShapes);
        TestTrue(TEXT("Actor archetype exposes its shared collision profile"), bHasProfile);
        if (!bHasProfile || BodyShapes.IsEmpty()) return false;
        const USphereComponent* RootSphere = Cast<USphereComponent>(Creature->GetRootComponent());
        if (!TestNotNull(TEXT("Actual wildlife root sphere exists"), RootSphere)) return false;
        TestEqual(TEXT("Actual root radius matches the placement profile"),
            RootSphere->GetUnscaledSphereRadius(), BodyShapes[0].RadiusCentimetres);

        TArray<UCapsuleComponent*> Capsules;
        Creature->GetComponents<UCapsuleComponent>(Capsules);
        const FKalmalaWildlifeBodyCollisionShape* CapsuleShape = BodyShapes.FindByPredicate(
            [](const FKalmalaWildlifeBodyCollisionShape& Shape)
            { return Shape.Type == EKalmalaWildlifeBodyShapeType::Capsule; });
        if (CapsuleShape != nullptr)
        {
            if (!TestTrue(TEXT("Snake collision component exists"), !Capsules.IsEmpty())) return false;
            const UCapsuleComponent* Capsule = Capsules[0];
            TestEqual(TEXT("Active snake capsule radius matches the shared profile"),
                Capsule->GetUnscaledCapsuleRadius(), CapsuleShape->RadiusCentimetres);
            TestEqual(TEXT("Active snake capsule half-height matches the shared profile"),
                Capsule->GetUnscaledCapsuleHalfHeight(), CapsuleShape->HalfHeightCentimetres);
            TestEqual(TEXT("Active snake capsule offset matches the shared profile"),
                Capsule->GetRelativeLocation(), CapsuleShape->RelativeLocation);
            TestEqual(TEXT("Active snake capsule orientation matches the shared profile"),
                Capsule->GetRelativeRotation(), CapsuleShape->RelativeRotation);
            TestEqual(TEXT("Active snake capsule remains query-only"),
                Capsule->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
        }
        else if (!Capsules.IsEmpty())
        {
            TestEqual(TEXT("Non-snake archetypes disable the secondary capsule"),
                Capsules[0]->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
        }
    }

    FKalmalaWildlifeBodyCollisionShape Sphere;
    Sphere.Type = EKalmalaWildlifeBodyShapeType::Sphere;
    Sphere.RadiusCentimetres = 50.0f;
    Sphere.HalfHeightCentimetres = 50.0f;
    TestFalse(TEXT("A clear sphere placement has no body overlap"),
        AKalmalaGameMode::DoesPopulationCollisionShapeOverlap(
            World, Sphere, FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator));

    FActorSpawnParameters BlockerParameters;
    BlockerParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Blocker = World->SpawnActor<AActor>(
        FVector::ZeroVector, FRotator::ZeroRotator, BlockerParameters);
    if (!TestNotNull(TEXT("Blocking collision fixture actor created"), Blocker)) return false;
    UBoxComponent* BlockerCollision = NewObject<UBoxComponent>(Blocker, TEXT("BodyClearanceBlocker"));
    if (!TestNotNull(TEXT("Blocking fixture component created"), BlockerCollision)) return false;
    Blocker->SetRootComponent(BlockerCollision);
    BlockerCollision->SetBoxExtent(FVector(60.0, 60.0, 60.0));
    BlockerCollision->SetCollisionProfileName(TEXT("BlockAll"));
    BlockerCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BlockerCollision->RegisterComponent();
    TestTrue(TEXT("A blocking body overlap is rejected"),
        AKalmalaGameMode::DoesPopulationCollisionShapeOverlap(
            World, Sphere, FVector::ZeroVector, FRotator::ZeroRotator));
    TestFalse(TEXT("The proposed actor can be ignored by its own placement query"),
        AKalmalaGameMode::DoesPopulationCollisionShapeOverlap(
            World, Sphere, FVector::ZeroVector, FRotator::ZeroRotator, Blocker));

    FKalmalaWildlifeBodyCollisionShape Capsule;
    Capsule.Type = EKalmalaWildlifeBodyShapeType::Capsule;
    Capsule.RadiusCentimetres = 20.0f;
    Capsule.HalfHeightCentimetres = 100.0f;
    Capsule.RelativeLocation = FVector(-22.0, 0.0, 0.0);
    Capsule.RelativeRotation = FRotator(0.0, 90.0, 0.0);
    TestTrue(TEXT("A capsule's full horizontal footprint must remain in world bounds"),
        AKalmalaGameMode::IsPopulationBodyShapeWithinWorldBounds(
            Capsule, FVector(FKalmalaWorldBounds::Radius - 20.0, 0.0, 100.0),
            FRotator::ZeroRotator));
    TestTrue(TEXT("A body touching the inclusive world boundary is allowed"),
        AKalmalaGameMode::IsPopulationBodyShapeWithinWorldBounds(
            Sphere, FVector(FKalmalaWorldBounds::Radius - 50.0, 0.0, 100.0),
            FRotator::ZeroRotator));
    TestFalse(TEXT("A body crossing world bounds is rejected"),
        AKalmalaGameMode::IsPopulationBodyShapeWithinWorldBounds(
            Sphere, FVector(FKalmalaWorldBounds::Radius - 49.0, 0.0, 100.0),
            FRotator::ZeroRotator));
    TestTrue(TEXT("Snake capsule geometry also rejects a blocking body overlap"),
        AKalmalaGameMode::DoesPopulationCollisionShapeOverlap(
            World, Capsule, FVector::ZeroVector, FRotator::ZeroRotator));
    TestFalse(TEXT("A clear snake capsule placement is accepted"),
        AKalmalaGameMode::DoesPopulationCollisionShapeOverlap(
            World, Capsule, FVector(1000.0, 0.0, 0.0), FRotator::ZeroRotator));

    FKalmalaWildlifeBodyCollisionShape InvalidShape = Sphere;
    InvalidShape.RadiusCentimetres = std::numeric_limits<float>::quiet_NaN();
    TestTrue(TEXT("Invalid body geometry fails closed as an overlap"),
        AKalmalaGameMode::DoesPopulationCollisionShapeOverlap(
            World, InvalidShape, FVector::ZeroVector, FRotator::ZeroRotator));
    TestFalse(TEXT("Invalid body geometry fails closed for bounds"),
        AKalmalaGameMode::IsPopulationBodyShapeWithinWorldBounds(
            InvalidShape, FVector::ZeroVector, FRotator::ZeroRotator));
    InvalidShape = Sphere;
    InvalidShape.Type = static_cast<EKalmalaWildlifeBodyShapeType>(255);
    TestTrue(TEXT("Unknown body shape types fail closed"),
        AKalmalaGameMode::DoesPopulationCollisionShapeOverlap(
            World, InvalidShape, FVector::ZeroVector, FRotator::ZeroRotator));

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population body-clearance GameMode created"), Mode)) return false;
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 19;
    Mode->WorldGenerationConfig = Config;
    const FVector2D Position(0.0, 0.0);
    const float GroundHeight = FKalmalaTerrainHeightSampler::SampleHeight(Config, Position);
    if (!FMath::IsFinite(GroundHeight)) return false;
    FKalmalaWorldPopulationSpawn Spawn;
    Spawn.Location = FVector(Position.X, Position.Y, GroundHeight);
    TArray<FKalmalaWildlifeBodyCollisionShape> RootProfile;
    AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
        EKalmalaWildlifeArchetype::Boar, RootProfile);
    if (!TestTrue(TEXT("Boar profile is available for placement"), !RootProfile.IsEmpty())) return false;
    const double TestSlopeRadians = FMath::DegreesToRadians(32.0);
    const FVector TestSlopeNormal(FMath::Sin(TestSlopeRadians), 0.0, FMath::Cos(TestSlopeRadians));
    double SphereSlopeOffset = 0.0;
    TestTrue(TEXT("A sphere support offset accepts a valid sloped plane"),
        AKalmalaGameMode::TryGetPopulationBodyVerticalSupportOffset(
            RootProfile, TestSlopeNormal, SphereSlopeOffset));
    TestTrue(TEXT("A sphere is raised by radius divided by normal Z on a slope"),
        FMath::IsNearlyEqual(SphereSlopeOffset,
            static_cast<double>(RootProfile[0].RadiusCentimetres) / TestSlopeNormal.Z));
    TestFalse(TEXT("A zero normal fails closed for body support"),
        AKalmalaGameMode::TryGetPopulationBodyVerticalSupportOffset(
            RootProfile, FVector::ZeroVector, SphereSlopeOffset));
    TestFalse(TEXT("A downward normal fails closed for body support"),
        AKalmalaGameMode::TryGetPopulationBodyVerticalSupportOffset(
            RootProfile, -FVector::UpVector, SphereSlopeOffset));

    FVector PlacementLocation;
    TestTrue(TEXT("Land placement location uses generated support and actual body extent"),
        Mode->TryGetPopulationActorPlacementLocation(
            Spawn, EKalmalaWildlifeArchetype::Boar, RootProfile, PlacementLocation));
    const FVector SampledSurfaceNormal = FKalmalaTerrainHeightSampler::SampleSurfaceNormal(
        Config, Position);
    double SampledSupportOffset = 0.0;
    TestTrue(TEXT("Sampled terrain normal yields a land body support offset"),
        AKalmalaGameMode::TryGetPopulationBodyVerticalSupportOffset(
            RootProfile, SampledSurfaceNormal, SampledSupportOffset));
    TestTrue(TEXT("Land actor anchor clears its sampled tangent plane"),
        PlacementLocation.Z >= GroundHeight + SampledSupportOffset);

    TArray<FKalmalaWildlifeBodyCollisionShape> GiantSnakeProfile;
    AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
        EKalmalaWildlifeArchetype::GiantSnake, GiantSnakeProfile);
    if (!TestTrue(TEXT("Giant Snake profile includes its root and capsule"), GiantSnakeProfile.Num() == 2)) return false;
    const FKalmalaWildlifeBodyCollisionShape& GiantSnakeCapsule = GiantSnakeProfile[1];
    const FVector CapsuleAxis = GiantSnakeCapsule.RelativeRotation.Quaternion()
        .RotateVector(FVector::UpVector).GetSafeNormal();
    const double CapsuleSupportAlongNormal = GiantSnakeCapsule.RadiusCentimetres
        + (GiantSnakeCapsule.HalfHeightCentimetres - GiantSnakeCapsule.RadiusCentimetres)
            * FMath::Abs(FVector::DotProduct(CapsuleAxis, TestSlopeNormal));
    const double CapsuleOffsetAlongNormal = FVector::DotProduct(
        GiantSnakeCapsule.RelativeLocation, TestSlopeNormal);
    const double ExpectedCapsuleSlopeOffset =
        (CapsuleSupportAlongNormal - CapsuleOffsetAlongNormal) / TestSlopeNormal.Z;
    double CapsuleSlopeOffset = 0.0;
    TestTrue(TEXT("A capsule support offset accepts the sampled nonzero slope"),
        AKalmalaGameMode::TryGetPopulationBodyVerticalSupportOffset(
            GiantSnakeProfile, TestSlopeNormal, CapsuleSlopeOffset));
    TestTrue(TEXT("Capsule axis and relative offset contribute to slope support"),
        FMath::IsNearlyEqual(CapsuleSlopeOffset, ExpectedCapsuleSlopeOffset));
    TestTrue(TEXT("Placement accounts for a snake's full active capsule extent"),
        Mode->TryGetPopulationActorPlacementLocation(
            Spawn, EKalmalaWildlifeArchetype::GiantSnake, GiantSnakeProfile, PlacementLocation));
    TestTrue(TEXT("Giant Snake actor anchor uses its capsule half-height"),
        PlacementLocation.Z >= GroundHeight + GiantSnakeProfile[1].HalfHeightCentimetres);

    TArray<FKalmalaWildlifeBodyCollisionShape> MosquitoProfile;
    AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
        EKalmalaWildlifeArchetype::MosquitoSwarm, MosquitoProfile);
    TestTrue(TEXT("Mosquito placement retains its existing flight-height anchor"),
        Mode->TryGetPopulationActorPlacementLocation(
            Spawn, EKalmalaWildlifeArchetype::MosquitoSwarm, MosquitoProfile, PlacementLocation));
    TestTrue(TEXT("Mosquito actor remains at its authored height above generated ground"),
        PlacementLocation.Z >= GroundHeight + AKalmalaWildlifeSpawn::MosquitoFlightHeightCentimetres);

    TArray<FKalmalaWildlifeBodyCollisionShape> ReefProfile;
    AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
        EKalmalaWildlifeArchetype::ReefMaw, ReefProfile);
    TestTrue(TEXT("Reef Maw preserves its existing deep-water actor anchor"),
        Mode->TryGetPopulationActorPlacementLocation(
            Spawn, EKalmalaWildlifeArchetype::ReefMaw, ReefProfile, PlacementLocation));
    TestTrue(TEXT("Reef Maw placement uses the existing spawn depth"),
        FMath::IsNearlyEqual(PlacementLocation.Z,
            static_cast<double>(FKalmalaTerrainHeightSampler::SeaLevelWorldHeight
                - AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres)));

    const FKalmalaWorldPopulationSpawn OutsideWorld = []
    {
        FKalmalaWorldPopulationSpawn Value;
        Value.Location = FVector(FKalmalaWorldBounds::Radius + 1.0, 0.0, 0.0);
        return Value;
    }();
    TestFalse(TEXT("The authoritative body-clearance predicate rejects out-of-bounds descriptors"),
        Mode->DoesPopulationDescriptorPassBodyClearance(OutsideWorld));
    FKalmalaWorldPopulationSpawn InvalidPosition;
    InvalidPosition.Location.X = std::numeric_limits<double>::quiet_NaN();
    TestFalse(TEXT("The authoritative body-clearance predicate rejects nonfinite descriptors"),
        Mode->DoesPopulationDescriptorPassBodyClearance(InvalidPosition));

    FKalmalaPopulationSpawnGateResults PlacementResults;
    PlacementResults.bSchedulerBudgetAvailable = true;
    PlacementResults.bSeededAdmissionRollPassed = true;
    PlacementResults.bPlayerDistancePassed = true;
    PlacementResults.bSpeciesHabitatPassed = true;
    PlacementResults.bTimePassed = true;
    PlacementResults.bWeatherPassed = true;
    PlacementResults.bProgressionPassed = true;
    PlacementResults.bCapsPassed = true;
    TestEqual(TEXT("S22a does not enable the final placement gate"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(PlacementResults),
        EKalmalaPopulationSpawnRejectionReason::PhysicalPlacement);
    return true;
}

#endif
