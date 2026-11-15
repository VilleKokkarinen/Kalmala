#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaGameMode.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaToolLifecycleContract.h"
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
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

namespace
{
bool AreReceiptTestStacksEqual(const TArray<FKalmalaInventoryStack>& Left,
    const TArray<FKalmalaInventoryStack>& Right)
{
    if (Left.Num() != Right.Num()) return false;
    for (int32 Index = 0; Index < Left.Num(); ++Index)
    {
        if (Left[Index].ItemId != Right[Index].ItemId || Left[Index].Quantity != Right[Index].Quantity
            || Left[Index].InstanceGuid != Right[Index].InstanceGuid
            || Left[Index].ItemGuids != Right[Index].ItemGuids) return false;
    }
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaGemstoneClaimReceiptTest,
    "Kalmala.Gameplay.M9.GemstoneClaimGainReceiptRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaGemstoneClaimReceiptTest::RunTest(const FString& Parameters)
{
    if (!TestNotNull(TEXT("Engine is available for the isolated gemstone world"), GEngine)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Gemstone claim world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Gemstone fixture selects the authoritative GameMode"), World->SetGameMode(FURL())))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }
    ON_SCOPE_EXIT
    {
        if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Destroyed);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };

    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    AKalmalaGameMode* GameMode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Gemstone world identity exists"), WorldState)
        || !TestNotNull(TEXT("Authoritative GameMode exists"), GameMode)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("World identity accepts the fixture seed"), WorldState->SelectWorldSeedFromServer(418))) return false;
    GameMode->WorldGenerationConfig = WorldState->GetWorldGenerationConfig();

    UKalmalaWorldPopulationSaveGame* PopulationSave = NewObject<UKalmalaWorldPopulationSaveGame>(GameMode);
    if (!TestNotNull(TEXT("Gemstone population save created"), PopulationSave)) return false;
    PopulationSave->InitializeForWorld(GameMode->WorldGenerationConfig);
    GameMode->PopulationSaveGame = PopulationSave;

