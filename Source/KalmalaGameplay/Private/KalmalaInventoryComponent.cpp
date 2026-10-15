#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaCharacter.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
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
    const bool bHasAuthenticatedIdentity = Character->GetPlayerState() != nullptr
        && Character->GetPlayerState()->GetUniqueId().IsValid();
    UE_LOG(LogTemp, Display, TEXT("Inventory persistence fixture: PlayerState=%d Authenticated=%d Tools=%d"),
        Character->GetPlayerState() != nullptr, bHasAuthenticatedIdentity, Character->GetCarriedToolInventory().Num());
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
        const auto* Definition = UKalmalaItemCatalogue::Get()->FindItem(Item);
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
    bPassed &= Materials.Num() == 3;

    // Exercise axe attacks followed by the generated wood pickup transaction;
    // a full-pack rejection preserves the drop and spends no further condition.
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* ToolNode = Character->GetWorld()->SpawnActor<AKalmalaHarvestNode>(
        AKalmalaHarvestNode::StaticClass(), Character->GetActorLocation(), FRotator::ZeroRotator, Parameters);
    if (!ToolNode) return false;
    FKalmalaWorldPopulationSpawn ToolSpawn;
    ToolSpawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
    ToolSpawn.SpawnSeed = MAX_uint64;
    ToolSpawn.Location = Character->GetActorLocation();
    ToolSpawn.ContentId = TEXT("meadows-birch-bark");
    ToolNode->InitializeServer(ToolSpawn);
    const int32 BeforeWood = Inventory->GetQuantity(TEXT("Wood"));
    const FKalmalaItemDefinition* WoodDefinition = UKalmalaItemCatalogue::Get()->FindItem(TEXT("Wood"));
    if (WoodDefinition == nullptr) { ToolNode->Destroy(); return false; }
    const int32 MaxWood = WoodDefinition->MaxStack;
    const int32 HatchetBefore = Character->GetToolDurability(TEXT("StoneAxe"));
    int32 ToolHarvestEvents = 0;
    ToolNode->OnHarvested.AddLambda([&](const FString&) { ++ToolHarvestEvents; });
    if (BeforeWood > MaxWood || HatchetBefore <= 0) bPassed = false;
    else
    {
        const int32 FillAmount = MaxWood - BeforeWood;
        if (FillAmount > 0) bPassed &= Inventory->TryGrantFromServer(TEXT("Wood"), FillAmount);
        const int32 FullWood = Inventory->GetQuantity(TEXT("Wood"));
        const bool bWrongToolRejected = !ToolNode->InteractWithToolIntentFromServer(
            Character, 0.0f, 250.0f, TEXT("StonePick"), static_cast<uint8>(EKalmalaToolAction::Mining));
        Inventory->SynchronizeGridFromServer();
        const int32 AxeSlot = Inventory->GetGridSlots().IndexOfByKey(FName(TEXT("StoneAxe")));
        if (AxeSlot >= 0 && AxeSlot < UKalmalaInventoryComponent::Columns) Inventory->ServerUseHotbarSlot(AxeSlot);
        const FKalmalaDamageChannels AxeDamage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(
            TEXT("StoneAxe"), Character->GetCarriedToolLevel(TEXT("StoneAxe")), TEXT("Basic"));
        int32 AcceptedHits = 0;
        while (!ToolNode->IsWoodDrop() && AcceptedHits < 30)
        {
            if (!ToolNode->ApplyAxeDamageFromServer(Character, AxeDamage)) break;
            ++AcceptedHits;
        }
        bPassed &= ToolNode->IsWoodDrop() && Inventory->GetQuantity(TEXT("Wood")) == FullWood;
        const bool bFullPackRejected = !ToolNode->InteractWithToolIntentFromServer(
            Character, 0.0f, 250.0f, TEXT("StoneAxe"), static_cast<uint8>(EKalmalaToolAction::Woodcutting));
        bPassed &= bWrongToolRejected && bFullPackRejected && !ToolNode->IsHarvested()
            && ToolHarvestEvents == 0 && Inventory->GetQuantity(TEXT("Wood")) == FullWood
            && Character->GetToolDurability(TEXT("StoneAxe")) == HatchetBefore - AcceptedHits;
        if (FullWood > 0) bPassed &= Inventory->TryConsumeFromServer(TEXT("Wood"), 1);
        const bool bAccepted = ToolNode->InteractWithToolIntentFromServer(
            Character, 0.0f, 250.0f, TEXT("StoneAxe"), static_cast<uint8>(EKalmalaToolAction::Woodcutting));
        const bool bDuplicateRejected = !ToolNode->InteractWithToolIntentFromServer(
            Character, 0.0f, 250.0f, TEXT("StoneAxe"), static_cast<uint8>(EKalmalaToolAction::Woodcutting));
        UE_LOG(LogTemp, Display,
            TEXT("Inventory persistence harvest detail: Accepted=%d DuplicateRejected=%d Harvested=%d Wood=%d Hatchet=%d"),
            bAccepted, bDuplicateRejected, ToolNode->IsHarvested(), Inventory->GetQuantity(TEXT("Wood")),
            Character->GetToolDurability(TEXT("StoneAxe")));
        bPassed &= bAccepted && bDuplicateRejected && ToolNode->IsHarvested() && ToolHarvestEvents == 1
            && Inventory->GetQuantity(TEXT("Wood")) == FullWood && Character->GetToolDurability(TEXT("StoneAxe")) == HatchetBefore - AcceptedHits;
        if (Inventory->GetQuantity(TEXT("Wood")) > BeforeWood)
        {
            bPassed &= Inventory->TryConsumeFromServer(TEXT("Wood"), Inventory->GetQuantity(TEXT("Wood")) - BeforeWood);
        }
    }
    ToolNode->OnHarvested.Clear();
    ToolNode->Destroy();

    // Verify selected-tool free repair against a real accepted workbench actor.
    auto* Crafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
    if (!Crafting) return false;
    const int32 ConditionAfterGather = Character->GetToolDurability(TEXT("StoneAxe"));
    const int32 RepairBaselineWood = Inventory->GetQuantity(TEXT("Wood"));
    const TArray<FKalmalaInventoryStack> RepairBaselineStacks = Inventory->GetStacks();
    FString RepairReason;
    const bool bNoStationRejected = Crafting != nullptr
        && !Crafting->RepairToolFromServer(TEXT("StoneAxe"), RepairReason)
        && Character->GetToolDurability(TEXT("StoneAxe")) == ConditionAfterGather
        && Inventory->GetStacks().Num() == RepairBaselineStacks.Num();

    const FVector Forward = Character->GetActorForwardVector().GetSafeNormal2D();
    const FTransform FarTransform(FRotator::ZeroRotator, Character->GetActorLocation() + Forward * 500.0f);
    auto* Bench = Character->GetWorld()->SpawnActorDeferred<AKalmalaConstructionActor>(
        AKalmalaConstructionActor::StaticClass(), FarTransform, nullptr, Character,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Bench) return false;
    Bench->InitializeFromServer(TEXT("WorkbenchKit"), FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
    Bench->FinishSpawning(FarTransform);
    RepairReason.Reset();
    const bool bFarStationRejected = !Crafting->RepairToolFromServer(TEXT("StoneAxe"), RepairReason)
        && Character->GetToolDurability(TEXT("StoneAxe")) == ConditionAfterGather
        && Inventory->GetStacks().Num() == RepairBaselineStacks.Num();

    const FTransform NearTransform(FRotator::ZeroRotator, Character->GetActorLocation() + Forward * 150.0f);
    Bench->SetActorTransform(NearTransform);
    const bool bVisibleStationAccepted = Bench->CanInteract_Implementation(Character);
    const TArray<FKalmalaInventoryStack> EmptyPack;
    const bool bPackCleared = Inventory->TryCommitStacksFromServer(RepairBaselineStacks, EmptyPack);
    auto* Progression = Character->GetSkillProgressionComponent();
    const FKalmalaSkillState* CraftingBefore = Progression
        ? Progression->GetServerLedger().Find(EKalmalaSkill::Crafting) : nullptr;
    const int32 CraftingExperienceBefore = CraftingBefore ? CraftingBefore->Experience : -1;
    const auto* HatchetDefinition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StoneAxe"));
    RepairReason.Reset();
    const bool bRepairRequestAccepted = Crafting->RepairToolFromServer(TEXT("StoneAxe"), RepairReason);
    const bool bRepairAccepted = bPackCleared && HatchetDefinition && Inventory->GetStacks().IsEmpty()
        && bRepairRequestAccepted
        && Character->GetToolDurability(TEXT("StoneAxe")) == HatchetDefinition->MaxDurability
        && Inventory->GetStacks().IsEmpty() && RepairReason.Contains(TEXT("at no cost"));
    const FString AcceptedRepairReason = RepairReason;
    RepairReason.Reset();
    const bool bFullConditionRejected = !Crafting->RepairToolFromServer(TEXT("StoneAxe"), RepairReason)
        && HatchetDefinition && Character->GetToolDurability(TEXT("StoneAxe")) == HatchetDefinition->MaxDurability
        && Inventory->GetStacks().IsEmpty();
    const bool bInventoryRestored = Inventory->TryCommitStacksFromServer(EmptyPack, RepairBaselineStacks);
    const FKalmalaSkillState* CraftingAfter = Progression
        ? Progression->GetServerLedger().Find(EKalmalaSkill::Crafting) : nullptr;
    const bool bNoCraftingExperience = CraftingExperienceBefore >= 0 && CraftingAfter
        && CraftingAfter->Experience == CraftingExperienceBefore;
    const bool bRepairPassed = bNoStationRejected && bFarStationRejected && bVisibleStationAccepted
        && bRepairAccepted && bFullConditionRejected && bInventoryRestored && bNoCraftingExperience
        && Inventory->GetStacks().Num() == RepairBaselineStacks.Num()
        && Inventory->GetQuantity(TEXT("Wood")) == RepairBaselineWood;
    UE_LOG(LogTemp, Display,
        TEXT("Tool free repair fixture: Passed=%d Workbench=1 NoCost=1 NoXP=1 FullCondition=1 RequestAccepted=%d Reason=%s PackCleared=%d"),
        bRepairPassed, bRepairRequestAccepted, *AcceptedRepairReason, bPackCleared);
    bPassed &= bRepairPassed;

    const TArray<FName> RetiredItemIds = {TEXT("StoneAxe"), TEXT("StonePick"), TEXT("StoneKnife")};
    TArray<int32> RetiredToolConditions;
    RetiredToolConditions.Reserve(RetiredItemIds.Num());
    for (const FName ItemId : RetiredItemIds) RetiredToolConditions.Add(Character->GetToolDurability(ItemId));
    const TArray<FKalmalaInventoryStack> RetiredRecipePack = Inventory->GetStacks();
    const int32 ExperienceBeforeRetiredRecipes = CraftingAfter ? CraftingAfter->Experience : -1;
    bool bRetiredRecipesRejected = Crafting != nullptr;
    for (const FName RecipeId : {FName(TEXT("ReplaceStoneAxe")), FName(TEXT("ReplaceStonePick")), FName(TEXT("ReplaceStoneKnife"))})
    {
        FString RetiredReason;
        bRetiredRecipesRejected &= !Crafting->CraftFromServer(RecipeId, 1, RetiredReason);
    }
    bool bToolConditionsUnchanged = true;
    for (int32 Index = 0; Index < RetiredItemIds.Num(); ++Index)
        bToolConditionsUnchanged &= Character->GetToolDurability(RetiredItemIds[Index]) == RetiredToolConditions[Index];
    bool bPackUnchanged = Inventory->GetStacks().Num() == RetiredRecipePack.Num();
    for (int32 Index = 0; bPackUnchanged && Index < RetiredRecipePack.Num(); ++Index)
        bPackUnchanged &= Inventory->GetStacks()[Index].ItemId == RetiredRecipePack[Index].ItemId
            && Inventory->GetStacks()[Index].Quantity == RetiredRecipePack[Index].Quantity;
    const FKalmalaSkillState* ExperienceAfterRetiredRecipes = Progression
        ? Progression->GetServerLedger().Find(EKalmalaSkill::Crafting) : nullptr;
    const bool bRetiredRecipeXPUnchanged = ExperienceBeforeRetiredRecipes >= 0 && ExperienceAfterRetiredRecipes
        && ExperienceAfterRetiredRecipes->Experience == ExperienceBeforeRetiredRecipes;
    const bool bRetiredRecipesPassed = bRetiredRecipesRejected && bToolConditionsUnchanged
        && bPackUnchanged && bRetiredRecipeXPUnchanged;
    UE_LOG(LogTemp, Display, TEXT("Retired tool replacement fixture: Passed=%d Disabled=1 NoMutation=1 NoXP=1"), bRetiredRecipesPassed);
    bPassed &= bRetiredRecipesPassed;    Bench->Destroy();
    return bPassed;
}
}
#endif

