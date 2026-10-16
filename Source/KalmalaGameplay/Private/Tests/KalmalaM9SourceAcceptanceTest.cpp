#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaForestLayout.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "OnlineSubsystemTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9SourceAcceptanceTest,
    "Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9SourceAcceptanceTest::RunTest(const FString& Parameters)
{
    struct FSourceCase
    {
        FName SourceId;
        FName ItemId;
        FName ToolItemId;
        int32 ToolLevel;
        EKalmalaToolAction Action;
        bool bGeneratedForest = false;
    };

    const FSourceCase Sources[] =
    {
        { TEXT("meadows-birch-trunk"), TEXT("Lightwood"), TEXT("BronzeAxe"), 1, EKalmalaToolAction::Woodcutting },
        { TEXT("elderwood-ironheart-trunk"), TEXT("Densewood"), TEXT("IronAxe"), 2, EKalmalaToolAction::Woodcutting },
        { TEXT("mire-peat-amber-seam"), TEXT("Amber"), TEXT("StonePick"), 1, EKalmalaToolAction::Mining },
        { TEXT("tundra-frost-salt-deposit"), TEXT("FrostSalt"), TEXT("StonePick"), 1, EKalmalaToolAction::Mining },
        { TEXT("meadows-birch-trunk"), TEXT("Lightwood"), TEXT("BronzeAxe"), 1, EKalmalaToolAction::Woodcutting, true },
        { TEXT("elderwood-ironheart-trunk"), TEXT("Densewood"), TEXT("IronAxe"), 2, EKalmalaToolAction::Woodcutting, true },
    };

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("M9 harvest acceptance world created"), World)) return false;
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("M9 harvest fixture creates its authoritative game mode"), World->SetGameMode(FURL())))
    {
        World->DestroyWorld(false);
        return false;
    }
    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();
    const AKalmalaWorldGenerationGameState* WorldState = World->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("M9 fixture has a server world identity"), WorldState))
    {
        World->DestroyWorld(false);
        return false;
    }

    UWorld* OtherWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Foreign-world rejection fixture created"), OtherWorld))
    {
        World->DestroyWorld(false);
        return false;
    }
    AKalmalaCharacter* ForeignPawn = OtherWorld->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Foreign-world pawn spawned"), ForeignPawn))
    {
        OtherWorld->DestroyWorld(false);
        World->DestroyWorld(false);
        return false;
    }

    const auto SetServerToolState = [this](AKalmalaCharacter* Pawn, const FName ToolItemId,
        const int32 Condition, const int32 ToolLevel)
    {
        TArray<FKalmalaToolState>& CarriedTools =
            const_cast<TArray<FKalmalaToolState>&>(Pawn->GetCarriedToolInventory());
        FKalmalaToolState* ToolState = CarriedTools.FindByPredicate([ToolItemId](const FKalmalaToolState& Candidate)
        {
            return Candidate.ItemId == ToolItemId;
        });
        if (ToolState == nullptr)
        {
            if (CarriedTools.Num() >= UKalmalaInventoryComponent::MaxSlots) return false;
            ToolState = &CarriedTools.AddDefaulted_GetRef();
            ToolState->ItemId = ToolItemId;
            ToolState->InstanceGuid = FGuid::NewGuid();
        }
        ToolState->ToolLevel = ToolLevel;
        ToolState->Durability = Condition;
        return true;
    };

    bool bAllFixturesValid = true;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Sources); ++Index)
    {
        const FSourceCase& Source = Sources[Index];
        FVector Location(static_cast<float>(Index) * 2000.0f, 0.0f, 0.0f);
        AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>(Location, FRotator::ZeroRotator);
        AKalmalaHarvestNode* Node = World->SpawnActor<AKalmalaHarvestNode>(Location, FRotator::ZeroRotator);
        if (!Pawn || !Node)
        {
            AddError(FString::Printf(TEXT("M9 source fixture failed to spawn: %s"), *Source.SourceId.ToString()));
            bAllFixturesValid = false;
            continue;
        }

        APlayerState* PlayerState = World->SpawnActor<APlayerState>();
        if (!TestNotNull(TEXT("M9 harvest fixture has an authenticated player state"), PlayerState))
        {
            bAllFixturesValid = false;
            continue;
        }
        const FUniqueNetIdStringRef TestNetId = FUniqueNetIdString::Create(
            FString::Printf(TEXT("m9-source-acceptance-%d"), Index), FName(TEXT("KalmalaM9SourceAcceptanceTest")));
        PlayerState->SetUniqueId(FUniqueNetIdRepl(*TestNetId));
        Pawn->SetPlayerState(PlayerState);

        if (!SetServerToolState(Pawn, Source.ToolItemId,
            FKalmalaToolLifecycleContract::FindDefinition(Source.ToolItemId)->MaxDurability, Source.ToolLevel))
        {
            bAllFixturesValid = false;
            continue;
        }
        TestEqual(TEXT("Fixture carries the source-required server tool tier"),
            Pawn->GetCarriedToolLevel(Source.ToolItemId), Source.ToolLevel);

        FKalmalaWorldPopulationSpawn Spawn;
        Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
        Spawn.SpatialKey = FIntPoint(Index, -2);
        Spawn.SpawnSeed = 918273ull + static_cast<uint64>(Index);
        Spawn.Location = Location;
        Spawn.ContentId = Source.SourceId;
        FVector2D ForestCenter = FVector2D::ZeroVector;
        TArray<FKalmalaForestTreeDescriptor> ForestTrees;
        if (Source.bGeneratedForest)
        {
            bool bInitialized = false;
            const bool bElderwood = Source.SourceId == TEXT("elderwood-ironheart-trunk");
            for (int32 Y = -16; Y <= 16 && !bInitialized; ++Y)
            {
                for (int32 X = -16; X <= 16 && !bInitialized; ++X)
                {
                    const FVector2D Center(X * 10000.0, Y * 10000.0);
                    const auto Candidates = FKalmalaForestLayout::Generate(WorldState->GetWorldGenerationConfig(), Center);
                    if (Candidates.Num() < 2) continue;
                    for (const auto& Tree : Candidates)
                    {
                        if (Tree.bElderwood != bElderwood) continue;
                        bInitialized = Node->InitializeForestServer(WorldState->GetWorldGenerationConfig(), Center, Tree.CandidateIndex);
                        if (bInitialized)
                        {
                            ForestCenter = Center;
                            ForestTrees = Candidates;
                        }
                        break;
                    }
                }
            }
            if (!TestTrue(TEXT("Generated biome candidate initializes for axe/pickup acceptance"), bInitialized))
            {
                Node->Destroy();
                bAllFixturesValid = false;
                continue;
            }
            Location = Node->GetActorLocation();
            Pawn->SetActorLocation(Location);
            TestEqual(TEXT("Generated candidate selects expected biome source"), Node->GetGatheringSourceId(), Source.SourceId);
        }
        else
        {
            Node->InitializeServer(Spawn);
        }
        if (!TestFalse(TEXT("Server fixture node has a stable population identity"), Node->GetPersistentSpawnId().IsEmpty()))
        {
            bAllFixturesValid = false;
            continue;
        }

        const int32 InitialCondition = Pawn->GetToolDurability(Source.ToolItemId);
        const FString SourceLabel = Source.SourceId.ToString();
        const auto AssertRejectedWithoutChanges = [this, Pawn, Node, Source, InitialCondition, SourceLabel]()
        {
            TestFalse(FString::Printf(TEXT("%s rejected request leaves its source available"), *SourceLabel), Node->IsHarvested());
            TestEqual(FString::Printf(TEXT("%s rejected request leaves owner material unchanged"), *SourceLabel),
                Pawn->GetInventoryComponent()->GetQuantity(Source.ItemId), 0);
            TestEqual(FString::Printf(TEXT("%s rejected request leaves owner pack empty"), *SourceLabel),
                Pawn->GetInventoryComponent()->GetStacks().Num(), 0);
            TestEqual(FString::Printf(TEXT("%s rejected request leaves tool condition unchanged"), *SourceLabel),
                Pawn->GetToolDurability(Source.ToolItemId), InitialCondition);
        };

        const uint8 ExpectedAction = static_cast<uint8>(Source.Action);
        const EKalmalaToolAction ForgedAction = Source.Action == EKalmalaToolAction::Woodcutting
            ? EKalmalaToolAction::Mining : EKalmalaToolAction::Woodcutting;
        TestFalse(FString::Printf(TEXT("%s rejects a forged tool ID"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, TEXT("ForgedTool"), ExpectedAction));
        AssertRejectedWithoutChanges();

        TestFalse(FString::Printf(TEXT("%s rejects a mismatched client action"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId,
                static_cast<uint8>(ForgedAction)));
        AssertRejectedWithoutChanges();

        TestFalse(FString::Printf(TEXT("%s rejects an excessive server trace distance"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 300.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        AssertRejectedWithoutChanges();

        Pawn->SetActorLocation(Location + FVector(900.0f, 0.0f, 0.0f));
        TestFalse(FString::Printf(TEXT("%s rejects an out-of-range harvester"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        AssertRejectedWithoutChanges();
        Pawn->SetActorLocation(Location);

        TestFalse(FString::Printf(TEXT("%s rejects a pawn from another world"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(ForeignPawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        AssertRejectedWithoutChanges();

        Pawn->SetRole(ROLE_SimulatedProxy);
        TestFalse(FString::Printf(TEXT("%s rejects non-authoritative client execution"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        Pawn->SetRole(ROLE_Authority);
        AssertRejectedWithoutChanges();

        int32 ExpectedYield = 0;
        TestTrue(FString::Printf(TEXT("%s has a valid bounded server reward"), *SourceLabel),
            FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(
                WorldState->GetWorldGenerationConfig(), Source.SourceId, Node->GetPersistentSpawnId(), ExpectedYield));
        // Observe the real pickup callback without invoking GameMode's disk writer.
        auto* PickupSave = NewObject<UKalmalaWorldPopulationSaveGame>(World);
        PickupSave->InitializeForWorld(WorldState->GetWorldGenerationConfig());
        FString DepletedId;
        FString SparseId;
        int32 SessionCallbacks = 0;
        int32 SparseCallbacks = 0;
        Node->OnM9ResourceDepleted.AddLambda([&](const FString& ResourceId) { DepletedId = ResourceId; ++SessionCallbacks; });
        Node->OnHarvested.AddLambda([&](const FString& ResourceId)
        {
            SparseId = ResourceId;
            ++SparseCallbacks;
            PickupSave->MarkHarvested(ResourceId);
        });

        int32 AcceptedUses = 1;
        if (Node->IsTreeResource())
        {
            TestFalse(TEXT("A standing tree cannot be interaction-harvested"),
                Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
            auto* Inventory = Pawn->GetInventoryComponent();
            Inventory->SynchronizeGridFromServer();
            const int32 Slot = Inventory->GetGridSlots().IndexOfByKey(Source.ToolItemId);
            if (!TestTrue(TEXT("Axe is in the usable hotbar"), Slot >= 0 && Slot < UKalmalaInventoryComponent::Columns))
            {
                Node->OnHarvested.Clear();
                Node->OnM9ResourceDepleted.Clear();
                Node->Destroy();
                bAllFixturesValid = false;
                continue;
            }
            Inventory->ServerUseHotbarSlot(Slot);
            const FKalmalaDamageChannels AxeDamage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(
                Source.ToolItemId, Source.ToolLevel, TEXT("Basic"));
            AcceptedUses = 0;
            while (!Node->IsWoodDrop() && AcceptedUses < 30)
            {
                if (!TestTrue(TEXT("Equipped axe damages the tree or fallen trunk"), Node->ApplyAxeDamageFromServer(Pawn, AxeDamage))) break;
                ++AcceptedUses;
            }
            TestTrue(TEXT("Broken trunk leaves a world wood drop"), Node->IsWoodDrop());
            TestEqual(TEXT("Breaking a trunk does not automatically grant wood"), Inventory->GetQuantity(Source.ItemId), 0);
        }
        TestEqual(TEXT("Damage before pickup emits no sparse depletion"), SparseCallbacks, 0);
        TestEqual(TEXT("Damage before pickup emits no session depletion"), SessionCallbacks, 0);
        TestFalse(TEXT("Damage does not persist depletion before pickup"),
            PickupSave->IsHarvested(Node->GetPersistentSpawnId()));
        if (Source.bGeneratedForest)
        {
            auto* Inventory = Pawn->GetInventoryComponent();
            // Nonstackable filler occupies every pack cell without replacing Main Hand.
            const int32 FillerCount = UKalmalaInventoryComponent::MaxSlots
                - Inventory->CountPackedTools(Pawn->GetCarriedToolInventory());
            if (TestTrue(TEXT("Generated pickup fixture fills every pack cell"),
                FillerCount > 0 && Inventory->TryGrantFromServer(TEXT("BoarHideBoots"), FillerCount)))
            {
                TestEqual(TEXT("Generated pickup rejection starts with forty occupied pack cells"),
                    Inventory->GetStacks().Num() + Inventory->CountPackedTools(Pawn->GetCarriedToolInventory()),
                    UKalmalaInventoryComponent::MaxSlots);
                const auto BeforeGrid = Inventory->GetGridInstanceGuids();
                const FGuid BeforeHand = Inventory->GetMainHandGuid();
                const int32 BeforeCondition = Pawn->GetToolDurability(Source.ToolItemId);
                TestFalse(TEXT("Full pack rejects generated wood pickup"),
                    Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
                TestTrue(TEXT("Rejected pickup retains the uncollected wood drop"), Node->IsWoodDrop());
                TestFalse(TEXT("Rejected pickup does not harvest the generated identity"), Node->IsHarvested());
                TestEqual(TEXT("Rejected pickup grants no wood"), Inventory->GetQuantity(Source.ItemId), 0);
                TestEqual(TEXT("Rejected pickup preserves filler quantity"), Inventory->GetQuantity(TEXT("BoarHideBoots")), FillerCount);
                TestTrue(TEXT("Rejected pickup preserves exact pack identities and cells"),
                    Inventory->GetGridInstanceGuids() == BeforeGrid);
                TestEqual(TEXT("Rejected pickup preserves exact Main Hand"), Inventory->GetMainHandGuid(), BeforeHand);
                TestEqual(TEXT("Rejected pickup spends no additional axe condition"),
                    Pawn->GetToolDurability(Source.ToolItemId), BeforeCondition);
                TestEqual(TEXT("Rejected pickup emits no sparse callback"), SparseCallbacks, 0);
                TestEqual(TEXT("Rejected pickup emits no session callback"), SessionCallbacks, 0);
                TestFalse(TEXT("Rejected pickup never enters the sparse save"),
                    PickupSave->IsHarvested(Node->GetPersistentSpawnId()));
                TestTrue(TEXT("Removing filler permits the same drop pickup to retry"),
                    Inventory->TryConsumeFromServer(TEXT("BoarHideBoots"), FillerCount));
            }
        }
        TestTrue(FString::Printf(TEXT("%s accepts harvest or wood pickup"), *SourceLabel),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        TestFalse(TEXT("Depleted resource cannot be collected twice"),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        TestTrue(FString::Printf(TEXT("%s accepted harvest depletes exactly once"), *SourceLabel), Node->IsHarvested());
        TestEqual(FString::Printf(TEXT("%s grants only its deterministic catalogue reward"), *SourceLabel),
            Pawn->GetInventoryComponent()->GetQuantity(Source.ItemId), ExpectedYield);
        TestEqual(FString::Printf(TEXT("%s accepted use spends one server-owned condition"), *SourceLabel),
            Pawn->GetToolDurability(Source.ToolItemId), InitialCondition - AcceptedUses);
        TestEqual(FString::Printf(TEXT("%s depletion callback carries its stable sparse ID"), *SourceLabel),
            DepletedId, Source.bGeneratedForest ? FString() : FKalmalaM9SourceLootContract::BuildResourceDepletionId(Source.SourceId, Node->GetPersistentSpawnId()));
        TestEqual(TEXT("Only generated forest pickup emits exactly one sparse callback"), SparseCallbacks, Source.bGeneratedForest ? 1 : 0);
        TestEqual(TEXT("Generated pickup bypasses session depletion"), SessionCallbacks, Source.bGeneratedForest ? 0 : 1);
        TestEqual(TEXT("Sparse callback retains exact generated identity"), SparseId,
            Source.bGeneratedForest ? Node->GetPersistentSpawnId() : FString());
        TArray<uint8> PickupBytes;
        TestTrue(TEXT("Pickup sparse ledger serializes in memory"),
            UGameplayStatics::SaveGameToMemory(PickupSave, PickupBytes));
        auto* ReloadedSave = Cast<UKalmalaWorldPopulationSaveGame>(
            UGameplayStatics::LoadGameFromMemory(PickupBytes));
        if (TestNotNull(TEXT("Pickup sparse ledger reloads in memory"), ReloadedSave))
        {
            TestTrue(TEXT("Pickup ledger retains exact world identity"),
                ReloadedSave->MatchesWorld(WorldState->GetWorldGenerationConfig()));
            TestEqual(TEXT("Only generated pickup persists its exact sparse identity"),
                ReloadedSave->IsHarvested(Node->GetPersistentSpawnId()), Source.bGeneratedForest);
            TestFalse(TEXT("Pickup never records the tree as a defeated creature"),
                ReloadedSave->IsDefeated(Node->GetPersistentSpawnId()));
            TestFalse(TEXT("Pickup cannot deplete an unrelated generated identity"),
                ReloadedSave->IsHarvested(Node->GetPersistentSpawnId() + TEXT(":other")));
            if (Source.bGeneratedForest)
            {
                // Exercise the production save reader and materializer in a separate,
                // unstarted world: no session startup or production disk slot writes.
                UWorld* ReloadWorld = UWorld::CreateWorld(EWorldType::Game, false);
                if (TestNotNull(TEXT("Pickup reload patch world created"), ReloadWorld))
                {
                    auto* ReloadMode = ReloadWorld->SpawnActor<AKalmalaGameMode>();
                    if (TestNotNull(TEXT("Pickup reload GameMode created"), ReloadMode))
                    {
                        ReloadMode->WorldGenerationConfig = WorldState->GetWorldGenerationConfig();
                        ReloadMode->TerrainPatchOrigin = ForestCenter;
                        ReloadMode->PopulationSaveGame = ReloadedSave;
                        const FIntPoint Coordinate = FIntPoint::ZeroValue;
                        for (int32 Pass = 0; Pass < 2; ++Pass)
                        {
                            ReloadMode->ActivateTerrainPatch(Coordinate);
                            const auto* Replacements = ReloadMode->ActiveForestNodes.Find(Coordinate);
                            if (!TestNotNull(TEXT("Real pickup save activates its forest patch"), Replacements)) break;
                            TestEqual(TEXT("Pickup reload omits exactly one generated tree"),
                                Replacements->Num(), ForestTrees.Num() - 1);
                            TSet<FString> LiveIds;
                            for (const auto& Replacement : *Replacements)
                            {
                                if (TestTrue(TEXT("Pickup reload replacement is live"), Replacement.IsValid()))
                                    LiveIds.Add(Replacement->GetPersistentSpawnId());
                            }
                            TestFalse(TEXT("Collected forest identity never reconstructs"),
                                LiveIds.Contains(Node->GetPersistentSpawnId()));
                            for (const auto& Tree : ForestTrees)
                            {
                                if (Tree.PersistentSpawnId == Node->GetPersistentSpawnId()) continue;
                                TestTrue(TEXT("Every uncollected neighbor reconstructs after pickup reload"),
                                    LiveIds.Contains(Tree.PersistentSpawnId));
                                TestFalse(TEXT("Patch reconstruction never persists neighbor depletion"),
                                    ReloadedSave->IsHarvested(Tree.PersistentSpawnId));
                            }
                            const auto Evicted = *Replacements;
                            ReloadMode->RefreshTerrainPatchNeighborhoods();
                            TestTrue(TEXT("Pickup reload eviction clears forest tracking"),
                                ReloadMode->ActiveForestNodes.IsEmpty());
                            for (const auto& Replacement : Evicted)
                                TestFalse(TEXT("Pickup reload eviction destroys neighbors"), Replacement.IsValid());
                            TestTrue(TEXT("Pickup depletion survives patch eviction"),
                                ReloadedSave->IsHarvested(Node->GetPersistentSpawnId()));
                        }
                    }
                    ReloadWorld->DestroyWorld(false);
                }
            }
        }
        // Release delegate captures before the next case and world teardown.
        Node->OnHarvested.Clear();
        Node->OnM9ResourceDepleted.Clear();
        Node->Destroy();
    }

    OtherWorld->DestroyWorld(false);
    World->DestroyWorld(false);
    return bAllFixturesValid;
}

#endif
