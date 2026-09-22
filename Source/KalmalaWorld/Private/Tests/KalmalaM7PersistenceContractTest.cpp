#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaM7PersistenceContract.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM7PersistenceContractTest,
    "Kalmala.World.M7.PersistenceContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM7PersistenceContractTest::RunTest(const FString& Parameters)
{
    const FKalmalaM7SaveIdentity WorldIdentity = FKalmalaM7SaveIdentity::ForWorld(418);
    UKalmalaM7PersistenceSaveGame* SaveGame = NewObject<UKalmalaM7PersistenceSaveGame>();
    SaveGame->Initialize(WorldIdentity);

    TestTrue(TEXT("The current generator revision is part of a valid world identity"), WorldIdentity.IsValid());
    TestTrue(TEXT("A new M7 container matches its initialized world identity"), SaveGame->Matches(WorldIdentity));
    TestEqual(
        TEXT("The current schema is accepted without migration"),
        static_cast<uint8>(UKalmalaM7PersistenceSaveGame::EvaluateSchemaVersion(UKalmalaM7PersistenceSaveGame::CurrentSchemaVersion)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::AcceptCurrent));
    TestEqual(
        TEXT("An unversioned legacy record requires an explicit migration"),
        static_cast<uint8>(UKalmalaM7PersistenceSaveGame::EvaluateSchemaVersion(0)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::MigrateBeforeLoad));
    TestEqual(
        TEXT("A future schema is rejected rather than guessed"),
        static_cast<uint8>(UKalmalaM7PersistenceSaveGame::EvaluateSchemaVersion(UKalmalaM7PersistenceSaveGame::CurrentSchemaVersion + 1)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::Reject));

    const FKalmalaM7SparseDelta ResourceDelta{ EKalmalaM7SparseDeltaKind::ResourceDepleted, TEXT("resource:meadow/1,1/0") };
    const FKalmalaM7SparseDelta CreatureDelta{ EKalmalaM7SparseDeltaKind::CreatureDefeated, TEXT("creature:0/1/-1/5678") };
    const FKalmalaM7SparseDelta DiscoveryDelta{ EKalmalaM7SparseDeltaKind::DiscoveryClaimed, TEXT("discovery:scroll/1,1/0") };
    TestTrue(TEXT("The gate accepts a server-selected resource depletion identity"), SaveGame->AddSparseDelta(ResourceDelta));
    TestTrue(TEXT("The gate accepts a server-selected creature defeat identity"), SaveGame->AddSparseDelta(CreatureDelta));
    TestTrue(TEXT("The gate accepts a server-selected discovery identity"), SaveGame->AddSparseDelta(DiscoveryDelta));
    TestFalse(TEXT("The gate rejects a duplicate sparse delta without mutation"), SaveGame->AddSparseDelta(ResourceDelta));
    TestFalse(TEXT("The gate rejects a blank sparse identity"), SaveGame->AddSparseDelta({ EKalmalaM7SparseDeltaKind::ResourceDepleted, TEXT("") }));
    TestFalse(TEXT("The gate rejects path-like sparse identities"), SaveGame->AddSparseDelta({ EKalmalaM7SparseDeltaKind::ResourceDepleted, TEXT("../foreign-save") }));
    TestTrue(TEXT("The gate retains each accepted sparse fact"), SaveGame->HasSparseDelta(ResourceDelta.Kind, ResourceDelta.StableId)
        && SaveGame->HasSparseDelta(CreatureDelta.Kind, CreatureDelta.StableId)
        && SaveGame->HasSparseDelta(DiscoveryDelta.Kind, DiscoveryDelta.StableId));

    TArray<uint8> SerializedSave;
    TestTrue(TEXT("The M7 gate serializes without writing a project save slot"), UGameplayStatics::SaveGameToMemory(SaveGame, SerializedSave));
    UKalmalaM7PersistenceSaveGame* ReloadedSave = Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(SerializedSave));
    if (TestNotNull(TEXT("The M7 gate reloads as its expected type"), ReloadedSave))
    {
        TestTrue(TEXT("The reloaded gate retains exact world and generator identity"), ReloadedSave->Matches(WorldIdentity));
        TestEqual(TEXT("The reloaded gate retains only the accepted sparse facts"), ReloadedSave->GetSparseDeltaCount(), 3);
        TestTrue(TEXT("The reloaded gate retains the resource depletion fact"), ReloadedSave->HasSparseDelta(ResourceDelta.Kind, ResourceDelta.StableId));
        TestTrue(TEXT("The reloaded gate retains the creature defeat fact"), ReloadedSave->HasSparseDelta(CreatureDelta.Kind, CreatureDelta.StableId));
        TestTrue(TEXT("The reloaded gate retains the discovery claim"), ReloadedSave->HasSparseDelta(DiscoveryDelta.Kind, DiscoveryDelta.StableId));
    }

    FKalmalaM7SaveIdentity DifferentWorld = WorldIdentity;
    DifferentWorld.WorldSeed = 419;
    TestFalse(TEXT("A different world seed cannot reuse M7 state"), SaveGame->Matches(DifferentWorld));
    DifferentWorld = WorldIdentity;
    DifferentWorld.GeneratorRevision = FKalmalaM7SaveIdentity::CurrentGeneratorRevision + 1;
    TestFalse(TEXT("A different generator revision cannot reuse M7 state"), SaveGame->Matches(DifferentWorld));

    const FKalmalaM7SaveIdentity PlayerIdentity = FKalmalaM7SaveIdentity::ForPlayer(418, TEXT("player-alpha"));
    const FKalmalaM7SaveIdentity OtherPlayerIdentity = FKalmalaM7SaveIdentity::ForPlayer(418, TEXT("player-beta"));
    TestTrue(TEXT("A player identity is valid only in player scope"), PlayerIdentity.IsValid());
    TestFalse(TEXT("Player-scoped state does not match another owner"), PlayerIdentity.Matches(OtherPlayerIdentity));
    TestFalse(TEXT("A player identity rejects whitespace that could corrupt slot identity"), FKalmalaM7SaveIdentity::ForPlayer(418, TEXT("player alpha")).IsValid());

    return true;
}

#endif