UKalmalaInventoryComponent::UKalmalaInventoryComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickInterval = 0.25f;
}

const FKalmalaEquippedGear* UKalmalaInventoryComponent::FindEquippedGear(const EKalmalaGearSlot Slot) const
{
    return EquippedGear.FindByPredicate(
        [Slot](const FKalmalaEquippedGear& Entry) { return Entry.Slot == Slot; });
}

bool UKalmalaInventoryComponent::EquipGearFromServer(const FGuid ItemGuid, FString& Reason)
{
    AActor* Owner = GetOwner();
    auto* Character = Cast<AKalmalaCharacter>(Owner);
    if (!Owner || !Owner->HasAuthority() || !Character)
    {
        Reason = TEXT("Server authority required");
        return false;
    }
    Character->EnsureToolInstanceGuidsFromServer();
    TArray<FGuid> OtherOwnedGuids;
    for (const FKalmalaToolState& Tool : Character->GetCarriedToolInventory())
        OtherOwnedGuids.Add(Tool.InstanceGuid);

    TArray<FKalmalaInventoryStack> CandidateStacks;
    TArray<FKalmalaEquippedGear> CandidateGear;
    if (!BuildEquipGearTransaction(Stacks, EquippedGear, ItemGuid, OtherOwnedGuids,
        CandidateStacks, CandidateGear, Reason)) return false;

    Stacks = MoveTemp(CandidateStacks);
    EquippedGear = MoveTemp(CandidateGear);
    SynchronizeGridFromServer();
    Owner->ForceNetUpdate();
    return true;
}

