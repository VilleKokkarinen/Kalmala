#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR RoastedBadgerRecipe[] = TEXT("RoastedBadgerRecipe");
    constexpr TCHAR RoastedBadgerReadyResult[] = TEXT("A8b Roasted Badger peer ready");
    constexpr TCHAR RoastedBadgerCraftedResult[] = TEXT("Crafted 2 Roasted Badger");
    constexpr TCHAR RoastedBadgerEatenResult[] = TEXT("Ate Roasted Badger; stamina use is 10% lower for 120 seconds");
    constexpr TCHAR RoastedBadgerDuplicateResult[] = TEXT("A steady meal is already active; wait for it to expire");
    constexpr TCHAR RoastedBadgerRackId[] = TEXT("A8bPeerRoastedRack");

    AKalmalaConstructionActor* FindRoastedBadgerTestRack(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (It->GetConstructionId().StartsWith(FString(RoastedBadgerRackId) + TEXT("_"))) return *It;
        }
        return nullptr;
    }

    bool GetRoastedBadgerTestOwnerPlayerId(const AKalmalaConstructionActor* Rack, int32& OutPlayerId)
    {
        if (!Rack) return false;
        const FString Prefix = FString(RoastedBadgerRackId) + TEXT("_");
        if (!Rack->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Rack->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }
}

