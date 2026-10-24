#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldBounds.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationAquaticSurfaceTest,
    "Kalmala.Gameplay.Population.AquaticSurface",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationAquaticSurfaceTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Only Reef Maw is accepted by the aquatic population policy"),
        AKalmalaGameMode::IsPopulationAquaticArchetype(EKalmalaWildlifeArchetype::ReefMaw));
    TestFalse(TEXT("Land wildlife is rejected by the aquatic population policy"),
        AKalmalaGameMode::IsPopulationAquaticArchetype(EKalmalaWildlifeArchetype::Boar));
    TestFalse(TEXT("Wolves are rejected by the aquatic population policy"),
        AKalmalaGameMode::IsPopulationAquaticArchetype(EKalmalaWildlifeArchetype::Wolf));
    TestTrue(TEXT("The existing Ocean habitat depth boundary remains inclusive"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(
            EKalmalaBiome::Ocean, false, AKalmalaWildlifeSpawn::ReefMawMinimumOceanDepthCentimetres));
    TestFalse(TEXT("Shallower than the existing Reef Maw minimum is rejected"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(
            EKalmalaBiome::Ocean, false,
            AKalmalaWildlifeSpawn::ReefMawMinimumOceanDepthCentimetres - 0.01f));
    TestFalse(TEXT("Lake water remains outside Reef Maw habitat"),
        AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(EKalmalaBiome::Ocean, true, 500.0f));

    FKalmalaOceanSample CenterSample;
    CenterSample.bIsValid = true;
    CenterSample.WaterDepth = AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres
        + AKalmalaWildlifeSpawn::ReefMawBodyCollisionRadiusCentimetres;
    CenterSample.TerrainHeight = FKalmalaTerrainHeightSampler::SeaLevelWorldHeight - CenterSample.WaterDepth;
    TestTrue(TEXT("The ocean exactly contains the full Reef Maw sphere at its center"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(CenterSample, 0.0));

    FKalmalaOceanSample TooShallowSample = CenterSample;
    TooShallowSample.WaterDepth -= 0.01f;
    TooShallowSample.TerrainHeight += 0.01f;
    TestFalse(TEXT("Water that does not contain the sphere's lowest point is rejected"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(TooShallowSample, 0.0));

    FKalmalaOceanSample TerrainOverlapSample = CenterSample;
    TerrainOverlapSample.WaterDepth += 80.0f;
    TerrainOverlapSample.TerrainHeight += 0.01f;
    TestFalse(TEXT("A water-depth value cannot hide generated terrain penetrating the body"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(TerrainOverlapSample, 0.0));

    FKalmalaOceanSample EdgeSample;
    EdgeSample.bIsValid = true;
    EdgeSample.WaterDepth = AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres;
    EdgeSample.TerrainHeight = FKalmalaTerrainHeightSampler::SeaLevelWorldHeight - EdgeSample.WaterDepth;
    TestTrue(TEXT("The sphere's horizontal edge may touch its lower water boundary"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(
            EdgeSample, AKalmalaWildlifeSpawn::ReefMawBodyCollisionRadiusCentimetres));
    TestFalse(TEXT("Samples outside the actual sphere footprint are rejected"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(
            EdgeSample, AKalmalaWildlifeSpawn::ReefMawBodyCollisionRadiusCentimetres + 0.01));

    TestFalse(TEXT("An invalid Ocean sample fails closed"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(FKalmalaOceanSample(), 0.0));
    FKalmalaOceanSample InvalidDepthSample = CenterSample;
    InvalidDepthSample.WaterDepth = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("A non-finite water depth fails closed"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(InvalidDepthSample, 0.0));
    InvalidDepthSample.WaterDepth = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("Infinite water depth fails closed"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(InvalidDepthSample, 0.0));
    FKalmalaOceanSample InvalidTerrainSample = CenterSample;
    InvalidTerrainSample.TerrainHeight = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("A non-finite terrain sample fails closed"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(InvalidTerrainSample, 0.0));
    TestFalse(TEXT("A non-finite footprint position fails closed"),
        AKalmalaGameMode::DoesReefMawBodyFitOceanSample(
            CenterSample, std::numeric_limits<double>::quiet_NaN()));

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 7;
    FKalmalaOceanSample InvalidPositionSample;
    TestFalse(TEXT("Swimming support rejects positions outside world bounds"),
        AKalmalaWildlifeSpawn::IsReefMawPositionNavigable(
            Config, FVector2D(FKalmalaWorldBounds::Radius + 1.0, 0.0), &InvalidPositionSample));
    TestFalse(TEXT("Ocean support rejects nonfinite positions"),
        AKalmalaWildlifeSpawn::IsReefMawOceanPosition(
            Config, FVector2D(std::numeric_limits<double>::quiet_NaN(), 0.0), &InvalidPositionSample));
    TestFalse(TEXT("Invalid support queries clear their output sample"), InvalidPositionSample.bIsValid);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population aquatic fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population aquatic fixture GameMode created"), Mode)) return false;
    FKalmalaWorldPopulationSpawn OutsideWorld;
    OutsideWorld.Location = FVector(FKalmalaWorldBounds::Radius + 1.0, 0.0, 0.0);
    TestFalse(TEXT("The authoritative aquatic predicate rejects an out-of-bounds descriptor"),
        Mode->DoesPopulationDescriptorPassAquaticSurface(OutsideWorld));

    AKalmalaWildlifeSpawn* ReefMaw = World->SpawnActor<AKalmalaWildlifeSpawn>();
    if (!TestNotNull(TEXT("Reef Maw collision fixture created"), ReefMaw)) return false;
    FKalmalaWorldPopulationSpawn Descriptor;
    Descriptor.SpawnSeed = 9;
    ReefMaw->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::ReefMaw);
    const USphereComponent* RootSphere = Cast<USphereComponent>(ReefMaw->GetRootComponent());
    if (!TestNotNull(TEXT("Reef Maw retains its root sphere collision component"), RootSphere)) return false;
    TestEqual(TEXT("Aquatic clearance uses the actual Reef Maw root-sphere radius"),
        RootSphere->GetUnscaledSphereRadius(), AKalmalaWildlifeSpawn::ReefMawBodyCollisionRadiusCentimetres);
    TestEqual(TEXT("The Reef Maw actor remains at its existing spawn depth"),
        ReefMaw->GetActorLocation().Z,
        FKalmalaTerrainHeightSampler::SeaLevelWorldHeight - AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres);
    FKalmalaPopulationSpawnGateResults PlacementResults;
    PlacementResults.bSchedulerBudgetAvailable = true;
    PlacementResults.bSeededAdmissionRollPassed = true;
    PlacementResults.bPlayerDistancePassed = true;
    PlacementResults.bSpeciesHabitatPassed = true;
    PlacementResults.bTimePassed = true;
    PlacementResults.bWeatherPassed = true;
    PlacementResults.bProgressionPassed = true;
    PlacementResults.bCapsPassed = true;
    TestEqual(TEXT("S21 leaves final placement fail-closed until S25"),
        AKalmalaGameMode::EvaluatePopulationSpawnGates(PlacementResults),
        EKalmalaPopulationSpawnRejectionReason::PhysicalPlacement);

    return true;
}

#endif
