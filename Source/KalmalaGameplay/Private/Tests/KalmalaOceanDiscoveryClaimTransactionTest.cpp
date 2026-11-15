#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaOceanDiscoveryCatalogue.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanDiscoveryClaimTransactionTest,
    "Kalmala.Gameplay.OceanTravel.DiscoveryClaimRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanDiscoveryClaimTransactionTest::RunTest(const FString& Parameters)
{
    if (!TestNotNull(TEXT("Engine is available for isolated Ocean discovery worlds"), GEngine)) return false;

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    FKalmalaOceanDiscoveryDescriptor CapacityDescriptor;
    FKalmalaOceanDiscoveryDescriptor RecoveryDescriptor;
    FKalmalaOceanDiscoveryDescriptor PostCollectionDescriptor;
    bool bFoundCapacityDescriptor = false;
    bool bFoundRecoveryDescriptor = false;
    bool bFoundPostCollectionDescriptor = false;
    TSet<FString> FoundClaimIds;
    for (int32 Y = -64; Y <= 64
        && (!bFoundCapacityDescriptor || !bFoundRecoveryDescriptor || !bFoundPostCollectionDescriptor); ++Y)
    {
        for (int32 X = -64; X <= 64
            && (!bFoundCapacityDescriptor || !bFoundRecoveryDescriptor || !bFoundPostCollectionDescriptor); ++X)
        {
            const TArray<FKalmalaOceanDiscoveryDescriptor> Descriptors =
                FKalmalaOceanDiscoveryCatalogue::BuildDescriptors(WorldConfig, FIntPoint(X, Y));
            if (Descriptors.IsEmpty()) continue;
            const FString ClaimId = FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(
                Descriptors[0].DiscoveryId, Descriptors[0].SpatialKey);
            if (ClaimId.IsEmpty() || FoundClaimIds.Contains(ClaimId)) continue;
            FoundClaimIds.Add(ClaimId);
            if (!bFoundCapacityDescriptor)
            {
                CapacityDescriptor = Descriptors[0];
                bFoundCapacityDescriptor = true;
            }
            else if (!bFoundRecoveryDescriptor)
            {
                RecoveryDescriptor = Descriptors[0];
                bFoundRecoveryDescriptor = true;
            }
            else
            {
                PostCollectionDescriptor = Descriptors[0];
                bFoundPostCollectionDescriptor = true;
            }
        }
    }
    if (!TestTrue(TEXT("Fixture finds a canonical Ocean discovery for capacity retry"), bFoundCapacityDescriptor)
        || !TestTrue(TEXT("Fixture finds a second canonical Ocean discovery for recovery"), bFoundRecoveryDescriptor)
        || !TestTrue(TEXT("Fixture finds a third canonical Ocean discovery for post-collection claim"),
            bFoundPostCollectionDescriptor)) return false;

    const FKalmalaOceanDiscoveryDefinition* CapacityDefinition =
        FKalmalaOceanDiscoveryCatalogue::FindDefinition(CapacityDescriptor.DiscoveryId);
    const FKalmalaOceanDiscoveryDefinition* RecoveryDefinition =
        FKalmalaOceanDiscoveryCatalogue::FindDefinition(RecoveryDescriptor.DiscoveryId);
    const FKalmalaOceanDiscoveryDefinition* PostCollectionDefinition =
        FKalmalaOceanDiscoveryCatalogue::FindDefinition(PostCollectionDescriptor.DiscoveryId);
    if (!TestNotNull(TEXT("Capacity discovery resolves its canonical reward"), CapacityDefinition)
        || !TestNotNull(TEXT("Recovery discovery resolves its canonical reward"), RecoveryDefinition)
        || !TestNotNull(TEXT("Post-collection discovery resolves its canonical reward"),
            PostCollectionDefinition)) return false;

    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("rf28-ocean-discovery-owner"), FName(TEXT("KalmalaRF28Test")));
    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    const FKalmalaM7SaveIdentity PlayerSaveIdentity =
        FKalmalaM7SaveIdentity::ForPlayer(WorldConfig.WorldSeed, PlayerIdentity);
    const FString ExpectedSlot = FString::Printf(TEXT("KalmalaOceanClaims_%llu_%d_%08x"),
        PlayerSaveIdentity.WorldSeed, PlayerSaveIdentity.GeneratorRevision, FCrc::StrCrc32(*PlayerIdentity));

    TMap<FString, TArray<uint8>> SaveSlotBytes;
    int32 LoadAttempts = 0;
    int32 WriteAttempts = 0;
    bool bFailNextInventoryCommit = false;
    bool bFailNextRollbackWrite = false;
    bool bFailNextCompletionWrite = false;
    const FString RecoveryClaimId = FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(
        RecoveryDescriptor.DiscoveryId, RecoveryDescriptor.SpatialKey);

    TArray<UWorld*> FixtureWorlds;
    ON_SCOPE_EXIT
    {
        for (UWorld* World : FixtureWorlds)
        {
            if (World == nullptr) continue;
            if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Destroyed);
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }
    };

    struct FDiscoveryFixture
    {
        UWorld* World = nullptr;
        AKalmalaGameMode* GameMode = nullptr;
        AKalmalaCharacter* Player = nullptr;
        UKalmalaInventoryComponent* Inventory = nullptr;
        UKalmalaDiscoveryProgressComponent* Feedback = nullptr;
    };

    const auto CreateFixture = [this, &FixtureWorlds, &WorldConfig, &CapacityDescriptor, &NetId](const TCHAR* Label)
        -> FDiscoveryFixture
    {
        FDiscoveryFixture Fixture;
        const FString Prefix(Label);
        Fixture.World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!TestNotNull(Prefix + TEXT(" world created"), Fixture.World)) return Fixture;
        FixtureWorlds.Add(Fixture.World);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Fixture.World);
        Fixture.World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        Fixture.World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
        if (!TestTrue(Prefix + TEXT(" selects the authoritative GameMode"), Fixture.World->SetGameMode(FURL())))
            return Fixture;
        Fixture.GameMode = Fixture.World->GetAuthGameMode<AKalmalaGameMode>();
        if (!TestNotNull(Prefix + TEXT(" GameMode created"), Fixture.GameMode)) return Fixture;

        AKalmalaWorldGenerationGameState* WorldState =
            Fixture.World->SpawnActor<AKalmalaWorldGenerationGameState>();
        if (!TestNotNull(Prefix + TEXT(" world identity state created"), WorldState)) return Fixture;
        Fixture.World->SetGameState(WorldState);
        if (!TestTrue(Prefix + TEXT(" world seed initialized"),
                WorldState->SelectWorldSeedFromServer(WorldConfig.WorldSeed))) return Fixture;
        Fixture.GameMode->WorldGenerationConfig = WorldConfig;

        APlayerState* PlayerState = Fixture.World->SpawnActor<APlayerState>();
        Fixture.Player = Fixture.World->SpawnActor<AKalmalaCharacter>();
        if (!TestNotNull(Prefix + TEXT(" authenticated player state created"), PlayerState)
            || !TestNotNull(Prefix + TEXT(" player pawn created"), Fixture.Player)) return Fixture;
        PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
        PlayerState->SetPlayerId(1);
        Fixture.Player->SetPlayerState(PlayerState);
        Fixture.Player->DispatchBeginPlay();
        Fixture.Player->SetActorLocation(CapacityDescriptor.Location);
        Fixture.Inventory = Fixture.Player->GetInventoryComponent();
        Fixture.Feedback = Fixture.Player->GetDiscoveryProgressComponent();
        TestNotNull(Prefix + TEXT(" inventory component created"), Fixture.Inventory);
        TestNotNull(Prefix + TEXT(" discovery feedback component created"), Fixture.Feedback);
        return Fixture;
    };

    const auto InstallSaveSeams = [this, &SaveSlotBytes, &LoadAttempts, &WriteAttempts,
        &ExpectedSlot, &bFailNextRollbackWrite, &bFailNextCompletionWrite, &RecoveryClaimId](
        AKalmalaGameMode* GameMode)
    {
        GameMode->OceanDiscoverySaveLoaderForTests = [this, &SaveSlotBytes, &LoadAttempts, &ExpectedSlot](
            const FString& Slot, const int32 UserIndex, USaveGame*& OutSaveGame)
        {
            ++LoadAttempts;
            TestEqual(TEXT("Ocean discovery loads the exact authenticated owner slot"), Slot, ExpectedSlot);
            TestEqual(TEXT("Ocean discovery reads local user zero"), UserIndex, 0);
            const TArray<uint8>* Bytes = SaveSlotBytes.Find(Slot);
            if (Bytes == nullptr) return false;
            OutSaveGame = UGameplayStatics::LoadGameFromMemory(*Bytes);
            return OutSaveGame != nullptr;
        };
        GameMode->OceanDiscoverySaveWriterForTests = [this, &SaveSlotBytes, &WriteAttempts,
            &ExpectedSlot, &bFailNextRollbackWrite, &bFailNextCompletionWrite, &RecoveryClaimId](
                UKalmalaM7PersistenceSaveGame* Candidate, const FString& Slot, const int32 UserIndex)
        {
            ++WriteAttempts;
            TestEqual(TEXT("Ocean discovery writes the exact authenticated owner slot"), Slot, ExpectedSlot);
            TestEqual(TEXT("Ocean discovery writes local user zero"), UserIndex, 0);
            if (Candidate == nullptr || Slot != ExpectedSlot || UserIndex != 0) return false;

            const bool bPending = Candidate->HasSparseDelta(
                EKalmalaM7SparseDeltaKind::DiscoveryRewardPending, RecoveryClaimId);
            const bool bClaimed = Candidate->HasSparseDelta(
                EKalmalaM7SparseDeltaKind::DiscoveryClaimed, RecoveryClaimId);
            if (bFailNextRollbackWrite && !bPending && !bClaimed)
            {
                bFailNextRollbackWrite = false;
                return false;
            }
            if (bFailNextCompletionWrite && bClaimed && !bPending)
            {
                bFailNextCompletionWrite = false;
                return false;
            }

            TArray<uint8> Bytes;
            if (!UGameplayStatics::SaveGameToMemory(Candidate, Bytes)) return false;
            SaveSlotBytes.Add(Slot, MoveTemp(Bytes));
            return true;
        };
    };

    FDiscoveryFixture First = CreateFixture(TEXT("Initial Ocean discovery"));
    if (!TestNotNull(TEXT("Initial Ocean discovery world is ready"), First.World)
        || !TestNotNull(TEXT("Initial Ocean discovery GameMode is ready"), First.GameMode)
        || !TestNotNull(TEXT("Initial Ocean discovery player is ready"), First.Player)
        || !TestNotNull(TEXT("Initial Ocean discovery inventory is ready"), First.Inventory)
        || !TestNotNull(TEXT("Initial Ocean discovery feedback is ready"), First.Feedback)) return false;
    InstallSaveSeams(First.GameMode);
    First.GameMode->OceanDiscoveryInventoryCommitGateForTests = [&bFailNextInventoryCommit]()
    {
        if (!bFailNextInventoryCommit) return true;
        bFailNextInventoryCommit = false;
        return false;
    };

    TArray<FKalmalaInventoryStack> NearlyFullPack;
    const UKalmalaItemCatalogue* ItemCatalogue = UKalmalaItemCatalogue::Get();
    if (!TestNotNull(TEXT("Canonical item catalogue is available"), ItemCatalogue)) return false;
    for (const FKalmalaItemDefinition& Item : ItemCatalogue->Items)
    {
        if (Item.ItemId == CapacityDefinition->RewardItemId) continue;
        TArray<FKalmalaInventoryStack> Candidate;
        FString Reason;
        if (UKalmalaInventoryComponent::BuildGrant(NearlyFullPack, Item.ItemId, 1, Candidate, Reason)
            && Candidate.Num() > NearlyFullPack.Num())
        {
            NearlyFullPack = MoveTemp(Candidate);
        }
        if (NearlyFullPack.Num() == UKalmalaInventoryComponent::MaxSlots - 1) break;
    }
    if (!TestEqual(TEXT("Catalogue can fill every stack cell except the reward cell"),
            NearlyFullPack.Num(), UKalmalaInventoryComponent::MaxSlots - 1)) return false;

    TArray<FKalmalaToolState>& PackedTools =
        const_cast<TArray<FKalmalaToolState>&>(First.Player->GetCarriedToolInventory());
    PackedTools.Reset();
    FKalmalaToolState PackedTool;
    PackedTool.ItemId = TEXT("StoneKnife");
    PackedTool.ToolLevel = 1;
    PackedTool.Durability = 1;
    PackedTool.InstanceGuid = FGuid::NewGuid();
    PackedTools.Add(PackedTool);
    First.Player->SetActorLocation(CapacityDescriptor.Location);
    if (!TestTrue(TEXT("The combined-capacity fixture installs its stack rows and packed tool"),
            First.Inventory->TryCommitStacksFromServer(First.Inventory->GetStacks(), NearlyFullPack))) return false;

    const uint32 SerialBeforeCapacityRejection = First.Feedback->GetFeedbackSerial();
    TestFalse(TEXT("A reward that exceeds combined stack and packed-tool capacity is rejected"),
        First.GameMode->ClaimOceanDiscovery(First.Player, CapacityDescriptor));
    TestEqual(TEXT("Combined-capacity rejection gives the owner a pack-full result"),
        First.Feedback->GetFeedbackLabel(), FString(TEXT("Pack full; sea discovery remains")));
    TestEqual(TEXT("Combined-capacity rejection publishes once"),
        First.Feedback->GetFeedbackSerial(), SerialBeforeCapacityRejection + 1);
    TestEqual(TEXT("Combined-capacity rejection writes no discovery ledger candidate"), WriteAttempts, 0);
    TestFalse(TEXT("Combined-capacity rejection creates no durable slot"), SaveSlotBytes.Contains(ExpectedSlot));
    TestEqual(TEXT("Combined-capacity rejection leaves inventory unchanged"),
        First.Inventory->GetStacks().Num(), NearlyFullPack.Num());

    PackedTools.Reset();
    TestTrue(TEXT("The same discovery succeeds after freeing the packed-tool cell"),
        First.GameMode->ClaimOceanDiscovery(First.Player, CapacityDescriptor));
    TestEqual(TEXT("Capacity retry grants the exact catalogue quantity"),
        First.Inventory->GetQuantity(CapacityDefinition->RewardItemId), CapacityDefinition->RewardQuantity);
    const FString CapacityClaimId = FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(
        CapacityDescriptor.DiscoveryId, CapacityDescriptor.SpatialKey);
    UKalmalaM7PersistenceSaveGame* FirstSave =
        First.GameMode->OceanDiscoverySaves.FindChecked(PlayerIdentity).Get();
    if (!TestNotNull(TEXT("Successful capacity retry caches its owner ledger"), FirstSave)) return false;
    TestTrue(TEXT("Successful capacity retry persists its completed claim"),
        FirstSave->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryClaimed, CapacityClaimId));
    TestFalse(TEXT("Successful capacity retry clears its pending marker"),
        FirstSave->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryRewardPending, CapacityClaimId));

    const TArray<FKalmalaInventoryStack> GrantedCapacityStacks = First.Inventory->GetStacks();
    TestTrue(TEXT("The recovery scenario starts with an empty player inventory"),
        First.Inventory->TryCommitStacksFromServer(GrantedCapacityStacks, TArray<FKalmalaInventoryStack>()));
    First.Player->SetActorLocation(RecoveryDescriptor.Location);
    bFailNextInventoryCommit = true;
    bFailNextRollbackWrite = true;
    const uint32 SerialBeforeInjectedFailures = First.Feedback->GetFeedbackSerial();
    TestFalse(TEXT("Injected inventory rejection and rollback-write failure reject the initial recovery claim"),
        First.GameMode->ClaimOceanDiscovery(First.Player, RecoveryDescriptor));
    TestEqual(TEXT("Injected transaction failure publishes owner-only Unavailable feedback"),
        First.Feedback->GetFeedback(), EKalmalaDiscoveryFeedback::Unavailable);
    TestEqual(TEXT("Injected transaction failure publishes one result"),
        First.Feedback->GetFeedbackSerial(), SerialBeforeInjectedFailures + 1);
    TestEqual(TEXT("Pending marker and failed rollback are both exercised"), WriteAttempts, 4);
    TestEqual(TEXT("Failed inventory commit leaves the reward ungranted"),
        First.Inventory->GetQuantity(RecoveryDefinition->RewardItemId), 0);

    const TArray<uint8> PendingSlotBytes = SaveSlotBytes.FindChecked(ExpectedSlot);
    UKalmalaM7PersistenceSaveGame* PendingAfterFailure =
        Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(PendingSlotBytes));
    if (!TestNotNull(TEXT("Failed compensation leaves a readable pending ledger"), PendingAfterFailure)) return false;
    TestTrue(TEXT("Pending ledger remains bound to the same authenticated owner"),
        PendingAfterFailure->Matches(PlayerSaveIdentity));
    TestTrue(TEXT("Failed compensation preserves the earlier completed discovery"),
        PendingAfterFailure->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryClaimed, CapacityClaimId));
    TestTrue(TEXT("Failed compensation leaves the undelivered reward pending"),
        PendingAfterFailure->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryRewardPending, RecoveryClaimId));
    TestFalse(TEXT("A failed reward transaction is not durably marked claimed"),
        PendingAfterFailure->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryClaimed, RecoveryClaimId));

    FDiscoveryFixture Recovery = CreateFixture(TEXT("Restarted Ocean discovery"));
    if (!TestNotNull(TEXT("Restarted Ocean discovery world is ready"), Recovery.World)
        || !TestNotNull(TEXT("Restarted Ocean discovery GameMode is ready"), Recovery.GameMode)
        || !TestNotNull(TEXT("Restarted Ocean discovery player is ready"), Recovery.Player)
        || !TestNotNull(TEXT("Restarted Ocean discovery inventory is ready"), Recovery.Inventory)
        || !TestNotNull(TEXT("Restarted Ocean discovery feedback is ready"), Recovery.Feedback)) return false;
    InstallSaveSeams(Recovery.GameMode);
    const TArray<FKalmalaToolState>& RecoveryTools = Recovery.Player->GetCarriedToolInventory();
    const_cast<TArray<FKalmalaToolState>&>(RecoveryTools).Reset();
    Recovery.Player->SetActorLocation(RecoveryDescriptor.Location);
    bFailNextCompletionWrite = true;
    const int32 WritesBeforeRecovery = WriteAttempts;
    TestTrue(TEXT("Restart retries and delivers the pending canonical reward"),
        Recovery.GameMode->ClaimOceanDiscovery(Recovery.Player, RecoveryDescriptor));
    TestEqual(TEXT("Restart recovery grants the exact catalogue quantity once"),
        Recovery.Inventory->GetQuantity(RecoveryDefinition->RewardItemId), RecoveryDefinition->RewardQuantity);
    TestEqual(TEXT("Recovery writes only the completed-claim candidate"), WriteAttempts, WritesBeforeRecovery + 1);
    UKalmalaM7PersistenceSaveGame* PendingAfterCompletionWriteFailure =
        Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(SaveSlotBytes.FindChecked(ExpectedSlot)));
    if (!TestNotNull(TEXT("Failed completion write retains its durable pending record"), PendingAfterCompletionWriteFailure))
        return false;
    TestTrue(TEXT("Failed completion write leaves the reward claim pending"),
        PendingAfterCompletionWriteFailure->HasSparseDelta(
            EKalmalaM7SparseDeltaKind::DiscoveryRewardPending, RecoveryClaimId));
    TestFalse(TEXT("Failed completion write does not publish a completed claim"),
        PendingAfterCompletionWriteFailure->HasSparseDelta(
            EKalmalaM7SparseDeltaKind::DiscoveryClaimed, RecoveryClaimId));

    const TObjectPtr<UKalmalaM7PersistenceSaveGame>* CachedPendingLedger =
        Recovery.GameMode->OceanDiscoverySaves.Find(PlayerIdentity);
    if (!TestNotNull(TEXT("Failed completion retry caches the loaded pending owner ledger"),
            CachedPendingLedger)
        || !TestTrue(TEXT("Cached pending ledger still matches the authenticated owner"),
            CachedPendingLedger->Get() != nullptr && CachedPendingLedger->Get()->Matches(PlayerSaveIdentity)))
    {
        return false;
    }
    const TWeakObjectPtr<UKalmalaM7PersistenceSaveGame> PendingLedgerBeforeCollection(
        CachedPendingLedger->Get());
    CollectGarbage(RF_NoFlags, true);
    TestTrue(TEXT("Full garbage collection keeps the cached pending owner ledger valid"),
        PendingLedgerBeforeCollection.IsValid());
    const TObjectPtr<UKalmalaM7PersistenceSaveGame>* CachedLedgerAfterCollection =
        Recovery.GameMode->OceanDiscoverySaves.Find(PlayerIdentity);
    if (!TestNotNull(TEXT("Pending owner ledger stays in the GameMode cache after collection"),
            CachedLedgerAfterCollection)) return false;
    TestTrue(TEXT("The cached pending ledger remains the loaded owner save"),
        CachedLedgerAfterCollection->Get() == PendingLedgerBeforeCollection.Get());

    const int32 RewardQuantityAfterRecovery = Recovery.Inventory->GetQuantity(RecoveryDefinition->RewardItemId);
    const int32 WritesBeforeFinalizeRetry = WriteAttempts;
    TestFalse(TEXT("A same-session retry finalizes without granting the reward again"),
        Recovery.GameMode->ClaimOceanDiscovery(Recovery.Player, RecoveryDescriptor));
    TestEqual(TEXT("Finalization retry does not duplicate the granted reward"),
        Recovery.Inventory->GetQuantity(RecoveryDefinition->RewardItemId), RewardQuantityAfterRecovery);
    TestEqual(TEXT("Finalization retry writes the completed claim once"), WriteAttempts, WritesBeforeFinalizeRetry + 1);
    UKalmalaM7PersistenceSaveGame* FinalSave =
        Recovery.GameMode->OceanDiscoverySaves.FindChecked(PlayerIdentity).Get();
    if (!TestNotNull(TEXT("Recovered owner ledger is cached"), FinalSave)) return false;
    TestTrue(TEXT("Recovery replaces pending with the completed claim"),
        FinalSave->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryClaimed, RecoveryClaimId)
            && !FinalSave->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryRewardPending, RecoveryClaimId));

    const int32 WritesBeforeDuplicate = WriteAttempts;
    TestFalse(TEXT("A completed Ocean reward cannot be replayed"),
        Recovery.GameMode->ClaimOceanDiscovery(Recovery.Player, RecoveryDescriptor));
    TestEqual(TEXT("Completed replay does not grant another reward"),
        Recovery.Inventory->GetQuantity(RecoveryDefinition->RewardItemId), RewardQuantityAfterRecovery);
    TestEqual(TEXT("Completed replay performs no extra save write"), WriteAttempts, WritesBeforeDuplicate);
    TestEqual(TEXT("Completed replay reports AlreadyFound"),
        Recovery.Feedback->GetFeedback(), EKalmalaDiscoveryFeedback::AlreadyFound);

    Recovery.Player->SetActorLocation(PostCollectionDescriptor.Location);
    const int32 PostCollectionRewardBefore =
        Recovery.Inventory->GetQuantity(PostCollectionDefinition->RewardItemId);
    const int32 WritesBeforePostCollectionClaim = WriteAttempts;
    TestTrue(TEXT("The same GameMode accepts a new discovery after full garbage collection"),
        Recovery.GameMode->ClaimOceanDiscovery(Recovery.Player, PostCollectionDescriptor));
    TestEqual(TEXT("The post-collection claim grants its exact reward once"),
        Recovery.Inventory->GetQuantity(PostCollectionDefinition->RewardItemId),
        PostCollectionRewardBefore + PostCollectionDefinition->RewardQuantity);
    const int32 WritesBeforePostCollectionReplay = WriteAttempts;
    TestFalse(TEXT("The post-collection discovery cannot be replayed"),
        Recovery.GameMode->ClaimOceanDiscovery(Recovery.Player, PostCollectionDescriptor));
    TestEqual(TEXT("Post-collection replay grants no extra reward"),
        Recovery.Inventory->GetQuantity(PostCollectionDefinition->RewardItemId),
        PostCollectionRewardBefore + PostCollectionDefinition->RewardQuantity);
    TestEqual(TEXT("Post-collection replay performs no extra save write"),
        WriteAttempts, WritesBeforePostCollectionReplay);
    TestEqual(TEXT("Post-collection discovery claim writes one pending and one completion record"),
        WriteAttempts, WritesBeforePostCollectionClaim + 2);
    return true;
}

#endif
