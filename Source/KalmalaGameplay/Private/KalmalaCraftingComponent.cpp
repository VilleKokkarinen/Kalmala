#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaRawFuelContract.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
FString GetToolDisplayName(const FName ToolId)
{
    if (ToolId == TEXT("ReedKnife")) return TEXT("Reed Knife");
    if (ToolId == TEXT("FieldHatchet")) return TEXT("Field Hatchet");
    if (ToolId == TEXT("StonePick")) return TEXT("Stone Pick");
    if (ToolId == TEXT("BronzeAxe")) return TEXT("Bronze Axe");
    if (ToolId == TEXT("IronAxe")) return TEXT("Iron Axe");
    return ToolId.ToString();
}

FString GetStationDisplayName(const FName KitId)
{
    const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(KitId);
    return Item ? Item->DisplayName : KitId.ToString();
}

FString GetRecipeStationNames(const FKalmalaRecipe& Recipe)
{
    FString Names;
    for (const FName Station : Recipe.RequiredStation)
    {
        if (!Names.IsEmpty()) Names += TEXT(" or ");
        Names += GetStationDisplayName(Station);
    }
    return Names;
}

bool IsFoodProcessingStation(const AKalmalaConstructionActor* Station)
{
    if (!Station) return false;
    const FName Kit = Station->GetConstructionKit();
    return Kit == TEXT("CookingRackKit") || Kit == TEXT("CauldronKit") || Kit == TEXT("FryingPanKit")
        || Kit == TEXT("SmokeFrameKit");
}

FString GetRecipeToolDisplayName(const FName ToolId)
{
    const FKalmalaItemDefinition* Item = UKalmalaItemCatalogue::Get()->FindItem(ToolId);
    return Item ? Item->DisplayName : ToolId.ToString();
}

FString GetMissingFoodHeatReason(const AKalmalaConstructionActor* Station)
{
    if (!Station) return TEXT("Need a usable lit hearth with positive heat within 2.5 m");
    const auto* StationItem = UKalmalaItemCatalogue::Get()->FindItem(Station->GetConstructionKit());
    const FString StationName = StationItem ? StationItem->DisplayName : Station->GetConstructionKit().ToString();
    return FString::Printf(TEXT("Need a usable lit hearth with positive heat within 2.5 m of both the player and %s"), *StationName);
}

}

UKalmalaCraftingComponent::UKalmalaCraftingComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = .25f;
}

AKalmalaCharacter* UKalmalaCraftingComponent::GetCharacter() const { return Cast<AKalmalaCharacter>(GetOwner()); }

FName UKalmalaCraftingComponent::GetLookedAtCookingStationKit() const
{
    const AKalmalaCharacter* Character = GetCharacter();
    AController* Controller = Character ? Character->GetController() : nullptr;
    if (!Character || !Character->IsLocallyControlled() || !Controller || !GetWorld()) return NAME_None;
    FVector ViewLocation;
    FRotator ViewRotation;
    Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CookingStationPrompt), false, Character);
    FHitResult Hit;
    if (!GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation,
            ViewLocation + ViewRotation.Vector() * 250.0f, ECC_Visibility, Query)) return NAME_None;
    const AKalmalaConstructionActor* Station = Cast<AKalmalaConstructionActor>(Hit.GetActor());
    if (!Station || Station->GetConstructionId().IsEmpty()
        || FVector::DistSquared(Character->GetActorLocation(), Station->GetActorLocation()) > FMath::Square(250.0f)) return NAME_None;
    const FName Kit = Station->GetConstructionKit();
    return Kit == TEXT("CookingRackKit") || Kit == TEXT("CauldronKit") || Kit == TEXT("FryingPanKit")
        ? Kit : NAME_None;
}

void UKalmalaCraftingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, LastResult, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, ResultSerial, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, bLastResultAccepted, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, StorageView, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, bStorageViewOpen, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, LastInteractedCookingStationKit, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UKalmalaCraftingComponent, CookingStationInteractionSerial, COND_OwnerOnly);
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

