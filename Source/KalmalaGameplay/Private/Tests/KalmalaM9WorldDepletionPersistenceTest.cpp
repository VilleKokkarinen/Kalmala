#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaM9WorldDepletionPersistenceTest,
    "Kalmala.Gameplay.M9.WorldDepletionPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaM9WorldDepletionPersistenceTest::RunTest(const FString& Parameters)
{
    const auto CreateWorldAndMode = [this](const uint64 Seed, UWorld*& OutWorld,
        FKalmalaWorldGenerationConfig& OutConfig) -> AKalmalaGameMode*
    {
        OutWorld = UWorld::CreateWorld(EWorldType::Game, false);
        if (!TestNotNull(TEXT("M9 persistence fixture world created"), OutWorld)) return nullptr;
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(OutWorld);

        AKalmalaWorldGenerationGameState* WorldState =
            OutWorld->SpawnActor<AKalmalaWorldGenerationGameState>();
        if (!TestNotNull(TEXT("M9 persistence fixture world state created"), WorldState)) return nullptr;
        OutWorld->SetGameState(WorldState);
        if (!TestTrue(TEXT("World state accepts its fixture seed"),
                WorldState->SelectWorldSeedFromServer(Seed))) return nullptr;

        OutWorld->SetGameInstance(NewObject<UGameInstance>(GEngine));
        OutWorld->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
        if (!TestTrue(TEXT("M9 persistence fixture selects its authoritative GameMode"),
                OutWorld->SetGameMode(FURL()))) return nullptr;

        AKalmalaGameMode* Mode = OutWorld->GetAuthGameMode<AKalmalaGameMode>();
        if (!TestNotNull(TEXT("M9 persistence fixture GameMode created"), Mode)) return nullptr;
        OutConfig = WorldState->GetWorldGenerationConfig();
        Mode->WorldGenerationConfig = OutConfig;

        UKalmalaWorldPopulationSaveGame* PopulationSave =
            NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
        if (!TestNotNull(TEXT("M9 persistence population save created"), PopulationSave)) return nullptr;
        PopulationSave->InitializeForWorld(OutConfig);
        Mode->PopulationSaveGame = PopulationSave;
        return Mode;
    };

    UWorld* World = nullptr;
    FKalmalaWorldGenerationConfig Config;
    AKalmalaGameMode* Mode = CreateWorldAndMode(418, World, Config);
    ON_SCOPE_EXIT
    {
        if (World)
        {
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }
    };
    if (!TestNotNull(TEXT("Primary M9 persistence GameMode available"), Mode)) return false;

    FKalmalaWorldPopulationSpawn SaltDescriptor;
    bool bFoundSalt = false;
    for (int32 Y = -128; Y <= 128 && !bFoundSalt; ++Y)
    {
        for (int32 X = -128; X <= 128 && !bFoundSalt; ++X)
        {
            const FIntPoint Key(X, Y);
            for (const FKalmalaWorldPopulationSpawn& Spawn :
                FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                    Config, Key, EKalmalaWorldPopulationKind::HarvestNode))
            {
                if (Spawn.ContentId != TEXT("tundra-frost-salt-deposit")) continue;
                SaltDescriptor = Spawn;
                bFoundSalt = true;
                break;
            }
        }
    }
    if (!TestTrue(TEXT("Seed 418 has a generated, harvestable M9 salt source"), bFoundSalt)) return false;

    const FKalmalaM7SaveIdentity WorldIdentity = FKalmalaM7SaveIdentity::ForWorld(Config.WorldSeed);
    const FString StableResourceId = FKalmalaM9SourceLootContract::BuildResourceDepletionId(
        SaltDescriptor.ContentId, FKalmalaWorldPopulationLayout::GetPersistentSpawnId(SaltDescriptor));
    if (!TestTrue(TEXT("Generated M9 source has its stable sparse identity"),
            FKalmalaM7SparseDelta::IsValidStableId(StableResourceId))) return false;

    TArray<uint8> DurableM9Bytes;
    TArray<FString> PersistenceOrder;
    int32 M9WriteAttempts = 0;
    bool bFailNextM9Write = true;
    const FString ExpectedM9Slot = FString::Printf(TEXT("KalmalaM9ResourceDepletions_%llu_%d"),
        WorldIdentity.WorldSeed, WorldIdentity.GeneratorRevision);
    Mode->M9ResourceSaveLoaderForTests = [](const FString&, int32)
    {
        return static_cast<UKalmalaM7PersistenceSaveGame*>(nullptr);
    };
    Mode->LoadM9ResourceDepletionSave();
    if (!TestTrue(TEXT("Initial empty M9 ledger serializes in memory"),
            UGameplayStatics::SaveGameToMemory(Mode->M9ResourceDepletionSaveGame, DurableM9Bytes))) return false;
    Mode->M9ResourceSaveWriteAttemptForTests = [this, &DurableM9Bytes, &PersistenceOrder, &M9WriteAttempts,
        &bFailNextM9Write, ExpectedM9Slot](UKalmalaM7PersistenceSaveGame* Candidate,
            const FString& Slot, const int32 UserIndex)
    {
        ++M9WriteAttempts;
        PersistenceOrder.Add(TEXT("M9"));
        TestEqual(TEXT("M9 depletion uses its seed and revision scoped slot"), Slot, ExpectedM9Slot);
        TestEqual(TEXT("M9 depletion writes as local user zero"), UserIndex, 0);
        if (bFailNextM9Write)
        {
            bFailNextM9Write = false;
            return false;
        }
        return UGameplayStatics::SaveGameToMemory(Candidate, DurableM9Bytes);
    };

    Mode->ActivatePopulationKey(SaltDescriptor.SpatialKey);
    AKalmalaHarvestNode* SaltNode = nullptr;
    for (TActorIterator<AKalmalaHarvestNode> It(World); It; ++It)
    {
        if (It->GetPersistentSpawnId() == FKalmalaWorldPopulationLayout::GetPersistentSpawnId(SaltDescriptor))
        {
            SaltNode = *It;
            break;
        }
    }
    if (!TestNotNull(TEXT("Normal GameMode activation materializes the generated salt source"), SaltNode)) return false;

    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(
        AKalmalaCharacter::StaticClass(), SaltDescriptor.Location, FRotator::ZeroRotator);
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("M9 persistence player created"), Player)
        || !TestNotNull(TEXT("M9 persistence owner state created"), PlayerState)) return false;
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("m9-world-depletion-owner"), FName(TEXT("KalmalaM9WorldDepletion")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    PlayerState->SetPlayerId(1);
    Player->SetPlayerState(PlayerState);
    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();

    UKalmalaPlayerDiscoverySaveGameV2* PlayerSave =
        NewObject<UKalmalaPlayerDiscoverySaveGameV2>(Mode);
    if (!TestNotNull(TEXT("M9 persistence player save fixture created"), PlayerSave)) return false;
    PlayerSave->InitializeForPlayer(Config, PlayerIdentity);
    Mode->PlayerDiscoverySaves.Add(PlayerIdentity, PlayerSave);
    TArray<uint8> PlayerSaveBytes;
    if (!TestTrue(TEXT("Initial player save serializes in memory"),
            UGameplayStatics::SaveGameToMemory(PlayerSave, PlayerSaveBytes))) return false;
    const TArray<uint8> InitialPlayerSaveBytes = PlayerSaveBytes;
    int32 PlayerSaveWrites = 0;
    Mode->PlayerDiscoverySaveWriterForTests = [&PlayerSaveBytes, &PlayerSaveWrites, &PersistenceOrder](
        UKalmalaPlayerDiscoverySaveGameV2* Candidate, const FString&, int32)
    {
        ++PlayerSaveWrites;
        PersistenceOrder.Add(TEXT("Player"));
        return UGameplayStatics::SaveGameToMemory(Candidate, PlayerSaveBytes);
    };

    UKalmalaSkillProgressionComponent* SkillProgression =
        Player->FindComponentByClass<UKalmalaSkillProgressionComponent>();
    if (!TestNotNull(TEXT("M9 persistence player skill component exists"), SkillProgression)) return false;
    const_cast<FKalmalaSkillProgressionLedger&>(SkillProgression->GetServerLedger()).Initialize();
    TArray<FKalmalaToolState>& CarriedTools =
        const_cast<TArray<FKalmalaToolState>&>(Player->GetCarriedToolInventory());
    const auto* StonePickDefinition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StonePick"));
    if (!TestNotNull(TEXT("Server Stone Pick definition exists"), StonePickDefinition)) return false;
    FKalmalaToolState& PickState = CarriedTools.AddDefaulted_GetRef();
    PickState.ItemId = TEXT("StonePick");
    PickState.InstanceGuid = FGuid::NewGuid();
    PickState.ToolLevel = 1;
    PickState.Durability = StonePickDefinition->MaxDurability;

    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    if (!TestNotNull(TEXT("M9 persistence player inventory exists"), Inventory)) return false;
    const FName RewardItemId = SaltNode->GetHarvestItemId();
    const int32 RewardBefore = Inventory->GetQuantity(RewardItemId);
    const int32 DurabilityBefore = Player->GetToolDurability(TEXT("StonePick"));

    TestFalse(TEXT("Failed M9 depletion write rejects the real generated-source harvest"),
        SaltNode->InteractWithToolIntentFromServer(Player, 100.0f, 250.0f,
            TEXT("StonePick"), static_cast<uint8>(EKalmalaToolAction::Mining)));
    TestEqual(TEXT("Failed M9 write grants no live reward"), Inventory->GetQuantity(RewardItemId), RewardBefore);
    TestEqual(TEXT("Failed M9 write restores the tool condition"),
        Player->GetToolDurability(TEXT("StonePick")), DurabilityBefore);
    TestFalse(TEXT("Failed M9 write leaves the source available"), SaltNode->IsHarvested());
    TestEqual(TEXT("Failed M9 write never reaches durable player persistence"), PlayerSaveWrites, 0);
    TestTrue(TEXT("Failed M9 write preserves the exact durable player save"),
        PlayerSaveBytes == InitialPlayerSaveBytes);
    TestFalse(TEXT("Failed M9 write leaves no depletion in the in-memory ledger"),
        Mode->HasM9ResourceDepletionFromServer(StableResourceId));
    UKalmalaM7PersistenceSaveGame* FailedM9Reload =
        Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(DurableM9Bytes));
    if (!TestNotNull(TEXT("Failed M9 write leaves the prior world ledger readable"), FailedM9Reload)) return false;
    TestFalse(TEXT("Failed M9 write leaves no durable resource delta"),
        FailedM9Reload->HasSparseDelta(EKalmalaM7SparseDeltaKind::ResourceDepleted, StableResourceId));
    TestEqual(TEXT("The injected M9 failure was attempted once"), M9WriteAttempts, 1);

    bFailNextM9Write = false;
    TestTrue(TEXT("The same generated source harvest succeeds after its writer recovers"),
        SaltNode->InteractWithToolIntentFromServer(Player, 100.0f, 250.0f,
            TEXT("StonePick"), static_cast<uint8>(EKalmalaToolAction::Mining)));
    TestTrue(TEXT("Successful M9 harvest grants its server-selected reward"),
        Inventory->GetQuantity(RewardItemId) > RewardBefore);
    TestTrue(TEXT("Successful M9 harvest is marked harvested"), SaltNode->IsHarvested());
    TestTrue(TEXT("Successful M9 harvest stores exactly one ResourceDepleted delta"),
        Mode->M9ResourceDepletionSaveGame->GetSparseDeltaCount() == 1
            && Mode->HasM9ResourceDepletionFromServer(StableResourceId));
    TestEqual(TEXT("Successful M9 harvest performs one durable player write"), PlayerSaveWrites, 1);
    TestEqual(TEXT("Failed and successful harvests each write once"), M9WriteAttempts, 2);
    TestTrue(TEXT("The successful world depletion write precedes the durable player reward"),
        PersistenceOrder.Num() == 3 && PersistenceOrder[0] == TEXT("M9")
            && PersistenceOrder[1] == TEXT("M9") && PersistenceOrder[2] == TEXT("Player"));

    UKalmalaM7PersistenceSaveGame* DurableM9Save =
        Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(DurableM9Bytes));
    if (!TestNotNull(TEXT("Successful M9 world ledger reloads from its serialized bytes"), DurableM9Save)) return false;
    TestTrue(TEXT("Reloaded M9 ledger matches the exact world identity"), DurableM9Save->Matches(WorldIdentity));
    TestTrue(TEXT("Reloaded M9 ledger contains the exact resource depletion"),
        DurableM9Save->HasSparseDelta(EKalmalaM7SparseDeltaKind::ResourceDepleted, StableResourceId));
    TestEqual(TEXT("Reloaded M9 ledger contains one delta"), DurableM9Save->GetSparseDeltaCount(), 1);

    FKalmalaM7SaveIdentity OtherSeedIdentity = FKalmalaM7SaveIdentity::ForWorld(Config.WorldSeed + 1);
    TestFalse(TEXT("Another world seed cannot inherit the M9 ledger"), DurableM9Save->Matches(OtherSeedIdentity));
    FKalmalaM7SaveIdentity OtherRevisionIdentity = WorldIdentity;
    ++OtherRevisionIdentity.GeneratorRevision;
    TestFalse(TEXT("Another generator revision cannot inherit the M9 ledger"),
        DurableM9Save->Matches(OtherRevisionIdentity));

    UWorld* ReloadWorld = nullptr;
    FKalmalaWorldGenerationConfig ReloadConfig;
    AKalmalaGameMode* ReloadMode = CreateWorldAndMode(418, ReloadWorld, ReloadConfig);
    ON_SCOPE_EXIT
    {
        if (ReloadWorld)
        {
            ReloadWorld->DestroyWorld(false);
            GEngine->DestroyWorldContext(ReloadWorld);
        }
    };
    if (!TestNotNull(TEXT("Fresh same-world GameMode created"), ReloadMode)) return false;
    ReloadMode->M9ResourceSaveLoaderForTests = [&DurableM9Bytes, ExpectedM9Slot, this](
        const FString& Slot, const int32 UserIndex)
    {
        TestEqual(TEXT("Fresh load uses the exact world ledger slot"), Slot, ExpectedM9Slot);
        TestEqual(TEXT("Fresh load uses local user zero"), UserIndex, 0);
        return Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(DurableM9Bytes));
    };
    ReloadMode->LoadM9ResourceDepletionSave();
    TestTrue(TEXT("Fresh same-world load accepts the saved depletion"),
        ReloadMode->HasM9ResourceDepletionFromServer(StableResourceId));
    ReloadMode->ActivatePopulationKey(SaltDescriptor.SpatialKey);
    int32 RestoredMatchingSources = 0;
    for (TActorIterator<AKalmalaHarvestNode> It(ReloadWorld); It; ++It)
        RestoredMatchingSources += It->GetPersistentSpawnId()
            == FKalmalaWorldPopulationLayout::GetPersistentSpawnId(SaltDescriptor) ? 1 : 0;
    TestEqual(TEXT("Fresh activation suppresses the durably depleted generated source"),
        RestoredMatchingSources, 0);

    UWorld* OtherWorld = nullptr;
    FKalmalaWorldGenerationConfig OtherConfig;
    AKalmalaGameMode* OtherMode = CreateWorldAndMode(Config.WorldSeed + 1, OtherWorld, OtherConfig);
    ON_SCOPE_EXIT
    {
        if (OtherWorld)
        {
            OtherWorld->DestroyWorld(false);
            GEngine->DestroyWorldContext(OtherWorld);
        }
    };
    if (!TestNotNull(TEXT("Independent-seed GameMode created"), OtherMode)) return false;
    OtherMode->M9ResourceSaveLoaderForTests = [&DurableM9Bytes](const FString&, int32)
    {
        return Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(DurableM9Bytes));
    };
    OtherMode->LoadM9ResourceDepletionSave();
    TestFalse(TEXT("A loaded M9 ledger from another seed fails closed"),
        OtherMode->bM9ResourceDepletionPersistenceWritable);
    TestNull(TEXT("Mismatched M9 data is not installed into the other world"),
        OtherMode->M9ResourceDepletionSaveGame.Get());

    OtherMode->M9ResourceSaveLoaderForTests = [](const FString&, int32)
    {
        return static_cast<UKalmalaM7PersistenceSaveGame*>(nullptr);
    };
    OtherMode->LoadM9ResourceDepletionSave();
    TestTrue(TEXT("Independent seed receives its own empty writable M9 ledger"),
        OtherMode->bM9ResourceDepletionPersistenceWritable
            && OtherMode->M9ResourceDepletionSaveGame->Matches(
                FKalmalaM7SaveIdentity::ForWorld(OtherConfig.WorldSeed)));
    TestFalse(TEXT("Independent seed does not contain the original source ID"),
        OtherMode->HasM9ResourceDepletionFromServer(StableResourceId));

    return true;
}

#endif
