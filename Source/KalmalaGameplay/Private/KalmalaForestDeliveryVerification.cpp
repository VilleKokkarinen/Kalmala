#include "KalmalaCharacter.h"
#include "KalmalaForestLayout.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaWorldGenerationGameState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AKalmalaCharacter::VerifyForestDelivery(const float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestDeliveryTest")) || !IsLocallyControlled()) return;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || State->PlayerArray.Num() < 2) return;
    ForestDeliveryElapsed += DeltaSeconds;
    if (ForestDeliveryElapsed < 1) return;
    ForestDeliveryElapsed = 0;
    const auto& Config = State->GetWorldGenerationConfig();
    for (TActorIterator<AKalmalaGeneratedTerrainPatch> Patch(GetWorld()); Patch; ++Patch)
    {
        if (!Patch->HasGenerationData()) continue;
        const auto Trees = FKalmalaForestLayout::Generate(Config, Patch->GetGeneratedPatchCenter());
        if (Trees.IsEmpty()) continue;
        const FString Key = Trees[0].PersistentSpawnId;
        if (ForestDeliveryReported.Contains(Key)) continue;
        const auto& Suppressed = Patch->GetSuppressedForestCandidatesForVerification();
        // Fresh isolated saves: every candidate must have exactly one standing
        // replacement and no decorative duplicate, including on remote peers.
        if (Suppressed.Num() != Trees.Num() || Patch->GetCosmeticForestCountForVerification() != 0) continue;
        TArray<AKalmalaHarvestNode*> Owned;
        for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
            if (Node->GetOwner() == *Patch) Owned.Add(*Node);
        if (Owned.Num() != Trees.Num())
        {
            if (ForestDeliveryDiagnostics++ < 20)
                UE_LOG(LogTemp, Display, TEXT("Forest delivery pending: Authority=%d Key=%s Owned=%d Expected=%d"), HasAuthority(), *Key, Owned.Num(), Trees.Num());
            continue;
        }
        bool bMatches = true;
        FString Ids;
        for (const auto& Tree : Trees)
        {
            const auto* Found = Owned.FindByPredicate([&Tree](const auto* Node) { return Node->GetPersistentSpawnId() == Tree.PersistentSpawnId; });
            const auto* Node = Found ? *Found : nullptr;
            const FVector4 Dimensions(Tree.TrunkRadius, Tree.TrunkHeight, Tree.CanopyRadius, Tree.CanopyHeight);
            bMatches &= Suppressed.Contains(Tree.CandidateIndex) && Node
                && Node->GetActorLocation().Equals(Tree.Location, 1.0)
                // Replicated movement rounds location to whole centimetres and
                // compresses yaw to a byte (half-step error at most 360/512).
                && FMath::Abs(FMath::FindDeltaAngleDegrees(Node->GetActorRotation().Yaw, Tree.Yaw)) <= 360.0f / 512.0f + 0.01f
                && Node->GetForestDimensionsForVerification().Equals(Dimensions, 0.01f)
                && Node->GetGatheringSourceId() == (Tree.bElderwood ? FName(TEXT("elderwood-ironheart-trunk")) : FName(TEXT("meadows-birch-trunk")))
                && Node->CanReceiveAxeAttack() && !Node->IsFelledTrunk() && !Node->IsWoodDrop()
                && (!HasAuthority() || Node->bNetUseOwnerRelevancy) && Node->IsReplicatingMovement();
            Ids += Tree.PersistentSpawnId + TEXT(";");
            if (!bMatches && Node && ForestDeliveryDiagnostics++ < 20)
            {
                UE_LOG(LogTemp, Display, TEXT("Forest delivery pending actor: Id=%s Distance=%.3f Yaw=%.3f Shape=%d Source=%s Standing=%d Movement=%d"),
                    *Tree.PersistentSpawnId, FVector::Distance(Node->GetActorLocation(), Tree.Location),
                    FMath::FindDeltaAngleDegrees(Node->GetActorRotation().Yaw, Tree.Yaw),
                    Node->GetForestDimensionsForVerification().Equals(Dimensions, 0.01f), *Node->GetGatheringSourceId().ToString(),
                    Node->CanReceiveAxeAttack() && !Node->IsFelledTrunk() && !Node->IsWoodDrop(), Node->IsReplicatingMovement());
                break;
            }
        }
        if (!bMatches) continue; // Replication properties/actor channels may arrive in different frames.
        ForestDeliveryReported.Add(Key);
        UE_LOG(LogTemp, Display, TEXT("Forest delivery: Passed=1 Authority=%d Seed=%llu Trees=%d Cosmetics=0 Ids=%s"),
            HasAuthority(), Config.WorldSeed, Trees.Num(), *Ids);
    }
#endif
}
