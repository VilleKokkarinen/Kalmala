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

    TArray<FKalmalaWorldPopulationSpawn> Descriptors;
    for (int32 KeyY = -2; KeyY <= 2; ++KeyY)
    {
        for (int32 KeyX = -2; KeyX <= 2; ++KeyX)
        {
            const FIntPoint CandidateKey(HomeKey.X + KeyX, HomeKey.Y + KeyY);
            Mode->ActivePopulationSpatialKeys.Add(CandidateKey);
            Mode->PopulationKeyWildlifeActorBudgets.Add(CandidateKey, 20);
            Descriptors.Append(FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                Mode->WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife));
        }
    }
    if (!TestTrue(TEXT("A deterministic fixed wildlife candidate exists for the configured-source check"),
            !Descriptors.IsEmpty()))
    {
        return false;
    }
    FKalmalaWorldPopulationSpawn Candidate;
    EKalmalaWildlifeArchetype CandidateArchetype = EKalmalaWildlifeArchetype::Rat;
    bool bFoundCandidate = false;
    for (const FKalmalaWorldPopulationSpawn& PotentialCandidate : Descriptors)
    {
        EKalmalaWildlifeArchetype PotentialArchetype;
        if (!Mode->DoesPopulationDescriptorPassSpeciesHabitat(PotentialCandidate, &PotentialArchetype)
            || PotentialArchetype == EKalmalaWildlifeArchetype::Wolf
            || Mode->GetPopulationSpeciesAreaCap(PotentialArchetype) < 2
            || !Mode->DoesPopulationDescriptorPassCaps(PotentialCandidate)
            || !Mode->DoesPopulationDescriptorPassPhysicalPlacement(PotentialCandidate))
        {
            continue;
        }

        Candidate = PotentialCandidate;
        CandidateArchetype = PotentialArchetype;
        bFoundCandidate = true;
        break;
    }
    if (!TestTrue(TEXT("A non-pack candidate passes the current habitat, shared-cap and placement gates"),
            bFoundCandidate))
    {
        return false;
    }
    TestTrue(TEXT("The candidate has room under existing shared species and total caps"),
        Mode->DoesPopulationDescriptorPassCaps(Candidate));

    const TArray<FVector> NearbyCandidateOffsets = {
        FVector(1000.0, 0.0, 0.0), FVector(-1000.0, 0.0, 0.0),
        FVector(0.0, 1000.0, 0.0), FVector(0.0, -1000.0, 0.0),
        FVector(800.0, 800.0, 0.0), FVector(-800.0, 800.0, 0.0),
        FVector(800.0, -800.0, 0.0), FVector(-800.0, -800.0, 0.0)
    };
    FKalmalaWorldPopulationSpawn SecondCandidate;
    bool bFoundSecondCandidate = false;
    for (const FVector& Offset : NearbyCandidateOffsets)
    {
        FKalmalaWorldPopulationSpawn PotentialCandidate = Candidate;
        PotentialCandidate.Location += Offset;
        PotentialCandidate.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(
            FVector2D(PotentialCandidate.Location));
        Mode->ActivePopulationSpatialKeys.Add(PotentialCandidate.SpatialKey);
        Mode->PopulationKeyWildlifeActorBudgets.Add(PotentialCandidate.SpatialKey, 20);

        EKalmalaWildlifeArchetype PotentialArchetype;
        if (!FKalmalaEnvironmentalSpawnerPolicy::IsCandidateWithinSourceRadius(
                Candidate.Location, PotentialCandidate.Location)
            || !Mode->DoesPopulationDescriptorPassSpeciesHabitat(PotentialCandidate, &PotentialArchetype)
            || PotentialArchetype != CandidateArchetype
            || !Mode->DoesPopulationDescriptorPassCaps(PotentialCandidate)
            || !Mode->DoesPopulationDescriptorPassPhysicalPlacement(PotentialCandidate))
        {
            continue;
        }

        SecondCandidate = MoveTemp(PotentialCandidate);
        bFoundSecondCandidate = true;
        break;
    }
    if (!TestTrue(TEXT("A distinct same-species candidate fits within the configured source candidate radius"),
            bFoundSecondCandidate))
    {
        return false;
    }

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

    const FString LiveSourceId = TEXT("configured-live-source");
    const FString LiveSourceSlot0Id =
        FKalmalaEnvironmentalSpawnerPolicy::GetStableSlotPersistentSpawnId(LiveSourceId, 0);
    uint64 LiveSourceCheckOrdinal = 0;
    for (; LiveSourceCheckOrdinal < 256; ++LiveSourceCheckOrdinal)
    {
        if (FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
                Mode->WorldGenerationConfig, Candidate.SpatialKey, LiveSourceSlot0Id,
                LiveSourceCheckOrdinal)
            <= FKalmalaMonsterSpawnTuning::PopulationAdmissionRollMaxAccepted)
        {
            break;
        }
    }
    if (!TestTrue(TEXT("A deterministic check ordinal passes the live source slot's seeded admission roll"),
            LiveSourceCheckOrdinal < 256))
    {
        return false;
    }

    FKalmalaEnvironmentalSpawnerCheck LiveSourceCheck;
    LiveSourceCheck.bSourceConfigured = true;
    LiveSourceCheck.SourceId = LiveSourceId;
    LiveSourceCheck.SourceSlotIndex = 0;
    LiveSourceCheck.SourceOrigin = Candidate.Location;
    LiveSourceCheck.NextDueSeconds = 16.0;
    LiveSourceCheck.CheckOrdinal = LiveSourceCheckOrdinal;
    int32 LiveSourceRemainingBudget = 16;
    int32 LiveSourceCandidatesExamined = -1;
    TArray<FKalmalaPopulationSpawnAttempt> LiveSourceAttempts;
    if (!TestTrue(TEXT("A configured source successfully creates its first living actor"),
            Mode->TryCreateConfiguredEnvironmentalSpawnerCandidates(LiveSourceCheck, { Candidate },
                LiveSourceRemainingBudget, 16.0, LiveSourceCandidatesExamined, LiveSourceAttempts)))
    {
        return false;
    }
    TestEqual(TEXT("The successful live source check records one passing attempt"),
        LiveSourceAttempts.Num(), 1);
    TestEqual(TEXT("The successful live source check consumes one candidate budget slot"),
        LiveSourceRemainingBudget, 15);
    if (LiveSourceAttempts.Num() == 1)
    {
        TestEqual(TEXT("Successful source creation passes the final cap and placement gates"),
            LiveSourceAttempts[0].RejectionReason, EKalmalaPopulationSpawnRejectionReason::None);
        TestTrue(TEXT("Successful source creation preserves the configured source slot identity"),
            LiveSourceAttempts[0].Descriptor.PersistentSpawnIdOverride == LiveSourceSlot0Id);
    }

    AKalmalaWildlifeSpawn* FirstSourceCreature = nullptr;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
    {
        if (IsValid(*It) && (*It)->GetPersistentSpawnId() == LiveSourceSlot0Id)
        {
            FirstSourceCreature = *It;
            break;
        }
    }
    if (!TestNotNull(TEXT("The successful configured source actor is live"), FirstSourceCreature))
    {
        return false;
    }
    const FString* RegisteredLiveSourceId =
        Mode->EnvironmentalSpawnerSourceIdBySpawnId.Find(LiveSourceSlot0Id);
    TestTrue(TEXT("Successful source creation registers the actor under its configured source"),
        RegisteredLiveSourceId != nullptr && *RegisteredLiveSourceId == LiveSourceId);

    const FVector FirstSourceBoundaryLocation(
        Candidate.Location.X + FKalmalaEnvironmentalSpawnerPolicy::LocalLivingRadiusCentimetres,
        Candidate.Location.Y, FirstSourceCreature->GetActorLocation().Z);
    FirstSourceCreature->SetActorLocation(FirstSourceBoundaryLocation);

    const FString OtherSourceId = TEXT("configured-other-source");
    const FString OtherSourceSlotId =
        FKalmalaEnvironmentalSpawnerPolicy::GetStableSlotPersistentSpawnId(OtherSourceId, 0);
    FKalmalaWorldPopulationSpawn OtherSourceDescriptor = Candidate;
    OtherSourceDescriptor.Location = FVector(Candidate.Location.X,
        Candidate.Location.Y + 2000.0, FirstSourceCreature->GetActorLocation().Z);
    OtherSourceDescriptor.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(
        FVector2D(OtherSourceDescriptor.Location));
    OtherSourceDescriptor.PersistentSpawnIdOverride = OtherSourceSlotId;
    Mode->ActivePopulationSpatialKeys.Add(OtherSourceDescriptor.SpatialKey);
    Mode->PopulationKeyWildlifeActorBudgets.Add(OtherSourceDescriptor.SpatialKey, 20);
    FActorSpawnParameters OtherSourceSpawnParameters;
    OtherSourceSpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* OtherSourceCreature = World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), OtherSourceDescriptor.Location,
        FRotator::ZeroRotator, OtherSourceSpawnParameters);
    if (!TestNotNull(TEXT("A live actor owned by another configured source is available"),
            OtherSourceCreature))
    {
        return false;
    }
    OtherSourceCreature->InitializeServer(OtherSourceDescriptor, CandidateArchetype);
    Mode->RegisterLivePopulationCreature(OtherSourceCreature);
    Mode->EnvironmentalSpawnerSourceIdBySpawnId.Add(OtherSourceSlotId, OtherSourceId);
    const double OtherSourceDeltaX =
        OtherSourceCreature->GetActorLocation().X - Candidate.Location.X;
    const double OtherSourceDeltaY =
        OtherSourceCreature->GetActorLocation().Y - Candidate.Location.Y;
    TestTrue(TEXT("The other-source actor is inside this source's local-cap radius"),
        OtherSourceDeltaX * OtherSourceDeltaX + OtherSourceDeltaY * OtherSourceDeltaY
            <= FMath::Square(FKalmalaEnvironmentalSpawnerPolicy::LocalLivingRadiusCentimetres));

    const TArray<FKalmalaEnvironmentalSpawnerCreatureSample> LiveSourceSamples =
        Mode->GetEnvironmentalSpawnerLivingSamples(LiveSourceId);
    TestEqual(TEXT("The live adapter includes the source actor and excludes a nearby other-source actor"),
        LiveSourceSamples.Num(), 1);
    if (LiveSourceSamples.Num() == 1)
    {
        TestTrue(TEXT("The source sample uses the actor's current position at the inclusive 24 m boundary"),
            LiveSourceSamples[0].Location.Equals(FirstSourceCreature->GetActorLocation()));
    }
    int32 LiveSourceCount = -1;
    TestTrue(TEXT("The source policy can count the adapter's live samples"),
        FKalmalaEnvironmentalSpawnerPolicy::TryCountLivingOwnedWithinRadius(true, true,
            LiveSourceId, 1, Candidate.Location, LiveSourceSamples, LiveSourceCount));
    TestEqual(TEXT("The source-owned actor at exactly 24 m counts for local admission"),
        LiveSourceCount, 1);
    const TArray<FKalmalaEnvironmentalSpawnerCreatureSample> OtherSourceSamples =
        Mode->GetEnvironmentalSpawnerLivingSamples(OtherSourceId);
    TestEqual(TEXT("The nearby foreign actor remains registered to its own source"),
        OtherSourceSamples.Num(), 1);
    if (OtherSourceSamples.Num() == 1)
    {
        TestTrue(TEXT("The foreign-source adapter retains the other actor at its current position"),
            OtherSourceSamples[0].Location.Equals(OtherSourceCreature->GetActorLocation()));
    }

    const FString LiveSourceSlot1Id =
        FKalmalaEnvironmentalSpawnerPolicy::GetStableSlotPersistentSpawnId(LiveSourceId, 1);
    uint64 SecondLiveSourceCheckOrdinal = 0;
    for (; SecondLiveSourceCheckOrdinal < 256; ++SecondLiveSourceCheckOrdinal)
    {
        if (FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
                Mode->WorldGenerationConfig, SecondCandidate.SpatialKey, LiveSourceSlot1Id,
                SecondLiveSourceCheckOrdinal)
            <= FKalmalaMonsterSpawnTuning::PopulationAdmissionRollMaxAccepted)
        {
            break;
        }
    }
    if (!TestTrue(TEXT("A deterministic check ordinal passes the second live source slot's seeded admission roll"),
            SecondLiveSourceCheckOrdinal < 256))
    {
        return false;
    }

    FKalmalaPopulationSpawnAttempt SecondLiveSourceAttempt;
    SecondLiveSourceAttempt.Descriptor = SecondCandidate;
    SecondLiveSourceAttempt.bIsEnvironmentalSpawnerAttempt = true;
    SecondLiveSourceAttempt.bEnvironmentalSpawnerConfigured = true;
    SecondLiveSourceAttempt.EnvironmentalSpawnerSourceId = LiveSourceId;
    SecondLiveSourceAttempt.EnvironmentalSpawnerSlotIndex = 1;
    SecondLiveSourceAttempt.EnvironmentalSpawnerOrigin = Candidate.Location;
    TestTrue(TEXT("A second same-source candidate within 24 m sees the live actor and fits the remaining local slot"),
        Mode->DoesEnvironmentalSpawnerAttemptPassLocalCap(SecondLiveSourceAttempt, 1));

    FKalmalaEnvironmentalSpawnerCheck SecondLiveSourceCheck;
    SecondLiveSourceCheck.bSourceConfigured = true;
    SecondLiveSourceCheck.SourceId = LiveSourceId;
    SecondLiveSourceCheck.SourceSlotIndex = 1;
    SecondLiveSourceCheck.SourceOrigin = Candidate.Location;
    SecondLiveSourceCheck.NextDueSeconds = 24.0;
    SecondLiveSourceCheck.CheckOrdinal = SecondLiveSourceCheckOrdinal;
    int32 SecondLiveSourceRemainingBudget = 16;
    int32 SecondLiveSourceCandidatesExamined = -1;
    TArray<FKalmalaPopulationSpawnAttempt> SecondLiveSourceAttempts;
    if (!TestTrue(TEXT("A second same-source candidate creates within the inclusive local radius"),
            Mode->TryCreateConfiguredEnvironmentalSpawnerCandidates(SecondLiveSourceCheck,
                { SecondCandidate }, SecondLiveSourceRemainingBudget, 24.0,
                SecondLiveSourceCandidatesExamined, SecondLiveSourceAttempts)))
    {
        return false;
    }
    TestEqual(TEXT("The second same-source candidate records one passing attempt"),
        SecondLiveSourceAttempts.Num(), 1);
    TestEqual(TEXT("The second same-source candidate consumes one candidate budget slot"),
        SecondLiveSourceRemainingBudget, 15);
    const FString* RegisteredSecondSourceId =
        Mode->EnvironmentalSpawnerSourceIdBySpawnId.Find(LiveSourceSlot1Id);
    TestTrue(TEXT("The second live actor is registered to the same configured source"),
        RegisteredSecondSourceId != nullptr && *RegisteredSecondSourceId == LiveSourceId);

    AKalmalaWildlifeSpawn* SecondSourceCreature = nullptr;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
    {
        if (IsValid(*It) && (*It)->GetPersistentSpawnId() == LiveSourceSlot1Id)
        {
            SecondSourceCreature = *It;
            break;
        }
    }
    if (!TestNotNull(TEXT("The second configured source actor is live"), SecondSourceCreature))
    {
        return false;
    }

    const TArray<FKalmalaEnvironmentalSpawnerCreatureSample> LiveSourceSamplesAfterSecondCreation =
        Mode->GetEnvironmentalSpawnerLivingSamples(LiveSourceId);
    TestEqual(TEXT("Same-source sampling retains both live actors and excludes the nearby foreign actor"),
        LiveSourceSamplesAfterSecondCreation.Num(), 2);
    bool bFirstSourceActorWasSampled = false;
    bool bSecondSourceActorWasSampled = false;
    for (const FKalmalaEnvironmentalSpawnerCreatureSample& Sample : LiveSourceSamplesAfterSecondCreation)
    {
        bFirstSourceActorWasSampled = bFirstSourceActorWasSampled
            || Sample.Location.Equals(FirstSourceCreature->GetActorLocation());
        bSecondSourceActorWasSampled = bSecondSourceActorWasSampled
            || Sample.Location.Equals(SecondSourceCreature->GetActorLocation());
    }
    TestTrue(TEXT("The source adapter still includes its first live actor"), bFirstSourceActorWasSampled);
    TestTrue(TEXT("The source adapter includes the newly created second live actor"),
        bSecondSourceActorWasSampled);
    int32 LiveSourceCountAfterSecondCreation = -1;
    TestTrue(TEXT("The second candidate's live registry state remains countable"),
        FKalmalaEnvironmentalSpawnerPolicy::TryCountLivingOwnedWithinRadius(true, true,
            LiveSourceId, 1, Candidate.Location, LiveSourceSamplesAfterSecondCreation,
            LiveSourceCountAfterSecondCreation));
    TestEqual(TEXT("Both same-source living actors count inside the inclusive 24 m radius"),
        LiveSourceCountAfterSecondCreation, 2);
    return true;
}

#endif
