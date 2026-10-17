#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "KalmalaForestLayout.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void AKalmalaCharacter::VerifyForestDelivery(const float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestDeliveryTest")) || !IsLocallyControlled()) return;
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup")))
    {
        // Use an eye-position fixture camera so the normal 250 cm interaction
        // trace can reach the drop; the default boom alone is 320 cm long.
        CameraBoom->TargetArmLength = 0;
        CameraBoom->TargetOffset = GetPawnViewLocation() - GetActorLocation();
    }
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || State->PlayerArray.Num() < 2) return;
    ForestDeliveryElapsed += DeltaSeconds;
    if (ForestDeliveryElapsed < 1) return;
    ForestDeliveryElapsed = 0;
    const auto& Config = State->GetWorldGenerationConfig();
    for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
    {
        const FString Id = Node->GetPersistentSpawnId();
        if (!Id.StartsWith(TEXT("forest:v1:")) || !Node->IsHarvested()) continue;
        const FString Key = Id + TEXT(":Collected");
        if (ForestDeliveryReported.Contains(Key)) continue;
        ForestDeliveryReported.Add(Key);
        UE_LOG(LogTemp, Display, TEXT("Forest depletion delivery: Passed=1 Authority=%d Seed=%llu Id=%s Collected=1"),
            HasAuthority(), Config.WorldSeed, *Id);
    }
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
        if (!Report.EndsWith(TEXT(":Trunk")) && !Report.EndsWith(TEXT(":Drop")) && !Report.EndsWith(TEXT(":Collected"))) ++StandingPatches;
    if (StandingPatches < 9) return;
    if (HasAuthority())
    {
        ForestDriverReadySeconds += 1;
        if (ForestDriverReadySeconds < 15) return;
        AKalmalaCharacter* Remote = nullptr;
        for (TActorIterator<AKalmalaCharacter> Pawn(GetWorld()); Pawn; ++Pawn)
            if (Pawn->GetController() && !Pawn->IsLocallyControlled()) { Remote = *Pawn; break; }
        if (!Remote) return;
        if (bForestDriverPrepared)
        {
            if (ForestPickupId.IsEmpty()) return;
            AKalmalaHarvestNode* Pickup = nullptr;
            for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
                if (Node->GetPersistentSpawnId() == ForestPickupId) { Pickup = *Node; break; }
            const int32 Delta = Remote->GetInventoryComponent()->GetQuantity(ForestPickupItem) - ForestPickupBefore;
            if (Delta > ForestPickupExpected)
            {
                bForestPickupReported = true;
                UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 ServerPickupExcess Id=%s Delta=%d Expected=%d"),
                    *ForestPickupId, Delta, ForestPickupExpected);
                return;
            }
            // Probe a live drop before collection, so depletion cannot mask a
            // missing trace-range guard. No fixture reward or state is injected.
            if (Pickup && Pickup->IsWoodDrop() && !Pickup->IsHarvested() && !bForestPickupRangeChecked)
            {
                const bool bAccepted = Pickup->InteractWithToolIntentFromServer(Remote,
                    InteractionRange + 1.0f, InteractionRange, NAME_None, 0);
                const int32 After = Remote->GetInventoryComponent()->GetQuantity(ForestPickupItem) - ForestPickupBefore;
                if (bAccepted || Pickup->IsHarvested() || !Pickup->IsWoodDrop() || After != 0)
                {
                    UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 OutOfRangePickup Id=%s Delta=%d"), *ForestPickupId, After);
                    return;
                }
                bForestPickupRangeChecked = true;
                UE_LOG(LogTemp, Display, TEXT("Forest range rejection: Passed=1 Authority=1 Seed=%llu Id=%s Item=%s Delta=0 LiveDrop=1"),
                    Config.WorldSeed, *ForestPickupId, *ForestPickupItem.ToString());
            }
            // A second authoritative player cannot collect the client's depleted
            // exact-ID drop. Keep the real transaction/range checks in the path.
            if (Pickup && Pickup->IsHarvested() && Delta == ForestPickupExpected && ForestPickupHostRequests < 3)
            {
                SetActorLocation(Pickup->GetActorLocation() + FVector(150, 0, 88));
                const bool bAccepted = Pickup->InteractWithToolIntentFromServer(this, 150.0f, InteractionRange, NAME_None, 0);
                ++ForestPickupHostRequests;
                if (bAccepted)
                {
                    UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 HostDepletedPickupAccepted Id=%s"), *ForestPickupId);
                    return;
                }
                ForestPickupStableTicks = 0;
                return;
            }
            const int32 HostDelta = GetInventoryComponent()->GetQuantity(ForestPickupItem) - ForestPickupHostBefore;
            if (HostDelta != 0)
            {
                UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 HostDepletedPickupReward Id=%s Delta=%d"), *ForestPickupId, HostDelta);
                return;
            }
            if (Pickup && Pickup->IsHarvested() && Delta == ForestPickupExpected && ForestPickupHostRequests == 3) ++ForestPickupStableTicks;
            else ForestPickupStableTicks = 0;
            if (ForestPickupStableTicks >= 5 && bForestPickupRangeChecked && !bForestPickupReported)
            {
                bForestPickupReported = true;
                UE_LOG(LogTemp, Display, TEXT("Forest second collector: Passed=1 Authority=1 Seed=%llu Id=%s Item=%s Delta=0 Requests=3 Stable=5"),
                    Config.WorldSeed, *ForestPickupId, *ForestPickupItem.ToString());
                UE_LOG(LogTemp, Display, TEXT("Forest pickup authority: Passed=1 Authority=1 Seed=%llu Id=%s Item=%s Delta=%d Expected=%d Stable=5"),
                    Config.WorldSeed, *ForestPickupId, *ForestPickupItem.ToString(), Delta, ForestPickupExpected);
            }
            return;
        }
        AKalmalaHarvestNode* Target = nullptr;
        for (TActorIterator<AKalmalaHarvestNode> Node(GetWorld()); Node; ++Node)
            if (Node->GetPersistentSpawnId().StartsWith(TEXT("forest:v1:")) && Node->CanReceiveAxeAttack()
                && (!Target || FVector::DistSquared(Node->GetActorLocation(), Remote->GetActorLocation())
                    < FVector::DistSquared(Target->GetActorLocation(), Remote->GetActorLocation()))) Target = *Node;
        if (!Target) return;
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup")))
        {
            FKalmalaToolServerSelection Selection;
            if (!FKalmalaToolLifecycleContract::BuildServerSelection(Target->GetGatheringSourceId(), Selection)) return;
            ForestPickupExpected = Selection.RewardQuantity;
            if (FKalmalaM9SourceLootContract::IsM9Source(Target->GetGatheringSourceId())
                && !FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(Config, Target->GetGatheringSourceId(),
                    Target->GetPersistentSpawnId(), ForestPickupExpected)) return;
            ForestPickupId = Target->GetPersistentSpawnId();
            ForestPickupItem = Selection.RewardItemId;
            ForestPickupBefore = Remote->GetInventoryComponent()->GetQuantity(ForestPickupItem);
            ForestPickupHostBefore = GetInventoryComponent()->GetQuantity(ForestPickupItem);
        }
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
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup")))
        {
            // Keep normal movement/aim updates active while holding position.
            Remote->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Remote->GetCharacterMovement()->StopMovementImmediately();
            Remote->CameraBoom->TargetArmLength = 0;
            Remote->CameraBoom->TargetOffset = Remote->GetPawnViewLocation() - Remote->GetActorLocation();
        }
        else Remote->GetCharacterMovement()->DisableMovement();
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
        if (!Target) return;
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup")) && (Target->IsWoodDrop() || !ForestPickupId.IsEmpty()))
        {
            if (bForestPickupReported) return;
            if (ForestPickupId.IsEmpty())
            {
                // Preserve the drop long enough for both observers to sample it.
                if (ForestDriverStage != TEXT("Drop")) { ForestDriverStage = TEXT("Drop"); ForestDriverStageWait = 0; }
                if (++ForestDriverStageWait < 5) return;
                FKalmalaToolServerSelection Selection;
                if (!FKalmalaToolLifecycleContract::BuildServerSelection(Target->GetGatheringSourceId(), Selection)) return;
                ForestPickupExpected = Selection.RewardQuantity;
                if (FKalmalaM9SourceLootContract::IsM9Source(Target->GetGatheringSourceId())
                    && !FKalmalaM9SourceLootContract::BuildHarvestRewardQuantity(Config, Target->GetGatheringSourceId(),
                        Target->GetPersistentSpawnId(), ForestPickupExpected)) return;
                ForestPickupId = Target->GetPersistentSpawnId();
                ForestPickupItem = Selection.RewardItemId;
                ForestPickupBefore = GetInventoryComponent()->GetQuantity(ForestPickupItem);
                // Both fixture camera origins now match the pawn eye. Normal
                // camera/movement updates carry this aim to the server cache.
                Controller->SetControlRotation((Target->GetActorLocation() + FVector(0, 0, 10) - GetPawnViewLocation()).Rotation());
                return; // Allow normal control-rotation replication before interacting.
            }
            if (Target->GetPersistentSpawnId() != ForestPickupId) return;
            ++ForestPickupTicks;
            // Exercise repeated ordinary owned requests, including after depletion.
            // No target, quantity or reward is supplied to the server.
            if (ForestPickupTicks <= 3) RequestInteract();
            // Explicitly retry after replicated depletion, rather than assuming
            // the initial three requests arrived after the successful pickup.
            if (ForestPickupTicks > 3 && Target->IsHarvested() && ForestPickupDepletedRequests < 3)
            {
                RequestInteract();
                ++ForestPickupDepletedRequests;
                ForestPickupStableTicks = 0;
                return; // Observe reward stability only on subsequent ticks.
            }
            const int32 Delta = GetInventoryComponent()->GetQuantity(ForestPickupItem) - ForestPickupBefore;
            if (ForestPickupTicks > 3 && ForestPickupDepletedRequests == 3 && Target->IsHarvested() && Delta == ForestPickupExpected)
                ++ForestPickupStableTicks;
            else ForestPickupStableTicks = 0;
            if (ForestPickupStableTicks >= 5)
            {
                bForestPickupReported = true;
                UE_LOG(LogTemp, Display, TEXT("Forest pickup delivery: Passed=1 Authority=0 Seed=%llu Id=%s Item=%s Delta=%d Expected=%d Requests=3 DepletedRequests=3 Stable=5"),
                    Config.WorldSeed, *ForestPickupId, *ForestPickupItem.ToString(), Delta, ForestPickupExpected);
            }
            else if (ForestPickupTicks >= 30)
            {
                bForestPickupReported = true;
                UE_LOG(LogTemp, Error, TEXT("Forest delivery: Passed=0 PickupTimeout Id=%s Delta=%d Expected=%d Collected=%d"),
                    *ForestPickupId, Delta, ForestPickupExpected, Target->IsHarvested());
            }
            return;
        }
        if (Target->IsWoodDrop()) return;
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
