#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaWildlifeSpawn.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMirelingGemRewardDeliveryTest,
    "Kalmala.Gameplay.Wildlife.MirelingGemRewardDelivery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMirelingGemRewardDeliveryTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Mireling gem delivery fixture world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Mireling gem fixture world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("Mireling gem fixture selects a stable generated-world seed"),
            WorldState->SelectWorldSeedFromServer(19418))) return false;
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Mireling gem fixture installs the authoritative GameMode"), World->SetGameMode(FURL())))
        return false;

    AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Mireling gem fixture GameMode is available"), Mode)) return false;
    Mode->WorldGenerationConfig = WorldState->GetWorldGenerationConfig();
    UKalmalaWorldPopulationSaveGame* InitialSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Mireling gem fixture population save created"), InitialSave)) return false;
    InitialSave->InitializeForWorld(Mode->WorldGenerationConfig);
    Mode->PopulationSaveGame = InitialSave;

    TArray<uint8> DurablePopulationBytes;
    int32 PopulationWriteAttempts = 0;
    Mode->PopulationSaveWriteAttemptForTests = [&DurablePopulationBytes, &PopulationWriteAttempts](
        UKalmalaWorldPopulationSaveGame* Candidate, const FString&, int32)
    {
        ++PopulationWriteAttempts;
        return UGameplayStatics::SaveGameToMemory(Candidate, DurablePopulationBytes);
    };

    FKalmalaWorldPopulationSpawn GemSpawn;
    FString SpawnId;
    bool bFoundDuskDiamondMireling = false;
    for (int32 Y = -64; Y <= 64 && !bFoundDuskDiamondMireling; ++Y)
    {
        for (int32 X = -64; X <= 64 && !bFoundDuskDiamondMireling; ++X)
        {
            for (const FKalmalaWorldPopulationSpawn& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                Mode->WorldGenerationConfig, FIntPoint(X, Y), EKalmalaWorldPopulationKind::Wildlife))
            {
                const FString CandidateId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (Mode->SelectEnemyGemForSpawn(CandidateId, false) != FName(TEXT("DuskDiamond"))) continue;
                GemSpawn = Spawn;
                SpawnId = CandidateId;
                bFoundDuskDiamondMireling = true;
                break;
            }
        }
    }
    if (!TestTrue(TEXT("A generated Mossy Mire Mireling has its canonical Dusk Diamond outcome"),
            bFoundDuskDiamondMireling)) return false;

    APlayerController* Controller = World->SpawnActor<APlayerController>();
    AKalmalaCharacter* Attacker = World->SpawnActor<AKalmalaCharacter>(
        GemSpawn.Location + FVector(100.0, 0.0, 0.0), FRotator::ZeroRotator);
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Mireling gem owner controller created"), Controller)
        || !TestNotNull(TEXT("Mireling gem owner character created"), Attacker)
        || !TestNotNull(TEXT("Mireling gem owner player state created"), PlayerState)) return false;
    World->AddController(Controller);
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("mireling-gem-delivery-owner"), FName(TEXT("KalmalaMirelingGemDeliveryTest")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    Controller->Possess(Attacker);
    Attacker->SetPlayerState(PlayerState);
    const FString OwnerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* Mireling = World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), GemSpawn.Location, FRotator::ZeroRotator, SpawnParameters);
    if (!TestNotNull(TEXT("Generated Mireling actor created"), Mireling)) return false;
    Mireling->InitializeServer(GemSpawn);
    if (!TestEqual(TEXT("The selected generated descriptor initializes as a Mireling"),
            Mireling->GetArchetype(), EKalmalaWildlifeArchetype::Mireling)
        || !TestEqual(TEXT("Generated actor keeps the exact stable descriptor identity"),
            Mireling->GetPersistentSpawnId(), SpawnId)) return false;
    if (UKalmalaResistanceComponent* Resistance = Mireling->FindComponentByClass<UKalmalaResistanceComponent>())
        Resistance->Profile.Armor = 0.0f;
    else
    {
        TestTrue(TEXT("Generated Mireling has its resistance component"), false);
        return false;
    }
    TestTrue(TEXT("Authenticated server damage accepts the generated Mireling defeat"),
        Mireling->ApplyCombatDamageFromServer(100.0f, Attacker));
    if (!TestTrue(TEXT("Generated Mireling is defeated through its production callback"), Mireling->IsDefeated()))
        return false;

    UKalmalaWorldPopulationSaveGame* PersistedSave = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Mireling defeat claim persisted to the isolated save"), PersistedSave)) return false;
    TestTrue(TEXT("Persisted Mireling identity is recorded as defeated"), PersistedSave->IsDefeated(SpawnId));
    if (!TestEqual(TEXT("Persisted defeat creates exactly one reward claim"),
            PersistedSave->GetPendingWildlifeRewards().Num(), 1)) return false;
    const FKalmalaPendingWildlifeReward PersistedClaim = PersistedSave->GetPendingWildlifeRewards()[0];
    const FString ExpectedClaimId = TEXT("mireling-gem|") + SpawnId;
    TestEqual(TEXT("Persisted claim retains the full Mireling gem prefix"), PersistedClaim.PersistentSpawnId,
        ExpectedClaimId);
    TestEqual(TEXT("Persisted claim belongs to the authenticated attacker"), PersistedClaim.PlayerIdentity,
        OwnerIdentity);
    TestEqual(TEXT("Persisted claim keeps the generated Dusk Diamond outcome"), PersistedClaim.GemItemId,
        FName(TEXT("DuskDiamond")));
    Mode->PopulationSaveGame = PersistedSave;

    UKalmalaInventoryComponent* Inventory = Attacker->GetInventoryComponent();
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    if (!TestNotNull(TEXT("Mireling gem owner inventory exists"), Inventory)
        || !TestNotNull(TEXT("Mireling gem fixture item catalogue exists"), Catalogue)) return false;
    const int32 FullPackCapacity = UKalmalaInventoryComponent::MaxSlots
        - Inventory->CountPackedTools(Attacker->GetCarriedToolInventory());
    for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
    {
        if (Inventory->GetStacks().Num() >= FullPackCapacity) break;
        if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("DuskDiamond")
            || Definition.ItemId == TEXT("BoneFragment") || Definition.ItemId == TEXT("BadgerFur")
            || Definition.ItemId == TEXT("BadgerMeat") || Definition.ItemId == TEXT("RatTail")
            || Definition.ItemId == TEXT("ViperFang") || Definition.ItemId == TEXT("GiantScale")
            || Definition.ItemId.ToString().EndsWith(TEXT("Kit"))
            || Catalogue->FindBuildable(Definition.ItemId) != nullptr
            || Inventory->GetQuantity(Definition.ItemId) > 0) continue;
        Inventory->TryGrantFromServer(Definition.ItemId, 1);
    }
    if (!TestEqual(TEXT("Owner inventory is full before delivery"), Inventory->GetStacks().Num(), FullPackCapacity)
        || !TestEqual(TEXT("Dusk Diamond is absent from the full owner inventory"),
            Inventory->GetQuantity(TEXT("DuskDiamond")), 0)) return false;

    const TArray<uint8> DurableBytesBeforeFullRetry = DurablePopulationBytes;
    const int32 WritesBeforeFullRetry = PopulationWriteAttempts;
    Mode->DeliverPendingWildlifeRewards();
    TestEqual(TEXT("Full inventory rejects Dusk Diamond delivery"), Inventory->GetQuantity(TEXT("DuskDiamond")), 0);
    TestTrue(TEXT("Full inventory leaves the persisted reward bytes unchanged"),
        DurablePopulationBytes == DurableBytesBeforeFullRetry);
    TestEqual(TEXT("Full inventory does not write a claim change"), PopulationWriteAttempts, WritesBeforeFullRetry);
    if (!TestEqual(TEXT("Full inventory retains the same pending claim"),
            Mode->PopulationSaveGame->GetPendingWildlifeRewards().Num(), 1)) return false;
    const FKalmalaPendingWildlifeReward& RetainedClaim = Mode->PopulationSaveGame->GetPendingWildlifeRewards()[0];
    TestEqual(TEXT("Retry retains the original stable claim identity"), RetainedClaim.PersistentSpawnId,
        PersistedClaim.PersistentSpawnId);
    TestEqual(TEXT("Retry retains the original authenticated owner"), RetainedClaim.PlayerIdentity,
        PersistedClaim.PlayerIdentity);
    TestEqual(TEXT("Retry does not reroll the persisted gem outcome"), RetainedClaim.GemItemId,
        PersistedClaim.GemItemId);

    const FKalmalaInventoryStack FirstStack = Inventory->GetStacks()[0];
    if (!TestTrue(TEXT("One full inventory cell can be freed"),
            Inventory->TryConsumeFromServer(FirstStack.ItemId, FirstStack.Quantity))) return false;
    TestEqual(TEXT("Freed inventory now has one available cell"), Inventory->GetStacks().Num(), FullPackCapacity - 1);
    Mode->DeliverPendingWildlifeRewards();
    TestEqual(TEXT("Retry grants exactly one Dusk Diamond"), Inventory->GetQuantity(TEXT("DuskDiamond")), 1);
    TestEqual(TEXT("Successful gem delivery fills the available cell"), Inventory->GetStacks().Num(), FullPackCapacity);
    TestEqual(TEXT("Successful delivery saves the claim removal once"),
        PopulationWriteAttempts, WritesBeforeFullRetry + 1);
    TestTrue(TEXT("Successful delivery removes the pending claim"),
        Mode->PopulationSaveGame->GetPendingWildlifeRewards().IsEmpty());

    UKalmalaWorldPopulationSaveGame* DeliveredSave = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Delivered reward save reloads from the isolated persistence seam"), DeliveredSave))
        return false;
    TestTrue(TEXT("Successful delivery preserves the defeated spawn ledger"), DeliveredSave->IsDefeated(SpawnId));
    TestTrue(TEXT("Successful delivery is durably removed from the claim queue"),
        DeliveredSave->GetPendingWildlifeRewards().IsEmpty());
    Mode->DeliverPendingWildlifeRewards();
    TestEqual(TEXT("Repeated delivery does not duplicate the gem"), Inventory->GetQuantity(TEXT("DuskDiamond")), 1);
    return true;
}

#endif