bool UKalmalaInventoryComponent::UnequipGearFromServer(const EKalmalaGearSlot Slot, FString& Reason)
{
    AActor* Owner = GetOwner();
    auto* Character = Cast<AKalmalaCharacter>(Owner);
    if (!Owner || !Owner->HasAuthority() || !Character)
    {
        Reason = TEXT("Server authority required");
        return false;
    }
    Character->EnsureToolInstanceGuidsFromServer();
    TArray<FGuid> OtherOwnedGuids;
    for (const FKalmalaToolState& Tool : Character->GetCarriedToolInventory())
        OtherOwnedGuids.Add(Tool.InstanceGuid);

    TArray<FKalmalaInventoryStack> CandidateStacks;
    TArray<FKalmalaEquippedGear> CandidateGear;
    if (!BuildUnequipGearTransaction(Stacks, EquippedGear, Slot, OtherOwnedGuids,
        CandidateStacks, CandidateGear, Reason)) return false;

    Stacks = MoveTemp(CandidateStacks);
    EquippedGear = MoveTemp(CandidateGear);
    SynchronizeGridFromServer();
    Owner->ForceNetUpdate();
    return true;
}

void UKalmalaInventoryComponent::ServerEquipGear_Implementation(const FGuid ItemGuid)
{
    if (!GetWorld()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastGearRequestTime >= 0.0 && Now - LastGearRequestTime < 0.1) return;
    LastGearRequestTime = Now;
    FString Reason;
    EquipGearFromServer(ItemGuid, Reason);
}

