#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR A8bVestRecipe[] = TEXT("BadgerFurVestRecipe");
    constexpr TCHAR A8bVestReadyResult[] = TEXT("A8b Badger Fur Vest peer ready");
    constexpr TCHAR A8bVestCraftedResult[] = TEXT("Crafted 1 Badger Fur Vest");
    constexpr TCHAR A8bVestArmorResult[] = TEXT("A8b Badger Fur Vest armor applied");
    constexpr TCHAR A8bVestWorkbenchId[] = TEXT("A8bPeerVestWorkbench");

    const FKalmalaInventoryStack* FindVestStack(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory ? Inventory->GetStacks().FindByPredicate([](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == TEXT("BadgerFurVest");
        }) : nullptr;
    }

    AKalmalaConstructionActor* FindVestTestWorkbench(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (It->GetConstructionId().StartsWith(FString(A8bVestWorkbenchId) + TEXT("_"))) return *It;
        }
        return nullptr;
    }

    bool GetVestTestOwnerPlayerId(const AKalmalaConstructionActor* Workbench, int32& OutPlayerId)
    {
        if (!Workbench) return false;
        const FString Prefix = FString(A8bVestWorkbenchId) + TEXT("_");
        if (!Workbench->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Workbench->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }

    const FKalmalaEquippedGear* FindVest(const UKalmalaInventoryComponent* Inventory)
    {
        const FKalmalaEquippedGear* Gear = Inventory
            ? Inventory->FindEquippedGear(EKalmalaGearSlot::Chest) : nullptr;
        return Gear && Gear->ItemId == TEXT("BadgerFurVest") ? Gear : nullptr;
    }
}

