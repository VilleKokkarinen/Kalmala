#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaM9ExplorationRewardRollbackTest,
    "Kalmala.Gameplay.M9.ExplorationRewardRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaM9ExplorationRewardRollbackTest::RunTest(const FString& Parameters)
{
    if (!TestNotNull(TEXT("Engine is available for an isolated M9 reward world"), GEngine)) return false;

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("M9 reward fixture world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Destroyed);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };

    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("M9 reward fixture world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("World state accepts the deterministic fixture seed"),
            WorldState->SelectWorldSeedFromServer(418))) return false;

    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Fixture selects the authoritative GameMode"), World->SetGameMode(FURL()))) return false;
    AKalmalaGameMode* GameMode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Authoritative GameMode created"), GameMode)) return false;

    const FKalmalaWorldGenerationConfig WorldConfig = WorldState->GetWorldGenerationConfig();
    GameMode->WorldGenerationConfig = WorldConfig;

    FKalmalaM9ExplorationRewardDescriptor Descriptor;
    bool bFoundDescriptor = false;
    for (int32 Y = -160; Y <= 160 && !bFoundDescriptor; ++Y)
    {
        for (int32 X = -160; X <= 160 && !bFoundDescriptor; ++X)
        {
            const TArray<FKalmalaM9ExplorationRewardDescriptor> Candidates =
                FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(WorldConfig, FIntPoint(X, Y));
            if (Candidates.IsEmpty()) continue;
            Descriptor = Candidates[0];
            bFoundDescriptor = true;
        }
    }
    if (!TestTrue(TEXT("Seed 418 has a canonical M9 exploration reward descriptor"), bFoundDescriptor)) return false;

    const FKalmalaM9ExplorationRewardDefinition* Definition =
        FKalmalaM9ExplorationRewardCatalogue::FindDefinition(Descriptor.CandidateId);
    if (!TestNotNull(TEXT("Canonical M9 descriptor resolves its reward definition"), Definition)) return false;
    const FString StableIdentity = FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
        Descriptor.CandidateId, Descriptor.SpatialKey);
    if (!TestTrue(TEXT("Canonical descriptor has a stable claim identity"), !StableIdentity.IsEmpty())) return false;

    GameMode->ActivePopulationSpatialKeys.Add(Descriptor.SpatialKey);
    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(
        AKalmalaCharacter::StaticClass(), Descriptor.Location, FRotator::ZeroRotator);
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Authenticated M9 player pawn created"), Player)
        || !TestNotNull(TEXT("Authenticated M9 PlayerState created"), PlayerState)) return false;
    Player->DispatchBeginPlay();
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("rf30-m9-exploration-owner"), FName(TEXT("KalmalaRF30Test")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    PlayerState->SetPlayerId(1);
    Player->SetPlayerState(PlayerState);
    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    const FString ExpectedSlot = FString::Printf(TEXT("KalmalaPlayerDiscoveries_%llu_%08x"),
        WorldConfig.WorldSeed, FCrc::StrCrc32(*PlayerIdentity));

    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    UKalmalaDiscoveryProgressComponent* Feedback = Player->GetDiscoveryProgressComponent();
    if (!TestNotNull(TEXT("M9 player inventory created"), Inventory)
        || !TestNotNull(TEXT("M9 player feedback component created"), Feedback)) return false;

    TArray<uint8> PersistedPlayerBytes;
    TArray<TPair<FName, int32>> PublishedOwnerReceipts;
    Inventory->GainReceiptPublicationForTests =
        [this, &PersistedPlayerBytes, &PublishedOwnerReceipts](const FName ItemId, const int32 Quantity)
    {
        TestTrue(TEXT("Owner gain receipt publishes only after player-save persistence succeeds"),
            !PersistedPlayerBytes.IsEmpty());
        PublishedOwnerReceipts.Emplace(ItemId, Quantity);
    };
    ON_SCOPE_EXIT
    {
        if (IsValid(Inventory)) Inventory->GainReceiptPublicationForTests = {};
    };

    int32 LoadAttempts = 0;
    int32 SaveGateAttempts = 0;
    int32 PlayerSaveWriteAttempts = 0;
    GameMode->PlayerDiscoverySaveLoaderForTests = [this, &LoadAttempts, ExpectedSlot](
        const FString& Slot, const int32 UserIndex, USaveGame*& OutSaveGame)
    {
        ++LoadAttempts;
        TestEqual(TEXT("M9 claim loads the exact authenticated owner slot"), Slot, ExpectedSlot);
        TestEqual(TEXT("M9 claim loads local user zero"), UserIndex, 0);
        OutSaveGame = nullptr;
        return false;
    };
    GameMode->PlayerStateSaveGateForTests = [&SaveGateAttempts]()
    {
        ++SaveGateAttempts;
        return SaveGateAttempts > 1;
    };
    GameMode->PlayerDiscoverySaveWriterForTests = [this, &PlayerSaveWriteAttempts,
        &PersistedPlayerBytes, ExpectedSlot](UKalmalaPlayerDiscoverySaveGameV2* Candidate,
            const FString& Slot, const int32 UserIndex)
    {
        ++PlayerSaveWriteAttempts;
        TestEqual(TEXT("M9 claim writes the exact authenticated owner slot"), Slot, ExpectedSlot);
        TestEqual(TEXT("M9 claim writes local user zero"), UserIndex, 0);
        return Candidate != nullptr && Slot == ExpectedSlot && UserIndex == 0
            && UGameplayStatics::SaveGameToMemory(Candidate, PersistedPlayerBytes);
    };

    const TArray<FKalmalaInventoryStack> InventoryBeforeClaim = Inventory->GetStacks();
    const int32 RewardQuantityBeforeClaim = Inventory->GetQuantity(Definition->RewardItemId);
    const auto HasSameInventoryState = [](const TArray<FKalmalaInventoryStack>& Current,
        const TArray<FKalmalaInventoryStack>& Expected)
    {
        if (Current.Num() != Expected.Num()) return false;
        for (int32 Index = 0; Index < Current.Num(); ++Index)
        {
            if (Current[Index].ItemId != Expected[Index].ItemId
                || Current[Index].Quantity != Expected[Index].Quantity
                || Current[Index].InstanceGuid != Expected[Index].InstanceGuid
                || Current[Index].ItemGuids != Expected[Index].ItemGuids) return false;
        }
        return true;
    };
    const uint32 FeedbackSerialBeforeFailure = Feedback->GetFeedbackSerial();
    TestFalse(TEXT("Rejected player-save write rejects the first M9 claim"),
        GameMode->ClaimM9ExplorationReward(Player, Descriptor));
    TestEqual(TEXT("Failed M9 persistence reports Unavailable to the owner"),
        Feedback->GetFeedback(), EKalmalaDiscoveryFeedback::Unavailable);
    TestEqual(TEXT("Failed M9 persistence publishes one owner result"),
        Feedback->GetFeedbackSerial(), FeedbackSerialBeforeFailure + 1);
    TestEqual(TEXT("Failed M9 persistence reports the save rejection"),
        Feedback->GetFeedbackLabel(), FString(TEXT("Land discovery could not be saved")));
    TestEqual(TEXT("The injected first write is rejected at the player-save gate"), SaveGateAttempts, 1);
    TestEqual(TEXT("Rejected player-save candidate never reaches the slot writer"), PlayerSaveWriteAttempts, 0);
    TestEqual(TEXT("Initial owner save slot is loaded once"), LoadAttempts, 1);
    TestTrue(TEXT("Rejected M9 claim restores every inventory stack and item identity"),
        HasSameInventoryState(Inventory->GetStacks(), InventoryBeforeClaim));
    TestEqual(TEXT("Rejected M9 claim grants no material"),
        Inventory->GetQuantity(Definition->RewardItemId), RewardQuantityBeforeClaim);
    TestTrue(TEXT("Rejected M9 claim publishes no owner gain receipt"), PublishedOwnerReceipts.IsEmpty());
    TestTrue(TEXT("Failed first attempt creates no durable player-save bytes"), PersistedPlayerBytes.IsEmpty());

    const TObjectPtr<UKalmalaPlayerDiscoverySaveGameV2>* CachedAfterFailure =
        GameMode->PlayerDiscoverySaves.Find(PlayerIdentity);
    TestTrue(TEXT("Failed first attempt leaves the cached player claim absent"),
        CachedAfterFailure != nullptr && CachedAfterFailure->Get() != nullptr
            && !CachedAfterFailure->Get()->HasDiscovery(StableIdentity)
            && CachedAfterFailure->Get()->GetM9ClaimIds().IsEmpty());
    const TSet<FString>* SessionClaimsAfterFailure = GameMode->SessionM9ExplorationClaims.Find(PlayerIdentity);
    TestTrue(TEXT("Failed first attempt does not consume a session claim slot"),
        SessionClaimsAfterFailure == nullptr || SessionClaimsAfterFailure->IsEmpty());

    const uint32 FeedbackSerialBeforeRetry = Feedback->GetFeedbackSerial();
    TestTrue(TEXT("The same canonical descriptor succeeds after persistence recovers"),
        GameMode->ClaimM9ExplorationReward(Player, Descriptor));
    TestEqual(TEXT("Successful retry grants the exact catalogue quantity"),
        Inventory->GetQuantity(Definition->RewardItemId), RewardQuantityBeforeClaim + Definition->RewardQuantity);
    TestEqual(TEXT("Successful retry reports LandmarkFound to the owner"),
        Feedback->GetFeedback(), EKalmalaDiscoveryFeedback::LandmarkFound);
    TestEqual(TEXT("Successful retry publishes one result after the failure"),
        Feedback->GetFeedbackSerial(), FeedbackSerialBeforeRetry + 1);
    TestEqual(TEXT("Only the recovered player-save write reaches the slot writer"), PlayerSaveWriteAttempts, 1);
    TestEqual(TEXT("Successful retry publishes exactly one owner gain receipt"), PublishedOwnerReceipts.Num(), 1);
    if (PublishedOwnerReceipts.Num() == 1)
    {
        TestEqual(TEXT("Owner receipt identifies the canonical M9 reward"),
            PublishedOwnerReceipts[0].Key, Definition->RewardItemId);
        TestEqual(TEXT("Owner receipt reports the exact catalogue quantity"),
            PublishedOwnerReceipts[0].Value, Definition->RewardQuantity);
    }
    TestTrue(TEXT("Successful retry creates durable player-save bytes"), !PersistedPlayerBytes.IsEmpty());

    UKalmalaPlayerDiscoverySaveGameV2* PersistedSave = Cast<UKalmalaPlayerDiscoverySaveGameV2>(
        UGameplayStatics::LoadGameFromMemory(PersistedPlayerBytes));
    if (!TestNotNull(TEXT("Recovered M9 player slot reloads"), PersistedSave)) return false;
    TestTrue(TEXT("Durable player save contains exactly this canonical M9 claim"),
        PersistedSave->MatchesPlayer(WorldConfig, PlayerIdentity)
            && PersistedSave->HasDiscovery(StableIdentity)
            && PersistedSave->GetM9ClaimIds().Num() == 1
            && PersistedSave->GetM9ClaimIds().Contains(StableIdentity));
    const TSet<FString>* SessionClaimsAfterRetry = GameMode->SessionM9ExplorationClaims.Find(PlayerIdentity);
    TestTrue(TEXT("Successful retry consumes exactly one session claim slot"),
        SessionClaimsAfterRetry != nullptr && SessionClaimsAfterRetry->Num() == 1
            && SessionClaimsAfterRetry->Contains(StableIdentity));

    const TArray<FKalmalaInventoryStack> InventoryAfterRetry = Inventory->GetStacks();
    const int32 WritesBeforeReplay = PlayerSaveWriteAttempts;
    const int32 GateAttemptsBeforeReplay = SaveGateAttempts;
    const uint32 FeedbackSerialBeforeReplay = Feedback->GetFeedbackSerial();
    TestFalse(TEXT("A later replay of the completed M9 claim is rejected"),
        GameMode->ClaimM9ExplorationReward(Player, Descriptor));
    TestEqual(TEXT("Completed replay reports AlreadyFound to the owner"),
        Feedback->GetFeedback(), EKalmalaDiscoveryFeedback::AlreadyFound);
    TestEqual(TEXT("Completed replay publishes one owner result"),
        Feedback->GetFeedbackSerial(), FeedbackSerialBeforeReplay + 1);
    TestEqual(TEXT("Completed replay grants no extra reward"),
        Inventory->GetQuantity(Definition->RewardItemId), RewardQuantityBeforeClaim + Definition->RewardQuantity);
    TestEqual(TEXT("Completed replay publishes no duplicate owner gain receipt"), PublishedOwnerReceipts.Num(), 1);
    TestTrue(TEXT("Completed replay preserves every granted inventory identity"),
        HasSameInventoryState(Inventory->GetStacks(), InventoryAfterRetry));
    TestEqual(TEXT("Completed replay performs no save attempt"), PlayerSaveWriteAttempts, WritesBeforeReplay);
    TestEqual(TEXT("Completed replay does not consume another save-gate attempt"),
        SaveGateAttempts, GateAttemptsBeforeReplay);
    const TSet<FString>* SessionClaimsAfterReplay = GameMode->SessionM9ExplorationClaims.Find(PlayerIdentity);
    TestTrue(TEXT("Completed replay retains exactly one session claim"),
        SessionClaimsAfterReplay != nullptr && SessionClaimsAfterReplay->Num() == 1
            && SessionClaimsAfterReplay->Contains(StableIdentity));
    return true;
}

#endif