AKalmalaCampfire* UKalmalaCraftingComponent::FindNearbyLitFire(const AKalmalaConstructionActor* RequiredStation) const
{
    const auto* Character = GetCharacter();
    if (!Character || !GetWorld()) return nullptr;
    AKalmalaCampfire* Closest = nullptr;
    double Best = FMath::Square(250.0);
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It)
    {
        const AKalmalaCampfire* Fire = *It;
        if (!IsValid(Fire) || !Fire->CanUse(Character) || !Fire->IsLit()) continue;
        const FVector HeatPoint = RequiredStation ? RequiredStation->GetActorLocation() : Character->GetActorLocation();
        const float AvailableHeat = Fire->GetWarmthContributionAt(HeatPoint);
        if (!FMath::IsFinite(AvailableHeat) || AvailableHeat <= 0.0f) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), Fire->GetActorLocation());
        const double StationDistance = RequiredStation
            ? FVector::DistSquared(RequiredStation->GetActorLocation(), Fire->GetActorLocation()) : 0.0;
        if (FMath::IsFinite(Distance) && Distance <= Best
            && (!RequiredStation || (FMath::IsFinite(StationDistance) && StationDistance <= FMath::Square(250.0))))
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
    const auto* Recipe = UKalmalaRecipeCatalogue::Get()->Find(RecipeId);
    Reason = TEXT("Unknown or disabled recipe");
    if (!Recipe || !Recipe->bEnabled) return false;
    if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(Recipe->Output))
    {
        Reason = TEXT("Use the construction hammer to build this directly from its recipe materials");
        return false;
    }
    TArray<FKalmalaInventoryStack> Costs; int32 OutputCount;
    Reason = TEXT("Invalid batch quantity");
    if (!UKalmalaRecipeCatalogue::Scale(*Recipe, Batch, Costs, OutputCount)) return false;
    if (FKalmalaToolProgressionContract::IsStationAttachmentKit(Recipe->Output))
    {
        const FName StationKit = FKalmalaToolProgressionContract::GetAttachmentStationKit(Recipe->Output);
        const AKalmalaConstructionActor* Station = FindNearbyToolProgressionStation(StationKit);
        const TCHAR* StationName = StationKit == TEXT("WorkbenchKit") ? TEXT("Workbench") : TEXT("Forge");
        Reason = FString::Printf(TEXT("Need a visible same-world %s within 2.5 m"), StationName);
        if (!Station) return false;
    }
    const AKalmalaConstructionActor* RequiredStation = Recipe->RequiredStation.IsEmpty()
        ? nullptr : FindNearbyConstruction(Recipe->RequiredStation);
    if (!Recipe->RequiredStation.IsEmpty() && !RequiredStation)
    {
        Reason = FString::Printf(TEXT("Need a visible same-world %s within 2.5 m"),
            *GetRecipeStationNames(*Recipe));
        return false;
    }
    if (IsFoodProcessingStation(RequiredStation) && !FindNearbyLitFire(RequiredStation))
    {
        Reason = GetMissingFoodHeatReason(RequiredStation);
        return false;
    }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    if (!Inventory) { Reason = TEXT("Waiting for pack"); return false; }
    if (!UKalmalaRecipeCatalogue::HasRequiredTool(Recipe->RequiredTool, Inventory->GetStacks()))
    {
        Reason = FString::Printf(TEXT("Need a %s in your pack"), *GetRecipeToolDisplayName(Recipe->RequiredTool));
        return false;
    }
    if (!Inventory || !Inventory->TryExchangeFromServer(Costs, Recipe->Output, OutputCount, Reason)) return false;
    if (Recipe->ExperienceAward > 0)
    {
        if (auto* Progression = Character->GetSkillProgressionComponent())
            Progression->AwardExperienceFromAcceptedServerAction(Recipe->ExperienceSkill, Recipe->ExperienceAward);
    }
    const auto* OutputItem = UKalmalaItemCatalogue::Get()->FindItem(Recipe->Output);
    Reason = FString::Printf(TEXT("Crafted %d %s"), OutputCount, OutputItem ? *OutputItem->DisplayName : *Recipe->Output.ToString());
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

    const int32 EffectiveLevel = FKalmalaToolProgressionContract::GetEffectiveStationLevel(Station);
    Reason = FString::Printf(TEXT("Nearby %s is level %d; level %d is required"),
        StationName, EffectiveLevel, Entry->RequiredStationLevel);
    if (EffectiveLevel != Entry->RequiredStationLevel) return false;

    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    Reason = TEXT("Pack unavailable");
    if (!Inventory) return false;
    TArray<FKalmalaToolState> CandidateTools;
    TArray<FKalmalaInventoryStack> CandidateInventory;
    const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
    if (!FKalmalaToolProgressionContract::BuildServerUpgradeFromCharacter(
        Character, Station->GetConstructionKit(), EffectiveLevel, ToolId,
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

bool UKalmalaCraftingComponent::PlaceFromServer(FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    Reason = TEXT("Need your carried Construction Hammer to build");
    if (Character->GetCarriedToolLevel(TEXT("ConstructionHammer")) < 1) return false;
    const auto* State = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (!State || !State->GetWorldGenerationConfig().IsValid()) { Reason = TEXT("Waiting for world"); return false; }
    int32 Count = 0;
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It) if (IsValid(*It)) ++Count;
    if (Count >= 32) { Reason = TEXT("Session hearth limit reached (32)"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    TArray<FKalmalaInventoryStack> Costs;
    if (!UKalmalaRecipeCatalogue::BuildDirectMaterialCost(TEXT("CampfireKit"), Costs, Reason)
        || !Inventory || !FKalmalaRawFuelContract::AddCosts(Inventory->GetStacks(), 1, Costs, Reason))
    {
        if (Reason == TEXT("Ready")) Reason = TEXT("Hearth placement cost exceeds its bound");
        return false;
    }
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

bool UKalmalaCraftingComponent::PlaceConstructionFromServer(const FName BuildableId, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    Reason = TEXT("Need your carried Construction Hammer to build");
    if (Character->GetCarriedToolLevel(TEXT("ConstructionHammer")) < 1) return false;
    if (BuildableId == TEXT("CampfireKit")) return PlaceFromServer(Reason);
    Reason = TEXT("Unknown buildable identity");
    if (!FKalmalaPlacementPreview::IsSupportedKit(BuildableId)) return false;
    const FKalmalaPlacementPreview Preview = FKalmalaPlacementPreview::Evaluate(GetWorld(), Character, BuildableId);
    Reason = Preview.Message;
    if (!Preview.bIsValid || FVector::DistSquared(Character->GetActorLocation(), Preview.Location) > FMath::Square(250.0f)) return false;

    if (FKalmalaToolProgressionContract::IsStationAttachmentKit(BuildableId))
    {
        int32 ActiveAttachmentCount = 0;
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
        {
            const AKalmalaConstructionActor* Existing = *It;
            if (IsValid(Existing) && Existing->GetWorld() == GetWorld() && !Existing->GetConstructionId().IsEmpty()
                && FKalmalaToolProgressionContract::IsStationAttachmentKit(Existing->GetConstructionKit()))
            {
                ++ActiveAttachmentCount;
            }
        }
        Reason = FString::Printf(TEXT("Session station attachment limit reached (%d)"),
            FKalmalaToolProgressionContract::MaxStationAttachments);
        if (ActiveAttachmentCount >= FKalmalaToolProgressionContract::MaxStationAttachments) return false;

        const FName RequiredStationKit = FKalmalaToolProgressionContract::GetAttachmentStationKit(BuildableId);
        AKalmalaConstructionActor* NearbyStation = FindNearbyToolProgressionStation(RequiredStationKit);
        if (!NearbyStation)
        {
            return FKalmalaToolProgressionContract::CanPlaceAttachment(
                BuildableId, RequiredStationKit, 0.0f, false, false, Reason);
        }
        const float StationDistance = FVector::Distance(Preview.Location, NearbyStation->GetActorLocation());
        const bool bAlreadyUpgraded = FKalmalaToolProgressionContract::GetEffectiveStationLevel(NearbyStation)
            > FKalmalaToolProgressionContract::GetBaseStationLevel(RequiredStationKit);
        if (!FKalmalaToolProgressionContract::CanPlaceAttachment(
            BuildableId, NearbyStation ? NearbyStation->GetConstructionKit() : NAME_None,
            StationDistance, true, bAlreadyUpgraded, Reason)) return false;
    }
    if (BuildableId == TEXT("DryingLineKit"))
    {
        int32 ActiveDryingLines = 0;
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
        {
            const AKalmalaConstructionActor* Existing = *It;
            if (IsValid(Existing) && Existing->GetWorld() == GetWorld()
                && Existing->GetConstructionKit() == TEXT("DryingLineKit")
                && !Existing->GetConstructionId().IsEmpty())
            {
                ++ActiveDryingLines;
            }
        }
        Reason = FString::Printf(TEXT("Session Drying Line limit reached (%d)"),
            AKalmalaConstructionActor::MaxSessionDryingLines);
        if (ActiveDryingLines >= AKalmalaConstructionActor::MaxSessionDryingLines) return false;
    }

    const FRotator Rotation(0.0f, Character->GetActorRotation().Yaw, 0.0f);
    if (Rotation.ContainsNaN()) { Reason = TEXT("Invalid placement rotation"); return false; }
    auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
    TArray<FKalmalaInventoryStack> Cost;
    if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(BuildableId))
    {
        if (!UKalmalaRecipeCatalogue::BuildDirectMaterialCost(BuildableId, Cost, Reason)) return false;
    }
    else
    {
        Cost = {{BuildableId, 1}};
    }
    TArray<FKalmalaInventoryStack> Scratch;
    TArray<FKalmalaInventoryStack> InventoryBefore;
    if (Inventory) InventoryBefore = Inventory->GetStacks();
    if (!Inventory || !UKalmalaInventoryComponent::BuildExchange(Inventory->GetStacks(), Cost, NAME_None, 0, Scratch, Reason)) return false;
    const FTransform Transform(Rotation, Preview.Location);
    auto* GameMode = GetWorld()->GetAuthGameMode<AKalmalaGameMode>();
    const bool bTransientAttachment = FKalmalaToolProgressionContract::IsStationAttachmentKit(BuildableId);
    const bool bSessionOnlyKit = FKalmalaPlacementPreview::IsSessionOnlyKit(BuildableId);
    if (!GameMode || (!bTransientAttachment && !bSessionOnlyKit && !GameMode->CanPersistConstruction(BuildableId, Transform)))
    {
        Reason = TEXT("Construction save limit reached or unavailable");
        return false;
    }
    auto* Construction = GetWorld()->SpawnActorDeferred<AKalmalaConstructionActor>(AKalmalaConstructionActor::StaticClass(), Transform, nullptr, Character,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Construction) { Reason = TEXT("Could not allocate construction"); return false; }
    if (!Inventory->TryExchangeFromServer(Cost, NAME_None, 0, Reason)) { Construction->Destroy(); return false; }
    Construction->InitializeFromServer(BuildableId, FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
    Construction->FinishSpawning(Transform);
    if (!bTransientAttachment && !bSessionOnlyKit && !GameMode->PersistConstruction(Construction))
    {
        Construction->Destroy();
        Inventory->TryCommitStacksFromServer(Scratch, InventoryBefore);
        Reason = TEXT("Could not save construction; materials restored");
        return false;
    }
    Reason = UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(BuildableId)
        ? TEXT("Built directly from Wood and Fibre with the construction hammer")
        : bTransientAttachment
        ? TEXT("Placed the paid station attachment; its level bonus lasts for this session")
        : BuildableId == TEXT("DryingLineKit")
        ? TEXT("Placed the Drying Line; it lasts for this server session")
        : TEXT("Placed construction; server accepted the buildable and ground");
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

void UKalmalaCraftingComponent::ServerPlaceConstruction_Implementation(const FName BuildableId)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool Accepted = PlaceConstructionFromServer(BuildableId, Reason); PublishResult(Reason, Accepted);
}

void UKalmalaCraftingComponent::ServerRefuel_Implementation()
{
    if (!AcceptRequest()) return;
    auto* Fire = FindNearbyFire(true);
    const bool bAccepted = Fire != nullptr && Fire->TryRefuelFromServer(GetCharacter());
    PublishResult(bAccepted ? TEXT("Added one raw fuel item (60 seconds)")
        : TEXT("Need a usable nearby hearth, one Wood, Lightwood, Densewood, or Coal, and 60 seconds of free capacity"), bAccepted);
}

void UKalmalaCraftingComponent::ServerLight_Implementation()
{
    if (!AcceptRequest()) return;
    auto* Fire = FindNearbyFire(true);
    const bool bAccepted = Fire && Fire->CanInteract_Implementation(GetCharacter());
    if (bAccepted) Fire->Interact_Implementation(GetCharacter());
    PublishResult(bAccepted ? TEXT("Hearth lit") : TEXT("Need a usable nearby unlit hearth with dry fuel"), bAccepted);
}

bool UKalmalaCraftingComponent::RepairToolFromServer(const FName ToolId, FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;
    const bool bAtRepairStation = FindNearbyWorkbench() != nullptr
        || FindNearbyToolProgressionStation(TEXT("ForgeKit")) != nullptr;
    if (!bAtRepairStation)
    {
        Reason = TEXT("Need a visible same-world Workbench or Forge within 2.5 m");
        return false;
    }

    int32* CurrentDurability = Character->FindToolDurabilityFromServer(ToolId);
    if (CurrentDurability == nullptr) { Reason = TEXT("Unknown tool"); return false; }
    FKalmalaToolState CurrentState;
    CurrentState.ToolId = ToolId;
    CurrentState.Durability = *CurrentDurability;
    CurrentState.ToolLevel = Character->GetCarriedToolLevel(ToolId);
    FKalmalaToolState RepairedState;
    if (!FKalmalaToolLifecycleContract::BuildServerFreeRepair(true, true, CurrentState, RepairedState))
    {
        Reason = TEXT("Tool is already at full condition or has invalid condition");
        return false;
    }

    *CurrentDurability = RepairedState.Durability;
    Character->ForceNetUpdate();
    Reason = TEXT("Repaired selected tool to full condition at no cost");
    return true;
}

bool UKalmalaCraftingComponent::RepairAllToolsFromServer(
    AKalmalaConstructionActor* GrindingStone,
    FString& Reason)
{
    auto* Character = GetCharacter();
    Reason = TEXT("Server authority required");
    if (!Character || !Character->HasAuthority() || !Character->GetController()) return false;

    const bool bAtGrindingStone = IsValid(GrindingStone)
        && GrindingStone->GetWorld() == Character->GetWorld()
        && GrindingStone->HasAuthority()
        && GrindingStone->GetConstructionKit() == TEXT("GrindingStoneKit")
        && GrindingStone->CanInteract_Implementation(Character);
    Reason = TEXT("Need to interact with a visible same-world Grinding Stone within 2.5 m");
    if (!bAtGrindingStone) return false;

    TArray<FKalmalaToolState> CandidateTools;
    int32 RepairedCount = 0;
    if (!FKalmalaToolLifecycleContract::BuildServerRepairAll(
        true, true, Character->CarriedTools, CandidateTools, RepairedCount))
    {
        Reason = TEXT("Carried tool state is invalid; no tools were changed");
        return false;
    }
    if (RepairedCount == 0)
    {
        Reason = TEXT("All carried tools are already at full condition");
        return true;
    }

    Character->CarriedTools = MoveTemp(CandidateTools);
    Character->ForceNetUpdate();
    Reason = FString::Printf(TEXT("Grinding Stone repaired %d carried tool%s to full condition at no cost"),
        RepairedCount, RepairedCount == 1 ? TEXT("") : TEXT("s"));
    return true;
}

void UKalmalaCraftingComponent::ServerRepairTool_Implementation(const FName ToolId)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = RepairToolFromServer(ToolId, Reason);
    PublishResult(Reason, bAccepted);
}

FString UKalmalaCraftingComponent::GetRecipeAvailability(FName Id) const
{
    const auto* R = UKalmalaRecipeCatalogue::Get()->Find(Id);
    if (!R || !R->bEnabled) return TEXT("Recipe unavailable");
    auto* Character = GetCharacter();
    if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(R->Output))
    {
        if (!Character || Character->GetCarriedToolLevel(TEXT("ConstructionHammer")) < 1)
            return TEXT("Need your carried Construction Hammer");
        const auto* Inventory = Character->FindComponentByClass<UKalmalaInventoryComponent>();
        if (!Inventory) return TEXT("Waiting for pack");
        TArray<FKalmalaInventoryStack> Costs;
        FString Reason;
        if (!UKalmalaRecipeCatalogue::BuildDirectMaterialCost(R->Output, Costs, Reason)) return Reason;
        if (R->Output == TEXT("CampfireKit")
            && !FKalmalaRawFuelContract::AddCosts(Inventory->GetStacks(), 1, Costs, Reason)) return Reason;
        TArray<FKalmalaInventoryStack> Candidate;
        UKalmalaInventoryComponent::BuildExchange(Inventory->GetStacks(), Costs, NAME_None, 0, Candidate, Reason);
        return Reason;
    }
    if (FKalmalaToolProgressionContract::IsStationAttachmentKit(R->Output))
    {
        const FName StationKit = FKalmalaToolProgressionContract::GetAttachmentStationKit(R->Output);
        if (!FindNearbyToolProgressionStation(StationKit))
        {
            const TCHAR* StationName = StationKit == TEXT("WorkbenchKit") ? TEXT("Workbench") : TEXT("Forge");
            return FString::Printf(TEXT("Need a visible same-world %s within 2.5 m"), StationName);
        }
    }
    const AKalmalaConstructionActor* RequiredStation = R->RequiredStation.IsEmpty()
        ? nullptr : FindNearbyConstruction(R->RequiredStation);
    if (!R->RequiredStation.IsEmpty() && !RequiredStation)
    {
        return FString::Printf(TEXT("Need a visible same-world %s within 2.5 m"),
            *GetRecipeStationNames(*R));
    }
    if (IsFoodProcessingStation(RequiredStation) && !FindNearbyLitFire(RequiredStation))
    {
        return GetMissingFoodHeatReason(RequiredStation);
    }
    auto* C = Character; auto* Inv = C ? C->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Inv) return TEXT("Waiting for pack");
    if (!UKalmalaRecipeCatalogue::HasRequiredTool(R->RequiredTool, Inv->GetStacks()))
        return FString::Printf(TEXT("Need a %s in your pack"), *GetRecipeToolDisplayName(R->RequiredTool));
    TArray<FKalmalaInventoryStack> Costs; int32 OutputCount = 0;
    FString Reason;
    if (!UKalmalaRecipeCatalogue::Scale(*R, 1, Costs, OutputCount)) return TEXT("Recipe material list is invalid");
    TArray<FKalmalaInventoryStack> Next;
    UKalmalaInventoryComponent::BuildExchange(Inv->GetStacks(), Costs, R->Output, OutputCount, Next, Reason);
    return Reason;
}

FString UKalmalaCraftingComponent::GetToolProgressionText() const
{
    const AKalmalaCharacter* Character = GetCharacter();
    const UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
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

        if (Entry.RequiredSkill != EKalmalaSkill::None)
        {
            const TCHAR* SkillName = Entry.RequiredSkill == EKalmalaSkill::Crafting
                ? TEXT("Crafting") : TEXT("Skill");
            Text += FString::Printf(TEXT("Requires %s level %d (second-tier unlock). "),
                SkillName, Entry.RequiredSkillLevel);
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
            const int32 Level = FKalmalaToolProgressionContract::GetEffectiveStationLevel(Station);
            Text += FString::Printf(TEXT(". Nearby %s level %d; required level %d"),
                StationName, Level, Entry.RequiredStationLevel);
        }
        else
        {
            Text += FString::Printf(TEXT(". Need a visible same-world %s level %d within 2.5 m"),
                StationName, Entry.RequiredStationLevel);
        }
        Text += TEXT(".\n");
    }
    Text += TEXT("Workbench and Forge bases are level 1. A nearby paid tool rack or anvil adds one level, up to level 2. Attachments last only for this session until M9 persistence is approved; the server checks placement and station level.");
    return Text;
}

