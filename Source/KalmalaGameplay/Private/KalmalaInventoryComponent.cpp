#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

UKalmalaInventoryComponent::UKalmalaInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickInterval = 1.0f;
}

void UKalmalaInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaInventoryComponent, Stacks, COND_OwnerOnly);
}

int32 UKalmalaInventoryComponent::GetQuantity(FName ItemId) const
{
    const auto* Stack = Stacks.FindByPredicate([ItemId](const FKalmalaInventoryStack& Entry) { return Entry.ItemId == ItemId; });
    return Stack ? Stack->Quantity : 0;
}

bool UKalmalaInventoryComponent::TryGrantFromServer(FName ItemId, int32 Quantity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    const auto* Catalogue = GetDefault<UKalmalaItemCatalogue>();
    if (!Catalogue->CanAddToStack(ItemId, GetQuantity(ItemId), Quantity)) return false;
    auto* Stack = Stacks.FindByPredicate([ItemId](const FKalmalaInventoryStack& Entry) { return Entry.ItemId == ItemId; });
    if (!Stack)
    {
        if (Stacks.Num() >= MaxSlots) return false;
        Stack = &Stacks.AddDefaulted_GetRef();
        Stack->ItemId = ItemId;
    }
    Stack->Quantity += Quantity;
    GetOwner()->ForceNetUpdate();
    return true;
}

bool UKalmalaInventoryComponent::TryConsumeFromServer(FName ItemId, int32 Quantity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()
        || !GetDefault<UKalmalaItemCatalogue>()->IsValidStack(ItemId, Quantity)) return false;
    const int32 Index = Stacks.IndexOfByPredicate([ItemId](const FKalmalaInventoryStack& Entry) { return Entry.ItemId == ItemId; });
    if (Index == INDEX_NONE || Stacks[Index].Quantity < Quantity) return false;
    Stacks[Index].Quantity -= Quantity;
    if (Stacks[Index].Quantity == 0) Stacks.RemoveAt(Index);
    GetOwner()->ForceNetUpdate();
    return true;
}

void UKalmalaInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    SetComponentTickEnabled(FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryTest")));
#endif
}

void UKalmalaInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaTime, TickType, ThisTick);
#if !UE_BUILD_SHIPPING
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (bVerificationComplete || !Pawn || !Pawn->GetPlayerState()) return;
    if (Pawn->HasAuthority())
    {
        const bool bPassed = Stacks.IsEmpty() && TryGrantFromServer(TEXT("Wood"), 10)
            && TryConsumeFromServer(TEXT("Wood"), 3)
            && !TryGrantFromServer(TEXT("Wood"), MAX_int32)
            && !TryGrantFromServer(TEXT("Forged"), 1)
            && !TryConsumeFromServer(TEXT("Wood"), -1)
            && !TryConsumeFromServer(TEXT("Wood"), 8)
            && TryGrantFromServer(TEXT("Stone"), 1) && TryConsumeFromServer(TEXT("Stone"), 1)
            && GetQuantity(TEXT("Wood")) == 7 && Stacks.Num() == 1;
        UE_LOG(LogTemp, Display, TEXT("Inventory server: Passed=%d Wood=%d Slots=%d"), bPassed, GetQuantity(TEXT("Wood")), Stacks.Num());
    }
    else if (Pawn->IsLocallyControlled())
    {
        if (GetQuantity(TEXT("Wood")) != 7) return;
        const bool bRejected = !TryGrantFromServer(TEXT("Wood"), 1) && !TryConsumeFromServer(TEXT("Wood"), 1);
        UE_LOG(LogTemp, Display, TEXT("Inventory owner: Rejected=%d Wood=%d Slots=%d"), bRejected, GetQuantity(TEXT("Wood")), Stacks.Num());
    }
    else
    {
        // Repeat remote checks so the runner observes privacy after owner replication arrives.
        UE_LOG(LogTemp, Display, TEXT("Inventory remote: Empty=%d"), Stacks.IsEmpty());
        return;
    }
    bVerificationComplete = true;
#endif
}
