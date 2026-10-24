#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaEnvironmentalSpawnerPolicy.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaEnvironmentalSpawnerAdmissionTest,
    "Kalmala.Gameplay.Population.EnvironmentalSpawnerAdmission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaEnvironmentalSpawnerAdmissionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Environmental source fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Environmental source fixture GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig.WorldSeed = 9317;
    UKalmalaWorldPopulationSaveGame* Save = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Environmental source sparse save fixture created"), Save)) return false;
    Save->InitializeForWorld(Mode->WorldGenerationConfig);
    Mode->PopulationSaveGame = Save;

    const FIntPoint HomeKey(0, 0);
    Mode->ActivePopulationSpatialKeys.Add(HomeKey);
    Mode->PopulationKeyWildlifeActorBudgets.Add(HomeKey, 20);

    const FString SourceId = TEXT("configured-source-a");
    const FVector SourceOrigin = FVector::ZeroVector;
    const TArray<FVector> SourceOriginLocation = { SourceOrigin };
    FGuid FirstReservation;
    FGuid SecondReservation;
    FGuid RejectedReservation;
    TestTrue(TEXT("A source-owned reservation consumes one shared wildlife slot"),
        Mode->TryReservePopulationCreatureCapacity(SourceOriginLocation, HomeKey,
            EKalmalaWildlifeArchetype::Rat, SourceId, 0, SourceOrigin, FirstReservation));
    TestTrue(TEXT("A second finite source slot shares the same local source cap"),
        Mode->TryReservePopulationCreatureCapacity(SourceOriginLocation, HomeKey,
            EKalmalaWildlifeArchetype::Rat, SourceId, 1, SourceOrigin, SecondReservation));

    int32 WorldTotal = 0;
    int32 NearbySpecies = 0;
    int32 HomeKeyTotal = 0;
    Mode->GetPopulationCreatureCapCounts(SourceOrigin, EKalmalaWildlifeArchetype::Rat,
        HomeKey, WorldTotal, NearbySpecies, HomeKeyTotal);
    TestEqual(TEXT("Pending source reservations count toward the shared total cap"), WorldTotal, 2);
    TestEqual(TEXT("Typed reservations count toward the shared species cap"), NearbySpecies, 2);
    TestEqual(TEXT("Pending source reservations count toward their home-key budget"), HomeKeyTotal, 2);
    TestFalse(TEXT("A third source-owned actor is rejected by the local cap"),
        Mode->TryReservePopulationCreatureCapacity(SourceOriginLocation, HomeKey,
            EKalmalaWildlifeArchetype::Rat, SourceId, 0, SourceOrigin, RejectedReservation));
    TestFalse(TEXT("Rejected local admission leaves no partial reservation token"), RejectedReservation.IsValid());

    const bool bReleasedFirst = Mode->ReleasePopulationCreatureReservation(FirstReservation);
    TestTrue(TEXT("Releasing a source reservation frees its local slot"), bReleasedFirst);
    FGuid ReplacementReservation;
    TestTrue(TEXT("The freed local slot may be reserved again"),
        Mode->TryReservePopulationCreatureCapacity(SourceOriginLocation, HomeKey,
            EKalmalaWildlifeArchetype::Rat, SourceId, 0, SourceOrigin, ReplacementReservation));
    TestTrue(TEXT("All source reservations can be released"),
        Mode->ReleasePopulationCreatureReservation(SecondReservation)
        && Mode->ReleasePopulationCreatureReservation(ReplacementReservation));

    const TArray<FVector> SourceWolfPack = {
        SourceOrigin, SourceOrigin + FVector(100.0, 0.0, 0.0), SourceOrigin + FVector(0.0, 100.0, 0.0)
    };
    FGuid RejectedPackReservation;
    TestFalse(TEXT("A three-individual source Wolf pack cannot partially fit a two-actor local cap"),
        Mode->TryReservePopulationCreatureCapacity(SourceWolfPack, HomeKey,
            EKalmalaWildlifeArchetype::Wolf, SourceId, 0, SourceOrigin, RejectedPackReservation));
    TestFalse(TEXT("A rejected source pack returns no reservation token"), RejectedPackReservation.IsValid());
    TestTrue(TEXT("A rejected source pack leaves no provisional shared reservations"),
        Mode->PendingPopulationCreatureReservations.IsEmpty());

    const TArray<FKalmalaWorldPopulationSpawn> Descriptors =
        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Mode->WorldGenerationConfig,
            HomeKey, EKalmalaWorldPopulationKind::Wildlife);
    if (!TestTrue(TEXT("A deterministic fixed wildlife candidate exists for the configured-source check"),
            !Descriptors.IsEmpty()))
    {
        return false;
    }
    const FKalmalaWorldPopulationSpawn& Candidate = Descriptors[0];
    TestTrue(TEXT("The candidate has room under existing shared species and total caps"),
        Mode->DoesPopulationDescriptorPassCaps(Candidate));

    APlayerController* Controller = World->SpawnActor<APlayerController>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    FActorSpawnParameters PawnParameters;
    PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(
        AKalmalaCharacter::StaticClass(), Candidate.Location + FVector(6400.0, 0.0, 0.0),
        FRotator::ZeroRotator, PawnParameters);
    if (!TestTrue(TEXT("A server-owned player fixture is possessed and connected"),
            IsValid(Controller) && IsValid(PlayerState) && IsValid(Player)))
    {
        return false;
    }
    PlayerState->SetPlayerId(1);
    Controller->SetPlayerState(PlayerState);
    Controller->Possess(Player);
    World->AddController(Controller);

    FGuid LocalCapReservationA;
    FGuid LocalCapReservationB;
    const TArray<FVector> CandidateLocation = { Candidate.Location };
    TestTrue(TEXT("First pending local actor reserves capacity for gate evaluation"),
        Mode->TryReservePopulationCreatureCapacity(CandidateLocation, Candidate.SpatialKey,
            EKalmalaWildlifeArchetype::Rat, SourceId, 0, Candidate.Location, LocalCapReservationA));
    TestTrue(TEXT("Second pending local actor fills the configured local cap"),
        Mode->TryReservePopulationCreatureCapacity(CandidateLocation, Candidate.SpatialKey,
            EKalmalaWildlifeArchetype::Rat, SourceId, 1, Candidate.Location, LocalCapReservationB));
    TestTrue(TEXT("The full local source count does not bypass or exceed shared caps"),
        Mode->DoesPopulationDescriptorPassCaps(Candidate));

    uint64 PassingCheckOrdinal = 0;
    const FString CandidateSpawnId =
        FKalmalaEnvironmentalSpawnerPolicy::GetStableSlotPersistentSpawnId(SourceId, 0);
    for (; PassingCheckOrdinal < 256; ++PassingCheckOrdinal)
    {
        if (FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
                Mode->WorldGenerationConfig, Candidate.SpatialKey, CandidateSpawnId,
                PassingCheckOrdinal)
            <= FKalmalaMonsterSpawnTuning::PopulationAdmissionRollMaxAccepted)
        {
            break;
        }
    }
    if (!TestTrue(TEXT("A deterministic check ordinal passes the seeded admission roll"),
            PassingCheckOrdinal < 256))
    {
        return false;
    }

    FKalmalaEnvironmentalSpawnerCheck DisabledCheck;
    DisabledCheck.SourceId = SourceId;
    DisabledCheck.SourceSlotIndex = 0;
    DisabledCheck.SourceOrigin = Candidate.Location;
    DisabledCheck.NextDueSeconds = 8.0;
    int32 RemainingBudget = 16;
    int32 CandidatesExamined = -1;
    TArray<FKalmalaPopulationSpawnAttempt> Attempts;
    TestFalse(TEXT("The source path remains disabled without explicit configuration"),
        Mode->TryCreateConfiguredEnvironmentalSpawnerCandidates(DisabledCheck, { Candidate },
            RemainingBudget, 8.0, CandidatesExamined, Attempts));
    TestEqual(TEXT("An unconfigured source consumes no shared candidate budget"), RemainingBudget, 16);
    TestEqual(TEXT("An unconfigured source does not advance its due time"), DisabledCheck.NextDueSeconds, 8.0);
    TestTrue(TEXT("An unconfigured source creates no attempt record"), Attempts.IsEmpty());
    int32 WildlifeActorCount = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It) ++WildlifeActorCount;
    TestEqual(TEXT("An unconfigured source creates no wildlife actor"), WildlifeActorCount, 0);
    int32 WildlifeActorCount = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It) ++WildlifeActorCount;
    TestEqual(TEXT("An unconfigured source creates no wildlife actor"), WildlifeActorCount, 0);

    FKalmalaEnvironmentalSpawnerCheck ConfiguredCheck;
    ConfiguredCheck.bSourceConfigured = true;
    ConfiguredCheck.SourceId = SourceId;
    ConfiguredCheck.SourceSlotIndex = 0;
    ConfiguredCheck.SourceOrigin = Candidate.Location;
    ConfiguredCheck.NextDueSeconds = 8.0;
    ConfiguredCheck.CheckOrdinal = PassingCheckOrdinal;
    TestFalse(TEXT("A full local cap rejects the configured candidate before creation"),
        Mode->TryCreateConfiguredEnvironmentalSpawnerCandidates(ConfiguredCheck, { Candidate },
            RemainingBudget, 8.0, CandidatesExamined, Attempts));
    TestEqual(TEXT("One examined source candidate consumes one shared wake slot"), RemainingBudget, 15);
    TestEqual(TEXT("Serviced source work uses service-relative due time"), ConfiguredCheck.NextDueSeconds, 16.0);
    TestEqual(TEXT("The bounded source check records its examined attempt"), Attempts.Num(), 1);
    if (Attempts.Num() == 1)
    {
        TestTrue(TEXT("Player exclusion and all earlier species/time/weather/progression gates ran"),
            Attempts[0].GateResults.bPlayerDistancePassed
            && Attempts[0].GateResults.bSpeciesHabitatPassed
            && Attempts[0].GateResults.bTimePassed
            && Attempts[0].GateResults.bWeatherPassed
            && Attempts[0].GateResults.bProgressionPassed);
        TestFalse(TEXT("The source-local two-creature cap rejects admission"),
            Attempts[0].GateResults.bCapsPassed);
        TestEqual(TEXT("The cap rejection remains the first rejection even after placement evaluation"),
            Attempts[0].RejectionReason, EKalmalaPopulationSpawnRejectionReason::Caps);
        TestEqual(TEXT("Actor/save identity is derived from the configured source and finite slot"),
            Attempts[0].Descriptor.PersistentSpawnIdOverride,
            FKalmalaEnvironmentalSpawnerPolicy::GetStableSlotPersistentSpawnId(SourceId, 0));
    }
    WildlifeActorCount = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It) ++WildlifeActorCount;
    TestEqual(TEXT("A candidate rejected by its safety gates creates no wildlife actor"), WildlifeActorCount, 0);

    TestTrue(TEXT("Pending local-cap fixtures can be released after rejection"),
        Mode->ReleasePopulationCreatureReservation(LocalCapReservationA)
        && Mode->ReleasePopulationCreatureReservation(LocalCapReservationB));

    const FString SourceSlotId =
        FKalmalaEnvironmentalSpawnerPolicy::GetStableSlotPersistentSpawnId(SourceId, 0);
    Save->MarkDefeated(SourceSlotId);
    ConfiguredCheck.NextDueSeconds = 8.0;
    RemainingBudget = 16;
    Attempts.Reset();
    TestFalse(TEXT("A defeated finite source slot cannot select a replacement candidate"),
        Mode->TryCreateConfiguredEnvironmentalSpawnerCandidates(ConfiguredCheck, { Candidate },
            RemainingBudget, 8.0, CandidatesExamined, Attempts));
    TestEqual(TEXT("A permanently depleted slot examines no replacement work"), RemainingBudget, 16);
    TestTrue(TEXT("A defeated source slot emits no later attempt"), Attempts.IsEmpty());
    return true;
}

#endif
