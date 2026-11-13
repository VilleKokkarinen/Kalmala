#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaBiomeContentContract.h"
#include "KalmalaBiomeExpansionContract.h"
#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaHazardSpawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationDepletionWriteRetryTest,
    "Kalmala.Gameplay.Population.DepletionWriteRetry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationDepletionWriteRetryTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population depletion retry world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Population depletion retry world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("World state accepts the deterministic fixture seed"),
            WorldState->SelectWorldSeedFromServer(418))) return false;
    const FKalmalaWorldGenerationConfig Config = WorldState->GetWorldGenerationConfig();

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population depletion retry GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig = Config;

    UKalmalaWorldPopulationSaveGame* PopulationSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Population depletion save fixture created"), PopulationSave)) return false;
    PopulationSave->InitializeForWorld(Config);
    Mode->PopulationSaveGame = PopulationSave;

    const auto BuildMaterializableDescriptors = [Config](const FIntPoint& Key,
        const EKalmalaWorldPopulationKind Kind)
    {
        TArray<FKalmalaWorldPopulationSpawn> Spawns =
            FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Config, Key, Kind);
        TArray<FKalmalaWorldPopulationSpawn> SaltSlots;
        if (Kind == EKalmalaWorldPopulationKind::HarvestNode)
        {
            SaltSlots = Spawns.FilterByPredicate([](const FKalmalaWorldPopulationSpawn& Spawn)
            {
                return Spawn.ContentId == TEXT("tundra-frost-salt-deposit")
                    || Spawn.ContentId == TEXT("gem-opal-rock");
            });
            Spawns.RemoveAll([](const FKalmalaWorldPopulationSpawn& Spawn)
            {
                return Spawn.ContentId == TEXT("tundra-frost-salt-deposit")
                    || Spawn.ContentId == TEXT("gem-opal-rock");
            });
        }

        const FVector2D KeyCenter = (FVector2D(Key) + FVector2D(0.5f, 0.5f))
            * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
        const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(Config, KeyCenter));
        if (KeyBiome == EKalmalaBiome::ShimmeringLakes || KeyBiome == EKalmalaBiome::Elderwood
            || KeyBiome == EKalmalaBiome::MossyMire || KeyBiome == EKalmalaBiome::FreezingTundra
            || KeyBiome == EKalmalaBiome::ThunderMountains)
        {
            Spawns.SetNum(FMath::Min(Spawns.Num(),
                FKalmalaBiomeExpansionContract::ApplyPopulationBudget(Spawns.Num(), Kind, KeyBiome)));
        }
        Spawns.Append(SaltSlots);
        return Spawns;
    };

    FKalmalaWorldPopulationSpawn HarvestDescriptor;
    FKalmalaWorldPopulationSpawn HazardDescriptor;
    bool bFoundHarvestDescriptor = false;
    bool bFoundHazardDescriptor = false;
    for (int32 Y = -128; Y <= 128 && (!bFoundHarvestDescriptor || !bFoundHazardDescriptor); ++Y)
    {
        for (int32 X = -128; X <= 128 && (!bFoundHarvestDescriptor || !bFoundHazardDescriptor); ++X)
        {
            const FIntPoint Key(X, Y);
            const FVector2D KeyCenter = (FVector2D(Key) + FVector2D(0.5f, 0.5f))
                * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
            if (!FKalmalaWorldBounds::Contains(Config, KeyCenter)) continue;

            if (!bFoundHarvestDescriptor)
            {
                const TArray<FKalmalaWorldPopulationSpawn> HarvestDescriptors =
                    BuildMaterializableDescriptors(Key, EKalmalaWorldPopulationKind::HarvestNode);
                const FKalmalaWorldPopulationSpawn* Candidate = HarvestDescriptors.FindByPredicate(
                    [](const FKalmalaWorldPopulationSpawn& Spawn)
                    {
                        return FKalmalaBiomeContentContract::IsHandGatherableSourceId(Spawn.ContentId);
                    });
                if (Candidate != nullptr)
                {
                    HarvestDescriptor = *Candidate;
                    bFoundHarvestDescriptor = true;
                }
            }

            if (!bFoundHazardDescriptor)
            {
                const TArray<FKalmalaWorldPopulationSpawn> HazardDescriptors =
                    BuildMaterializableDescriptors(Key, EKalmalaWorldPopulationKind::Hazard);
                if (!HazardDescriptors.IsEmpty())
                {
                    HazardDescriptor = HazardDescriptors[0];
                    bFoundHazardDescriptor = true;
                }
            }
        }
    }
    if (!TestTrue(TEXT("Seed 418 has a materializable hand-gatherable source"), bFoundHarvestDescriptor)
        || !TestTrue(TEXT("Seed 418 has a materializable hazard"), bFoundHazardDescriptor)) return false;

    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(
        AKalmalaCharacter::StaticClass(), HarvestDescriptor.Location, FRotator::ZeroRotator);
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Harvest owner character created"), Player)
        || !TestNotNull(TEXT("Harvest owner player state created"), PlayerState)) return false;
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("population-depletion-retry-owner"), FName(TEXT("KalmalaDepletionRetry")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    PlayerState->SetPlayerId(1);
    Player->SetPlayerState(PlayerState);
    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    UKalmalaPlayerDiscoverySaveGameV2* PlayerSave =
        NewObject<UKalmalaPlayerDiscoverySaveGameV2>(Mode);
    if (!TestNotNull(TEXT("Harvest owner save fixture created"), PlayerSave)) return false;
    PlayerSave->InitializeForPlayer(Config, PlayerIdentity);
    Mode->PlayerDiscoverySaves.Add(PlayerIdentity, PlayerSave);
    TArray<uint8> PlayerSaveBytes;
    if (!TestTrue(TEXT("Initial player state serializes before harvesting"),
            UGameplayStatics::SaveGameToMemory(PlayerSave, PlayerSaveBytes))) return false;
    const TArray<uint8> InitialPlayerSaveBytes = PlayerSaveBytes;
    Mode->PlayerDiscoverySaveWriterForTests = [&PlayerSaveBytes](
        UKalmalaPlayerDiscoverySaveGameV2* Candidate, const FString&, int32)
    {
        return UGameplayStatics::SaveGameToMemory(Candidate, PlayerSaveBytes);
    };

    AKalmalaHarvestNode* HarvestNode = World->SpawnActor<AKalmalaHarvestNode>(
        AKalmalaHarvestNode::StaticClass(), HarvestDescriptor.Location, FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Generated hand-gatherable node created"), HarvestNode)) return false;
    HarvestNode->InitializeServer(HarvestDescriptor);
    HarvestNode->OnHarvested.AddUObject(Mode, &AKalmalaGameMode::RecordHarvestedSpawn);
    const FString HarvestId = HarvestNode->GetPersistentSpawnId();
    const FName RewardItemId = HarvestNode->GetHarvestItemId();
    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    if (!TestTrue(TEXT("Generated harvest node has a stable identity"), !HarvestId.IsEmpty())
        || !TestFalse(TEXT("Generated source has its canonical primary reward"), RewardItemId.IsNone())
        || !TestNotNull(TEXT("Harvest owner inventory exists"), Inventory)) return false;
    const int32 RewardBefore = Inventory->GetQuantity(RewardItemId);

    TArray<uint8> DurablePopulationBytes;
    if (!TestTrue(TEXT("Initial empty population ledger serializes"),
            UGameplayStatics::SaveGameToMemory(PopulationSave, DurablePopulationBytes))) return false;
    int32 PopulationWriteAttempts = 0;
    bool bFailNextPopulationWrite = true;
    Mode->PopulationSaveWriteAttemptForTests = [&DurablePopulationBytes, &PopulationWriteAttempts,
        &bFailNextPopulationWrite](UKalmalaWorldPopulationSaveGame* Candidate, const FString&, int32)
    {
        ++PopulationWriteAttempts;
        if (bFailNextPopulationWrite)
        {
            bFailNextPopulationWrite = false;
            return false;
        }
        return UGameplayStatics::SaveGameToMemory(Candidate, DurablePopulationBytes);
    };

    TestFalse(TEXT("A failed depletion-slot write rejects the harvest before durable player grant"),
        HarvestNode->InteractWithToolIntentFromServer(Player, 100.0f, 250.0f, NAME_None,
            static_cast<uint8>(EKalmalaToolAction::Gathering)));
    TestFalse(TEXT("Rejected harvest is not queued as an accepted depletion"),
        Mode->PendingPopulationDepletionWriteIds.Contains(HarvestId));
    TestEqual(TEXT("Failed population write grants no harvest item"),
        Inventory->GetQuantity(RewardItemId), RewardBefore);
    TestFalse(TEXT("Failed population write rolls back the live sparse ledger"),
        PopulationSave->IsHarvested(HarvestId));
    TestFalse(TEXT("Failed population write leaves the source available"), HarvestNode->IsHarvested());
    TestEqual(TEXT("Harvest performs one failed immediate write"), PopulationWriteAttempts, 1);
    TestTrue(TEXT("Failed population write leaves the durable player save unchanged"),
        PlayerSaveBytes == InitialPlayerSaveBytes);

    const TArray<uint8> FailedHarvestBytes = DurablePopulationBytes;
    UKalmalaWorldPopulationSaveGame* FailedHarvestReload =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(FailedHarvestBytes));
    if (!TestNotNull(TEXT("Failed harvest leaves the prior durable save readable"), FailedHarvestReload)) return false;
    TestFalse(TEXT("Failed harvest does not claim durable depletion"), FailedHarvestReload->IsHarvested(HarvestId));

    int32 PlayerStateSaveGateCalls = 0;
    Mode->PlayerStateSaveGateForTests = [&PlayerStateSaveGateCalls]()
    {
        return ++PlayerStateSaveGateCalls != 1;
    };
    TestFalse(TEXT("A failed player-state write rejects the harvest after population prewrite"),
        HarvestNode->InteractWithToolIntentFromServer(Player, 100.0f, 250.0f, NAME_None,
            static_cast<uint8>(EKalmalaToolAction::Gathering)));
    TestEqual(TEXT("Player-state rollback is written after the injected save failure"),
        PlayerStateSaveGateCalls, 2);
    TestEqual(TEXT("Rejected player save grants no harvest item"),
        Inventory->GetQuantity(RewardItemId), RewardBefore);
    TestFalse(TEXT("Compensated player-save failure restores live population state"),
        PopulationSave->IsHarvested(HarvestId));
    TestFalse(TEXT("Compensated player-save failure leaves the source available"), HarvestNode->IsHarvested());
    TestFalse(TEXT("Compensated harvest leaves no pending depletion"),
        Mode->PendingPopulationDepletionWriteIds.Contains(HarvestId));
    TestEqual(TEXT("Population prewrite and compensation each write once"), PopulationWriteAttempts, 3);
    const TArray<uint8> CompensatedHarvestBytes = DurablePopulationBytes;
    UKalmalaWorldPopulationSaveGame* CompensatedHarvestReload =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(CompensatedHarvestBytes));
    if (!TestNotNull(TEXT("Compensated population save reloads from memory"), CompensatedHarvestReload)) return false;
    TestFalse(TEXT("Compensated player-save failure leaves no durable depletion"),
        CompensatedHarvestReload->IsHarvested(HarvestId));
    Mode->PlayerStateSaveGateForTests = TFunction<bool()>();

    TestTrue(TEXT("The same source succeeds once the population write becomes available"),
        HarvestNode->InteractWithToolIntentFromServer(Player, 100.0f, 250.0f, NAME_None,
            static_cast<uint8>(EKalmalaToolAction::Gathering)));
    TestTrue(TEXT("Successful harvest grants its primary item once"),
        Inventory->GetQuantity(RewardItemId) > RewardBefore);
    TestTrue(TEXT("Successful harvest remains depleted in the live sparse ledger"),
        PopulationSave->IsHarvested(HarvestId));
    TestEqual(TEXT("Successful harvest writes the population ledger after the injected failure"),
        PopulationWriteAttempts, 4);

    const int32 RewardAfterAcceptedHarvest = Inventory->GetQuantity(RewardItemId);
    TestFalse(TEXT("A durably harvested source cannot be accepted a second time"),
        HarvestNode->InteractWithToolIntentFromServer(Player, 100.0f, 250.0f, NAME_None,
            static_cast<uint8>(EKalmalaToolAction::Gathering)));
    TestEqual(TEXT("Rejected duplicate does not grant a second item"),
        Inventory->GetQuantity(RewardItemId), RewardAfterAcceptedHarvest);
    TestEqual(TEXT("Rejected duplicate does not issue another population write"), PopulationWriteAttempts, 4);

    UKalmalaWorldPopulationSaveGame* HarvestReload =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Successful harvest reloads from memory"), HarvestReload)) return false;
    TestTrue(TEXT("Successful harvest keeps the source depleted after reload"),
        HarvestReload->IsHarvested(HarvestId));

    AKalmalaHazardSpawn* Hazard = World->SpawnActor<AKalmalaHazardSpawn>(
        AKalmalaHazardSpawn::StaticClass(), HazardDescriptor.Location, FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Generated hazard fixture created"), Hazard)) return false;
    Hazard->InitializeServer(HazardDescriptor);
    Hazard->OnDefeated.AddUObject(Mode, &AKalmalaGameMode::RecordDefeatedSpawn);
    const FString HazardId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(HazardDescriptor);
    if (!TestTrue(TEXT("Generated hazard has a stable identity"), !HazardId.IsEmpty())) return false;

    bFailNextPopulationWrite = true;
    TestTrue(TEXT("The server accepts the hazard defeat despite a failed population write"),
        Hazard->DefeatServer());
    TestTrue(TEXT("Accepted hazard defeat remains scheduled until durable"),
        Mode->PendingPopulationDepletionWriteIds.Contains(HazardId));
    TestEqual(TEXT("Hazard defeat makes one failed immediate write"), PopulationWriteAttempts, 5);

    const TArray<uint8> FailedHazardBytes = DurablePopulationBytes;
    UKalmalaWorldPopulationSaveGame* FailedHazardReload =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(FailedHazardBytes));
    if (!TestNotNull(TEXT("Failed hazard write leaves the prior durable save readable"), FailedHazardReload)) return false;
    TestTrue(TEXT("Prior successful harvest remains durable after another write failure"),
        FailedHazardReload->IsHarvested(HarvestId));
    TestFalse(TEXT("Failed hazard write does not claim durable defeat"),
        FailedHazardReload->IsDefeated(HazardId));

    Mode->RetryPendingPopulationDepletionWrites(0.5);
    TestEqual(TEXT("Hazard persistence does not retry before its scheduled deadline"), PopulationWriteAttempts, 5);
    Mode->RetryPendingPopulationDepletionWrites(2.0);
    TestTrue(TEXT("Hazard retry clears its pending write after success"),
        Mode->PendingPopulationDepletionWriteIds.IsEmpty());
    TestEqual(TEXT("Hazard persistence retries until the stable ID is written"), PopulationWriteAttempts, 6);
    UKalmalaWorldPopulationSaveGame* FinalReload =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Successful hazard retry reloads from memory"), FinalReload)) return false;
    TestTrue(TEXT("Final save retains the successful harvest depletion"), FinalReload->IsHarvested(HarvestId));
    TestTrue(TEXT("Final save retains the successful hazard defeat"), FinalReload->IsDefeated(HazardId));

    UWorld* ReloadWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Fresh depletion reload world created"), ReloadWorld)) return false;
    ON_SCOPE_EXIT { ReloadWorld->DestroyWorld(false); };
    AKalmalaWorldGenerationGameState* ReloadWorldState =
        ReloadWorld->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Fresh depletion reload world state created"), ReloadWorldState)) return false;
    ReloadWorld->SetGameState(ReloadWorldState);
    if (!TestTrue(TEXT("Fresh reload uses the original immutable seed"),
            ReloadWorldState->SelectWorldSeedFromServer(Config.WorldSeed))) return false;
    AKalmalaGameMode* ReloadMode = ReloadWorld->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Fresh depletion reload GameMode created"), ReloadMode)) return false;
    ReloadMode->WorldGenerationConfig = Config;
    ReloadMode->PopulationSaveGame = FinalReload;
    ReloadMode->ActivatePopulationKey(HarvestDescriptor.SpatialKey);
    ReloadMode->ActivatePopulationKey(HazardDescriptor.SpatialKey);

    int32 RestoredHarvestActors = 0;
    for (TActorIterator<AKalmalaHarvestNode> It(ReloadWorld); It; ++It)
        RestoredHarvestActors += It->GetPersistentSpawnId() == HarvestId ? 1 : 0;
    int32 RestoredHazardActors = 0;
    for (TActorIterator<AKalmalaHazardSpawn> It(ReloadWorld); It; ++It)
        RestoredHazardActors += It->GetActorLocation().Equals(HazardDescriptor.Location, 0.1f) ? 1 : 0;
    TestEqual(TEXT("Fresh activation does not restore the durably harvested source"), RestoredHarvestActors, 0);
    TestEqual(TEXT("Fresh activation does not restore the durably defeated hazard"), RestoredHazardActors, 0);
    return true;
}

#endif
