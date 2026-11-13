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
        TEXT("Unversioned save data is rejected during development"),
        static_cast<uint8>(UKalmalaM7PersistenceSaveGame::EvaluateSchemaVersion(0)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::Reject));
    TestEqual(
        TEXT("The prior development schema is rejected without migration"),
        static_cast<uint8>(UKalmalaM7PersistenceSaveGame::EvaluateSchemaVersion(UKalmalaM7PersistenceSaveGame::CurrentSchemaVersion - 1)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::Reject));
    TestEqual(
        TEXT("A future schema is rejected rather than guessed"),
        static_cast<uint8>(UKalmalaM7PersistenceSaveGame::EvaluateSchemaVersion(UKalmalaM7PersistenceSaveGame::CurrentSchemaVersion + 1)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::Reject));

    const FKalmalaM7SparseDelta ResourceDelta{ EKalmalaM7SparseDeltaKind::ResourceDepleted, TEXT("resource:meadow/1,1/0") };
    const FKalmalaM7SparseDelta CreatureDelta{ EKalmalaM7SparseDeltaKind::CreatureDefeated, TEXT("creature:0/1/-1/5678") };
    const FKalmalaM7SparseDelta DiscoveryDelta{ EKalmalaM7SparseDeltaKind::DiscoveryClaimed, TEXT("discovery:scroll/1,1/0") };
    const FKalmalaM7SparseDelta PendingRewardDelta{ EKalmalaM7SparseDeltaKind::DiscoveryRewardPending, TEXT("ocean-discovery:1:ocean-driftwood-cache-marker:1,1") };
    TestTrue(TEXT("The gate accepts a server-selected resource depletion identity"), SaveGame->AddSparseDelta(ResourceDelta));
    TestTrue(TEXT("The gate accepts a server-selected creature defeat identity"), SaveGame->AddSparseDelta(CreatureDelta));
    TestTrue(TEXT("The gate accepts a server-selected discovery identity"), SaveGame->AddSparseDelta(DiscoveryDelta));
    TestFalse(TEXT("The gate rejects a pending Ocean reward in a world-scoped ledger"),
        SaveGame->AddSparseDelta(PendingRewardDelta));
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

    UKalmalaM7PersistenceSaveGame* PlayerSave = NewObject<UKalmalaM7PersistenceSaveGame>();
    PlayerSave->Initialize(PlayerIdentity);
    TestTrue(TEXT("An authenticated player's ledger accepts a pending Ocean reward"),
        PlayerSave->AddSparseDelta(PendingRewardDelta));
    TestFalse(TEXT("A pending reward cannot also be finalized as claimed"),
        PlayerSave->AddSparseDelta({ EKalmalaM7SparseDeltaKind::DiscoveryClaimed, PendingRewardDelta.StableId }));
    TArray<uint8> PendingBytes;
    TestTrue(TEXT("The pending reward serializes in memory"), UGameplayStatics::SaveGameToMemory(PlayerSave, PendingBytes));
    UKalmalaM7PersistenceSaveGame* ReloadedPlayerSave =
        Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(PendingBytes));
    if (TestNotNull(TEXT("The pending reward reloads as its expected type"), ReloadedPlayerSave))
    {
        TestTrue(TEXT("The reloaded pending reward remains owner-scoped and valid"),
            ReloadedPlayerSave->Matches(PlayerIdentity));
        TestTrue(TEXT("The reloaded ledger retains its pending Ocean reward"),
            ReloadedPlayerSave->HasSparseDelta(PendingRewardDelta.Kind, PendingRewardDelta.StableId));
        TestTrue(TEXT("Finalization can remove the pending reward"),
            ReloadedPlayerSave->RemoveSparseDelta(PendingRewardDelta.Kind, PendingRewardDelta.StableId));
        TestFalse(TEXT("A finalized ledger no longer reports that reward as pending"),
            ReloadedPlayerSave->HasSparseDelta(PendingRewardDelta.Kind, PendingRewardDelta.StableId));
        TestFalse(TEXT("A pending reward cannot be removed twice"),
            ReloadedPlayerSave->RemoveSparseDelta(PendingRewardDelta.Kind, PendingRewardDelta.StableId));
    }

    return true;
}

#endif
