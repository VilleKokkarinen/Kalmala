#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR OpalHelmPeerForgePrefix[] = TEXT("RF44OpalForge_");
    constexpr TCHAR OpalHelmPeerReadyWrongLevel[] = TEXT("RF44 Opal Helm ready: WrongLevel");
    constexpr TCHAR OpalHelmPeerReadyDistant[] = TEXT("RF44 Opal Helm ready: Distant");
    constexpr TCHAR OpalHelmPeerReadyOccluded[] = TEXT("RF44 Opal Helm ready: Occluded");
    constexpr TCHAR OpalHelmPeerReadyForeignGuid[] = TEXT("RF44 Opal Helm ready: ForeignGuid");
    constexpr TCHAR OpalHelmPeerReadyInsufficient[] = TEXT("RF44 Opal Helm ready: Insufficient");
    constexpr TCHAR OpalHelmPeerReadyCadence[] = TEXT("RF44 Opal Helm ready: Cadence");
    constexpr TCHAR OpalHelmPeerReadyUpgrade[] = TEXT("RF44 Opal Helm ready: Upgrade");
    constexpr TCHAR OpalHelmPeerReadyRepair[] = TEXT("RF44 Opal Helm ready: Repair");
    constexpr TCHAR OpalHelmPeerUpgradeResult[] = TEXT("Opal Helm upgraded to level 2");
    constexpr TCHAR OpalHelmPeerRepairResult[] = TEXT("Opal Helm repaired to full condition at no cost");

    const FKalmalaEquippedGear* FindOpalHelm(const UKalmalaInventoryComponent* Inventory)
    {
        const FKalmalaEquippedGear* Gear = Inventory
            ? Inventory->FindEquippedGear(EKalmalaGearSlot::Head) : nullptr;
        return Gear && Gear->ItemId == TEXT("OpalHelm") ? Gear : nullptr;
    }

    AKalmalaConstructionActor* FindOpalPeerForge(UWorld* World, int32* OutOwnerPlayerId = nullptr)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            const FString Id = It->GetConstructionId();
            if (!Id.StartsWith(OpalHelmPeerForgePrefix)) continue;
            const FString Suffix = Id.RightChop(UE_ARRAY_COUNT(OpalHelmPeerForgePrefix) - 1);
            if (!Suffix.IsNumeric()) continue;
            if (OutOwnerPlayerId) *OutOwnerPlayerId = FCString::Atoi(*Suffix);
            return *It;
        }
        return nullptr;
    }

    bool HasBaseOpalPayment(const UKalmalaInventoryComponent* Inventory)
    {
        return Inventory && Inventory->GetQuantity(TEXT("Iron")) == 4
            && Inventory->GetQuantity(TEXT("DeerHide")) == 4;
    }

    bool HasLevelOneHelm(const UKalmalaInventoryComponent* Inventory, const FGuid ItemGuid)
    {
        const FKalmalaEquippedGear* Gear = FindOpalHelm(Inventory);
        return Gear && Gear->ItemGuid == ItemGuid && Gear->Level == 1 && Gear->ConditionSpent == 0;
    }
}