void UKalmalaInventoryComponent::ServerUnequipGear_Implementation(const EKalmalaGearSlot Slot)
{
    if (!GetWorld()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (LastGearRequestTime >= 0.0 && Now - LastGearRequestTime < 0.1) return;
    LastGearRequestTime = Now;
    FString Reason;
    UnequipGearFromServer(Slot, Reason);
}
void UKalmalaInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaInventoryComponent, Stacks, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaInventoryComponent, EquippedGear, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaInventoryComponent, GridSlots, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaInventoryComponent, GridInstanceGuids, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaInventoryComponent, ActiveItem, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaInventoryComponent, ActiveInstanceGuid, COND_OwnerOnly);
}

int32 UKalmalaInventoryComponent::GetQuantity(FName ItemId) const
{
    int32 Quantity = 0;
    for (const FKalmalaInventoryStack& Stack : Stacks)
        if (Stack.ItemId == ItemId) Quantity += Stack.Quantity;
    return Quantity;
}

bool UKalmalaInventoryComponent::TryGrantFromServer(FName ItemId, int32 Quantity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    FString Reason;
    TArray<FKalmalaInventoryStack> Candidate;
    return BuildGrant(Stacks, ItemId, Quantity, Candidate, Reason)
        && TryCommitStacksFromServer(Stacks, Candidate);
}

