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
    constexpr TCHAR A8bAntidoteRecipe[] = TEXT("AntidoteRecipe");
    constexpr TCHAR A8bAntidoteReadyResult[] = TEXT("A8b Antidote peer ready");
    constexpr TCHAR A8bAntidotePoisonReadyResult[] = TEXT("A8b Antidote poison ready");
    constexpr TCHAR A8bAntidoteResistanceReadyResult[] = TEXT("A8b Antidote resistance ready");
    constexpr TCHAR A8bAntidoteCauldronId[] = TEXT("A8bPeerAntidoteCauldron");

    const FKalmalaInventoryStack* FindAntidoteStack(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory ? Inventory->GetStacks().FindByPredicate([](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == TEXT("Antidote");
        }) : nullptr;
    }

    AKalmalaConstructionActor* FindAntidoteTestCauldron(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (It->GetConstructionId().StartsWith(FString(A8bAntidoteCauldronId) + TEXT("_"))) return *It;
        }
        return nullptr;
    }

    bool GetAntidoteOwnerId(const AKalmalaConstructionActor* Cauldron, int32& OutPlayerId)
    {
        if (!Cauldron) return false;
        const FString Prefix = FString(A8bAntidoteCauldronId) + TEXT("_");
        if (!Cauldron->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Cauldron->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }
}

