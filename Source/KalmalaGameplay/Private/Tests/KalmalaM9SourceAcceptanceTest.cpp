#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaCombatComponent.h"
#include "Components/SphereComponent.h"
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
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"
#include "TimerManager.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9SourceAcceptanceTest,
    "Kalmala.Gameplay.M9.SecondWaveHarvestAcceptance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9SourceAcceptanceTest::RunTest(const FString& Parameters)
{
    // Synchronous fixture world ticks need distinct timer frames; restore the
    // enclosing editor frame when the isolated worlds have been destroyed.
    TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);
    // Slot I/O is opt-in and must resolve inside a fresh OS-temporary user root.
    FString SlotRoot;
    const bool bRealSlots = FParse::Value(FCommandLine::Get(), TEXT("KalmalaForestSlotRoot="), SlotRoot);
    if (bRealSlots)
    {
        SlotRoot = FPaths::ConvertRelativePathToFull(SlotRoot);
        FPaths::NormalizeDirectoryName(SlotRoot);
        FString Saved = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
        FPaths::NormalizeDirectoryName(Saved);
        const FString ExpectedSaved = SlotRoot / TEXT("Saved");
        const FString Temporary = FPaths::ConvertRelativePathToFull(FPlatformProcess::UserTempDir());
        if (!TestTrue(TEXT("Slot user root is inside OS temporary storage"), FPaths::IsUnderDirectory(SlotRoot, Temporary))
            || !TestTrue(TEXT("Resolved Saved is the explicit isolated root"), FPaths::IsSamePath(Saved, ExpectedSaved))
            || !TestTrue(TEXT("Slot output paths stay below MAX_PATH"), (Saved / TEXT("SaveGames/KalmalaPopulationDeltas_18446744073709551615.sav")).Len() < 260)
            || !TestFalse(TEXT("Isolated root has no existing save directory"), IFileManager::Get().DirectoryExists(*(Saved / TEXT("SaveGames")))))
            return false;
        AddInfo(FString::Printf(TEXT("Real forest population slots: Saved=%s"), *Saved));
    }
    struct FSourceCase
    {
        FName SourceId;
        FName ItemId;
        FName ToolItemId;
        int32 ToolLevel;
        EKalmalaToolAction Action;
        bool bGeneratedForest = false;
        int32 FixedRewardQuantity = 0;
    };

    const FSourceCase Sources[] =
    {
        { TEXT("meadows-birch-trunk"), TEXT("Wood"), TEXT("StoneAxe"), 1, EKalmalaToolAction::Woodcutting },
        { TEXT("elderwood-ironheart-trunk"), TEXT("Densewood"), TEXT("IronAxe"), 2, EKalmalaToolAction::Woodcutting },
        { TEXT("mire-peat-amber-seam"), TEXT("Amber"), TEXT("StonePick"), 1, EKalmalaToolAction::Mining },
        { TEXT("tundra-frost-salt-deposit"), TEXT("FrostSalt"), TEXT("StonePick"), 1, EKalmalaToolAction::Mining },
        { TEXT("meadows-birch-trunk"), TEXT("Wood"), TEXT("StoneAxe"), 1, EKalmalaToolAction::Woodcutting, true },
        { TEXT("elderwood-ironheart-trunk"), TEXT("Densewood"), TEXT("IronAxe"), 2, EKalmalaToolAction::Woodcutting, true },
        { TEXT("meadows-aspen-trunk"), TEXT("Lightwood"), TEXT("StoneAxe"), 1, EKalmalaToolAction::Woodcutting, true, 4 },
    };

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("M9 harvest acceptance world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); };
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
    const auto AdvanceFixtureTime = [World](const int32 Steps)
    {
        // TimeOnly advances the world clock but deliberately omits timers.
        // Small steps also stay below the world's maximum frame-time clamp.
        for (int32 Step = 0; Step < Steps; ++Step)
        {
            ++GFrameCounter;
            World->Tick(LEVELTICK_TimeOnly, 0.05f);
            World->GetTimerManager().Tick(0.05f);
        }
    };
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

        const bool bAutomaticPickupCase = Source.bGeneratedForest && Source.ItemId == TEXT("Wood");
        APlayerController* PickupController = bAutomaticPickupCase ? World->SpawnActor<APlayerController>() : nullptr;
        ON_SCOPE_EXIT { if (PickupController) PickupController->Destroy(); };
        if (PickupController) PickupController->Possess(Pawn);
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
            const bool bAspen = Source.SourceId == TEXT("meadows-aspen-trunk");
            for (int32 Y = -16; Y <= 16 && !bInitialized; ++Y)
            {
                for (int32 X = -16; X <= 16 && !bInitialized; ++X)
                {
                    const FVector2D Center(X * 10000.0, Y * 10000.0);
                    const auto Candidates = FKalmalaForestLayout::Generate(WorldState->GetWorldGenerationConfig(), Center);
                    if (Candidates.Num() < 2) continue;
                    for (const auto& Tree : Candidates)
                    {
                        if (Tree.bElderwood != bElderwood || Tree.bAspen != bAspen) continue;
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

        int32 ExpectedYield = Source.FixedRewardQuantity;
        if (ExpectedYield == 0)
        {
            TestTrue(FString::Printf(TEXT("%s has a valid bounded server reward"), *SourceLabel),
                FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(
                    WorldState->GetWorldGenerationConfig(), Source.SourceId, Node->GetPersistentSpawnId(), ExpectedYield));
        }
        else
        {
            FKalmalaToolServerSelection Selection;
            TestTrue(FString::Printf(TEXT("%s has a fixed server bundle reward"), *SourceLabel),
                FKalmalaToolLifecycleContract::BuildServerSelection(Source.SourceId, Selection));
            TestEqual(FString::Printf(TEXT("%s selection uses its fixed yield"), *SourceLabel),
                Selection.RewardQuantity, ExpectedYield);
        }
        // Default coverage snapshots the final boundary; isolated slot mode leaves it disabled.
        auto* PickupSave = NewObject<UKalmalaWorldPopulationSaveGame>(World);
        PickupSave->InitializeForWorld(WorldState->GetWorldGenerationConfig());
        FString DepletedId;
        FString SparseId;
        int32 SessionCallbacks = 0;
        int32 SparseCallbacks = 0;
        Node->OnM9ResourceDepleted.AddLambda([&](const FString& ResourceId) { DepletedId = ResourceId; ++SessionCallbacks; });
        auto* PickupMode = World->GetAuthGameMode<AKalmalaGameMode>();
        TArray<uint8> ProductionPickupBytes;
        int32 ProductionWrites = 0;
        const FString PopulationSlot = FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"),
            WorldState->GetWorldGenerationConfig().WorldSeed);
        const bool bCaseUsesSlot = bRealSlots && Source.bGeneratedForest;
        ON_SCOPE_EXIT
        {
            if (bCaseUsesSlot && UGameplayStatics::DoesSaveGameExist(PopulationSlot, 0))
                TestTrue(TEXT("Run-owned population slot removed"), UGameplayStatics::DeleteGameInSlot(PopulationSlot, 0));
        };
        if (Source.bGeneratedForest)
        {
            PickupMode->WorldGenerationConfig = WorldState->GetWorldGenerationConfig();
            PickupMode->PopulationSaveGame = PickupSave;
            if (bCaseUsesSlot)
            {
                TestFalse(TEXT("Generated biome starts without an existing slot"), UGameplayStatics::DoesSaveGameExist(PopulationSlot, 0));
                PickupMode->LoadPopulationSave();
                PickupSave = PickupMode->PopulationSaveGame;
                TestFalse(TEXT("Real slot mode disables writer override"), static_cast<bool>(PickupMode->PopulationSaveWriterForTests));
            }
            else
            {
                PickupMode->PopulationSaveWriterForTests = [&](UKalmalaWorldPopulationSaveGame* WrittenSave,
                    const FString& Slot, const int32 UserIndex)
                {
                    ++ProductionWrites;
                    TestTrue(TEXT("Real pickup writes the production population save"), WrittenSave == PickupSave);
                    TestTrue(TEXT("Real pickup marks exact depletion before production write"),
                        WrittenSave->IsHarvested(Node->GetPersistentSpawnId()));
                    TestEqual(TEXT("Real pickup uses seed-scoped population slot"), Slot,
                        FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"), WorldState->GetWorldGenerationConfig().WorldSeed));
                    TestEqual(TEXT("Real pickup uses population user zero"), UserIndex, 0);
                    TestTrue(TEXT("Real pickup production snapshot serializes"),
                        UGameplayStatics::SaveGameToMemory(WrittenSave, ProductionPickupBytes));
                };
            }
            Node->OnHarvested.AddUObject(PickupMode, &AKalmalaGameMode::RecordHarvestedSpawn);
        }
        Node->OnHarvested.AddLambda([&](const FString& ResourceId)
        {
            SparseId = ResourceId;
            ++SparseCallbacks;
            // Observer only: production RecordHarvestedSpawn owns sparse mutation.
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
                PickupMode->PopulationSaveWriterForTests = {};
                PickupMode->PopulationSaveGame = nullptr;
                Node->OnHarvested.Clear();
                Node->OnM9ResourceDepleted.Clear();
                Node->Destroy();
                bAllFixturesValid = false;
                continue;
            }
            // BeginPlay selected the starter hammer at this world's current time.
            // Let the ordinary hotbar request cadence expire before selecting the axe.
            AdvanceFixtureTime(5);
            Inventory->ServerUseHotbarSlot(Slot);
            TestEqual(TEXT("Hotbar selects the fixture axe"), Inventory->GetActiveItem(), Source.ToolItemId);
            const FKalmalaDamageChannels AxeDamage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(
                Source.ToolItemId, Source.ToolLevel, TEXT("Basic"));
            TestTrue(TEXT("Fixture axe supplies positive attack damage"), AxeDamage.Total() > 0.0f);
            AcceptedUses = 0;
            while (!Node->IsWoodDrop() && AcceptedUses < 30)
            {
                if (!TestTrue(TEXT("Equipped axe damages the tree or fallen trunk"), Node->ApplyAxeDamageFromServer(Pawn, AxeDamage))) break;
                ++AcceptedUses;
                if (Node->IsTreeFalling())
                {
                    const FString FallingId = Node->GetPersistentSpawnId();
                    const FTransform TreeRoot = Node->GetActorTransform();
                    auto* Timber = Node->FindComponentByClass<UProceduralMeshComponent>();
                    if (!TestNotNull(TEXT("Tree has its original procedural timber"), Timber)) break;
                    const auto* FallingSection = Timber->GetProcMeshSection(0);
                    if (!TestNotNull(TEXT("Falling tree retains its mesh section"), FallingSection)) break;
                    const FVector TrunkTop = FallingSection->ProcVertexBuffer[4].Position;
                    TestTrue(TEXT("Only falling trees enable presentation tick"), Node->IsActorTickEnabled());
                    const int32 FallingCondition = Pawn->GetToolDurability(Source.ToolItemId);
                    TestFalse(TEXT("Falling tree is neither targetable trunk nor pickup"),
                        Node->CanReceiveAxeAttack() || Node->IsFelledTrunk() || Node->CanInteract_Implementation(Pawn));
                    TestFalse(TEXT("Overlapping accepted-intent hit cannot damage a falling tree"),
                        Node->ApplyAxeDamageFromServer(Pawn, AxeDamage));
                    TestEqual(TEXT("Rejected overlap spends no condition"),
                        Pawn->GetToolDurability(Source.ToolItemId), FallingCondition);
                    AdvanceFixtureTime(12);
                    TestTrue(TEXT("Tree remains in transition before the settle deadline"), Node->IsTreeFalling());
                    TestTrue(TEXT("Replicated start time reconstructs the midpoint progress"),
                        FMath::IsNearlyEqual(Node->GetTreeFallProgress(), 0.5f, 0.01f));
                    Node->Tick(0.0f); // Fixture clock steps deliberately do not tick actors.
                    const FVector MidAxis = Timber->GetRelativeTransform().TransformVector(FVector::UpVector);
                    TestTrue(TEXT("Midpoint actually rotates the presentation"), MidAxis.X > 0 && MidAxis.Z > 0);
                    AdvanceFixtureTime(24);
                    TestTrue(TEXT("Server timer makes the trunk targetable"), Node->IsFelledTrunk() && Node->CanReceiveAxeAttack());
                    TestEqual(TEXT("Settling retains exact persistent identity"), Node->GetPersistentSpawnId(), FallingId);
                    TestTrue(TEXT("Falling never relocates the authoritative tree root"), Node->GetActorTransform().Equals(TreeRoot));
                    TestTrue(TEXT("Settled timber stops presentation tick"), !Node->IsActorTickEnabled());
                    TestTrue(TEXT("Settled timber is horizontal"), Timber->GetRelativeTransform().TransformVector(
                        FVector::UpVector).Equals(FVector::ForwardVector, 0.0001));
                    const auto* SettledSection = Timber->GetProcMeshSection(0);
                    if (!TestNotNull(TEXT("Settled log retains its mesh section"), SettledSection)) break;
                    TestTrue(TEXT("Settled log preserves the falling trunk's full dimensions"),
                        SettledSection->ProcVertexBuffer[4].Position.Equals(TrunkTop));
                    TestEqual(TEXT("Settled mesh is timber and two cut faces without canopy"),
                        SettledSection->ProcVertexBuffer.Num(), 24);
                    TestEqual(TEXT("Settling spends no additional condition"), Pawn->GetToolDurability(Source.ToolItemId), FallingCondition);
                    TestEqual(TEXT("Every accepted hit spent exactly one condition"), FallingCondition, InitialCondition - AcceptedUses);
                    TestEqual(TEXT("Settling grants no material"), Inventory->GetQuantity(Source.ItemId), 0);

                    TestEqual(TEXT("Settled log removes the invisible root sphere target"),
                        Node->FindComponentByClass<USphereComponent>()->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
                    FCollisionQueryParams LogQuery(SCENE_QUERY_STAT(KalmalaSettledLogTest), false, Pawn);
                    FHitResult LogHit;
                    const FTransform Root = Node->GetActorTransform();
                    TestTrue(TEXT("Visibility hits the presented timber away from the identity root"),
                        World->LineTraceSingleByChannel(LogHit,
                            Root.TransformPosition(FVector(TrunkTop.Z * 0.75f, 0, 150)),
                            Root.TransformPosition(FVector(TrunkTop.Z * 0.75f, 0, -50)), ECC_Visibility, LogQuery)
                        && LogHit.GetActor() == Node && LogHit.GetComponent() == Timber);
                    TestFalse(TEXT("Visibility no longer hits the empty space behind the root"),
                        World->LineTraceSingleByChannel(LogHit,
                            Root.TransformPosition(FVector(-50, 0, 100)),
                            Root.TransformPosition(FVector(-50, 0, -100)), ECC_Visibility, LogQuery));

                    Pawn->SetActorLocation(Root.TransformPosition(FVector(TrunkTop.Z - 15, 400, 88)));
                    TestFalse(TEXT("Distant timber hit is rejected despite a valid settled phase"),
                        Node->ApplyAxeDamageFromServer(Pawn, AxeDamage));
                    TestEqual(TEXT("Rejected distant hit spends no condition"), Pawn->GetToolDurability(Source.ToolItemId), FallingCondition);
                    Pawn->SetActorLocation(Root.TransformPosition(FVector(TrunkTop.Z - 15, 130, 88)));
                    Pawn->SetActorRotation(Root.TransformRotation(FRotator(0, -90, 0).Quaternion()));
                    TestTrue(TEXT("Timber-tip fixture is outside old identity-root reach"),
                        FVector::Dist(Pawn->GetActorLocation(), Node->GetActorLocation()) > 220);
                    TestTrue(TEXT("Timber-tip fixture is within surface reach"), Node->IsWithinAxeRange(Pawn->GetActorLocation()));
                    auto* Combat = Pawn->GetCombatComponent();
                    Combat->ServerRequestAttack(1);
                    TestEqual(TEXT("Ordinary server axe targeting selects the settled log tip"),
                        Combat->GetActionPhase(), EKalmalaCombatActionPhase::Windup);
                    AdvanceFixtureTime(5);
                    Combat->TickComponent(0.0f, LEVELTICK_All, nullptr);
                    TestEqual(TEXT("Server revalidates and hits timber outside root reach"), Combat->GetFeedback(), EKalmalaCombatFeedback::Hit);
                    ++AcceptedUses;
                    TestEqual(TEXT("Ordinary timber hit spends one condition"),
                        Pawn->GetToolDurability(Source.ToolItemId), FallingCondition - 1);
                }
            }
            Pawn->SetActorLocation(Location);
            TestTrue(TEXT("Broken trunk leaves a world wood drop"), Node->IsWoodDrop());
            TestEqual(TEXT("Breaking a trunk does not automatically grant wood"), Inventory->GetQuantity(Source.ItemId), 0);
            auto* Bundle = Node->FindComponentByClass<UProceduralMeshComponent>();
            const float Height = Source.bGeneratedForest ? Node->GetForestDimensionsForVerification().Y : 280.0f;
            const FVector Anchor = Node->GetActorTransform().TransformPosition(FVector(Height * 0.5f, 0, 0));
            TestTrue(TEXT("Bundle rests at the settled timber midpoint"),
                Bundle && Bundle->GetComponentLocation().Equals(Anchor, 0.01f));
            TestTrue(TEXT("Bundle pickup anchor matches presentation"), Node->GetHarvestInteractionLocation().Equals(Anchor, 0.01f));
            TestTrue(TEXT("Bundle preserves exact tree root"), Node->GetActorLocation().Equals(Location));
            TestTrue(TEXT("Bundle removes the old identity-root sphere target"),
                Node->FindComponentByClass<USphereComponent>()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
            Pawn->SetActorLocation(Anchor + Node->GetActorTransform().TransformVector(FVector(180, 0, 88)));
            TestTrue(TEXT("Bundle pickup works beyond identity-root reach"),
                FVector::Dist(Pawn->GetActorLocation(), Location) > 250.0f);
            FCollisionQueryParams BundleQuery(SCENE_QUERY_STAT(KalmalaBundleTarget), true);
            BundleQuery.AddIgnoredActor(Pawn);
            FHitResult BundleHit;
            TestTrue(TEXT("Visibility trace selects the presented collectible bundle"),
                World->LineTraceSingleByChannel(BundleHit, Anchor + FVector(0, 0, 150),
                    Anchor - FVector(0, 0, 50), ECC_Visibility, BundleQuery) && BundleHit.GetActor() == Node);
            TestFalse(TEXT("Visibility trace misses the now-empty tree root"),
                World->LineTraceSingleByChannel(BundleHit, Location + FVector(0, 0, 150),
                    Location - FVector(0, 0, 50), ECC_Visibility, BundleQuery) && BundleHit.GetActor() == Node);
            TestTrue(TEXT("Pawn beside the bundle is physically eligible"), Node->CanInteract_Implementation(Pawn));

        }
        TestEqual(TEXT("Damage before pickup never invokes production writer"), ProductionWrites, 0);
        TestEqual(TEXT("Damage before pickup emits no sparse depletion"), SparseCallbacks, 0);
        TestEqual(TEXT("Damage before pickup emits no session depletion"), SessionCallbacks, 0);
        TestFalse(TEXT("Damage does not persist depletion before pickup"),
            PickupSave->IsHarvested(Node->GetPersistentSpawnId()));
        if (Source.bGeneratedForest)
        {
            auto* Inventory = Pawn->GetInventoryComponent();
            // Standing-tree rejection is not pickup coverage: exercise the drop branch
            // before capacity rejection and before the successful sparse-ledger write.
            const auto PickupGrid = Inventory->GetGridInstanceGuids();
            const FGuid PickupHand = Inventory->GetMainHandGuid();
            const int32 PickupCondition = Pawn->GetToolDurability(Source.ToolItemId);
            const auto AssertInvalidPickup = [&, this](const TCHAR* Label, AKalmalaCharacter* RequestPawn,
                const float TraceDistance, const float MaximumRange)
            {
                TestFalse(Label, Node->InteractWithToolIntentFromServer(
                    RequestPawn, TraceDistance, MaximumRange, Source.ToolItemId, ExpectedAction));
                TestTrue(TEXT("Invalid pickup retains generated wood drop"), Node->IsWoodDrop());
                TestFalse(TEXT("Invalid pickup leaves generated identity unharvested"), Node->IsHarvested());
                TestEqual(TEXT("Invalid pickup grants no owner wood"), Inventory->GetQuantity(Source.ItemId), 0);
                TestEqual(TEXT("Invalid pickup grants no foreign-world wood"),
                    ForeignPawn->GetInventoryComponent()->GetQuantity(Source.ItemId), 0);
                TestTrue(TEXT("Invalid pickup preserves exact pack cells"), Inventory->GetGridInstanceGuids() == PickupGrid);
                TestEqual(TEXT("Invalid pickup preserves Main Hand"), Inventory->GetMainHandGuid(), PickupHand);
                TestEqual(TEXT("Invalid pickup preserves axe condition"), Pawn->GetToolDurability(Source.ToolItemId), PickupCondition);
                TestEqual(TEXT("Invalid pickup emits no sparse callback"), SparseCallbacks, 0);
                TestEqual(TEXT("Invalid pickup emits no session callback"), SessionCallbacks, 0);
                TestFalse(TEXT("Invalid pickup leaves sparse ledger untouched"), PickupSave->IsHarvested(Node->GetPersistentSpawnId()));
            };
            AssertInvalidPickup(TEXT("Generated drop rejects null interactor"), nullptr, 100.0f, 250.0f);
            AssertInvalidPickup(TEXT("Generated drop rejects foreign-world interactor"), ForeignPawn, 100.0f, 250.0f);
            AssertInvalidPickup(TEXT("Generated drop rejects negative trace distance"), Pawn, -1.0f, 250.0f);
            AssertInvalidPickup(TEXT("Generated drop rejects trace beyond range"), Pawn, 251.0f, 250.0f);
            AssertInvalidPickup(TEXT("Generated drop rejects zero maximum range"), Pawn, 0.0f, 0.0f);
            AssertInvalidPickup(TEXT("Generated drop rejects negative maximum range"), Pawn, 0.0f, -1.0f);
            Pawn->SetActorLocation(Node->GetHarvestInteractionLocation() + FVector(900.0f, 0.0f, 0.0f));
            AssertInvalidPickup(TEXT("Generated drop rejects distant pawn despite short trace"), Pawn, 100.0f, 250.0f);
            Pawn->SetActorLocation(Node->GetHarvestInteractionLocation() + FVector(0, 130, 88));
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
        AKalmalaCharacter* Rival = nullptr;
        APlayerState* RivalState = nullptr;
        TArray<FGuid> RivalGrid;
        FGuid RivalHand;
        if (Source.bGeneratedForest)
        {
            FActorSpawnParameters RivalParameters;
            RivalParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Rival = World->SpawnActor<AKalmalaCharacter>(Location, FRotator::ZeroRotator, RivalParameters);
            RivalState = World->SpawnActor<APlayerState>();
            if (!TestNotNull(TEXT("Competing pickup pawn spawned"), Rival)
                || !TestNotNull(TEXT("Competing pickup player state spawned"), RivalState))
            {
                if (Rival) Rival->Destroy();
                if (RivalState) RivalState->Destroy();
                PickupMode->PopulationSaveWriterForTests = {};
                PickupMode->PopulationSaveGame = nullptr;
                Node->OnHarvested.Clear();
                Node->OnM9ResourceDepleted.Clear();
                Node->Destroy();
                bAllFixturesValid = false;
                continue;
            }
            const FUniqueNetIdStringRef RivalId = FUniqueNetIdString::Create(
                FString::Printf(TEXT("m9-forest-rival-%d"), Index), FName(TEXT("KalmalaM9SourceAcceptanceTest")));
            RivalState->SetUniqueId(FUniqueNetIdRepl(*RivalId));
            Rival->SetPlayerState(RivalState);
            Rival->SetActorLocation(Node->GetHarvestInteractionLocation() + FVector(0, -130, 88));
            Rival->GetInventoryComponent()->SynchronizeGridFromServer();
            RivalGrid = Rival->GetInventoryComponent()->GetGridInstanceGuids();
            RivalHand = Rival->GetInventoryComponent()->GetMainHandGuid();
            TestTrue(TEXT("Nearby rival is eligible before the first pickup"), Node->CanInteract_Implementation(Rival));
            TestEqual(TEXT("Rival starts without the generated wood reward"),
                Rival->GetInventoryComponent()->GetQuantity(Source.ItemId), 0);
        }
        TestEqual(TEXT("Rejected pickups never invoke production writer"), ProductionWrites, 0);
        if (bCaseUsesSlot)
            TestFalse(TEXT("Damage, invalid requests and full pack create no population slot"),
                UGameplayStatics::DoesSaveGameExist(PopulationSlot, 0));
        FName PickupItem;
        int32 PickupQuantity = 0;
        if (Node->IsWoodDrop())
        {
            TestTrue(TEXT("Drop advertises exact pickup metadata"), Node->GetPickupInfo(PickupItem, PickupQuantity));
            TestEqual(TEXT("Crosshair item matches the server reward"), PickupItem, Source.ItemId);
            TestEqual(TEXT("Crosshair quantity matches the deterministic server grant"), PickupQuantity, ExpectedYield);
        }
        if (bAutomaticPickupCase)
        {
            Pawn->SetActorLocation(Node->GetHarvestInteractionLocation() + FVector(0, 80, 88));
            Pawn->ServerSetAutomaticPickupEnabled(false);
            Pawn->UpdateAutomaticPickupFromServer(0.25f);
            TestFalse(TEXT("Disabled pickup leaves nearby wood intact"), Node->IsHarvested());
            Pawn->ServerSetAutomaticPickupEnabled(true);
            Pawn->SetActorLocation(Node->GetHarvestInteractionLocation() + FVector(0, 200, 88));
            Pawn->UpdateAutomaticPickupFromServer(0.25f);
            TestFalse(TEXT("Server radius rejects farther wood without requiring a view target"), Node->IsHarvested());
            Pawn->SetActorLocation(Node->GetHarvestInteractionLocation() + FVector(0, 80, 88));
            Pawn->UpdateAutomaticPickupFromServer(0.25f);
            TestTrue(TEXT("Walking into radius collects wood through the production server tick"), Node->IsHarvested());
            Pawn->ServerSetAutomaticPickupEnabled(false);
        }
        else
        {
            TestTrue(FString::Printf(TEXT("%s accepts manual harvest or wood pickup with auto disabled"), *SourceLabel),
                Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        }
        if (Rival)
        {
            TestFalse(TEXT("Nearby rival cannot collect the same generated drop"),
                Node->InteractWithToolIntentFromServer(Rival, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
            TestEqual(TEXT("Losing pickup grants no rival wood"), Rival->GetInventoryComponent()->GetQuantity(Source.ItemId), 0);
            TestTrue(TEXT("Losing pickup preserves exact rival pack cells"),
                Rival->GetInventoryComponent()->GetGridInstanceGuids() == RivalGrid);
            TestEqual(TEXT("Losing pickup preserves rival Main Hand"),
                Rival->GetInventoryComponent()->GetMainHandGuid(), RivalHand);
            TestEqual(TEXT("Competing pickup emits only the winner's sparse callback"), SparseCallbacks, 1);
            TestEqual(TEXT("Competing pickup emits no session callback"), SessionCallbacks, 0);
            Rival->Destroy();
            RivalState->Destroy();
        }
        TestFalse(TEXT("Depleted resource cannot be collected twice"),
            Node->InteractWithToolIntentFromServer(Pawn, 100.0f, 250.0f, Source.ToolItemId, ExpectedAction));
        TestTrue(FString::Printf(TEXT("%s accepted harvest depletes exactly once"), *SourceLabel), Node->IsHarvested());
        TestEqual(FString::Printf(TEXT("%s grants only its deterministic catalogue reward"), *SourceLabel),
            Pawn->GetInventoryComponent()->GetQuantity(Source.ItemId), ExpectedYield);
        if (Source.SourceId == TEXT("meadows-birch-trunk"))
        {
            TestEqual(TEXT("Meadow log pickup never grants the retired Lightwood reward"),
                Pawn->GetInventoryComponent()->GetQuantity(TEXT("Lightwood")), 0);
        }
        TestEqual(FString::Printf(TEXT("%s accepted use spends one server-owned condition"), *SourceLabel),
            Pawn->GetToolDurability(Source.ToolItemId), InitialCondition - AcceptedUses);
        TestEqual(FString::Printf(TEXT("%s depletion callback carries its stable sparse ID"), *SourceLabel),
            DepletedId, Source.bGeneratedForest ? FString() : FKalmalaM9SourceLootContract::BuildResourceDepletionId(Source.SourceId, Node->GetPersistentSpawnId()));
        TestEqual(TEXT("Only generated forest pickup emits exactly one sparse callback"), SparseCallbacks, Source.bGeneratedForest ? 1 : 0);
        TestEqual(TEXT("Generated pickup bypasses session depletion"), SessionCallbacks, Source.bGeneratedForest ? 0 : 1);
        TestEqual(TEXT("Sparse callback retains exact generated identity"), SparseId,
            Source.bGeneratedForest ? Node->GetPersistentSpawnId() : FString());
        TestEqual(TEXT("Only successful generated pickup reaches production writer"),
            ProductionWrites, Source.bGeneratedForest && !bCaseUsesSlot ? 1 : 0);
        if (bCaseUsesSlot)
            TestTrue(TEXT("Successful generated pickup creates the real population slot"),
                UGameplayStatics::DoesSaveGameExist(PopulationSlot, 0));
        TArray<uint8> PickupBytes = ProductionPickupBytes;
        if (!Source.bGeneratedForest) TestTrue(TEXT("Pickup sparse ledger serializes in memory"),
            UGameplayStatics::SaveGameToMemory(PickupSave, PickupBytes));
        // Keep fixture I/O outside project Saved and production population slots.
        const FString PickupFile = Source.bGeneratedForest
            ? FPaths::Combine(FPlatformProcess::UserTempDir(),
                TEXT("kf-") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".bin"))
            : FString();
        bool bPickupFileCreated = false;
        ON_SCOPE_EXIT
        {
            if (bPickupFileCreated)
                TestTrue(TEXT("Temporary forest snapshot is removed"),
                    IFileManager::Get().Delete(*PickupFile, false, true));
        };
        if (Source.bGeneratedForest && !bCaseUsesSlot)
        {
            // Never overwrite another file, even in the unlikely event of a GUID collision.
            if (!TestTrue(TEXT("Forest snapshot path stays below MAX_PATH"), PickupFile.Len() < 260)
                || !TestFalse(TEXT("Forest snapshot path is unused"), IFileManager::Get().FileExists(*PickupFile)))
            {
                PickupBytes.Reset();
            }
            else
            {
                // Cleanup also covers a partially written file after a failed write.
                bPickupFileCreated = true;
                TestTrue(TEXT("Production pickup snapshot writes to isolated disk file"),
                    FFileHelper::SaveArrayToFile(PickupBytes, *PickupFile));
                const TArray<uint8> ExpectedBytes = PickupBytes;
                PickupBytes.Reset();
                TestTrue(TEXT("Production pickup snapshot reads back from disk"),
                    FFileHelper::LoadFileToArray(PickupBytes, *PickupFile));
                TestTrue(TEXT("Disk snapshot preserves exact production bytes"), PickupBytes == ExpectedBytes);
            }
        }
        auto* ReloadedSave = Cast<UKalmalaWorldPopulationSaveGame>(bCaseUsesSlot
            ? UGameplayStatics::LoadGameFromSlot(PopulationSlot, 0)
            : UGameplayStatics::LoadGameFromMemory(PickupBytes));
        if (TestNotNull(TEXT("Pickup sparse ledger reloads through selected persistence path"), ReloadedSave))
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
                // unstarted world, using the startup slot loader in real-slot mode.
                UWorld* ReloadWorld = UWorld::CreateWorld(EWorldType::Game, false);
                if (TestNotNull(TEXT("Pickup reload patch world created"), ReloadWorld))
                {
                    auto* ReloadMode = ReloadWorld->SpawnActor<AKalmalaGameMode>();
                    if (TestNotNull(TEXT("Pickup reload GameMode created"), ReloadMode))
                    {
                        ReloadMode->WorldGenerationConfig = WorldState->GetWorldGenerationConfig();
                        ReloadMode->TerrainPatchOrigin = ForestCenter;
                        ReloadMode->PopulationSaveGame = ReloadedSave;
                        if (bCaseUsesSlot)
                        {
                            // Same startup loader as BeginPlay, fresh GameMode and UObject.
                            ReloadMode->PopulationSaveGame = nullptr;
                            ReloadMode->LoadPopulationSave();
                            ReloadedSave = ReloadMode->PopulationSaveGame;
                            TestTrue(TEXT("Startup loader restores collected identity from real slot"),
                                ReloadedSave && ReloadedSave->IsHarvested(Node->GetPersistentSpawnId()));
                            TestTrue(TEXT("Startup loader returns a distinct save object"), ReloadedSave != PickupSave);
                            AddInfo(FString::Printf(TEXT("Real population slot reload: Source=%s Slot=%s ID=%s"),
                                *SourceLabel, *PopulationSlot, *Node->GetPersistentSpawnId()));
                        }
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
        // Release both writer and delegate captures before the next case.
        PickupMode->PopulationSaveWriterForTests = {};
        PickupMode->PopulationSaveGame = nullptr;
        Node->OnHarvested.Clear();
        Node->OnM9ResourceDepleted.Clear();
        Node->Destroy();
    }

    OtherWorld->DestroyWorld(false);
    World->DestroyWorld(false);
    return bAllFixturesValid;
}

#endif
