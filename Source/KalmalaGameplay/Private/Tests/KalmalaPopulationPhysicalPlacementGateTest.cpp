#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWolfPackContract.h"
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
    Mode->WorldGenerationConfig.WorldSeed = 418;
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

    ExistingCreature->SetActorEnableCollision(false);
    ExistingCreature->Destroy();

    FKalmalaWorldPopulationSpawn WolfPack;
    bool bFoundValidWolfPack = false;
    constexpr int32 SearchSpacingCentimetres = 25000;
    const int32 SearchExtentCentimetres = static_cast<int32>(FKalmalaWorldBounds::Radius);
    for (int32 Y = -SearchExtentCentimetres + SearchSpacingCentimetres;
        Y < SearchExtentCentimetres && !bFoundValidWolfPack; Y += SearchSpacingCentimetres)
    {
        for (int32 X = -SearchExtentCentimetres + SearchSpacingCentimetres;
            X < SearchExtentCentimetres && !bFoundValidWolfPack; X += SearchSpacingCentimetres)
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
            if (Mode->DoesPopulationDescriptorPassSpeciesHabitat(Candidate, &Archetype)
                && Archetype == EKalmalaWildlifeArchetype::Wolf
                && Mode->DoesPopulationDescriptorPassPhysicalPlacement(Candidate))
            {
                WolfPack = Candidate;
                bFoundValidWolfPack = true;
            }
        }
    }
    if (!TestTrue(TEXT("A valid generated Wolf pack passes final physical placement without sources"),
        bFoundValidWolfPack)) return false;

    // Production's 1800 cm fields contain the 240 cm pack anchor too; use a
    // narrower test field to prove S25 evaluates each member origin separately.
    const double IsolatedSuppressionRadiusCentimetres = 200.0;
    Mode->PopulationSuppressionRadiusCmForTests = IsolatedSuppressionRadiusCentimetres;
    TArray<FKalmalaWildlifeBodyCollisionShape> WolfShapes;
    if (!TestTrue(TEXT("Wolf exposes its active placement body profile"),
        AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            EKalmalaWildlifeArchetype::Wolf, WolfShapes))) return false;

    TArray<FVector> MemberActorLocations;
    MemberActorLocations.Reserve(FKalmalaWolfPackContract::MemberCount);
    for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
    {
        FKalmalaWorldPopulationSpawn MemberDescriptor;
        if (!TestTrue(FString::Printf(TEXT("Wolf member %d descriptor is generated"), MemberIndex),
            FKalmalaWolfPackContract::TryBuildMemberSpawn(
                Mode->WorldGenerationConfig, WolfPack, MemberIndex, MemberDescriptor))) return false;
        FVector MemberActorLocation;
        if (!TestTrue(FString::Printf(TEXT("Wolf member %d has its actual proposed origin"), MemberIndex),
            Mode->TryGetPopulationActorPlacementLocation(
                MemberDescriptor, EKalmalaWildlifeArchetype::Wolf, WolfShapes, MemberActorLocation))) return false;
        MemberActorLocations.Add(MemberActorLocation);
        TestTrue(FString::Printf(TEXT("Wolf member %d origin lies outside the isolated anchor field"), MemberIndex),
            FVector::Dist2D(MemberActorLocation, WolfPack.Location)
                > IsolatedSuppressionRadiusCentimetres);
    }
    TestEqual(TEXT("The valid formation has all three member origins"),
        MemberActorLocations.Num(), FKalmalaWolfPackContract::MemberCount);
    TestTrue(TEXT("The final placement gate still passes without suppression sources"),
        Mode->DoesPopulationDescriptorPassPhysicalPlacement(WolfPack));

    const auto SpawnSuppressionSource = [World](const FName Kit, const FString& ConstructionId,
        const FVector& Location) -> AKalmalaConstructionActor*
    {
        FActorSpawnParameters SourceParameters;
        SourceParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaConstructionActor* Source = World->SpawnActor<AKalmalaConstructionActor>(
            Location, FRotator::ZeroRotator, SourceParameters);
        if (IsValid(Source))
        {
            Source->InitializeFromServer(Kit, ConstructionId);
            Source->SetActorEnableCollision(false);
        }
        return Source;
    };

    const auto VerifySourceAtMember = [&](const int32 MemberIndex, const FName Kit,
        const FString& SourceLabel, const bool bIsStation) -> bool
    {
        const FString ConstructionId = FString::Printf(
            TEXT("physical-placement-%s-%d"), *SourceLabel, MemberIndex);
        AKalmalaConstructionActor* Source = SpawnSuppressionSource(
            Kit, ConstructionId, MemberActorLocations[MemberIndex]);
        if (!TestNotNull(*FString::Printf(TEXT("%s source for Wolf member %d is placed"),
                *SourceLabel, MemberIndex), Source)) return false;
        TestTrue(FString::Printf(TEXT("%s source is centered on Wolf member %d actual origin"),
            *SourceLabel, MemberIndex),
            Source->GetActorLocation().Equals(MemberActorLocations[MemberIndex]));

        const FString MemberMessage = FString::Printf(
            TEXT("%s source overlaps the actual Wolf member %d origin"), *SourceLabel, MemberIndex);
        const FString AnchorMessage = FString::Printf(
            TEXT("%s source does not overlap the Wolf pack anchor"), *SourceLabel);
        if (bIsStation)
        {
            TestTrue(*MemberMessage,
                Mode->DoesPopulationPointOverlapStationCylinder(MemberActorLocations[MemberIndex]));
            TestFalse(*AnchorMessage, Mode->DoesPopulationPointOverlapStationCylinder(WolfPack.Location));
            TestFalse(TEXT("A station does not emit camp-sphere suppression at the member origin"),
                Mode->DoesPopulationPointOverlapCampSphere(MemberActorLocations[MemberIndex]));
        }
        else
        {
            TestTrue(*MemberMessage,
                Mode->DoesPopulationPointOverlapCampSphere(MemberActorLocations[MemberIndex]));
            TestFalse(*AnchorMessage, Mode->DoesPopulationPointOverlapCampSphere(WolfPack.Location));
            TestFalse(TEXT("A Camp Bed does not emit station-cylinder suppression at the member origin"),
                Mode->DoesPopulationPointOverlapStationCylinder(MemberActorLocations[MemberIndex]));
        }

        for (int32 OtherMemberIndex = 0;
            OtherMemberIndex < MemberActorLocations.Num(); ++OtherMemberIndex)
        {
            if (OtherMemberIndex == MemberIndex) continue;
            const FString OtherMemberMessage = FString::Printf(
                TEXT("%s source leaves Wolf member %d outside its isolated field"),
                *SourceLabel, OtherMemberIndex);
            const FVector& OtherMemberLocation = MemberActorLocations[OtherMemberIndex];
            const bool bOtherMemberOverlaps = bIsStation
                ? Mode->DoesPopulationPointOverlapStationCylinder(OtherMemberLocation)
                : Mode->DoesPopulationPointOverlapCampSphere(OtherMemberLocation);
            TestFalse(*OtherMemberMessage, bOtherMemberOverlaps);
        }

        TestFalse(FString::Printf(TEXT("S25 rejects the Wolf pack for %s at member %d"),
            *SourceLabel, MemberIndex), Mode->DoesPopulationDescriptorPassPhysicalPlacement(WolfPack));
        Source->Destroy();
        TestTrue(FString::Printf(TEXT("S25 passes again after removing %s at member %d"),
            *SourceLabel, MemberIndex), Mode->DoesPopulationDescriptorPassPhysicalPlacement(WolfPack));
        return true;
    };

    const FName StationKits[] = { TEXT("WorkbenchKit"), TEXT("ForgeKit") };
    const FString StationLabels[] = { TEXT("Workbench"), TEXT("Forge") };
    constexpr int32 StationCount = static_cast<int32>(UE_ARRAY_COUNT(StationKits));
    for (int32 MemberIndex = 0; MemberIndex < MemberActorLocations.Num(); ++MemberIndex)
    {
        for (int32 StationIndex = 0; StationIndex < StationCount; ++StationIndex)
        {
            if (!VerifySourceAtMember(MemberIndex, StationKits[StationIndex],
                    StationLabels[StationIndex], true)) return false;
        }
        if (!VerifySourceAtMember(MemberIndex, TEXT("CampBedKit"), TEXT("Camp Bed"), false)) return false;
    }

    Mode->PopulationSuppressionRadiusCmForTests = -1.0;
    return true;
}

#endif