FString UKalmalaCraftingComponent::GetRecipeDescription(FName Id) const
{
    const auto* R = UKalmalaRecipeCatalogue::Get()->Find(Id);
    if (!R) return TEXT("Unknown recipe");
    if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(R->Output))
    {
        FString Text = R->DisplayName + TEXT("\nBuild directly with the Construction Hammer; no kit is created.\nRaw material cost: ");
        TArray<FKalmalaInventoryStack> Costs;
        FString Failure;
        if (!UKalmalaRecipeCatalogue::BuildDirectMaterialCost(R->Output, Costs, Failure)) return Failure;
        for (int32 Index = 0; Index < Costs.Num(); ++Index)
        {
            const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Costs[Index].ItemId);
            Text += FString::Printf(TEXT("%s%d %s"), Index ? TEXT(", ") : TEXT(""), Costs[Index].Quantity,
                Item ? *Item->DisplayName : *Costs[Index].ItemId.ToString());
        }
        if (R->Output == TEXT("CampfireKit"))
            Text += TEXT("\nIgnition: one raw Wood, Lightwood, Densewood, or Coal is also consumed to start the hearth with 60 seconds of fuel.");
        const auto* BuildItem = UKalmalaItemCatalogue::Get()->FindItem(R->Output);
        Text += FString::Printf(TEXT("\nOutput: %s construction (no kit item created)."),
            BuildItem ? *BuildItem->DisplayName : *R->Output.ToString());
        if (BuildItem) Text += TEXT("\nDescription: ") + BuildItem->Description;
        Text += R->Output == TEXT("CampfireKit")
            ? TEXT("\nBuild quantity: one hearth per request. Placement: clear, dry, gently sloping ground ahead. The server rechecks terrain, slope, water, overlap, range, payment, and the session limit. Failure: the availability text below names missing materials.")
            : TEXT("\nBuild quantity: one construction per request; repeat to build another.\nPlacement: clear, dry, gently sloping ground. The server rechecks terrain, slope, overlap, range, payment, and save identity. Failure: the availability text below names missing materials.");
        return Text;
    }
    FString Text = R->DisplayName + TEXT("\nCost: ");
    for (const auto& Cost : R->Ingredients)
    {
        const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Cost.ItemId);
        Text += FString::Printf(TEXT("%d %s  "), Cost.Quantity,
            Item ? *Item->DisplayName : *Cost.ItemId.ToString());
    }
    Text += FString::Printf(TEXT("\nCost list is for batch 1; larger batches multiply each listed quantity.\nMaximum batch: up to %d per request; this panel submits batch 1 per press."),
        R->MaxBatch);
    const auto* OutputItem = UKalmalaItemCatalogue::Get()->FindItem(R->Output);
    Text += FString::Printf(TEXT("\nOutput: %d %s (stack limit %d per inventory stack)"),
        R->OutputCount, OutputItem ? *OutputItem->DisplayName : *R->Output.ToString(),
        OutputItem ? OutputItem->MaxStack : 0);
    if (OutputItem) Text += TEXT("\nDescription: ") + OutputItem->Description;
    if (!R->RequiredTool.IsNone())
        Text += FString::Printf(TEXT("\nTool: carry a %s in your pack; it is reusable and not consumed."),
            *GetRecipeToolDisplayName(R->RequiredTool));
    if (R->Output == TEXT("DryingLineKit"))
        Text += TEXT("\nPlacement: server-authoritative and limited to five lines per session; the line is not saved until M9 migration is approved.");
    if (R->Output == TEXT("DriedFieldMeat"))
        Text += TEXT("\nProcessing: no hearth or raw fuel is required.");
    if (FKalmalaToolProgressionContract::IsStationAttachmentKit(R->Output))
    {
        const FName StationKit = FKalmalaToolProgressionContract::GetAttachmentStationKit(R->Output);
        const TCHAR* StationName = StationKit == TEXT("WorkbenchKit") ? TEXT("Workbench") : TEXT("Forge");
        Text += FString::Printf(TEXT("\nStation: craft at a visible same-world %s within 2.5 m.\nPlacement: place within 1.25 m of that station; the level bonus lasts for this session until M9 save migration is approved."), StationName);
        return Text;
    }
    const AKalmalaConstructionActor* RequiredStation = R->RequiredStation.IsEmpty()
        ? nullptr : FindNearbyConstruction(R->RequiredStation);
    if (!R->RequiredStation.IsEmpty())
    {
        const FString StationName = GetRecipeStationNames(*R);
        Text += FString::Printf(TEXT("\nStation: visible same-world %s within 2.5 m"), *StationName);
        if (IsFoodProcessingStation(RequiredStation))
            Text += FString::Printf(TEXT("\nHeat: a usable lit hearth with positive heat must be within 2.5 m of both the player and %s. Its fuel burns at the normal rate while lit; the recipe adds no fuel cost."), *StationName);
    }
    else if (R->RequiredTool.IsNone())
        Text += TEXT("\nHandcrafted; no station");

    Text += TEXT("\nFailure: the availability text below names the first unmet requirement. Rejected requests preserve ingredients and tool condition.");
    return Text;
}

