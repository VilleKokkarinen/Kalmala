#include "KalmalaCraftingComponent.h"

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"

void UKalmalaCraftingComponent::ServerPlantGardenCarrot_Implementation(
    AKalmalaConstructionActor* Plot, const EKalmalaGardenCropMode Mode)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = PlantGardenCropFromServer(Plot, Mode, Reason);
    PublishResult(Reason, bAccepted);
}

void UKalmalaCraftingComponent::ServerHarvestGardenCrop_Implementation(AKalmalaConstructionActor* Plot)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = HarvestGardenCropFromServer(Plot, Reason);
    PublishResult(Reason, bAccepted);
}

bool UKalmalaCraftingComponent::PlantGardenCropFromServer(AKalmalaConstructionActor* Plot,
    const EKalmalaGardenCropMode Mode, FString& Reason)
{
    AKalmalaCharacter* Character = GetCharacter();
    if (!Character || !Character->HasAuthority() || !IsValid(Plot) || !Plot->HasAuthority()
        || Plot->GetWorld() != GetWorld() || Plot->GetConstructionKit() != TEXT("GardenPlotKit")
        || !Plot->CanInteract_Implementation(Character)
        || !IsStationContextTargetCurrent(Plot, TEXT("GardenPlotKit"), Plot->GetConstructionId()))
    {
        Reason = TEXT("Interact with a visible Garden Plot while holding a usable Planting Trowel");
        return false;
    }
    const FName PaymentItem = FKalmalaGardenPlotState::GetPlantingPaymentItem(Mode);
    if (PaymentItem.IsNone())
    {
        Reason = TEXT("That crop mode is not available yet");
        return false;
    }
    if (!Plot->GetGardenPlotState().IsEmpty())
    {
        Reason = TEXT("This Garden Plot already has a growing crop");
        return false;
    }

    UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
    if (!Inventory)
    {
        Reason = TEXT("Inventory unavailable");
        return false;
    }

    const TArray<FKalmalaInventoryStack> BeforeStacks = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> AfterStacks;
    if (!UKalmalaInventoryComponent::BuildExchange(BeforeStacks, {{PaymentItem, 1}}, NAME_None, 0,
        AfterStacks, Reason)) return false;

    FKalmalaGardenPlotState CandidateState = Plot->GetGardenPlotState();
    if (!CandidateState.PlantCrop(Mode))
    {
        Reason = TEXT("Garden Plot is unavailable");
        return false;
    }

    const FGuid TrowelGuid = Inventory->GetActiveInstanceGuid();
    int32* TrowelCondition = Character->FindToolDurabilityFromServer(TrowelGuid);
    const int32 MaximumCondition = FKalmalaToolLifecycleContract::GetMaxDurability(
        TEXT("PlantingTrowel"), Character->GetCarriedToolLevel(TrowelGuid));
    if (!Plot->HasUsablePlantingTrowel(Character) || !TrowelCondition
        || MaximumCondition != 100 || *TrowelCondition <= 0 || *TrowelCondition > MaximumCondition)
    {
        Reason = TEXT("A usable Planting Trowel must be in your Main Hand");
        return false;
    }

    AKalmalaGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (!GameMode)
    {
        Reason = TEXT("Garden Plot service unavailable");
        return false;
    }
    const FKalmalaGardenPlotState PreviousState = Plot->GetGardenPlotState();
    const int32 PreviousCondition = *TrowelCondition;
    if (!Inventory->TryCommitStacksFromServer(BeforeStacks, AfterStacks))
    {
        Reason = TEXT("Inventory changed; planting was rejected");
        return false;
    }
    --*TrowelCondition;
    if (!GameMode->PersistPlayerStateFromServer(Character))
    {
        *TrowelCondition = PreviousCondition;
        if (!Inventory->TryCommitStacksFromServer(AfterStacks, BeforeStacks))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore planting materials after Planting Trowel save rejection."));
        Reason = TEXT("Planting Trowel condition could not be saved");
        return false;
    }
    if (!GameMode->PersistGardenPlotState(Plot, CandidateState))
    {
        *TrowelCondition = PreviousCondition;
        if (!Inventory->TryCommitStacksFromServer(AfterStacks, BeforeStacks))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore planting materials after Garden Plot save rejection."));
        if (!GameMode->PersistPlayerStateFromServer(Character))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore Planting Trowel condition after Garden Plot save rejection."));
        Reason = TEXT("Garden Plot crop could not be saved; planting was rolled back");
        return false;
    }
    if (!Plot->ApplyGardenPlotStateFromServer(CandidateState))
    {
        if (!Inventory->TryCommitStacksFromServer(AfterStacks, BeforeStacks))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore planting materials after Garden Plot actor rejection."));
        if (!GameMode->PersistGardenPlotState(Plot, PreviousState))
            UE_LOG(LogTemp, Error, TEXT("Failed to roll back Garden Plot save after actor rejection."));
        *TrowelCondition = PreviousCondition;
        if (!GameMode->PersistPlayerStateFromServer(Character))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore Planting Trowel condition after actor rejection."));
        Reason = TEXT("Garden Plot changed; planting was rolled back");
        return false;
    }

    Character->ForceNetUpdate();
    if (Mode == EKalmalaGardenCropMode::PotatoFood)
        Reason = TEXT("Potato tuber planted · harvest 3 Potatoes after 600 active seconds");
    else if (Mode == EKalmalaGardenCropMode::CarrotFood)
        Reason = TEXT("Carrot planted · harvest 3 Carrots after 600 active seconds");
    else if (Mode == EKalmalaGardenCropMode::RutabagaFood)
        Reason = TEXT("Rutabaga planted · harvest 3 Rutabagas after 600 active seconds");
    else if (Mode == EKalmalaGardenCropMode::RutabagaSeed)
        Reason = TEXT("Rutabaga seed planted · harvest 3 Rutabaga Seeds after 600 active seconds");
    else if (Mode == EKalmalaGardenCropMode::OnionFood)
        Reason = TEXT("Onion planted · harvest 3 Onions after 600 active seconds");
    else if (Mode == EKalmalaGardenCropMode::OnionSeed)
        Reason = TEXT("Onion seed planted · harvest 3 Onion Seeds after 600 active seconds");
    else
        Reason = TEXT("Carrot seed planted · harvest 3 Carrot Seeds after 600 active seconds");
    return true;
}

