#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerModelComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AKalmalaConstructionActor* UKalmalaCraftingComponent::FindNearbyConstruction(FName Kit, FName AlternateKit) const
{
    const auto* Character = GetCharacter();
    if (!Character || !GetWorld()) return nullptr;
    AKalmalaConstructionActor* Closest = nullptr;
    double Best = FMath::Square(250.0);
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
    {
        if (!IsValid(*It)) continue;
        const bool bMatchesKit = Kit == TEXT("StorageKit")
            ? AKalmalaConstructionActor::IsStorageKit(It->GetConstructionKit())
            : It->GetConstructionKit() == Kit;
        const bool bMatchesAlternateKit = !AlternateKit.IsNone() && It->GetConstructionKit() == AlternateKit;
        if ((!bMatchesKit && !bMatchesAlternateKit) || !It->CanUse(Character)) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation());
        if (Distance < Best || (Distance == Best && (!Closest || It->GetConstructionId() < Closest->GetConstructionId())))
        { Best = Distance; Closest = *It; }
    }
    return Closest;
}

AKalmalaConstructionActor* UKalmalaCraftingComponent::FindNearbyConstruction(const TArray<FName>& RequiredStations) const
{
    const AKalmalaCharacter* Character = GetCharacter();
    if (!Character) return nullptr;
    AKalmalaConstructionActor* Closest = nullptr;
    double Best = FMath::Square(250.0);
    for (const FName Station : RequiredStations)
    {
        AKalmalaConstructionActor* Candidate = FindNearbyConstruction(Station);
        if (!Candidate) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), Candidate->GetActorLocation());
        if (Distance < Best || (Distance == Best && (!Closest || Candidate->GetConstructionId() < Closest->GetConstructionId())))
        {
            Best = Distance;
            Closest = Candidate;
        }
    }
    return Closest;
}

AKalmalaConstructionActor* UKalmalaCraftingComponent::FindNearbyWorkbench() const { return FindNearbyConstruction(TEXT("WorkbenchKit")); }

FString UKalmalaCraftingComponent::GetNearbyWorkbenchText() const
{
    return FindNearbyWorkbench() ? TEXT("Workbench: ready for floor, wall and roof assembly") : TEXT("No visible workbench within 2.5 m");
}

FString UKalmalaCraftingComponent::GetNearbyConstructionText() const
{
    const auto* Character = GetCharacter();
    if (!Character || !GetWorld()) return TEXT("Construction: none visible within 2.5 m");
    const AKalmalaConstructionActor* Closest = nullptr;
    double Best = FMath::Square(250.0);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ConstructionFeedback), false, Character);
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
    {
        if (!IsValid(*It) || It->GetConstructionId().IsEmpty()) continue;
        const double Distance = FVector::DistSquared(Character->GetActorLocation(), It->GetActorLocation());
        if (!FMath::IsFinite(Distance) || Distance > Best) continue;
        FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), It->GetActorLocation(), ECC_Visibility, Query)
            || Hit.GetActor() != *It) continue;
        if (Distance == Best && Closest && It->GetConstructionId() >= Closest->GetConstructionId()) continue;
        Best = Distance; Closest = *It;
    }
    if (!Closest) return TEXT("Construction: none visible within 2.5 m");
    const auto* Item = UKalmalaItemCatalogue::Get()->FindBuildable(Closest->GetConstructionKit());
    const bool bRoof = Closest->GetConstructionKit() == TEXT("RoofKit");
    return FString::Printf(TEXT("Construction: %s\nHealth: %.1f / %.0f\n%s"),
        Item ? *Item->DisplayName : TEXT("Structure"), Closest->GetHealth(), AKalmalaConstructionActor::MaximumHealth,
        bRoof ? TEXT("Rain-immune roof") : Closest->GetHealth() <= AKalmalaConstructionActor::RainHealthFloor
            ? TEXT("Rain-wear limit reached (50%)\nAn overhead roof prevents further rain wear")
            : Closest->GetHealth() < AKalmalaConstructionActor::MaximumHealth
                ? TEXT("Rain-worn - minimum health from rain: 50%\nAn overhead roof prevents further rain wear")
                : TEXT("No rain wear - minimum health from rain: 50%\nAn overhead roof prevents rain wear"));
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
    if (!IsValid(Construction) || !AKalmalaConstructionActor::IsStorageKit(Construction->GetConstructionKit())
        || !Construction->CanInteract_Implementation(Character)) return false;
    ActiveStorage = Construction; RefreshStorageView(); Character->ForceNetUpdate();
    return bStorageViewOpen;
}