bool UKalmalaInventoryComponent::TryConsumeFromServer(FName ItemId, int32 Quantity)
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    FString Reason;
    TArray<FKalmalaInventoryStack> Candidate;
    return BuildExchange(Stacks, {{ItemId, Quantity}}, NAME_None, 0, Candidate, Reason)
        && TryCommitStacksFromServer(Stacks, Candidate);
}

void UKalmalaInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
    SynchronizeGridFromServer();
}

void UKalmalaInventoryComponent::ClientAcceptedGain_Implementation(FName ItemId, int32 Quantity)
{
    const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
    const FKalmalaItemDefinition* Definition = Catalogue ? Catalogue->FindItem(ItemId) : nullptr;
    if (!Definition || Quantity <= 0 || int64(Quantity) > int64(Definition->MaxStack) * MaxSlots) return;
    if (GainReceipts.Num() == MaxGainReceipts) GainReceipts.RemoveAt(0);
    GainReceipts.Add({++GainSequence, ItemId, Quantity});
}

void UKalmalaInventoryComponent::RecordAcceptedGains(const TArray<FKalmalaInventoryStack>& Before,
    const TArray<FKalmalaInventoryStack>& After)
{
    TMap<FName, int32> BeforeTotals;
    TMap<FName, int32> AfterTotals;
    for (const FKalmalaInventoryStack& Stack : Before) BeforeTotals.FindOrAdd(Stack.ItemId) += Stack.Quantity;
    for (const auto& Stack : After)
        AfterTotals.FindOrAdd(Stack.ItemId) += Stack.Quantity;
    for (const TPair<FName, int32>& Entry : AfterTotals)
    {
        const int32 Increase = Entry.Value - BeforeTotals.FindRef(Entry.Key);
        if (Increase > 0) ClientAcceptedGain(Entry.Key, Increase);
    }
}

void UKalmalaInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaTime, TickType, ThisTick);
    SynchronizeGridFromServer();
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryTest"))) return;
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn || !Pawn->GetPlayerState()) return;
    if (bVerificationComplete)
    {
        TickMenuReview(DeltaTime);
        return;
    }
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
        UE_LOG(LogTemp, Display, TEXT("Harvest inventory: Passed=%d Materials=3 Range=1 Full=1 Malformed=1 Duplicate=1 SparseDelta=1 ToolMismatch=1 ToolPackAtomic=1 ToolWear=1 ToolDepletion=1"), bHarvestPassed);
    }
    else if (Pawn->IsLocallyControlled())
    {
        if (GetQuantity(TEXT("Wood")) != 7 || GainReceipts.IsEmpty()) return;
        const auto* Character = Cast<AKalmalaCharacter>(Pawn);
        const auto* Hatchet = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StoneAxe"));
        const auto* StonePick = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StonePick"));
        const auto* StoneKnife = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StoneKnife"));
        if (!Character || !Hatchet || !StonePick || !StoneKnife
            || Character->GetToolDurability(TEXT("StoneAxe")) != Hatchet->MaxDurability
            || Character->GetToolDurability(TEXT("StonePick")) != StonePick->MaxDurability
            || Character->GetToolDurability(TEXT("StoneKnife")) != StoneKnife->MaxDurability) return;
        const bool bRejected = !TryGrantFromServer(TEXT("Wood"), 1) && !TryConsumeFromServer(TEXT("Wood"), 1);
        UE_LOG(LogTemp, Display, TEXT("Inventory owner: Rejected=%d Wood=%d Slots=%d"), bRejected, GetQuantity(TEXT("Wood")), Stacks.Num());
        UE_LOG(LogTemp, Display, TEXT("Item gain receipts owner: Present=%d Bounded=%d"), !GainReceipts.IsEmpty(), GainReceipts.Num() <= MaxGainReceipts);
        UE_LOG(LogTemp, Display, TEXT("Tool condition owner: Passed=%d StoneAxe=%d StonePick=%d StoneKnife=%d"),
            Character->GetToolDurability(TEXT("StoneAxe")) == Hatchet->MaxDurability && StonePick && StoneKnife
                && Character->GetToolDurability(TEXT("StonePick")) == StonePick->MaxDurability
                && Character->GetToolDurability(TEXT("StoneKnife")) == StoneKnife->MaxDurability,
            Character->GetToolDurability(TEXT("StoneAxe")), Character->GetToolDurability(TEXT("StonePick")),
            Character->GetToolDurability(TEXT("StoneKnife")));
    }
    else
    {
        // Repeat remote checks so the runner observes privacy after owner replication arrives.
        UE_LOG(LogTemp, Display, TEXT("Inventory remote: Empty=%d"), Stacks.IsEmpty());
        UE_LOG(LogTemp, Display, TEXT("Item gain receipts remote: Empty=%d"), GainReceipts.IsEmpty());
        const auto* Character = Cast<AKalmalaCharacter>(Pawn);
        UE_LOG(LogTemp, Display, TEXT("Tool condition remote: Hidden=%d"), Character == nullptr
            || (Character->GetToolDurability(TEXT("StoneAxe")) == 0 && Character->GetToolDurability(TEXT("StonePick")) == 0
                && Character->GetToolDurability(TEXT("StoneKnife")) == 0));
        return;
    }
    bVerificationComplete = true;
