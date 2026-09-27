#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaPlayerStatusComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UKalmalaCraftingComponent::UKalmalaCraftingComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .25f;
}

AKalmalaCharacter* UKalmalaCraftingComponent::GetCharacter() const { return Cast<AKalmalaCharacter>(GetOwner()); }

void UKalmalaCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, LastResult, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, ResultSerial, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, bLastResultAccepted, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, StorageView, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, bStorageViewOpen, COND_OwnerOnly);
}

bool UKalmalaCraftingComponent::AcceptRequest()
{
    auto* Character = GetCharacter();
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextRequestTime) { PublishResult(TEXT("Please wait before the next action"), false); return false; }
    NextRequestTime = Now + .2;
    return true;
}

void UKalmalaCraftingComponent::PublishResult(const FString& Result, const bool bAccepted)
{
    if (GetOwner() && GetOwner()->HasAuthority())
    {
        LastResult = Result.Left(160);
        bLastResultAccepted = bAccepted;
        ResultSerial = ResultSerial == TNumericLimits<uint32>::Max() ? 1 : ResultSerial + 1;
        GetOwner()->ForceNetUpdate();
    }
}

AKalmalaCampfire* UKalmalaCraftingComponent::FindNearbyFire(bool bRequireUsable) const
{
    const auto* Character = GetCharacter();
    if (!Character || !GetWorld()) return nullptr;
    AKalmalaCampfire* Closest = nullptr; double Best = FMath::Square(250.0);
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It)
    {
        if (!IsValid(*It) || (bRequireUsable && !It->CanUse(Character))) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation());
        if (Distance <= Best) { Best = Distance; Closest = *It; }
    }
    return Closest;
}

AKalmalaCampfire* UKalmalaCraftingComponent::FindNearbyLitFire() const
{
    const auto* Character = GetCharacter();
    if (!Character || !GetWorld()) return nullptr;
    AKalmalaCampfire* Closest = nullptr;
    double Best = FMath::Square(250.0);
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It)
    {
        const AKalmalaCampfire* Fire = *It;
        if (!IsValid(Fire) || !Fire->CanUse(Character) || !Fire->IsLit()
            || !FMath::IsFinite(Fire->GetEffectiveWarmth()) || Fire->GetEffectiveWarmth() <= 0.0f) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), Fire->GetActorLocation());
        if (FMath::IsFinite(Distance) && Distance <= Best)
        {
            Best = Distance;
            Closest = *It;
        }
    }
    return Closest;
}

bool UKalmalaCraftingComponent::CraftFromServer(FName RecipeId, int32 Batch, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const auto* Recipe = GetDefault<UKalmalaRecipeCatalogue>()->Find(RecipeId);
    Reason = TEXT("Unknown or disabled recipe");
    if (!Recipe || !Recipe->bEnabled) return false;
    TArray<FKalmalaInventoryStack> Costs; int32 OutputCount;
    Reason = TEXT("Invalid batch quantity");
    if (!UKalmalaRecipeCatalogue::Scale(*Recipe, Batch, Costs, OutputCount)) return false;
    if (Recipe->bRequiresLitCampfire && !FindNearbyLitFire())
    {
        Reason = TEXT("Need a usable lit hearth with heat within 2.5 m");
        return false;
    }
    if (Recipe->bRequiresCampfire && !FindNearbyFire(true) && !FindNearbyWorkbench()) { Reason = TEXT("Need a usable hearth or workbench within 2.5 m"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    if (!Inventory || !Inventory->TryExchangeFromServer(Costs, Recipe->Output, OutputCount, Reason)) return false;
    Reason = FString::Printf(TEXT("Crafted %d %s"), OutputCount, *GetDefault<UKalmalaItemCatalogue>()->FindItem(Recipe->Output)->DisplayName);
    return true;
}

void UKalmalaCraftingComponent::ServerCraft_Implementation(FName RecipeId, int32 Batch)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool Accepted=CraftFromServer(RecipeId, Batch, Reason); PublishResult(Reason, Accepted);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("KalmalaCraftingTest")))
        UE_LOG(LogTemp,Display,TEXT("Crafting RPC: Recipe=%s Batch=%d Accepted=%d"),*RecipeId.ToString(),Batch,Accepted);
