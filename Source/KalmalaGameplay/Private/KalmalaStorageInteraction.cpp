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
    return FindNearbyWorkbench() ? TEXT("Joiner's bench: ready for floor, wall and roof assembly") : TEXT("No visible workbench within 2.5 m");
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
    const auto* Item = UKalmalaItemCatalogue::Get()->FindItem(Closest->GetConstructionKit());
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
    if (AKalmalaConstructionActor::IsCraftingStationKit(Kit))
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
        PublishResult(TEXT("Cooking rack opened"), true);
    else if (Kit == TEXT("CauldronKit"))
        PublishResult(TEXT("Cauldron opened"), true);
    else if (Kit == TEXT("FryingPanKit"))
        PublishResult(TEXT("Frying pan opened"), true);
    else if (Construction->GetConstructionKit() == TEXT("SmokeFrameKit"))
        PublishResult(TEXT("Smoke frame ready; use Camp crafting with a lit hearth and one extra raw fuel item per serving"), true);
    else PublishResult(TEXT("Joiner's bench ready; use the Construction Hammer menu to build floors, walls, and roofs from Wood and Fibre"), true);
}

bool UKalmalaCraftingComponent::IsStationContextTargetCurrent(AKalmalaConstructionActor* ExpectedActor,
    const FName ExpectedKit, const FString& ExpectedConstructionId) const
{
    const AKalmalaCharacter* Character = GetCharacter();
    const bool bStorageContext = AKalmalaConstructionActor::IsStorageKit(ExpectedKit);
    if (!Character || !Character->GetController() || !IsValid(ExpectedActor)
        || ExpectedActor != LastStationContextActor.Get() || ExpectedKit != LastStationContextKit
        || ExpectedConstructionId.IsEmpty() || ExpectedConstructionId != LastStationContextConstructionId
        || (!AKalmalaConstructionActor::IsCraftingStationKit(ExpectedKit) && !bStorageContext)
        || (bStorageContext && !bStorageViewOpen)) return false;
    const FVector CharacterLocation = Character->GetActorLocation();
    const FVector StationLocation = ExpectedActor->GetActorLocation();
    const float Range = Character->GetInteractionRange();
    return Character->GetWorld() == ExpectedActor->GetWorld()
        && !CharacterLocation.ContainsNaN() && !StationLocation.ContainsNaN()
        && FMath::IsFinite(Range) && Range > 0.0f
        && FVector::DistSquared(CharacterLocation, StationLocation) <= FMath::Square(Range);
}

bool UKalmalaCraftingComponent::IsStationContextUsable(AKalmalaConstructionActor* ExpectedActor,
    const FName ExpectedKit, const FString& ExpectedConstructionId) const
{
    const AKalmalaCharacter* Character = GetCharacter();
    return Character && IsStationContextTargetCurrent(ExpectedActor, ExpectedKit, ExpectedConstructionId)
        && ExpectedActor->CanUse(Character);
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
    const bool bAccepted = OpenStorageFromServer(FindNearbyConstruction(TEXT("StorageKit")));
    PublishResult(bAccepted ? TEXT("Nearby chest inspected") : TEXT("Need a registered, visible chest within 2.5 m"), bAccepted);
}
void UKalmalaCraftingComponent::ServerCloseStorage_Implementation() { ClearStorageView(); }
void UKalmalaCraftingComponent::ServerDepositStorage_Implementation(FName ItemId)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool bAccepted = TransferStorageFromServer(ItemId, true, Reason); PublishResult(Reason, bAccepted);
}
void UKalmalaCraftingComponent::ServerWithdrawStorage_Implementation(FName ItemId)
{
    if (!AcceptRequest()) return;
    FString Reason; const bool bAccepted = TransferStorageFromServer(ItemId, false, Reason); PublishResult(Reason, bAccepted);
}