    FKalmalaWorldPopulationSpawn Spawn;
    Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
    Spawn.SpatialKey = FIntPoint(0, 0);
    Spawn.SpawnSeed = 418;
    Spawn.Location = FVector::ZeroVector;
    Spawn.ContentId = TEXT("gem-opal-rock");
    AKalmalaHarvestNode* Node = World->SpawnActor<AKalmalaHarvestNode>(
        AKalmalaHarvestNode::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Generated Opal node spawned"), Node)) return false;
    Node->InitializeServer(Spawn);
    const FString SpawnId = Node->GetPersistentSpawnId();
    FKalmalaGemstoneState InitialGem;
    InitialGem.SpawnId = SpawnId;
    InitialGem.SourceId = Spawn.ContentId;
    InitialGem.Hits = 3;
    InitialGem.Outcome = 2;
    if (SpawnId.IsEmpty() || !TestTrue(TEXT("Extracted Opal state seeds the world ledger"),
            PopulationSave->SetGemstone(InitialGem))) return false;
    Node->GemHits = InitialGem.Hits;
    Node->GemOutcome = InitialGem.Outcome;

    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(
        AKalmalaCharacter::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Gemstone owner pawn created"), Player)) return false;
    Player->DispatchBeginPlay();
    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    UKalmalaCraftingComponent* Crafting = Player->FindComponentByClass<UKalmalaCraftingComponent>();
    if (!TestNotNull(TEXT("Gemstone owner inventory exists"), Inventory)
        || !TestNotNull(TEXT("Gemstone owner interaction feedback exists"), Crafting)) return false;

    const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
    const int32 BeforeOpals = Inventory->GetQuantity(TEXT("StarOpal"));
    TArray<TPair<FName, int32>> PublishedOwnerReceipts;
    Inventory->GainReceiptPublicationForTests = [&PublishedOwnerReceipts](const FName ItemId, const int32 Quantity)
    {
        PublishedOwnerReceipts.Emplace(ItemId, Quantity);
    };
    ON_SCOPE_EXIT
    {
        if (IsValid(Inventory)) Inventory->GainReceiptPublicationForTests = {};
    };

    TArray<uint8> DurablePopulationBytes;
    if (!TestTrue(TEXT("Initial gem state serializes to the in-memory slot"),
            UGameplayStatics::SaveGameToMemory(PopulationSave, DurablePopulationBytes))) return false;
    const TArray<uint8> InitialPopulationBytes = DurablePopulationBytes;
    int32 PopulationWriteAttempts = 0;
    bool bFailNextPopulationWrite = true;
    GameMode->PopulationSaveWriteAttemptForTests = [&DurablePopulationBytes, &PopulationWriteAttempts,
        &bFailNextPopulationWrite](UKalmalaWorldPopulationSaveGame* Candidate, const FString&, const int32)
    {
        ++PopulationWriteAttempts;
        if (bFailNextPopulationWrite)
        {
            bFailNextPopulationWrite = false;
            return false;
        }
        return Candidate != nullptr && UGameplayStatics::SaveGameToMemory(Candidate, DurablePopulationBytes);
    };

    const uint32 FailureFeedbackSerial = Crafting->GetResultSerial();
    TestFalse(TEXT("Rejected gemstone-slot write fails the claim"), Node->ClaimGemFromServer(Player));
    TestEqual(TEXT("Gemstone save rejection reports one owner result"),
        Crafting->GetResultSerial(), FailureFeedbackSerial + 1);
    TestFalse(TEXT("Gemstone save rejection is reported as failure"), Crafting->WasLastResultAccepted());
    TestEqual(TEXT("Gemstone save rejection has a clear failure label"),
        Crafting->GetLastResult(), FString(TEXT("Gem claim could not be saved")));
    TestTrue(TEXT("Failed gemstone claim restores every inventory row and identity"),
        AreReceiptTestStacksEqual(Inventory->GetStacks(), Before));
    TestEqual(TEXT("Failed gemstone claim grants no Opal"), Inventory->GetQuantity(TEXT("StarOpal")), BeforeOpals);
    TestEqual(TEXT("Failed gemstone claim retains extracted state"), Node->GetGemOutcome(), uint8(2));
    TestFalse(TEXT("Failed gemstone claim leaves the node available"), Node->IsHarvested());
    TestTrue(TEXT("Failed gemstone claim restores the live population row"),
        PopulationSave->FindGemstone(SpawnId) != nullptr
            && PopulationSave->FindGemstone(SpawnId)->Hits == 3
            && PopulationSave->FindGemstone(SpawnId)->Outcome == 2);
    TestTrue(TEXT("Failed gemstone claim leaves durable save bytes unchanged"),
        DurablePopulationBytes == InitialPopulationBytes);
    TestTrue(TEXT("Failed gemstone claim publishes no owner gain receipt"), PublishedOwnerReceipts.IsEmpty());

    const uint32 RetryFeedbackSerial = Crafting->GetResultSerial();
    TestTrue(TEXT("Gemstone claim succeeds after the save recovers"), Node->ClaimGemFromServer(Player));
    TestEqual(TEXT("Recovered gemstone claim grants exactly one Opal"),
        Inventory->GetQuantity(TEXT("StarOpal")), BeforeOpals + 1);
    TestEqual(TEXT("Recovered gemstone claim publishes one accurate owner receipt"), PublishedOwnerReceipts.Num(), 1);
    TestTrue(TEXT("Recovered receipt contains exactly one Star Opal"), PublishedOwnerReceipts.Num() == 1
        && PublishedOwnerReceipts[0].Key == TEXT("StarOpal") && PublishedOwnerReceipts[0].Value == 1);
    TestEqual(TEXT("Recovered claim writes the claimed state exactly once more"), PopulationWriteAttempts, 2);
    TestEqual(TEXT("Recovered claim keeps its success feedback behavior"), Crafting->GetResultSerial(), RetryFeedbackSerial);
    TestEqual(TEXT("Recovered claim marks the source claimed"), Node->GetGemOutcome(), uint8(3));
    TestTrue(TEXT("Recovered claim persists the claimed gemstone row"),
        PopulationSave->FindGemstone(SpawnId) != nullptr && PopulationSave->FindGemstone(SpawnId)->Outcome == 3);

    TestFalse(TEXT("Claimed gemstone cannot be replayed"), Node->ClaimGemFromServer(Player));
    TestEqual(TEXT("Replay publishes no additional owner receipt"), PublishedOwnerReceipts.Num(), 1);
    TestEqual(TEXT("Replay performs no additional population save write"), PopulationWriteAttempts, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaDurableGainReceiptRollbackTest,
    "Kalmala.Gameplay.Inventory.DurableRefundsDoNotPublishGainReceipts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaDurableGainReceiptRollbackTest::RunTest(const FString& Parameters)
{
    if (!TestNotNull(TEXT("Engine is available for the isolated tool transaction world"), GEngine)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Tool transaction world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Tool transaction fixture selects the authoritative GameMode"), World->SetGameMode(FURL())))
    {
        GEngine->DestroyWorldContext(World);
        World->DestroyWorld(false);
        return false;
    }
    ON_SCOPE_EXIT
    {
        if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Destroyed);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };

    AKalmalaWorldGenerationGameState* WorldState = World->GetGameState<AKalmalaWorldGenerationGameState>();
    AKalmalaGameMode* GameMode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Tool fixture world state exists"), WorldState)
        || !TestNotNull(TEXT("Tool fixture GameMode exists"), GameMode)) return false;
    if (!TestTrue(TEXT("Tool fixture selects the deterministic world seed"), WorldState->SelectWorldSeedFromServer(418))) return false;
    GameMode->WorldGenerationConfig = WorldState->GetWorldGenerationConfig();

    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Tool transaction owner pawn created"), Player)
        || !TestNotNull(TEXT("Tool transaction owner state created"), PlayerState)
        || !TestNotNull(TEXT("Tool transaction owner controller created"), Controller)) return false;
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("rf43-durable-tool-owner"), FName(TEXT("KalmalaRF43Test")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    PlayerState->SetPlayerId(1);
    Player->SetPlayerState(PlayerState);
    Controller->Possess(Player);
    Player->DispatchBeginPlay();

    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    UKalmalaCraftingComponent* Crafting = Player->FindComponentByClass<UKalmalaCraftingComponent>();
    if (!TestNotNull(TEXT("Tool transaction inventory exists"), Inventory)
        || !TestNotNull(TEXT("Tool transaction crafting component exists"), Crafting)) return false;
    TArray<FKalmalaToolState>& Tools = const_cast<TArray<FKalmalaToolState>&>(Player->GetCarriedToolInventory());
    Tools.Reset();
    Inventory->SynchronizeGridFromServer();
    TestTrue(TEXT("Fixture installs enough ordinary materials"),
        Inventory->TryGrantFromServer(TEXT("Wood"), 20) && Inventory->TryGrantFromServer(TEXT("Stone"), 5));

    TArray<uint8> DurablePlayerBytes;
    GameMode->PlayerDiscoverySaveLoaderForTests = [](const FString&, const int32, USaveGame*& OutSaveGame)
    {
        OutSaveGame = nullptr;
        return false;
    };
    GameMode->PlayerDiscoverySaveWriterForTests = [&DurablePlayerBytes](
        UKalmalaPlayerDiscoverySaveGameV2* Candidate, const FString&, const int32)
    {
        return Candidate != nullptr && UGameplayStatics::SaveGameToMemory(Candidate, DurablePlayerBytes);
    };

    TArray<TPair<FName, int32>> PublishedOwnerReceipts;
    Inventory->GainReceiptPublicationForTests = [&PublishedOwnerReceipts](const FName ItemId, const int32 Quantity)
    {
        PublishedOwnerReceipts.Emplace(ItemId, Quantity);
    };
    ON_SCOPE_EXIT
    {
        if (IsValid(Inventory)) Inventory->GainReceiptPublicationForTests = {};
    };

    const TArray<FKalmalaInventoryStack> BeforeHammerCraft = Inventory->GetStacks();
    FString Reason;
    GameMode->PlayerStateSaveGateForTests = []() { return false; };
    TestFalse(TEXT("Failed carried-hammer save rejects the craft"), Crafting->CraftFromServer(TEXT("BuildingHammer"), 1, Reason));
    TestTrue(TEXT("Failed carried-hammer craft reports its save failure"), Reason.Contains(TEXT("Could not save")));
    TestTrue(TEXT("Failed carried-hammer craft restores every material stack"),
        AreReceiptTestStacksEqual(Inventory->GetStacks(), BeforeHammerCraft));
    TestEqual(TEXT("Failed carried-hammer craft restores the carried-tool list"), Tools.Num(), 0);
    TestTrue(TEXT("Failed carried-hammer craft leaves durable player bytes unchanged"), DurablePlayerBytes.IsEmpty());
    TestTrue(TEXT("Failed carried-hammer craft publishes no refund receipt"), PublishedOwnerReceipts.IsEmpty());
    GameMode->PlayerStateSaveGateForTests = {};
    TestTrue(TEXT("Carried-hammer craft succeeds after save recovery"), Crafting->CraftFromServer(TEXT("BuildingHammer"), 1, Reason));
    TestTrue(TEXT("Recovered craft adds the carried Construction Hammer"),
        Player->GetCarriedToolLevel(TEXT("ConstructionHammer")) == 1);
    TestEqual(TEXT("Recovered carried-hammer craft still publishes no pack receipt"), PublishedOwnerReceipts.Num(), 0);
    const TArray<uint8> PlayerBytesAfterHammerCraft = DurablePlayerBytes;

    FKalmalaToolState Torch;
    Torch.ItemId = TEXT("BasicTorch");
    Torch.ToolLevel = 1;
    Torch.Durability = FKalmalaToolLifecycleContract::GetMaxDurability(Torch.ItemId, Torch.ToolLevel);
    Torch.InstanceGuid = FGuid::NewGuid();
    Torch.RemainingBurnSeconds = 100.0f;
    Tools.Add(Torch);
    const TArray<FKalmalaInventoryStack> BeforeRefuel = Inventory->GetStacks();
    const int32 TorchIndex = Tools.IndexOfByPredicate([&Torch](const FKalmalaToolState& Candidate)
    {
        return Candidate.InstanceGuid == Torch.InstanceGuid;
    });
    if (!TestTrue(TEXT("Refuel fixture found its exact carried torch"), TorchIndex != INDEX_NONE)) return false;
    const int32 WoodBeforeRefuel = Inventory->GetQuantity(TEXT("Wood"));
    GameMode->PlayerStateSaveGateForTests = []() { return false; };
    TestFalse(TEXT("Failed refuel save rejects the payment"), Crafting->RefuelTorchFromServer(Torch.InstanceGuid, Reason));
    TestTrue(TEXT("Failed refuel restores every material row"), AreReceiptTestStacksEqual(Inventory->GetStacks(), BeforeRefuel));
    TestEqual(TEXT("Failed refuel restores the exact torch fuel"), Tools[TorchIndex].RemainingBurnSeconds, 100.0f);
    TestTrue(TEXT("Failed refuel leaves durable player bytes unchanged"), DurablePlayerBytes == PlayerBytesAfterHammerCraft);
    TestTrue(TEXT("Failed refuel publishes no restored-Wood receipt"), PublishedOwnerReceipts.IsEmpty());
    GameMode->PlayerStateSaveGateForTests = {};
    TestTrue(TEXT("Refuel succeeds after save recovery"), Crafting->RefuelTorchFromServer(Torch.InstanceGuid, Reason));
    TestEqual(TEXT("Recovered refuel consumes one Wood"), Inventory->GetQuantity(TEXT("Wood")), WoodBeforeRefuel - 1);
    TestEqual(TEXT("Recovered refuel restores the authored fuel capacity"),
        Tools[TorchIndex].RemainingBurnSeconds, FKalmalaToolLifecycleContract::BasicTorchBurnDurationSeconds);
    TestTrue(TEXT("Recovered refuel publishes no item receipt"), PublishedOwnerReceipts.IsEmpty());

    GameMode->ConstructionSaveGame = NewObject<UKalmalaConstructionSaveGameV2>(GameMode);
    GameMode->ConstructionSaveGame->InitializeForWorld(GameMode->WorldGenerationConfig);
    TArray<uint8> DurableConstructionBytes;
    if (!TestTrue(TEXT("Initial construction state serializes to the in-memory slot"),
            UGameplayStatics::SaveGameToMemory(GameMode->ConstructionSaveGame, DurableConstructionBytes))) return false;
    const TArray<uint8> InitialConstructionBytes = DurableConstructionBytes;
    int32 ConstructionWriteAttempts = 0;
    bool bFailNextConstructionWrite = true;
    GameMode->ConstructionSaveWriteAttemptForTests = [&DurableConstructionBytes, &ConstructionWriteAttempts,
        &bFailNextConstructionWrite](UKalmalaConstructionSaveGameV2* Candidate, const FString&, const int32)
    {
        ++ConstructionWriteAttempts;
        if (bFailNextConstructionWrite)
        {
            bFailNextConstructionWrite = false;
            return false;
        }
        return Candidate != nullptr && UGameplayStatics::SaveGameToMemory(Candidate, DurableConstructionBytes);
    };
    Crafting->ConstructionPlacementForTests = [Player](const FName, FVector& OutLocation, FRotator& OutRotation)
    {
        OutLocation = Player->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f);
        OutRotation = FRotator::ZeroRotator;
        return true;
    };
    const TArray<FKalmalaInventoryStack> BeforeConstruction = Inventory->GetStacks();
    const int32 WoodBeforeConstruction = Inventory->GetQuantity(TEXT("Wood"));
    TestFalse(TEXT("Failed direct-material construction save rejects placement"),
        Crafting->PlaceConstructionFromServer(TEXT("FloorKit"), Reason));
    TestTrue(TEXT("Failed construction reports its save failure"), Reason.Contains(TEXT("Could not save construction")));
    TestTrue(TEXT("Failed construction restores exact material rows"),
        AreReceiptTestStacksEqual(Inventory->GetStacks(), BeforeConstruction));
    TestTrue(TEXT("Failed construction leaves the persistent catalogue empty"),
        GameMode->ConstructionSaveGame->GetRecords().IsEmpty());
    TestTrue(TEXT("Failed construction leaves durable save bytes unchanged"),
        DurableConstructionBytes == InitialConstructionBytes);
    TestTrue(TEXT("Failed construction refund publishes no owner gain receipt"), PublishedOwnerReceipts.IsEmpty());
    TestTrue(TEXT("Direct-material construction retry succeeds after save recovery"),
        Crafting->PlaceConstructionFromServer(TEXT("FloorKit"), Reason));
    TestEqual(TEXT("Recovered construction performs exactly two write attempts"), ConstructionWriteAttempts, 2);
    TestEqual(TEXT("Recovered construction records one placed object"), GameMode->ConstructionSaveGame->GetRecords().Num(), 1);
    TestTrue(TEXT("Recovered construction writes its durable state"), DurableConstructionBytes != InitialConstructionBytes);
    TestEqual(TEXT("Recovered construction spends exactly six Wood"),
        Inventory->GetQuantity(TEXT("Wood")), WoodBeforeConstruction - 6);
    TestTrue(TEXT("Successful placed objects produce no private item receipt"), PublishedOwnerReceipts.IsEmpty());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
