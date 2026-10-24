#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

namespace
{
    constexpr TCHAR A8bTonicRecipe[] = TEXT("RatTailTonicRecipe");
    constexpr TCHAR A8bTonicReadyResult[] = TEXT("A8b Rat-tail Tonic peer ready");
    constexpr TCHAR A8bTonicCraftedResult[] = TEXT("Crafted 1 Rat-tail Tonic");
    constexpr TCHAR A8bTonicCauldronId[] = TEXT("A8bPeerTonicCauldron");

    const FKalmalaInventoryStack* FindTonicStack(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory ? Inventory->GetStacks().FindByPredicate([](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == TEXT("RatTailTonic");
        }) : nullptr;
    }

    AKalmalaConstructionActor* FindTonicTestCauldron(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
            if (It->GetConstructionId().StartsWith(FString(A8bTonicCauldronId) + TEXT("_"))) return *It;
        return nullptr;
    }

    bool GetTonicTestOwnerPlayerId(const AKalmalaConstructionActor* Cauldron, int32& OutPlayerId)
    {
        if (!Cauldron) return false;
        const FString Prefix = FString(A8bTonicCauldronId) + TEXT("_");
        if (!Cauldron->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Cauldron->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }
}

void UKalmalaCraftingComponent::RunA8bTonicPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    A8bTonicPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        // The listen host owns fixture orchestration. The actual craft request
        // is sent by the selected remote client's locally controlled pawn.
        if (!Character->IsLocallyControlled() || A8bTonicPeerStage >= 3) return;
        if (A8bTonicPeerStage == 0)
        {
            if (A8bTonicPeerElapsed < 3.0f) return;
            AKalmalaCharacter* Target = nullptr;
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                APlayerController* Controller = Cast<APlayerController>(It->GetController());
                if (!It->GetPlayerState() || !Controller || Controller->IsLocalController()) continue;
                if (!Target || It->GetPlayerState()->GetPlayerId() < Target->GetPlayerState()->GetPlayerId())
                    Target = *It;
            }
            // The harness starts its observer only after the first remote joins.
            // Wait for both remote peers before choosing the lower PlayerId as owner.
            int32 RemoteCount = 0;
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                const APlayerController* Controller = Cast<APlayerController>(It->GetController());
                if (It->GetPlayerState() && Controller && !Controller->IsLocalController()) ++RemoteCount;
            }
            if (RemoteCount < 2 || !Target) return;

            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            const bool bCleanBaseline = TargetInventory
                && TargetInventory->GetQuantity(TEXT("RatTail")) == 0
                && TargetInventory->GetQuantity(TEXT("Lingonberry")) == 0
                && TargetInventory->GetQuantity(TEXT("RatTailTonic")) == 0;
            if (!bCleanBaseline || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Tonic peer FAILED: remote owner baseline or crafting component is invalid"));
                A8bTonicPeerStage = 99;
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
                UE_LOG(LogTemp, Error, TEXT("A8b Tonic peer FAILED: Cauldron or test hearth did not spawn"));
                A8bTonicPeerStage = 99;
                return;
            }
            const FString CauldronId = FString::Printf(TEXT("%s_%d"), A8bTonicCauldronId,
                Target->GetPlayerState()->GetPlayerId());
            Cauldron->InitializeFromServer(TEXT("CauldronKit"), CauldronId);
            Fire->SetSharedFromServer(true);

            if (AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
            {
                FKalmalaWeatherState DryWeather = WorldState->GetWeatherState();
                DryWeather.PrecipitationIntensity = 0.0f;
                DryWeather.WindStrength = 0.0f;
                WorldState->SetWeatherStateFromServer(DryWeather);
            }

            const bool bMaterialsGranted = TargetInventory->TryGrantFromServer(TEXT("RatTail"), 2)
                && TargetInventory->TryGrantFromServer(TEXT("Lingonberry"), 2)
                && TargetInventory->TryGrantFromServer(TEXT("Wood"), 1);
            const bool bFireReady = bMaterialsGranted && Fire->TryRefuelFromServer(Target)
                && Fire->TryLightFromServer(Target);
            const float StationHeat = Fire->GetWarmthContributionAt(Cauldron->GetActorLocation());
            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Cauldron->GetActorLocation())
                    <= FMath::Square(250.0f)
                && FVector::DistSquared(Fire->GetActorLocation(), Cauldron->GetActorLocation())
                    <= FMath::Square(250.0f)
                && FMath::IsFinite(StationHeat) && StationHeat > 0.0f;
            if (!bFireReady || !bStationReady)
            {
                Cauldron->Destroy();
                Fire->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Tonic peer FAILED: materials=%d fire=%d station=%d heat=%.2f"),
                    bMaterialsGranted ? 1 : 0, bFireReady ? 1 : 0, bStationReady ? 1 : 0, StationHeat);
                A8bTonicPeerStage = 99;
                return;
            }

            A8bTonicPeerTarget = Target;
            TargetCrafting->PublishResult(A8bTonicReadyResult, true);
            A8bTonicPeerReadyResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Cauldron->ForceNetUpdate();
            A8bTonicPeerStage = 1;
            A8bTonicPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display, TEXT("A8b Tonic peer fixture: Ready=1 RemoteOwner=1 Inputs=2/2 Cauldron=1 Heat=1"));
        }
        else if (A8bTonicPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bTonicPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Tonic peer FAILED: remote owner disappeared"));
                A8bTonicPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bTonicPeerReadyResultSerial) return;
            const FKalmalaInventoryStack* Tonic = FindTonicStack(TargetInventory);
            const bool bGuid = Tonic && Tonic->Quantity == 1 && Tonic->ItemGuids.Num() == 1
                && Tonic->ItemGuids[0].IsValid();
            const bool bPassed = TargetCrafting->GetLastResult() == A8bTonicCraftedResult
                && TargetInventory->GetQuantity(TEXT("RatTail")) == 0
                && TargetInventory->GetQuantity(TEXT("Lingonberry")) == 0
                && TargetInventory->GetQuantity(TEXT("RatTailTonic")) == 1 && bGuid;
            if (bPassed)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("A8b Tonic peer server: Passed=1 RemoteOwner=1 AcceptedResult=1 Payment=1 Output=1 Guid=1"));
            }
            else
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Tonic peer server: Passed=0 RemoteOwner=1 Result=%s RatTail=%d Lingonberry=%d Output=%d Guid=%d"),
                    *TargetCrafting->GetLastResult(), TargetInventory->GetQuantity(TEXT("RatTail")),
                    TargetInventory->GetQuantity(TEXT("Lingonberry")), TargetInventory->GetQuantity(TEXT("RatTailTonic")),
                    bGuid ? 1 : 0);
            }
            A8bTonicPeerStage = bPassed ? 2 : 99;
        }
        else if (A8bTonicPeerStage == 2)
        {
            const bool bPrivate = Inventory->GetQuantity(TEXT("RatTailTonic")) == 0;
            if (bPrivate)
            {
                UE_LOG(LogTemp, Display, TEXT("A8b Tonic peer listen host: Passed=1 OwnTonic=0"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Tonic peer listen host: Passed=0 OwnTonic=%d"),
                    Inventory->GetQuantity(TEXT("RatTailTonic")));
            }
            A8bTonicPeerStage = bPrivate ? 3 : 99;
        }
        return;
    }

    if (!Character->IsLocallyControlled()) return;
    if (A8bTonicPeerStage == 0)
    {
        // The owner receives an owner-only readiness result. The observer sees
        // the public fixture actor, but not the owner's private result or items.
        if (GetLastResult() == A8bTonicReadyResult)
        {
            // Wait until the actual station, heat and exact owner materials are
            // present locally before sending the ordinary craft RPC.
            if (Inventory->GetQuantity(TEXT("RatTail")) != 2
                || Inventory->GetQuantity(TEXT("Lingonberry")) != 2
                || GetRecipeAvailability(A8bTonicRecipe) != TEXT("Ready")) return;
            FString DirectReason;
            if (CraftFromServer(A8bTonicRecipe, 1, DirectReason))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Tonic peer FAILED: client direct craft bypassed server authority"));
                A8bTonicPeerStage = 99;
                return;
            }
            ServerCraft(A8bTonicRecipe, 1);
            A8bTonicPeerStage = 1;
            A8bTonicPeerElapsed = 0.0f;
            return;
        }
        if (FindTonicTestCauldron(GetWorld()))
        {
            if (!bA8bTonicPeerFixtureObserved)
            {
                bA8bTonicPeerFixtureObserved = true;
                A8bTonicPeerElapsed = 0.0f;
            }
            else if (A8bTonicPeerElapsed >= 5.0f)
            {
                A8bTonicPeerStage = 2;
                A8bTonicPeerElapsed = 0.0f;
            }
        }
        return;
    }
    if (A8bTonicPeerStage == 1)
    {
        if (GetLastResult() == A8bTonicReadyResult || GetResultSerial() == 0) return;
        const FKalmalaInventoryStack* Tonic = FindTonicStack(Inventory);
        const bool bGuid = Tonic && Tonic->Quantity == 1 && Tonic->ItemGuids.Num() == 1
            && Tonic->ItemGuids[0].IsValid();
        const bool bPassed = GetLastResult() == A8bTonicCraftedResult && WasLastResultAccepted()
            && Inventory->GetQuantity(TEXT("RatTail")) == 0
            && Inventory->GetQuantity(TEXT("Lingonberry")) == 0
            && Inventory->GetQuantity(TEXT("RatTailTonic")) == 1 && bGuid;
        if (bPassed)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Tonic peer owner: Passed=1 Authority=0 AcceptedResult=1 Payment=1 Output=1 Guid=1"));
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Tonic peer owner: Passed=0 Authority=0 Result=%s Accepted=%d RatTail=%d Lingonberry=%d Output=%d Guid=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0, Inventory->GetQuantity(TEXT("RatTail")),
                Inventory->GetQuantity(TEXT("Lingonberry")), Inventory->GetQuantity(TEXT("RatTailTonic")), bGuid ? 1 : 0);
        }
        A8bTonicPeerStage = bPassed ? 3 : 99;
        return;
    }

    if (A8bTonicPeerStage == 2)
    {
        if (GetLastResult() == A8bTonicReadyResult)
        {
            A8bTonicPeerStage = 0;
            bA8bTonicPeerFixtureObserved = false;
            return;
        }
        AKalmalaConstructionActor* Cauldron = FindTonicTestCauldron(GetWorld());
        if (!Cauldron) return;
        if (A8bTonicPeerElapsed < 5.0f) return;
        int32 OwnerPlayerId = INDEX_NONE;
        AKalmalaCharacter* OwnerCharacter = nullptr;
        if (GetTonicTestOwnerPlayerId(Cauldron, OwnerPlayerId))
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
                if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == OwnerPlayerId)
                {
                    OwnerCharacter = *It;
                    break;
                }
        const UKalmalaInventoryComponent* OwnerInventory = OwnerCharacter
            ? OwnerCharacter->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        const bool bPrivate = OwnerInventory != nullptr && !OwnerCharacter->IsLocallyControlled()
            && OwnerInventory->GetQuantity(TEXT("RatTail")) == 0
            && OwnerInventory->GetQuantity(TEXT("Lingonberry")) == 0
            && OwnerInventory->GetQuantity(TEXT("RatTailTonic")) == 0
            && Inventory->GetQuantity(TEXT("RatTail")) == 0
            && Inventory->GetQuantity(TEXT("Lingonberry")) == 0
            && Inventory->GetQuantity(TEXT("RatTailTonic")) == 0;
        if (bPrivate)
        {
            UE_LOG(LogTemp, Display, TEXT("A8b Tonic peer observer: Passed=1 Authority=0 TargetPrivate=1 Items=0/0/0"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Tonic peer observer: Passed=0 Authority=0 TargetFound=%d TargetPrivate=%d OwnItems=%d/%d/%d"),
                OwnerInventory != nullptr ? 1 : 0,
                OwnerInventory && OwnerInventory->GetQuantity(TEXT("RatTail")) == 0
                    && OwnerInventory->GetQuantity(TEXT("Lingonberry")) == 0
                    && OwnerInventory->GetQuantity(TEXT("RatTailTonic")) == 0 ? 1 : 0,
                Inventory->GetQuantity(TEXT("RatTail")), Inventory->GetQuantity(TEXT("Lingonberry")),
                Inventory->GetQuantity(TEXT("RatTailTonic")));
        }
        A8bTonicPeerStage = bPrivate ? 3 : 99;
    }
}

#endif
