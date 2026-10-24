#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaDamageProfile.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaPlayerStatusComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR A8bWolfPeltBootsRecipe[] = TEXT("WolfPeltBootsRecipe");
    constexpr TCHAR A8bWolfPeltBootsReadyResult[] = TEXT("A8b Wolf Pelt Boots peer ready");
    constexpr TCHAR A8bWolfPeltBootsCraftedResult[] = TEXT("Crafted 1 Wolf Pelt Boots");
    constexpr TCHAR A8bWolfPeltBootsArmorResult[] = TEXT("A8b Wolf Pelt Boots armor applied");
    constexpr TCHAR A8bWolfPeltBootsWorkbenchId[] = TEXT("A8bPeerWolfPeltBootsWorkbench");

    const FKalmalaInventoryStack* FindWolfPeltBootsStack(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory ? Inventory->GetStacks().FindByPredicate([](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == TEXT("WolfPeltBoots");
        }) : nullptr;
    }

    const FKalmalaEquippedGear* FindWolfPeltBootsEquipped(const UKalmalaInventoryComponent* Inventory)
    {
        const FKalmalaEquippedGear* Gear = Inventory
            ? Inventory->FindEquippedGear(EKalmalaGearSlot::Boots) : nullptr;
        return Gear && Gear->ItemId == TEXT("WolfPeltBoots") ? Gear : nullptr;
    }

    AKalmalaConstructionActor* FindWolfPeltBootsWorkbench(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (It->GetConstructionId().StartsWith(FString(A8bWolfPeltBootsWorkbenchId) + TEXT("_"))) return *It;
        }
        return nullptr;
    }

    bool GetWolfPeltBootsOwnerId(const AKalmalaConstructionActor* Workbench, int32& OutPlayerId)
    {
        if (!Workbench) return false;
        const FString Prefix = FString(A8bWolfPeltBootsWorkbenchId) + TEXT("_");
        if (!Workbench->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Workbench->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }
}

