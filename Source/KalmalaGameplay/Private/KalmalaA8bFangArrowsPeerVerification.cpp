#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaArrowProjectile.h"
#include "KalmalaCharacter.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR FangArrowsRecipe[] = TEXT("FangArrowsRecipe");
    constexpr TCHAR FangArrowsReadyResult[] = TEXT("A8b Fang Arrows peer ready");
    constexpr TCHAR FangArrowsCraftedResult[] = TEXT("Crafted 20 Fang Arrows");
    constexpr TCHAR FangArrowsShotResult[] = TEXT("A8b Fang Arrows peer shot accepted");
    constexpr TCHAR FangArrowsWorkbenchId[] = TEXT("A8bPeerFangWorkbench");
    constexpr TCHAR FangArrowsTargetId[] = TEXT("a8b-peer-fang-target");

    const FKalmalaInventoryStack* FindFangArrowsStack(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory ? Inventory->GetStacks().FindByPredicate([](const FKalmalaInventoryStack& Stack)
        {
            return Stack.ItemId == TEXT("FangArrows");
        }) : nullptr;
    }

    AKalmalaConstructionActor* FindFangArrowsWorkbench(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (It->GetConstructionId().StartsWith(FString(FangArrowsWorkbenchId) + TEXT("_"))) return *It;
        }
        return nullptr;
    }

    bool GetFangArrowsWorkbenchOwnerId(const AKalmalaConstructionActor* Workbench, int32& OutPlayerId)
    {
        if (!Workbench) return false;
        const FString Prefix = FString(FangArrowsWorkbenchId) + TEXT("_");
        if (!Workbench->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Workbench->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }

    AKalmalaWildlifeSpawn* FindFangArrowsTarget(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
        {
            if (It->GetCreatureNicheId() == FName(FangArrowsTargetId)) return *It;
        }
        return nullptr;
    }

    void AimFangArrowsPeerAt(AKalmalaCharacter* Shooter, const AKalmalaWildlifeSpawn* Target)
    {
        APlayerController* Controller = Shooter && Target
            ? Cast<APlayerController>(Shooter->GetController()) : nullptr;
        if (!Controller) return;
        const FVector ViewLocation = Shooter->GetPawnViewLocation();
        const FRotator AimRotation = (Target->GetActorLocation() - ViewLocation).Rotation();
        Controller->SetControlRotation(AimRotation);
        if (APlayerCameraManager* CameraManager = Controller->PlayerCameraManager)
        {
            FMinimalViewInfo View = CameraManager->GetCameraCacheView();
            View.Location = ViewLocation;
            View.Rotation = AimRotation;
            CameraManager->SetCameraCachePOV(View);
            CameraManager->SetLastFrameCameraCachePOV(View);
        }
    }
}

