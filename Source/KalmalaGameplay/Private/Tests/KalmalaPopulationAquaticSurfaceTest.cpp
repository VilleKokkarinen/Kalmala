#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaTerrainHeightSampler.h"
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
    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 7;
    Mode->WorldGenerationConfig = Config;

    EKalmalaWildlifeArchetype OceanSlotArchetype = EKalmalaWildlifeArchetype::Mireling;
    if (!TestTrue(TEXT("Ocean slot 9 selects the Reef Maw archetype"),
        AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(
            9, EKalmalaBiome::Ocean, OceanSlotArchetype)
            && OceanSlotArchetype == EKalmalaWildlifeArchetype::ReefMaw))
    {
        return false;
    }

    FKalmalaWorldPopulationSpawn ReefMawDescriptor;
    bool bFoundValidReefMawDescriptor = false;
    constexpr int32 SearchSpacingCentimetres = 25000;
    const int32 SearchExtentCentimetres = static_cast<int32>(FKalmalaWorldBounds::Radius);
    const float MinimumFullBodyDepth = AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres
        + AKalmalaWildlifeSpawn::ReefMawBodyCollisionRadiusCentimetres;
    for (int32 Y = -SearchExtentCentimetres + SearchSpacingCentimetres;
        Y < SearchExtentCentimetres && !bFoundValidReefMawDescriptor; Y += SearchSpacingCentimetres)
    {
        for (int32 X = -SearchExtentCentimetres + SearchSpacingCentimetres;
            X < SearchExtentCentimetres && !bFoundValidReefMawDescriptor; X += SearchSpacingCentimetres)
        {
            const FVector2D Position(X, Y);
            FKalmalaOceanSample CenterOceanSample;
            if (!AKalmalaWildlifeSpawn::IsReefMawPositionNavigable(
                    Config, Position, &CenterOceanSample)
                || CenterOceanSample.WaterDepth < MinimumFullBodyDepth)
            {
                continue;
            }

            FKalmalaWorldPopulationSpawn Candidate;
            Candidate.Kind = EKalmalaWorldPopulationKind::Wildlife;
            Candidate.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(Position);
            Candidate.SpawnSeed = 9;
            Candidate.Location = FVector(Position.X, Position.Y,
                FKalmalaTerrainHeightSampler::SampleHeight(Config, Position));
            if (Mode->DoesPopulationDescriptorPassAquaticSurface(Candidate))
            {
                ReefMawDescriptor = Candidate;
                bFoundValidReefMawDescriptor = true;
            }
        }
    }
    if (!TestTrue(TEXT("A valid generated Ocean descriptor passes full Reef Maw placement"),
        bFoundValidReefMawDescriptor))
    {
        return false;
    }

    const double BodyRadius = AKalmalaWildlifeSpawn::ReefMawBodyCollisionRadiusCentimetres;
    const double SampleStepLimit = FKalmalaTerrainHeightSampler::NormalSampleDistance * 0.25;
    const int32 SampleIntervals = FMath::Max(1,
        FMath::CeilToInt(BodyRadius / SampleStepLimit));
    const double SampleStep = BodyRadius / SampleIntervals;
    const FVector2D CenterPosition(ReefMawDescriptor.Location);
    TArray<FVector2D> ExpectedSamplePositions;
    for (int32 XIndex = -SampleIntervals; XIndex <= SampleIntervals; ++XIndex)
    {
        for (int32 YIndex = -SampleIntervals; YIndex <= SampleIntervals; ++YIndex)
        {
            const FVector2D Offset(XIndex * SampleStep, YIndex * SampleStep);
            if (Offset.SizeSquared() > FMath::Square(BodyRadius)) continue;
            ExpectedSamplePositions.Add(CenterPosition + Offset);
        }
    }

    TArray<FVector2D> ObservedSamplePositions;
    int32 CenterSampleIndex = INDEX_NONE;
    Mode->PopulationAquaticSurfaceSampleForTests = [&ObservedSamplePositions, &CenterSampleIndex](
        const FVector2D& Position, const bool bIsCenterSample, bool&, FKalmalaOceanSample&)
    {
        if (bIsCenterSample) CenterSampleIndex = ObservedSamplePositions.Num();
        ObservedSamplePositions.Add(Position);
    };
    TestTrue(TEXT("The descriptor-level Reef Maw policy accepts the valid full-water formation"),
        Mode->DoesPopulationDescriptorPassAquaticSurface(ReefMawDescriptor));
    TestEqual(TEXT("The policy samples the center and every point in the sphere footprint"),
        ObservedSamplePositions.Num(), ExpectedSamplePositions.Num());
    TestTrue(TEXT("The center sample is present in the footprint sequence"),
        ObservedSamplePositions.IsValidIndex(CenterSampleIndex)
            && ExpectedSamplePositions.Num() > 1);
    for (int32 SampleIndex = 0; SampleIndex < ExpectedSamplePositions.Num(); ++SampleIndex)
    {
        const FString Message = FString::Printf(TEXT("Aquatic sample %d matches the expected footprint point"), SampleIndex);
        TestTrue(*Message, ObservedSamplePositions.IsValidIndex(SampleIndex)
            && ObservedSamplePositions[SampleIndex].Equals(ExpectedSamplePositions[SampleIndex]));
    }

    for (int32 FailedSampleIndex = 0; FailedSampleIndex < ExpectedSamplePositions.Num(); ++FailedSampleIndex)
    {
        ObservedSamplePositions.Reset();
        Mode->PopulationAquaticSurfaceSampleForTests = [FailedSampleIndex, &ObservedSamplePositions](
            const FVector2D& Position, bool, bool&, FKalmalaOceanSample& OceanSample)
        {
            const int32 CurrentSampleIndex = ObservedSamplePositions.Num();
            ObservedSamplePositions.Add(Position);
            if (CurrentSampleIndex == FailedSampleIndex)
            {
                OceanSample.WaterDepth = AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres - 0.01f;
            }
        };
        const FString RejectionMessage = FString::Printf(
            TEXT("A depth failure at Reef Maw sample %d rejects the descriptor"), FailedSampleIndex);
        TestFalse(*RejectionMessage,
            Mode->DoesPopulationDescriptorPassAquaticSurface(ReefMawDescriptor));
        TestEqual(TEXT("The aquatic policy stops at the failed sample"),
            ObservedSamplePositions.Num(), FailedSampleIndex + 1);
        const FString PositionMessage = FString::Printf(
            TEXT("The expected footprint point %d caused rejection"), FailedSampleIndex);
        TestTrue(*PositionMessage, ObservedSamplePositions.IsValidIndex(FailedSampleIndex)
            && ObservedSamplePositions[FailedSampleIndex].Equals(ExpectedSamplePositions[FailedSampleIndex]));
    }

    ObservedSamplePositions.Reset();
    bool bCenterSupportFailureInjected = false;
    Mode->PopulationAquaticSurfaceSampleForTests = [&ObservedSamplePositions, &bCenterSupportFailureInjected](
        const FVector2D& Position, const bool bIsCenterSample, bool& bHasSwimmingSupport, FKalmalaOceanSample&)
    {
        ObservedSamplePositions.Add(Position);
        if (bIsCenterSample)
        {
            bHasSwimmingSupport = false;
            bCenterSupportFailureInjected = true;
        }
    };
    TestFalse(TEXT("The descriptor rejects missing center swimming support"),
        Mode->DoesPopulationDescriptorPassAquaticSurface(ReefMawDescriptor));
    TestTrue(TEXT("The center swimming-support failure was exercised"), bCenterSupportFailureInjected);
    TestTrue(TEXT("Center support rejection occurs at the expected center sample"),
        ObservedSamplePositions.Num() == CenterSampleIndex + 1
            && ObservedSamplePositions.IsValidIndex(CenterSampleIndex)
            && ObservedSamplePositions[CenterSampleIndex].Equals(CenterPosition));

    int32 TerrainFailureSampleIndex = INDEX_NONE;
    for (int32 SampleIndex = 0; SampleIndex < ExpectedSamplePositions.Num(); ++SampleIndex)
    {
        if (SampleIndex != CenterSampleIndex)
        {
            TerrainFailureSampleIndex = SampleIndex;
            break;
        }
    }
    ObservedSamplePositions.Reset();
    Mode->PopulationAquaticSurfaceSampleForTests = [TerrainFailureSampleIndex, &ObservedSamplePositions](
        const FVector2D& Position, bool, bool&, FKalmalaOceanSample& OceanSample)
    {
        const int32 CurrentSampleIndex = ObservedSamplePositions.Num();
        ObservedSamplePositions.Add(Position);
        if (CurrentSampleIndex == TerrainFailureSampleIndex)
        {
            OceanSample.TerrainHeight = FKalmalaTerrainHeightSampler::SeaLevelWorldHeight;
        }
    };
    TestFalse(TEXT("A terrain-overlapping sphere footprint sample rejects the descriptor"),
        Mode->DoesPopulationDescriptorPassAquaticSurface(ReefMawDescriptor));
    TestTrue(TEXT("The terrain-overlap sample caused descriptor rejection"),
        ObservedSamplePositions.Num() == TerrainFailureSampleIndex + 1
            && ObservedSamplePositions.IsValidIndex(TerrainFailureSampleIndex)
            && ObservedSamplePositions[TerrainFailureSampleIndex].Equals(
                ExpectedSamplePositions[TerrainFailureSampleIndex]));
    Mode->PopulationAquaticSurfaceSampleForTests = nullptr;

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
        static_cast<double>(FKalmalaTerrainHeightSampler::SeaLevelWorldHeight) - AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres);
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