bool UKalmalaCraftingComponent::HarvestGardenCropFromServer(AKalmalaConstructionActor* Plot, FString& Reason)
{
    AKalmalaCharacter* Character = GetCharacter();
    if (!Character || !Character->HasAuthority() || !IsValid(Plot) || !Plot->HasAuthority()
        || Plot->GetWorld() != GetWorld() || Plot->GetConstructionKit() != TEXT("GardenPlotKit")
        || !Plot->CanInteract_Implementation(Character)
        || !IsStationContextTargetCurrent(Plot, TEXT("GardenPlotKit"), Plot->GetConstructionId()))
    {
        Reason = TEXT("Interact with a visible Garden Plot while holding a usable Planting Trowel");
        return false;
    }
    const FKalmalaGardenPlotState PreviousState = Plot->GetGardenPlotState();
    if (!PreviousState.IsMature())
    {
        Reason = TEXT("This crop is not ready to harvest");
        return false;
    }

    const FName HarvestItem = FKalmalaGardenPlotState::GetHarvestItem(PreviousState.CropMode);
    if (HarvestItem.IsNone())
    {
        Reason = TEXT("This crop mode cannot be harvested");
        return false;
    }
    UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
    AKalmalaGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (!Inventory || !GameMode)
    {
        Reason = TEXT("Garden Plot service unavailable");
        return false;
    }

    const TArray<FKalmalaInventoryStack> BeforeStacks = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> CandidateStacks;
    if (!UKalmalaInventoryComponent::BuildGrant(BeforeStacks, HarvestItem, FKalmalaGardenPlotState::HarvestYield,
        CandidateStacks, Reason)) return false;
    FKalmalaGardenPlotState EmptyState = PreviousState;
    if (!EmptyState.ClearMatureCrop())
    {
        Reason = TEXT("Garden Plot crop state is invalid");
        return false;
    }
    if (!GameMode->PersistGardenPlotState(Plot, EmptyState))
    {
        Reason = TEXT("Harvest state could not be saved");
        return false;
    }
    if (!Inventory->TryCommitStacksFromServer(BeforeStacks, CandidateStacks))
    {
        if (!GameMode->PersistGardenPlotState(Plot, PreviousState))
            UE_LOG(LogTemp, Error, TEXT("Garden Plot harvest-state rollback failed after inventory changed."));
        Reason = TEXT("Inventory changed; harvest was rejected");
        return false;
    }
    if (!Plot->ApplyGardenPlotStateFromServer(EmptyState))
    {
        if (!Inventory->TryCommitStacksFromServer(CandidateStacks, BeforeStacks))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore harvested produce after Garden Plot actor rejection."));
        if (!GameMode->PersistGardenPlotState(Plot, PreviousState))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore mature Garden Plot state after actor rejection."));
        Reason = TEXT("Garden Plot changed; harvest was rolled back");
        return false;
    }
    Character->ForceNetUpdate();
    Reason = FString::Printf(TEXT("Harvested 3 %s"), *HarvestItem.ToString());
    return true;
}