#endif
}

bool UKalmalaCraftingComponent::PlaceFromServer(FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) { Reason = TEXT("Waiting for world"); return false; }
    int32 Count = 0;
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It) if (IsValid(*It)) ++Count;
    if (Count >= 32) { Reason = TEXT("Session hearth limit reached (32)"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    const TArray<FKalmalaInventoryStack> Costs = {{TEXT("CampfireKit"),1},{TEXT("Fuel"),1}};
    TArray<FKalmalaInventoryStack> Preview;
    if (!Inventory || !UKalmalaInventoryComponent::BuildExchange(Inventory->GetStacks(), Costs, NAME_None, 0, Preview, Reason)) return false;
    const FVector Forward = FRotator(0, Character->GetActorRotation().Yaw, 0).Vector();
    const FVector Probe = Character->GetActorLocation() + Forward * 165;
    if (!FKalmalaWorldBounds::Contains(State->GetWorldGenerationConfig(), FVector2D(Probe), 300)) { Reason = TEXT("Too close to the world edge"); return false; }
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CampfirePlacement), false, Character);
    FHitResult Ground;
    Reason = TEXT("Need clear, dry, gently sloping ground ahead");
    if (Probe.ContainsNaN() || !GetWorld()->LineTraceSingleByChannel(Ground, Probe + FVector(0,0,100), Probe-FVector(0,0,350), ECC_Visibility, Query)
        || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor()) || Ground.ImpactNormal.Z < .85f
        || FVector::DistSquared(Character->GetActorLocation(), Ground.ImpactPoint) > FMath::Square(250.0)) return false;
    const auto& Config = State->GetWorldGenerationConfig();
    if (FKalmalaOceanSampler::Sample(Config, FVector2D(Ground.ImpactPoint)).IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(Config, FVector2D(Ground.ImpactPoint))
        || (FKalmalaRegionalGeneration::Sample(Config, FVector2D(Ground.ImpactPoint)).bHasWater)) return false;
    const FVector Location = Ground.ImpactPoint + FVector(0,0,56);
    if (GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(54), Query)) return false;
    // Allocation precedes payment; deferred construction has no gameplay callbacks before payment.
    auto* Fire = GetWorld()->SpawnActorDeferred<AKalmalaCampfire>(AKalmalaCampfire::StaticClass(), FTransform(Location), nullptr, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Fire) { Reason = TEXT("Could not allocate hearth"); return false; }
    if (!Inventory->TryExchangeFromServer(Costs, NAME_None, 0, Reason)) { Fire->Destroy(); return false; }
    Fire->InitializePaidFromServer(Character);
    Fire->FinishSpawning(FTransform(Location));
    Reason = TEXT("Placed hearth with 60 seconds of fuel; light it nearby");
    return true;
}

