#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
            const bool bNodeMatches = Suppressed.Contains(Tree.CandidateIndex) && Node
                && Node->GetActorLocation().Equals(Tree.Location, 1.0)
                // Replicated movement rounds location to whole centimetres and
                // compresses yaw to a byte (half-step error at most 360/512).
                && FMath::Abs(FMath::FindDeltaAngleDegrees(Node->GetActorRotation().Yaw, Tree.Yaw)) <= 360.0f / 512.0f + 0.01f
                && Node->GetForestDimensionsForVerification().Equals(Dimensions, 0.01f)
                && Node->GetGatheringSourceId() == (Tree.bElderwood ? FName(TEXT("elderwood-ironheart-trunk")) : FName(TEXT("meadows-birch-trunk")))
                && !Node->IsHarvested()
                && (!HasAuthority() || Node->bNetUseOwnerRelevancy) && Node->IsReplicatingMovement();
            const bool bStanding = Node && Node->CanReceiveAxeAttack() && !Node->IsFelledTrunk() && !Node->IsWoodDrop();
            bMatches &= bNodeMatches && bStanding;
            // Inspect current replicated state even after standing delivery was
            // reported. A late join may first observe a trunk or wood bundle.
            if (bNodeMatches && (Node->IsFelledTrunk() || Node->IsWoodDrop()))
            {
                const FString Stage = Node->IsFelledTrunk() ? TEXT("Trunk") : TEXT("Drop");
                const FString StageKey = Tree.PersistentSpawnId + TEXT(":") + Stage;
                if (!ForestDeliveryReported.Contains(StageKey))
                {
                    ForestDeliveryReported.Add(StageKey);
                    UE_LOG(LogTemp, Display, TEXT("Forest stage delivery: Passed=1 Authority=%d Seed=%llu Id=%s Stage=%s Cosmetics=0"),
                        HasAuthority(), Config.WorldSeed, *Tree.PersistentSpawnId, *Stage);
                }
            }
            Ids += Tree.PersistentSpawnId + TEXT(";");
            if (!bNodeMatches && Node && ForestDeliveryDiagnostics++ < 20)
            {
                UE_LOG(LogTemp, Display, TEXT("Forest delivery pending actor: Id=%s Distance=%.3f Yaw=%.3f Shape=%d Source=%s Standing=%d Movement=%d"),
                    *Tree.PersistentSpawnId, FVector::Distance(Node->GetActorLocation(), Tree.Location),
                    FMath::FindDeltaAngleDegrees(Node->GetActorRotation().Yaw, Tree.Yaw),
                    Node->GetForestDimensionsForVerification().Equals(Dimensions, 0.01f), *Node->GetGatheringSourceId().ToString(),
                    Node->CanReceiveAxeAttack() && !Node->IsFelledTrunk() && !Node->IsWoodDrop(), Node->IsReplicatingMovement());
            }
        }
        if (!bMatches || ForestDeliveryReported.Contains(Key)) continue; // Replication properties/actor channels may arrive in different frames.
        ForestDeliveryReported.Add(Key);
        UE_LOG(LogTemp, Display, TEXT("Forest delivery: Passed=1 Authority=%d Seed=%llu Trees=%d Cosmetics=0 Ids=%s"),
            HasAuthority(), Config.WorldSeed, Trees.Num(), *Ids);
    }
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestStageDriver"))) return;
    // Standing coverage must be captured before the fixture mutates any tree.
    int32 StandingPatches = 0;
    for (const FString& Report : ForestDeliveryReported)
        if (!Report.EndsWith(TEXT(":Trunk")) && !Report.EndsWith(TEXT(":Drop"))) ++StandingPatches;
    if (StandingPatches < 9) return;
    if (HasAuthority())
    {
        ForestDriverReadySeconds += 1;
        if (bForestDriverPrepared || ForestDriverReadySeconds < 15) return;
        AKalmalaCharacter* Remote = nullptr;
        for (TActorIterator<AKalmalaCharacter> Pawn(GetWorld()); Pawn; ++Pawn)
            if (Pawn->GetController() && !Pawn->IsLocallyControlled()) { Remote = *Pawn; break; }
        if (!Remote) return;
        AKalmalaHarvestNode* Target = nullptr;
        for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
            if (Node->GetPersistentSpawnId().StartsWith(TEXT("forest:v1:")) && Node->CanReceiveAxeAttack()
                && (!Target || FVector::DistSquared(Node->GetActorLocation(), Remote->GetActorLocation())
                    < FVector::DistSquared(Target->GetActorLocation(), Remote->GetActorLocation()))) Target = *Node;
        if (!Target) return;
        FKalmalaToolState Axe;
        Axe.ItemId = TEXT("IronAxe");
        Axe.InstanceGuid = FGuid::NewGuid();
        Axe.ToolLevel = 1;
        Axe.Durability = FKalmalaToolLifecycleContract::GetMaxDurability(Axe.ItemId, Axe.ToolLevel);
        Remote->CarriedTools.Add(Axe);
        Remote->GetInventoryComponent()->SynchronizeGridFromServer();
        FString Reason;
        if (!Remote->GetInventoryComponent()->SetMainHandFromServer(Axe.InstanceGuid, Reason))
        {
            UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 DriverEquip=%s"), *Reason);
            bForestDriverPrepared = true;
            return;
        }
        Remote->GetCharacterMovement()->DisableMovement();
        Remote->SetActorLocationAndRotation(Target->GetActorLocation() + FVector(-150, 0, 88), FRotator::ZeroRotator);
        Remote->ForceNetUpdate();
        bForestDriverPrepared = true;
        UE_LOG(LogTemp, Display, TEXT("Forest driver prepared: Id=%s"), *Target->GetPersistentSpawnId());
    }
    else if (GetInventoryComponent()->GetActiveItem() == TEXT("IronAxe"))
    {
        AKalmalaHarvestNode* Target = nullptr;
        for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
            if (Node->GetPersistentSpawnId().StartsWith(TEXT("forest:v1:"))
                && FVector::DistSquared(Node->GetActorLocation(), GetActorLocation()) < FMath::Square(210.0f)
                && (!Target || FVector::DistSquared(Node->GetActorLocation(), GetActorLocation())
                    < FVector::DistSquared(Target->GetActorLocation(), GetActorLocation()))) Target = *Node;
        if (!Target || Target->IsWoodDrop()) return;
        // Optional trunk-only peer gate keeps this state stable while a third
        // observer joins. Ordinary stage/drop runs retain their existing cadence.
        if (Target->IsFelledTrunk() && FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestHoldTrunk"))) return;
        const FString Stage = Target->IsFelledTrunk() ? TEXT("Trunk") : TEXT("Standing");
        if (ForestDriverStage != Stage) { ForestDriverStage = Stage; ForestDriverStageWait = 0; }
        // Leave each state observable across the one-second peer sampling cadence.
        if (++ForestDriverStageWait < 4 || ForestDriverRequests >= 60) return;
        ++ForestDriverRequests;
        RequestAttack(); // Normal owned RPC carries sequence only; server selects and validates the target.
        if (ForestDriverRequests == 60)
            UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 DriverAttackBudgetExhausted Id=%s"), *Target->GetPersistentSpawnId());
    }
#endif
}
