#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AKalmalaConstructionActor* UKalmalaCraftingComponent::FindNearbyConstruction(FName Kit) const
{
    const auto* Character = GetCharacter();
    if (!Character || !GetWorld()) return nullptr;
    AKalmalaConstructionActor* Closest = nullptr;
    double Best = FMath::Square(250.0);
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
    {
        if (!IsValid(*It) || It->GetConstructionKit() != Kit || !It->CanUse(Character)) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation());
        if (Distance < Best || (Distance == Best && (!Closest || It->GetConstructionId() < Closest->GetConstructionId())))
        { Best = Distance; Closest = *It; }
    }
    return Closest;
}

AKalmalaConstructionActor* UKalmalaCraftingComponent::FindNearbyWorkbench() const { return FindNearbyConstruction(TEXT("WorkbenchKit")); }

FString UKalmalaCraftingComponent::GetNearbyWorkbenchText() const
{
    return FindNearbyWorkbench() ? TEXT("Joiner's bench: ready for floor, wall and roof assembly") : TEXT("No visible workbench within 2.5 m");
}

void UKalmalaCraftingComponent::ClearStorageView()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    ActiveStorage.Reset(); StorageView.Reset(); bStorageViewOpen = false; GetOwner()->ForceNetUpdate();
}

void UKalmalaCraftingComponent::RefreshStorageView()
{
    auto* Character = GetCharacter();
    if (!Character || !Character->HasAuthority()) return;
    auto* Chest = ActiveStorage.Get();
    auto* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    TArray<FKalmalaInventoryStack> Contents;
    if (!Chest || !Chest->CanInteract_Implementation(Character) || !Mode || !Mode->ReadStorage(Chest, Contents))
    { ClearStorageView(); return; }
    StorageView = MoveTemp(Contents); bStorageViewOpen = true;
}

bool UKalmalaCraftingComponent::OpenStorageFromServer(AKalmalaConstructionActor* Construction)
{
    auto* Character = GetCharacter();
    if (!Character || !Character->HasAuthority()) return false;
    ClearStorageView();
    if (!IsValid(Construction) || Construction->GetConstructionKit() != TEXT("StorageKit")
        || !Construction->CanInteract_Implementation(Character)) return false;
    ActiveStorage = Construction; RefreshStorageView(); Character->ForceNetUpdate();
    return bStorageViewOpen;
}

void UKalmalaCraftingComponent::InteractWithConstructionFromServer(AKalmalaConstructionActor* Construction)
{
    if (!AcceptRequest() || !IsValid(Construction) || !Construction->CanInteract_Implementation(GetCharacter())) return;
    if (Construction->GetConstructionKit() == TEXT("StorageKit"))
        PublishResult(OpenStorageFromServer(Construction) ? TEXT("Chest inspected; use Camp crafting to transfer items") : TEXT("Storage unavailable"));
    else PublishResult(TEXT("Joiner's bench ready; use Camp crafting to assemble floor, wall and roof kits"));
}

bool UKalmalaCraftingComponent::TransferStorageFromServer(FName ItemId, bool bDeposit, FString& Reason)
{
    Reason = TEXT("Inspect a visible chest within 2.5 m first");
    auto* Character = GetCharacter();
    if (!Character || !Character->HasAuthority()) return false;
    RefreshStorageView();
    auto* Chest = ActiveStorage.Get();
    auto* Mode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    auto* Pack = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    if (!bStorageViewOpen || !Chest || !Mode || !Pack) return false;
    // Read fresh server contents on every action; never use a client's snapshot or quantity.
    const bool Accepted = Pack->TransferStorageFromServer(StorageView, ItemId, bDeposit,
        [&](const auto& Next) { return Mode->PersistStorage(Chest, Next); }, Reason);
    RefreshStorageView(); Character->ForceNetUpdate();
    return Accepted;
}

void UKalmalaCraftingComponent::ServerOpenStorage_Implementation()
{
    if (!AcceptRequest()) return;
    PublishResult(OpenStorageFromServer(FindNearbyConstruction(TEXT("StorageKit")))
        ? TEXT("Nearby chest inspected") : TEXT("Need a saved, visible chest within 2.5 m"));
}
void UKalmalaCraftingComponent::ServerCloseStorage_Implementation() { ClearStorageView(); }
void UKalmalaCraftingComponent::ServerDepositStorage_Implementation(FName ItemId)
{
    if (!AcceptRequest()) return;
    FString Reason; TransferStorageFromServer(ItemId, true, Reason); PublishResult(Reason);
}
void UKalmalaCraftingComponent::ServerWithdrawStorage_Implementation(FName ItemId)
{
    if (!AcceptRequest()) return;
    FString Reason; TransferStorageFromServer(ItemId, false, Reason); PublishResult(Reason);
}
