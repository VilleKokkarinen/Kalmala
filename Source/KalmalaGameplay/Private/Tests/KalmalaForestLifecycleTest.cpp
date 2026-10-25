#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaForestLayout.h"
#include "KalmalaForestResourceContract.h"
#include "KalmalaForestTundraTestSupport.h"
#include "KalmalaHarvestNode.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "ProceduralMeshComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestLifecycleTest,
    "Kalmala.Gameplay.Harvest.ForestReconstructionLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaForestLifecycleTest::RunTest(const FString& Parameters)
{
    const FKalmalaWorldGenerationConfig Config{418ull};
    FVector2D PatchCenter = FVector2D::ZeroVector;
    TArray<FKalmalaForestTreeDescriptor> Trees;
    if (!TestTrue(TEXT("Fixture finds an accepted Freezing Tundra Birch candidate"),
        KalmalaForestTundraTestSupport::FindPatch(Config, 1, PatchCenter, Trees))) return false;
    const FKalmalaForestTreeDescriptor Tree = Trees[0];
    TestEqual(TEXT("Shared descriptor keeps Tundra resource kind explicit"),
        Tree.TreeKind, EKalmalaForestTreeKind::FreezingTundraBirch);
    TestEqual(TEXT("Tundra descriptor preserves its original candidate identity"),
        Tree.PersistentSpawnId, FKalmalaForestLayout::GetPersistentSpawnId(Config, PatchCenter, Tree.CandidateIndex));
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
    const FTransform TreeTransform(FRotator(0, Tree.Yaw, 0), Tree.Location);
    const double TraceReach = Tree.CanopyRadius + 100.0;
    const FVector TraceStart = TreeTransform.TransformPosition(FVector(-TraceReach, 0, Tree.TrunkHeight * 0.5));
    const FVector TraceEnd = TreeTransform.TransformPosition(FVector(TraceReach, 0, Tree.TrunkHeight * 0.5));
    const auto CheckWorldSelection = [&](AKalmalaHarvestNode* ExpectedNode)
    {
        // Exercise both the gameplay query default and the explicit complex-mesh path.
        for (const bool bTraceComplex : { false, true })
        {
            FCollisionQueryParams QueryParams;
            QueryParams.bTraceComplex = bTraceComplex;
            FHitResult Hit;
            const bool bHit = World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
            if (ExpectedNode)
            {
                TestTrue(TEXT("World visibility query selects generated trunk"), bHit);
                TestTrue(TEXT("World query selects exact live forest actor"), Hit.GetActor() == ExpectedNode);
            }
            else
            {
                TestFalse(TEXT("Evicted tree leaves no blocking world collision"), bHit);
            }
        }
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
        FKalmalaForestResourceContract::GetSourceId(Tree.TreeKind));
    TestEqual(TEXT("Tundra materialization uses the existing Birch source"), Node->GetGatheringSourceId(),
        FName(TEXT("meadows-birch-trunk")));
    TestTrue(TEXT("Standing forest tree accepts axe attacks"), Node->CanReceiveAxeAttack());
    CheckStandingCollision(Node);
    CheckWorldSelection(Node);
    TestTrue(TEXT("Forest bypasses session-only depletion identity"), Node->GetM9ResourceDepletionId().IsEmpty());
    TestFalse(TEXT("Initialized forest identity cannot be replaced"),
        Node->InitializeForestServer(Config, PatchCenter, Tree.CandidateIndex));
    TestTrue(TEXT("Stream-out destroys node"), Node->Destroy());
    CheckWorldSelection(nullptr);
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
        CheckWorldSelection(Reloaded);
        Reloaded->Destroy();
        CheckWorldSelection(nullptr);
    }
    // Seed replicated session state directly: this isolates eviction/reconstruction
    // from the separate axe/persistence/pickup integration fixtures.
    const auto* StageProperty = FindFProperty<FByteProperty>(AKalmalaHarvestNode::StaticClass(), TEXT("TreeStage"));
    const auto* HealthProperty = FindFProperty<FFloatProperty>(AKalmalaHarvestNode::StaticClass(), TEXT("TreeHealth"));
    if (TestNotNull(TEXT("Tree stage property available"), StageProperty)
        && TestNotNull(TEXT("Tree health property available"), HealthProperty))
    {
        for (uint8 Stage = 0; Stage <= 2; ++Stage)
        {
            auto* Uncollected = SpawnNode();
            if (!TestNotNull(TEXT("Uncollected session node spawned"), Uncollected)) break;
            if (!TestTrue(TEXT("Session node reconstructs"),
                Uncollected->InitializeForestServer(Config, PatchCenter, Tree.CandidateIndex)))
            {
                Uncollected->Destroy();
                break;
            }
            // Partially damaged standing tree, partially damaged trunk, uncollected drop.
            const float SessionHealth = Stage == 2 ? 0.0f : 17.0f;
            StageProperty->SetPropertyValue_InContainer(Uncollected, Stage);
            HealthProperty->SetPropertyValue_InContainer(Uncollected, SessionHealth);
            TestEqual(TEXT("Fixture seeds requested session stage"),
                StageProperty->GetPropertyValue_InContainer(Uncollected), Stage);
            TestEqual(TEXT("Fixture seeds requested session health"),
                HealthProperty->GetPropertyValue_InContainer(Uncollected), SessionHealth);
            TestFalse(TEXT("Uncollected state is not harvested"), Uncollected->IsHarvested());
            TestTrue(TEXT("Eviction destroys uncollected session state"), Uncollected->Destroy());
            CheckWorldSelection(nullptr);
            TestEqual(TEXT("Uncollected eviction never records sparse depletion"), SparseCallbacks, 0);
            TestEqual(TEXT("Uncollected eviction never records session depletion"), SessionCallbacks, 0);

            auto* Restored = SpawnNode();
            if (!TestNotNull(TEXT("Session replacement spawned"), Restored)) break;
            if (TestTrue(TEXT("Uncollected candidate reconstructs again"),
                Restored->InitializeForestServer(Config, PatchCenter, Tree.CandidateIndex)))
            {
                TestEqual(TEXT("Reconstruction resets stage to standing"),
                    StageProperty->GetPropertyValue_InContainer(Restored), uint8(0));
                TestEqual(TEXT("Reconstruction resets tree health to full"),
                    HealthProperty->GetPropertyValue_InContainer(Restored), 100.0f);
                TestEqual(TEXT("Session reset retains sparse identity"), Restored->GetPersistentSpawnId(), Tree.PersistentSpawnId);
                TestTrue(TEXT("Session reset restores attackable unharvested tree"),
                    Restored->CanReceiveAxeAttack() && !Restored->IsHarvested());
                CheckStandingCollision(Restored);
                CheckWorldSelection(Restored);
            }
            Restored->Destroy();
            CheckWorldSelection(nullptr);
        }
    }
    World->DestroyWorld(false);
    TestEqual(TEXT("World teardown never records sparse depletion"), SparseCallbacks, 0);
    TestEqual(TEXT("World teardown never records session depletion"), SessionCallbacks, 0);
    return true;
}

#endif
