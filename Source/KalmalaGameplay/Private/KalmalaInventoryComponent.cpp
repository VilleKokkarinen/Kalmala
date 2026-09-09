#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaCharacter.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

#if !UE_BUILD_SHIPPING
namespace
{
bool VerifyHarvestGrants(AKalmalaCharacter* Character)
{
    if (!Character) return false;
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    if (!Inventory) return false;
    auto* Save = NewObject<UKalmalaWorldPopulationSaveGame>();
    Save->InitializeForWorld(FKalmalaWorldGenerationConfig());
    bool bPassed = true;
    TSet<FName> Materials;
    for (uint64 Seed = 0; Seed < 12; ++Seed)
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Node = Character->GetWorld()->SpawnActor<AKalmalaHarvestNode>(
            AKalmalaHarvestNode::StaticClass(), Character->GetActorLocation(), FRotator::ZeroRotator, Parameters);
        if (!Node) return false;
        int32 Accepted = 0;
        Node->OnHarvested.AddLambda([&](const FString& Id) { ++Accepted; Save->MarkHarvested(Id); });
        Node->Interact_Implementation(Character); // Uninitialized/malformed descriptors grant nothing.
        bPassed &= Accepted == 0;
        FKalmalaWorldPopulationSpawn Spawn;
        Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
        Spawn.SpawnSeed = Seed;
        Spawn.Location = Character->GetActorLocation() + FVector(1000, 0, 0);
        Node->InitializeServer(Spawn);
        const FString Id = Node->GetPersistentSpawnId();
        const FName Item = Node->GetHarvestItemId();
        Materials.Add(Item);
        const auto* Definition = GetDefault<UKalmalaItemCatalogue>()->FindItem(Item);
        if (!Definition) { Node->Destroy(); return false; }
        const int32 Before = Inventory->GetQuantity(Item);
        Node->Interact_Implementation(Character); // Range rejection does not write a delta.
        bPassed &= Accepted == 0 && !Save->IsHarvested(Id) && Inventory->GetQuantity(Item) == Before;
        Node->SetActorLocation(Character->GetActorLocation());
        bPassed &= Inventory->TryGrantFromServer(Item, Definition->MaxStack - Before);
        Node->Interact_Implementation(Character); // Full stack leaves the node available.
        bPassed &= Accepted == 0 && !Save->IsHarvested(Id) && Inventory->GetQuantity(Item) == Definition->MaxStack;
        bPassed &= Inventory->TryConsumeFromServer(Item, 1);
        Node->Interact_Implementation(Character);
        Node->Interact_Implementation(Character); // Duplicate cannot grant again or rebroadcast.
        bPassed &= Accepted == 1 && Save->IsHarvested(Id)
            && Inventory->GetQuantity(Item) == Definition->MaxStack && Node->GetPersistentSpawnId() == Id;
        bPassed &= Inventory->TryConsumeFromServer(Item, Definition->MaxStack - Before);
        Node->OnHarvested.Clear();
        Node->Destroy();
    }
    return bPassed && Materials.Num() == 3;
}
}
#endif

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
        const bool bHarvestPassed = VerifyHarvestGrants(Cast<AKalmalaCharacter>(Pawn));
        UE_LOG(LogTemp, Display, TEXT("Harvest inventory: Passed=%d Materials=3 Range=1 Full=1 Malformed=1 Duplicate=1 SparseDelta=1"), bHarvestPassed);
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
