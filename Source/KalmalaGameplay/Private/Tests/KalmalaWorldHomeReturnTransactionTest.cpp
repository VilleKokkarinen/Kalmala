#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaGameMode.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaStorageSaveGame.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldHomeReturnTransactionTest,
    "Kalmala.Gameplay.Persistence.WorldHomeReturnTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWorldHomeReturnTransactionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Home-return fixture world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Destroyed);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };

    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Home-return world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("Home-return world identity initialized"), WorldState->SelectWorldSeedFromServer(418)))
        return false;
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Home-return fixture has the authoritative GameMode"), World->SetGameMode(FURL())))
        return false;
    AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Home-return GameMode created"), Mode)) return false;
    const FKalmalaWorldGenerationConfig WorldConfig = WorldState->GetWorldGenerationConfig();
    Mode->WorldGenerationConfig = WorldConfig;

    UKalmalaWorldPopulationSaveGame* PopulationSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    UKalmalaConstructionSaveGameV2* ConstructionSave = NewObject<UKalmalaConstructionSaveGameV2>(Mode);
    UKalmalaStorageSaveGame* StorageSave = NewObject<UKalmalaStorageSaveGame>(Mode);
    if (!TestNotNull(TEXT("Population save initialized"), PopulationSave)
        || !TestNotNull(TEXT("Construction save initialized"), ConstructionSave)
        || !TestNotNull(TEXT("Storage save initialized"), StorageSave)) return false;
    PopulationSave->InitializeForWorld(WorldConfig);
    ConstructionSave->InitializeForWorld(WorldConfig);
    StorageSave->InitializeForWorld(WorldConfig);
    Mode->PopulationSaveGame = PopulationSave;
    Mode->ConstructionSaveGame = ConstructionSave;
    Mode->StorageSaveGame = StorageSave;

    struct FPlayerFixture
    {
        APlayerController* Controller = nullptr;
        AKalmalaCharacter* Character = nullptr;
        APlayerState* PlayerState = nullptr;
        FString Identity;
        FString Slot;
    };
    TSet<const APlayerController*> LocalControllers;
    TSet<const APlayerController*> ConnectedRemoteControllers;
    TMap<FString, FString> IdentityByPlayerSlot;
    int32 PlayerStateCandidateSerializationAttempts = 0;
    const auto AddPlayer = [this, World, &LocalControllers, &ConnectedRemoteControllers,
        &IdentityByPlayerSlot, &WorldConfig](const FString& Role, const bool bLocal, const int32 PlayerId)
    {
        FPlayerFixture Fixture;
        Fixture.Controller = World->SpawnActor<APlayerController>();
        Fixture.Character = World->SpawnActor<AKalmalaCharacter>();
        Fixture.PlayerState = World->SpawnActor<APlayerState>();
        if (!TestNotNull(Role + TEXT(" controller spawned"), Fixture.Controller)
            || !TestNotNull(Role + TEXT(" character spawned"), Fixture.Character)
            || !TestNotNull(Role + TEXT(" player state spawned"), Fixture.PlayerState)) return Fixture;

        const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
            FString::Printf(TEXT("rf23-%s"), *Role), FName(TEXT("KalmalaRF23Test")));
        Fixture.PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
        Fixture.PlayerState->SetPlayerId(PlayerId);
        Fixture.Identity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
        Fixture.Slot = FString::Printf(TEXT("KalmalaPlayerDiscoveries_%llu_%08x"),
            WorldConfig.WorldSeed, FCrc::StrCrc32(*Fixture.Identity));
        IdentityByPlayerSlot.Add(Fixture.Slot, Fixture.Identity);
        Fixture.Character->SetPlayerState(Fixture.PlayerState);
        Fixture.Controller->SetPlayerState(Fixture.PlayerState);
        if (bLocal)
        {
            LocalControllers.Add(Fixture.Controller);
        }
        else
        {
            ConnectedRemoteControllers.Add(Fixture.Controller);
        }
        Fixture.Character->DispatchBeginPlay();
        Fixture.Controller->Possess(Fixture.Character);
        World->AddController(Fixture.Controller);
        return Fixture;
    };

    ENetMode FixtureNetMode = NM_Standalone;
    double HomeReturnNowSeconds = 0.0;
    Mode->HomeReturnNetModeForTests = [&FixtureNetMode](const UWorld*) { return FixtureNetMode; };
    Mode->HomeReturnRetryTimeForTests = [&HomeReturnNowSeconds](const UWorld*) { return HomeReturnNowSeconds; };
    Mode->HomeReturnLocalControllerForTests = [&LocalControllers](const APlayerController* Controller)
    {
        return LocalControllers.Contains(Controller);
    };
    Mode->HomeReturnConnectionForTests = [&ConnectedRemoteControllers](const APlayerController* Controller)
    {
        return ConnectedRemoteControllers.Contains(Controller);
    };

    FString FailedSlot;
    TMap<FString, TArray<uint8>> SavedSlots;
    TMap<FString, int32> WriteAttempts;
    Mode->PlayerStateCandidateSerializationAttemptForTests = [&PlayerStateCandidateSerializationAttempts]()
    {
        ++PlayerStateCandidateSerializationAttempts;
    };
    const auto WriteSave = [this, &FailedSlot, &SavedSlots, &WriteAttempts](USaveGame* SaveGame,
        const FString& Slot)
    {
        ++WriteAttempts.FindOrAdd(Slot);
        if (Slot == FailedSlot) return false;
        TArray<uint8> Bytes;
        if (!UGameplayStatics::SaveGameToMemory(SaveGame, Bytes) || Bytes.IsEmpty()) return false;
        SavedSlots.Add(Slot, MoveTemp(Bytes));
        return true;
    };
    Mode->PopulationSaveWriteAttemptForTests = [WriteSave](UKalmalaWorldPopulationSaveGame* SaveGame,
        const FString& Slot, const int32)
    {
        return WriteSave(SaveGame, Slot);
    };
    Mode->WorldComponentSaveWriterForTests = [WriteSave](USaveGame* SaveGame, const FString& Slot,
        const int32)
    {
        return WriteSave(SaveGame, Slot);
    };
    Mode->PlayerDiscoverySaveLoaderForTests = [](const FString&, const int32, USaveGame*& OutSaveGame)
    {
        OutSaveGame = nullptr;
        return false;
    };
    Mode->PlayerDiscoverySaveWriterForTests = [this, &WorldConfig, &IdentityByPlayerSlot, WriteSave](
        UKalmalaPlayerDiscoverySaveGameV2* SaveGame, const FString& Slot, const int32)
    {
        const FString* Identity = IdentityByPlayerSlot.Find(Slot);
        TestNotNull(TEXT("Player write uses a registered authenticated slot"), Identity);
        if (Identity == nullptr || SaveGame == nullptr
            || !TestTrue(TEXT("Player write candidate matches its authenticated world and owner"),
                SaveGame->MatchesPlayer(WorldConfig, *Identity))) return false;
        return WriteSave(SaveGame, Slot);
    };

    TArray<APlayerController*> TravelControllers;
    TArray<FString> CompletionEvents;
    Mode->HomeReturnTravelForTests = [&TravelControllers, &CompletionEvents](APlayerController* Controller)
    {
        TravelControllers.Add(Controller);
        CompletionEvents.Add(TEXT("Home"));
    };
    TArray<AKalmalaCharacter*> ReturnedClientCharacters;
    Mode->HomeReturnClientResponseForTests = [&ReturnedClientCharacters](AKalmalaCharacter* Character,
        const bool bSaved)
    {
        if (bSaved) ReturnedClientCharacters.Add(Character);
    };

    const FPlayerFixture Host = AddPlayer(TEXT("host"), true, 1);
    if (!TestNotNull(TEXT("Solo host fixture is complete"), Host.Character)) return false;
    const FString PopulationSlot = FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"), WorldConfig.WorldSeed);
    const FString ConstructionSlot = FString::Printf(TEXT("KalmalaConstruction_%llu"), WorldConfig.WorldSeed);
    const FString StorageSlot = UKalmalaStorageSaveGame::MakeSlotName(WorldConfig);

    const TArray<FString> SoloFailureSlots = { PopulationSlot, ConstructionSlot, StorageSlot, Host.Slot };
    for (const FString& Slot : SoloFailureSlots)
    {
        FailedSlot = Slot;
        SavedSlots.Reset();
        const int32 TravelCountBefore = TravelControllers.Num();
        TestFalse(FString::Printf(TEXT("Solo return stays in session after failed %s write"), *Slot),
            Mode->TryReturnToHomeFromSolo(Host.Controller));
        TestFalse(TEXT("Failed solo return does not latch its travel state"), Mode->bSoloReturnToHomeInProgress);
        TestEqual(TEXT("Failed solo return does not travel Home"), TravelControllers.Num(), TravelCountBefore);
        FailedSlot.Reset();
        TestTrue(TEXT("Solo world remains writable for retry after a failure"),
            Mode->TrySaveWorldFromLocalHost(Host.Controller));
    }

    SavedSlots.Reset();
    WriteAttempts.Reset();
    TestTrue(TEXT("Solo return succeeds after all writes recover"), Mode->TryReturnToHomeFromSolo(Host.Controller));
    TestTrue(TEXT("Successful solo return latches travel"), Mode->bSoloReturnToHomeInProgress);
    TestEqual(TEXT("Successful solo return travels exactly once"), TravelControllers.Num(), 1);
    TestTrue(TEXT("Solo saves population"), WriteAttempts.FindRef(PopulationSlot) > 0);
    TestTrue(TEXT("Solo saves construction"), WriteAttempts.FindRef(ConstructionSlot) > 0);
    TestTrue(TEXT("Solo saves storage"), WriteAttempts.FindRef(StorageSlot) > 0);
    TestTrue(TEXT("Solo writes the authenticated player record before and with world state"),
        WriteAttempts.FindRef(Host.Slot) >= 2);
    const auto CheckSavedSlotsReopen = [this, &SavedSlots, &WorldConfig, &IdentityByPlayerSlot,
        &PopulationSlot, &ConstructionSlot, &StorageSlot](const TCHAR* Label)
    {
        TestTrue(FString::Printf(TEXT("%s writes a population checkpoint"), Label), SavedSlots.Contains(PopulationSlot));
        TestTrue(FString::Printf(TEXT("%s writes a construction checkpoint"), Label), SavedSlots.Contains(ConstructionSlot));
        TestTrue(FString::Printf(TEXT("%s writes a storage checkpoint"), Label), SavedSlots.Contains(StorageSlot));
        for (const TPair<FString, TArray<uint8>>& SlotBytes : SavedSlots)
        {
            USaveGame* Loaded = UGameplayStatics::LoadGameFromMemory(SlotBytes.Value);
            TestNotNull(FString::Printf(TEXT("%s reopens %s from preserved bytes"), Label, *SlotBytes.Key), Loaded);
            if (Loaded == nullptr) continue;
            if (SlotBytes.Key == PopulationSlot)
            {
                UKalmalaWorldPopulationSaveGame* Save = Cast<UKalmalaWorldPopulationSaveGame>(Loaded);
                TestTrue(FString::Printf(TEXT("%s population checkpoint retains world identity"), Label),
                    Save != nullptr && Save->MatchesWorld(WorldConfig));
            }
            else if (SlotBytes.Key == ConstructionSlot)
            {
                UKalmalaConstructionSaveGameV2* Save = Cast<UKalmalaConstructionSaveGameV2>(Loaded);
                TestTrue(FString::Printf(TEXT("%s construction checkpoint retains world identity"), Label),
                    Save != nullptr && Save->MatchesWorld(WorldConfig));
            }
            else if (SlotBytes.Key == StorageSlot)
            {
                UKalmalaStorageSaveGame* Save = Cast<UKalmalaStorageSaveGame>(Loaded);
                TestTrue(FString::Printf(TEXT("%s storage checkpoint retains world identity"), Label),
                    Save != nullptr && Save->MatchesWorld(WorldConfig));
            }
            else if (const FString* Identity = IdentityByPlayerSlot.Find(SlotBytes.Key))
            {
                UKalmalaPlayerDiscoverySaveGameV2* Save = Cast<UKalmalaPlayerDiscoverySaveGameV2>(Loaded);
                TestTrue(FString::Printf(TEXT("%s player checkpoint retains owner and world identity"), Label),
                    Save != nullptr && Save->MatchesPlayer(WorldConfig, *Identity));
            }
        }
    };
    CheckSavedSlotsReopen(TEXT("Solo return"));

    // The travel seam keeps this fixture alive so the remaining server/client paths can share its saves.
    Mode->bSoloReturnToHomeInProgress = false;
    TravelControllers.Reset();
    CompletionEvents.Reset();
    SavedSlots.Reset();
    WriteAttempts.Reset();
    const FPlayerFixture Owner = AddPlayer(TEXT("owner"), false, 2);
    const FPlayerFixture Observer = AddPlayer(TEXT("observer"), false, 3);
    if (!TestNotNull(TEXT("Network owner fixture is complete"), Owner.Character)
        || !TestNotNull(TEXT("Network observer fixture is complete"), Observer.Character)) return false;

    FixtureNetMode = NM_Standalone;
    TestFalse(TEXT("Client return is rejected in a standalone world"),
        Mode->TryReturnNetworkClientToHomeFromServer(Owner.Controller, Owner.Character));
    FixtureNetMode = NM_DedicatedServer;
    FailedSlot = Owner.Slot;
    const int32 CandidateAttemptsBeforeFailure = PlayerStateCandidateSerializationAttempts;
    TestFalse(TEXT("Client save failure leaves the client connected and retryable"),
        Mode->TryReturnNetworkClientToHomeFromServer(Owner.Controller, Owner.Character));
    TestEqual(TEXT("First failed client return prepares one save candidate"),
        PlayerStateCandidateSerializationAttempts, CandidateAttemptsBeforeFailure + 1);
    TestEqual(TEXT("First failed client return attempts one owner-slot write"), WriteAttempts.FindRef(Owner.Slot), 1);
    TestFalse(TEXT("Failed client save clears its pending return token"),
        Mode->PendingNetworkHomeReturns.Contains(TWeakObjectPtr<APlayerController>(Owner.Controller)));

    const int32 CandidateAttemptsAfterFailure = PlayerStateCandidateSerializationAttempts;
    TestFalse(TEXT("Immediate failed client return retry is throttled"),
        Mode->TryReturnNetworkClientToHomeFromServer(Owner.Controller, Owner.Character));
    TestEqual(TEXT("Immediate retry does not serialize another save candidate"),
        PlayerStateCandidateSerializationAttempts, CandidateAttemptsAfterFailure);
    TestEqual(TEXT("Immediate retry does not attempt another owner-slot write"), WriteAttempts.FindRef(Owner.Slot), 1);
    TestFalse(TEXT("Throttled retry does not create a pending return token"),
        Mode->PendingNetworkHomeReturns.Contains(TWeakObjectPtr<APlayerController>(Owner.Controller)));

    HomeReturnNowSeconds = AKalmalaGameMode::NetworkHomeReturnRetryIntervalSeconds - 0.01;
    TestFalse(TEXT("A request just inside the retry interval remains throttled"),
        Mode->TryReturnNetworkClientToHomeFromServer(Owner.Controller, Owner.Character));
    TestEqual(TEXT("Pre-expiry retry does not serialize another candidate"),
        PlayerStateCandidateSerializationAttempts, CandidateAttemptsAfterFailure);
    TestEqual(TEXT("Pre-expiry retry does not perform another write"), WriteAttempts.FindRef(Owner.Slot), 1);

    TestTrue(TEXT("Another controller can return while the failed owner is backed off"),
        Mode->TryReturnNetworkClientToHomeFromServer(Observer.Controller, Observer.Character));
    TestEqual(TEXT("Independent controller persists its own record"), WriteAttempts.FindRef(Observer.Slot), 1);

    HomeReturnNowSeconds = AKalmalaGameMode::NetworkHomeReturnRetryIntervalSeconds;
    FailedSlot.Reset();
    SavedSlots.Reset();
    WriteAttempts.Reset();
    const int32 CandidateAttemptsBeforeRetry = PlayerStateCandidateSerializationAttempts;
    TestTrue(TEXT("Network client return persists the authenticated requesting owner"),
        Mode->TryReturnNetworkClientToHomeFromServer(Owner.Controller, Owner.Character));
    TestEqual(TEXT("Later client retry prepares one new save candidate"),
        PlayerStateCandidateSerializationAttempts, CandidateAttemptsBeforeRetry + 1);
    TestEqual(TEXT("Client return writes only the requesting player's record"),
        WriteAttempts.FindRef(Owner.Slot), 1);
    TestEqual(TEXT("Client return does not write another player's record"),
        WriteAttempts.FindRef(Observer.Slot), 0);
    TestEqual(TEXT("Client return writes no population checkpoint"), WriteAttempts.FindRef(PopulationSlot), 0);
    TestEqual(TEXT("Client return writes no construction checkpoint"), WriteAttempts.FindRef(ConstructionSlot), 0);
    TestEqual(TEXT("Client return writes no storage checkpoint"), WriteAttempts.FindRef(StorageSlot), 0);
    TestTrue(TEXT("Client return tracks only the authenticated controller"),
        Mode->PendingNetworkHomeReturns.Contains(TWeakObjectPtr<APlayerController>(Owner.Controller)));
    const int32 CandidateAttemptsAfterSuccess = PlayerStateCandidateSerializationAttempts;
    TestFalse(TEXT("Successful client return retains its one-return guard"),
        Mode->TryReturnNetworkClientToHomeFromServer(Owner.Controller, Owner.Character));
    TestEqual(TEXT("Duplicate successful return does not serialize a candidate"),
        PlayerStateCandidateSerializationAttempts, CandidateAttemptsAfterSuccess);
    TestEqual(TEXT("Duplicate successful return does not write its player slot"), WriteAttempts.FindRef(Owner.Slot), 1);
    TestFalse(TEXT("Client cannot submit another player's pawn"),
        Mode->TryReturnNetworkClientToHomeFromServer(Owner.Controller, Observer.Character));

    FixtureNetMode = NM_ListenServer;
    Mode->PendingNetworkHomeReturns.Reset();
    TestFalse(TEXT("A remote controller cannot save the host world"),
        Mode->TrySaveWorldFromLocalHost(Owner.Controller));
    TestFalse(TEXT("A remote controller cannot initiate listen-host return"),
        Mode->TryReturnListenHostToHome(Owner.Controller));
    SavedSlots.Reset();
    WriteAttempts.Reset();
    const TArray<FString> HostFailureSlots = {
        PopulationSlot, ConstructionSlot, StorageSlot, Host.Slot, Owner.Slot, Observer.Slot
    };
    for (const FString& Slot : HostFailureSlots)
    {
        FailedSlot = Slot;
        const int32 TravelCountBefore = TravelControllers.Num();
        TestFalse(FString::Printf(TEXT("Listen-host return stays active after failed %s write"), *Slot),
            Mode->TryReturnListenHostToHome(Host.Controller));
        TestFalse(TEXT("Failed listen-host return does not latch its travel state"), Mode->bListenHostReturnInProgress);
        TestTrue(TEXT("Failed listen-host return leaves the pending drain empty"),
            Mode->PendingListenHostReturnClients.IsEmpty());
        TestEqual(TEXT("Failed listen-host return does not travel Home"), TravelControllers.Num(), TravelCountBefore);
        FailedSlot.Reset();
        TestTrue(TEXT("Listen-host session can retry world saving after a failure"),
            Mode->TrySaveWorldFromLocalHost(Host.Controller));
    }

    FailedSlot.Reset();
    SavedSlots.Reset();
    WriteAttempts.Reset();
    TestTrue(TEXT("Listen-host return starts after all player and world writes succeed"),
        Mode->TryReturnListenHostToHome(Host.Controller));
    TestTrue(TEXT("Listen-host return is now draining clients"), Mode->bListenHostReturnInProgress);
    TestEqual(TEXT("Listen-host return waits for both snapshotted clients"),
        Mode->PendingListenHostReturnClients.Num(), 2);
    TestTrue(TEXT("Listen host saves population"), WriteAttempts.FindRef(PopulationSlot) > 0);
    TestTrue(TEXT("Listen host saves construction"), WriteAttempts.FindRef(ConstructionSlot) > 0);
    TestTrue(TEXT("Listen host saves storage"), WriteAttempts.FindRef(StorageSlot) > 0);
    TestTrue(TEXT("Listen host saves every current player record"),
        WriteAttempts.FindRef(Host.Slot) >= 2 && WriteAttempts.FindRef(Owner.Slot) >= 2
            && WriteAttempts.FindRef(Observer.Slot) >= 2);
    TestTrue(TEXT("Both snapshotted clients receive a successful local return request"),
        ReturnedClientCharacters.Contains(Owner.Character) && ReturnedClientCharacters.Contains(Observer.Character));

    FString LateJoinError;
    Mode->PreLogin(TEXT(""), TEXT("127.0.0.1"), FUniqueNetIdRepl(), LateJoinError);
    TestFalse(TEXT("New network joins are rejected after host return begins"), LateJoinError.IsEmpty());
    // Model a connection accepted before the return flag that finishes PostLogin afterward.
    const FPlayerFixture LateJoin = AddPlayer(TEXT("late"), false, 4);
    if (!TestNotNull(TEXT("In-flight late join fixture is complete"), LateJoin.Character)) return false;
    TestTrue(TEXT("PostLogin includes an accepted late join in the pending drain"),
        Mode->HandleInFlightListenHostJoin(LateJoin.Controller));
    TestTrue(TEXT("An accepted late join is added to the host's pending drain"),
        Mode->PendingListenHostReturnClients.Contains(TWeakObjectPtr<APlayerController>(LateJoin.Controller)));
    TestTrue(TEXT("A late join is asked to return only after its record saves"),
        ReturnedClientCharacters.Contains(LateJoin.Character));

    const FPlayerFixture LateJoinFailure = AddPlayer(TEXT("late-failure"), false, 5);
    if (!TestNotNull(TEXT("Late-join retry fixture is complete"), LateJoinFailure.Character)) return false;
    FailedSlot = LateJoinFailure.Slot;
    const int32 ClientResponsesBeforeLateJoin = ReturnedClientCharacters.Num();
    TestTrue(TEXT("PostLogin keeps a failed late-join save in the pending drain"),
        Mode->HandleInFlightListenHostJoin(LateJoinFailure.Controller));
    TestTrue(TEXT("A late join with a failed save remains in the host's pending drain"),
        Mode->PendingListenHostReturnClients.Contains(TWeakObjectPtr<APlayerController>(LateJoinFailure.Controller)));
    TestEqual(TEXT("A late join is not told to return before its save succeeds"),
        ReturnedClientCharacters.Num(), ClientResponsesBeforeLateJoin);
    FailedSlot.Reset();
    TestTrue(TEXT("Late join can save its own record and retry return"),
        Mode->TryReturnNetworkClientToHomeFromServer(LateJoinFailure.Controller, LateJoinFailure.Character));

    int32 DriverTeardownAttempts = 0;
    Mode->ListenHostNetDriverTeardownForTests = [&DriverTeardownAttempts, &CompletionEvents](UWorld*)
    {
        ++DriverTeardownAttempts;
        CompletionEvents.Add(TEXT("DriverTeardown"));
        return DriverTeardownAttempts > 1;
    };
    Mode->CompleteListenHostReturn();
    TestEqual(TEXT("Host cannot tear down the driver while a client remains"), DriverTeardownAttempts, 0);
    TestTrue(TEXT("Client and host returns remain pending until logout"), Mode->bListenHostReturnInProgress);

    Mode->Logout(Owner.Controller);
    TestEqual(TEXT("Host still waits after the first client logout"), Mode->PendingListenHostReturnClients.Num(), 3);
    Mode->Logout(Observer.Controller);
    TestEqual(TEXT("Host still waits for both late joins"), Mode->PendingListenHostReturnClients.Num(), 2);
    Mode->Logout(LateJoin.Controller);
    TestEqual(TEXT("Host still waits for the late join that retried its save"),
        Mode->PendingListenHostReturnClients.Num(), 1);
    Mode->Logout(LateJoinFailure.Controller);
    TestTrue(TEXT("All remote clients leave before host teardown"),
        Mode->PendingListenHostReturnClients.IsEmpty());
    CheckSavedSlotsReopen(TEXT("Listen-host return"));

    Mode->CompleteListenHostReturn();
    TestEqual(TEXT("Incomplete listen-driver teardown does not travel Home"), DriverTeardownAttempts, 1);
    TestTrue(TEXT("Listen-host return stays retryable until driver teardown finishes"),
        Mode->bListenHostReturnInProgress);
    TestTrue(TEXT("Host does not travel before driver teardown finishes"), TravelControllers.IsEmpty());
    Mode->CompleteListenHostReturn();
    TestEqual(TEXT("Driver teardown is checked again before host travel"), DriverTeardownAttempts, 2);
    TestFalse(TEXT("Completed listen-host return clears its pending state"), Mode->bListenHostReturnInProgress);
    TestEqual(TEXT("Host travels Home only after disconnects and driver teardown"), TravelControllers.Num(), 1);
    TestTrue(TEXT("Listen host owns the final Home transition"), TravelControllers[0] == Host.Controller);
    TestTrue(TEXT("Driver teardown precedes Home travel"), CompletionEvents.Num() >= 2
        && CompletionEvents[CompletionEvents.Num() - 2] == TEXT("DriverTeardown")
        && CompletionEvents.Last() == TEXT("Home"));
    return true;
}

#endif