void UKalmalaCraftingComponent::InteractWithConstructionFromServer(AKalmalaConstructionActor* Construction)
{
    if (!AcceptRequest() || !IsValid(Construction) || !Construction->CanInteract_Implementation(GetCharacter())) return;
    const FName Kit = Construction->GetConstructionKit();
    if (Kit == TEXT("TimberDoorKit"))
    {
        FString Reason;
        const bool bAccepted = Construction->ToggleDoorFromServer(GetCharacter(), Reason);
        PublishResult(Reason, bAccepted);
        return;
    }
    if (Kit == TEXT("CampBedKit"))
    {
        PublishResult(TEXT("Resting briefly; movement or damage will end the pose."), true);
        return;
    }
    if (AKalmalaConstructionActor::IsCraftingStationKit(Kit))
    {
        LastStationContextActor = Construction;
        LastStationContextKit = Kit;
        LastStationContextConstructionId = Construction->GetConstructionId();
        StationContextInteractionSerial = StationContextInteractionSerial == TNumericLimits<uint32>::Max()
            ? 1 : StationContextInteractionSerial + 1;
        GetOwner()->ForceNetUpdate();
    }
    else if (Kit == TEXT("GardenPlotKit"))
    {
        LastStationContextActor = Construction;
        LastStationContextKit = Kit;
        LastStationContextConstructionId = Construction->GetConstructionId();
        StationContextInteractionSerial = StationContextInteractionSerial == TNumericLimits<uint32>::Max()
            ? 1 : StationContextInteractionSerial + 1;
        GetOwner()->ForceNetUpdate();
    }
    if (Kit == TEXT("CookingRackKit") || Kit == TEXT("CauldronKit") || Kit == TEXT("FryingPanKit"))
    {
        LastInteractedCookingStationKit = Kit;
        CookingStationInteractionSerial = CookingStationInteractionSerial == TNumericLimits<uint32>::Max()
            ? 1 : CookingStationInteractionSerial + 1;
        GetOwner()->ForceNetUpdate();
    }
    if (Construction->GetConstructionKit() == TEXT("GrindingStoneKit"))
    {
        FString Reason;
        const bool bAccepted = RepairAllToolsFromServer(Construction, Reason);
        if (bAccepted)
        {
            if (auto* Character = GetCharacter())
            {
                if (Character->HasAuthority())
                {
                    if (auto* PlayerModel = Character->FindComponentByClass<UKalmalaPlayerModelComponent>())
                        PlayerModel->MulticastPlayGrindingStoneSharpening();
                }
            }
        }
        PublishResult(Reason, bAccepted);
    }
    else if (AKalmalaConstructionActor::IsStorageKit(Construction->GetConstructionKit()))
    {
        const bool bAccepted = OpenStorageFromServer(Construction);
        if (bAccepted)
        {
            LastStationContextActor = Construction;
            LastStationContextKit = Kit;
            LastStationContextConstructionId = Construction->GetConstructionId();
            StationContextInteractionSerial = StationContextInteractionSerial == TNumericLimits<uint32>::Max()
                ? 1 : StationContextInteractionSerial + 1;
            GetOwner()->ForceNetUpdate();
        }
        PublishResult(bAccepted
            ? TEXT("Chest storage opened")
            : TEXT("Storage unavailable"), bAccepted);
    }
    else if (Kit == TEXT("CookingRackKit"))
        PublishResult(TEXT("Cooking Rack opened"), true);
    else if (Kit == TEXT("CauldronKit"))
        PublishResult(TEXT("Cauldron opened"), true);
    else if (Kit == TEXT("FryingPanKit"))
        PublishResult(TEXT("Frying Pan opened"), true);
    else if (Kit == TEXT("CharcoalKilnKit"))
        PublishResult(TEXT("Charcoal Kiln ready; load Wood and collect Coal from its processing panel"), true);
    else if (Kit == TEXT("SmelterKit"))
        PublishResult(TEXT("Smelter ready; load ore and Coal, then collect refined bars from its processing panel"), true);
    else if (Construction->GetConstructionKit() == TEXT("SmokeFrameKit"))
        PublishResult(TEXT("Smoke frame ready; use Camp crafting with a lit hearth and one extra raw fuel item per serving"), true);
    else PublishResult(TEXT("Workbench ready; use the Construction Hammer menu to build floors, walls, and roofs from Wood"), true);
}

