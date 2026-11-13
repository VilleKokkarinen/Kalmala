#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaGameMode.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "KalmalaWolfPackContract.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationCreationTest,
    "Kalmala.Gameplay.Population.CreationLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationCreationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population creation fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population creation GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig.WorldSeed = 418;

    UKalmalaWorldPopulationSaveGame* Save = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Population save fixture created"), Save)) return false;
    Save->InitializeForWorld(Mode->WorldGenerationConfig);
    Mode->PopulationSaveGame = Save;

    FKalmalaWorldPopulationSpawn Descriptor;
    Descriptor.Kind = EKalmalaWorldPopulationKind::Wildlife;
    Descriptor.SpatialKey = FIntPoint(-1, 2);
    Descriptor.SpawnSeed = 987654321;
    Descriptor.Location = FVector(-6400.0, 12800.0, 0.0);
    const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptor);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), Descriptor.Location,
        FRotator::ZeroRotator, SpawnParameters);
    if (!TestNotNull(TEXT("Living wildlife fixture created"), Creature)) return false;
    Creature->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::Rat);
    Mode->RegisterLivePopulationCreature(Creature);
    TestTrue(TEXT("An initialized stable spawn ID is present in the live registry"),
        Mode->HasLivePopulationCreatureWithSpawnId(PersistentSpawnId));

    FKalmalaPopulationSpawnAttempt RejectedAttempt;
    RejectedAttempt.Descriptor = Descriptor;
    int32 WildlifeCountBeforeRejectedAttempt = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
    {
        ++WildlifeCountBeforeRejectedAttempt;
    }
    TestFalse(TEXT("A descriptor without all nine passing gate results cannot create an actor"),
        Mode->TryCreatePopulationSpawnTransaction(RejectedAttempt, 10.0));
    int32 WildlifeCountAfterRejectedAttempt = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
    {
        ++WildlifeCountAfterRejectedAttempt;
    }
    TestEqual(TEXT("Rejected attempts leave the actor population unchanged"),
        WildlifeCountAfterRejectedAttempt, WildlifeCountBeforeRejectedAttempt);

    FActorSpawnParameters PlayerSpawnParameters;
    PlayerSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaCharacter* FirstPlayer = World->SpawnActor<AKalmalaCharacter>(
        Descriptor.Location + FVector(FKalmalaMonsterSpawnTuning::CountRadiusCm * 2.0, 0.0, 0.0),
        FRotator::ZeroRotator, PlayerSpawnParameters);
    AKalmalaCharacter* SecondPlayer = World->SpawnActor<AKalmalaCharacter>(
        Descriptor.Location + FVector(FKalmalaMonsterSpawnTuning::CountRadiusCm, 0.0, 0.0),
        FRotator::ZeroRotator, PlayerSpawnParameters);
    if (!TestNotNull(TEXT("First living player fixture created"), FirstPlayer)
        || !TestNotNull(TEXT("Second living player fixture created"), SecondPlayer)) return false;
    const TArray<AKalmalaCharacter*> LivingPlayers = { FirstPlayer, SecondPlayer };

    Mode->UpdatePopulationCreatureUnloads(10.0, LivingPlayers);
    TestTrue(TEXT("A second living player at exactly 96 m keeps the creature loaded while the first is farther away"),
        IsValid(Creature));
    TestFalse(TEXT("A living player at the exact 96 m boundary does not start an unload cooldown"),
        Mode->PopulationCreatureReactivationDueTimes.Contains(PersistentSpawnId));

    FirstPlayer->SetActorLocation(Descriptor.Location + FVector(
        FKalmalaMonsterSpawnTuning::CountRadiusCm * 2.0 + 1.0, 0.0, 0.0));
    SecondPlayer->SetActorLocation(Descriptor.Location + FVector(
        FKalmalaMonsterSpawnTuning::CountRadiusCm + 1.0, 0.0, 0.0));
    Mode->UpdatePopulationCreatureUnloads(11.0, LivingPlayers);
    TestFalse(TEXT("A creature unloads after both living players are outside the 96 m radius"),
        IsValid(Creature));
    TestFalse(TEXT("Unloaded wildlife is removed from the live registry"),
        Mode->HasLivePopulationCreatureWithSpawnId(PersistentSpawnId));
    TestEqual(TEXT("Unload preserves the stable slot through its 30-second cooldown"),
        Mode->PopulationCreatureReactivationDueTimes.FindRef(PersistentSpawnId), 41.0);
    TestFalse(TEXT("Unload does not mark the persistent slot defeated"),
        Save->IsDefeated(PersistentSpawnId));
    TestFalse(TEXT("Reactivation is blocked immediately before cooldown expiry"),
        Mode->IsPopulationSpawnReactivationReady(PersistentSpawnId, 40.999));
    TestTrue(TEXT("Reactivation is allowed at the inclusive cooldown expiry"),
        Mode->IsPopulationSpawnReactivationReady(PersistentSpawnId, 41.0));

    FActorSpawnParameters ReloadParameters;
    ReloadParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* ReloadedCreature = World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), Descriptor.Location,
        FRotator::ZeroRotator, ReloadParameters);
    if (!TestNotNull(TEXT("The cooled-down descriptor can be reloaded"), ReloadedCreature)) return false;
    ReloadedCreature->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::Rat);
    Mode->RegisterLivePopulationCreature(ReloadedCreature);
    TestEqual(TEXT("Reload preserves the descriptor's stable spawn identity"),
        ReloadedCreature->GetPersistentSpawnId(), PersistentSpawnId);

    FirstPlayer->SetActorLocation(Descriptor.Location + FVector(
        FKalmalaMonsterSpawnTuning::CountRadiusCm, 0.0, 0.0));
    Mode->UpdatePopulationCreatureUnloads(41.0, LivingPlayers);
    TestTrue(TEXT("The reloaded living actor remains tracked at the inclusive player radius"),
        IsValid(ReloadedCreature)
        && Mode->HasLivePopulationCreatureWithSpawnId(PersistentSpawnId));
    TestFalse(TEXT("Cooldown state clears after the same slot is reactivated"),
        Mode->PopulationCreatureReactivationDueTimes.Contains(PersistentSpawnId));
    TestFalse(TEXT("Reload does not turn an intentional unload into a defeat"),
        Save->IsDefeated(PersistentSpawnId));

    const auto PassingGates = []()
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
        Results.bPhysicalPlacementPassed = true;
        return Results;
    };
    const auto CountWildlifeActors = [&World]()
    {
        int32 Count = 0;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It) ++Count;
        return Count;
    };
    const auto FindCreatureById = [&World](const FString& SpawnId) -> AKalmalaWildlifeSpawn*
    {
        for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
        {
            if (It->GetPersistentSpawnId() == SpawnId) return *It;
        }
        return nullptr;
    };
    const auto DoesCandidatePassCaps = [Mode](const FKalmalaWorldPopulationSpawn& Candidate)
    {
        const bool bWasActive = Mode->ActivePopulationSpatialKeys.Contains(Candidate.SpatialKey);
        const int32* ExistingBudget = Mode->PopulationKeyWildlifeActorBudgets.Find(Candidate.SpatialKey);
        const bool bHadBudget = ExistingBudget != nullptr;
        const int32 PreviousBudget = bHadBudget ? *ExistingBudget : 0;
        Mode->ActivePopulationSpatialKeys.Add(Candidate.SpatialKey);
        Mode->PopulationKeyWildlifeActorBudgets.Add(Candidate.SpatialKey, 20);
        const bool bPasses = Mode->DoesPopulationDescriptorPassCaps(Candidate);
        if (!bPasses)
        {
            if (!bWasActive) Mode->ActivePopulationSpatialKeys.Remove(Candidate.SpatialKey);
            if (bHadBudget) Mode->PopulationKeyWildlifeActorBudgets.Add(Candidate.SpatialKey, PreviousBudget);
            else Mode->PopulationKeyWildlifeActorBudgets.Remove(Candidate.SpatialKey);
        }
        return bPasses;
    };

    FKalmalaWorldPopulationSpawn NaturalSingleDescriptor;
    bool bFoundNaturalSingle = false;
    const FIntPoint SearchCenter = Descriptor.SpatialKey;
    for (int32 KeyY = SearchCenter.Y - 4; KeyY <= SearchCenter.Y + 4 && !bFoundNaturalSingle; ++KeyY)
    {
        for (int32 KeyX = SearchCenter.X - 4; KeyX <= SearchCenter.X + 4 && !bFoundNaturalSingle; ++KeyX)
        {
            const FIntPoint CandidateKey(KeyX, KeyY);
            const TArray<FKalmalaWorldPopulationSpawn> Candidates =
                FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                    Mode->WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
            for (const FKalmalaWorldPopulationSpawn& Candidate : Candidates)
            {
                EKalmalaWildlifeArchetype CandidateArchetype;
                if (!Mode->DoesPopulationDescriptorPassSpeciesHabitat(Candidate, &CandidateArchetype)
                    || CandidateArchetype == EKalmalaWildlifeArchetype::Wolf
                    || CandidateArchetype == EKalmalaWildlifeArchetype::ReefMaw
                    || Mode->GetPopulationSpeciesAreaCap(CandidateArchetype) < 2
                    || FVector::Dist2D(Candidate.Location, ReloadedCreature->GetActorLocation())
                        <= FKalmalaMonsterSpawnTuning::CountRadiusCm
                    || !Mode->DoesPopulationDescriptorPassPhysicalPlacement(Candidate))
                {
                    continue;
                }

                if (!DoesCandidatePassCaps(Candidate)) continue;
                NaturalSingleDescriptor = Candidate;
                bFoundNaturalSingle = true;
                break;
            }
        }
    }
    if (!TestTrue(TEXT("A generated non-pack wildlife descriptor passes natural habitat, placement, and capacity checks"),
            bFoundNaturalSingle))
    {
        return false;
    }

    FKalmalaWorldPopulationSpawn NaturalWolfDescriptor;
    bool bFoundNaturalWolf = false;
    constexpr int32 SearchSpacingCentimetres = 25000;
    const int32 SearchExtentCentimetres = static_cast<int32>(FKalmalaWorldBounds::Radius);
    for (int32 Y = -SearchExtentCentimetres + SearchSpacingCentimetres;
        Y < SearchExtentCentimetres && !bFoundNaturalWolf; Y += SearchSpacingCentimetres)
    {
        for (int32 X = -SearchExtentCentimetres + SearchSpacingCentimetres;
            X < SearchExtentCentimetres && !bFoundNaturalWolf; X += SearchSpacingCentimetres)
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
            EKalmalaWildlifeArchetype CandidateArchetype;
            if (!Mode->DoesPopulationDescriptorPassSpeciesHabitat(Candidate, &CandidateArchetype)
                || CandidateArchetype != EKalmalaWildlifeArchetype::Wolf
                || FVector::Dist2D(Candidate.Location, NaturalSingleDescriptor.Location)
                    <= FKalmalaMonsterSpawnTuning::CountRadiusCm * 2.0f
                || !Mode->DoesPopulationDescriptorPassPhysicalPlacement(Candidate))
            {
                continue;
            }

            if (!DoesCandidatePassCaps(Candidate)) continue;
            NaturalWolfDescriptor = Candidate;
            bFoundNaturalWolf = true;
            break;
        }
    }
    if (!TestTrue(TEXT("A valid tundra location supplies a physically valid natural Wolf pack descriptor"),
            bFoundNaturalWolf))
    {
        return false;
    }

    int32 PopulationSaveWriteCalls = 0;
    Mode->PopulationSaveWriterForTests = [&PopulationSaveWriteCalls](
        UKalmalaWorldPopulationSaveGame*, const FString&, int32)
    {
        ++PopulationSaveWriteCalls;
    };

    FKalmalaPopulationSpawnAttempt NaturalSingleAttempt;
    NaturalSingleAttempt.Descriptor = NaturalSingleDescriptor;
    NaturalSingleAttempt.GateResults = PassingGates();
    const int32 WildlifeBeforeSingle = CountWildlifeActors();
    TestFalse(TEXT("The single-actor transaction uses the natural population path"),
        NaturalSingleAttempt.bIsEnvironmentalSpawnerAttempt);
    TestTrue(TEXT("A natural single-actor transaction succeeds"),
        Mode->TryCreatePopulationSpawnTransaction(NaturalSingleAttempt, 50.0));
    TestEqual(TEXT("Natural single creation leaves no pending capacity reservation"),
        Mode->PendingPopulationCreatureReservations.Num(), 0);
    TestEqual(TEXT("Natural single creation publishes one initialized wildlife actor"),
        CountWildlifeActors(), WildlifeBeforeSingle + 1);
    const FString NaturalSingleId =
        FKalmalaWorldPopulationLayout::GetPersistentSpawnId(NaturalSingleDescriptor);
    AKalmalaWildlifeSpawn* NaturalSingleCreature = FindCreatureById(NaturalSingleId);
    TestNotNull(TEXT("The natural single stable ID is present in the world"), NaturalSingleCreature);
    TestTrue(TEXT("Natural single creation registers a live actor and its defeat callback"),
        IsValid(NaturalSingleCreature)
        && NaturalSingleCreature->GetArchetype() != EKalmalaWildlifeArchetype::Wolf
        && !NaturalSingleCreature->IsDefeated()
        && NaturalSingleCreature->GetHealth() > 0.0f
        && Mode->HasLivePopulationCreatureWithSpawnId(NaturalSingleId)
        && NaturalSingleCreature->OnDefeated.IsBoundToObject(Mode));
    TestFalse(TEXT("Natural single creation does not publish environmental source ownership"),
        Mode->EnvironmentalSpawnerSourceIdBySpawnId.Contains(NaturalSingleId));

    TArray<FString> WolfMemberIds;
    for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
    {
        WolfMemberIds.Add(FKalmalaWolfPackContract::GetMemberPersistentSpawnId(
            NaturalWolfDescriptor, MemberIndex));
    }
    const int32 WildlifeBeforeWolfFailure = CountWildlifeActors();
    const int32 LiveRegistryBeforeWolfFailure = Mode->LivePopulationCreatures.Num();
    const int32 PendingRewardsBeforeWolfFailure = Save->GetPendingWildlifeRewards().Num();
    int32 SpawnGateCalls = 0;
    int32 RolledBackWolfDefeatCallbacks = 0;
    AKalmalaWildlifeSpawn* FirstWolfCreatedDuringFailure = nullptr;
    Mode->PopulationActorSpawnAllowedForTests = [&World, &WolfMemberIds, &SpawnGateCalls,
        &RolledBackWolfDefeatCallbacks, &FirstWolfCreatedDuringFailure](const int32 MemberIndex)
    {
        ++SpawnGateCalls;
        if (MemberIndex == 1)
        {
            for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
            {
                if (It->GetPersistentSpawnId() == WolfMemberIds[0])
                {
                    FirstWolfCreatedDuringFailure = *It;
                    break;
                }
            }
            if (IsValid(FirstWolfCreatedDuringFailure))
            {
                FirstWolfCreatedDuringFailure->OnDefeated.AddLambda(
                    [&RolledBackWolfDefeatCallbacks](const FString&) { ++RolledBackWolfDefeatCallbacks; });
            }
            return false;
        }
        return true;
    };
    FKalmalaPopulationSpawnAttempt FailedWolfAttempt;
    FailedWolfAttempt.Descriptor = NaturalWolfDescriptor;
    FailedWolfAttempt.GateResults = PassingGates();
    TestFalse(TEXT("An injected second-member failure rolls back the partial Wolf transaction"),
        Mode->TryCreatePopulationSpawnTransaction(FailedWolfAttempt, 60.0));
    Mode->PopulationActorSpawnAllowedForTests = nullptr;
    TestEqual(TEXT("The failure is injected only after the first Wolf member was created"),
        SpawnGateCalls, 2);
    TestNotNull(TEXT("The first Wolf member existed before the later-member failure"),
        FirstWolfCreatedDuringFailure);
    TestFalse(TEXT("The partial Wolf actor is destroyed during rollback"),
        IsValid(FirstWolfCreatedDuringFailure));
    TestEqual(TEXT("Rollback never broadcasts the defeated callback"),
        RolledBackWolfDefeatCallbacks, 0);
    TestEqual(TEXT("Wolf rollback leaves no additional wildlife actors"),
        CountWildlifeActors(), WildlifeBeforeWolfFailure);
    TestEqual(TEXT("Wolf rollback removes every new live registry entry"),
        Mode->LivePopulationCreatures.Num(), LiveRegistryBeforeWolfFailure);
    TestEqual(TEXT("Wolf rollback releases its full capacity reservation"),
        Mode->PendingPopulationCreatureReservations.Num(), 0);
    for (const FString& WolfMemberId : WolfMemberIds)
    {
        TestFalse(FString::Printf(TEXT("Wolf rollback does not mark %s defeated"), *WolfMemberId),
            Save->IsDefeated(WolfMemberId));
        TestFalse(FString::Printf(TEXT("Wolf rollback leaves %s out of the live registry"), *WolfMemberId),
            Mode->HasLivePopulationCreatureWithSpawnId(WolfMemberId));
        TestFalse(FString::Printf(TEXT("Wolf rollback publishes no source owner for %s"), *WolfMemberId),
            Mode->EnvironmentalSpawnerSourceIdBySpawnId.Contains(WolfMemberId));
    }
    TestEqual(TEXT("Wolf rollback queues no rewards"),
        Save->GetPendingWildlifeRewards().Num(), PendingRewardsBeforeWolfFailure);
    TestEqual(TEXT("Wolf rollback performs no defeat save callback"),
        PopulationSaveWriteCalls, 0);

    FKalmalaPopulationSpawnAttempt NaturalWolfAttempt;
    NaturalWolfAttempt.Descriptor = NaturalWolfDescriptor;
    NaturalWolfAttempt.GateResults = PassingGates();
    const int32 WildlifeBeforeWolf = CountWildlifeActors();
    TestFalse(TEXT("The Wolf transaction also uses the natural population path"),
        NaturalWolfAttempt.bIsEnvironmentalSpawnerAttempt);
    TestTrue(TEXT("A natural Wolf pack transaction succeeds after the failed attempt is rolled back"),
        Mode->TryCreatePopulationSpawnTransaction(NaturalWolfAttempt, 70.0));
    TestEqual(TEXT("Wolf pack creation publishes all three initialized wildlife actors"),
        CountWildlifeActors(), WildlifeBeforeWolf + FKalmalaWolfPackContract::MemberCount);
    TestEqual(TEXT("Wolf pack creation releases its complete capacity reservation"),
        Mode->PendingPopulationCreatureReservations.Num(), 0);
    for (const FString& WolfMemberId : WolfMemberIds)
    {
        AKalmalaWildlifeSpawn* WolfMember = FindCreatureById(WolfMemberId);
        TestTrue(FString::Printf(TEXT("Wolf member %s is initialized, live, and defeat-bound"), *WolfMemberId),
            IsValid(WolfMember)
            && WolfMember->GetArchetype() == EKalmalaWildlifeArchetype::Wolf
            && !WolfMember->IsDefeated() && WolfMember->GetHealth() > 0.0f
            && Mode->HasLivePopulationCreatureWithSpawnId(WolfMemberId)
            && WolfMember->OnDefeated.IsBoundToObject(Mode));
        TestFalse(FString::Printf(TEXT("Natural Wolf member %s has no environmental source owner"), *WolfMemberId),
            Mode->EnvironmentalSpawnerSourceIdBySpawnId.Contains(WolfMemberId));
    }
    TestEqual(TEXT("Successful creation attempts do not mark generated wildlife defeated"),
        Save->GetPendingWildlifeRewards().Num(), PendingRewardsBeforeWolfFailure);
    TestEqual(TEXT("Successful creation itself does not invoke the defeat save callback"),
        PopulationSaveWriteCalls, 0);
    return true;
}

#endif