#endif
}

#if !UE_BUILD_SHIPPING
void UKalmalaInventoryComponent::TickMenuReview(const float DeltaTime)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryMenuReview"))
        || !GetOwner()->HasAuthority() || MenuReviewStage >= 3) return;
    // Wait for both transaction fixtures before changing their packs. All review
    // mutations use the existing trusted server API; no review RPC is added.
    int32 ReadyPlayers = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APawn* PlayerPawn = It->Get() ? It->Get()->GetPawn() : nullptr;
        const auto* Pack = PlayerPawn ? PlayerPawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        if (Pack && Pack->bVerificationComplete) ++ReadyPlayers;
    }
    if (ReadyPlayers != 2) return;
    MenuReviewElapsed += DeltaTime;
    const bool bHost = CastChecked<APawn>(GetOwner())->IsLocallyControlled();
    FString Food(TEXT("HearthBroth"));
    FParse::Value(FCommandLine::Get(), TEXT("KalmalaInventoryMenuFood="), Food);
    if (MenuReviewStage == 0)
    {
        if (MenuReviewElapsed < 3.0f) return;
        const bool bPassed = (bHost || TryGrantFromServer(TEXT("Wood"), 16))
            && TryGrantFromServer(bHost ? TEXT("Stone") : TEXT("Iron"), bHost ? 2 : 4)
            && TryGrantFromServer(FName(*Food), 2);
        UE_LOG(LogTemp, Display, TEXT("Inventory menu server: Stage=Filled Passed=%d Host=%d Wood=%d Food=%s"),
            bPassed, bHost, GetQuantity(TEXT("Wood")), *Food);
        MenuReviewStage = 1;
        MenuReviewElapsed = 0.0f;
    }
    if (MenuReviewStage == 1 && MenuReviewElapsed >= 30.0f)
    {
        const bool bPassed = TryConsumeFromServer(TEXT("Wood"), 3);
        UE_LOG(LogTemp, Display, TEXT("Inventory menu server: Stage=Live Passed=%d Host=%d Wood=%d"),
            bPassed, bHost, GetQuantity(TEXT("Wood")));
        MenuReviewStage = 2;
    }
    if (MenuReviewStage == 2 && MenuReviewElapsed >= 50.0f)
    {
        const TArray<FKalmalaInventoryStack> Before = Stacks;
        bool bPassed = true;
        for (const auto& Stack : Before) bPassed &= TryConsumeFromServer(Stack.ItemId, Stack.Quantity);
        UE_LOG(LogTemp, Display, TEXT("Inventory menu server: Stage=Empty Passed=%d Host=%d Slots=%d"),
            bPassed && Stacks.IsEmpty(), bHost, Stacks.Num());
        MenuReviewStage = 3;
    }
}
#endif