void UKalmalaCraftingComponent::RunOpalHelmPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    OpalHelmPeerElapsed += DeltaTime;
    OpalHelmPeerLocalElapsed += DeltaTime;

    const auto Fail = [this](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Error, TEXT("Opal Helm peer FAILED: %s"), Reason);
        OpalHelmPeerStage = 99;
        OpalHelmPeerLocalStage = 99;
    };

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || OpalHelmPeerStage >= 17) return;

        if (OpalHelmPeerStage == 0)
        {
            if (OpalHelmPeerElapsed < 3.0f) return;
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
            if (RemoteCount < 2 || !Target) return;

            UKalmalaInventoryComponent* TargetInventory = Target->GetInventoryComponent();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            if (!TargetInventory || !TargetCrafting || TargetInventory->GetQuantity(TEXT("OpalHelm")) != 0
                || FindOpalHelm(TargetInventory) || !FMath::IsNearlyEqual(Target->GetHealth(), 100.0f, 0.1f))
            {
                Fail(TEXT("remote owner baseline or crafting component is invalid"));
                return;
            }

            Target->GetCharacterMovement()->StopMovementImmediately();
            Target->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
            Target->SetActorLocation(Target->GetActorLocation() + FVector(0.0f, 1400.0f, 0.0f),
                false, nullptr, ETeleportType::TeleportPhysics);
            Target->SetActorRotation(FRotator::ZeroRotator);
            Target->GetController()->SetControlRotation(FRotator::ZeroRotator);
            Target->ForceNetUpdate();

            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            const FVector ForgeLocation = Target->GetActorLocation() + FVector(150.0f, 0.0f, 0.0f);
            AKalmalaConstructionActor* Forge = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, ForgeLocation),
                SpawnParameters);
            if (!Forge)
            {
                Fail(TEXT("test Forge did not spawn"));
                return;
            }
            const int32 OwnerPlayerId = Target->GetPlayerState()->GetPlayerId();
            Forge->InitializeFromServer(TEXT("ForgeKit"), FString::Printf(TEXT("%s%d"),
                OpalHelmPeerForgePrefix, OwnerPlayerId));
            FCollisionQueryParams ForgeQuery(SCENE_QUERY_STAT(OpalHelmPeerInitialForge), false, Target);
            FHitResult ForgeHit;
            const bool bInitialForgeVisible = GetWorld()->LineTraceSingleByChannel(ForgeHit,
                Target->GetPawnViewLocation(), Forge->GetActorLocation(), ECC_Visibility, ForgeQuery)
                && ForgeHit.GetActor() == Forge;
            if (FVector::DistSquared(Target->GetActorLocation(), Forge->GetActorLocation()) > FMath::Square(250.0f)
                || FKalmalaToolProgressionContract::GetEffectiveStationLevel(Forge) != 1 || !bInitialForgeVisible)
            {
                Forge->Destroy();
                Fail(TEXT("wrong-level Forge fixture was not visible and within range"));
                return;
            }

            const bool bGranted = TargetInventory->TryGrantFromServer(TEXT("OpalHelm"), 1)
                && TargetInventory->TryGrantFromServer(TEXT("Iron"), 4)
                && TargetInventory->TryGrantFromServer(TEXT("DeerHide"), 4);
            const FKalmalaInventoryStack* HelmStack = TargetInventory->GetStacks().FindByPredicate(
                [](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("OpalHelm"); });
            if (!bGranted || !HelmStack || HelmStack->Quantity != 1 || HelmStack->ItemGuids.Num() != 1
                || !HelmStack->ItemGuids[0].IsValid() || !HasBaseOpalPayment(TargetInventory))
            {
                Forge->Destroy();
                Fail(TEXT("server could not prepare the exact owner item and material fixture"));
                return;
            }

            OpalHelmPeerItemGuid = HelmStack->ItemGuids[0];
            FString EquipReason;
            if (!TargetInventory->EquipGearFromServer(OpalHelmPeerItemGuid, EquipReason)
                || !HasLevelOneHelm(TargetInventory, OpalHelmPeerItemGuid))
            {
                Forge->Destroy();
                Fail(TEXT("server could not equip the exact level-one Opal Helm"));
                return;
            }

            OpalHelmPeerTarget = Target;
            OpalHelmPeerForge = Forge;
            TargetCrafting->PublishInteractionResultFromServer(OpalHelmPeerReadyWrongLevel, true);
            OpalHelmPeerResultSerial = TargetCrafting->GetResultSerial();
            OpalHelmPeerStage = 1;
            OpalHelmPeerElapsed = 0.0f;
            Forge->ForceNetUpdate();
            UE_LOG(LogTemp, Display,
                TEXT("Opal Helm peer fixture: Ready=1 RemoteOwner=1 ForgeLevel=1 HelmGuid=1 Iron=4 Hide=4"));
            return;
        }

        AKalmalaCharacter* Target = OpalHelmPeerTarget.Get();
        UKalmalaInventoryComponent* TargetInventory = Target ? Target->GetInventoryComponent() : nullptr;
        UKalmalaCraftingComponent* TargetCrafting = Target
            ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
        AKalmalaConstructionActor* Forge = OpalHelmPeerForge.Get();
        if (!Target || !TargetInventory || !TargetCrafting || !Forge)
        {
            if (OpalHelmPeerElapsed < 30.0f) return;
            Fail(TEXT("remote owner or station disappeared during maintenance"));
            return;
        }

        const auto AwaitResult = [this, TargetCrafting, &Fail]()
        {
            if (TargetCrafting->GetResultSerial() == OpalHelmPeerResultSerial)
            {
                if (OpalHelmPeerElapsed < 30.0f) return false;
                Fail(TEXT("remote maintenance RPC timed out"));
                return false;
            }
            return true;
        };
        const auto SingleRejectedResult = [this, TargetCrafting, TargetInventory]()
        {
            return TargetCrafting->GetResultSerial() == OpalHelmPeerResultSerial + 1
                && !TargetCrafting->WasLastResultAccepted()
                && HasLevelOneHelm(TargetInventory, OpalHelmPeerItemGuid)
                && HasBaseOpalPayment(TargetInventory);
        };
        const auto PublishNextOwnerStep = [this, TargetCrafting](const TCHAR* Step)
        {
            TargetCrafting->PublishInteractionResultFromServer(Step, true);
            OpalHelmPeerResultSerial = TargetCrafting->GetResultSerial();
            OpalHelmPeerElapsed = 0.0f;
        };

        switch (OpalHelmPeerStage)
        {
        case 1:
            if (!AwaitResult()) return;
            if (!SingleRejectedResult() || FKalmalaToolProgressionContract::GetEffectiveStationLevel(Forge) != 1
                || !TargetCrafting->GetLastResult().Contains(TEXT("visible same-world Forge")))
            {
                Fail(TEXT("wrong Forge level did not reject the remote upgrade without payment"));
                return;
            }
            OpalHelmPeerStage = 2;
            OpalHelmPeerElapsed = 0.0f;
            return;
        case 2:
            if (OpalHelmPeerElapsed < 0.75f) return;
            Forge->SetActorLocation(Target->GetActorLocation() + FVector(600.0f, 0.0f, 0.0f));
            Forge->ForceNetUpdate();
            PublishNextOwnerStep(OpalHelmPeerReadyDistant);
            OpalHelmPeerStage = 3;
            return;
        case 3:
            if (!AwaitResult()) return;
            if (!SingleRejectedResult() || !TargetCrafting->GetLastResult().Contains(TEXT("visible same-world Forge")))
            {
                Fail(TEXT("distant Forge did not reject the remote upgrade without payment"));
                return;
            }
            OpalHelmPeerStage = 4;
            OpalHelmPeerElapsed = 0.0f;
            return;
        case 4:
            if (OpalHelmPeerElapsed < 0.75f) return;
            Forge->SetActorLocation(Target->GetActorLocation() + FVector(150.0f, 0.0f, 0.0f));
            {
                FActorSpawnParameters SpawnParameters;
                SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                const FVector BlockerLocation = Target->GetActorLocation() + FVector(75.0f, 0.0f, 32.0f);
                AKalmalaConstructionActor* Blocker = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                    AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, BlockerLocation),
                    SpawnParameters);
                if (!Blocker)
                {
                    Fail(TEXT("Forge line-of-sight blocker did not spawn"));
                    return;
                }
                Blocker->InitializeFromServer(TEXT("WallKit"), FString::Printf(TEXT("RF44OpalBlock_%d"),
                    Target->GetPlayerState()->GetPlayerId()));
                FCollisionQueryParams Query(SCENE_QUERY_STAT(OpalHelmPeerVisibility), false, Target);
                FHitResult Hit;
                const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Target->GetPawnViewLocation(),
                    Forge->GetActorLocation(), ECC_Visibility, Query) && Hit.GetActor() == Blocker;
                if (!bBlocked)
                {
                    Blocker->Destroy();
                    Fail(TEXT("the fixture blocker did not occlude the Forge visibility trace"));
                    return;
                }
                OpalHelmPeerOccluder = Blocker;
            }
            Forge->ForceNetUpdate();
            PublishNextOwnerStep(OpalHelmPeerReadyOccluded);
            OpalHelmPeerStage = 5;
            return;
        case 5:
            if (!AwaitResult()) return;
            if (!SingleRejectedResult() || !TargetCrafting->GetLastResult().Contains(TEXT("visible same-world Forge")))
            {
                Fail(TEXT("occluded Forge did not reject the remote upgrade without payment"));
                return;
            }
            OpalHelmPeerStage = 6;
            OpalHelmPeerElapsed = 0.0f;
            return;
        case 6:
            if (OpalHelmPeerElapsed < 0.75f) return;
            if (AKalmalaConstructionActor* Blocker = OpalHelmPeerOccluder.Get()) Blocker->Destroy();
            {
                FActorSpawnParameters SpawnParameters;
                SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                const FVector AnvilLocation = Forge->GetActorLocation() + FVector(0.0f, 100.0f, 0.0f);
                AKalmalaConstructionActor* Anvil = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                    AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, AnvilLocation),
                    SpawnParameters);
                if (!Anvil)
                {
                    Fail(TEXT("Forge Anvil did not spawn"));
                    return;
                }
                Anvil->InitializeFromServer(TEXT("ForgeAnvilKit"), FString::Printf(TEXT("RF44OpalAnvil_%d"),
                    Target->GetPlayerState()->GetPlayerId()));
                if (FKalmalaToolProgressionContract::GetEffectiveStationLevel(Forge) != 2)
                {
                    Fail(TEXT("Forge Anvil did not produce the authored level-two station"));
                    return;
                }
            }
            PublishNextOwnerStep(OpalHelmPeerReadyForeignGuid);
            OpalHelmPeerStage = 7;
            return;
        case 7:
            if (!AwaitResult()) return;
            if (!SingleRejectedResult() || !TargetCrafting->GetLastResult().Contains(
                    TEXT("visible same-world Forge")))
            {
                Fail(TEXT("a foreign equipped-item GUID changed gear or payment"));
                return;
            }
            OpalHelmPeerStage = 8;
            OpalHelmPeerElapsed = 0.0f;
            return;
        case 8:
            if (OpalHelmPeerElapsed < 0.75f) return;
            PublishNextOwnerStep(OpalHelmPeerReadyInsufficient);
            OpalHelmPeerStage = 9;
            return;
        case 9:
            if (!AwaitResult()) return;
            if (!SingleRejectedResult() || !TargetCrafting->GetLastResult().Contains(TEXT("Iron")))
            {
                Fail(TEXT("insufficient Forge-upgrade materials were not rejected without partial payment"));
                return;
            }
            OpalHelmPeerStage = 10;
            OpalHelmPeerElapsed = 0.0f;
            return;
        case 10:
            if (OpalHelmPeerElapsed < 0.75f) return;
            PublishNextOwnerStep(OpalHelmPeerReadyCadence);
            OpalHelmPeerStage = 11;
            return;
        case 11:
            if (TargetCrafting->GetResultSerial() != OpalHelmPeerResultSerial + 2)
            {
                if (OpalHelmPeerElapsed >= 30.0f)
                    Fail(TEXT("rapid duplicate RPCs did not both return owner results"));
                return;
            }
            if (TargetCrafting->GetLastResult() != TEXT("Please wait before the next action")
                || TargetCrafting->WasLastResultAccepted() || !HasLevelOneHelm(TargetInventory, OpalHelmPeerItemGuid)
                || !HasBaseOpalPayment(TargetInventory))
            {
                Fail(TEXT("rapid duplicate RPCs were not throttled before changing gear or payment"));
                return;
            }
            OpalHelmPeerStage = 12;
            OpalHelmPeerElapsed = 0.0f;
            return;
        case 12:
            if (OpalHelmPeerElapsed < 0.75f) return;
            if (!TargetInventory->TryGrantFromServer(TEXT("Iron"), 1) || TargetInventory->GetQuantity(TEXT("Iron")) != 5)
            {
                Fail(TEXT("could not restore the exact authored upgrade payment"));
                return;
            }
            PublishNextOwnerStep(OpalHelmPeerReadyUpgrade);
            OpalHelmPeerStage = 13;
            return;
        case 13:
            if (!AwaitResult()) return;
            {
                const FKalmalaEquippedGear* Gear = FindOpalHelm(TargetInventory);
                FKalmalaGearLevelStats LevelTwoStats;
                const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
                const bool bStats = Catalogue && Catalogue->GetGearLevelStats(TEXT("OpalHelm"), 2, LevelTwoStats)
                    && LevelTwoStats.Armor == 13 && LevelTwoStats.MaxDurability == 160;
                const bool bPaid = TargetInventory->GetQuantity(TEXT("Iron")) == 0
                    && TargetInventory->GetQuantity(TEXT("DeerHide")) == 2;
                if (TargetCrafting->GetResultSerial() != OpalHelmPeerResultSerial + 1
                    || TargetCrafting->GetLastResult() != OpalHelmPeerUpgradeResult
                    || !TargetCrafting->WasLastResultAccepted() || !Gear || Gear->ItemGuid != OpalHelmPeerItemGuid
                    || Gear->Level != 2 || Gear->ConditionSpent != 0 || !bPaid || !bStats)
                {
                    Fail(TEXT("remote upgrade did not return exact level, payment, stats, and owner success"));
                    return;
                }
            }
            OpalHelmPeerStage = 14;
            OpalHelmPeerElapsed = 0.0f;
            return;
        case 14:
            if (OpalHelmPeerElapsed < 0.75f) return;
            {
                FActorSpawnParameters SpawnParameters;
                SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                const FVector SourceLocation = Target->GetActorLocation() + FVector(0.0f, 160.0f, 0.0f);
                AKalmalaCharacter* DamageSource = GetWorld()->SpawnActor<AKalmalaCharacter>(
                    AKalmalaCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, SourceLocation),
                    SpawnParameters);
                FKalmalaDamageChannels Hit;
                Hit.Slash = 20.0f;
                const bool bWorn = DamageSource && Target->ApplyWildlifeDamageFromServer(DamageSource, Hit)
                    && Target->GetHealth() < 100.0f
                    && FindOpalHelm(TargetInventory)
                    && FindOpalHelm(TargetInventory)->ConditionSpent == 1;
                if (DamageSource) DamageSource->Destroy();
                if (!bWorn)
                {
                    Fail(TEXT("server could not produce one accepted Opal Helm wear point"));
                    return;
                }
            }
            PublishNextOwnerStep(OpalHelmPeerReadyRepair);
            OpalHelmPeerStage = 15;
            return;
        case 15:
            if (!AwaitResult()) return;
            {
                const FKalmalaEquippedGear* Gear = FindOpalHelm(TargetInventory);
                if (TargetCrafting->GetResultSerial() != OpalHelmPeerResultSerial + 1
                    || TargetCrafting->GetLastResult() != OpalHelmPeerRepairResult
                    || !TargetCrafting->WasLastResultAccepted() || !Gear || Gear->ItemGuid != OpalHelmPeerItemGuid
                    || Gear->Level != 2 || Gear->ConditionSpent != 0
                    || TargetInventory->GetQuantity(TEXT("Iron")) != 0
                    || TargetInventory->GetQuantity(TEXT("DeerHide")) != 2)
                {
                    Fail(TEXT("remote free repair did not preserve level and payment or clear condition"));
                    return;
                }
            }
            OpalHelmPeerStage = 16;
            OpalHelmPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("Opal Helm peer server: Passed=1 RemoteOwner=1 WrongLevel=1 Distant=1 Occluded=1 ForeignGuid=1 Insufficient=1 Payment=1 Upgrade=1 Repair=1 Cadence=1"));
            return;
        case 16:
            if (Inventory->GetQuantity(TEXT("OpalHelm")) != 0 || FindOpalHelm(Inventory) != nullptr)
            {
                Fail(TEXT("listen host received another player's equipped Opal Helm"));
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("Opal Helm peer listen host: Passed=1 OwnHelm=0"));
            OpalHelmPeerStage = 17;
            return;
        default:
            return;
        }
    }

    if (!Character->IsLocallyControlled()) return;

    if (OpalHelmPeerLocalStage == 0 && GetLastResult() == OpalHelmPeerReadyWrongLevel)
    {
        const FKalmalaEquippedGear* Gear = FindOpalHelm(Inventory);
        if (!Gear || !Gear->ItemGuid.IsValid() || !HasBaseOpalPayment(Inventory)) return;
        OpalHelmPeerItemGuid = Gear->ItemGuid;
        if (!HasLevelOneHelm(Inventory, OpalHelmPeerItemGuid)) return;
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        ServerUpgradeOpalHelm(OpalHelmPeerItemGuid);
        OpalHelmPeerLocalStage = 1;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 1)
    {
        if (GetResultSerial() == OpalHelmPeerLocalResultSerial)
        {
            if (OpalHelmPeerLocalElapsed > 30.0f) Fail(TEXT("owner did not receive the wrong-level result"));
            return;
        }
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 1
            || WasLastResultAccepted()
            || !GetLastResult().Contains(TEXT("visible same-world Forge"))
            || !HasLevelOneHelm(Inventory, OpalHelmPeerItemGuid) || !HasBaseOpalPayment(Inventory))
        {
            Fail(TEXT("owner did not receive the wrong-level rejection without state changes"));
            return;
        }
        OpalHelmPeerLocalStage = 2;
    }
    if (OpalHelmPeerLocalStage == 2 && GetLastResult() == OpalHelmPeerReadyDistant)
    {
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        ServerUpgradeOpalHelm(OpalHelmPeerItemGuid);
        OpalHelmPeerLocalStage = 3;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 3)
    {
        if (GetResultSerial() == OpalHelmPeerLocalResultSerial) return;
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 1 || WasLastResultAccepted()
            || !GetLastResult().Contains(TEXT("visible same-world Forge"))
            || !HasLevelOneHelm(Inventory, OpalHelmPeerItemGuid) || !HasBaseOpalPayment(Inventory))
        {
            Fail(TEXT("owner did not receive the distant-Forge rejection"));
            return;
        }
        OpalHelmPeerLocalStage = 4;
    }
    if (OpalHelmPeerLocalStage == 4 && GetLastResult() == OpalHelmPeerReadyOccluded)
    {
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        ServerUpgradeOpalHelm(OpalHelmPeerItemGuid);
        OpalHelmPeerLocalStage = 5;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 5)
    {
        if (GetResultSerial() == OpalHelmPeerLocalResultSerial) return;
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 1 || WasLastResultAccepted()
            || !GetLastResult().Contains(TEXT("visible same-world Forge"))
            || !HasLevelOneHelm(Inventory, OpalHelmPeerItemGuid) || !HasBaseOpalPayment(Inventory))
        {
            Fail(TEXT("owner did not receive the occluded-Forge rejection"));
            return;
        }
        OpalHelmPeerLocalStage = 6;
    }
    if (OpalHelmPeerLocalStage == 6 && GetLastResult() == OpalHelmPeerReadyForeignGuid)
    {
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        ServerUpgradeOpalHelm(FGuid::NewGuid());
        OpalHelmPeerLocalStage = 7;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 7)
    {
        if (GetResultSerial() == OpalHelmPeerLocalResultSerial) return;
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 1 || WasLastResultAccepted()
            || !GetLastResult().Contains(TEXT("visible same-world Forge"))
            || !HasLevelOneHelm(Inventory, OpalHelmPeerItemGuid) || !HasBaseOpalPayment(Inventory))
        {
            Fail(TEXT("owner foreign-GUID request changed gear or payment"));
            return;
        }
        OpalHelmPeerLocalStage = 8;
    }
    if (OpalHelmPeerLocalStage == 8 && GetLastResult() == OpalHelmPeerReadyInsufficient)
    {
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        ServerUpgradeOpalHelm(OpalHelmPeerItemGuid);
        OpalHelmPeerLocalStage = 9;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 9)
    {
        if (GetResultSerial() == OpalHelmPeerLocalResultSerial) return;
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 1 || WasLastResultAccepted()
            || !GetLastResult().Contains(TEXT("Iron")) || !HasLevelOneHelm(Inventory, OpalHelmPeerItemGuid)
            || !HasBaseOpalPayment(Inventory))
        {
            Fail(TEXT("owner insufficient-material request changed gear or payment"));
            return;
        }
        OpalHelmPeerLocalStage = 10;
    }
    if (OpalHelmPeerLocalStage == 10 && GetLastResult() == OpalHelmPeerReadyCadence)
    {
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        const FGuid ForeignItemGuid = FGuid::NewGuid();
        ServerUpgradeOpalHelm(ForeignItemGuid);
        ServerUpgradeOpalHelm(ForeignItemGuid);
        OpalHelmPeerLocalStage = 11;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 11)
    {
        if (GetResultSerial() < OpalHelmPeerLocalResultSerial + 2)
        {
            if (OpalHelmPeerLocalElapsed >= 30.0f)
                Fail(TEXT("owner did not receive both rapid-request results"));
            return;
        }
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 2
            || GetLastResult() != TEXT("Please wait before the next action") || WasLastResultAccepted()
            || !HasLevelOneHelm(Inventory, OpalHelmPeerItemGuid) || !HasBaseOpalPayment(Inventory))
        {
            Fail(TEXT("owner did not receive the throttled duplicate-RPC result"));
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("Opal Helm peer owner cadence: Passed=1 RapidRepeat=Rejected State=Unchanged"));
        OpalHelmPeerLocalStage = 12;
    }
    if (OpalHelmPeerLocalStage == 12 && GetLastResult() == OpalHelmPeerReadyUpgrade)
    {
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        ServerUpgradeOpalHelm(OpalHelmPeerItemGuid);
        OpalHelmPeerLocalStage = 13;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 13)
    {
        if (GetResultSerial() == OpalHelmPeerLocalResultSerial) return;
        const FKalmalaEquippedGear* Gear = FindOpalHelm(Inventory);
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 1
            || GetLastResult() != OpalHelmPeerUpgradeResult || !WasLastResultAccepted()
            || !Gear || Gear->ItemGuid != OpalHelmPeerItemGuid || Gear->Level != 2 || Gear->ConditionSpent != 0
            || Inventory->GetQuantity(TEXT("Iron")) != 0 || Inventory->GetQuantity(TEXT("DeerHide")) != 2)
        {
            Fail(TEXT("owner did not receive the upgraded helm, exact payment, and success result"));
            return;
        }
        OpalHelmPeerLocalStage = 14;
    }
    if (OpalHelmPeerLocalStage == 14 && GetLastResult() == OpalHelmPeerReadyRepair)
    {
        const FKalmalaEquippedGear* Gear = FindOpalHelm(Inventory);
        if (!Gear || Gear->ItemGuid != OpalHelmPeerItemGuid || Gear->Level != 2 || Gear->ConditionSpent != 1) return;
        OpalHelmPeerLocalResultSerial = GetResultSerial();
        ServerRepairOpalHelm(OpalHelmPeerItemGuid);
        OpalHelmPeerLocalStage = 15;
        OpalHelmPeerLocalElapsed = 0.0f;
        return;
    }
    if (OpalHelmPeerLocalStage == 15)
    {
        if (GetResultSerial() == OpalHelmPeerLocalResultSerial) return;
        const FKalmalaEquippedGear* Gear = FindOpalHelm(Inventory);
        if (GetResultSerial() != OpalHelmPeerLocalResultSerial + 1
            || GetLastResult() != OpalHelmPeerRepairResult || !WasLastResultAccepted()
            || !Gear || Gear->ItemGuid != OpalHelmPeerItemGuid || Gear->Level != 2 || Gear->ConditionSpent != 0
            || Inventory->GetQuantity(TEXT("Iron")) != 0 || Inventory->GetQuantity(TEXT("DeerHide")) != 2)
        {
            Fail(TEXT("owner did not receive the repaired helm and free-repair result"));
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("Opal Helm peer owner: Passed=1 Authority=0 Upgrade=1 Repair=1 Feedback=1 Cadence=1"));
        OpalHelmPeerLocalStage = 16;
    }

    if (OpalHelmPeerLocalStage >= 16) return;

    int32 OwnerPlayerId = INDEX_NONE;
    if (AKalmalaConstructionActor* Forge = FindOpalPeerForge(GetWorld(), &OwnerPlayerId))
    {
        if (!bOpalHelmPeerFixtureObserved)
        {
            bOpalHelmPeerFixtureObserved = true;
            OpalHelmPeerObservedOwnerPlayerId = OwnerPlayerId;
            OpalHelmPeerLocalElapsed = 0.0f;
        }
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == OpalHelmPeerObservedOwnerPlayerId)
            {
                Owner = *It;
                break;
            }
        }
        if (Owner && Owner->GetHealth() < 100.0f && OpalHelmPeerLocalElapsed >= 1.0f)
        {
            UKalmalaInventoryComponent* OwnerInventory = Owner->GetInventoryComponent();
            const bool bPrivate = OwnerInventory && OwnerInventory->GetStacks().IsEmpty()
                && OwnerInventory->GetEquippedGear().IsEmpty()
                && Inventory->GetQuantity(TEXT("OpalHelm")) == 0 && FindOpalHelm(Inventory) == nullptr;
            if (bPrivate)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("Opal Helm peer observer: Passed=1 Authority=0 TargetPrivate=1"));
                bOpalHelmPeerFixtureObserved = false;
                OpalHelmPeerLocalStage = 17;
            }
            else
            {
                Fail(TEXT("observer received another player's equipped Opal Helm data"));
            }
        }
        if (OpalHelmPeerLocalElapsed > 60.0f)
            Fail(TEXT("observer did not see the worn Opal Helm fixture"));
        (void)Forge;
    }
}

#endif // !UE_BUILD_SHIPPING