bool UKalmalaCraftingComponent::PlaceConstructionFromServer(const FName KitId, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    if (KitId == TEXT("CampfireKit")) return PlaceFromServer(Reason);
    Reason = TEXT("Unknown construction kit");
    if (!FKalmalaPlacementPreview::IsSupportedKit(KitId)) return false;
    const FKalmalaPlacementPreview Preview = FKalmalaPlacementPreview::Evaluate(GetWorld(), Character, KitId);
    Reason = Preview.Message;
    if (!Preview.bIsValid || FVector::DistSquared(Character->GetActorLocation(), Preview.Location) > FMath::Square(250.0f)) return false;
    const FRotator Rotation(0.0f, Character->GetActorRotation().Yaw, 0.0f);
    if (Rotation.ContainsNaN()) { Reason = TEXT("Invalid placement rotation"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    const TArray<FKalmalaInventoryStack> Cost = {{KitId, 1}};
    TArray<FKalmalaInventoryStack> Scratch;
    if (!Inventory || !UKalmalaInventoryComponent::BuildExchange(Inventory->GetStacks(), Cost, NAME_None, 0, Scratch, Reason)) return false;
    const FTransform Transform(Rotation, Preview.Location);
    auto* GameMode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    if (!GameMode || !GameMode->CanPersistConstruction(KitId, Transform)) { Reason = TEXT("Construction save limit reached or unavailable"); return false; }
    auto* Construction = GetWorld()->SpawnActorDeferred<AKalmalaConstructionActor>(AKalmalaConstructionActor::StaticClass(), Transform, nullptr, Character,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Construction) { Reason = TEXT("Could not allocate construction"); return false; }
    if (!Inventory->TryExchangeFromServer(Cost, NAME_None, 0, Reason)) { Construction->Destroy(); return false; }
    Construction->InitializeFromServer(KitId, FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
    Construction->FinishSpawning(Transform);
    if (!GameMode->PersistConstruction(Construction))
    {
        Construction->Destroy();
        Inventory->TryGrantFromServer(KitId, 1);
        Reason = TEXT("Could not save construction; kit restored");
        return false;
    }
    Reason = TEXT("Placed construction; server accepted the kit and ground");
    return true;
}

void UKalmalaCraftingComponent::ServerPlaceCampfire_Implementation()
{
    if (!AcceptRequest()) return;
    FString Reason; const bool Accepted=PlaceFromServer(Reason); PublishResult(Reason, Accepted);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("KalmalaCraftingTest")))
        UE_LOG(LogTemp,Display,TEXT("Crafting placement RPC: Accepted=%d"),Accepted);
#endif
}

void UKalmalaCraftingComponent::ServerPlaceConstruction_Implementation(const FName KitId)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool Accepted = PlaceConstructionFromServer(KitId, Reason); PublishResult(Reason, Accepted);
}

void UKalmalaCraftingComponent::ServerRefuel_Implementation()
{
    if (!AcceptRequest()) return;
    auto* Fire = FindNearbyFire(true);
    const bool bAccepted = Fire != nullptr && Fire->TryRefuelFromServer(GetCharacter());
    PublishResult(bAccepted ? TEXT("Added one ember bundle (60 seconds)")
        : TEXT("Need a usable nearby hearth, one fuel bundle and 60 seconds of free capacity"), bAccepted);
}

void UKalmalaCraftingComponent::ServerLight_Implementation()
{
    if (!AcceptRequest()) return;
    auto* Fire = FindNearbyFire(true);
    const bool bAccepted = Fire && Fire->CanInteract_Implementation(GetCharacter());
    if (bAccepted) Fire->Interact_Implementation(GetCharacter());
    PublishResult(bAccepted ? TEXT("Hearth lit") : TEXT("Need a usable nearby unlit hearth with dry fuel"), bAccepted);
}

