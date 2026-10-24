#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldPopulationLayout.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationRingCandidateSelectorTest,
    "Kalmala.Gameplay.Population.RingCandidateSelector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationRingCandidateSelectorTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population ring fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population ring fixture GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig.WorldSeed = 1;

    APlayerController* Controller = World->SpawnActor<APlayerController>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    FActorSpawnParameters PawnParameters;
    PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaCharacter* Character = World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
        FVector::ZeroVector, FRotator::ZeroRotator, PawnParameters);
    if (PlayerState != nullptr) PlayerState->SetPlayerId(1);
    if (Controller != nullptr && PlayerState != nullptr) Controller->SetPlayerState(PlayerState);
    if (Controller != nullptr && Character != nullptr)
    {
        Controller->Possess(Character);
        World->AddController(Controller);
    }
    if (!TestTrue(TEXT("Living player fixture is fully possessed"), IsValid(Controller)
        && IsValid(PlayerState) && IsValid(Character) && Character->GetController() == Controller))
    {
        return false;
    }
    const TArray<AKalmalaCharacter*> LivingPlayers = Mode->GetLivingPlayerPawnsForPopulation();
    if (!TestTrue(TEXT("The selector receives the authoritative living player"),
        LivingPlayers.Num() == 1 && LivingPlayers[0] == Character))
    {
        return false;
    }

    const FIntPoint Key(0, 0);
    TArray<FKalmalaWorldPopulationSpawn> Descriptors;
    const auto AddDescriptor = [&Descriptors, Key](const uint64 Seed, const FVector& Location, const FName ContentId)
    {
        FKalmalaWorldPopulationSpawn& Spawn = Descriptors.AddDefaulted_GetRef();
        Spawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Spawn.SpatialKey = Key;
        Spawn.SpawnSeed = Seed;
        Spawn.Location = Location;
        Spawn.ContentId = ContentId;
    };
    AddDescriptor(1001, FVector(3200.0, 0.0, 0.0), FName(TEXT("meadow-creature-a")));
    AddDescriptor(1002, FVector(5000.0, 0.0, 0.0), FName(TEXT("meadow-creature-b")));
    AddDescriptor(1003, FVector(9600.0, 0.0, 0.0), FName(TEXT("meadow-creature-c")));
    AddDescriptor(1005, FVector(9601.0, 0.0, 0.0), FName(TEXT("meadow-creature-d")));
    AddDescriptor(1008, FVector(3199.0, 0.0, 0.0), FName(TEXT("meadow-creature-e")));
    AddDescriptor(1006, FVector(5000.0, 100.0, 0.0), FName(TEXT("meadow-creature-f")));

    const uint8 Roll1001 = FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
        Mode->WorldGenerationConfig, Key, FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptors[0]), 7);
    const uint8 RepeatRoll1001 = FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
        Mode->WorldGenerationConfig, Key, FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptors[0]), 7);
    const uint8 Roll1005 = FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
        Mode->WorldGenerationConfig, Key, FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptors[3]), 7);
    const uint8 Roll1008 = FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
        Mode->WorldGenerationConfig, Key, FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptors[4]), 8);
    const uint8 Roll1006 = FKalmalaMonsterSpawnTuning::GetPopulationSpawnAdmissionRoll(
        Mode->WorldGenerationConfig, Key, FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptors[5]), 8);
    TestEqual(TEXT("The dedicated seed returns a roll in the 0–99 range"), Roll1001, uint8(18));
    TestEqual(TEXT("The same world, key, ID and check ordinal repeat the roll"), RepeatRoll1001, Roll1001);
    TestEqual(TEXT("The outside-radius descriptor has an accepted roll for the ring test"), Roll1005, uint8(14));
    TestEqual(TEXT("The inside-boundary descriptor has an accepted roll for the ring test"), Roll1008, uint8(1));
    TestTrue(TEXT("A passing ring descriptor can still be rejected by its seeded roll"), Roll1006 > 24);

    FKalmalaPopulationSchedulerKeyWork Work;
    Work.SpatialKey = Key;
    Work.CandidateBudget = 4;
    Work.CheckOrdinal = 7;
    Mode->SelectPopulationRingCandidates(Work, Descriptors, LivingPlayers);
    TestTrue(TEXT("Selection stays within the four-descriptor scheduler budget"), Work.Candidates.Num() == 3);
    TestTrue(TEXT("The inclusive 32 m and 96 m ring boundaries pass, while the next centimetre outside fails"),
        Work.Candidates.Num() == 3
        && Work.Candidates[0].SpawnSeed == 1001
        && Work.Candidates[1].SpawnSeed == 1002
        && Work.Candidates[2].SpawnSeed == 1003);
    TestTrue(TEXT("Selected descriptors keep original identity, location and content ID"),
        Work.Candidates[0].SpawnSeed == Descriptors[0].SpawnSeed
        && Work.Candidates[0].Location == Descriptors[0].Location
        && Work.Candidates[0].ContentId == Descriptors[0].ContentId
        && FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Work.Candidates[0])
            == FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptors[0])
        && Work.Candidates[2].SpawnSeed == Descriptors[2].SpawnSeed
        && Work.Candidates[2].Location == Descriptors[2].Location
        && Work.Candidates[2].ContentId == Descriptors[2].ContentId);
    TestEqual(TEXT("The cursor advances across accepted and rejected descriptors"),
        Mode->PopulationKeyCandidateCursors.FindRef(Key), 4);

    Work.CandidateBudget = 2;
    Work.CheckOrdinal = 8;
    Mode->SelectPopulationRingCandidates(Work, Descriptors, LivingPlayers);
    TestTrue(TEXT("A passing roll inside 32 m and a rejected roll yield no candidate"),
        Work.Candidates.IsEmpty());
    TestEqual(TEXT("The cursor advances even when every examined descriptor is rejected"),
        Mode->PopulationKeyCandidateCursors.FindRef(Key), 0);

    Mode->ActivePopulationSchedulerKeys.Add(Key);
    Mode->PopulationKeyNextDueTimes.Add(Key, 0.0);
    Mode->NextPopulationSchedulerWakeTime = 0.0;
    Mode->UpdatePopulationSchedulerWake(10.0);
    TestTrue(TEXT("One serviced key receives one check ordinal"),
        Mode->PopulationSchedulerWakeWork.Num() == 1
        && Mode->PopulationSchedulerWakeWork[0].CheckOrdinal == 0
        && Mode->PopulationKeyServicedCheckOrdinals.FindRef(Key) == 1);
    Mode->UpdatePopulationSchedulerWake(10.0);
    TestTrue(TEXT("A repeated call before the next wake does not advance the ordinal"),
        Mode->PopulationSchedulerWakeWork.IsEmpty()
        && Mode->PopulationKeyServicedCheckOrdinals.FindRef(Key) == 1);
    Mode->UpdatePopulationKeySchedule(10.1);
    TestTrue(TEXT("An inactive key releases its cursor and roll ordinal state"),
        !Mode->ActivePopulationSchedulerKeys.Contains(Key)
        && !Mode->PopulationKeyNextDueTimes.Contains(Key)
        && !Mode->PopulationKeyCandidateCursors.Contains(Key)
        && !Mode->PopulationKeyServicedCheckOrdinals.Contains(Key));
    return true;
}

#endif
