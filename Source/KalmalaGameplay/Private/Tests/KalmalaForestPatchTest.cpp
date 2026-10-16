#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaForestLayout.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "KalmalaCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestPatchLifecycleTest,
    "Kalmala.Gameplay.Harvest.ForestPatchLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestPatchLifecycleTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{418ull};
    FVector2D Center = FVector2D::ZeroVector;
    TArray<FKalmalaForestTreeDescriptor> Trees;
    for (int32 Y = -8; Y <= 8 && Trees.Num() < 2; ++Y)
    {
        for (int32 X = -8; X <= 8 && Trees.Num() < 2; ++X)
        {
            Center = FVector2D(X * 10000.0, Y * 10000.0);
            Trees = FKalmalaForestLayout::Generate(Config, Center);
        }
    }
    if (!TestTrue(TEXT("Fixture finds at least two generated trees"), Trees.Num() >= 2)) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Patch lifecycle world created"), World)) return false;
    auto* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("GameMode spawned without starting a session"), Mode))
    {
        World->DestroyWorld(false);
        return false;
    }
    Mode->WorldGenerationConfig = Config;
    Mode->TerrainPatchOrigin = Center;
    auto* Save = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    Save->InitializeForWorld(Config);
    Save->MarkHarvested(Trees[0].PersistentSpawnId);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Sparse forest save serializes in memory"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    Mode->PopulationSaveGame = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Sparse forest save reloads in memory"), Mode->PopulationSaveGame.Get()))
    {
        World->DestroyWorld(false);
        return false;
    }
    const FArrayProperty* SuppressionProperty = FindFProperty<FArrayProperty>(
        AKalmalaGeneratedTerrainPatch::StaticClass(), TEXT("SuppressedForestCandidates"));
    if (!TestNotNull(TEXT("Patch exposes reflected suppression array"), SuppressionProperty)
        || !TestTrue(TEXT("Suppression ordinals use integer elements"),
            SuppressionProperty && CastField<FIntProperty>(SuppressionProperty->Inner) != nullptr))
    {
        World->DestroyWorld(false);
        return false;
    }
    TArray<int32> ExpectedOrdinals;
    for (const auto& Tree : Trees) ExpectedOrdinals.Add(Tree.CandidateIndex);
    ExpectedOrdinals.Sort();
    const FIntPoint Coordinate = FIntPoint::ZeroValue;
    for (int32 Pass = 0; Pass < 2; ++Pass)
    {
        Mode->ActivateTerrainPatch(Coordinate);
        TestTrue(TEXT("Actual activation registers patch"), Mode->ActiveTerrainPatchCoordinates.Contains(Coordinate));
        const auto* Nodes = Mode->ActiveForestNodes.Find(Coordinate);
        auto* PatchEntry = Mode->ActiveTerrainPatches.Find(Coordinate);
        if (!TestNotNull(TEXT("Activation tracks forest actors"), Nodes)
            || !TestNotNull(TEXT("Activation tracks terrain actor"), PatchEntry)) break;
        auto* Patch = PatchEntry->Get();
        if (!TestNotNull(TEXT("Tracked terrain actor is live"), Patch)) break;
        // Read the replicated contract without exposing a production-only test accessor.
        const auto* Suppressed = SuppressionProperty->ContainerPtrToValuePtr<TArray<int32>>(Patch);
        TestTrue(TEXT("Saved and live candidates suppress exactly the canonical cosmetic ordinals"),
            *Suppressed == ExpectedOrdinals);
        TestTrue(TEXT("Saved depletion suppresses its original candidate ordinal"),
            Suppressed->Contains(Trees[0].CandidateIndex));
        for (int32 Index = 1; Index < Trees.Num(); ++Index)
            TestTrue(TEXT("Live replacement suppresses its original candidate ordinal"),
                Suppressed->Contains(Trees[Index].CandidateIndex));
        TestEqual(TEXT("Saved candidate has no live replacement"), Nodes->Num(), Trees.Num() - 1);
        TSet<FString> LiveIds;
        for (const auto& WeakNode : *Nodes)
        {
            auto* Node = WeakNode.Get();
            if (!TestNotNull(TEXT("Tracked replacement is live"), Node)) continue;
            LiveIds.Add(Node->GetPersistentSpawnId());
            TestTrue(TEXT("Replacement belongs to registered patch"), Node->GetOwner() == PatchEntry->Get());
            TestTrue(TEXT("Replacement starts standing and unharvested"), Node->CanReceiveAxeAttack() && !Node->IsHarvested());
        }
        TestFalse(TEXT("Saved depletion stays absent after activation"), LiveIds.Contains(Trees[0].PersistentSpawnId));
        for (int32 Index = 1; Index < Trees.Num(); ++Index)
            TestTrue(TEXT("Every undepleted identity reconstructs"), LiveIds.Contains(Trees[Index].PersistentSpawnId));
        const auto EvictedNodes = *Nodes;
        TWeakObjectPtr<AKalmalaGeneratedTerrainPatch> EvictedPatch = PatchEntry->Get();
        Mode->ActivateTerrainPatch(Coordinate);
        TestEqual(TEXT("Duplicate activation does not add patches"), Mode->ActiveTerrainPatches.Num(), 1);
        TestTrue(TEXT("Duplicate activation retains terrain actor"), Mode->ActiveTerrainPatches.FindChecked(Coordinate) == EvictedPatch.Get());
        TestEqual(TEXT("Duplicate activation retains actor count"), Mode->ActiveForestNodes.FindChecked(Coordinate).Num(), EvictedNodes.Num());
        TestTrue(TEXT("Duplicate activation retains canonical cosmetic suppression"),
            *SuppressionProperty->ContainerPtrToValuePtr<TArray<int32>>(Patch) == ExpectedOrdinals);
        // No player controllers: the actual neighborhood refresh must evict everything.
        Mode->RefreshTerrainPatchNeighborhoods();
        TestTrue(TEXT("Eviction clears all patch bookkeeping"), Mode->ActiveTerrainPatchCoordinates.IsEmpty()
            && Mode->ActiveTerrainPatches.IsEmpty() && Mode->ActiveForestNodes.IsEmpty());
        TestFalse(TEXT("Eviction destroys terrain actor"), EvictedPatch.IsValid());
        for (const auto& Node : EvictedNodes)
            TestFalse(TEXT("Eviction destroys forest actor"), Node.IsValid());
        TestTrue(TEXT("Eviction preserves saved depletion"), Mode->PopulationSaveGame->IsHarvested(Trees[0].PersistentSpawnId));
        for (int32 Index = 1; Index < Trees.Num(); ++Index)
            TestFalse(TEXT("Eviction never depletes uncollected tree"), Mode->PopulationSaveGame->IsHarvested(Trees[Index].PersistentSpawnId));
    }
    // A shared patch belongs to the union of player neighborhoods, not the first owner.
    auto* FirstController = World->SpawnActor<APlayerController>();
    auto* SecondController = World->SpawnActor<APlayerController>();
    FActorSpawnParameters PawnParameters;
    PawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* FirstPawn = World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
        FVector(Center.X, Center.Y, 0.0), FRotator::ZeroRotator, PawnParameters);
    auto* SecondPawn = World->SpawnActor<AKalmalaCharacter>(AKalmalaCharacter::StaticClass(),
        FVector(Center.X, Center.Y, 0.0), FRotator::ZeroRotator, PawnParameters);
    if (!TestNotNull(TEXT("First neighborhood controller created"), FirstController)
        || !TestNotNull(TEXT("Second neighborhood controller created"), SecondController)
        || !TestNotNull(TEXT("First neighborhood pawn created"), FirstPawn)
        || !TestNotNull(TEXT("Second neighborhood pawn created"), SecondPawn))
    {
        World->DestroyWorld(false);
        return false;
    }
    FirstController->Possess(FirstPawn);
    SecondController->Possess(SecondPawn);
    TestTrue(TEXT("Both controllers possess neighborhood pawns"),
        FirstController->GetPawn() == FirstPawn && SecondController->GetPawn() == SecondPawn);
    Mode->RefreshTerrainPatchNeighborhoods();
    auto* SharedPatchEntry = Mode->ActiveTerrainPatches.Find(Coordinate);
    const auto* SharedNodes = Mode->ActiveForestNodes.Find(Coordinate);
    if (!TestNotNull(TEXT("Shared neighborhood activates forest patch"), SharedPatchEntry)
        || !TestNotNull(TEXT("Shared neighborhood tracks forest nodes"), SharedNodes)
        || !TestNotNull(TEXT("Shared neighborhood terrain actor is live"), SharedPatchEntry->Get()))
    {
        World->DestroyWorld(false);
        return false;
    }
    TWeakObjectPtr<AKalmalaGeneratedTerrainPatch> SharedPatch = SharedPatchEntry->Get();
    const auto RetainedNodes = *SharedNodes;
    TestEqual(TEXT("Shared neighborhood excludes saved depletion"), RetainedNodes.Num(), Trees.Num() - 1);
    const int32 SharedPatchCount = Mode->ActiveTerrainPatches.Num();
    TestEqual(TEXT("Shared radius-one neighborhood has nine patches"), SharedPatchCount, 9);
    // Adjacent radius-one neighborhoods overlap in six patches: retain all nine
    // stationary patches and evict only the three newly introduced patches on return.
    const auto OriginalPatches = Mode->ActiveTerrainPatches;
    const auto OriginalForest = Mode->ActiveForestNodes;
    const FVector2D AdjacentCenter = FKalmalaTerrainPatchLayout::GetPatchCenter(Center, 1, 0);
    TestTrue(TEXT("First player moves to adjacent neighborhood"), FirstPawn->SetActorLocation(
        FVector(AdjacentCenter.X, AdjacentCenter.Y, 0.0), false, nullptr, ETeleportType::TeleportPhysics));
    Mode->RefreshTerrainPatchNeighborhoods();
    TestEqual(TEXT("Adjacent neighborhood union has twelve patches"), Mode->ActiveTerrainPatches.Num(), 12);
    TArray<TWeakObjectPtr<AKalmalaGeneratedTerrainPatch>> IntroducedPatches;
    TArray<TWeakObjectPtr<AKalmalaHarvestNode>> IntroducedNodes;
    for (const auto& Entry : Mode->ActiveTerrainPatches)
    {
        if (!OriginalPatches.Contains(Entry.Key))
        {
            TestEqual(TEXT("Only the leading column is introduced"), Entry.Key.X, 2);
            IntroducedPatches.Add(Entry.Value.Get());
            if (const auto* Forest = Mode->ActiveForestNodes.Find(Entry.Key))
                IntroducedNodes.Append(*Forest);
        }
    }
    TestEqual(TEXT("Adjacent movement introduces exactly three patches"), IntroducedPatches.Num(), 3);
    TestTrue(TEXT("First player returns from adjacent neighborhood"), FirstPawn->SetActorLocation(
        FVector(Center.X, Center.Y, 0.0), false, nullptr, ETeleportType::TeleportPhysics));
    Mode->RefreshTerrainPatchNeighborhoods();
    TestEqual(TEXT("Adjacent return restores nine-patch union"), Mode->ActiveTerrainPatches.Num(), SharedPatchCount);
    for (const auto& Entry : OriginalPatches)
    {
        TestTrue(TEXT("Adjacent round trip retains every stationary terrain actor"),
            IsValid(Entry.Value.Get()) && Mode->ActiveTerrainPatches.FindRef(Entry.Key) == Entry.Value);
        const auto* Forest = Mode->ActiveForestNodes.Find(Entry.Key);
        TestTrue(TEXT("Adjacent round trip retains every stationary forest actor list"),
            Forest && *Forest == OriginalForest.FindChecked(Entry.Key));
    }
    for (const auto& Patch : IntroducedPatches)
        TestFalse(TEXT("Adjacent return destroys introduced terrain actors"), Patch.IsValid());
    for (const auto& Node : IntroducedNodes)
        TestFalse(TEXT("Adjacent return destroys introduced forest actors"), Node.IsValid());
    // Separate radius-one neighborhoods fit within the existing 25-patch budget.
    const FIntPoint RemoteCoordinate(4, 0);
    const FVector2D RemoteCenter = FKalmalaTerrainPatchLayout::GetPatchCenter(Center, 4, 0);
    TestTrue(TEXT("First player moves to disjoint neighborhood"), FirstPawn->SetActorLocation(
        FVector(RemoteCenter.X, RemoteCenter.Y, 0.0), false, nullptr, ETeleportType::TeleportPhysics));
    Mode->RefreshTerrainPatchNeighborhoods();
    TestEqual(TEXT("Disjoint players retain both complete neighborhoods"), Mode->ActiveTerrainPatches.Num(), SharedPatchCount * 2);
    TestTrue(TEXT("Stationary player retains exact original patch during movement"),
        Mode->ActiveTerrainPatches.FindRef(Coordinate) == SharedPatch.Get());
    const auto* StationaryNodes = Mode->ActiveForestNodes.Find(Coordinate);
    TestTrue(TEXT("Stationary player retains exact forest actors during movement"),
        StationaryNodes && *StationaryNodes == RetainedNodes);
    auto* RemotePatchEntry = Mode->ActiveTerrainPatches.Find(RemoteCoordinate);
    const auto* RemoteNodes = Mode->ActiveForestNodes.Find(RemoteCoordinate);
    if (!TestNotNull(TEXT("Movement activates remote terrain patch"), RemotePatchEntry)
        || !TestNotNull(TEXT("Movement tracks remote forest list"), RemoteNodes))
    {
        World->DestroyWorld(false);
        return false;
    }
    TWeakObjectPtr<AKalmalaGeneratedTerrainPatch> RemotePatch = RemotePatchEntry->Get();
    TestTrue(TEXT("Remote terrain actor is live"), RemotePatch.IsValid());
    const auto DepartedNodes = *RemoteNodes;
    TestTrue(TEXT("First player returns to shared neighborhood"), FirstPawn->SetActorLocation(
        FVector(Center.X, Center.Y, 0.0), false, nullptr, ETeleportType::TeleportPhysics));
    Mode->RefreshTerrainPatchNeighborhoods();
    TestEqual(TEXT("Returning player collapses union to shared neighborhood"), Mode->ActiveTerrainPatches.Num(), SharedPatchCount);
    TestFalse(TEXT("Departure destroys remote terrain actor"), RemotePatch.IsValid());
    TestFalse(TEXT("Departure removes remote forest tracking"), Mode->ActiveForestNodes.Contains(RemoteCoordinate));
    TestFalse(TEXT("Departure removes remote coordinate tracking"), Mode->ActiveTerrainPatchCoordinates.Contains(RemoteCoordinate));
    for (const auto& Node : DepartedNodes)
        TestFalse(TEXT("Departure destroys remote forest actors"), Node.IsValid());
    TestTrue(TEXT("Return retains original shared terrain actor"),
        Mode->ActiveTerrainPatches.FindRef(Coordinate) == SharedPatch.Get() && SharedPatch.IsValid());
    const auto* ReturnedNodes = Mode->ActiveForestNodes.Find(Coordinate);
    TestTrue(TEXT("Return retains original shared forest actors"), ReturnedNodes && *ReturnedNodes == RetainedNodes);
    FirstController->UnPossess();
    Mode->RefreshTerrainPatchNeighborhoods();
    TestEqual(TEXT("Remaining player retains neighborhood count"), Mode->ActiveTerrainPatches.Num(), SharedPatchCount);
    TestTrue(TEXT("Remaining player retains exact shared terrain actor"),
        Mode->ActiveTerrainPatches.FindRef(Coordinate) == SharedPatch.Get() && SharedPatch.IsValid());
    const auto* RemainingNodes = Mode->ActiveForestNodes.Find(Coordinate);
    TestTrue(TEXT("Remaining player retains exact forest actor list"), RemainingNodes && *RemainingNodes == RetainedNodes);
    for (const auto& Node : RetainedNodes)
        TestTrue(TEXT("First player leaving never destroys shared forest actors"), Node.IsValid());
    SecondController->UnPossess();
    Mode->RefreshTerrainPatchNeighborhoods();
    TestTrue(TEXT("Last player leaving clears neighborhood tracking"), Mode->ActiveTerrainPatchCoordinates.IsEmpty()
        && Mode->ActiveTerrainPatches.IsEmpty() && Mode->ActiveForestNodes.IsEmpty());
    TestFalse(TEXT("Last player leaving destroys shared terrain actor"), SharedPatch.IsValid());
    for (const auto& Node : RetainedNodes)
        TestFalse(TEXT("Last player leaving destroys shared forest actors"), Node.IsValid());
    TestTrue(TEXT("Player departure preserves prior saved depletion"), Mode->PopulationSaveGame->IsHarvested(Trees[0].PersistentSpawnId));
    for (int32 Index = 1; Index < Trees.Num(); ++Index)
        TestFalse(TEXT("Player departure never harvests uncollected trees"), Mode->PopulationSaveGame->IsHarvested(Trees[Index].PersistentSpawnId));
    FirstController->Destroy();
    SecondController->Destroy();
    FirstPawn->Destroy();
    SecondPawn->Destroy();
    // Missing or foreign-world saves must not hide this world's candidates.
    for (int32 SaveCase = 0; SaveCase < 2; ++SaveCase)
    {
        UKalmalaWorldPopulationSaveGame* ForeignSave = nullptr;
        if (SaveCase == 0)
        {
            Mode->PopulationSaveGame = nullptr;
        }
        else
        {
            ForeignSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
            ForeignSave->InitializeForWorld(FKalmalaWorldGenerationConfig{Config.WorldSeed + 1ull});
            ForeignSave->MarkHarvested(Trees[0].PersistentSpawnId);
            TestFalse(TEXT("Foreign save explicitly fails world matching"), ForeignSave->MatchesWorld(Config));
            Mode->PopulationSaveGame = ForeignSave;
        }
        Mode->ActivateTerrainPatch(Coordinate);
        const auto* Nodes = Mode->ActiveForestNodes.Find(Coordinate);
        auto* PatchEntry = Mode->ActiveTerrainPatches.Find(Coordinate);
        if (!TestNotNull(TEXT("Save guard activation tracks forest actors"), Nodes)
            || !TestNotNull(TEXT("Save guard activation tracks patch"), PatchEntry)
            || !TestNotNull(TEXT("Save guard terrain actor is live"), PatchEntry->Get())) break;
        TestEqual(TEXT("Missing or mismatched save retains every candidate"), Nodes->Num(), Trees.Num());
        TSet<FString> LiveIds;
        for (const auto& WeakNode : *Nodes)
        {
            if (auto* Node = WeakNode.Get())
            {
                LiveIds.Add(Node->GetPersistentSpawnId());
                // Exercise the production delegate binding and rejection guard only.
                // A matching-save broadcast would write a disk slot; real pickup is separate coverage.
                TestTrue(TEXT("Materialized node retains GameMode depletion binding"), Node->OnHarvested.IsBoundToObject(Mode));
                Node->OnHarvested.Broadcast(Node->GetPersistentSpawnId());
            }
        }
        for (const auto& Tree : Trees)
            TestTrue(TEXT("Save guard preserves each generated identity"), LiveIds.Contains(Tree.PersistentSpawnId));
        TestTrue(TEXT("Save guard activation suppresses exactly all represented cosmetics"),
            *SuppressionProperty->ContainerPtrToValuePtr<TArray<int32>>(PatchEntry->Get()) == ExpectedOrdinals);
        if (ForeignSave)
        {
            TestTrue(TEXT("Rejected callback preserves foreign save's existing depletion"),
                ForeignSave->IsHarvested(Trees[0].PersistentSpawnId));
            for (int32 Index = 1; Index < Trees.Num(); ++Index)
                TestFalse(TEXT("Bound callback cannot contaminate a foreign world's save"),
                    ForeignSave->IsHarvested(Trees[Index].PersistentSpawnId));
        }
        const auto EvictedNodes = *Nodes;
        Mode->RefreshTerrainPatchNeighborhoods();
        TestTrue(TEXT("Save guard eviction clears tracking"), Mode->ActiveTerrainPatchCoordinates.IsEmpty()
            && Mode->ActiveTerrainPatches.IsEmpty() && Mode->ActiveForestNodes.IsEmpty());
        for (const auto& Node : EvictedNodes)
            TestFalse(TEXT("Save guard eviction destroys replacements"), Node.IsValid());
    }
    // Run the production-bound callback with only its final disk boundary replaced.
    // Keep the previously seeded ID and persist a second tree, then reconstruct
    // from the bytes captured at write time rather than the live save object.
    Mode->PopulationSaveGame = Save;
    int32 WriteCalls = 0;
    TArray<uint8> WrittenBytes;
    Mode->PopulationSaveWriterForTests = [&](UKalmalaWorldPopulationSaveGame* WrittenSave,
        const FString& Slot, const int32 UserIndex)
    {
        ++WriteCalls;
        TestTrue(TEXT("Production callback writes its current world save"), WrittenSave == Save);
        TestEqual(TEXT("Production callback selects the seed-scoped population slot"), Slot,
            FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"), Config.WorldSeed));
        TestEqual(TEXT("Production callback retains population user index"), UserIndex, 0);
        TestTrue(TEXT("Production callback marks depletion before writing"),
            WrittenSave->IsHarvested(Trees[1].PersistentSpawnId));
        TestTrue(TEXT("Production callback write serializes in memory"),
            UGameplayStatics::SaveGameToMemory(WrittenSave, WrittenBytes));
    };
    // Rejected production callbacks must never reach the final disk boundary.
    Mode->RecordHarvestedSpawn(FString());
    TestEqual(TEXT("Empty harvest identity never writes population save"), WriteCalls, 0);
    Mode->SetRole(ROLE_SimulatedProxy);
    Mode->RecordHarvestedSpawn(Trees[1].PersistentSpawnId);
    Mode->SetRole(ROLE_Authority);
    TestEqual(TEXT("Non-authoritative harvest callback never writes population save"), WriteCalls, 0);
    TestFalse(TEXT("Rejected callbacks never mark the matching save"), Save->IsHarvested(Trees[1].PersistentSpawnId));
    auto* RejectedWorldSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    RejectedWorldSave->InitializeForWorld(FKalmalaWorldGenerationConfig{Config.WorldSeed + 1ull});
    Mode->PopulationSaveGame = RejectedWorldSave;
    Mode->RecordHarvestedSpawn(Trees[1].PersistentSpawnId);
    TestEqual(TEXT("Foreign-world harvest callback never writes population save"), WriteCalls, 0);
    TestFalse(TEXT("Foreign-world callback never contaminates its save"), RejectedWorldSave->IsHarvested(Trees[1].PersistentSpawnId));
    Mode->PopulationSaveGame = Save;
    Mode->ActivateTerrainPatch(Coordinate);
    TestEqual(TEXT("Activation does not write sparse depletion"), WriteCalls, 0);
    const auto* CallbackNodes = Mode->ActiveForestNodes.Find(Coordinate);
    if (TestNotNull(TEXT("Callback fixture activates forest actors"), CallbackNodes))
    {
        AKalmalaHarvestNode* CallbackNode = nullptr;
        for (const auto& Node : *CallbackNodes)
            if (Node.IsValid() && Node->GetPersistentSpawnId() == Trees[1].PersistentSpawnId) CallbackNode = Node.Get();
        if (TestNotNull(TEXT("Callback fixture finds the exact uncollected tree"), CallbackNode))
        {
            TestTrue(TEXT("Successful callback uses production GameMode binding"), CallbackNode->OnHarvested.IsBoundToObject(Mode));
            CallbackNode->OnHarvested.Broadcast(CallbackNode->GetPersistentSpawnId());
            TestEqual(TEXT("One bound depletion callback performs one write"), WriteCalls, 1);
        }
    }
    Mode->RefreshTerrainPatchNeighborhoods();
    TestEqual(TEXT("Callback fixture eviction never writes depletion"), WriteCalls, 1);
    auto* WrittenSave = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(WrittenBytes));
    if (TestNotNull(TEXT("Production callback bytes reload as population save"), WrittenSave))
    {
        TestTrue(TEXT("Production write retains world identity"), WrittenSave->MatchesWorld(Config));
        Mode->PopulationSaveGame = WrittenSave;
        Mode->ActivateTerrainPatch(Coordinate);
        const auto* ReloadNodes = Mode->ActiveForestNodes.Find(Coordinate);
        if (TestNotNull(TEXT("Production callback snapshot reconstructs patch"), ReloadNodes))
        {
            TestEqual(TEXT("Production snapshot excludes both depleted trees"), ReloadNodes->Num(), Trees.Num() - 2);
            for (const auto& Tree : Trees)
            {
                const bool bDepleted = Tree.PersistentSpawnId == Trees[0].PersistentSpawnId
                    || Tree.PersistentSpawnId == Trees[1].PersistentSpawnId;
                TestEqual(TEXT("Production snapshot preserves exactly the expected depletion"),
                    WrittenSave->IsHarvested(Tree.PersistentSpawnId), bDepleted);
                bool bLive = false;
                for (const auto& Node : *ReloadNodes)
                    bLive |= Node.IsValid() && Node->GetPersistentSpawnId() == Tree.PersistentSpawnId;
                TestEqual(TEXT("Production snapshot reconstructs only uncollected candidates"), bLive, !bDepleted);
                TestFalse(TEXT("Production harvesting does not record creature defeat"), WrittenSave->IsDefeated(Tree.PersistentSpawnId));
            }
        }
        Mode->RefreshTerrainPatchNeighborhoods();
        TestEqual(TEXT("Production snapshot reload and eviction add no writes"), WriteCalls, 1);
    }
    Mode->PopulationSaveWriterForTests = {};
    World->DestroyWorld(false);
    return true;
}

#endif