FString UKalmalaCraftingComponent::GetRecipeAvailability(FName Id) const
{
    const auto* R = GetDefault<UKalmalaRecipeCatalogue>()->Find(Id);
    if (!R || !R->bEnabled) return TEXT("Recipe unavailable");
    if (R->bRequiresLitCampfire && !FindNearbyLitFire()) return TEXT("Need a usable lit hearth with heat within 2.5 m");
    if (R->bRequiresCampfire && !FindNearbyFire(true) && !FindNearbyWorkbench()) return TEXT("Need a usable hearth or workbench within 2.5 m");
AKalmalaConstructionActor* UKalmalaCraftingComponent::FindNearbyToolProgressionStation(const FName Kit) const
{
    if (Kit == TEXT("WorkbenchKit")) return FindNearbyConstruction(Kit);
    if (Kit != TEXT("ForgeKit")) return nullptr;

    const AKalmalaCharacter* Character = GetCharacter();
    if (!Character || !Character->GetController() || !GetWorld()) return nullptr;
    const AKalmalaWorldGenerationGameState* State =
        GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) return nullptr;

    AKalmalaConstructionActor* Closest = nullptr;
    double Best = FMath::Square(250.0);
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
    {
        AKalmalaConstructionActor* Candidate = *It;
        if (!IsValid(Candidate) || Candidate->GetWorld() != Character->GetWorld()
            || Candidate->GetConstructionKit() != Kit || Candidate->GetConstructionId().IsEmpty()
            || (Character->HasAuthority() && !Candidate->HasAuthority())
            || Character->GetActorLocation().ContainsNaN() || Candidate->GetActorLocation().ContainsNaN())
        {
            continue;
        }
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), Candidate->GetActorLocation());
        if (!FMath::IsFinite(Distance) || Distance > Best) continue;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ToolProgressionStation), false, Character);
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(
            Hit, Character->GetPawnViewLocation(), Candidate->GetActorLocation(), ECC_Visibility, Query)
            || Hit.GetActor() != Candidate) continue;
        if (Distance < Best || (Distance == Best
            && (!Closest || Candidate->GetConstructionId() < Closest->GetConstructionId())))
        {
            Best = Distance;
            Closest = Candidate;
        }
    }
    return Closest;
}

bool UKalmalaCraftingComponent::ProgressToolFromServer(const FName ToolId, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;

    const FKalmalaToolProgressionEntry* Entry = FKalmalaToolProgressionContract::FindEntry(ToolId);
    Reason = TEXT("Unknown tool progression");
    if (!Entry || !FKalmalaToolProgressionContract::IsCatalogueValid()) return false;
    const FName StationKit = FKalmalaToolProgressionContract::GetStationKit(Entry->RequiredStation);
    const TCHAR* StationName = Entry->RequiredStation == EKalmalaToolStationKind::Workbench
        ? TEXT("Workbench") : TEXT("Forge");
    const AKalmalaConstructionActor* Station = FindNearbyToolProgressionStation(StationKit);
    Reason = FString::Printf(TEXT("Need a visible same-world %s within 2.5 m"), StationName);
    if (!Station) return false;

    const int32 EffectiveLevel = FKalmalaToolProgressionContract::GetBaseStationLevel(Station->GetConstructionKit());
    Reason = FString::Printf(TEXT("Nearby %s is level %d; level %d is required"),
        StationName, EffectiveLevel, Entry->RequiredStationLevel);
    if (EffectiveLevel != Entry->RequiredStationLevel) return false;

    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    Reason = TEXT("Pack unavailable");
    if (!Inventory) return false;
    TArray<FKalmalaToolState> CandidateTools;
    TArray<FKalmalaInventoryStack> CandidateInventory;
    const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
    if (!FKalmalaToolProgressionContract::BuildServerUpgrade(
        true, Station->GetConstructionKit(), EffectiveLevel, ToolId,
        Character->CarriedTools, Before, CandidateTools, CandidateInventory, Reason))
    {
        return false;
    }
    if (!Inventory->TryCommitStacksFromServer(Before, CandidateInventory))
    {
        Reason = TEXT("Pack changed; tool progression was not applied");
        return false;
    }

    Character->CarriedTools = MoveTemp(CandidateTools);
    Character->ForceNetUpdate();
    Reason = FString::Printf(TEXT("Crafted %s at level %d; previous tool exchanged where required"),
        *ToolId.ToString(), Entry->TargetToolLevel);
    return true;
}

void UKalmalaCraftingComponent::ServerProgressTool_Implementation(const FName ToolId)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = ProgressToolFromServer(ToolId, Reason);
    PublishResult(Reason, bAccepted);
}

    auto* C = GetCharacter(); auto* Inv = C ? C->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Inv) return TEXT("Waiting for pack");
    TArray<FKalmalaInventoryStack> Next; FString Reason;
    UKalmalaInventoryComponent::BuildExchange(Inv->GetStacks(), R->Ingredients, R->Output, R->OutputCount, Next, Reason);
    return Reason;
}