FString UKalmalaCraftingComponent::GetFoodText() const
{
    const auto* Character = GetCharacter();
    const auto* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    const auto* Status = Character ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    const int32 RoastCount = Inventory ? Inventory->GetQuantity(UKalmalaPlayerStatusComponent::RoastedFieldMeatItemId) : 0;
    const int32 BrothCount = Inventory ? Inventory->GetQuantity(UKalmalaPlayerStatusComponent::HearthBrothItemId) : 0;
    const int32 SmokedCount = Inventory ? Inventory->GetQuantity(UKalmalaPlayerStatusComponent::SmokedFieldMeatItemId) : 0;
    const int32 DriedCount = Inventory ? Inventory->GetQuantity(UKalmalaPlayerStatusComponent::DriedFieldMeatItemId) : 0;
    const float Remaining = Status ? Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId) : 0.0f;
    if (Remaining > 0.0f)
    {
        return FString::Printf(TEXT("Steady meal: %.0f seconds remaining; stamina use is 10%% lower. Wait for expiry; food effects cannot stack or replace this meal.\nAvailable: Roasted field meat %d; Hearth broth %d; Smoked field meat %d; Dried field meat %d."),
            Remaining, RoastCount, BrothCount, SmokedCount, DriedCount);
    }
    return FString::Printf(TEXT("Roasted field meat: %d available. Hearth broth: %d available. Smoked field meat: %d available. Dried field meat: %d available. Food is optional. Eat one for 120 seconds of 10%% lower stamina use; only one meal can be active."),
        RoastCount, BrothCount, SmokedCount, DriedCount);
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

    const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(FoodItemId);
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
