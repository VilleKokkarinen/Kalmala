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

    // Exercise the generated-source tool transaction, including a full-pack
    // rejection that must preserve condition and leave the node available.
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
    const int32 HatchetBefore = Character->GetToolDurability(TEXT("FieldHatchet"));
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
        const bool bFullPackRejected = !ToolNode->InteractWithToolIntentFromServer(
            Character, 0.0f, 250.0f, TEXT("FieldHatchet"), static_cast<uint8>(EKalmalaToolAction::Woodcutting));
        bPassed &= bWrongToolRejected && bFullPackRejected && !ToolNode->IsHarvested()
            && ToolHarvestEvents == 0 && Inventory->GetQuantity(TEXT("Wood")) == FullWood
            && Character->GetToolDurability(TEXT("FieldHatchet")) == HatchetBefore;
        if (FullWood > 0) bPassed &= Inventory->TryConsumeFromServer(TEXT("Wood"), 1);
        const bool bAccepted = ToolNode->InteractWithToolIntentFromServer(
            Character, 0.0f, 250.0f, TEXT("FieldHatchet"), static_cast<uint8>(EKalmalaToolAction::Woodcutting));
        const bool bDuplicateRejected = !ToolNode->InteractWithToolIntentFromServer(
            Character, 0.0f, 250.0f, TEXT("FieldHatchet"), static_cast<uint8>(EKalmalaToolAction::Woodcutting));
        bPassed &= bAccepted && bDuplicateRejected && ToolNode->IsHarvested() && ToolHarvestEvents == 1
            && Inventory->GetQuantity(TEXT("Wood")) == FullWood && Character->GetToolDurability(TEXT("FieldHatchet")) == HatchetBefore - 1;
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
    const int32 ConditionAfterGather = Character->GetToolDurability(TEXT("FieldHatchet"));
    const int32 RepairBaselineWood = Inventory->GetQuantity(TEXT("Wood"));
    const TArray<FKalmalaInventoryStack> RepairBaselineStacks = Inventory->GetStacks();
    FString RepairReason;
    const bool bNoStationRejected = Crafting != nullptr
        && !Crafting->RepairToolFromServer(TEXT("FieldHatchet"), RepairReason)
        && Character->GetToolDurability(TEXT("FieldHatchet")) == ConditionAfterGather
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
    const bool bFarStationRejected = !Crafting->RepairToolFromServer(TEXT("FieldHatchet"), RepairReason)
        && Character->GetToolDurability(TEXT("FieldHatchet")) == ConditionAfterGather
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
    const auto* HatchetDefinition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("FieldHatchet"));
    RepairReason.Reset();
    const bool bRepairAccepted = bPackCleared && HatchetDefinition && Inventory->GetStacks().IsEmpty()
        && Crafting->RepairToolFromServer(TEXT("FieldHatchet"), RepairReason)
        && Character->GetToolDurability(TEXT("FieldHatchet")) == HatchetDefinition->MaxDurability
        && Inventory->GetStacks().IsEmpty() && RepairReason.Contains(TEXT("at no cost"));
    RepairReason.Reset();
    const bool bFullConditionRejected = !Crafting->RepairToolFromServer(TEXT("FieldHatchet"), RepairReason)
        && HatchetDefinition && Character->GetToolDurability(TEXT("FieldHatchet")) == HatchetDefinition->MaxDurability
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
    UE_LOG(LogTemp, Display, TEXT("Tool free repair fixture: Passed=%d Workbench=1 NoCost=1 NoXP=1 FullCondition=1"), bRepairPassed);
    bPassed &= bRepairPassed;

    const TArray<FName> RetiredToolIds = {TEXT("FieldHatchet"), TEXT("StonePick"), TEXT("ReedKnife")};
    TArray<int32> RetiredToolConditions;
    RetiredToolConditions.Reserve(RetiredToolIds.Num());
    for (const FName ToolId : RetiredToolIds) RetiredToolConditions.Add(Character->GetToolDurability(ToolId));
    const TArray<FKalmalaInventoryStack> RetiredRecipePack = Inventory->GetStacks();
    const int32 ExperienceBeforeRetiredRecipes = CraftingAfter ? CraftingAfter->Experience : -1;
    bool bRetiredRecipesRejected = Crafting != nullptr;
    for (const FName RecipeId : {FName(TEXT("ReplaceFieldHatchet")), FName(TEXT("ReplaceStonePick")), FName(TEXT("ReplaceReedKnife"))})
    {
        FString RetiredReason;
        bRetiredRecipesRejected &= !Crafting->CraftFromServer(RecipeId, 1, RetiredReason);
    }
    bool bToolConditionsUnchanged = true;
    for (int32 Index = 0; Index < RetiredToolIds.Num(); ++Index)
        bToolConditionsUnchanged &= Character->GetToolDurability(RetiredToolIds[Index]) == RetiredToolConditions[Index];
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
    const auto* Catalogue = UKalmalaItemCatalogue::Get();
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
        || !UKalmalaItemCatalogue::Get()->IsValidStack(ItemId, Quantity)) return false;
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
        UE_LOG(LogTemp, Display, TEXT("Harvest inventory: Passed=%d Materials=3 Range=1 Full=1 Malformed=1 Duplicate=1 SparseDelta=1 ToolMismatch=1 ToolPackAtomic=1 ToolWear=1 ToolDepletion=1"), bHarvestPassed);
    }
    else if (Pawn->IsLocallyControlled())
    {
        if (GetQuantity(TEXT("Wood")) != 7) return;
        const auto* Character = Cast<AKalmalaCharacter>(Pawn);
        const auto* Hatchet = FKalmalaToolLifecycleContract::FindDefinition(TEXT("FieldHatchet"));
        const auto* StonePick = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StonePick"));
        const auto* ReedKnife = FKalmalaToolLifecycleContract::FindDefinition(TEXT("ReedKnife"));
        if (!Character || !Hatchet || !StonePick || !ReedKnife
            || Character->GetToolDurability(TEXT("FieldHatchet")) != Hatchet->MaxDurability
            || Character->GetToolDurability(TEXT("StonePick")) != StonePick->MaxDurability
            || Character->GetToolDurability(TEXT("ReedKnife")) != ReedKnife->MaxDurability) return;
        const bool bRejected = !TryGrantFromServer(TEXT("Wood"), 1) && !TryConsumeFromServer(TEXT("Wood"), 1);
        UE_LOG(LogTemp, Display, TEXT("Inventory owner: Rejected=%d Wood=%d Slots=%d"), bRejected, GetQuantity(TEXT("Wood")), Stacks.Num());
        UE_LOG(LogTemp, Display, TEXT("Tool condition owner: Passed=%d FieldHatchet=%d StonePick=%d ReedKnife=%d"),
            Character->GetToolDurability(TEXT("FieldHatchet")) == Hatchet->MaxDurability && StonePick && ReedKnife
                && Character->GetToolDurability(TEXT("StonePick")) == StonePick->MaxDurability
                && Character->GetToolDurability(TEXT("ReedKnife")) == ReedKnife->MaxDurability,
            Character->GetToolDurability(TEXT("FieldHatchet")), Character->GetToolDurability(TEXT("StonePick")),
            Character->GetToolDurability(TEXT("ReedKnife")));
    }
    else
    {
        // Repeat remote checks so the runner observes privacy after owner replication arrives.
        UE_LOG(LogTemp, Display, TEXT("Inventory remote: Empty=%d"), Stacks.IsEmpty());
        const auto* Character = Cast<AKalmalaCharacter>(Pawn);
        UE_LOG(LogTemp, Display, TEXT("Tool condition remote: Hidden=%d"), Character == nullptr
            || (Character->GetToolDurability(TEXT("FieldHatchet")) == 0 && Character->GetToolDurability(TEXT("StonePick")) == 0
                && Character->GetToolDurability(TEXT("ReedKnife")) == 0));
        return;
    }
    bVerificationComplete = true;
#endif
}
