#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Containers/Set.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR A8bPeatHeartStewRecipe[] = TEXT("PeatHeartStewRecipe");
    constexpr TCHAR A8bPeatHeartStewReadyResult[] = TEXT("A8b Peat Heart Stew peer ready");
    constexpr TCHAR A8bPeatHeartStewRackId[] = TEXT("A8bPeerPeatStewCauldron");

    const FKalmalaInventoryStack* FindPeatHeartStewStack(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory ? Inventory->GetStacks().FindByPredicate([](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == TEXT("PeatHeartStew");
        }) : nullptr;
    }

    AKalmalaConstructionActor* FindPeatHeartStewTestCauldron(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (It->GetConstructionId().StartsWith(FString(A8bPeatHeartStewRackId) + TEXT("_"))) return *It;
        }
        return nullptr;
    }

    bool GetPeatHeartStewOwnerId(const AKalmalaConstructionActor* Cauldron, int32& OutPlayerId)
    {
        if (!Cauldron) return false;
        const FString Prefix = FString(A8bPeatHeartStewRackId) + TEXT("_");
        if (!Cauldron->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Cauldron->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }

    bool HasUniquePeatHeartStewItemGuids(const FKalmalaInventoryStack* Stack)
    {
        if (!Stack || Stack->ItemGuids.Num() != Stack->Quantity) return false;
        TSet<FGuid> UniqueGuids;
        for (const FGuid& Guid : Stack->ItemGuids)
        {
            if (!Guid.IsValid() || UniqueGuids.Contains(Guid)) return false;
            UniqueGuids.Add(Guid);
        }
        return true;
    }
}

