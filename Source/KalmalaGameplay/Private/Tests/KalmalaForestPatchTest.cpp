#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaForestLayout.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
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
    World->DestroyWorld(false);
    return true;
}

#endif