FString UKalmalaCraftingComponent::GetRecipeDescription(FName Id) const
{
    const auto* R = GetDefault<UKalmalaRecipeCatalogue>()->Find(Id);
    if (!R) return TEXT("Unknown recipe");
    FString Text = R->DisplayName + TEXT("\nCost: ");
    for (const auto& Cost : R->Ingredients)
        Text += FString::Printf(TEXT("%d %s  "), Cost.Quantity, *GetDefault<UKalmalaItemCatalogue>()->FindItem(Cost.ItemId)->DisplayName);
    Text += FString::Printf(TEXT("\nOutput: %d (stack limit %d)"),R->OutputCount,GetDefault<UKalmalaItemCatalogue>()->FindItem(R->Output)->MaxStack);
    if (R->bRequiresLitCampfire) return Text + TEXT("\nStation: usable lit hearth with heat within 2.5 m");
    return Text + (R->bRequiresCampfire ? TEXT("\nStation: nearby usable hearth or workbench") : TEXT("\nHandcrafted; no station"));
}

FString UKalmalaCraftingComponent::GetFoodText() const
{
    const auto* Character = GetCharacter();
    const auto* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const auto* Status = Character ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    const int32 Count = Inventory ? Inventory->GetQuantity(UKalmalaPlayerStatusComponent::RoastedFieldMeatItemId) : 0;
    const float Remaining = Status ? Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId) : 0.0f;
    if (Remaining > 0.0f)
    {
        return FString::Printf(TEXT("Steady meal: %.0f seconds remaining; stamina use is 10%% lower. Additional meals cannot stack or replace it."), Remaining);
    }
    if (Count > 0)
    {
        return FString::Printf(TEXT("Roasted field meat: %d available. Eat one for 120 seconds of 10%% lower stamina use."), Count);
    }
    return TEXT("Roasted field meat: none. Roast boar or deer meat at a nearby usable lit hearth.");
}

FString UKalmalaCraftingComponent::GetNearbyFireText() const
{
    const auto* Fire = FindNearbyFire(false);
    return Fire ? Fire->GetStatusText() : TEXT("No hearth within 2.5 m");
}

void UKalmalaCraftingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(DeltaTime, TickType, ThisTick);
    if (GetOwner() && GetOwner()->HasAuthority() && bStorageViewOpen) RefreshStorageView();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCraftingTest"))) RunVerification(DeltaTime);
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaStorageTest"))) RunStorageVerification(DeltaTime);
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaPersistedCampTest"))
        || FParse::Param(FCommandLine::Get(), TEXT("KalmalaPersistedCampRestoreTest"))) RunPersistedCampVerification(DeltaTime);
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaRainVerticalSliceTest"))) RunRainVerticalSliceVerification(DeltaTime);
#endif
}

