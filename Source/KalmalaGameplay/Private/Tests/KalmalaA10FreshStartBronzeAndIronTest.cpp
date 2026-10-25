#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationLayout.h"
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
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaA10FreshStartBronzeAndIronTest,
    "Kalmala.Gameplay.Progression.FreshStartBronzeAndIron",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaA10FreshStartBronzeAndIronTest::RunTest(const FString& Parameters)
{
    const FGuid RunGuid = FGuid::NewGuid();
    const FString RunId = RunGuid.ToString(EGuidFormats::Digits);
    const uint64 FixtureSeed = (static_cast<uint64>(RunGuid.A) << 32) | RunGuid.B;
    TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Fresh-start fixture world was created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Fresh-start fixture has an authoritative GameMode"), World->SetGameMode(FURL())))
        return false;

    constexpr float FixtureZ = 500.0f;
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(FVector(0, 0, FixtureZ),
        FRotator::ZeroRotator);
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Fresh-start player controller spawned"), Controller)
        || !TestNotNull(TEXT("Fresh-start character spawned"), Player)
        || !TestNotNull(TEXT("Fresh-start player state spawned"), PlayerState)) return false;

    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        FString::Printf(TEXT("a10-fresh-start-%s"), *RunId), FName(TEXT("KalmalaA10FreshStart")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    Controller->Possess(Player);
    Player->SetPlayerState(PlayerState);

    // Start only after identity and possession are set, so normal BeginPlay
    // initializes this fresh player and its starter kit exactly once.
    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();
    Controller->SetViewTarget(Player);

    AKalmalaGameMode* GameMode = World->GetAuthGameMode<AKalmalaGameMode>();
    const AKalmalaWorldGenerationGameState* WorldState = World->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Fresh-start fixture has its save authority"), GameMode)
        || !TestNotNull(TEXT("Fresh-start fixture has a world identity"), WorldState)
        || !TestTrue(TEXT("Fresh-start fixture uses a valid generated-world identity"),
            WorldState->GetWorldGenerationConfig().IsValid())) return false;

    const auto AdvanceFixtureTime = [World](const int32 Steps)
    {
        for (int32 Step = 0; Step < Steps; ++Step)
        {
            ++GFrameCounter;
            World->Tick(LEVELTICK_TimeOnly, 0.05f);
            World->GetTimerManager().Tick(0.05f);
        }
    };
    const auto WaitForServerAction = [&AdvanceFixtureTime]() { AdvanceFixtureTime(5); };

    const auto SpawnStation = [this, World, GameMode](const FName KitId, const FString& ConstructionId,
        const FVector& Location) -> AKalmalaConstructionActor*
    {
        AKalmalaConstructionActor* Station = World->SpawnActor<AKalmalaConstructionActor>(Location,
            FRotator::ZeroRotator);
        if (!TestNotNull(FString::Printf(TEXT("%s station fixture spawned"), *KitId.ToString()), Station))
            return nullptr;
        Station->InitializeFromServer(KitId, ConstructionId);
        if (!TestTrue(FString::Printf(TEXT("%s fixture is saved through the construction boundary"),
                *KitId.ToString()), GameMode->PersistConstruction(Station))) return nullptr;
        return Station;
    };

    const FVector KilnLocation(40000.0f, 12000.0f, FixtureZ);
    const FVector SmelterLocation(42000.0f, 12000.0f, FixtureZ);
    const FVector ForgeLocation(44000.0f, 12000.0f, FixtureZ);
    AKalmalaConstructionActor* Kiln = SpawnStation(TEXT("CharcoalKilnKit"),
        TEXT("a10-fresh-kiln-") + RunId, KilnLocation);
    AKalmalaConstructionActor* Smelter = SpawnStation(TEXT("SmelterKit"),
        TEXT("a10-fresh-smelter-") + RunId, SmelterLocation);
    AKalmalaConstructionActor* Forge = SpawnStation(TEXT("ForgeKit"),
        TEXT("a10-fresh-forge-") + RunId, ForgeLocation);
    if (!Kiln || !Smelter || !Forge) return false;

    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    UKalmalaCraftingComponent* Crafting = Player->FindComponentByClass<UKalmalaCraftingComponent>();
    if (!TestNotNull(TEXT("Fresh-start inventory component is available"), Inventory)
        || !TestNotNull(TEXT("Fresh-start crafting component is available"), Crafting)) return false;

    const FName StarterMaterials[] = {
        TEXT("Wood"), TEXT("CopperOre"), TEXT("TinOre"), TEXT("IronOre"), TEXT("Coal"),
        TEXT("Copper"), TEXT("Tin"), TEXT("Bronze"), TEXT("Iron")
    };
    for (const FName ItemId : StarterMaterials)
        TestEqual(FString::Printf(TEXT("Fresh start grants no %s"), *ItemId.ToString()),
            Inventory->GetQuantity(ItemId), 0);
    TestEqual(TEXT("Fresh start has no Bronze Pick"), Player->GetCarriedToolLevel(TEXT("BronzePick")), 0);
    TestEqual(TEXT("Fresh start has no Iron Pick"), Player->GetCarriedToolLevel(TEXT("IronPick")), 0);

    const auto FindStarterGuid = [this, Player](const FName ItemId) -> FGuid
    {
        const FKalmalaToolState* Tool = Player->GetCarriedToolInventory().FindByPredicate(
            [ItemId](const FKalmalaToolState& Candidate) { return Candidate.ItemId == ItemId; });
        TestNotNull(FString::Printf(TEXT("Authored starter kit contains %s"), *ItemId.ToString()), Tool);
        return Tool ? Tool->InstanceGuid : FGuid();
    };
    const FGuid StoneKnifeGuid = FindStarterGuid(TEXT("StoneKnife"));
    const FGuid StonePickGuid = FindStarterGuid(TEXT("StonePick"));
    if (!StoneKnifeGuid.IsValid() || !StonePickGuid.IsValid()) return false;

    const auto AimAndMoveTo = [Player, Controller](const FVector& TargetLocation)
    {
        const FVector PlayerLocation = TargetLocation - FVector(170.0f, 0.0f, 0.0f);
        Player->SetActorLocation(PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Controller->SetActorLocation(PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
        const FRotator Aim = (TargetLocation - Player->GetPawnViewLocation()).Rotation();
        Controller->SetControlRotation(Aim);
        Player->SetActorRotation(Aim);
    };
    const auto InteractWithStation = [this, Player, Crafting, &AimAndMoveTo](
        AKalmalaConstructionActor* Station, const FName KitId, const FString& ConstructionId)
    {
        if (!Station) return false;
        AimAndMoveTo(Station->GetActorLocation());
        Player->ServerRequestInteract(NAME_None, 0);
        return TestTrue(FString::Printf(TEXT("Server interaction selects %s station context"), *KitId.ToString()),
            Crafting->IsStationContextUsable(Station, KitId, ConstructionId));
    };

    const auto FindFixtureLocation = [this, WorldState](const EKalmalaBiome ExpectedBiome,
        const double MinimumRadius, const double MaximumRadius, const bool bRequireSafeTerrain,
        const TCHAR* BiomeLabel) -> FVector
    {
        const FKalmalaWorldGenerationConfig& Config = WorldState->GetWorldGenerationConfig();
        for (double Radius = MinimumRadius; Radius <= MaximumRadius; Radius += 5000.0)
        {
            for (int32 AngleDegrees = 0; AngleDegrees < 360; AngleDegrees += 5)
            {
                const double Angle = FMath::DegreesToRadians(static_cast<double>(AngleDegrees));
                const FVector2D Position(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
                const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(Config, Position));
                if (Biome != ExpectedBiome
                    || (bRequireSafeTerrain
                        && !FKalmalaWorldPopulationLayout::IsTerrainSafeDiscoveryLocation(Config, Position)))
                    continue;
                return FVector(Position.X, Position.Y,
                    FKalmalaTerrainHeightSampler::SampleHeight(Config, Position));
            }
        }
        TestFalse(FString::Printf(TEXT("World seed has a deterministic %s source-fixture location"),
            BiomeLabel), true);
        return FVector::ZeroVector;
    };
    const FVector ReedFixtureLocation = FindFixtureLocation(EKalmalaBiome::ShimmeringLakes,
        35000.0, 300000.0, true, TEXT("safe Shimmering Lakes"));
    const FVector CopperFixtureLocation = FindFixtureLocation(EKalmalaBiome::Elderwood,
        75000.0, 400000.0, true, TEXT("Elderwood"));
    const FVector TinFixtureLocation = ReedFixtureLocation;
    const FVector IronFixtureLocation = FindFixtureLocation(EKalmalaBiome::MossyMire,
        300000.0, 1600000.0, false, TEXT("Mossy Mire"));
    if (ReedFixtureLocation.IsZero() || CopperFixtureLocation.IsZero()
        || IronFixtureLocation.IsZero()) return false;

    int32 HarvestSerial = 0;
    const auto SpawnSource = [this, World, WorldState, GameMode, FixtureSeed,
        &HarvestSerial](const FName SourceId,
        const FVector& Location) -> AKalmalaHarvestNode*
    {
        FKalmalaWorldPopulationSpawn Spawn;
        Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
        Spawn.SpatialKey = FIntPoint(2000 + HarvestSerial, 2100);
        Spawn.SpawnSeed = WorldState->GetWorldGenerationConfig().WorldSeed
            ^ FixtureSeed ^ static_cast<uint64>(0xA100 + HarvestSerial);
        Spawn.Location = Location;
        Spawn.ContentId = SourceId;
        AKalmalaHarvestNode* Node = World->SpawnActor<AKalmalaHarvestNode>(Location,
            FRotator::ZeroRotator);
        if (!TestNotNull(FString::Printf(TEXT("%s deterministic source fixture spawned"),
                *SourceId.ToString()), Node)) return nullptr;
        Node->InitializeServer(Spawn);
        EKalmalaBiome ExpectedBiome = EKalmalaBiome::Ocean;
        if (SourceId == TEXT("lakes-reed-cluster") || SourceId == TEXT("lakes-tin-shore-gravel"))
            ExpectedBiome = EKalmalaBiome::ShimmeringLakes;
        else if (SourceId == TEXT("elderwood-copper-vein"))
            ExpectedBiome = EKalmalaBiome::Elderwood;
        else if (SourceId == TEXT("mire-bog-iron"))
            ExpectedBiome = EKalmalaBiome::MossyMire;
        TestTrue(FString::Printf(TEXT("%s fixture position matches its source biome"), *SourceId.ToString()),
            FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(
                WorldState->GetWorldGenerationConfig(), FVector2D(Location))) == ExpectedBiome);
        if (SourceId == TEXT("lakes-tin-shore-gravel"))
        {
            TestTrue(TEXT("Tin fixture uses safe Shimmering Lakes terrain"),
                FKalmalaWorldPopulationLayout::IsTerrainSafeDiscoveryLocation(
                    WorldState->GetWorldGenerationConfig(), FVector2D(Location)));
        }
        if (SourceId == TEXT("elderwood-copper-vein"))
        {
            TestTrue(TEXT("Copper fixture uses generated safe Elderwood terrain"),
                FKalmalaWorldPopulationLayout::IsTerrainSafeDiscoveryLocation(
                    WorldState->GetWorldGenerationConfig(), FVector2D(Location)));
        }
        Node->OnHarvested.AddUObject(GameMode, &AKalmalaGameMode::RecordHarvestedSpawn);
        ++HarvestSerial;
        if (!TestFalse(TEXT("Source fixture receives a stable persistent identity"),
                Node->GetPersistentSpawnId().IsEmpty())) return nullptr;
        return Node;
    };
    TArray<FString> HarvestedFixtureIds;
    const auto HarvestWithTool = [this, Player, Inventory, GameMode, &HarvestedFixtureIds,
        &AimAndMoveTo, &SpawnSource](
        const FName SourceId, const FName ToolId, const FGuid ToolGuid, const FName ExpectedItem,
        const int32 ExpectedQuantity, const EKalmalaToolAction ExpectedAction,
        const FVector& Location) -> bool
    {
        FString Reason;
        if (Inventory->GetActiveItem() != ToolId
            && !TestTrue(FString::Printf(TEXT("Server equips %s for the source"), *ToolId.ToString()),
                Inventory->SetMainHandFromServer(ToolGuid, Reason))) return false;
        AKalmalaHarvestNode* Node = SpawnSource(SourceId, Location);
        if (!Node) return false;
        FKalmalaToolServerSelection Selection;
        if (!TestTrue(FString::Printf(TEXT("%s has an authored source selection"), *SourceId.ToString()),
                FKalmalaToolLifecycleContract::BuildServerSelection(SourceId, Selection))) return false;
        TestEqual(TEXT("Server source selection awards the expected item"), Selection.RewardItemId, ExpectedItem);
        TestTrue(TEXT("Server source selection has the expected action"), Selection.Action == ExpectedAction);
        FName PickupItem;
        int32 PickupQuantity = 0;
        TestTrue(TEXT("Server source exposes its authored reward"), Node->GetPickupInfo(PickupItem, PickupQuantity));
        TestEqual(TEXT("Source reward item is exact"), PickupItem, ExpectedItem);
        TestEqual(TEXT("Source reward quantity is exact"), PickupQuantity, ExpectedQuantity);

        AimAndMoveTo(Node->GetActorLocation());
        const FString PersistentSpawnId = Node->GetPersistentSpawnId();
        const int32 Before = Inventory->GetQuantity(ExpectedItem);
        Player->ServerRequestInteract(ToolId, static_cast<uint8>(ExpectedAction));
        const bool bAccepted = Node->IsHarvested()
            && Inventory->GetQuantity(ExpectedItem) == Before + ExpectedQuantity;
        TestTrue(FString::Printf(TEXT("Normal server interaction harvests %s into the player inventory"),
            *SourceId.ToString()), bAccepted);
        TestTrue(TEXT("Accepted source depletion is recorded by the server population save"),
            GameMode->PopulationSaveGame != nullptr
                && GameMode->PopulationSaveGame->IsHarvested(PersistentSpawnId));
        if (bAccepted) HarvestedFixtureIds.Add(PersistentSpawnId);
        if (Node->IsHarvested()) Node->Destroy();
        return bAccepted;
    };

    const auto RepairAtForgeIfNeeded = [this, Player, Crafting, Inventory, Forge,
        &WaitForServerAction, &InteractWithStation, ForgeId = TEXT("a10-fresh-forge-") + RunId]()
    {
        const FName ActiveItem = Inventory->GetActiveItem();
        if (ActiveItem.IsNone() || Player->GetToolDurability(ActiveItem) > 0) return true;
        if (!InteractWithStation(Forge, TEXT("ForgeKit"), ForgeId)) return false;
        WaitForServerAction();
        Crafting->ServerRepairAllTools();
        return TestTrue(TEXT("Forge repairs exhausted starter-tool condition through the server request"),
            Player->GetToolDurability(ActiveItem) > 0);
    };

    const FName ForgeConstructionId = TEXT("a10-fresh-forge-") + RunId;
    int32 WoodSourceCount = 0;
    while (Inventory->GetQuantity(TEXT("Wood")) < 60 && WoodSourceCount < 80)
    {
        if (!RepairAtForgeIfNeeded()) return false;
        if (!HarvestWithTool(TEXT("lakes-reed-cluster"), TEXT("StoneKnife"), StoneKnifeGuid,
                TEXT("Wood"), 1, EKalmalaToolAction::Gathering, ReedFixtureLocation)) return false;
        ++WoodSourceCount;
    }
    if (!TestEqual(TEXT("Fresh-start wood came only from accepted source transactions"),
            Inventory->GetQuantity(TEXT("Wood")), 60)) return false;

    const auto HarvestOreNodes = [&HarvestWithTool, &RepairAtForgeIfNeeded](
        const FName SourceId, const FName PickId, const FGuid PickGuid,
        const FName ExpectedOre, const int32 NodeCount,
            const FVector& Origin)
    {
        for (int32 Index = 0; Index < NodeCount; ++Index)
        {
            if (!RepairAtForgeIfNeeded()) return false;
            const FVector Location = Origin;
            if (!HarvestWithTool(SourceId, PickId, PickGuid, ExpectedOre, 3,
                    EKalmalaToolAction::Mining, Location)) return false;
        }
        return true;
    };

    if (!TestTrue(TEXT("Stone Pick obtains Copper through seven normal server source interactions"),
            HarvestOreNodes(TEXT("elderwood-copper-vein"), TEXT("StonePick"), StonePickGuid,
                TEXT("CopperOre"), 7,
                CopperFixtureLocation))) return false;
    if (!TestTrue(TEXT("Stone Pick obtains Tin through four normal server source interactions"),
            HarvestOreNodes(TEXT("lakes-tin-shore-gravel"), TEXT("StonePick"), StonePickGuid,
                TEXT("TinOre"), 4,
                TinFixtureLocation))) return false;
    TestEqual(TEXT("Fresh-start Copper Ore is earned from seven three-ore nodes"),
        Inventory->GetQuantity(TEXT("CopperOre")), 21);
    TestEqual(TEXT("Fresh-start Tin Ore is earned from four three-ore nodes"),
        Inventory->GetQuantity(TEXT("TinOre")), 12);

    const FString KilnId = TEXT("a10-fresh-kiln-") + RunId;
    for (int32 Batch = 0; Batch < 4; ++Batch)
    {
        if (!InteractWithStation(Kiln, TEXT("CharcoalKilnKit"), KilnId)) return false;
        for (int32 Unit = 0; Unit < FKalmalaCharcoalKilnState::InputCapacity; ++Unit)
        {
            WaitForServerAction();
            Crafting->ServerLoadCharcoalKilnWood();
            TestTrue(TEXT("Accepted Kiln request consumes one earned Wood"),
                Crafting->WasLastResultAccepted());
        }
        Kiln->Tick(FKalmalaCharcoalKilnState::JobDurationSeconds
            * FKalmalaCharcoalKilnState::InputCapacity);
        TestEqual(TEXT("Production actor Tick completes ten Kiln jobs"),
            Kiln->GetCharcoalKilnState().OutputCoal, FKalmalaCharcoalKilnState::OutputCapacity);
        for (int32 Unit = 0; Unit < FKalmalaCharcoalKilnState::OutputCapacity; ++Unit)
        {
            WaitForServerAction();
            Crafting->ServerCollectCharcoalKilnCoal();
            TestTrue(TEXT("Accepted Kiln collection returns one saved Coal"),
                Crafting->WasLastResultAccepted());
        }
    }
    TestEqual(TEXT("Forty Kiln outputs were collected"), Inventory->GetQuantity(TEXT("Coal")), 40);
    TestEqual(TEXT("Kiln fuel consumes forty earned Wood"), Inventory->GetQuantity(TEXT("Wood")), 20);

    struct FSmeltBatch { FName Ore; FName Bar; EKalmalaSmelterOre Kind; int32 Count; };
    const FSmeltBatch SmeltBatches[] = {
        { TEXT("CopperOre"), TEXT("Copper"), EKalmalaSmelterOre::Copper, 10 },
        { TEXT("CopperOre"), TEXT("Copper"), EKalmalaSmelterOre::Copper, 10 },
        { TEXT("TinOre"), TEXT("Tin"), EKalmalaSmelterOre::Tin, 10 },
    };
    const FString SmelterId = TEXT("a10-fresh-smelter-") + RunId;
    const auto Smelt = [this, Smelter, Crafting, Inventory, &InteractWithStation,
        &WaitForServerAction, &SmelterId](const FSmeltBatch& Batch)
    {
        if (!InteractWithStation(Smelter, TEXT("SmelterKit"), SmelterId)) return false;
        for (int32 Unit = 0; Unit < Batch.Count; ++Unit)
        {
            WaitForServerAction();
            Crafting->ServerLoadSmelterItem(Batch.Ore);
            if (!TestTrue(TEXT("Smelter accepts one earned ore through its server request"),
                    Crafting->WasLastResultAccepted())) return false;
        }
        for (int32 Unit = 0; Unit < Batch.Count; ++Unit)
        {
            WaitForServerAction();
            Crafting->ServerLoadSmelterItem(TEXT("Coal"));
            if (!TestTrue(TEXT("Smelter accepts one Kiln Coal through its server request"),
                    Crafting->WasLastResultAccepted())) return false;
        }
        Smelter->Tick(FKalmalaSmelterState::JobDurationSeconds * Batch.Count);
        if (!TestEqual(TEXT("Production actor Tick completes the Smelter batch"),
                Smelter->GetSmelterState().GetOutputCount(Batch.Kind), Batch.Count)) return false;
        for (int32 Unit = 0; Unit < Batch.Count; ++Unit)
        {
            WaitForServerAction();
            Crafting->ServerCollectSmelterItem(Batch.Bar);
            if (!TestTrue(TEXT("Smelter collection returns one saved matching bar"),
                    Crafting->WasLastResultAccepted())) return false;
        }
        return true;
    };
    for (const FSmeltBatch& Batch : SmeltBatches)
        if (!Smelt(Batch)) return false;

    TestEqual(TEXT("Bronze recipe receives twenty Copper bars"), Inventory->GetQuantity(TEXT("Copper")), 20);
    TestEqual(TEXT("Bronze recipe receives ten Tin bars"), Inventory->GetQuantity(TEXT("Tin")), 10);
    TestEqual(TEXT("Smelting still requires no direct Bronze grant"), Inventory->GetQuantity(TEXT("Bronze")), 0);

    if (!InteractWithStation(Forge, TEXT("ForgeKit"), ForgeConstructionId)) return false;
    for (int32 Batch = 0; Batch < 2; ++Batch)
    {
        WaitForServerAction();
        Crafting->ServerCraft(TEXT("BronzeAlloy"), 5);
        if (!TestTrue(TEXT("Forge converts earned Copper and Tin into five Bronze bars"),
                Crafting->WasLastResultAccepted())) return false;
    }
    TestEqual(TEXT("Forge produces the ten Bronze bars required by the approved pick recipe"),
        Inventory->GetQuantity(TEXT("Bronze")), 10);

    WaitForServerAction();
    Crafting->ServerProgressTool(TEXT("BronzePick"));
    if (!TestTrue(TEXT("Forge acquisition creates Bronze Pick from ten earned Wood and Bronze"),
            Crafting->WasLastResultAccepted())) return false;
    TestEqual(TEXT("Bronze Pick is a real level-one carried tool"),
        Player->GetCarriedToolLevel(TEXT("BronzePick")), 1);
    TestEqual(TEXT("Bronze Pick recipe consumes ten Wood"), Inventory->GetQuantity(TEXT("Wood")), 10);
    TestEqual(TEXT("Bronze Pick recipe consumes ten Bronze"), Inventory->GetQuantity(TEXT("Bronze")), 0);

    FGuid BronzePickGuid;
    for (const FKalmalaToolState& Tool : Player->GetCarriedToolInventory())
        if (Tool.ItemId == TEXT("BronzePick")) BronzePickGuid = Tool.InstanceGuid;
    if (!TestTrue(TEXT("Fresh Bronze Pick has a durable physical identity"), BronzePickGuid.IsValid())) return false;

    if (!TestTrue(TEXT("Only the acquired Bronze Pick qualifies for Iron ore"),
            Player->GetCarriedToolLevel(TEXT("BronzePick")) == 1 && BronzePickGuid.IsValid())) return false;
    if (!HarvestOreNodes(TEXT("mire-bog-iron"), TEXT("BronzePick"), BronzePickGuid,
            TEXT("IronOre"), 4, IronFixtureLocation)) return false;
    TestEqual(TEXT("Bronze Pick mines four normal three-ore Iron nodes"),
        Inventory->GetQuantity(TEXT("IronOre")), 12);
    const FSmeltBatch IronBatch{ TEXT("IronOre"), TEXT("Iron"), EKalmalaSmelterOre::Iron, 10 };
    if (!Smelt(IronBatch)) return false;
    TestEqual(TEXT("Iron smelting spends the last ten earned Coal"), Inventory->GetQuantity(TEXT("Coal")), 0);
    TestEqual(TEXT("Iron bars are earned through Smelter production"), Inventory->GetQuantity(TEXT("Iron")), 10);

    if (!InteractWithStation(Forge, TEXT("ForgeKit"), ForgeConstructionId)) return false;
    WaitForServerAction();
    Crafting->ServerProgressTool(TEXT("IronPick"));
    if (!TestTrue(TEXT("Forge acquisition creates Iron Pick from ten earned Wood and Iron"),
            Crafting->WasLastResultAccepted())) return false;
    TestEqual(TEXT("Iron Pick is a real level-one carried tool"),
        Player->GetCarriedToolLevel(TEXT("IronPick")), 1);
    TestEqual(TEXT("Iron Pick recipe consumes the final ten Wood"), Inventory->GetQuantity(TEXT("Wood")), 0);
    TestEqual(TEXT("Iron Pick recipe consumes ten Iron bars"), Inventory->GetQuantity(TEXT("Iron")), 0);
    TestEqual(TEXT("One unprocessed Copper Ore remains from the seven nodes"),
        Inventory->GetQuantity(TEXT("CopperOre")), 1);
    TestEqual(TEXT("Two unprocessed Tin Ore remain from the four nodes"),
        Inventory->GetQuantity(TEXT("TinOre")), 2);
    TestEqual(TEXT("Two unprocessed Iron Ore remain from the four nodes"),
        Inventory->GetQuantity(TEXT("IronOre")), 2);

    TestEqual(TEXT("Every normally harvested fixture has a stable identity"), HarvestedFixtureIds.Num(), 75);
    const FString PopulationSlot = FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"),
        static_cast<unsigned long long>(WorldState->GetWorldGenerationConfig().WorldSeed));
    const auto* SavedPopulation = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromSlot(PopulationSlot, 0));
    if (!TestNotNull(TEXT("World source depletion SaveGame reloads from disk"), SavedPopulation)) return false;
    for (const FString& PersistentSpawnId : HarvestedFixtureIds)
        TestTrue(TEXT("Reloaded world save retains each normally harvested source identity"),
            SavedPopulation->IsHarvested(PersistentSpawnId));

    AddInfo(FString::Printf(TEXT("No-debug source-to-pick route exercised with deterministic source/station fixtures: %d Wood source interactions, 7 Copper, 4 Tin and 4 Iron nodes; 40 Kiln Coal; Bronze and Iron Picks acquired."),
        WoodSourceCount));
    return true;
}

#endif