bool UKalmalaCraftingComponent::IsStationContextTargetCurrent(AKalmalaConstructionActor* ExpectedActor,
    const FName ExpectedKit, const FString& ExpectedConstructionId) const
{
    const AKalmalaCharacter* Character = GetCharacter();
    const bool bStorageContext = AKalmalaConstructionActor::IsStorageKit(ExpectedKit);
    const bool bCultivationContext = ExpectedKit == TEXT("GardenPlotKit");
    if (!Character || !Character->GetController() || !IsValid(ExpectedActor)
        || ExpectedActor != LastStationContextActor.Get() || ExpectedKit != LastStationContextKit
        || ExpectedConstructionId.IsEmpty() || ExpectedConstructionId != LastStationContextConstructionId
        || (!AKalmalaConstructionActor::IsCraftingStationKit(ExpectedKit) && !bStorageContext && !bCultivationContext)
        || (bStorageContext && !bStorageViewOpen)) return false;
    const FVector CharacterLocation = Character->GetActorLocation();
    const FVector StationLocation = ExpectedActor->GetActorLocation();
    const float Range = Character->GetInteractionRange();
    const bool bTargetUsable = !bCultivationContext
        || (ExpectedActor->GetConstructionKit() == ExpectedKit && ExpectedActor->CanUse(Character)
            && ExpectedActor->HasUsablePlantingTrowel(Character));
    return Character->GetWorld() == ExpectedActor->GetWorld()
        && !CharacterLocation.ContainsNaN() && !StationLocation.ContainsNaN()
        && FMath::IsFinite(Range) && Range > 0.0f
        && FVector::DistSquared(CharacterLocation, StationLocation) <= FMath::Square(Range)
        && bTargetUsable;
}

bool UKalmalaCraftingComponent::IsStationContextUsable(AKalmalaConstructionActor* ExpectedActor,
    const FName ExpectedKit, const FString& ExpectedConstructionId) const
{
    const AKalmalaCharacter* Character = GetCharacter();
    return Character && IsStationContextTargetCurrent(ExpectedActor, ExpectedKit, ExpectedConstructionId)
        && ExpectedActor->CanUse(Character)
        && (ExpectedKit != TEXT("GardenPlotKit") || ExpectedActor->HasUsablePlantingTrowel(Character));
}

bool UKalmalaCraftingComponent::TransferStorageFromServer(const FGuid ItemGuid, const bool bDeposit, FString& Reason)
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
    const bool Accepted = Pack->TransferStorageFromServer(StorageView, ItemGuid, bDeposit,
        [&](const auto& Next) { return Mode->PersistStorage(Chest, Next); }, Reason);
    RefreshStorageView(); Character->ForceNetUpdate();
    return Accepted;
}

void UKalmalaCraftingComponent::ServerOpenStorage_Implementation()
{
    if (!AcceptRequest()) return;
    const bool bAccepted = OpenStorageFromServer(FindNearbyConstruction(TEXT("StorageKit")));
    PublishResult(bAccepted ? TEXT("Nearby chest inspected") : TEXT("Need a registered, visible chest within 2.5 m"), bAccepted);
}
void UKalmalaCraftingComponent::ServerCloseStorage_Implementation() { ClearStorageView(); }
void UKalmalaCraftingComponent::ServerDepositStorage_Implementation(FGuid ItemGuid)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool bAccepted = TransferStorageFromServer(ItemGuid, true, Reason); PublishResult(Reason, bAccepted);
}
void UKalmalaCraftingComponent::ServerWithdrawStorage_Implementation(FGuid ItemGuid)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool bAccepted = TransferStorageFromServer(ItemGuid, false, Reason); PublishResult(Reason, bAccepted);
}

AKalmalaConstructionActor* UKalmalaCraftingComponent::GetCurrentCharcoalKilnFromServer() const
{
    AKalmalaCharacter* Character = GetCharacter();
    AKalmalaConstructionActor* Kiln = LastStationContextActor.Get();
    if (!Character || !Character->HasAuthority() || !Character->GetController()
        || LastStationContextKit != TEXT("CharcoalKilnKit") || !Kiln
        || Kiln->GetConstructionKit() != TEXT("CharcoalKilnKit")
        || Kiln->GetConstructionId() != LastStationContextConstructionId
        || !IsStationContextUsable(Kiln, TEXT("CharcoalKilnKit"), LastStationContextConstructionId)
        || !Kiln->CanInteract_Implementation(Character)) return nullptr;
    return Kiln;
}