void UKalmalaCraftingComponent::RunA8bWolfPeltBootsPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    A8bWolfPeltBootsPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || A8bWolfPeltBootsPeerStage >= 4) return;
        if (A8bWolfPeltBootsPeerStage == 0)
        {
            if (A8bWolfPeltBootsPeerElapsed < 3.0f) return;
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

            A8bWolfPeltBootsPeerHostBaselineWolfPelt = Inventory->GetQuantity(TEXT("WolfPelt"));
            A8bWolfPeltBootsPeerHostBaselineIron = Inventory->GetQuantity(TEXT("Iron"));
            A8bWolfPeltBootsPeerHostBaselineBoots = Inventory->GetQuantity(TEXT("WolfPeltBoots"));
            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            if (!TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: remote owner components are incomplete"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            A8bWolfPeltBootsPeerBaselineWolfPelt = TargetInventory->GetQuantity(TEXT("WolfPelt"));
            A8bWolfPeltBootsPeerBaselineIron = TargetInventory->GetQuantity(TEXT("Iron"));
            A8bWolfPeltBootsPeerBaselineBoots = TargetInventory->GetQuantity(TEXT("WolfPeltBoots"));
            A8bWolfPeltBootsPeerBaselineHealth = Target->GetHealth();
            if (!FMath::IsNearlyEqual(A8bWolfPeltBootsPeerBaselineHealth, 100.0f, 0.1f))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Wolf Pelt Boots peer FAILED: owner health baseline is %.1f"),
                    A8bWolfPeltBootsPeerBaselineHealth);
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            if (!TargetInventory->TryGrantFromServer(TEXT("WolfPelt"), 5)
                || !TargetInventory->TryGrantFromServer(TEXT("Iron"), 5))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: could not stage recipe materials"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            const FVector WorkbenchLocation = Target->GetActorLocation() + Target->GetActorRightVector() * 150.0f;
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Workbench = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, WorkbenchLocation), SpawnParameters);
            if (!Workbench)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: Workbench did not spawn"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            const FString ConstructionId = FString::Printf(TEXT("%s_%d"), A8bWolfPeltBootsWorkbenchId,
                Target->GetPlayerState()->GetPlayerId());
            Workbench->InitializeFromServer(TEXT("WorkbenchKit"), ConstructionId);
            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Workbench->GetActorLocation())
                    <= FMath::Square(250.0f)
                && TargetCrafting->GetRecipeAvailability(A8bWolfPeltBootsRecipe) == TEXT("Ready");
            if (!bStationReady)
            {
                Workbench->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: Workbench recipe is not available"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            A8bWolfPeltBootsPeerTarget = Target;
            TargetCrafting->PublishResult(A8bWolfPeltBootsReadyResult, true);
            A8bWolfPeltBootsPeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Workbench->ForceNetUpdate();
            A8bWolfPeltBootsPeerStage = 1;
            A8bWolfPeltBootsPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Wolf Pelt Boots peer fixture: Ready=1 RemoteOwner=1 WolfPelt=5 Iron=5 Workbench=1"));
        }
        else if (A8bWolfPeltBootsPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bWolfPeltBootsPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: owner disappeared before craft result"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bWolfPeltBootsPeerResultSerial)
            {
                if (A8bWolfPeltBootsPeerElapsed > 20.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer server: Passed=0 craft result timed out"));
                    A8bWolfPeltBootsPeerStage = 99;
                }
                return;
            }

            const FKalmalaInventoryStack* Boots = FindWolfPeltBootsStack(TargetInventory);
            const bool bGuid = Boots && Boots->Quantity == A8bWolfPeltBootsPeerBaselineBoots + 1
                && Boots->ItemGuids.Num() == Boots->Quantity
                && Boots->ItemGuids.ContainsByPredicate([](const FGuid& Guid) { return Guid.IsValid(); });
            const bool bCraftPassed = TargetCrafting->GetLastResult() == A8bWolfPeltBootsCraftedResult
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("WolfPelt")) == A8bWolfPeltBootsPeerBaselineWolfPelt
                && TargetInventory->GetQuantity(TEXT("Iron")) == A8bWolfPeltBootsPeerBaselineIron
                && TargetInventory->GetQuantity(TEXT("WolfPeltBoots")) == A8bWolfPeltBootsPeerBaselineBoots + 1
                && bGuid;
            if (!bCraftPassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Wolf Pelt Boots peer server: Passed=0 Result=%s Accepted=%d WolfPelt=%d Iron=%d Output=%d Guid=%d"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("WolfPelt")), TargetInventory->GetQuantity(TEXT("Iron")),
                    TargetInventory->GetQuantity(TEXT("WolfPeltBoots")), bGuid ? 1 : 0);
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            A8bWolfPeltBootsPeerItemGuid = Boots->ItemGuids[0];
            A8bWolfPeltBootsPeerStage = 2;
            A8bWolfPeltBootsPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display, TEXT("A8b Wolf Pelt Boots peer server craft: Accepted=1 Payment=1 Output=1 Guid=1"));
        }
        else if (A8bWolfPeltBootsPeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bWolfPeltBootsPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            const FKalmalaEquippedGear* Boots = FindWolfPeltBootsEquipped(TargetInventory);
            if (!Target || !TargetInventory || !TargetCrafting || !Boots)
            {
                if (A8bWolfPeltBootsPeerElapsed < 30.0f) return;
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer server: Passed=0 remote equip timed out"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            if (Boots->ItemGuid != A8bWolfPeltBootsPeerItemGuid || Boots->ConditionSpent != 0)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: equipped Boots identity or condition is invalid"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            FKalmalaGearLevelStats GearStats;
            const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
            const bool bStats = Catalogue && Catalogue->GetGearLevelStats(TEXT("WolfPeltBoots"), 1, GearStats)
                && GearStats.Armor == 10 && GearStats.MaxDurability == 120;
            const UKalmalaResistanceComponent* Defenses = Target->FindComponentByClass<UKalmalaResistanceComponent>();
            FKalmalaDamageChannels Incoming;
            Incoming.Crush = 20.0f;
            float ExpectedDamage = 0.0f;
            const bool bDamageProfile = Defenses && Defenses->Profile.Resolve(Incoming, ExpectedDamage,
                bStats ? GearStats.Armor : 0) && FMath::IsNearlyEqual(ExpectedDamage, 10.0f, 0.1f);
            if (!bStats || !bDamageProfile || TargetInventory->GetQuantity(TEXT("WolfPelt")) != A8bWolfPeltBootsPeerBaselineWolfPelt
                || TargetInventory->GetQuantity(TEXT("Iron")) != A8bWolfPeltBootsPeerBaselineIron
                || TargetInventory->GetQuantity(TEXT("WolfPeltBoots")) != A8bWolfPeltBootsPeerBaselineBoots)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Wolf Pelt Boots peer server: Passed=0 Profile=%d Damage=%d Materials=%d"),
                    bStats ? 1 : 0, bDamageProfile ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("WolfPelt")) == A8bWolfPeltBootsPeerBaselineWolfPelt
                        && TargetInventory->GetQuantity(TEXT("Iron")) == A8bWolfPeltBootsPeerBaselineIron ? 1 : 0);
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector SourceLocation = Target->GetActorLocation() + Target->GetActorRightVector() * 100.0f;
            AKalmalaWildlifeSpawn* DamageSource = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(
                AKalmalaWildlifeSpawn::StaticClass(), FTransform(FRotator::ZeroRotator, SourceLocation), SpawnParameters);
            if (!DamageSource)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: armor damage source did not spawn"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            const float ExpectedHealth = FMath::Clamp(A8bWolfPeltBootsPeerBaselineHealth - ExpectedDamage, 1.0f, 100.0f);
            const bool bDamageAccepted = Target->ApplyWildlifeDamageFromServer(DamageSource, Incoming);
            DamageSource->Destroy();
            const float HealthAfter = Target->GetHealth();
            const FKalmalaEquippedGear* WornBoots = FindWolfPeltBootsEquipped(TargetInventory);
            const bool bArmorApplied = bDamageAccepted
                && FMath::IsNearlyEqual(HealthAfter, ExpectedHealth, 0.1f)
                && WornBoots && WornBoots->ConditionSpent == 1;
            if (!bArmorApplied)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Wolf Pelt Boots peer server: Passed=0 DamageAccepted=%d Armor=%d Health=%.1f/%.1f Condition=%d"),
                    bDamageAccepted ? 1 : 0, GearStats.Armor, HealthAfter, ExpectedHealth,
                    WornBoots ? WornBoots->ConditionSpent : INDEX_NONE);
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }

            TargetCrafting->PublishResult(A8bWolfPeltBootsArmorResult, true);
            Target->ForceNetUpdate();
            A8bWolfPeltBootsPeerStage = 3;
            A8bWolfPeltBootsPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Wolf Pelt Boots peer server: Passed=1 Craft=1 Payment=1 Output=1 Guid=1 Equip=1 Armor=10 Damage=10 Condition=1"));
        }
        else if (A8bWolfPeltBootsPeerStage == 3)
        {
            const AKalmalaCharacter* Target = A8bWolfPeltBootsPeerTarget.Get();
            const bool bHostItemsStable = Inventory->GetQuantity(TEXT("WolfPelt")) == A8bWolfPeltBootsPeerHostBaselineWolfPelt
                && Inventory->GetQuantity(TEXT("Iron")) == A8bWolfPeltBootsPeerHostBaselineIron
                && Inventory->GetQuantity(TEXT("WolfPeltBoots")) == A8bWolfPeltBootsPeerHostBaselineBoots;
            const bool bFinalOwnerState = Target
                && Target->GetHealth() <= A8bWolfPeltBootsPeerBaselineHealth - 9.9f;
            if (!bHostItemsStable || !bFinalOwnerState)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Wolf Pelt Boots peer listen host: Passed=0 HostItems=%d OwnerHealth=%d"),
                    bHostItemsStable ? 1 : 0, bFinalOwnerState ? 1 : 0);
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("A8b Wolf Pelt Boots peer listen host: Passed=1 HostItems=Stable"));
            A8bWolfPeltBootsPeerStage = 4;
        }
        return;
    }

    if (Character->IsLocallyControlled() && A8bWolfPeltBootsPeerStage == 0
        && !bA8bWolfPeltBootsPeerFixtureObserved)
    {
        AKalmalaConstructionActor* Workbench = FindWolfPeltBootsWorkbench(GetWorld());
        if (Workbench)
        {
            int32 OwnerPlayerId = INDEX_NONE;
            if (!GetWolfPeltBootsOwnerId(Workbench, OwnerPlayerId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer observer: Passed=0 invalid owner identity"));
                A8bWolfPeltBootsPeerStage = 99;
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
                A8bWolfPeltBootsPeerObservedOwnerPlayerId = OwnerPlayerId;
                A8bWolfPeltBootsPeerObserverBaselineHealth = 100.0f;
                bA8bWolfPeltBootsPeerFixtureObserved = true;
                A8bWolfPeltBootsPeerElapsed = 0.0f;
            }
        }
    }

    if (Character->IsLocallyControlled() && bA8bWolfPeltBootsPeerFixtureObserved)
    {
        if (A8bWolfPeltBootsPeerStage >= 1) return;
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == A8bWolfPeltBootsPeerObservedOwnerPlayerId)
            {
                Owner = *It;
                break;
            }
        }
        const UKalmalaInventoryComponent* OwnerInventory = Owner
            ? Owner->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        const UKalmalaPlayerStatusComponent* OwnerStatus = Owner
            ? Owner->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
        const bool bPublicArmorDamage = Owner && Owner->GetHealth() <= A8bWolfPeltBootsPeerObserverBaselineHealth - 9.9f;
        const bool bPrivate = Owner && !Owner->IsLocallyControlled() && OwnerInventory && OwnerStatus
            && OwnerInventory->GetStacks().IsEmpty() && OwnerInventory->GetEquippedGear().IsEmpty()
            && Owner->GetCarriedToolInventory().IsEmpty() && OwnerStatus->GetStatuses().IsEmpty()
            && Inventory->GetQuantity(TEXT("WolfPelt")) == 0
            && Inventory->GetQuantity(TEXT("Iron")) == 0
            && Inventory->GetQuantity(TEXT("WolfPeltBoots")) == 0;
        if (bPublicArmorDamage && bPrivate)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Wolf Pelt Boots peer observer: Passed=1 Authority=0 PublicArmorDamage=1 TargetPrivate=1"));
            A8bWolfPeltBootsPeerStage = 1;
        }
        else if (A8bWolfPeltBootsPeerElapsed > 40.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Wolf Pelt Boots peer observer: Passed=0 OwnerFound=%d PublicArmorDamage=%d TargetPrivate=%d"),
                Owner ? 1 : 0, bPublicArmorDamage ? 1 : 0, bPrivate ? 1 : 0);
            A8bWolfPeltBootsPeerStage = 99;
        }
        return;
    }

    if (Character->IsLocallyControlled())
    {
        if (A8bWolfPeltBootsPeerStage == 0 && GetLastResult() == A8bWolfPeltBootsReadyResult)
        {
            if (Inventory->GetQuantity(TEXT("WolfPelt")) < 5 || Inventory->GetQuantity(TEXT("Iron")) < 5
                || GetRecipeAvailability(A8bWolfPeltBootsRecipe) != TEXT("Ready"))
            {
                if (A8bWolfPeltBootsPeerElapsed < 15.0f) return;
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer owner: Passed=0 Workbench or recipe inputs did not replicate"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            A8bWolfPeltBootsPeerBaselineWolfPelt = Inventory->GetQuantity(TEXT("WolfPelt")) - 5;
            A8bWolfPeltBootsPeerBaselineIron = Inventory->GetQuantity(TEXT("Iron")) - 5;
            A8bWolfPeltBootsPeerBaselineBoots = Inventory->GetQuantity(TEXT("WolfPeltBoots"));
            A8bWolfPeltBootsPeerBaselineHealth = Character->GetHealth();
            FString DirectReason;
            if (CraftFromServer(A8bWolfPeltBootsRecipe, 1, DirectReason))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: direct client craft bypassed server authority"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            A8bWolfPeltBootsPeerResultSerial = GetResultSerial();
            ServerCraft(A8bWolfPeltBootsRecipe, 1);
            A8bWolfPeltBootsPeerStage = 1;
            A8bWolfPeltBootsPeerElapsed = 0.0f;
            return;
        }
        if (A8bWolfPeltBootsPeerStage == 1)
        {
            if (GetResultSerial() == A8bWolfPeltBootsPeerResultSerial)
            {
                if (A8bWolfPeltBootsPeerElapsed > 20.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer owner: Passed=0 craft result timed out"));
                    A8bWolfPeltBootsPeerStage = 99;
                }
                return;
            }
            const FKalmalaInventoryStack* Boots = FindWolfPeltBootsStack(Inventory);
            const bool bCraftPassed = GetLastResult() == A8bWolfPeltBootsCraftedResult && WasLastResultAccepted()
                && Inventory->GetQuantity(TEXT("WolfPelt")) == A8bWolfPeltBootsPeerBaselineWolfPelt
                && Inventory->GetQuantity(TEXT("Iron")) == A8bWolfPeltBootsPeerBaselineIron
                && Inventory->GetQuantity(TEXT("WolfPeltBoots")) == A8bWolfPeltBootsPeerBaselineBoots + 1
                && Boots && Boots->ItemGuids.Num() == Boots->Quantity
                && Boots->ItemGuids.ContainsByPredicate([](const FGuid& Guid) { return Guid.IsValid(); });
            if (!bCraftPassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Wolf Pelt Boots peer owner: Passed=0 Result=%s Accepted=%d WolfPelt=%d Iron=%d Output=%d"),
                    *GetLastResult(), WasLastResultAccepted() ? 1 : 0,
                    Inventory->GetQuantity(TEXT("WolfPelt")), Inventory->GetQuantity(TEXT("Iron")),
                    Inventory->GetQuantity(TEXT("WolfPeltBoots")));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            A8bWolfPeltBootsPeerItemGuid = Boots->ItemGuids[0];
            FString DirectReason;
            if (Inventory->EquipGearFromServer(A8bWolfPeltBootsPeerItemGuid, DirectReason))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer FAILED: direct client equip bypassed server authority"));
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            A8bWolfPeltBootsPeerGearRequestSerial = Inventory->GetGearRequestResultSerial();
            Inventory->ServerEquipGear(A8bWolfPeltBootsPeerItemGuid);
            A8bWolfPeltBootsPeerStage = 2;
            A8bWolfPeltBootsPeerElapsed = 0.0f;
            return;
        }
        if (A8bWolfPeltBootsPeerStage == 2)
        {
            if (Inventory->GetGearRequestResultSerial() == A8bWolfPeltBootsPeerGearRequestSerial)
            {
                if (A8bWolfPeltBootsPeerElapsed > 30.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer owner: Passed=0 gear RPC timed out"));
                    A8bWolfPeltBootsPeerStage = 99;
                }
                return;
            }
            if (!Inventory->WasLastGearRequestAccepted())
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Wolf Pelt Boots peer owner: Passed=0 GearResult=%s"),
                    *Inventory->GetLastGearRequestReason());
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            const FKalmalaEquippedGear* Boots = FindWolfPeltBootsEquipped(Inventory);
            const bool bAcceptedDamage = GetLastResult() == A8bWolfPeltBootsArmorResult
                && Character->GetHealth() <= A8bWolfPeltBootsPeerBaselineHealth - 9.9f;
            if (!Boots || Boots->ItemGuid != A8bWolfPeltBootsPeerItemGuid || Boots->ConditionSpent != 1
                || Inventory->GetQuantity(TEXT("WolfPelt")) != A8bWolfPeltBootsPeerBaselineWolfPelt
                || Inventory->GetQuantity(TEXT("Iron")) != A8bWolfPeltBootsPeerBaselineIron
                || Inventory->GetQuantity(TEXT("WolfPeltBoots")) != A8bWolfPeltBootsPeerBaselineBoots
                || !bAcceptedDamage)
            {
                if (A8bWolfPeltBootsPeerElapsed < 15.0f) return;
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Wolf Pelt Boots peer owner: Passed=0 Equip=%d Condition=%d ArmorResult=%d Health=%.1f"),
                    Boots && Boots->ItemGuid == A8bWolfPeltBootsPeerItemGuid ? 1 : 0,
                    Boots ? Boots->ConditionSpent : INDEX_NONE,
                    GetLastResult() == A8bWolfPeltBootsArmorResult ? 1 : 0, Character->GetHealth());
                A8bWolfPeltBootsPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("A8b Wolf Pelt Boots peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Equip=1 ArmorDamage=1 Condition=1 Health=%.1f"),
                Character->GetHealth());
            A8bWolfPeltBootsPeerStage = 3;
        }
        return;
    }

}

#endif

#include "KalmalaA8bFangSpearPeerVerification.inl"
