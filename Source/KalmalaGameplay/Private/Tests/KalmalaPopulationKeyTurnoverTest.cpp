#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaCharacter.h"
#include "KalmalaDiscoveryActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationKeyTurnoverTest,
    "Kalmala.Gameplay.Population.KeyTurnover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationKeyTurnoverTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population key-turnover world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Population key-turnover world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("World state accepts the deterministic fixture seed"),
            WorldState->SelectWorldSeedFromServer(418))) return false;

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population key-turnover GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig = WorldState->GetWorldGenerationConfig();

    UKalmalaWorldPopulationSaveGame* PopulationSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Population key-turnover save created"), PopulationSave)) return false;
    PopulationSave->InitializeForWorld(Mode->WorldGenerationConfig);
    Mode->PopulationSaveGame = PopulationSave;

    FIntPoint M9Key = FIntPoint::ZeroValue;
    FKalmalaM9ExplorationRewardDescriptor M9Descriptor;
    bool bFoundThunderM9Descriptor = false;
    for (int32 Y = -160; Y <= 160 && !bFoundThunderM9Descriptor; ++Y)
    {
        for (int32 X = -160; X <= 160 && !bFoundThunderM9Descriptor; ++X)
        {
            const FIntPoint Key(X, Y);
            const FVector2D KeyCenter = (FVector2D(Key) + FVector2D(0.5f, 0.5f))
                * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
            if (!FKalmalaWorldBounds::Contains(Mode->WorldGenerationConfig, KeyCenter)
                || FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Mode->WorldGenerationConfig, KeyCenter))
                    != EKalmalaBiome::ThunderMountains)
            {
                continue;
            }

            const TArray<FKalmalaM9ExplorationRewardDescriptor> Descriptors =
                FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(Mode->WorldGenerationConfig, Key);
            if (Descriptors.IsEmpty()) continue;
            M9Key = Key;
            M9Descriptor = Descriptors[0];
            bFoundThunderM9Descriptor = true;
        }
    }
    if (!TestTrue(TEXT("Seed 418 has an eligible canonical Thunder Mountains M9 reward"),
            bFoundThunderM9Descriptor)) return false;

    APlayerController* Controller = World->SpawnActor<APlayerController>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    FActorSpawnParameters PawnParameters;
    PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
        M9Descriptor.Location, FRotator::ZeroRotator, PawnParameters);
    if (!TestNotNull(TEXT("Key-turnover player controller created"), Controller)
        || !TestNotNull(TEXT("Key-turnover player state created"), PlayerState)
        || !TestNotNull(TEXT("Key-turnover player character created"), Player)) return false;

    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("population-key-turnover-owner"), FName(TEXT("KalmalaKeyTurnover")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    PlayerState->SetPlayerId(1);
    Controller->SetPlayerState(PlayerState);
    Controller->Possess(Player);
    Player->SetPlayerState(PlayerState);
    World->AddController(Controller);
    if (!TestTrue(TEXT("Key-turnover player is possessed by the server controller"),
            Player->GetController() == Controller)) return false;

    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    UKalmalaPlayerDiscoverySaveGameV2* PlayerSave = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(Mode);
    if (!TestNotNull(TEXT("M9 owner save fixture created"), PlayerSave)) return false;
    PlayerSave->InitializeForPlayer(Mode->WorldGenerationConfig, PlayerIdentity);
    Mode->PlayerDiscoverySaves.Add(PlayerIdentity, PlayerSave);

    TArray<uint8> PlayerSaveBytes;
    int32 PlayerSaveWrites = 0;
    Mode->PlayerDiscoverySaveWriterForTests = [&PlayerSaveBytes, &PlayerSaveWrites](
        UKalmalaPlayerDiscoverySaveGameV2* Candidate, const FString&, int32)
    {
        ++PlayerSaveWrites;
        return UGameplayStatics::SaveGameToMemory(Candidate, PlayerSaveBytes);
    };

    const auto CountM9Actors = [World](const FKalmalaM9ExplorationRewardDescriptor& Descriptor)
    {
        int32 Count = 0;
        for (TActorIterator<AKalmalaDiscoveryActor> It(World); It; ++It)
        {
            const FKalmalaM9ExplorationRewardDescriptor& ActorDescriptor = It->GetM9ExplorationRewardDescriptor();
            if (ActorDescriptor.CandidateId == Descriptor.CandidateId
                && ActorDescriptor.SpatialKey == Descriptor.SpatialKey)
            {
                ++Count;
            }
        }
        return Count;
    };
    const auto CountHarvestNodes = [World](const FString& PersistentSpawnId)
    {
        int32 Count = 0;
        for (TActorIterator<AKalmalaHarvestNode> It(World); It; ++It)
        {
            if (It->GetPersistentSpawnId() == PersistentSpawnId) ++Count;
        }
        return Count;
    };

    Mode->ActivatePopulationKey(M9Key);
    TestEqual(TEXT("Normal key activation materializes the canonical M9 reward actor"),
        CountM9Actors(M9Descriptor), 1);
    const FKalmalaM9ExplorationRewardDefinition* RewardDefinition =
        FKalmalaM9ExplorationRewardCatalogue::FindDefinition(M9Descriptor.CandidateId);
    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    if (!TestNotNull(TEXT("Canonical M9 reward definition exists"), RewardDefinition)
        || !TestNotNull(TEXT("Key-turnover player inventory exists"), Inventory)) return false;
    Player->SetActorLocation(M9Descriptor.Location, false);
    const int32 RewardQuantityBefore = Inventory->GetQuantity(RewardDefinition->RewardItemId);
    const FString M9ClaimId = FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
        M9Descriptor.CandidateId, M9Descriptor.SpatialKey);
    if (!TestTrue(TEXT("The owner accepts the canonical M9 claim before traversal"),
            Mode->ClaimM9ExplorationReward(Player, M9Descriptor))) return false;
    TestEqual(TEXT("Accepted M9 claim grants its canonical quantity"),
        Inventory->GetQuantity(RewardDefinition->RewardItemId),
        RewardQuantityBefore + RewardDefinition->RewardQuantity);
    TestEqual(TEXT("Accepted M9 claim uses one isolated in-memory player-save write"), PlayerSaveWrites, 1);
    TestTrue(TEXT("Accepted M9 claim was serialized in memory"), !PlayerSaveBytes.IsEmpty());

    FString DepletedSourceId;
    for (TActorIterator<AKalmalaHarvestNode> It(World); It; ++It)
    {
        if (FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(It->GetActorLocation())) == M9Key
            && !It->GetPersistentSpawnId().IsEmpty() && It->GetM9ResourceDepletionId().IsEmpty())
        {
            DepletedSourceId = It->GetPersistentSpawnId();
            break;
        }
    }
    if (!TestTrue(TEXT("Thunder Mountains activation materializes a persistent harvest source"),
            !DepletedSourceId.IsEmpty())) return false;

    TArray<uint8> PopulationSaveBytes;
    int32 PopulationSaveWrites = 0;
    bool bPopulationSaveSerialized = false;
    Mode->PopulationSaveWriterForTests = [&PopulationSaveBytes, &PopulationSaveWrites, &bPopulationSaveSerialized](
        UKalmalaWorldPopulationSaveGame* Candidate, const FString&, int32)
    {
        ++PopulationSaveWrites;
        bPopulationSaveSerialized = UGameplayStatics::SaveGameToMemory(Candidate, PopulationSaveBytes);
    };
    Mode->RecordHarvestedSpawn(DepletedSourceId);
    TestTrue(TEXT("The real GameMode harvest callback records sparse depletion"),
        PopulationSave->IsHarvested(DepletedSourceId));
    TestEqual(TEXT("Harvest callback persists through the isolated in-memory writer"), PopulationSaveWrites, 1);
    TestTrue(TEXT("Harvest callback serialized the population save to memory"), bPopulationSaveSerialized);

    TArray<FIntPoint> InitialKeys;
    InitialKeys.Add(M9Key);
    FIntPoint TenthKey = FIntPoint::ZeroValue;
    bool bFoundTenthKey = false;
    for (int32 Y = -160; Y <= 160 && !bFoundTenthKey; ++Y)
    {
        for (int32 X = -160; X <= 160 && !bFoundTenthKey; ++X)
        {
            const FIntPoint Key(X, Y);
            const int32 FromM9 = FMath::Max(FMath::Abs(X - M9Key.X), FMath::Abs(Y - M9Key.Y));
            const FVector2D Center = (FVector2D(Key) + FVector2D(0.5f, 0.5f))
                * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
            if (FromM9 >= 6 && FKalmalaWorldBounds::Contains(Mode->WorldGenerationConfig, Center)
                && !FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Mode->WorldGenerationConfig,
                    Key, EKalmalaWorldPopulationKind::HarvestNode).IsEmpty())
            {
                TenthKey = Key;
                bFoundTenthKey = true;
            }
        }
    }
    if (!TestTrue(TEXT("A distant tenth in-bounds population key exists"), bFoundTenthKey)) return false;

    for (int32 Y = -160; Y <= 160 && InitialKeys.Num() < 9; ++Y)
    {
        for (int32 X = -160; X <= 160 && InitialKeys.Num() < 9; ++X)
        {
            const FIntPoint Key(X, Y);
            const FVector2D Center = (FVector2D(Key) + FVector2D(0.5f, 0.5f))
                * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
            const int32 FromM9 = FMath::Max(FMath::Abs(X - M9Key.X), FMath::Abs(Y - M9Key.Y));
            const int32 FromTenth = FMath::Max(FMath::Abs(X - TenthKey.X), FMath::Abs(Y - TenthKey.Y));
            if (Key == M9Key || !FKalmalaWorldBounds::Contains(Mode->WorldGenerationConfig, Center)
                || FromM9 < 3 || FromTenth < 3)
            {
                continue;
            }
            InitialKeys.Add(Key);
        }
    }
    if (!TestEqual(TEXT("Fixture selected nine distant initial keys"), InitialKeys.Num(), 9)) return false;

    for (int32 KeyIndex = 1; KeyIndex < InitialKeys.Num(); ++KeyIndex)
    {
        Mode->ActivatePopulationKey(InitialKeys[KeyIndex]);
    }
    TestEqual(TEXT("Nine keys materialize through normal GameMode activation"),
        Mode->ActivePopulationSpatialKeys.Num(), 9);
    TestTrue(TEXT("The selected harvest source is materialized exactly once"),
        CountHarvestNodes(DepletedSourceId) == 1);

    FString CooldownSpawnId;
    for (const FIntPoint& Key : InitialKeys)
    {
        const TArray<FKalmalaWorldPopulationSpawn> WildlifeDescriptors =
            FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Mode->WorldGenerationConfig,
                Key, EKalmalaWorldPopulationKind::Wildlife);
        if (!WildlifeDescriptors.IsEmpty())
        {
            CooldownSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(WildlifeDescriptors[0]);
            break;
        }
    }
    if (!TestTrue(TEXT("An active key has a stable wildlife slot for cooldown retention"),
            !CooldownSpawnId.IsEmpty())) return false;
    Mode->PopulationCreatureReactivationDueTimes.Add(CooldownSpawnId, 300.0);
    Mode->UpdatePopulationKeySchedule(1.0);
    TestTrue(TEXT("The initial supported M9 key receives scheduler state"),
        Mode->ActivePopulationSchedulerKeys.Contains(M9Key));

    const FVector2D TenthCenter = (FVector2D(TenthKey) + FVector2D(0.5f, 0.5f))
        * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
    Player->SetActorLocation(FVector(TenthCenter.X, TenthCenter.Y, 0.0), false);
    Mode->ActivatePopulationKey(TenthKey);
    TestTrue(TEXT("The player's new key activates after the nine-key cap"),
        Mode->ActivePopulationSpatialKeys.Contains(TenthKey));
    TestTrue(TEXT("Key turnover never exceeds the existing nine-key cap"),
        Mode->ActivePopulationSpatialKeys.Num() <= 9);
    TestFalse(TEXT("Turnover retires the unsupported Thunder Mountains key"),
        Mode->ActivePopulationSpatialKeys.Contains(M9Key));
    TestFalse(TEXT("Turnover removes the retired key's actor registry"),
        Mode->ActivePopulationKeyActors.Contains(M9Key));
    TestEqual(TEXT("Retirement destroys the old canonical M9 marker"), CountM9Actors(M9Descriptor), 0);
    TestEqual(TEXT("Retirement destroys the old depleted source actor"),
        CountHarvestNodes(DepletedSourceId), 0);
    TestFalse(TEXT("Retirement clears per-key scheduler state"),
        Mode->ActivePopulationSchedulerKeys.Contains(M9Key)
            || Mode->PopulationKeyNextDueTimes.Contains(M9Key)
            || Mode->PopulationKeyCandidateCursors.Contains(M9Key)
            || Mode->PopulationKeyServicedCheckOrdinals.Contains(M9Key));
    TestTrue(TEXT("The living wildlife reactivation cooldown survives key retirement"),
        Mode->PopulationCreatureReactivationDueTimes.Contains(CooldownSpawnId));
    TestTrue(TEXT("The durable accepted M9 claim remains cached across key retirement"),
        Mode->SessionM9ExplorationClaims.FindRef(PlayerIdentity).Contains(M9ClaimId));

    UKalmalaWorldPopulationSaveGame* ReloadedPopulationSave =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(PopulationSaveBytes));
    if (!TestNotNull(TEXT("Sparse population depletion reloads from memory"), ReloadedPopulationSave)) return false;
    Mode->PopulationSaveGame = ReloadedPopulationSave;
    TestTrue(TEXT("Reloaded sparse save retains the harvested source ID"),
        ReloadedPopulationSave->IsHarvested(DepletedSourceId));

    Player->SetActorLocation(M9Descriptor.Location, false);
    Mode->ActivatePopulationKey(M9Key);
    TestTrue(TEXT("The prior Thunder Mountains key can be reactivated"),
        Mode->ActivePopulationSpatialKeys.Contains(M9Key));
    TestEqual(TEXT("Reactivation creates exactly one canonical M9 actor"), CountM9Actors(M9Descriptor), 1);
    TestEqual(TEXT("Reactivation does not restore the persisted depleted source"),
        CountHarvestNodes(DepletedSourceId), 0);

    UKalmalaPlayerDiscoverySaveGameV2* ReloadedPlayerSave =
        Cast<UKalmalaPlayerDiscoverySaveGameV2>(UGameplayStatics::LoadGameFromMemory(PlayerSaveBytes));
    if (!TestNotNull(TEXT("Accepted M9 owner claim reloads from memory"), ReloadedPlayerSave)) return false;
    TestTrue(TEXT("Reloaded owner save retains the accepted M9 claim"),
        ReloadedPlayerSave->GetM9ClaimIds().Contains(M9ClaimId));
    Mode->PlayerDiscoverySaves.Add(PlayerIdentity, ReloadedPlayerSave);
    Mode->SessionM9ExplorationClaims.Remove(PlayerIdentity);
    const int32 RewardQuantityBeforeReplay = Inventory->GetQuantity(RewardDefinition->RewardItemId);
    TestFalse(TEXT("A reactivated M9 actor cannot repay a previously accepted claim"),
        Mode->ClaimM9ExplorationReward(Player, M9Descriptor));
    TestEqual(TEXT("Rejected replay adds no second M9 reward"),
        Inventory->GetQuantity(RewardDefinition->RewardItemId), RewardQuantityBeforeReplay);
    TestEqual(TEXT("Rejected replay does not write the owner save again"), PlayerSaveWrites, 1);
    return true;
}

#endif
