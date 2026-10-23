#include "KalmalaHarvestNode.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Misc/Crc.h"

bool AKalmalaHarvestNode::ApplyPickHitFromServer(AKalmalaCharacter* Interactor)
{
    if (!HasAuthority() || !CanReceivePickAttack() || !IsValid(Interactor) || !Interactor->HasAuthority()
        || Interactor->GetWorld() != GetWorld() || Interactor->GetHealth() <= 1.0f || PersistentSpawnId.IsEmpty()
        || FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) > FMath::Square(220.0f)) return false;
    const auto* Inventory = Interactor->GetInventoryComponent();
    const FName ItemId = Inventory ? Inventory->GetActiveItem() : NAME_None;
    const auto* Tool = FKalmalaToolLifecycleContract::FindDefinition(ItemId);
    if (!Tool || Tool->Action != EKalmalaToolAction::Mining
        || FKalmalaToolLifecycleContract::GetMaximumMineableResourceTier(Tool->Kind) == EKalmalaToolTier::None
        || static_cast<int32>(FKalmalaToolLifecycleContract::GetToolTier(Tool->Kind))
            < FKalmalaGemstoneContract::MinimumPickTier(GatheringSourceId)) return false;
    int32* Condition = Interactor->FindToolDurabilityFromServer(ItemId);
    const int32 Maximum = FKalmalaToolLifecycleContract::GetMaxDurability(ItemId, Interactor->GetCarriedToolLevel(ItemId));
    auto* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    const auto* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!Condition || *Condition <= 0 || *Condition > Maximum || !Mode || !WorldState) return false;
    // Domain-separated, server-only per-hit outcome. Failed writes/retries cannot reroll it.
    const uint64 Seed = WorldState->GetWorldGenerationConfig().WorldSeed
        ^ static_cast<uint64>(FCrc::StrCrc32(*PersistentSpawnId)) ^ (uint64(GemHits + 1) * 0x9E3779B185EBCA87ull);
    FKalmalaGemstoneState Next;
    Next.SpawnId = PersistentSpawnId; Next.SourceId = GatheringSourceId;
    if (!FKalmalaGemstoneContract::Advance(GemHits, GemOutcome,
        FKalmalaGemstoneContract::Mix(Seed) % 10, Next.Hits, Next.Outcome)) return false;
    --*Condition;
    if (!Mode->PersistPlayerStateFromServer(Interactor)) { ++*Condition; return false; }
    if (!Mode->PersistGemstoneState(Next, GemHits, GemOutcome))
    {
        ++*Condition;
        if (!Mode->PersistPlayerStateFromServer(Interactor))
            UE_LOG(LogTemp, Error, TEXT("Gemstone hit rejected; pick condition rollback could not reach disk."));
        return false;
    }
    GemHits = Next.Hits; GemOutcome = Next.Outcome;
    BuildSourcePresentation();
    if (auto* Crafting = Interactor->FindComponentByClass<UKalmalaCraftingComponent>())
        Crafting->PublishInteractionResultFromServer(GemOutcome == 1 ? TEXT("Gem shattered")
            : GemOutcome == 2 ? TEXT("Gem extracted") : FString::Printf(TEXT("Gem cracking: %d/3"), GemHits), true);
    Interactor->ForceNetUpdate(); ForceNetUpdate();
    return true;
}

bool AKalmalaHarvestNode::ClaimGemFromServer(AKalmalaCharacter* Interactor)
{
    if (!IsGemstone() || GemOutcome != 2 || !CanInteract_Implementation(Interactor) || Interactor->GetHealth() <= 1.0f) return false;
    auto* Inventory = Interactor->GetInventoryComponent();
    auto* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    if (!Inventory || !Mode) return false;
    const auto Before = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> After;
    FString Reason;
    if (!UKalmalaInventoryComponent::BuildGrant(Before, GetHarvestItemId(), 1, After, Reason)
        || !Inventory->CanFitContents(After, Inventory->CountPackedTools(Interactor->GetCarriedToolInventory())))
    {
        if (auto* Crafting = Interactor->FindComponentByClass<UKalmalaCraftingComponent>())
            Crafting->PublishInteractionResultFromServer(TEXT("Inventory is full"), false);
        return false;
    }
    if (!Inventory->TryCommitStacksFromServer(Before, After)) return false;
    FKalmalaGemstoneState Claimed;
    Claimed.SpawnId = PersistentSpawnId; Claimed.SourceId = GatheringSourceId; Claimed.Hits = 3; Claimed.Outcome = 3;
    if (!Mode->PersistGemstoneState(Claimed, GemHits, GemOutcome))
    {
        if (!Inventory->TryCommitStacksFromServer(After, Before))
            UE_LOG(LogTemp, Error, TEXT("Rejected gemstone claim inventory rollback failed."));
        return false;
    }
    GemOutcome = 3;
    bHarvested = true;
    ApplyHarvestedState();
    ForceNetUpdate();
    return true;
}

void AKalmalaHarvestNode::OnRep_Gemstone()
{
    BuildSourcePresentation();
    ApplyHarvestedState();
}
