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
    constexpr TCHAR A8bScaleCuirassRecipe[] = TEXT("ScaleCuirassRecipe");
    constexpr TCHAR A8bScaleCuirassReadyResult[] = TEXT("A8b Scale Cuirass peer ready");
    constexpr TCHAR A8bScaleCuirassCraftedResult[] = TEXT("Crafted 1 Scale Cuirass");
    constexpr TCHAR A8bScaleCuirassArmorResult[] = TEXT("A8b Scale Cuirass armor applied");
    constexpr TCHAR A8bScaleCuirassForgeId[] = TEXT("A8bPeerScaleForge");

    const FKalmalaInventoryStack* FindScaleCuirassStack(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory ? Inventory->GetStacks().FindByPredicate([](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == TEXT("ScaleCuirass");
        }) : nullptr;
    }

    const FKalmalaEquippedGear* FindScaleCuirassChest(const UKalmalaInventoryComponent* Inventory)
    {
        const FKalmalaEquippedGear* Gear = Inventory
            ? Inventory->FindEquippedGear(EKalmalaGearSlot::Chest) : nullptr;
        return Gear && Gear->ItemId == TEXT("ScaleCuirass") ? Gear : nullptr;
    }

    AKalmalaConstructionActor* FindScaleCuirassForge(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (It->GetConstructionId().StartsWith(FString(A8bScaleCuirassForgeId) + TEXT("_"))) return *It;
        }
        return nullptr;
    }

    bool GetScaleCuirassOwnerId(const AKalmalaConstructionActor* Forge, int32& OutPlayerId)
    {
        if (!Forge) return false;
        const FString Prefix = FString(A8bScaleCuirassForgeId) + TEXT("_");
        if (!Forge->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Forge->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }
}