void UKalmalaCraftingComponent::RunA8bFangArrowsPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    A8bFangArrowsPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || A8bFangArrowsPeerStage >= 5) return;
        if (A8bFangArrowsPeerStage == 0)
        {
            if (A8bFangArrowsPeerElapsed < 3.0f) return;
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

            A8bFangArrowsPeerHostBaselineViperFang = Inventory->GetQuantity(TEXT("ViperFang"));
            A8bFangArrowsPeerHostBaselineFangArrows = Inventory->GetQuantity(TEXT("FangArrows"));
            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            UKalmalaCombatComponent* TargetCombat = Target->FindComponentByClass<UKalmalaCombatComponent>();
            if (!TargetInventory || !TargetCrafting || !TargetCombat)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: remote owner components are incomplete"));
                A8bFangArrowsPeerStage = 99;
                return;
            }

            A8bFangArrowsPeerBaselineViperFang = TargetInventory->GetQuantity(TEXT("ViperFang"));
            A8bFangArrowsPeerBaselineWood = TargetInventory->GetQuantity(TEXT("Wood"));
            A8bFangArrowsPeerBaselineFangArrows = TargetInventory->GetQuantity(TEXT("FangArrows"));
            if (!TargetInventory->TryGrantFromServer(TEXT("ViperFang"), 2)
                || !TargetInventory->TryGrantFromServer(TEXT("Wood"), 10))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: could not stage recipe materials"));
                A8bFangArrowsPeerStage = 99;
                return;
            }

            FKalmalaToolState Bow;
            Bow.ItemId = TEXT("Bow");
            Bow.InstanceGuid = FGuid::NewGuid();
            Bow.Durability = 100;
            Bow.ToolLevel = 1;
            Target->CarriedTools.Add(Bow);
            Target->EnsureToolInstanceGuidsFromServer();
            TargetInventory->SynchronizeGridFromServer();
            FString MainHandReason;
            const bool bBowReady = TargetInventory->SetMainHandFromServer(Bow.InstanceGuid, MainHandReason);
            if (!bBowReady)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: test Bow could not be equipped: %s"), *MainHandReason);
                A8bFangArrowsPeerStage = 99;
                return;
            }
            A8bFangArrowsPeerBowGuid = Bow.InstanceGuid;
            A8bFangArrowsPeerBaselineBowCondition = Target->GetToolDurability(Bow.InstanceGuid);

            const FVector Forward = Target->GetActorForwardVector().GetSafeNormal();
            const FVector WildlifeLocation = Target->GetPawnViewLocation() + Forward * 650.0f
                + FVector(0.0f, 0.0f, 1200.0f);
            const FVector WorkbenchLocation = Target->GetActorLocation() + Target->GetActorRightVector() * 150.0f;
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Workbench = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, WorkbenchLocation), SpawnParameters);
            AKalmalaWildlifeSpawn* Wildlife = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(
                AKalmalaWildlifeSpawn::StaticClass(), FTransform(FRotator::ZeroRotator, WildlifeLocation), SpawnParameters);
            if (!Workbench || !Wildlife)
            {
                if (Workbench) Workbench->Destroy();
                if (Wildlife) Wildlife->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: Workbench or wildlife target did not spawn"));
                A8bFangArrowsPeerStage = 99;
                return;
            }

            const FString WorkbenchId = FString::Printf(TEXT("%s_%d"), FangArrowsWorkbenchId,
                Target->GetPlayerState()->GetPlayerId());
            Workbench->InitializeFromServer(TEXT("WorkbenchKit"), WorkbenchId);
            FKalmalaWorldPopulationSpawn WildlifeSpawn;
            WildlifeSpawn.SpawnSeed = 8675309;
            WildlifeSpawn.Location = WildlifeLocation;
            WildlifeSpawn.ContentId = FName(FangArrowsTargetId);
            Wildlife->InitializeServer(WildlifeSpawn, EKalmalaWildlifeArchetype::MoonBear);
            if (UKalmalaResistanceComponent* Resistance = Wildlife->FindComponentByClass<UKalmalaResistanceComponent>())
                Resistance->Profile.Armor = 0.0f;
            AimFangArrowsPeerAt(Target, Wildlife);

            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Workbench->GetActorLocation())
                    <= FMath::Square(250.0f)
                && TargetCrafting->GetRecipeAvailability(FangArrowsRecipe) == TEXT("Ready");
            const bool bTargetReady = Wildlife->GetCreatureNicheId() == FName(FangArrowsTargetId)
                && FMath::IsFinite(Wildlife->GetHealth()) && Wildlife->GetHealth() > 100.0f
                && FMath::IsNearlyEqual(Wildlife->GetFangArrowPoisonRemainingSeconds(), 0.0f);
            if (!bStationReady || !bTargetReady)
            {
                Workbench->Destroy();
                Wildlife->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: station=%d target=%d"),
                    bStationReady ? 1 : 0, bTargetReady ? 1 : 0);
                A8bFangArrowsPeerStage = 99;
                return;
            }

            A8bFangArrowsPeerTarget = Target;
            A8bFangArrowsPeerWildlife = Wildlife;
            A8bFangArrowsPeerBaselineWildlifeHealth = Wildlife->GetHealth();
            TargetCrafting->PublishResult(FangArrowsReadyResult, true);
            A8bFangArrowsPeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Workbench->ForceNetUpdate();
            Wildlife->ForceNetUpdate();
            A8bFangArrowsPeerStage = 1;
            A8bFangArrowsPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Arrows peer fixture: Ready=1 RemoteOwner=1 ViperFang=2 Wood=10 Bow=1 Workbench=1 Target=MoonBear"));
        }
        else if (A8bFangArrowsPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bFangArrowsPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            const FKalmalaInventoryStack* Arrows = FindFangArrowsStack(TargetInventory);
            if (!Target || !TargetInventory || !TargetCrafting)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: owner disappeared before craft result"));
                A8bFangArrowsPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bFangArrowsPeerResultSerial) return;
            const bool bGuidOutput = Arrows && Arrows->Quantity == A8bFangArrowsPeerBaselineFangArrows + 20
                && Arrows->ItemGuids.Num() == Arrows->Quantity
                && Arrows->ItemGuids.ContainsByPredicate([](const FGuid& Guid) { return Guid.IsValid(); });
            const bool bCrafted = TargetCrafting->GetLastResult() == FangArrowsCraftedResult
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("ViperFang")) == A8bFangArrowsPeerBaselineViperFang
                && TargetInventory->GetQuantity(TEXT("Wood")) == A8bFangArrowsPeerBaselineWood
                && TargetInventory->GetQuantity(TEXT("FangArrows")) == A8bFangArrowsPeerBaselineFangArrows + 20
                && bGuidOutput;
            if (!bCrafted)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Arrows peer server: Passed=0 CraftResult=%s Accepted=%d Fang=%d Wood=%d Output=%d Guid=%d"),
                    *TargetCrafting->GetLastResult(), TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("ViperFang")), TargetInventory->GetQuantity(TEXT("Wood")),
                    TargetInventory->GetQuantity(TEXT("FangArrows")), bGuidOutput ? 1 : 0);
                A8bFangArrowsPeerStage = 99;
                return;
            }
            A8bFangArrowsPeerResultSerial = TargetCrafting->GetResultSerial();
            A8bFangArrowsPeerStage = 2;
            A8bFangArrowsPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display, TEXT("A8b Fang Arrows peer server craft: Accepted=1 Payment=1 Output=20 Guid=1"));
        }
        else if (A8bFangArrowsPeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bFangArrowsPeerTarget.Get();
            AKalmalaWildlifeSpawn* Wildlife = A8bFangArrowsPeerWildlife.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            if (!Target || !Wildlife || !TargetInventory)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: test owner or target disappeared before shot"));
                A8bFangArrowsPeerStage = 99;
                return;
            }
            AimFangArrowsPeerAt(Target, Wildlife);
            const float PierceDamage = A8bFangArrowsPeerBaselineWildlifeHealth - Wildlife->GetHealth();
            const float PoisonSeconds = Wildlife->GetFangArrowPoisonRemainingSeconds();
            const bool bShotConsumed = TargetInventory->GetQuantity(TEXT("FangArrows"))
                    == A8bFangArrowsPeerBaselineFangArrows + 19
                && Target->GetToolDurability(A8bFangArrowsPeerBowGuid) == A8bFangArrowsPeerBaselineBowCondition - 1;
            if (bShotConsumed && PoisonSeconds > 0.0f && PierceDamage >= 49.9f)
            {
                const bool bPierceExactly50 = FMath::IsNearlyEqual(PierceDamage, 50.0f, 0.1f)
                    || FMath::IsNearlyEqual(PierceDamage, 51.0f, 0.1f);
                if (!bPierceExactly50)
                {
                    UE_LOG(LogTemp, Error,
                        TEXT("A8b Fang Arrows peer server: Passed=0 PierceDamage=%.2f Poison=%.2f"),
                        PierceDamage, PoisonSeconds);
                    A8bFangArrowsPeerStage = 99;
                    return;
                }
                A8bFangArrowsPeerHealthAfterShot = Wildlife->GetHealth();
                A8bFangArrowsPeerStage = FMath::IsNearlyEqual(PierceDamage, 51.0f, 0.1f) ? 4 : 3;
                A8bFangArrowsPeerElapsed = 0.0f;
                return;
            }
            if (A8bFangArrowsPeerElapsed > 20.0f)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Arrows peer server: Passed=0 Ammo=%d BowCondition=%d Health=%.2f Baseline=%.2f Poison=%.2f"),
                    TargetInventory->GetQuantity(TEXT("FangArrows")), Target->GetToolDurability(A8bFangArrowsPeerBowGuid),
                    Wildlife->GetHealth(), A8bFangArrowsPeerBaselineWildlifeHealth, PoisonSeconds);
                A8bFangArrowsPeerStage = 99;
            }
        }
        else if (A8bFangArrowsPeerStage == 3)
        {
            AKalmalaWildlifeSpawn* Wildlife = A8bFangArrowsPeerWildlife.Get();
            if (!Wildlife)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: wildlife target disappeared during poison"));
                A8bFangArrowsPeerStage = 99;
                return;
            }
            const float PoisonSeconds = Wildlife->GetFangArrowPoisonRemainingSeconds();
            const bool bPoisonTick = PoisonSeconds > 0.0f && PoisonSeconds <= 4.05f
                && FMath::IsNearlyEqual(Wildlife->GetHealth(), A8bFangArrowsPeerHealthAfterShot - 1.0f, 0.1f);
            if (bPoisonTick)
            {
                A8bFangArrowsPeerStage = 4;
                A8bFangArrowsPeerElapsed = 0.0f;
            }
            else if (A8bFangArrowsPeerElapsed > 7.0f)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Arrows peer server: Passed=0 PoisonTick=0 Health=%.2f AfterShot=%.2f Poison=%.2f"),
                    Wildlife->GetHealth(), A8bFangArrowsPeerHealthAfterShot, PoisonSeconds);
                A8bFangArrowsPeerStage = 99;
            }
        }
        else if (A8bFangArrowsPeerStage == 4)
        {
            if (A8bFangArrowsPeerElapsed < 0.1f) return;
            AKalmalaCharacter* Target = A8bFangArrowsPeerTarget.Get();
            AKalmalaWildlifeSpawn* Wildlife = A8bFangArrowsPeerWildlife.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            const bool bHostPrivate = Inventory->GetQuantity(TEXT("ViperFang"))
                    == A8bFangArrowsPeerHostBaselineViperFang
                && Inventory->GetQuantity(TEXT("FangArrows"))
                    == A8bFangArrowsPeerHostBaselineFangArrows;
            const bool bFinalState = Target && Wildlife && TargetInventory
                && TargetInventory->GetQuantity(TEXT("ViperFang")) == A8bFangArrowsPeerBaselineViperFang
                && TargetInventory->GetQuantity(TEXT("Wood")) == A8bFangArrowsPeerBaselineWood
                && TargetInventory->GetQuantity(TEXT("FangArrows")) == A8bFangArrowsPeerBaselineFangArrows + 19
                && FMath::IsNearlyEqual(Target->GetToolDurability(A8bFangArrowsPeerBowGuid),
                    A8bFangArrowsPeerBaselineBowCondition - 1.0f, 0.1f)
                && FMath::IsNearlyEqual(Wildlife->GetHealth(), A8bFangArrowsPeerBaselineWildlifeHealth - 51.0f, 0.1f);
            if (!bHostPrivate || !bFinalState)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Arrows peer server: Passed=0 HostPrivate=%d FinalState=%d"),
                    bHostPrivate ? 1 : 0, bFinalState ? 1 : 0);
                A8bFangArrowsPeerStage = 99;
                return;
            }
            if (UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>())
                TargetCrafting->PublishResult(FangArrowsShotResult, true);
            Target->ForceNetUpdate();
            Wildlife->ForceNetUpdate();
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Arrows peer server: Passed=1 Craft=1 Payment=1 Output=20 Shot=1 Pierce=50 Poison=5 Tick=1"));
            UE_LOG(LogTemp, Display, TEXT("A8b Fang Arrows peer listen host: Passed=1 OwnMaterials=0 OwnArrows=0"));
            A8bFangArrowsPeerStage = 5;
        }
        return;
    }

    if (!Character->IsLocallyControlled()) return;
    UKalmalaCombatComponent* Combat = Character->FindComponentByClass<UKalmalaCombatComponent>();
    UKalmalaCraftingComponent* OwnerCrafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
    if (!Combat || !OwnerCrafting) return;
    if (bA8bFangArrowsPeerFixtureObserved)
    {
        if (A8bFangArrowsPeerStage >= 5) return;
        if (A8bFangArrowsPeerElapsed < 0.1f) return;
        AKalmalaWildlifeSpawn* Wildlife = FindFangArrowsTarget(GetWorld());
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == A8bFangArrowsPeerObservedOwnerPlayerId)
            {
                Owner = *It;
                break;
            }
        }
        const UKalmalaInventoryComponent* OwnerInventory = Owner
            ? Owner->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        const UKalmalaPlayerStatusComponent* OwnerStatus = Owner
            ? Owner->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
        const bool bPublicPoisonDamage = Wildlife
            && Wildlife->GetHealth() <= A8bFangArrowsPeerObserverBaselineHealth - 50.9f;
        const bool bPrivate = Owner && !Owner->IsLocallyControlled() && OwnerInventory && OwnerStatus
            && OwnerInventory->GetStacks().IsEmpty() && OwnerInventory->GetEquippedGear().IsEmpty()
            && Owner->GetCarriedToolInventory().IsEmpty() && OwnerStatus->GetStatuses().IsEmpty()
            && Inventory->GetQuantity(TEXT("ViperFang")) == 0
            && Inventory->GetQuantity(TEXT("FangArrows")) == 0;
        if (bPublicPoisonDamage && bPrivate)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Arrows peer observer: Passed=1 Authority=0 PublicPoisonDamage=1 TargetPrivate=1"));
            A8bFangArrowsPeerStage = 5;
        }
        else if (A8bFangArrowsPeerElapsed > 35.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Arrows peer observer: Passed=0 Authority=0 TargetFound=%d PublicPoisonDamage=%d TargetPrivate=%d"),
                Wildlife ? 1 : 0, bPublicPoisonDamage ? 1 : 0, bPrivate ? 1 : 0);
            A8bFangArrowsPeerStage = 99;
        }
        return;
    }

    if (A8bFangArrowsPeerStage == 0 && FindFangArrowsWorkbench(GetWorld()))
    {
        AKalmalaConstructionActor* Workbench = FindFangArrowsWorkbench(GetWorld());
        int32 OwnerPlayerId = INDEX_NONE;
        if (!GetFangArrowsWorkbenchOwnerId(Workbench, OwnerPlayerId))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer observer: Passed=0 invalid owner identity"));
            A8bFangArrowsPeerStage = 99;
            return;
        }
        if (Character->GetPlayerState()->GetPlayerId() != OwnerPlayerId)
        {
            AKalmalaWildlifeSpawn* Wildlife = FindFangArrowsTarget(GetWorld());
            if (!Wildlife) return;
            bA8bFangArrowsPeerFixtureObserved = true;
            A8bFangArrowsPeerObservedOwnerPlayerId = OwnerPlayerId;
            A8bFangArrowsPeerObserverBaselineHealth = Wildlife->GetHealth();
            A8bFangArrowsPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Arrows peer observer: FixtureSeen=1 TargetPlayerId=%d Health=%.1f"),
                OwnerPlayerId, A8bFangArrowsPeerObserverBaselineHealth);
            return;
        }
    }

    if (A8bFangArrowsPeerStage == 0 && GetLastResult() == FangArrowsReadyResult)
    {
        AKalmalaWildlifeSpawn* Wildlife = FindFangArrowsTarget(GetWorld());
        if (!Wildlife || Inventory->GetQuantity(TEXT("ViperFang")) < 2
            || Inventory->GetQuantity(TEXT("Wood")) < 10
            || Character->GetInventoryComponent()->GetActiveItem() != TEXT("Bow")
            || !OwnerCrafting->GetRecipeAvailability(FangArrowsRecipe).Equals(TEXT("Ready")))
        {
            if (A8bFangArrowsPeerElapsed < 15.0f) return;
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Arrows peer owner FAILED: target=%d Fang=%d Wood=%d MainHand=%s Recipe=%s"),
                Wildlife ? 1 : 0, Inventory->GetQuantity(TEXT("ViperFang")), Inventory->GetQuantity(TEXT("Wood")),
                *Character->GetInventoryComponent()->GetActiveItem().ToString(), *OwnerCrafting->GetRecipeAvailability(FangArrowsRecipe));
            A8bFangArrowsPeerStage = 99;
            return;
        }
        A8bFangArrowsPeerBaselineViperFang = Inventory->GetQuantity(TEXT("ViperFang")) - 2;
        A8bFangArrowsPeerBaselineWood = Inventory->GetQuantity(TEXT("Wood")) - 10;
        A8bFangArrowsPeerBaselineFangArrows = Inventory->GetQuantity(TEXT("FangArrows"));
        A8bFangArrowsPeerBaselineWildlifeHealth = Wildlife->GetHealth();
        FString DirectReason;
        if (CraftFromServer(FangArrowsRecipe, 1, DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer FAILED: direct client craft bypassed server authority"));
            A8bFangArrowsPeerStage = 99;
            return;
        }
        A8bFangArrowsPeerResultSerial = GetResultSerial();
        ServerCraft(FangArrowsRecipe, 1);
        A8bFangArrowsPeerStage = 1;
        A8bFangArrowsPeerElapsed = 0.0f;
        return;
    }
    if (A8bFangArrowsPeerStage == 1)
    {
        if (GetResultSerial() == A8bFangArrowsPeerResultSerial)
        {
            if (A8bFangArrowsPeerElapsed > 15.0f)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Arrows peer owner: Passed=0 craft result replication timed out Serial=%u Baseline=%u"),
                    GetResultSerial(), A8bFangArrowsPeerResultSerial);
                A8bFangArrowsPeerStage = 99;
            }
            return;
        }
        if (GetLastResult() != FangArrowsCraftedResult || !WasLastResultAccepted()
            || Inventory->GetQuantity(TEXT("ViperFang")) != A8bFangArrowsPeerBaselineViperFang
            || Inventory->GetQuantity(TEXT("Wood")) != A8bFangArrowsPeerBaselineWood
            || Inventory->GetQuantity(TEXT("FangArrows")) != A8bFangArrowsPeerBaselineFangArrows + 20)
        {
            if (A8bFangArrowsPeerElapsed < 15.0f) return;
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Arrows peer owner: Passed=0 CraftResult=%s Accepted=%d Fang=%d Wood=%d Output=%d"),
                *GetLastResult(), WasLastResultAccepted() ? 1 : 0,
                Inventory->GetQuantity(TEXT("ViperFang")), Inventory->GetQuantity(TEXT("Wood")),
                Inventory->GetQuantity(TEXT("FangArrows")));
            A8bFangArrowsPeerStage = 99;
            return;
        }
        const FKalmalaInventoryStack* Arrows = FindFangArrowsStack(Inventory);
        if (!Arrows || Arrows->ItemGuids.IsEmpty() || !Arrows->ItemGuids[0].IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer owner FAILED: crafted arrow unit has no valid identity"));
            A8bFangArrowsPeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Fang Arrows peer owner craft: Accepted=1 Payment=1 Output=20 Serial=%u"), GetResultSerial());
        Combat->ServerPreferArrowStack(Arrows->ItemGuids[0]);
        A8bFangArrowsPeerStage = 2;
        A8bFangArrowsPeerElapsed = 0.0f;
    }
    else if (A8bFangArrowsPeerStage == 2)
    {
        if (Combat->GetArrowItemIdForDisplay() != TEXT("FangArrows") || !Combat->CanDrawBow())
        {
            if (A8bFangArrowsPeerElapsed < 15.0f) return;
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Arrows peer owner: Passed=0 selection timeout Arrow=%s Ammo=%d CanDraw=%d Bow=%s"),
                *Combat->GetArrowItemIdForDisplay().ToString(), Inventory->GetQuantity(TEXT("FangArrows")),
                Combat->CanDrawBow() ? 1 : 0,
                *Character->GetInventoryComponent()->GetActiveItem().ToString());
            A8bFangArrowsPeerStage = 99;
            return;
        }
        if (AKalmalaWildlifeSpawn* Wildlife = FindFangArrowsTarget(GetWorld())) AimFangArrowsPeerAt(Character, Wildlife);
        Combat->StartBowDraw(6501);
        if (!Combat->IsBowDrawing())
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer owner FAILED: server Bow draw did not start"));
            A8bFangArrowsPeerStage = 99;
            return;
        }
        A8bFangArrowsPeerStage = 3;
        A8bFangArrowsPeerElapsed = 0.0f;
    }
    else if (A8bFangArrowsPeerStage == 3)
    {
        AKalmalaWildlifeSpawn* Wildlife = FindFangArrowsTarget(GetWorld());
        if (Wildlife) AimFangArrowsPeerAt(Character, Wildlife);
        if (A8bFangArrowsPeerElapsed < 1.7f) return;
        Combat->ReleaseBowDraw(6502, false);
        A8bFangArrowsPeerStage = 4;
        A8bFangArrowsPeerElapsed = 0.0f;
    }
    else if (A8bFangArrowsPeerStage == 4)
    {
        if (OwnerCrafting->GetLastResult() != FangArrowsShotResult || !OwnerCrafting->WasLastResultAccepted())
        {
            if (A8bFangArrowsPeerElapsed < 20.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Arrows peer owner: Passed=0 server shot acceptance did not arrive"));
            A8bFangArrowsPeerStage = 99;
            return;
        }
        AKalmalaWildlifeSpawn* Wildlife = FindFangArrowsTarget(GetWorld());
        const bool bShotDelivered = Inventory->GetQuantity(TEXT("FangArrows")) == A8bFangArrowsPeerBaselineFangArrows + 19
            && Character->GetInventoryComponent()->GetActiveItem() == TEXT("Bow")
            && Character->GetToolDurability(Character->GetInventoryComponent()->GetActiveInstanceGuid()) == 99
            && Combat->GetActionSerial() > 0 && Wildlife
            && Wildlife->GetHealth() <= A8bFangArrowsPeerBaselineWildlifeHealth - 50.9f;
        if (!bShotDelivered)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Arrows peer owner: Passed=0 Ammo=%d BowCondition=%d Action=%u Target=%d Health=%.2f Baseline=%.2f"),
                Inventory->GetQuantity(TEXT("FangArrows")),
                Character->GetToolDurability(Character->GetInventoryComponent()->GetActiveInstanceGuid()),
                Combat->GetActionSerial(), Wildlife ? 1 : 0, Wildlife ? Wildlife->GetHealth() : -1.0f,
                A8bFangArrowsPeerBaselineWildlifeHealth);
            A8bFangArrowsPeerStage = 99;
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("A8b Fang Arrows peer owner: Passed=1 Authority=0 Craft=1 Payment=1 Output=20 Shot=1 Ammo=19"));
        A8bFangArrowsPeerStage = 5;
    }
}

#endif