void UKalmalaCraftingComponent::RunA8bAntidotePeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    A8bAntidotePeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || A8bAntidotePeerStage >= 6) return;
        if (A8bAntidotePeerStage == 0)
        {
            if (A8bAntidotePeerElapsed < 3.0f) return;
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
                || TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId)
                || TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId)
                || TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId)
                || TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: fresh remote owner status baseline is invalid"));
                A8bAntidotePeerStage = 99;
                return;
            }

            A8bAntidotePeerBaselineVenomSac = TargetInventory->GetQuantity(TEXT("VenomSac"));
            A8bAntidotePeerBaselineLingonberry = TargetInventory->GetQuantity(TEXT("Lingonberry"));
            A8bAntidotePeerBaselineDose = TargetInventory->GetQuantity(TEXT("Antidote"));
            A8bAntidotePeerHostBaselineVenomSac = Inventory->GetQuantity(TEXT("VenomSac"));
            A8bAntidotePeerHostBaselineLingonberry = Inventory->GetQuantity(TEXT("Lingonberry"));
            A8bAntidotePeerHostBaselineDose = Inventory->GetQuantity(TEXT("Antidote"));
            if (!TargetInventory->TryGrantFromServer(TEXT("VenomSac"), 2)
                || !TargetInventory->TryGrantFromServer(TEXT("Lingonberry"), 4)
                || !TargetInventory->TryGrantFromServer(TEXT("Wood"), 1))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: could not stage Cauldron recipe and hearth fuel"));
                A8bAntidotePeerStage = 99;
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
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: Cauldron or test hearth did not spawn"));
                A8bAntidotePeerStage = 99;
                return;
            }
            const FString ConstructionId = FString::Printf(TEXT("%s_%d"), A8bAntidoteCauldronId,
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
                && TargetCrafting->GetRecipeAvailability(A8bAntidoteRecipe) == TEXT("Ready");
            if (!bFireReady || !bStationReady)
            {
                Cauldron->Destroy();
                Fire->Destroy();
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Antidote peer FAILED: fire=%d station=%d heat=%.2f recipe=%s"),
                    bFireReady ? 1 : 0, bStationReady ? 1 : 0, StationHeat,
                    *TargetCrafting->GetRecipeAvailability(A8bAntidoteRecipe));
                A8bAntidotePeerStage = 99;
                return;
            }

            A8bAntidotePeerTarget = Target;
            TargetCrafting->PublishResult(A8bAntidoteReadyResult, true);
            A8bAntidotePeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Cauldron->ForceNetUpdate();
            Fire->ForceNetUpdate();
            A8bAntidotePeerStage = 1;
            A8bAntidotePeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Antidote peer fixture: Ready=1 RemoteOwner=1 VenomSac=2 Lingonberry=4 Cauldron=1 Heat=1"));
        }
        else if (A8bAntidotePeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bAntidotePeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetStatus)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: owner disappeared before craft result"));
                A8bAntidotePeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bAntidotePeerResultSerial)
            {
                if (A8bAntidotePeerElapsed > 25.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer server: Passed=0 craft result timed out"));
                    A8bAntidotePeerStage = 99;
                }
                return;
            }
            const FKalmalaInventoryStack* Antidote = FindAntidoteStack(TargetInventory);
            const bool bGuids = Antidote && Antidote->Quantity == A8bAntidotePeerBaselineDose + 2
                && Antidote->ItemGuids.Num() == Antidote->Quantity
                && Antidote->ItemGuids[0].IsValid() && Antidote->ItemGuids[1].IsValid()
                && Antidote->ItemGuids[0] != Antidote->ItemGuids[1];
            const bool bCraftPassed = TargetCrafting->GetLastResult().Contains(TEXT("Crafted 2 Antidote"))
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("VenomSac")) == A8bAntidotePeerBaselineVenomSac
                && TargetInventory->GetQuantity(TEXT("Lingonberry")) == A8bAntidotePeerBaselineLingonberry
                && TargetInventory->GetQuantity(TEXT("Antidote")) == A8bAntidotePeerBaselineDose + 2 && bGuids;
            if (!bCraftPassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Antidote peer server: Passed=0 Craft=%d Accepted=%d VenomSac=%d Lingonberry=%d Output=%d Guid=%d"),
                    TargetCrafting->GetLastResult().Contains(TEXT("Crafted 2 Antidote")) ? 1 : 0,
                    TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("VenomSac")), TargetInventory->GetQuantity(TEXT("Lingonberry")),
                    TargetInventory->GetQuantity(TEXT("Antidote")), bGuids ? 1 : 0);
                A8bAntidotePeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("A8b Antidote peer server craft: Accepted=1 Payment=1 Output=2 UniqueGuid=1"));
            A8bAntidotePeerStage = 2;
            A8bAntidotePeerElapsed = 0.0f;
        }
        else if (A8bAntidotePeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bAntidotePeerTarget.Get();
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetCrafting || !TargetStatus)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: owner disappeared before poison setup"));
                A8bAntidotePeerStage = 99;
                return;
            }
            // Keep the craft result visible to the owner before the poison-ready
            // signal replaces it. Major poison supersedes weak damage while the
            // Antidote contract must clear the old weak entry and cap the new one.
            if (A8bAntidotePeerElapsed < 2.0f) return;
            if (!TargetStatus->ApplyWeakPoisonFromServer(10.0f)
                || !TargetStatus->ApplyMajorPoisonFromServer(10.0f))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer server: Passed=0 could not stage weak and major poison"));
                A8bAntidotePeerStage = 99;
                return;
            }
            TargetCrafting->PublishResult(A8bAntidotePoisonReadyResult, true);
            A8bAntidotePeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            A8bAntidotePeerStage = 3;
            A8bAntidotePeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Antidote peer server: PoisonStaged=1 Weak=10 Major=10"));
        }
        else if (A8bAntidotePeerStage == 3)
        {
            AKalmalaCharacter* Target = A8bAntidotePeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetStatus)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: owner disappeared before first use result"));
                A8bAntidotePeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bAntidotePeerResultSerial)
            {
                if (A8bAntidotePeerElapsed > 30.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer server: Passed=0 first use timed out"));
                    A8bAntidotePeerStage = 99;
                }
                return;
            }
            const float WeakRemaining = TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId);
            const float ResistanceRemaining = TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId);
            const float CooldownRemaining = TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId);
            const bool bFirstUsePassed = TargetCrafting->GetLastResult().StartsWith(TEXT("Used Antidote;"))
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("VenomSac")) == A8bAntidotePeerBaselineVenomSac
                && TargetInventory->GetQuantity(TEXT("Lingonberry")) == A8bAntidotePeerBaselineLingonberry
                && TargetInventory->GetQuantity(TEXT("Antidote")) == A8bAntidotePeerBaselineDose + 1
                && !TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId)
                && WeakRemaining > 0.0f && WeakRemaining <= 5.1f
                && ResistanceRemaining > 590.0f && CooldownRemaining > 50.0f;
            if (!bFirstUsePassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Antidote peer server: Passed=0 Use=%d Accepted=%d Dose=%d Major=%d Weak=%.2f Resistance=%.1f Cooldown=%.1f"),
                    TargetCrafting->GetLastResult().StartsWith(TEXT("Used Antidote;")) ? 1 : 0,
                    TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("Antidote")),
                    TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId) ? 1 : 0,
                    WeakRemaining, ResistanceRemaining, CooldownRemaining);
                A8bAntidotePeerStage = 99;
                return;
            }

            UE_LOG(LogTemp, Display,
                TEXT("A8b Antidote peer server first use: Passed=1 ClearWeak=1 ReduceMajor=1 Resistance=1 Cooldown=1"));
            A8bAntidotePeerStage = 4;
            A8bAntidotePeerElapsed = 0.0f;
            return;
        }
        else if (A8bAntidotePeerStage == 4)
        {
            if (A8bAntidotePeerElapsed < 2.0f) return;
            AKalmalaCharacter* Target = A8bAntidotePeerTarget.Get();
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetCrafting || !TargetStatus)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: owner disappeared before resistance probe"));
                A8bAntidotePeerStage = 99;
                return;
            }
            const bool bWeakBlocked = !TargetStatus->ApplyWeakPoisonFromServer(5.0f);
            const bool bMajorReduced = TargetStatus->ApplyMajorPoisonFromServer(8.0f)
                && !TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId)
                && TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId) > 0.0f
                && TargetStatus->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId) <= 5.1f;
            if (!bWeakBlocked || !bMajorReduced)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Antidote peer server: Passed=0 WeakBlocked=%d MajorReduced=%d"),
                    bWeakBlocked ? 1 : 0, bMajorReduced ? 1 : 0);
                A8bAntidotePeerStage = 99;
                return;
            }
            TargetCrafting->PublishResult(A8bAntidoteResistanceReadyResult, true);
            A8bAntidotePeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            A8bAntidotePeerStage = 5;
            A8bAntidotePeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Antidote peer server treatment: Passed=1 ClearWeak=1 ReduceMajor=1 WeakBlocked=1 MajorReduced=1 Resistance=1 Cooldown=1"));
        }
        else if (A8bAntidotePeerStage == 5)
        {
            AKalmalaCharacter* Target = A8bAntidotePeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaPlayerStatusComponent* TargetStatus = Target
                ? Target->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetStatus)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: owner disappeared before cooldown rejection"));
                A8bAntidotePeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bAntidotePeerResultSerial)
            {
                if (A8bAntidotePeerElapsed > 30.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer server: Passed=0 cooldown rejection timed out"));
                    A8bAntidotePeerStage = 99;
                }
                return;
            }
            const bool bRejectedWithoutConsumption = !TargetCrafting->WasLastResultAccepted()
                && TargetCrafting->GetLastResult().Contains(TEXT("Draught recovery cooldown is active"))
                && TargetInventory->GetQuantity(TEXT("VenomSac")) == A8bAntidotePeerBaselineVenomSac
                && TargetInventory->GetQuantity(TEXT("Lingonberry")) == A8bAntidotePeerBaselineLingonberry
                && TargetInventory->GetQuantity(TEXT("Antidote")) == A8bAntidotePeerBaselineDose + 1
                && TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId)
                && TargetStatus->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId);
            const bool bHostStable = Inventory->GetQuantity(TEXT("VenomSac")) == A8bAntidotePeerHostBaselineVenomSac
                && Inventory->GetQuantity(TEXT("Lingonberry")) == A8bAntidotePeerHostBaselineLingonberry
                && Inventory->GetQuantity(TEXT("Antidote")) == A8bAntidotePeerHostBaselineDose;
            if (!bRejectedWithoutConsumption || !bHostStable)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Antidote peer server: Passed=0 CooldownReject=%d OwnerDoses=%d HostStable=%d Reason=%s"),
                    bRejectedWithoutConsumption ? 1 : 0, TargetInventory->GetQuantity(TEXT("Antidote")),
                    bHostStable ? 1 : 0, *TargetCrafting->GetLastResult());
                A8bAntidotePeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("A8b Antidote peer server: Passed=1 Craft=2 Payment=1 Output=2 ClearWeak=1 ReduceMajor=1 Resist=1 CooldownReject=1 DosePreserved=1"));
            UE_LOG(LogTemp, Display, TEXT("A8b Antidote peer listen host: Passed=1 HostItems=Stable"));
            A8bAntidotePeerStage = 6;
        }
        return;
    }

    if (Character->IsLocallyControlled() && A8bAntidotePeerStage == 0
        && !bA8bAntidotePeerFixtureObserved)
    {
        AKalmalaConstructionActor* Cauldron = FindAntidoteTestCauldron(GetWorld());
        if (Cauldron)
        {
            int32 OwnerPlayerId = INDEX_NONE;
            if (!GetAntidoteOwnerId(Cauldron, OwnerPlayerId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer observer: Passed=0 invalid owner identity"));
                A8bAntidotePeerStage = 99;
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
                A8bAntidotePeerObservedOwnerPlayerId = OwnerPlayerId;
                bA8bAntidotePeerFixtureObserved = true;
                A8bAntidotePeerElapsed = 0.0f;
            }
        }
    }

    if (Character->IsLocallyControlled() && bA8bAntidotePeerFixtureObserved)
    {
        if (A8bAntidotePeerStage >= 1) return;
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == A8bAntidotePeerObservedOwnerPlayerId)
            {
                Owner = *It;
                break;
            }
        }
        const UKalmalaInventoryComponent* OwnerInventory = Owner
            ? Owner->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        const UKalmalaPlayerStatusComponent* OwnerStatus = Owner
            ? Owner->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
        const bool bPublicPoisonDamage = Owner && Owner->GetHealth() < A8bAntidotePeerObserverBaselineHealth - 0.1f;
        const bool bPrivate = Owner && !Owner->IsLocallyControlled() && OwnerInventory && OwnerStatus
            && OwnerInventory->GetStacks().IsEmpty() && OwnerStatus->GetStatuses().IsEmpty()
            && Inventory->GetQuantity(TEXT("VenomSac")) == 0
            && Inventory->GetQuantity(TEXT("Lingonberry")) == 0
            && Inventory->GetQuantity(TEXT("Antidote")) == 0;
        if (bPublicPoisonDamage && bPrivate)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Antidote peer observer: Passed=1 Authority=0 PublicPoisonDamage=1 TargetPrivate=1"));
            A8bAntidotePeerStage = 1;
        }
        else if (A8bAntidotePeerElapsed > 45.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Antidote peer observer: Passed=0 OwnerFound=%d PublicPoisonDamage=%d TargetPrivate=%d"),
                Owner ? 1 : 0, bPublicPoisonDamage ? 1 : 0, bPrivate ? 1 : 0);
            A8bAntidotePeerStage = 99;
        }
        return;
    }

    if (!Character->IsLocallyControlled()) return;
    if (A8bAntidotePeerStage == 0 && GetLastResult() == A8bAntidoteReadyResult)
    {
        if (Inventory->GetQuantity(TEXT("VenomSac")) < 2 || Inventory->GetQuantity(TEXT("Lingonberry")) < 4
            || GetRecipeAvailability(A8bAntidoteRecipe) != TEXT("Ready"))
        {
            if (A8bAntidotePeerElapsed < 20.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer owner: Passed=0 Cauldron, heat or recipe inputs did not replicate"));
            A8bAntidotePeerStage = 99;
            return;
        }
        A8bAntidotePeerBaselineVenomSac = Inventory->GetQuantity(TEXT("VenomSac")) - 2;
        A8bAntidotePeerBaselineLingonberry = Inventory->GetQuantity(TEXT("Lingonberry")) - 4;
        A8bAntidotePeerBaselineDose = Inventory->GetQuantity(TEXT("Antidote"));
        FString DirectReason;
        if (CraftFromServer(A8bAntidoteRecipe, 2, DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: direct client crafting bypassed server authority"));
            A8bAntidotePeerStage = 99;
            return;
        }
        A8bAntidotePeerResultSerial = GetResultSerial();
        ServerCraft(A8bAntidoteRecipe, 2);
        A8bAntidotePeerStage = 1;
        A8bAntidotePeerElapsed = 0.0f;
        return;
    }
    if (A8bAntidotePeerStage == 1)
    {
        if (GetResultSerial() == A8bAntidotePeerResultSerial)
        {
            if (A8bAntidotePeerElapsed > 25.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer owner: Passed=0 craft result timed out"));
                A8bAntidotePeerStage = 99;
            }
            return;
        }
        const FKalmalaInventoryStack* Antidote = FindAntidoteStack(Inventory);
        const bool bCraftPassed = GetLastResult().Contains(TEXT("Crafted 2 Antidote")) && WasLastResultAccepted()
            && Inventory->GetQuantity(TEXT("VenomSac")) == A8bAntidotePeerBaselineVenomSac
            && Inventory->GetQuantity(TEXT("Lingonberry")) == A8bAntidotePeerBaselineLingonberry
            && Antidote && Antidote->Quantity == A8bAntidotePeerBaselineDose + 2
            && Antidote->ItemGuids.Num() == Antidote->Quantity
            && Antidote->ItemGuids[0].IsValid() && Antidote->ItemGuids[1].IsValid()
            && Antidote->ItemGuids[0] != Antidote->ItemGuids[1];
        if (!bCraftPassed)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Antidote peer owner: Passed=0 Authority=0 Result=%s Accepted=%d Output=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0, Inventory->GetQuantity(TEXT("Antidote")));
            A8bAntidotePeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Antidote peer owner: Crafted=1 Authority=0 Payment=1 Output=2 UniqueGuid=1"));
        A8bAntidotePeerStage = 2;
        A8bAntidotePeerElapsed = 0.0f;
        return;
    }
    if (A8bAntidotePeerStage == 2 && GetLastResult() == A8bAntidotePoisonReadyResult)
    {
        const UKalmalaPlayerStatusComponent* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
        if (!Status || !Status->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId)
            || !Status->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId))
        {
            if (A8bAntidotePeerElapsed < 10.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer owner: Passed=0 weak/major poison setup did not replicate"));
            A8bAntidotePeerStage = 99;
            return;
        }
        FString DirectReason;
        if (ConsumeFoodFromServer(TEXT("Antidote"), DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer FAILED: direct client use bypassed server authority"));
            A8bAntidotePeerStage = 99;
            return;
        }
        A8bAntidotePeerResultSerial = GetResultSerial();
        ServerConsumeFood(TEXT("Antidote"));
        A8bAntidotePeerStage = 3;
        A8bAntidotePeerElapsed = 0.0f;
        return;
    }
    if (A8bAntidotePeerStage == 3)
    {
        if (GetResultSerial() == A8bAntidotePeerResultSerial)
        {
            if (A8bAntidotePeerElapsed > 25.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer owner: Passed=0 first dose use timed out"));
                A8bAntidotePeerStage = 99;
            }
            return;
        }
        const UKalmalaPlayerStatusComponent* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
        const bool bStatusApplied = Status && !Status->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId)
            && Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId) > 0.0f
            && Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId) <= 5.1f
            && Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId) > 590.0f
            && Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId) > 50.0f;
        if (!GetLastResult().StartsWith(TEXT("Used Antidote;")) || !WasLastResultAccepted()
            || Inventory->GetQuantity(TEXT("VenomSac")) != A8bAntidotePeerBaselineVenomSac
            || Inventory->GetQuantity(TEXT("Lingonberry")) != A8bAntidotePeerBaselineLingonberry
            || Inventory->GetQuantity(TEXT("Antidote")) != A8bAntidotePeerBaselineDose + 1 || !bStatusApplied)
        {
            if (A8bAntidotePeerElapsed < 10.0f) return;
            UE_LOG(LogTemp, Error,
                TEXT("A8b Antidote peer owner: Passed=0 Result=%s Accepted=%d Doses=%d StatusApplied=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0, Inventory->GetQuantity(TEXT("Antidote")),
                bStatusApplied ? 1 : 0);
            A8bAntidotePeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Antidote peer owner: Used=1 ClearedWeak=1 ReducedMajor=1 Resistance=1 Cooldown=1 DoseRemaining=1"));
        A8bAntidotePeerStage = 4;
        A8bAntidotePeerElapsed = 0.0f;
        return;
    }
    if (A8bAntidotePeerStage == 4 && GetLastResult() == A8bAntidoteResistanceReadyResult)
    {
        const UKalmalaPlayerStatusComponent* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
        if (!Status || !Status->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId)
            || !Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId)
            || Status->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId)
            || Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId) > 5.1f)
        {
            if (A8bAntidotePeerElapsed < 10.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer owner: Passed=0 resisted poison profile did not replicate"));
            A8bAntidotePeerStage = 99;
            return;
        }
        A8bAntidotePeerResultSerial = GetResultSerial();
        ServerConsumeFood(TEXT("Antidote"));
        A8bAntidotePeerStage = 5;
        A8bAntidotePeerElapsed = 0.0f;
        return;
    }
    if (A8bAntidotePeerStage == 5)
    {
        if (GetResultSerial() == A8bAntidotePeerResultSerial)
        {
            if (A8bAntidotePeerElapsed > 25.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Antidote peer owner: Passed=0 cooldown rejection timed out"));
                A8bAntidotePeerStage = 99;
            }
            return;
        }
        const UKalmalaPlayerStatusComponent* Status = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
        const bool bRejected = !WasLastResultAccepted()
            && GetLastResult().Contains(TEXT("Draught recovery cooldown is active"));
        const bool bStatePreserved = Inventory->GetQuantity(TEXT("Antidote")) == A8bAntidotePeerBaselineDose + 1
            && Status && Status->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId)
            && Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId);
        if (!bRejected || !bStatePreserved)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Antidote peer owner: Passed=0 Rejected=%d Preserved=%d Doses=%d Reason=%s"),
                bRejected ? 1 : 0, bStatePreserved ? 1 : 0, Inventory->GetQuantity(TEXT("Antidote")), *GetLastResult());
            A8bAntidotePeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Antidote peer owner: Passed=1 Authority=0 Resistance=1 CooldownReject=1 DosePreserved=1"));
        A8bAntidotePeerStage = 6;
    }
}

#endif
