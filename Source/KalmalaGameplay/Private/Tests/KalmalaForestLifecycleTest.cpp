#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaForestLayout.h"
#include "KalmalaHarvestNode.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "ProceduralMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestLifecycleTest,
    "Kalmala.Gameplay.Harvest.ForestReconstructionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestLifecycleTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{418ull};
    FVector2D PatchCenter = FVector2D::ZeroVector;
    TArray<FKalmalaForestTreeDescriptor> Trees;
    // Find a real accepted candidate, without hard-coding a filtered array ordinal.
    for (int32 Y = -8; Y <= 8 && Trees.IsEmpty(); ++Y)
    {
        for (int32 X = -8; X <= 8 && Trees.IsEmpty(); ++X)
        {
            PatchCenter = FVector2D(X * 10000.0, Y * 10000.0);
            Trees = FKalmalaForestLayout::Generate(Config, PatchCenter);
        }
    }
    if (!TestFalse(TEXT("Fixture finds an accepted generated forest candidate"), Trees.IsEmpty())) return false;
    const FKalmalaForestTreeDescriptor Tree = Trees[0];
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Forest lifecycle world created"), World)) return false;

    auto* PatchOwner = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("Patch relevance owner spawned"), PatchOwner))
    {
        World->DestroyWorld(false);
        return false;
    }
    PatchOwner->bAlwaysRelevant = true;

    int32 SparseCallbacks = 0;
    int32 SessionCallbacks = 0;
    const auto SpawnNode = [&]()
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = PatchOwner;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Node = World->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(),
            FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
        if (Node)
        {
            Node->OnHarvested.AddLambda([&](const FString&) { ++SparseCallbacks; });
            Node->OnM9ResourceDepleted.AddLambda([&](const FString&) { ++SessionCallbacks; });
        }
        return Node;
    };
    const auto CheckStandingCollision = [&](AKalmalaHarvestNode* StandingNode)
    {
        auto* Mesh = StandingNode->FindComponentByClass<UProceduralMeshComponent>();
        if (!TestNotNull(TEXT("Reconstructed tree has procedural collision mesh"), Mesh)) return;
        TestEqual(TEXT("Standing mesh supports query traces"), Mesh->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
        TestEqual(TEXT("Standing mesh blocks attack visibility channel"),
            Mesh->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
        FCollisionQueryParams QueryParams;
        QueryParams.bTraceComplex = true;
        const FTransform Transform = StandingNode->GetActorTransform();
        const double Reach = Tree.CanopyRadius + 100.0;
        const double Height = Tree.TrunkHeight * 0.5;
        FHitResult Hit;
        // Above the root sphere and below the canopy: only the generated trunk can hit.
        TestTrue(TEXT("Attack trace hits reconstructed trunk above root sphere"),
            Mesh->LineTraceComponent(Hit,
                Transform.TransformPosition(FVector(-Reach, 0, Height)),
                Transform.TransformPosition(FVector(Reach, 0, Height)), QueryParams));
        TestTrue(TEXT("Trunk trace resolves to the authoritative harvest actor"), Hit.GetActor() == StandingNode);
        FHitResult Miss;
        TestFalse(TEXT("Trace beside trunk below canopy misses tree mesh"),
            Mesh->LineTraceComponent(Miss,
                Transform.TransformPosition(FVector(-Reach, Tree.TrunkRadius * 2.0, Height)),
                Transform.TransformPosition(FVector(Reach, Tree.TrunkRadius * 2.0, Height)), QueryParams));
    };
    auto* Node = SpawnNode();
    if (!TestNotNull(TEXT("Forest node spawned"), Node))
    {
        World->DestroyWorld(false);
        return false;
    }
    TestFalse(TEXT("Legacy nodes retain independent relevance"), Node->bNetUseOwnerRelevancy);
    Node->SetRole(ROLE_SimulatedProxy);
    TestFalse(TEXT("Client cannot reconstruct a forest node"),
        Node->InitializeForestServer(Config, PatchCenter, Tree.CandidateIndex));
    TestTrue(TEXT("Client rejection preserves empty identity"), Node->GetPersistentSpawnId().IsEmpty());
    TestFalse(TEXT("Client rejection preserves independent relevance"), Node->bNetUseOwnerRelevancy);
    Node->SetRole(ROLE_Authority);
    TestFalse(TEXT("Missing candidate cannot initialize"), Node->InitializeForestServer(Config, PatchCenter, INDEX_NONE));
    TestTrue(TEXT("Server reconstructs accepted candidate"), Node->InitializeForestServer(Config, PatchCenter, Tree.CandidateIndex));
    TestEqual(TEXT("Stable sparse identity survives reconstruction"), Node->GetPersistentSpawnId(), Tree.PersistentSpawnId);
    TestTrue(TEXT("Reconstruction restores generated position"), Node->GetActorLocation().Equals(Tree.Location));
    TestTrue(TEXT("Reconstruction restores generated yaw"), Node->GetActorRotation().Equals(FRotator(0, Tree.Yaw, 0)));
    TestTrue(TEXT("Generated transforms replicate"), Node->IsReplicatingMovement());
    TestTrue(TEXT("Generated trees use patch owner relevance"), Node->bNetUseOwnerRelevancy);
    TestTrue(TEXT("Reconstruction retains patch relevance owner"), Node->GetOwner() == PatchOwner);
    TestEqual(TEXT("Biome selects canonical trunk source"), Node->GetGatheringSourceId(),
        FName(Tree.bElderwood ? TEXT("elderwood-ironheart-trunk") : TEXT("meadows-birch-trunk")));
    TestTrue(TEXT("Standing forest tree accepts axe attacks"), Node->CanReceiveAxeAttack());
    CheckStandingCollision(Node);
    TestTrue(TEXT("Forest bypasses session-only depletion identity"), Node->GetM9ResourceDepletionId().IsEmpty());
    TestFalse(TEXT("Initialized forest identity cannot be replaced"),
        Node->InitializeForestServer(Config, PatchCenter, Tree.CandidateIndex));
    TestTrue(TEXT("Stream-out destroys node"), Node->Destroy());
    TestEqual(TEXT("Stream-out never records sparse depletion"), SparseCallbacks, 0);
    TestEqual(TEXT("Stream-out never records session depletion"), SessionCallbacks, 0);

    auto* Reloaded = SpawnNode();
    if (TestNotNull(TEXT("Replacement node spawned after stream-out"), Reloaded))
    {
        TestTrue(TEXT("Uncollected tree reconstructs after stream-out"),
            Reloaded->InitializeForestServer(Config, PatchCenter, Tree.CandidateIndex));
        TestEqual(TEXT("Reload retains original sparse identity"), Reloaded->GetPersistentSpawnId(), Tree.PersistentSpawnId);
        TestTrue(TEXT("Reload restores patch owner relevance"), Reloaded->bNetUseOwnerRelevancy
            && Reloaded->GetOwner() == PatchOwner);
        TestTrue(TEXT("Reload restores standing state"), Reloaded->CanReceiveAxeAttack()
            && !Reloaded->IsFelledTrunk() && !Reloaded->IsWoodDrop() && !Reloaded->IsHarvested());
        CheckStandingCollision(Reloaded);
        Reloaded->Destroy();
    }
    World->DestroyWorld(false);
    TestEqual(TEXT("World teardown never records sparse depletion"), SparseCallbacks, 0);
    TestEqual(TEXT("World teardown never records session depletion"), SessionCallbacks, 0);
    return true;
}

#endif
