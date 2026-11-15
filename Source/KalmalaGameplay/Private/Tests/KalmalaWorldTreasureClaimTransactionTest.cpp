#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaGemstoneContract.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

namespace
{
bool AreTreasureTestStacksEqual(const TArray<FKalmalaInventoryStack>& Left,
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldTreasureClaimTransactionTest,
    "Kalmala.Gameplay.WorldPopulation.TreasureClaimTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWorldTreasureClaimTransactionTest::RunTest(const FString& Parameters)
{
    if (!TestNotNull(TEXT("Engine is available for an isolated treasure claim world"), GEngine)) return false;

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 418;
    FKalmalaWorldDiscoveryDescriptor Descriptor;
    bool bFoundDescriptor = false;
    for (int32 Y = -40; Y <= 40 && !bFoundDescriptor; ++Y)
    {
        for (int32 X = -40; X <= 40 && !bFoundDescriptor; ++X)
        {
            for (const FKalmalaWorldDiscoveryDescriptor& Candidate :
                FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(
                    WorldConfig, FIntPoint(X, Y), EKalmalaWorldDiscoveryKind::PointOfInterest))
            {
                const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(WorldConfig, FVector2D(Candidate.Location)));
                const TArray<FName> Items = FKalmalaGemstoneContract::SelectTreasure(
                    Biome, Candidate.DescriptorSeed ^ WorldConfig.WorldSeed);
                if (Items.Num() == 1 && Items[0] == TEXT("Garnet"))
                {
                    Descriptor = Candidate;
                    bFoundDescriptor = true;
                    break;
                }
            }
        }
    }
    if (!TestTrue(TEXT("Fixture finds a canonical one-Garnet point of interest"), bFoundDescriptor)) return false;

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Isolated treasure claim world is created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Isolated world selects the authoritative GameMode"), World->SetGameMode(FURL())))
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

    AKalmalaGameMode* GameMode = World->GetAuthGameMode<AKalmalaGameMode>();
    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("World identity state exists"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("World identity accepts the fixture seed"),
            WorldState->SelectWorldSeedFromServer(WorldConfig.WorldSeed))) return false;
    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Authoritative GameMode exists"), GameMode)
        || !TestNotNull(TEXT("Treasure owner pawn exists"), Player)
        || !TestNotNull(TEXT("Treasure owner player state exists"), PlayerState)) return false;
    Player->SetPlayerState(PlayerState);
    Player->DispatchBeginPlay();
    Player->SetActorLocation(Descriptor.Location);
    GameMode->WorldGenerationConfig = WorldConfig;

    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    UKalmalaCraftingComponent* Crafting = Player->FindComponentByClass<UKalmalaCraftingComponent>();
    if (!TestNotNull(TEXT("Treasure owner inventory exists"), Inventory)
        || !TestNotNull(TEXT("Treasure owner feedback component exists"), Crafting)) return false;

    TArray<FKalmalaToolState>& PackedTools =
        const_cast<TArray<FKalmalaToolState>&>(Player->GetCarriedToolInventory());
    PackedTools.Reset();
    FKalmalaToolState PackedPick;
    PackedPick.ItemId = TEXT("StonePick");
    PackedPick.ToolLevel = 1;
    PackedPick.Durability = 1;
    PackedPick.InstanceGuid = FGuid::NewGuid();
    PackedTools.Add(PackedPick);
    Inventory->SynchronizeGridFromServer();

    TArray<TPair<FName, int32>> PublishedGains;
    Inventory->GainReceiptPublicationForTests = [&PublishedGains](const FName ItemId, const int32 Quantity)
    {
        PublishedGains.Emplace(ItemId, Quantity);
    };

    UKalmalaWorldPopulationSaveGame* InitialSave =
        NewObject<UKalmalaWorldPopulationSaveGame>(GameMode);
    if (!TestNotNull(TEXT("Initial population save exists"), InitialSave)) return false;
    InitialSave->InitializeForWorld(WorldConfig);
    FKalmalaTreasureState Treasure;
    Treasure.Id = TEXT("treasure:") + FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Descriptor);
    Treasure.Items.Add(TEXT("Garnet"));
    if (!TestTrue(TEXT("Initial treasure state is accepted"), InitialSave->SetTreasure(Treasure))) return false;
    GameMode->PopulationSaveGame = InitialSave;

    TArray<uint8> DurablePopulationBytes;
    if (!TestTrue(TEXT("Initial population state serializes"),
            UGameplayStatics::SaveGameToMemory(InitialSave, DurablePopulationBytes))) return false;
    const TArray<uint8> InitialPopulationBytes = DurablePopulationBytes;
    int32 PopulationWriteAttempts = 0;
    bool bFailNextPopulationWrite = false;
    GameMode->PopulationSaveWriteAttemptForTests = [&DurablePopulationBytes, &PopulationWriteAttempts,
        &bFailNextPopulationWrite](UKalmalaWorldPopulationSaveGame* Candidate, const FString&, const int32)
    {
        ++PopulationWriteAttempts;
        if (bFailNextPopulationWrite)
        {
            bFailNextPopulationWrite = false;
            return false;
        }
        return UGameplayStatics::SaveGameToMemory(Candidate, DurablePopulationBytes);
    };

    TArray<FKalmalaInventoryStack> NearlyFullPack;
    const UKalmalaItemCatalogue* ItemCatalogue = UKalmalaItemCatalogue::Get();
    if (!TestNotNull(TEXT("Canonical item catalogue is available"), ItemCatalogue)) return false;
    for (const FKalmalaItemDefinition& Item : ItemCatalogue->Items)
    {
        if (Item.ItemId == TEXT("Garnet")) continue;
        TArray<FKalmalaInventoryStack> Candidate;
        FString Reason;
        if (UKalmalaInventoryComponent::BuildGrant(NearlyFullPack, Item.ItemId, 1, Candidate, Reason)
            && Candidate.Num() > NearlyFullPack.Num())
        {
            NearlyFullPack = MoveTemp(Candidate);
        }
        if (NearlyFullPack.Num() == UKalmalaInventoryComponent::MaxSlots - 1) break;
    }
    if (!TestEqual(TEXT("Catalogue fills every stack cell except the treasure cell"),
            NearlyFullPack.Num(), UKalmalaInventoryComponent::MaxSlots - 1)) return false;

    const TArray<FKalmalaInventoryStack> InitialStacks = Inventory->GetStacks();
    if (!Inventory->TryCommitStacksFromServer(InitialStacks, TArray<FKalmalaInventoryStack>(), false)
        || !Inventory->TryCommitStacksFromServer(TArray<FKalmalaInventoryStack>(), NearlyFullPack, false))
    {
        TestTrue(TEXT("Packed-tool capacity fixture installs its stack rows"), false);
        return false;
    }
    TArray<FKalmalaInventoryStack> FullCandidate;
    FString GrantReason;
    if (!TestTrue(TEXT("Treasure reward candidate is catalogue-valid"),
            UKalmalaInventoryComponent::BuildGrant(NearlyFullPack, TEXT("Garnet"), 1, FullCandidate, GrantReason)))
        return false;
    TestTrue(TEXT("Thirty-nine stacks fit beside one carried pick"),
        Inventory->CanFitContents(NearlyFullPack, Inventory->CountPackedTools(Player->GetCarriedToolInventory())));
    TestFalse(TEXT("The Garnet stack exceeds capacity only when the carried pick is counted"),
        Inventory->CanFitContents(FullCandidate, Inventory->CountPackedTools(Player->GetCarriedToolInventory())));

    const TArray<FKalmalaInventoryStack> PackedCapacitySnapshot = Inventory->GetStacks();
    const uint32 CapacityResultSerial = Crafting->GetResultSerial();
    TestFalse(TEXT("Packed-tool capacity rejects the treasure claim"), GameMode->ClaimWorldTreasure(Player, Descriptor));
    TestTrue(TEXT("Packed-tool capacity rejection publishes the explicit owner result"),
        Crafting->GetLastResult() == TEXT("Inventory is full; treasure remains"));
    TestEqual(TEXT("Packed-tool capacity rejection publishes one result"),
        Crafting->GetResultSerial(), CapacityResultSerial + 1);
    TestTrue(TEXT("Packed-tool capacity rejection preserves inventory"),
        AreTreasureTestStacksEqual(Inventory->GetStacks(), PackedCapacitySnapshot));
    TestEqual(TEXT("Packed-tool capacity rejection does not write population save"), PopulationWriteAttempts, 0);
    TestTrue(TEXT("Packed-tool capacity rejection leaves save bytes unchanged"), DurablePopulationBytes == InitialPopulationBytes);
    const FKalmalaTreasureState* AfterCapacityRejection = GameMode->PopulationSaveGame->FindTreasure(Treasure.Id);
    if (!TestNotNull(TEXT("Packed-tool capacity rejection retains the treasure record"), AfterCapacityRejection)) return false;
    TestFalse(TEXT("Packed-tool capacity rejection leaves the treasure unclaimed"), AfterCapacityRejection->bClaimed);
    TestTrue(TEXT("Packed-tool capacity rejection emits no gain receipt"), PublishedGains.IsEmpty());

    if (!Inventory->TryCommitStacksFromServer(PackedCapacitySnapshot, InitialStacks, false))
    {
        TestTrue(TEXT("Treasure retry fixture restores the original inventory"), false);
        return false;
    }
    const uint32 SaveFailureResultSerial = Crafting->GetResultSerial();
    bFailNextPopulationWrite = true;
    TestFalse(TEXT("Injected population write failure rejects the treasure claim"),
        GameMode->ClaimWorldTreasure(Player, Descriptor));
    TestTrue(TEXT("Save failure tells the owner the treasure remains"),
        Crafting->GetLastResult() == TEXT("Treasure remains; claim could not be saved"));
    TestEqual(TEXT("Save failure publishes one owner result"), Crafting->GetResultSerial(), SaveFailureResultSerial + 1);
    TestTrue(TEXT("Save failure restores the original inventory"),
        AreTreasureTestStacksEqual(Inventory->GetStacks(), InitialStacks));
    TestEqual(TEXT("Save failure attempts exactly one population write"), PopulationWriteAttempts, 1);
    TestTrue(TEXT("Save failure leaves durable bytes unchanged"), DurablePopulationBytes == InitialPopulationBytes);
    const FKalmalaTreasureState* AfterSaveFailure = GameMode->PopulationSaveGame->FindTreasure(Treasure.Id);
    if (!TestNotNull(TEXT("Save failure retains the treasure record"), AfterSaveFailure)) return false;
    TestFalse(TEXT("Save failure leaves the treasure unclaimed"), AfterSaveFailure->bClaimed);
    TestTrue(TEXT("Save failure emits no gain receipt"), PublishedGains.IsEmpty());

    TArray<FKalmalaInventoryStack> ExpectedAfter;
    if (!TestTrue(TEXT("Recovered reward candidate remains valid"),
            UKalmalaInventoryComponent::BuildGrant(InitialStacks, TEXT("Garnet"), 1, ExpectedAfter, GrantReason)))
        return false;
    const uint32 SuccessResultSerial = Crafting->GetResultSerial();
    TestTrue(TEXT("Recovered claim grants the treasure"), GameMode->ClaimWorldTreasure(Player, Descriptor));
    TestEqual(TEXT("Recovered claim publishes one success result"), Crafting->GetResultSerial(), SuccessResultSerial + 1);
    TestEqual(TEXT("Recovered claim tells the owner the treasure was collected"),
        Crafting->GetLastResult(), FString(TEXT("Treasure collected")));
    TestTrue(TEXT("Recovered claim grants the exact stack state"),
        AreTreasureTestStacksEqual(Inventory->GetStacks(), ExpectedAfter));
    TestEqual(TEXT("Recovered claim publishes exactly one gain receipt"), PublishedGains.Num(), 1);
    if (PublishedGains.Num() == 1)
    {
        TestEqual(TEXT("Recovered receipt names the exact treasure item"), PublishedGains[0].Key, FName(TEXT("Garnet")));
        TestEqual(TEXT("Recovered receipt grants one treasure item"), PublishedGains[0].Value, 1);
    }
    TestEqual(TEXT("Recovered claim performs exactly one successful write"), PopulationWriteAttempts, 2);
    const FKalmalaTreasureState* LiveClaimedTreasure = GameMode->PopulationSaveGame->FindTreasure(Treasure.Id);
    if (!TestNotNull(TEXT("Recovered claim retains the live treasure record"), LiveClaimedTreasure)) return false;
    TestTrue(TEXT("Recovered claim marks the live treasure record claimed"), LiveClaimedTreasure->bClaimed);
    UKalmalaWorldPopulationSaveGame* DurableSave = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Recovered claim leaves a readable durable save"), DurableSave)) return false;
    const FKalmalaTreasureState* DurableTreasure = DurableSave->FindTreasure(Treasure.Id);
    if (!TestNotNull(TEXT("Recovered claim persists the treasure record"), DurableTreasure)) return false;
    TestTrue(TEXT("Recovered claim persists one claimed treasure record"), DurableTreasure->bClaimed);

    const int32 WritesBeforeReplay = PopulationWriteAttempts;
    TestFalse(TEXT("A claimed treasure cannot be claimed twice"), GameMode->ClaimWorldTreasure(Player, Descriptor));
    TestEqual(TEXT("Claim replay emits no duplicate gain receipt"), PublishedGains.Num(), 1);
    TestEqual(TEXT("Claim replay performs no additional save write"), PopulationWriteAttempts, WritesBeforeReplay);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