bool UKalmalaCraftingComponent::LoadCharcoalKilnWoodFromServer(FString& Reason)
{
    Reason = TEXT("Interact with a nearby Charcoal Kiln first");
    AKalmalaConstructionActor* Kiln = GetCurrentCharcoalKilnFromServer();
    AKalmalaCharacter* Character = GetCharacter();
    auto* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (!Kiln || !Character || !Inventory || !Mode) return false;

    FKalmalaCharcoalKilnState Candidate = Kiln->GetCharcoalKilnState();
    Reason = TEXT("Kiln input is full (10 Wood)");
    if (!Candidate.TryAddWood()) return false;
    Reason = TEXT("Need 1 Wood in your pack");
    if (!Inventory->TryConsumeFromServer(TEXT("Wood"), 1)) return false;
    if (!Mode->PersistCharcoalKilnState(Kiln, Candidate))
    {
        if (!Inventory->TryGrantFromServer(TEXT("Wood"), 1))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore Wood after Charcoal Kiln save rejection."));
        Reason = TEXT("Kiln could not be saved; no Wood was consumed");
        return false;
    }
    Kiln->ApplyCharcoalKilnStateFromServer(Candidate);
    Character->ForceNetUpdate();
    Reason = TEXT("Added 1 Wood to the Charcoal Kiln");
    return true;
}

bool UKalmalaCraftingComponent::CollectCharcoalKilnCoalFromServer(FString& Reason)
{
    Reason = TEXT("Interact with a nearby Charcoal Kiln first");
    AKalmalaConstructionActor* Kiln = GetCurrentCharcoalKilnFromServer();
    AKalmalaCharacter* Character = GetCharacter();
    auto* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (!Kiln || !Character || !Inventory || !Mode) return false;

    const FKalmalaCharcoalKilnState Previous = Kiln->GetCharcoalKilnState();
    FKalmalaCharcoalKilnState Candidate = Previous;
    Reason = TEXT("No Coal is ready to collect");
    if (!Candidate.TryTakeCoal()) return false;
    const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> After;
    if (!UKalmalaInventoryComponent::BuildGrant(Before, TEXT("Coal"), 1, After, Reason)) return false;
    if (!Mode->PersistCharcoalKilnState(Kiln, Candidate))
    {
        Reason = TEXT("Kiln could not be saved; Coal remains available");
        return false;
    }
    if (!Inventory->TryCommitStacksFromServer(Before, After))
    {
        if (!Mode->PersistCharcoalKilnState(Kiln, Previous))
            UE_LOG(LogTemp, Error, TEXT("Charcoal Kiln output rollback failed after inventory changed."));
        Reason = TEXT("Pack changed; Coal remains in the Kiln");
        return false;
    }
    Kiln->ApplyCharcoalKilnStateFromServer(Candidate);
    Character->ForceNetUpdate();
    Reason = TEXT("Collected 1 Coal");
    return true;
}

AKalmalaConstructionActor* UKalmalaCraftingComponent::GetCurrentSmelterFromServer() const
{
    AKalmalaCharacter* Character = GetCharacter();
    AKalmalaConstructionActor* Smelter = LastStationContextActor.Get();
    if (!Character || !Character->HasAuthority() || !Character->GetController()
        || LastStationContextKit != TEXT("SmelterKit") || !Smelter
        || Smelter->GetConstructionKit() != TEXT("SmelterKit")
        || Smelter->GetConstructionId() != LastStationContextConstructionId
        || !IsStationContextUsable(Smelter, TEXT("SmelterKit"), LastStationContextConstructionId)
        || !Smelter->CanInteract_Implementation(Character)) return nullptr;
    return Smelter;
}