void UKalmalaCraftingComponent::RunA8bVestPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    A8bVestPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        // The listen host orchestrates a real owner-client craft and equip RPC.
        if (!Character->IsLocallyControlled() || A8bVestPeerStage >= 4) return;
        if (A8bVestPeerStage == 0)
        {
            if (A8bVestPeerElapsed < 3.0f) return;
            AKalmalaCharacter* Target = nullptr;
            int32 RemoteCount = 0;
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                APlayerController* Controller = Cast<APlayerController>(It->GetController());
                if (!It->GetPlayerState() || !Controller || Controller->IsLocalController()) continue;
                ++RemoteCount;
                if (!Target || It->GetPlayerState()->GetPlayerId() < Target->GetPlayerState()->GetPlayerId())
                    Target = *It;
            }
            // The runner joins the owner first, then an observer. Wait for both.
            if (RemoteCount < 2 || !Target) return;

            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            const bool bBaseline = TargetInventory
                && TargetInventory->GetQuantity(TEXT("BadgerFur")) == 0
                && TargetInventory->GetQuantity(TEXT("BadgerFurVest")) == 0
                && TargetInventory->FindEquippedGear(EKalmalaGearSlot::Chest) == nullptr
                && FMath::IsNearlyEqual(Target->GetHealth(), 100.0f, 0.1f);
            if (!bBaseline || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: remote owner baseline or crafting component is invalid"));
                A8bVestPeerStage = 99;
                return;
            }

            const FVector BenchLocation = Target->GetActorLocation() + FVector(150.0f, 0.0f, 0.0f);
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Workbench = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, BenchLocation), SpawnParameters);
            if (!Workbench)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: Workbench did not spawn"));
                A8bVestPeerStage = 99;
                return;
            }

            const FString WorkbenchId = FString::Printf(TEXT("%s_%d"), A8bVestWorkbenchId,
                Target->GetPlayerState()->GetPlayerId());
            Workbench->InitializeFromServer(TEXT("WorkbenchKit"), WorkbenchId);
            const bool bMaterials = TargetInventory->TryGrantFromServer(TEXT("BadgerFur"), 6);
            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Workbench->GetActorLocation())
                    <= FMath::Square(250.0f)
                && TargetCrafting->GetRecipeAvailability(A8bVestRecipe) == TEXT("Ready");
            if (!bMaterials || !bStationReady)
            {
                Workbench->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: materials=%d station=%d"),
                    bMaterials ? 1 : 0, bStationReady ? 1 : 0);
                A8bVestPeerStage = 99;
                return;
            }

            A8bVestPeerTarget = Target;
            TargetCrafting->PublishResult(A8bVestReadyResult, true);
            A8bVestPeerReadyResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Workbench->ForceNetUpdate();
            A8bVestPeerStage = 1;
            A8bVestPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display, TEXT("A8b Vest peer fixture: Ready=1 RemoteOwner=1 BadgerFur=6 Workbench=1"));
        }
        else if (A8bVestPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bVestPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: remote owner disappeared before craft result"));
                A8bVestPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bVestPeerReadyResultSerial) return;

            const FKalmalaInventoryStack* Vest = FindVestStack(TargetInventory);
            const bool bGuid = Vest && Vest->Quantity == 1 && Vest->ItemGuids.Num() == 1
                && Vest->ItemGuids[0].IsValid();
            const bool bCraftPassed = TargetCrafting->GetLastResult() == A8bVestCraftedResult
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("BadgerFur")) == 0
                && TargetInventory->GetQuantity(TEXT("BadgerFurVest")) == 1 && bGuid;
            if (!bCraftPassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Vest peer server: Passed=0 Result=%s Accepted=%d Fur=%d Vest=%d Guid=%d"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("BadgerFur")), TargetInventory->GetQuantity(TEXT("BadgerFurVest")),
                    bGuid ? 1 : 0);
                A8bVestPeerStage = 99;
                return;
            }

            A8bVestPeerItemGuid = Vest->ItemGuids[0];
            A8bVestPeerStage = 2;
            A8bVestPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display, TEXT("A8b Vest peer server craft: Accepted=1 Payment=1 Output=1 Guid=1"));
        }
        else if (A8bVestPeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bVestPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            const FKalmalaEquippedGear* Vest = FindVest(TargetInventory);
            if (!Target || !TargetInventory || !TargetCrafting || !Vest)
            {
                if (A8bVestPeerElapsed < 30.0f) return;
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: remote ServerEquipGear request timed out"));
                A8bVestPeerStage = 99;
                return;
            }
            if (Vest->ItemGuid != A8bVestPeerItemGuid)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: equipped chest slot has the wrong item GUID"));
                A8bVestPeerStage = 99;
                return;
            }

            FKalmalaGearLevelStats GearStats;
            const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
            const bool bStats = Catalogue && Catalogue->GetGearLevelStats(TEXT("BadgerFurVest"), 1, GearStats)
                && GearStats.Armor == 6 && GearStats.MaxDurability == 100;
            const UKalmalaResistanceComponent* Defenses = Target->FindComponentByClass<UKalmalaResistanceComponent>();
            FKalmalaDamageChannels Incoming;
            Incoming.Crush = 10.0f;
            float ExpectedDamage = 0.0f;
            const bool bDamageProfile = Defenses && Defenses->Profile.Resolve(Incoming, ExpectedDamage,
                bStats ? GearStats.Armor : 0);
            AKalmalaWildlifeSpawn* DamageSource = nullptr;
            if (bStats && bDamageProfile && ExpectedDamage > 0.0f && Target->GetHealth() > 1.0f
                && Vest->ConditionSpent == 0 && TargetInventory->GetQuantity(TEXT("BadgerFur")) == 0
                && TargetInventory->GetQuantity(TEXT("BadgerFurVest")) == 0)
            {
                FActorSpawnParameters SpawnParameters;
                SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                const FVector SourceLocation = Target->GetActorLocation() + FVector(100.0f, 0.0f, 0.0f);
                DamageSource = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(
                    AKalmalaWildlifeSpawn::StaticClass(), FTransform(FRotator::ZeroRotator, SourceLocation), SpawnParameters);
            }
            if (!DamageSource)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Vest peer FAILED: profile=%d damage=%d source=%d"), bStats ? 1 : 0,
                    bDamageProfile ? 1 : 0, DamageSource ? 1 : 0);
                A8bVestPeerStage = 99;
                return;
            }

            const float HealthBefore = Target->GetHealth();
            const float ExpectedHealth = FMath::Clamp(HealthBefore - ExpectedDamage, 1.0f, 100.0f);
            const bool bDamageAccepted = Target->ApplyWildlifeDamageFromServer(DamageSource, Incoming);
            DamageSource->Destroy();
            const float HealthAfter = Target->GetHealth();
            const FKalmalaEquippedGear* WornVest = FindVest(TargetInventory);
            const bool bArmorApplied = bDamageAccepted && FMath::IsNearlyEqual(HealthAfter, ExpectedHealth, 0.1f)
                && WornVest && WornVest->ConditionSpent == 1;
            if (!bArmorApplied)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Vest peer server: Passed=0 DamageAccepted=%d Armor=%d Health=%.1f/%.1f Condition=%d"),
                    bDamageAccepted ? 1 : 0, GearStats.Armor, HealthAfter, ExpectedHealth,
                    WornVest ? WornVest->ConditionSpent : INDEX_NONE);
                A8bVestPeerStage = 99;
                return;
            }

            TargetCrafting->PublishResult(A8bVestArmorResult, true);
            Target->ForceNetUpdate();
            A8bVestPeerStage = 3;
            A8bVestPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Vest peer server: Passed=1 RemoteOwner=1 Craft=1 Payment=1 Guid=1 Equip=1 Armor=%d Damage=%.1f Health=%.1f Condition=1"),
                GearStats.Armor, ExpectedDamage, HealthAfter);
        }
        else if (A8bVestPeerStage == 3)
        {
            const FKalmalaEquippedGear* HostVest = FindVest(Inventory);
            const bool bPrivate = Inventory->GetQuantity(TEXT("BadgerFur")) == 0
                && Inventory->GetQuantity(TEXT("BadgerFurVest")) == 0 && HostVest == nullptr;
            if (bPrivate)
            {
                UE_LOG(LogTemp, Display, TEXT("A8b Vest peer listen host: Passed=1 OwnVest=0"));
                A8bVestPeerStage = 4;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer listen host: Passed=0 OwnFur=%d OwnVest=%d"),
                    Inventory->GetQuantity(TEXT("BadgerFur")), Inventory->GetQuantity(TEXT("BadgerFurVest")));
                A8bVestPeerStage = 99;
            }
        }
        return;
    }

    if (!Character->IsLocallyControlled()) return;
    if (A8bVestPeerStage == 0 && GetLastResult() == A8bVestReadyResult)
    {
        if (Inventory->GetQuantity(TEXT("BadgerFur")) != 6
            || GetRecipeAvailability(A8bVestRecipe) != TEXT("Ready")) return;
        A8bVestPeerBaselineHealth = Character->GetHealth();
        if (!FMath::IsNearlyEqual(A8bVestPeerBaselineHealth, 100.0f, 0.1f))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: owner health baseline is %.1f"), A8bVestPeerBaselineHealth);
            A8bVestPeerStage = 99;
            return;
        }
        FString DirectReason;
        if (CraftFromServer(A8bVestRecipe, 1, DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: client direct craft bypassed server authority"));
            A8bVestPeerStage = 99;
            return;
        }
        A8bVestPeerReadyResultSerial = GetResultSerial();
        ServerCraft(A8bVestRecipe, 1);
        A8bVestPeerStage = 1;
        A8bVestPeerElapsed = 0.0f;
        return;
    }
    if (A8bVestPeerStage == 1)
    {
        if (GetResultSerial() == A8bVestPeerReadyResultSerial || GetLastResult() == A8bVestReadyResult) return;
        if (GetLastResult() != A8bVestCraftedResult || !WasLastResultAccepted())
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer owner: Passed=0 CraftResult=%s Accepted=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0);
            A8bVestPeerStage = 99;
            return;
        }
        const FKalmalaInventoryStack* Vest = FindVestStack(Inventory);
        if (!Vest)
        {
            if (A8bVestPeerElapsed < 15.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer owner: Passed=0 Crafted vest did not replicate"));
            A8bVestPeerStage = 99;
            return;
        }
        const bool bGuid = Vest->Quantity == 1 && Vest->ItemGuids.Num() == 1 && Vest->ItemGuids[0].IsValid();
        if (!bGuid || Inventory->GetQuantity(TEXT("BadgerFur")) != 0)
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer owner: Passed=0 Output=%d Fur=%d Guid=%d"),
                Vest->Quantity, Inventory->GetQuantity(TEXT("BadgerFur")), bGuid ? 1 : 0);
            A8bVestPeerStage = 99;
            return;
        }
        A8bVestPeerItemGuid = Vest->ItemGuids[0];
        FString DirectReason;
        if (Inventory->EquipGearFromServer(A8bVestPeerItemGuid, DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer FAILED: client direct equip bypassed server authority"));
            A8bVestPeerStage = 99;
            return;
        }
        A8bVestPeerGearRequestSerial = Inventory->GetGearRequestResultSerial();
        Inventory->ServerEquipGear(A8bVestPeerItemGuid);
        A8bVestPeerStage = 2;
        A8bVestPeerElapsed = 0.0f;
        return;
    }
    if (A8bVestPeerStage == 2)
    {
        if (Inventory->GetGearRequestResultSerial() == A8bVestPeerGearRequestSerial)
        {
            if (A8bVestPeerElapsed < 30.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer owner: Passed=0 Gear RPC did not return"));
            A8bVestPeerStage = 99;
            return;
        }
        if (!Inventory->WasLastGearRequestAccepted())
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Vest peer owner: Passed=0 GearResult=%s"), *Inventory->GetLastGearRequestReason());
            A8bVestPeerStage = 99;
            return;
        }
        const FKalmalaEquippedGear* Vest = FindVest(Inventory);
        if (GetLastResult() != A8bVestArmorResult || !Vest || Vest->ItemGuid != A8bVestPeerItemGuid
            || Vest->ConditionSpent != 1 || Inventory->GetQuantity(TEXT("BadgerFur")) != 0
            || Inventory->GetQuantity(TEXT("BadgerFurVest")) != 0
            || Character->GetHealth() >= A8bVestPeerBaselineHealth)
        {
            if (A8bVestPeerElapsed < 15.0f) return;
            UE_LOG(LogTemp, Error,
                TEXT("A8b Vest peer owner: Passed=0 Equip=1 ArmorResult=%d Condition=%d Health=%.1f"),
                GetLastResult() == A8bVestArmorResult ? 1 : 0, Vest ? Vest->ConditionSpent : INDEX_NONE,
                Character->GetHealth());
            A8bVestPeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Vest peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Guid=1 Equip=1 ArmorDamage=1 Condition=1 Health=%.1f"),
            Character->GetHealth());
        A8bVestPeerStage = 3;
        return;
    }

    // The second remote is an observer. Public health confirms the server's
    // armor hit occurred; its target inventory and gear replicas must stay private.
    if (A8bVestPeerStage == 0)
    {
        AKalmalaConstructionActor* Workbench = FindVestTestWorkbench(GetWorld());
        if (!Workbench) return;
        if (!bA8bVestPeerFixtureObserved)
        {
            bA8bVestPeerFixtureObserved = true;
            if (!GetVestTestOwnerPlayerId(Workbench, A8bVestPeerObservedOwnerPlayerId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Vest peer observer: Passed=0 Invalid owner identity"));
                A8bVestPeerStage = 99;
                return;
            }
            A8bVestPeerElapsed = 0.0f;
        }
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == A8bVestPeerObservedOwnerPlayerId)
            {
                Owner = *It;
                break;
            }
        }
        if (!Owner || Owner->GetHealth() >= 100.0f) return;
        if (A8bVestPeerElapsed < 1.0f) return;
        UKalmalaInventoryComponent* OwnerInventory = Owner->FindComponentByClass<UKalmalaInventoryComponent>();
        const bool bPrivate = OwnerInventory && OwnerInventory->GetStacks().IsEmpty()
            && OwnerInventory->GetEquippedGear().IsEmpty()
            && Inventory->GetQuantity(TEXT("BadgerFur")) == 0
            && Inventory->GetQuantity(TEXT("BadgerFurVest")) == 0
            && FindVest(Inventory) == nullptr;
        if (bPrivate)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Vest peer observer: Passed=1 Authority=0 TargetPrivate=1 PublicHealth=%.1f Items=0/0"),
                Owner->GetHealth());
            A8bVestPeerStage = 4;
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Vest peer observer: Passed=0 Authority=0 TargetPrivate=%d OwnItems=%d/%d"),
                OwnerInventory && OwnerInventory->GetStacks().IsEmpty()
                    && OwnerInventory->GetEquippedGear().IsEmpty() ? 1 : 0,
                Inventory->GetQuantity(TEXT("BadgerFur")), Inventory->GetQuantity(TEXT("BadgerFurVest")));
            A8bVestPeerStage = 99;
        }
    }
}

#endif