void UKalmalaCraftingComponent::RunA8bRoastedBadgerPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    UKalmalaPlayerStatusComponent* Status = Character
        ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !Status || !GetWorld()) return;
    A8bRoastedBadgerPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || A8bRoastedBadgerPeerStage >= 5) return;
        if (A8bRoastedBadgerPeerStage == 0)
        {
            if (A8bRoastedBadgerPeerElapsed < 3.0f) return;
            AKalmalaCharacter* Target = nullptr;
            int32 RemoteCount = 0;
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                const APlayerController* Controller = Cast<APlayerController>(It->GetController());
                if (!It->GetPlayerState() || !Controller || Controller->IsLocalController()) continue;
                ++RemoteCount;
                if (!Target || It->GetPlayerState()->GetPlayerId() < Target->GetPlayerState()->GetPlayerId()) Target = *It;
            }
            if (RemoteCount < 2 || !Target) return;

            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            UKalmalaPlayerStatusComponent* TargetStatus = Target->FindComponentByClass<UKalmalaPlayerStatusComponent>();
            const bool bBaseline = TargetInventory && TargetCrafting && TargetStatus
                && TargetInventory->GetQuantity(TEXT("BadgerMeat")) == 0
                && TargetInventory->GetQuantity(TEXT("RoastedBadger")) == 0
                && !TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
            if (!bBaseline)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer FAILED: remote owner baseline is invalid"));
                A8bRoastedBadgerPeerStage = 99;
                return;
            }

            const FVector RackLocation = Target->GetActorLocation() + FVector(0.0f, 150.0f, 0.0f);
            const FVector FireLocation = RackLocation + FVector(150.0f, 0.0f, 0.0f);
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Rack = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, RackLocation), SpawnParameters);
            AKalmalaCampfire* Fire = GetWorld()->SpawnActor<AKalmalaCampfire>(
                AKalmalaCampfire::StaticClass(), FTransform(FRotator::ZeroRotator, FireLocation), SpawnParameters);
            if (!Rack || !Fire)
            {
                if (Rack) Rack->Destroy();
                if (Fire) Fire->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer FAILED: Cooking Rack or test hearth did not spawn"));
                A8bRoastedBadgerPeerStage = 99;
                return;
            }

            const FString ConstructionId = FString::Printf(TEXT("%s_%d"), RoastedBadgerRackId,
                Target->GetPlayerState()->GetPlayerId());
            Rack->InitializeFromServer(TEXT("CookingRackKit"), ConstructionId);
            Fire->SetSharedFromServer(true);
            if (AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            {
                FKalmalaWeatherState DryWeather = WorldState->GetWeatherState();
                DryWeather.PrecipitationIntensity = 0.0f;
                DryWeather.WindStrength = 0.0f;
                WorldState->SetWeatherStateFromServer(DryWeather);
            }

            const bool bMaterials = TargetInventory->TryGrantFromServer(TEXT("BadgerMeat"), 2)
                && TargetInventory->TryGrantFromServer(TEXT("Wood"), 1);
            const bool bFireReady = bMaterials && Fire->TryRefuelFromServer(Target) && Fire->TryLightFromServer(Target);
            const float StationHeat = Fire->GetWarmthContributionAt(Rack->GetActorLocation());
            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Rack->GetActorLocation())
                    <= FMath::Square(250.0f)
                && FVector::DistSquared(Fire->GetActorLocation(), Rack->GetActorLocation()) <= FMath::Square(250.0f)
                && FMath::IsFinite(StationHeat) && StationHeat > 0.0f
                && TargetCrafting->GetRecipeAvailability(RoastedBadgerRecipe) == TEXT("Ready");
            if (!bFireReady || !bStationReady)
            {
                Rack->Destroy();
                Fire->Destroy();
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Roasted Badger peer FAILED: materials=%d fire=%d station=%d heat=%.2f"),
                    bMaterials ? 1 : 0, bFireReady ? 1 : 0, bStationReady ? 1 : 0, StationHeat);
                A8bRoastedBadgerPeerStage = 99;
                return;
            }

            A8bRoastedBadgerPeerTarget = Target;
            A8bRoastedBadgerPeerBaselineStaminaCostAddition = TargetStatus->GetModifiers().StaminaCostAddition;
            TargetCrafting->PublishResult(RoastedBadgerReadyResult, true);
            A8bRoastedBadgerPeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Rack->ForceNetUpdate();
            A8bRoastedBadgerPeerStage = 1;
            A8bRoastedBadgerPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Roasted Badger peer fixture: Ready=1 RemoteOwner=1 BadgerMeat=2 CookingRack=1 Heat=1"));
        }
        else if (A8bRoastedBadgerPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bRoastedBadgerPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer FAILED: owner disappeared before craft result"));
                A8bRoastedBadgerPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bRoastedBadgerPeerResultSerial) return;
            const bool bCrafted = TargetCrafting->GetLastResult() == RoastedBadgerCraftedResult
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("BadgerMeat")) == 0
                && TargetInventory->GetQuantity(TEXT("RoastedBadger")) == 2;
            if (!bCrafted)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Roasted Badger peer server: Passed=0 CraftResult=%s Accepted=%d Meat=%d Output=%d"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("BadgerMeat")), TargetInventory->GetQuantity(TEXT("RoastedBadger")));
                A8bRoastedBadgerPeerStage = 99;
                return;
            }
            A8bRoastedBadgerPeerResultSerial = TargetCrafting->GetResultSerial();
            A8bRoastedBadgerPeerStage = 2;
            A8bRoastedBadgerPeerElapsed = 0.0f;
        }
        else if (A8bRoastedBadgerPeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bRoastedBadgerPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetStatus
                || TargetCrafting->GetResultSerial() == A8bRoastedBadgerPeerResultSerial) return;
            const float MealSeconds = TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
            const float ExpectedCostAddition = A8bRoastedBadgerPeerBaselineStaminaCostAddition
                + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
            const bool bMealAccepted = TargetCrafting->GetLastResult() == RoastedBadgerEatenResult
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("RoastedBadger")) == 1
                && MealSeconds > 100.0f
                && FMath::IsNearlyEqual(TargetStatus->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f);
            if (!bMealAccepted)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Roasted Badger peer server: Passed=0 EatResult=%s Accepted=%d Food=%d Meal=%.1f Cost=%.2f Expected=%.2f"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("RoastedBadger")), MealSeconds,
                    TargetStatus->GetModifiers().StaminaCostAddition, ExpectedCostAddition);
                A8bRoastedBadgerPeerStage = 99;
                return;
            }
            A8bRoastedBadgerPeerMealSecondsAfterUse = MealSeconds;
            A8bRoastedBadgerPeerResultSerial = TargetCrafting->GetResultSerial();
            A8bRoastedBadgerPeerStage = 3;
            A8bRoastedBadgerPeerElapsed = 0.0f;
        }
        else if (A8bRoastedBadgerPeerStage == 3)
        {
            AKalmalaCharacter* Target = A8bRoastedBadgerPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetStatus
                || TargetCrafting->GetResultSerial() == A8bRoastedBadgerPeerResultSerial) return;
            const float MealSeconds = TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
            const float ExpectedCostAddition = A8bRoastedBadgerPeerBaselineStaminaCostAddition
                + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
            const bool bDuplicateRejected = TargetCrafting->GetLastResult() == RoastedBadgerDuplicateResult
                && !TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("RoastedBadger")) == 1
                && MealSeconds > 0.0f && MealSeconds <= A8bRoastedBadgerPeerMealSecondsAfterUse + 0.01f
                && FMath::IsNearlyEqual(TargetStatus->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f);
            if (!bDuplicateRejected)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Roasted Badger peer server: Passed=0 DuplicateResult=%s Accepted=%d Food=%d Meal=%.1f Initial=%.1f"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("RoastedBadger")), MealSeconds,
                    A8bRoastedBadgerPeerMealSecondsAfterUse);
                A8bRoastedBadgerPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("A8b Roasted Badger peer server: Passed=1 RemoteOwner=1 Craft=1 Payment=1 Output=2 Eat=1 Meal=1 NoStack=1"));
            A8bRoastedBadgerPeerStage = 4;
            A8bRoastedBadgerPeerElapsed = 0.0f;
        }
        else if (A8bRoastedBadgerPeerStage == 4)
        {
            if (A8bRoastedBadgerPeerElapsed < 1.0f) return;
            const bool bHostPrivate = Inventory->GetQuantity(TEXT("BadgerMeat")) == 0
                && Inventory->GetQuantity(TEXT("RoastedBadger")) == 0
                && !Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
            if (bHostPrivate)
            {
                UE_LOG(LogTemp, Display, TEXT("A8b Roasted Badger peer listen host: Passed=1 OwnItems=0 Meal=0"));
                A8bRoastedBadgerPeerStage = 5;
            }
            else
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Roasted Badger peer listen host: Passed=0 OwnMeat=%d OwnFood=%d Meal=%d"),
                    Inventory->GetQuantity(TEXT("BadgerMeat")), Inventory->GetQuantity(TEXT("RoastedBadger")),
                    Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId) ? 1 : 0);
                A8bRoastedBadgerPeerStage = 99;
            }
        }
        return;
    }

    if (!Character->IsLocallyControlled()) return;
    if (bA8bRoastedBadgerPeerFixtureObserved)
    {
        if (A8bRoastedBadgerPeerStage >= 5) return;
        if (A8bRoastedBadgerPeerElapsed < 8.0f) return;
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == A8bRoastedBadgerPeerObservedOwnerPlayerId)
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
            && Inventory->GetQuantity(TEXT("BadgerMeat")) == 0
            && Inventory->GetQuantity(TEXT("RoastedBadger")) == 0
            && !Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
        if (bPrivate)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Roasted Badger peer observer: Passed=1 Authority=0 TargetPrivate=1 StatusPrivate=1"));
            A8bRoastedBadgerPeerStage = 5;
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Roasted Badger peer observer: Passed=0 Authority=0 TargetFound=%d TargetPrivate=%d StatusPrivate=%d"),
                Owner ? 1 : 0,
                OwnerInventory && OwnerInventory->GetStacks().IsEmpty() && OwnerInventory->GetEquippedGear().IsEmpty() ? 1 : 0,
                OwnerStatus && OwnerStatus->GetStatuses().IsEmpty() ? 1 : 0);
            A8bRoastedBadgerPeerStage = 99;
        }
        return;
    }
    if (A8bRoastedBadgerPeerStage == 0)
    {
        if (AKalmalaConstructionActor* Rack = FindRoastedBadgerTestRack(GetWorld()))
        {
            int32 OwnerPlayerId = INDEX_NONE;
            if (!GetRoastedBadgerTestOwnerPlayerId(Rack, OwnerPlayerId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer observer: Passed=0 invalid owner identity"));
                A8bRoastedBadgerPeerStage = 99;
                return;
            }
            if (Character->GetPlayerState()->GetPlayerId() != OwnerPlayerId)
            {
                bA8bRoastedBadgerPeerFixtureObserved = true;
                A8bRoastedBadgerPeerElapsed = 0.0f;
                A8bRoastedBadgerPeerObservedOwnerPlayerId = OwnerPlayerId;
                UE_LOG(LogTemp, Display,
                    TEXT("A8b Roasted Badger peer observer: FixtureSeen=1 TargetPlayerId=%d"), OwnerPlayerId);
                return;
            }
        }
    }
    if (A8bRoastedBadgerPeerStage == 0 && GetLastResult() == RoastedBadgerReadyResult)
    {
        if (Inventory->GetQuantity(TEXT("BadgerMeat")) != 2
            || GetRecipeAvailability(RoastedBadgerRecipe) != TEXT("Ready")) return;
        A8bRoastedBadgerPeerBaselineStaminaCostAddition = Status->GetModifiers().StaminaCostAddition;
        if (Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer FAILED: a meal was active before eating"));
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        FString DirectReason;
        if (CraftFromServer(RoastedBadgerRecipe, 2, DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer FAILED: direct client craft bypassed server authority"));
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        if (ConsumeFoodFromServer(TEXT("RoastedBadger"), DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer FAILED: direct client food use bypassed server authority"));
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        A8bRoastedBadgerPeerResultSerial = GetResultSerial();
        ServerCraft(RoastedBadgerRecipe, 2);
        A8bRoastedBadgerPeerStage = 1;
        A8bRoastedBadgerPeerElapsed = 0.0f;
        return;
    }
    if (A8bRoastedBadgerPeerStage == 1)
    {
        if (GetResultSerial() == A8bRoastedBadgerPeerResultSerial) return;
        if (GetLastResult() != RoastedBadgerCraftedResult || !WasLastResultAccepted())
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer owner: Passed=0 CraftResult=%s Accepted=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0);
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        if (Inventory->GetQuantity(TEXT("BadgerMeat")) != 0 || Inventory->GetQuantity(TEXT("RoastedBadger")) != 2)
        {
            if (A8bRoastedBadgerPeerElapsed < 15.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer owner: crafted food or payment did not replicate"));
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        A8bRoastedBadgerPeerResultSerial = GetResultSerial();
        ServerConsumeFood(TEXT("RoastedBadger"));
        A8bRoastedBadgerPeerStage = 2;
        A8bRoastedBadgerPeerElapsed = 0.0f;
        return;
    }
    if (A8bRoastedBadgerPeerStage == 2)
    {
        if (GetResultSerial() == A8bRoastedBadgerPeerResultSerial) return;
        if (GetLastResult() != RoastedBadgerEatenResult || !WasLastResultAccepted())
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Roasted Badger peer owner: Passed=0 EatResult=%s Accepted=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0);
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        const float MealSeconds = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
        const float ExpectedCostAddition = A8bRoastedBadgerPeerBaselineStaminaCostAddition
            + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
        if (Inventory->GetQuantity(TEXT("RoastedBadger")) != 1 || MealSeconds <= 100.0f
            || !FMath::IsNearlyEqual(Status->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f))
        {
            if (A8bRoastedBadgerPeerElapsed < 15.0f) return;
            UE_LOG(LogTemp, Error,
                TEXT("A8b Roasted Badger peer owner: Passed=0 Food=%d Meal=%.1f Cost=%.2f Expected=%.2f"),
                Inventory->GetQuantity(TEXT("RoastedBadger")), MealSeconds,
                Status->GetModifiers().StaminaCostAddition, ExpectedCostAddition);
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        A8bRoastedBadgerPeerMealSecondsAfterUse = MealSeconds;
        A8bRoastedBadgerPeerResultSerial = GetResultSerial();
        ServerConsumeFood(TEXT("RoastedBadger"));
        A8bRoastedBadgerPeerStage = 3;
        A8bRoastedBadgerPeerElapsed = 0.0f;
        return;
    }
    if (A8bRoastedBadgerPeerStage == 3)
    {
        if (GetResultSerial() == A8bRoastedBadgerPeerResultSerial) return;
        const float MealSeconds = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
        const float ExpectedCostAddition = A8bRoastedBadgerPeerBaselineStaminaCostAddition
            + UKalmalaPlayerStatusComponent::SteadyMealStaminaCostAddition;
        const bool bRejected = GetLastResult() == RoastedBadgerDuplicateResult && !WasLastResultAccepted()
            && Inventory->GetQuantity(TEXT("RoastedBadger")) == 1 && MealSeconds > 0.0f
            && MealSeconds <= A8bRoastedBadgerPeerMealSecondsAfterUse + 0.01f
            && FMath::IsNearlyEqual(Status->GetModifiers().StaminaCostAddition, ExpectedCostAddition, 0.01f);
        if (!bRejected)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Roasted Badger peer owner: Passed=0 DuplicateResult=%s Accepted=%d Food=%d Meal=%.1f Initial=%.1f"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0, Inventory->GetQuantity(TEXT("RoastedBadger")),
                MealSeconds, A8bRoastedBadgerPeerMealSecondsAfterUse);
            A8bRoastedBadgerPeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Roasted Badger peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Output=2 Eat=1 Meal=1 NoStack=1"));
        A8bRoastedBadgerPeerStage = 4;
        return;
    }

}

#endif