bool UKalmalaCraftingComponent::LoadSmelterItemFromServer(const FName ItemId, FString& Reason)
{
    Reason = TEXT("Interact with a nearby Smelter first");
    AKalmalaConstructionActor* Smelter = GetCurrentSmelterFromServer();
    AKalmalaCharacter* Character = GetCharacter();
    auto* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (!Smelter || !Character || !Inventory || !Mode) return false;

    EKalmalaSmelterOre Ore = EKalmalaSmelterOre::None;
    if (ItemId == TEXT("CopperOre")) Ore = EKalmalaSmelterOre::Copper;
    else if (ItemId == TEXT("TinOre")) Ore = EKalmalaSmelterOre::Tin;
    else if (ItemId == TEXT("IronOre")) Ore = EKalmalaSmelterOre::Iron;
    else if (ItemId != TEXT("Coal"))
    {
        Reason = TEXT("Smelter accepts only Copper Ore, Tin Ore, Iron Ore, or Coal");
        return false;
    }

    FKalmalaSmelterState Candidate = Smelter->GetSmelterState();
    if (Ore == EKalmalaSmelterOre::None)
    {
        Reason = TEXT("Smelter Coal input is full (10 Coal)");
        if (!Candidate.TryAddCoal()) return false;
    }
    else
    {
        Reason = TEXT("Smelter ore input is full (10 ore)");
        if (!Candidate.TryAddOre(Ore)) return false;
    }
    Reason = FString::Printf(TEXT("Need 1 %s in your pack"), *ItemId.ToString());
    if (!Inventory->TryConsumeFromServer(ItemId, 1)) return false;
    if (!Mode->PersistSmelterState(Smelter, Candidate))
    {
        if (!Inventory->TryGrantFromServer(ItemId, 1))
            UE_LOG(LogTemp, Error, TEXT("Failed to restore %s after Smelter save rejection."), *ItemId.ToString());
        Reason = TEXT("Smelter could not be saved; no input was consumed");
        return false;
    }
    Smelter->ApplySmelterStateFromServer(Candidate);
    Character->ForceNetUpdate();
    Reason = FString::Printf(TEXT("Added 1 %s to the Smelter"), *ItemId.ToString());
    return true;
}

bool UKalmalaCraftingComponent::CollectSmelterItemFromServer(const FName ItemId, FString& Reason)
{
    Reason = TEXT("Interact with a nearby Smelter first");
    AKalmalaConstructionActor* Smelter = GetCurrentSmelterFromServer();
    AKalmalaCharacter* Character = GetCharacter();
    auto* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    auto* Mode = GetWorld() ? GetWorld()->GetAuthGameMode<AKalmalaGameMode>() : nullptr;
    if (!Smelter || !Character || !Inventory || !Mode) return false;

    const EKalmalaSmelterOre Ore = ItemId == TEXT("Copper") ? EKalmalaSmelterOre::Copper
        : ItemId == TEXT("Tin") ? EKalmalaSmelterOre::Tin
        : ItemId == TEXT("Iron") ? EKalmalaSmelterOre::Iron : EKalmalaSmelterOre::None;
    if (Ore == EKalmalaSmelterOre::None)
    {
        Reason = TEXT("Smelter output selection is invalid");
        return false;
    }

    const FKalmalaSmelterState Previous = Smelter->GetSmelterState();
    FKalmalaSmelterState Candidate = Previous;
    Reason = FString::Printf(TEXT("No %s bar is ready to collect"), *ItemId.ToString());
    if (!Candidate.TryTakeOutput(Ore)) return false;
    const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> After;
    if (!UKalmalaInventoryComponent::BuildGrant(Before, ItemId, 1, After, Reason)) return false;
    if (!Mode->PersistSmelterState(Smelter, Candidate))
    {
        Reason = TEXT("Smelter could not be saved; the bar remains available");
        return false;
    }
    if (!Inventory->TryCommitStacksFromServer(Before, After))
    {
        if (!Mode->PersistSmelterState(Smelter, Previous))
            UE_LOG(LogTemp, Error, TEXT("Smelter output rollback failed after inventory changed."));
        Reason = TEXT("Pack changed; the bar remains in the Smelter");
        return false;
    }
    Smelter->ApplySmelterStateFromServer(Candidate);
    Character->ForceNetUpdate();
    Reason = FString::Printf(TEXT("Collected 1 %s bar"), *ItemId.ToString());
    return true;
}

void UKalmalaCraftingComponent::ServerLoadCharcoalKilnWood_Implementation()
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = LoadCharcoalKilnWoodFromServer(Reason);
    PublishResult(Reason, bAccepted);
}

void UKalmalaCraftingComponent::ServerCollectCharcoalKilnCoal_Implementation()
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = CollectCharcoalKilnCoalFromServer(Reason);
    PublishResult(Reason, bAccepted);
}

void UKalmalaCraftingComponent::ServerLoadSmelterItem_Implementation(const FName ItemId)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = LoadSmelterItemFromServer(ItemId, Reason);
    PublishResult(Reason, bAccepted);
}

void UKalmalaCraftingComponent::ServerCollectSmelterItem_Implementation(const FName ItemId)
{
    if (!AcceptRequest()) return;
    FString Reason;
    const bool bAccepted = CollectSmelterItemFromServer(ItemId, Reason);
    PublishResult(Reason, bAccepted);
}