bool UKalmalaCraftingComponent::ConsumeFoodFromServer(const FName FoodItemId, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    auto* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    Reason = TEXT("Unknown or unavailable food");
    if (!Inventory || !Status || !Status->CanApplyFoodFromServer(FoodItemId))
    {
        if (UKalmalaPlayerStatusComponent::IsKnownFoodItem(FoodItemId) && Status && Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
            Reason = TEXT("A steady meal is already active; wait for it to expire");
        return false;
    }

    const TArray<FKalmalaInventoryStack>& Before = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> Candidate;
    if (!UKalmalaInventoryComponent::BuildExchange(Before, {{FoodItemId, 1}}, NAME_None, 0, Candidate, Reason)) return false;
    if (!Inventory->TryCommitStacksFromServer(Before, Candidate))
    {
        Reason = TEXT("Pack changed; food was not consumed");
        return false;
    }
    if (!Status->ApplyFoodFromServer(FoodItemId))
    {
        Inventory->TryCommitStacksFromServer(Candidate, Before);
        Reason = TEXT("Meal changed; food was restored");
        return false;
    }

    const auto* Item = GetDefault<UKalmalaItemCatalogue>()->FindItem(FoodItemId);
    Reason = FString::Printf(TEXT("Ate %s; stamina use is 10%% lower for %.0f seconds"),
        Item ? *Item->DisplayName : TEXT("prepared food"), UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    return true;
}

void UKalmalaCraftingComponent::ServerConsumeFood_Implementation(const FName FoodItemId)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = ConsumeFoodFromServer(FoodItemId, Reason);
    PublishResult(Reason, bAccepted);
}
FString UKalmalaCraftingComponent::GetToolProgressionText() const
{
    const AKalmalaCharacter* Character = GetCharacter();
    const UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const UKalmalaItemCatalogue* Items = GetDefault<UKalmalaItemCatalogue>();
    if (!Character || !Inventory || !Items) return TEXT("Tool progression is waiting for your private inventory.\n");
    const auto ToolName = [](const FName ToolId)
    {
        if (ToolId == TEXT("BronzeAxe")) return FString(TEXT("Bronze Axe"));
        if (ToolId == TEXT("IronAxe")) return FString(TEXT("Iron Axe"));
        return ToolId.ToString();
    };

    FString Text = TEXT("\nTOOL PROGRESSION — OWNER ONLY\n");
    for (const FKalmalaToolProgressionEntry& Entry : FKalmalaToolProgressionContract::GetEntries())
    {
        const TCHAR* StationName = Entry.RequiredStation == EKalmalaToolStationKind::Workbench
            ? TEXT("Workbench") : TEXT("Forge");
        const FName StationKit = FKalmalaToolProgressionContract::GetStationKit(Entry.RequiredStation);
        const FKalmalaToolState* Existing = Character->GetCarriedToolInventory().FindByPredicate(
            [&Entry](const FKalmalaToolState& State) { return State.ToolId == Entry.ToolId; });
        Text += FString::Printf(TEXT("%s: target level %d; "), *ToolName(Entry.ToolId), Entry.TargetToolLevel);
        if (Existing)
        {
            Text += FString::Printf(TEXT("already carried at level %d (%d condition). "),
                Existing->ToolLevel, Existing->Durability);
        }
        else if (!Entry.PreviousToolId.IsNone())
        {
            const int32 PreviousLevel = Character->GetCarriedToolLevel(Entry.PreviousToolId);
            Text += PreviousLevel == Entry.PreviousToolLevel
                ? FString::Printf(TEXT("upgrades %s level %d. "), *ToolName(Entry.PreviousToolId), Entry.PreviousToolLevel)
                : FString::Printf(TEXT("requires carried %s level %d. "), *ToolName(Entry.PreviousToolId), Entry.PreviousToolLevel);
        }

        Text += TEXT("Cost: ");
        for (int32 Index = 0; Index < Entry.MaterialCosts.Num(); ++Index)
        {
            const FKalmalaToolMaterialCost& Cost = Entry.MaterialCosts[Index];
            const FKalmalaItemDefinition* Item = Items->FindItem(Cost.ItemId);
            Text += FString::Printf(TEXT("%s%d %s (have %d)"),
                Index == 0 ? TEXT("") : TEXT(", "), Cost.Quantity,
                Item ? *Item->DisplayName : *Cost.ItemId.ToString(),
                Inventory->GetQuantity(Cost.ItemId));
        }

        const AKalmalaConstructionActor* Station = FindNearbyToolProgressionStation(StationKit);
        if (Station)
        {
            const int32 Level = FKalmalaToolProgressionContract::GetBaseStationLevel(Station->GetConstructionKit());
            Text += FString::Printf(TEXT(". Nearby %s level %d; required level %d"),
                StationName, Level, Entry.RequiredStationLevel);
        }
        else
        {
            Text += FString::Printf(TEXT(". Need a visible same-world %s within 2.5 m"),
                StationName);
        }
        Text += TEXT(".\n");
    }
    Text += TEXT("Workbench and Forge bases are level 1. Attachments are required for level 2; the server checks the station again when you craft.");
    return Text;
}
