#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaForestLayout.h"
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
    World->DestroyWorld(false);
    return true;
}

#endif
