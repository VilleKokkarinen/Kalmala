#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWolfPackContract.h"
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
        AKalmalaGameMode::IsPopulationLandSurfaceSampleAllowed(true, true, true, FVector::UpVector),
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
    Mode->WorldGenerationConfig.WorldSeed = 418;

    FKalmalaWorldPopulationSpawn WolfPack;
    TArray<FVector> ExpectedSurfaceLocations;
    bool bFoundWolfPack = false;
    constexpr int32 SearchSpacingCentimetres = 25000;
    const int32 SearchExtentCentimetres = static_cast<int32>(FKalmalaWorldBounds::Radius);
    for (int32 Y = -SearchExtentCentimetres + SearchSpacingCentimetres;
        Y < SearchExtentCentimetres && !bFoundWolfPack; Y += SearchSpacingCentimetres)
    {
        for (int32 X = -SearchExtentCentimetres + SearchSpacingCentimetres;
            X < SearchExtentCentimetres && !bFoundWolfPack; X += SearchSpacingCentimetres)
        {
            const FVector2D Position(X, Y);
            if (!FKalmalaWorldBounds::Contains(Mode->WorldGenerationConfig, Position)
                || FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Mode->WorldGenerationConfig, Position))
                    != EKalmalaBiome::FreezingTundra)
            {
                continue;
            }

            FKalmalaWorldPopulationSpawn Candidate;
            Candidate.Kind = EKalmalaWorldPopulationKind::Wildlife;
            Candidate.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(Position);
            Candidate.SpawnSeed = 5;
            Candidate.Location = FVector(Position.X, Position.Y,
                FKalmalaTerrainHeightSampler::SampleHeight(Mode->WorldGenerationConfig, Position));
            EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
            if (!Mode->DoesPopulationDescriptorPassSpeciesHabitat(Candidate, &Archetype)
                || Archetype != EKalmalaWildlifeArchetype::Wolf)
            {
                continue;
            }

            TArray<FVector> CandidateSurfaceLocations = { Candidate.Location };
            bool bAllMembersBuild = Mode->DoesPopulationLandPositionPassSurface(Candidate.Location);
            if (!bAllMembersBuild)
            {
                continue;
            }
            for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
            {
                FKalmalaWorldPopulationSpawn MemberSpawn;
                if (!FKalmalaWolfPackContract::TryBuildMemberSpawn(
                    Mode->WorldGenerationConfig, Candidate, MemberIndex, MemberSpawn))
                {
                    bAllMembersBuild = false;
                    break;
                }
                CandidateSurfaceLocations.Add(MemberSpawn.Location);
                if (!Mode->DoesPopulationLandPositionPassSurface(MemberSpawn.Location))
                {
                    bAllMembersBuild = false;
                    break;
                }
            }

            if (bAllMembersBuild)
            {
                WolfPack = Candidate;
                ExpectedSurfaceLocations = MoveTemp(CandidateSurfaceLocations);
                bFoundWolfPack = true;
            }
        }
    }
    if (!TestTrue(TEXT("A generated Tundra Wolf pack with three buildable members was found"), bFoundWolfPack))
    {
        return false;
    }

    TArray<FVector> ObservedSurfaceLocations;
    Mode->PopulationLandSurfaceSampleForTests = [&ObservedSurfaceLocations](const FVector& Location,
        const bool bSampleAllowed)
    {
        ObservedSurfaceLocations.Add(Location);
        return bSampleAllowed;
    };
    TestTrue(TEXT("A Wolf pack passes the descriptor-level land-surface gate when all samples pass"),
        Mode->DoesPopulationDescriptorPassLandSurface(WolfPack));
    TestEqual(TEXT("The Wolf gate samples the anchor and each of its three generated members"),
        ObservedSurfaceLocations.Num(), 1 + FKalmalaWolfPackContract::MemberCount);
    for (int32 SampleIndex = 0; SampleIndex < ExpectedSurfaceLocations.Num(); ++SampleIndex)
    {
        const FString Message = FString::Printf(TEXT("Surface sample %d is the expected pack position"), SampleIndex);
        TestTrue(*Message, ObservedSurfaceLocations.IsValidIndex(SampleIndex)
            && ObservedSurfaceLocations[SampleIndex].Equals(ExpectedSurfaceLocations[SampleIndex]));
    }

    for (int32 FailedSampleIndex = 0; FailedSampleIndex < ExpectedSurfaceLocations.Num(); ++FailedSampleIndex)
    {
        ObservedSurfaceLocations.Reset();
        Mode->PopulationLandSurfaceSampleForTests = [FailedSampleIndex, &ObservedSurfaceLocations](
            const FVector& Location, const bool bSampleAllowed)
        {
            const int32 CurrentSampleIndex = ObservedSurfaceLocations.Num();
            ObservedSurfaceLocations.Add(Location);
            return bSampleAllowed && CurrentSampleIndex != FailedSampleIndex;
        };
        const FString RejectionMessage = FString::Printf(
            TEXT("Failing Wolf pack surface sample %d rejects descriptor placement"), FailedSampleIndex);
        TestFalse(*RejectionMessage, Mode->DoesPopulationDescriptorPassLandSurface(WolfPack));
        const FString CoverageMessage = FString::Printf(
            TEXT("The gate checks all four pack positions when sample %d fails"), FailedSampleIndex);
        TestEqual(*CoverageMessage, ObservedSurfaceLocations.Num(), ExpectedSurfaceLocations.Num());
    }
    Mode->PopulationLandSurfaceSampleForTests = nullptr;

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