void UKalmalaCraftingComponent::RunA8bScaleCuirassPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    A8bScaleCuirassPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || A8bScaleCuirassPeerStage >= 4) return;
        if (A8bScaleCuirassPeerStage == 0)
        {
            if (A8bScaleCuirassPeerElapsed < 3.0f) return;
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

            A8bScaleCuirassPeerHostBaselineGiantScale = Inventory->GetQuantity(TEXT("GiantScale"));
            A8bScaleCuirassPeerHostBaselineBronze = Inventory->GetQuantity(TEXT("Bronze"));
            A8bScaleCuirassPeerHostBaselineCuirass = Inventory->GetQuantity(TEXT("ScaleCuirass"));
            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            if (!TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: remote owner components are incomplete"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            A8bScaleCuirassPeerBaselineGiantScale = TargetInventory->GetQuantity(TEXT("GiantScale"));
            A8bScaleCuirassPeerBaselineBronze = TargetInventory->GetQuantity(TEXT("Bronze"));
            A8bScaleCuirassPeerBaselineCuirass = TargetInventory->GetQuantity(TEXT("ScaleCuirass"));
            A8bScaleCuirassPeerBaselineHealth = Target->GetHealth();
            if (!FMath::IsNearlyEqual(A8bScaleCuirassPeerBaselineHealth, 100.0f, 0.1f))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Scale Cuirass peer FAILED: owner health baseline is %.1f"),
                    A8bScaleCuirassPeerBaselineHealth);
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            if (!TargetInventory->TryGrantFromServer(TEXT("GiantScale"), 10)
                || !TargetInventory->TryGrantFromServer(TEXT("Bronze"), 10))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: could not stage recipe materials"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            const FVector ForgeLocation = Target->GetActorLocation() + Target->GetActorRightVector() * 150.0f;
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Forge = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, ForgeLocation), SpawnParameters);
            if (!Forge)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: Forge did not spawn"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            const FString ConstructionId = FString::Printf(TEXT("%s_%d"), A8bScaleCuirassForgeId,
                Target->GetPlayerState()->GetPlayerId());
            Forge->InitializeFromServer(TEXT("ForgeKit"), ConstructionId);
            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Forge->GetActorLocation())
                    <= FMath::Square(250.0f)
                && TargetCrafting->GetRecipeAvailability(A8bScaleCuirassRecipe) == TEXT("Ready");
            if (!bStationReady)
            {
                Forge->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: Forge recipe is not available"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            A8bScaleCuirassPeerTarget = Target;
            TargetCrafting->PublishResult(A8bScaleCuirassReadyResult, true);
            A8bScaleCuirassPeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Forge->ForceNetUpdate();
            A8bScaleCuirassPeerStage = 1;
            A8bScaleCuirassPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Scale Cuirass peer fixture: Ready=1 RemoteOwner=1 GiantScale=10 Bronze=10 Forge=1"));
        }
        else if (A8bScaleCuirassPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bScaleCuirassPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: owner disappeared before craft result"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bScaleCuirassPeerResultSerial)
            {
                if (A8bScaleCuirassPeerElapsed > 20.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer server: Passed=0 craft result timed out"));
                    A8bScaleCuirassPeerStage = 99;
                }
                return;
            }

            const FKalmalaInventoryStack* Cuirass = FindScaleCuirassStack(TargetInventory);
            const bool bGuid = Cuirass && Cuirass->Quantity == A8bScaleCuirassPeerBaselineCuirass + 1
                && Cuirass->ItemGuids.Num() == Cuirass->Quantity
                && Cuirass->ItemGuids.ContainsByPredicate([](const FGuid& Guid) { return Guid.IsValid(); });
            const bool bCraftPassed = TargetCrafting->GetLastResult() == A8bScaleCuirassCraftedResult
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("GiantScale")) == A8bScaleCuirassPeerBaselineGiantScale
                && TargetInventory->GetQuantity(TEXT("Bronze")) == A8bScaleCuirassPeerBaselineBronze
                && TargetInventory->GetQuantity(TEXT("ScaleCuirass")) == A8bScaleCuirassPeerBaselineCuirass + 1
                && bGuid;
            if (!bCraftPassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Scale Cuirass peer server: Passed=0 Result=%s Accepted=%d Scale=%d Bronze=%d Output=%d Guid=%d"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("GiantScale")), TargetInventory->GetQuantity(TEXT("Bronze")),
                    TargetInventory->GetQuantity(TEXT("ScaleCuirass")), bGuid ? 1 : 0);
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            A8bScaleCuirassPeerItemGuid = Cuirass->ItemGuids[0];
            A8bScaleCuirassPeerStage = 2;
            A8bScaleCuirassPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display, TEXT("A8b Scale Cuirass peer server craft: Accepted=1 Payment=1 Output=1 Guid=1"));
        }
        else if (A8bScaleCuirassPeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bScaleCuirassPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            const FKalmalaEquippedGear* Cuirass = FindScaleCuirassChest(TargetInventory);
            if (!Target || !TargetInventory || !TargetCrafting || !Cuirass)
            {
                if (A8bScaleCuirassPeerElapsed < 30.0f) return;
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer server: Passed=0 remote equip timed out"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            if (Cuirass->ItemGuid != A8bScaleCuirassPeerItemGuid || Cuirass->ConditionSpent != 0)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: equipped Chest identity or condition is invalid"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            FKalmalaGearLevelStats GearStats;
            const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
            const bool bStats = Catalogue && Catalogue->GetGearLevelStats(TEXT("ScaleCuirass"), 1, GearStats)
                && GearStats.Armor == 20 && GearStats.MaxDurability == 150;
            const UKalmalaResistanceComponent* Defenses = Target->FindComponentByClass<UKalmalaResistanceComponent>();
            FKalmalaDamageChannels Incoming;
            Incoming.Crush = 30.0f;
            float ExpectedDamage = 0.0f;
            const bool bDamageProfile = Defenses && Defenses->Profile.Resolve(Incoming, ExpectedDamage,
                bStats ? GearStats.Armor : 0) && FMath::IsNearlyEqual(ExpectedDamage, 10.0f, 0.1f);
            if (!bStats || !bDamageProfile || TargetInventory->GetQuantity(TEXT("GiantScale")) != A8bScaleCuirassPeerBaselineGiantScale
                || TargetInventory->GetQuantity(TEXT("Bronze")) != A8bScaleCuirassPeerBaselineBronze
                || TargetInventory->GetQuantity(TEXT("ScaleCuirass")) != A8bScaleCuirassPeerBaselineCuirass)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Scale Cuirass peer server: Passed=0 Profile=%d Damage=%d Materials=%d"),
                    bStats ? 1 : 0, bDamageProfile ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("GiantScale")) == A8bScaleCuirassPeerBaselineGiantScale
                        && TargetInventory->GetQuantity(TEXT("Bronze")) == A8bScaleCuirassPeerBaselineBronze ? 1 : 0);
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector SourceLocation = Target->GetActorLocation() + Target->GetActorRightVector() * 100.0f;
            AKalmalaWildlifeSpawn* DamageSource = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(
                AKalmalaWildlifeSpawn::StaticClass(), FTransform(FRotator::ZeroRotator, SourceLocation), SpawnParameters);
            if (!DamageSource)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: armor damage source did not spawn"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            const float ExpectedHealth = FMath::Clamp(A8bScaleCuirassPeerBaselineHealth - ExpectedDamage, 1.0f, 100.0f);
            const bool bDamageAccepted = Target->ApplyWildlifeDamageFromServer(DamageSource, Incoming);
            DamageSource->Destroy();
            const float HealthAfter = Target->GetHealth();
            const FKalmalaEquippedGear* WornCuirass = FindScaleCuirassChest(TargetInventory);
            const bool bArmorApplied = bDamageAccepted
                && FMath::IsNearlyEqual(HealthAfter, ExpectedHealth, 0.1f)
                && WornCuirass && WornCuirass->ConditionSpent == 1;
            if (!bArmorApplied)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Scale Cuirass peer server: Passed=0 DamageAccepted=%d Armor=%d Health=%.1f/%.1f Condition=%d"),
                    bDamageAccepted ? 1 : 0, GearStats.Armor, HealthAfter, ExpectedHealth,
                    WornCuirass ? WornCuirass->ConditionSpent : INDEX_NONE);
                A8bScaleCuirassPeerStage = 99;
                return;
            }

            TargetCrafting->PublishResult(A8bScaleCuirassArmorResult, true);
            Target->ForceNetUpdate();
            A8bScaleCuirassPeerStage = 3;
            A8bScaleCuirassPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Scale Cuirass peer server: Passed=1 Craft=1 Payment=1 Output=1 Guid=1 Equip=1 Armor=20 Damage=10 Condition=1"));
        }
        else if (A8bScaleCuirassPeerStage == 3)
        {
            const AKalmalaCharacter* Target = A8bScaleCuirassPeerTarget.Get();
            const bool bHostItemsStable = Inventory->GetQuantity(TEXT("GiantScale")) == A8bScaleCuirassPeerHostBaselineGiantScale
                && Inventory->GetQuantity(TEXT("Bronze")) == A8bScaleCuirassPeerHostBaselineBronze
                && Inventory->GetQuantity(TEXT("ScaleCuirass")) == A8bScaleCuirassPeerHostBaselineCuirass;
            const bool bFinalOwnerState = Target
                && Target->GetHealth() <= A8bScaleCuirassPeerBaselineHealth - 9.9f;
            if (!bHostItemsStable || !bFinalOwnerState)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Scale Cuirass peer listen host: Passed=0 HostItems=%d OwnerHealth=%d"),
                    bHostItemsStable ? 1 : 0, bFinalOwnerState ? 1 : 0);
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("A8b Scale Cuirass peer listen host: Passed=1 HostItems=Stable"));
            A8bScaleCuirassPeerStage = 4;
        }
        return;
    }

    if (Character->IsLocallyControlled() && A8bScaleCuirassPeerStage == 0
        && !bA8bScaleCuirassPeerFixtureObserved)
    {
        AKalmalaConstructionActor* Forge = FindScaleCuirassForge(GetWorld());
        if (Forge)
        {
            int32 OwnerPlayerId = INDEX_NONE;
            if (!GetScaleCuirassOwnerId(Forge, OwnerPlayerId))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer observer: Passed=0 invalid owner identity"));
                A8bScaleCuirassPeerStage = 99;
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
                A8bScaleCuirassPeerObservedOwnerPlayerId = OwnerPlayerId;
                A8bScaleCuirassPeerObserverBaselineHealth = 100.0f;
                bA8bScaleCuirassPeerFixtureObserved = true;
                A8bScaleCuirassPeerElapsed = 0.0f;
            }
        }
    }

    if (Character->IsLocallyControlled() && bA8bScaleCuirassPeerFixtureObserved)
    {
        if (A8bScaleCuirassPeerStage >= 1) return;
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == A8bScaleCuirassPeerObservedOwnerPlayerId)
            {
                Owner = *It;
                break;
            }
        }
        const UKalmalaInventoryComponent* OwnerInventory = Owner
            ? Owner->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        const UKalmalaPlayerStatusComponent* OwnerStatus = Owner
            ? Owner->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
        const bool bPublicArmorDamage = Owner && Owner->GetHealth() <= A8bScaleCuirassPeerObserverBaselineHealth - 9.9f;
        const bool bPrivate = Owner && !Owner->IsLocallyControlled() && OwnerInventory && OwnerStatus
            && OwnerInventory->GetStacks().IsEmpty() && OwnerInventory->GetEquippedGear().IsEmpty()
            && Owner->GetCarriedToolInventory().IsEmpty() && OwnerStatus->GetStatuses().IsEmpty()
            && Inventory->GetQuantity(TEXT("GiantScale")) == 0
            && Inventory->GetQuantity(TEXT("Bronze")) == 0
            && Inventory->GetQuantity(TEXT("ScaleCuirass")) == 0;
        if (bPublicArmorDamage && bPrivate)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Scale Cuirass peer observer: Passed=1 Authority=0 PublicArmorDamage=1 TargetPrivate=1"));
            A8bScaleCuirassPeerStage = 1;
        }
        else if (A8bScaleCuirassPeerElapsed > 40.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Scale Cuirass peer observer: Passed=0 OwnerFound=%d PublicArmorDamage=%d TargetPrivate=%d"),
                Owner ? 1 : 0, bPublicArmorDamage ? 1 : 0, bPrivate ? 1 : 0);
            A8bScaleCuirassPeerStage = 99;
        }
        return;
    }

    if (Character->IsLocallyControlled())
    {
        if (A8bScaleCuirassPeerStage == 0 && GetLastResult() == A8bScaleCuirassReadyResult)
        {
            if (Inventory->GetQuantity(TEXT("GiantScale")) < 10 || Inventory->GetQuantity(TEXT("Bronze")) < 10
                || GetRecipeAvailability(A8bScaleCuirassRecipe) != TEXT("Ready"))
            {
                if (A8bScaleCuirassPeerElapsed < 15.0f) return;
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer owner: Passed=0 Forge or recipe inputs did not replicate"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            A8bScaleCuirassPeerBaselineGiantScale = Inventory->GetQuantity(TEXT("GiantScale")) - 10;
            A8bScaleCuirassPeerBaselineBronze = Inventory->GetQuantity(TEXT("Bronze")) - 10;
            A8bScaleCuirassPeerBaselineCuirass = Inventory->GetQuantity(TEXT("ScaleCuirass"));
            A8bScaleCuirassPeerBaselineHealth = Character->GetHealth();
            FString DirectReason;
            if (CraftFromServer(A8bScaleCuirassRecipe, 1, DirectReason))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: direct client craft bypassed server authority"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            A8bScaleCuirassPeerResultSerial = GetResultSerial();
            ServerCraft(A8bScaleCuirassRecipe, 1);
            A8bScaleCuirassPeerStage = 1;
            A8bScaleCuirassPeerElapsed = 0.0f;
            return;
        }
        if (A8bScaleCuirassPeerStage == 1)
        {
            if (GetResultSerial() == A8bScaleCuirassPeerResultSerial)
            {
                if (A8bScaleCuirassPeerElapsed > 20.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer owner: Passed=0 craft result timed out"));
                    A8bScaleCuirassPeerStage = 99;
                }
                return;
            }
            const FKalmalaInventoryStack* Cuirass = FindScaleCuirassStack(Inventory);
            const bool bCraftPassed = GetLastResult() == A8bScaleCuirassCraftedResult && WasLastResultAccepted()
                && Inventory->GetQuantity(TEXT("GiantScale")) == A8bScaleCuirassPeerBaselineGiantScale
                && Inventory->GetQuantity(TEXT("Bronze")) == A8bScaleCuirassPeerBaselineBronze
                && Inventory->GetQuantity(TEXT("ScaleCuirass")) == A8bScaleCuirassPeerBaselineCuirass + 1
                && Cuirass && Cuirass->ItemGuids.Num() == Cuirass->Quantity
                && Cuirass->ItemGuids.ContainsByPredicate([](const FGuid& Guid) { return Guid.IsValid(); });
            if (!bCraftPassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Scale Cuirass peer owner: Passed=0 Result=%s Accepted=%d Scale=%d Bronze=%d Output=%d"),
                    *GetLastResult(), WasLastResultAccepted() ? 1 : 0,
                    Inventory->GetQuantity(TEXT("GiantScale")), Inventory->GetQuantity(TEXT("Bronze")),
                    Inventory->GetQuantity(TEXT("ScaleCuirass")));
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            A8bScaleCuirassPeerItemGuid = Cuirass->ItemGuids[0];
            FString DirectReason;
            if (Inventory->EquipGearFromServer(A8bScaleCuirassPeerItemGuid, DirectReason))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer FAILED: direct client equip bypassed server authority"));
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            A8bScaleCuirassPeerGearRequestSerial = Inventory->GetGearRequestResultSerial();
            Inventory->ServerEquipGear(A8bScaleCuirassPeerItemGuid);
            A8bScaleCuirassPeerStage = 2;
            A8bScaleCuirassPeerElapsed = 0.0f;
            return;
        }
        if (A8bScaleCuirassPeerStage == 2)
        {
            if (Inventory->GetGearRequestResultSerial() == A8bScaleCuirassPeerGearRequestSerial)
            {
                if (A8bScaleCuirassPeerElapsed > 30.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer owner: Passed=0 gear RPC timed out"));
                    A8bScaleCuirassPeerStage = 99;
                }
                return;
            }
            if (!Inventory->WasLastGearRequestAccepted())
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Scale Cuirass peer owner: Passed=0 GearResult=%s"),
                    *Inventory->GetLastGearRequestReason());
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            const FKalmalaEquippedGear* Cuirass = FindScaleCuirassChest(Inventory);
            const bool bAcceptedDamage = GetLastResult() == A8bScaleCuirassArmorResult
                && Character->GetHealth() <= A8bScaleCuirassPeerBaselineHealth - 9.9f;
            if (!Cuirass || Cuirass->ItemGuid != A8bScaleCuirassPeerItemGuid || Cuirass->ConditionSpent != 1
                || Inventory->GetQuantity(TEXT("GiantScale")) != A8bScaleCuirassPeerBaselineGiantScale
                || Inventory->GetQuantity(TEXT("Bronze")) != A8bScaleCuirassPeerBaselineBronze
                || Inventory->GetQuantity(TEXT("ScaleCuirass")) != A8bScaleCuirassPeerBaselineCuirass
                || !bAcceptedDamage)
            {
                if (A8bScaleCuirassPeerElapsed < 15.0f) return;
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Scale Cuirass peer owner: Passed=0 Equip=%d Condition=%d ArmorResult=%d Health=%.1f"),
                    Cuirass && Cuirass->ItemGuid == A8bScaleCuirassPeerItemGuid ? 1 : 0,
                    Cuirass ? Cuirass->ConditionSpent : INDEX_NONE,
                    GetLastResult() == A8bScaleCuirassArmorResult ? 1 : 0, Character->GetHealth());
                A8bScaleCuirassPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("A8b Scale Cuirass peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Equip=1 ArmorDamage=1 Condition=1 Health=%.1f"),
                Character->GetHealth());
            A8bScaleCuirassPeerStage = 3;
        }
        return;
    }

}

#endif