void UKalmalaCraftingComponent::RunA8bPeatHeartStewPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    UKalmalaPlayerStatusComponent* Status = Character
        ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !Status || !GetWorld()) return;
    A8bPeatHeartStewPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || A8bPeatHeartStewPeerStage >= 5) return;
        if (A8bPeatHeartStewPeerStage == 0)
        {
            if (A8bPeatHeartStewPeerElapsed < 3.0f) return;
            AKalmalaCharacter* Target = nullptr;
            int32 RemoteCount = 0;
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                const APlayerController* Controller = Cast<APlayerController>(It->GetController());
                if (!It->GetPlayerState() || !Controller || Controller->IsLocalController()) continue;
                ++RemoteCount;
                if (!Target || It->GetPlayerState()->GetPlayerId() < Target->GetPlayerState()->GetPlayerId())
                    Target = *It;
            }
            if (RemoteCount < 2 || !Target) return;

            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            UKalmalaPlayerStatusComponent* TargetStatus = Target->FindComponentByClass<UKalmalaPlayerStatusComponent>();
            if (!TargetInventory || !TargetCrafting || !TargetStatus
                || TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer FAILED: remote owner or meal baseline is invalid"));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }

            A8bPeatHeartStewPeerBaselinePeatHeart = TargetInventory->GetQuantity(TEXT("PeatHeart"));
            A8bPeatHeartStewPeerBaselinePotato = TargetInventory->GetQuantity(TEXT("Potato"));
            A8bPeatHeartStewPeerBaselineOnion = TargetInventory->GetQuantity(TEXT("Onion"));
            A8bPeatHeartStewPeerBaselineStew = TargetInventory->GetQuantity(TEXT("PeatHeartStew"));
            A8bPeatHeartStewPeerHostBaselinePeatHeart = Inventory->GetQuantity(TEXT("PeatHeart"));
            A8bPeatHeartStewPeerHostBaselinePotato = Inventory->GetQuantity(TEXT("Potato"));
            A8bPeatHeartStewPeerHostBaselineOnion = Inventory->GetQuantity(TEXT("Onion"));
            A8bPeatHeartStewPeerHostBaselineStew = Inventory->GetQuantity(TEXT("PeatHeartStew"));
            if (!TargetInventory->TryGrantFromServer(TEXT("PeatHeart"), 1)
                || !TargetInventory->TryGrantFromServer(TEXT("Potato"), 2)
                || !TargetInventory->TryGrantFromServer(TEXT("Onion"), 1)
                || !TargetInventory->TryGrantFromServer(TEXT("Wood"), 1))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer FAILED: recipe ingredients or hearth fuel did not fit"));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }

            const FVector CauldronLocation = Target->GetActorLocation() + FVector(0.0f, 150.0f, 0.0f);
            const FVector FireLocation = CauldronLocation + FVector(150.0f, 0.0f, 0.0f);
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Cauldron = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, CauldronLocation), SpawnParameters);
            AKalmalaCampfire* Fire = GetWorld()->SpawnActor<AKalmalaCampfire>(
                AKalmalaCampfire::StaticClass(), FTransform(FRotator::ZeroRotator, FireLocation), SpawnParameters);
            if (!Cauldron || !Fire)
            {
                if (Cauldron) Cauldron->Destroy();
                if (Fire) Fire->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer FAILED: Cauldron or test hearth did not spawn"));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            const FString ConstructionId = FString::Printf(TEXT("%s_%d"), A8bPeatHeartStewRackId,
                Target->GetPlayerState()->GetPlayerId());
            Cauldron->InitializeFromServer(TEXT("CauldronKit"), ConstructionId);
            Fire->SetSharedFromServer(true);
            if (AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            {
                FKalmalaWeatherState DryWeather = WorldState->GetWeatherState();
                DryWeather.PrecipitationIntensity = 0.0f;
                DryWeather.WindStrength = 0.0f;
                WorldState->SetWeatherStateFromServer(DryWeather);
            }

            const bool bFireReady = Fire->TryRefuelFromServer(Target) && Fire->TryLightFromServer(Target);
            const float StationHeat = Fire->GetWarmthContributionAt(Cauldron->GetActorLocation());
            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Cauldron->GetActorLocation())
                    <= FMath::Square(250.0f)
                && FVector::DistSquared(Fire->GetActorLocation(), Cauldron->GetActorLocation())
                    <= FMath::Square(250.0f)
                && FMath::IsFinite(StationHeat) && StationHeat > 0.0f
                && TargetCrafting->GetRecipeAvailability(A8bPeatHeartStewRecipe) == TEXT("Ready");
            if (!bFireReady || !bStationReady)
            {
                Cauldron->Destroy();
                Fire->Destroy();
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Peat Heart Stew peer FAILED: fire=%d station=%d heat=%.2f recipe=%s"),
                    bFireReady ? 1 : 0, bStationReady ? 1 : 0, StationHeat,
                    *TargetCrafting->GetRecipeAvailability(A8bPeatHeartStewRecipe));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }

            A8bPeatHeartStewPeerTarget = Target;
            A8bPeatHeartStewPeerBaselineStaminaCostAddition = TargetStatus->GetModifiers().StaminaCostAddition;
            TargetCrafting->PublishResult(A8bPeatHeartStewReadyResult, true);
            A8bPeatHeartStewPeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Cauldron->ForceNetUpdate();
            Fire->ForceNetUpdate();
            A8bPeatHeartStewPeerStage = 1;
            A8bPeatHeartStewPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Peat Heart Stew peer fixture: Ready=1 RemoteOwner=1 PeatHeart=1 Potato=2 Onion=1 Cauldron=1 Heat=1"));
        }
        else if (A8bPeatHeartStewPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bPeatHeartStewPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer FAILED: owner disappeared before craft result"));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bPeatHeartStewPeerResultSerial)
            {
                if (A8bPeatHeartStewPeerElapsed > 25.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer server: Passed=0 craft timed out"));
                    A8bPeatHeartStewPeerStage = 99;
                }
                return;
            }
            const FKalmalaInventoryStack* Stew = FindPeatHeartStewStack(TargetInventory);
            const bool bGuids = Stew && Stew->Quantity == A8bPeatHeartStewPeerBaselineStew + 2
                && HasUniquePeatHeartStewItemGuids(Stew);
            const bool bCrafted = TargetCrafting->GetLastResult() == TEXT("Crafted 2 Peat Heart Stew")
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("PeatHeart")) == A8bPeatHeartStewPeerBaselinePeatHeart
                && TargetInventory->GetQuantity(TEXT("Potato")) == A8bPeatHeartStewPeerBaselinePotato
                && TargetInventory->GetQuantity(TEXT("Onion")) == A8bPeatHeartStewPeerBaselineOnion
                && TargetInventory->GetQuantity(TEXT("PeatHeartStew")) == A8bPeatHeartStewPeerBaselineStew + 2 && bGuids;
            if (!bCrafted)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Peat Heart Stew peer server: Passed=0 Result=%s Accepted=%d Ingredients=%d/%d/%d Output=%d Guid=%d"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("PeatHeart")), TargetInventory->GetQuantity(TEXT("Potato")),
                    TargetInventory->GetQuantity(TEXT("Onion")), TargetInventory->GetQuantity(TEXT("PeatHeartStew")),
                    bGuids ? 1 : 0);
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("A8b Peat Heart Stew peer server craft: Accepted=1 Payment=1 Output=2 UniqueGuid=1"));
            A8bPeatHeartStewPeerResultSerial = TargetCrafting->GetResultSerial();
            A8bPeatHeartStewPeerStage = 2;
            A8bPeatHeartStewPeerElapsed = 0.0f;
        }
        else if (A8bPeatHeartStewPeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bPeatHeartStewPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetStatus)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer FAILED: owner disappeared before meal result"));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bPeatHeartStewPeerResultSerial)
            {
                if (A8bPeatHeartStewPeerElapsed > 30.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer server: Passed=0 first serving use timed out"));
                    A8bPeatHeartStewPeerStage = 99;
                }
                return;
            }
            const float MealSeconds = TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
            const float ExpectedCostAddition = A8bPeatHeartStewPeerBaselineStaminaCostAddition
                + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
            const bool bMealAccepted = TargetCrafting->GetLastResult().StartsWith(TEXT("Ate Peat Heart Stew;"))
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("PeatHeartStew")) == A8bPeatHeartStewPeerBaselineStew + 1
                && MealSeconds > 100.0f
                && FMath::IsNearlyEqual(TargetStatus->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f);
            if (!bMealAccepted)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Peat Heart Stew peer server: Passed=0 EatResult=%s Accepted=%d Food=%d Meal=%.1f Cost=%.2f Expected=%.2f"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("PeatHeartStew")), MealSeconds,
                    TargetStatus->GetModifiers().StaminaCostAddition, ExpectedCostAddition);
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            A8bPeatHeartStewPeerMealSecondsAfterUse = MealSeconds;
            A8bPeatHeartStewPeerResultSerial = TargetCrafting->GetResultSerial();
            A8bPeatHeartStewPeerStage = 3;
            A8bPeatHeartStewPeerElapsed = 0.0f;
        }
        else if (A8bPeatHeartStewPeerStage == 3)
        {
            AKalmalaCharacter* Target = A8bPeatHeartStewPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetStatus)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer FAILED: owner disappeared before duplicate-use result"));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bPeatHeartStewPeerResultSerial)
            {
                if (A8bPeatHeartStewPeerElapsed > 30.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer server: Passed=0 duplicate-use rejection timed out"));
                    A8bPeatHeartStewPeerStage = 99;
                }
                return;
            }
            const float MealSeconds = TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
            const float ExpectedCostAddition = A8bPeatHeartStewPeerBaselineStaminaCostAddition
                + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
            const bool bDuplicateRejected = TargetCrafting->GetLastResult()
                    == TEXT("A steady meal is already active; wait for it to expire")
                && !TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("PeatHeartStew")) == A8bPeatHeartStewPeerBaselineStew + 1
                && MealSeconds > 0.0f && MealSeconds <= A8bPeatHeartStewPeerMealSecondsAfterUse + 0.01f
                && FMath::IsNearlyEqual(TargetStatus->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f);
            if (!bDuplicateRejected)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Peat Heart Stew peer server: Passed=0 DuplicateResult=%s Accepted=%d Food=%d Meal=%.1f Initial=%.1f"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("PeatHeartStew")), MealSeconds,
                    A8bPeatHeartStewPeerMealSecondsAfterUse);
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("A8b Peat Heart Stew peer server: Passed=1 Craft=1 Payment=1 Output=2 Eat=1 Meal=1 NoStack=1"));
            A8bPeatHeartStewPeerStage = 4;
            A8bPeatHeartStewPeerElapsed = 0.0f;
        }
        else if (A8bPeatHeartStewPeerStage == 4)
        {
            if (A8bPeatHeartStewPeerElapsed < 1.0f) return;
            const bool bHostStable = Inventory->GetQuantity(TEXT("PeatHeart")) == A8bPeatHeartStewPeerHostBaselinePeatHeart
                && Inventory->GetQuantity(TEXT("Potato")) == A8bPeatHeartStewPeerHostBaselinePotato
                && Inventory->GetQuantity(TEXT("Onion")) == A8bPeatHeartStewPeerHostBaselineOnion
                && Inventory->GetQuantity(TEXT("PeatHeartStew")) == A8bPeatHeartStewPeerHostBaselineStew
                && !Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
            if (!bHostStable)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Peat Heart Stew peer listen host: Passed=0 ItemsStable=%d Meal=%d"),
                    Inventory->GetQuantity(TEXT("PeatHeart")) == A8bPeatHeartStewPeerHostBaselinePeatHeart
                        && Inventory->GetQuantity(TEXT("Potato")) == A8bPeatHeartStewPeerHostBaselinePotato
                        && Inventory->GetQuantity(TEXT("Onion")) == A8bPeatHeartStewPeerHostBaselineOnion
                        && Inventory->GetQuantity(TEXT("PeatHeartStew")) == A8bPeatHeartStewPeerHostBaselineStew ? 1 : 0,
                    Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId) ? 1 : 0);
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("A8b Peat Heart Stew peer listen host: Passed=1 HostItems=Stable Meal=0"));
            A8bPeatHeartStewPeerStage = 5;
        }
        return;
    }

    if (Character->IsLocallyControlled() && A8bPeatHeartStewPeerStage == 0
        && !bA8bPeatHeartStewPeerFixtureObserved)
    {
        AKalmalaConstructionActor* Cauldron = FindPeatHeartStewTestCauldron(GetWorld());
        if (!bA8bPeatHeartStewPeerObserverDiscoveryLogged && A8bPeatHeartStewPeerElapsed >= 1.0f)
        {
            int32 DiscoveredOwnerPlayerId = INDEX_NONE;
            const bool bOwnerIdValid = GetPeatHeartStewOwnerId(Cauldron, DiscoveredOwnerPlayerId);
            UE_LOG(LogTemp, Display,
                TEXT("A8b Peat Heart Stew peer observer discovery: LocalPlayerId=%d Cauldron=%d OwnerIdValid=%d OwnerPlayerId=%d"),
                Character->GetPlayerState()->GetPlayerId(), Cauldron ? 1 : 0, bOwnerIdValid ? 1 : 0,
                DiscoveredOwnerPlayerId);
            bA8bPeatHeartStewPeerObserverDiscoveryLogged = true;
        }
        if (Cauldron)
        {
            int32 OwnerPlayerId = INDEX_NONE;
            if (!GetPeatHeartStewOwnerId(Cauldron, OwnerPlayerId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer observer: Passed=0 invalid owner identity"));
                A8bPeatHeartStewPeerStage = 99;
                return;
            }
            if (Character->GetPlayerState()->GetPlayerId() != OwnerPlayerId)
            {
                AKalmalaCharacter* Owner = nullptr;
                for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
                {
                    if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == OwnerPlayerId)
                    {
                        Owner = *It;
                        break;
                    }
                }
                if (!Owner) return;
                A8bPeatHeartStewPeerObservedOwnerPlayerId = OwnerPlayerId;
                bA8bPeatHeartStewPeerFixtureObserved = true;
                A8bPeatHeartStewPeerElapsed = 0.0f;
                UE_LOG(LogTemp, Display,
                    TEXT("A8b Peat Heart Stew peer observer: FixtureSeen=1 LocalPlayerId=%d TargetPlayerId=%d"),
                    Character->GetPlayerState()->GetPlayerId(), OwnerPlayerId);
            }
        }
    }

    if (Character->IsLocallyControlled() && bA8bPeatHeartStewPeerFixtureObserved)
    {
        if (A8bPeatHeartStewPeerStage >= 1) return;
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == A8bPeatHeartStewPeerObservedOwnerPlayerId)
            {
                Owner = *It;
                break;
            }
        }
        const UKalmalaInventoryComponent* OwnerInventory = Owner
            ? Owner->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        const UKalmalaPlayerStatusComponent* OwnerStatus = Owner
            ? Owner->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
        const bool bPrivate = Owner && !Owner->IsLocallyControlled() && OwnerInventory && OwnerStatus
            && OwnerInventory->GetStacks().IsEmpty() && OwnerInventory->GetEquippedGear().IsEmpty()
            && OwnerStatus->GetStatuses().IsEmpty()
            && Inventory->GetQuantity(TEXT("PeatHeart")) == 0
            && Inventory->GetQuantity(TEXT("Potato")) == 0
            && Inventory->GetQuantity(TEXT("Onion")) == 0
            && Inventory->GetQuantity(TEXT("PeatHeartStew")) == 0;
        if (bPrivate && A8bPeatHeartStewPeerElapsed >= 8.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Peat Heart Stew peer observer: Passed=1 Authority=0 TargetPrivate=1 MealPrivate=1"));
            A8bPeatHeartStewPeerStage = 1;
        }
        else if (A8bPeatHeartStewPeerElapsed > 45.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Peat Heart Stew peer observer: Passed=0 OwnerFound=%d TargetPrivate=%d MealPrivate=%d"),
                Owner ? 1 : 0, bPrivate ? 1 : 0,
                OwnerStatus && OwnerStatus->GetStatuses().IsEmpty() ? 1 : 0);
            A8bPeatHeartStewPeerStage = 99;
        }
        return;
    }

    if (!Character->IsLocallyControlled()) return;
    if (A8bPeatHeartStewPeerStage == 0 && GetLastResult() == A8bPeatHeartStewReadyResult)
    {
        if (Inventory->GetQuantity(TEXT("PeatHeart")) < 1 || Inventory->GetQuantity(TEXT("Potato")) < 2
            || Inventory->GetQuantity(TEXT("Onion")) < 1
            || GetRecipeAvailability(A8bPeatHeartStewRecipe) != TEXT("Ready"))
        {
            if (A8bPeatHeartStewPeerElapsed < 20.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer owner: Passed=0 Cauldron, heat or recipe inputs did not replicate"));
            A8bPeatHeartStewPeerStage = 99;
            return;
        }
        A8bPeatHeartStewPeerBaselinePeatHeart = Inventory->GetQuantity(TEXT("PeatHeart")) - 1;
        A8bPeatHeartStewPeerBaselinePotato = Inventory->GetQuantity(TEXT("Potato")) - 2;
        A8bPeatHeartStewPeerBaselineOnion = Inventory->GetQuantity(TEXT("Onion")) - 1;
        A8bPeatHeartStewPeerBaselineStew = Inventory->GetQuantity(TEXT("PeatHeartStew"));
        A8bPeatHeartStewPeerBaselineStaminaCostAddition = Status->GetModifiers().StaminaCostAddition;
        FString DirectReason;
        if (CraftFromServer(A8bPeatHeartStewRecipe, 1, DirectReason)
            || ConsumeFoodFromServer(TEXT("PeatHeartStew"), DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer FAILED: direct client craft or food use bypassed authority"));
            A8bPeatHeartStewPeerStage = 99;
            return;
        }
        A8bPeatHeartStewPeerResultSerial = GetResultSerial();
        ServerCraft(A8bPeatHeartStewRecipe, 1);
        A8bPeatHeartStewPeerStage = 1;
        A8bPeatHeartStewPeerElapsed = 0.0f;
        return;
    }
    if (A8bPeatHeartStewPeerStage == 1)
    {
        if (GetResultSerial() == A8bPeatHeartStewPeerResultSerial)
        {
            if (A8bPeatHeartStewPeerElapsed > 25.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer owner: Passed=0 craft timed out"));
                A8bPeatHeartStewPeerStage = 99;
            }
            return;
        }
        const FKalmalaInventoryStack* Stew = FindPeatHeartStewStack(Inventory);
        const bool bCrafted = GetLastResult() == TEXT("Crafted 2 Peat Heart Stew") && WasLastResultAccepted()
            && Inventory->GetQuantity(TEXT("PeatHeart")) == A8bPeatHeartStewPeerBaselinePeatHeart
            && Inventory->GetQuantity(TEXT("Potato")) == A8bPeatHeartStewPeerBaselinePotato
            && Inventory->GetQuantity(TEXT("Onion")) == A8bPeatHeartStewPeerBaselineOnion
            && Stew && Stew->Quantity == A8bPeatHeartStewPeerBaselineStew + 2
            && HasUniquePeatHeartStewItemGuids(Stew);
        if (!bCrafted)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Peat Heart Stew peer owner: Passed=0 Result=%s Accepted=%d Output=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0, Inventory->GetQuantity(TEXT("PeatHeartStew")));
            A8bPeatHeartStewPeerStage = 99;
            return;
        }
        A8bPeatHeartStewPeerResultSerial = GetResultSerial();
        ServerConsumeFood(TEXT("PeatHeartStew"));
        A8bPeatHeartStewPeerStage = 2;
        A8bPeatHeartStewPeerElapsed = 0.0f;
        return;
    }
    if (A8bPeatHeartStewPeerStage == 2)
    {
        if (GetResultSerial() == A8bPeatHeartStewPeerResultSerial)
        {
            if (A8bPeatHeartStewPeerElapsed > 25.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer owner: Passed=0 first serving use timed out"));
                A8bPeatHeartStewPeerStage = 99;
            }
            return;
        }
        const float MealSeconds = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
        const float ExpectedCostAddition = A8bPeatHeartStewPeerBaselineStaminaCostAddition
            + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
        const bool bMealAccepted = GetLastResult().StartsWith(TEXT("Ate Peat Heart Stew;"))
            && WasLastResultAccepted()
            && Inventory->GetQuantity(TEXT("PeatHeartStew")) == A8bPeatHeartStewPeerBaselineStew + 1
            && MealSeconds > 100.0f
            && FMath::IsNearlyEqual(Status->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f);
        if (!bMealAccepted)
        {
            if (A8bPeatHeartStewPeerElapsed < 10.0f) return;
            UE_LOG(LogTemp, Error,
                TEXT("A8b Peat Heart Stew peer owner: Passed=0 EatResult=%s Accepted=%d Food=%d Meal=%.1f Cost=%.2f Expected=%.2f"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0,
                Inventory->GetQuantity(TEXT("PeatHeartStew")), MealSeconds,
                Status->GetModifiers().StaminaCostAddition, ExpectedCostAddition);
            A8bPeatHeartStewPeerStage = 99;
            return;
        }
        A8bPeatHeartStewPeerMealSecondsAfterUse = MealSeconds;
        A8bPeatHeartStewPeerResultSerial = GetResultSerial();
        ServerConsumeFood(TEXT("PeatHeartStew"));
        A8bPeatHeartStewPeerStage = 3;
        A8bPeatHeartStewPeerElapsed = 0.0f;
        return;
    }
    if (A8bPeatHeartStewPeerStage == 3)
    {
        if (GetResultSerial() == A8bPeatHeartStewPeerResultSerial)
        {
            if (A8bPeatHeartStewPeerElapsed > 25.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Peat Heart Stew peer owner: Passed=0 duplicate-use result timed out"));
                A8bPeatHeartStewPeerStage = 99;
            }
            return;
        }
        const float MealSeconds = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
        const float ExpectedCostAddition = A8bPeatHeartStewPeerBaselineStaminaCostAddition
            + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
        const bool bRejected = GetLastResult() == TEXT("A steady meal is already active; wait for it to expire")
            && !WasLastResultAccepted()
            && Inventory->GetQuantity(TEXT("PeatHeartStew")) == A8bPeatHeartStewPeerBaselineStew + 1
            && MealSeconds > 0.0f && MealSeconds <= A8bPeatHeartStewPeerMealSecondsAfterUse + 0.01f
            && FMath::IsNearlyEqual(Status->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f);
        if (!bRejected)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Peat Heart Stew peer owner: Passed=0 DuplicateResult=%s Accepted=%d Food=%d Meal=%.1f Initial=%.1f"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0, Inventory->GetQuantity(TEXT("PeatHeartStew")),
                MealSeconds, A8bPeatHeartStewPeerMealSecondsAfterUse);
            A8bPeatHeartStewPeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Peat Heart Stew peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Output=2 Eat=1 Meal=1 NoStack=1"));
        A8bPeatHeartStewPeerStage = 4;
    }
}

#endif
