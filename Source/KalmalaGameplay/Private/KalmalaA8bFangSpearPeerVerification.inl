#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCharacter.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWildlifeSpawn.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR FangSpearItemId[] = TEXT("FangSpear");
    constexpr TCHAR FangSpearReadyResult[] = TEXT("A8b Fang Spear peer ready");
    constexpr TCHAR FangSpearAttackResult[] = TEXT("A8b Fang Spear peer attack accepted");
    constexpr TCHAR FangSpearProgressedResult[] = TEXT("Crafted FangSpear at level 1");
    constexpr TCHAR FangSpearForgeId[] = TEXT("A8bPeerFangSpearForge");
    constexpr TCHAR FangSpearTargetId[] = TEXT("a8b-peer-fang-spear-target");

    bool HasReadyFangSpearOption(const FString& ToolOptions)
    {
        const int32 EntryStart = ToolOptions.Find(TEXT("New tool: Fang Spear 1"));
        if (EntryStart == INDEX_NONE) return false;
        const int32 StatusStart = ToolOptions.Find(
            TEXT("Status: "), ESearchCase::CaseSensitive, ESearchDir::FromStart, EntryStart);
        if (StatusStart == INDEX_NONE) return false;
        const int32 StatusEnd = ToolOptions.Find(
            TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, StatusStart);
        const FString Status = StatusEnd == INDEX_NONE
            ? ToolOptions.RightChop(StatusStart)
            : ToolOptions.Mid(StatusStart, StatusEnd - StatusStart);
        return Status == TEXT("Status: Ready to upgrade.");
    }

    const FKalmalaToolState* FindFangSpear(const AKalmalaCharacter* Character)
    {
        return Character ? Character->GetCarriedToolInventory().FindByPredicate(
            [](const FKalmalaToolState& Tool) { return Tool.ItemId == TEXT("FangSpear"); }) : nullptr;
    }

    int32 CountFangSpears(const AKalmalaCharacter* Character)
    {
        if (!Character) return 0;
        return Character->GetCarriedToolInventory().FilterByPredicate(
            [](const FKalmalaToolState& Tool) { return Tool.ItemId == TEXT("FangSpear"); }).Num();
    }

    AKalmalaConstructionActor* FindFangSpearForge(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
            if (It->GetConstructionId().StartsWith(FString(FangSpearForgeId) + TEXT("_"))) return *It;
        return nullptr;
    }

    bool GetFangSpearOwnerId(const AKalmalaConstructionActor* Forge, int32& OutPlayerId)
    {
        if (!Forge) return false;
        const FString Prefix = FString(FangSpearForgeId) + TEXT("_");
        if (!Forge->GetConstructionId().StartsWith(Prefix)) return false;
        const FString Suffix = Forge->GetConstructionId().RightChop(Prefix.Len());
        if (!Suffix.IsNumeric()) return false;
        OutPlayerId = FCString::Atoi(*Suffix);
        return true;
    }

    AKalmalaWildlifeSpawn* FindFangSpearTarget(UWorld* World)
    {
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
            if (It->GetCreatureNicheId() == FName(FangSpearTargetId)) return *It;
        return nullptr;
    }

    void FaceFangSpearTarget(AKalmalaCharacter* Character, const AActor* Target)
    {
        if (!Character || !Target) return;
        const FRotator Facing = (Target->GetActorLocation() - Character->GetActorLocation()).Rotation();
        const FRotator YawOnly(0.0f, Facing.Yaw, 0.0f);
        Character->SetActorRotation(YawOnly);
        if (APlayerController* Controller = Cast<APlayerController>(Character->GetController()))
            Controller->SetControlRotation(YawOnly);
    }
}

void UKalmalaCraftingComponent::RunA8bFangSpearPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !GetWorld()) return;
    A8bFangSpearPeerElapsed += DeltaTime;

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || A8bFangSpearPeerStage >= 4) return;
        if (A8bFangSpearPeerStage == 0)
        {
            if (A8bFangSpearPeerElapsed < 3.0f) return;
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

            A8bFangSpearPeerHostBaselineWolfFang = Inventory->GetQuantity(TEXT("WolfFang"));
            A8bFangSpearPeerHostBaselineIron = Inventory->GetQuantity(TEXT("Iron"));
            A8bFangSpearPeerHostBaselineDensewood = Inventory->GetQuantity(TEXT("Densewood"));
            A8bFangSpearPeerHostBaselineSpear = Inventory->GetQuantity(TEXT("FangSpear"));
            A8bFangSpearPeerHostBaselineTools = CountFangSpears(Character);
            UKalmalaInventoryComponent* TargetInventory = Target->FindComponentByClass<UKalmalaInventoryComponent>();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            UKalmalaCombatComponent* TargetCombat = Target->FindComponentByClass<UKalmalaCombatComponent>();
            if (!TargetInventory || !TargetCrafting || !TargetCombat || CountFangSpears(Target) != 0)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer FAILED: remote owner components incomplete or pre-existing test weapon found"));
                A8bFangSpearPeerStage = 99;
                return;
            }

            A8bFangSpearPeerBaselineWolfFang = TargetInventory->GetQuantity(TEXT("WolfFang"));
            A8bFangSpearPeerBaselineIron = TargetInventory->GetQuantity(TEXT("Iron"));
            A8bFangSpearPeerBaselineDensewood = TargetInventory->GetQuantity(TEXT("Densewood"));
            A8bFangSpearPeerBaselineSpear = TargetInventory->GetQuantity(TEXT("FangSpear"));
            if (!TargetInventory->TryGrantFromServer(TEXT("WolfFang"), 4)
                || !TargetInventory->TryGrantFromServer(TEXT("Iron"), 10)
                || !TargetInventory->TryGrantFromServer(TEXT("Densewood"), 10))
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer FAILED: could not stage Forge ingredients"));
                A8bFangSpearPeerStage = 99;
                return;
            }

            const FVector Forward = Target->GetActorForwardVector().GetSafeNormal2D();
            const FVector WildlifeLocation = Target->GetPawnViewLocation() + Forward * 170.0f;
            const FVector ForgeLocation = Target->GetPawnViewLocation() + FVector(0.0f, 0.0f, 100.0f);
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Forge = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, ForgeLocation), SpawnParameters);
            AKalmalaWildlifeSpawn* Wildlife = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(
                AKalmalaWildlifeSpawn::StaticClass(), FTransform(FRotator::ZeroRotator, WildlifeLocation), SpawnParameters);
            if (!Forge || !Wildlife)
            {
                if (Forge) Forge->Destroy();
                if (Wildlife) Wildlife->Destroy();
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer FAILED: Forge or controlled target did not spawn"));
                A8bFangSpearPeerStage = 99;
                return;
            }

            const FString ForgeId = FString::Printf(TEXT("%s_%d"), FangSpearForgeId,
                Target->GetPlayerState()->GetPlayerId());
            Forge->InitializeFromServer(TEXT("ForgeKit"), ForgeId);
            FKalmalaWorldPopulationSpawn WildlifeSpawn;
            WildlifeSpawn.SpawnSeed = 8675321;
            WildlifeSpawn.Location = WildlifeLocation;
            WildlifeSpawn.ContentId = FName(FangSpearTargetId);
            Wildlife->InitializeServer(WildlifeSpawn, EKalmalaWildlifeArchetype::MoonBear);
            Wildlife->SetActorTickEnabled(false);
            if (UKalmalaResistanceComponent* Resistance = Wildlife->FindComponentByClass<UKalmalaResistanceComponent>())
                Resistance->Profile.Armor = 0.0f;
            FaceFangSpearTarget(Target, Wildlife);

            const double ForgeDistance = FVector::Dist(Target->GetActorLocation(), Forge->GetActorLocation());
            const FString ToolOptions = TargetCrafting->GetToolProgressionText(TEXT("ForgeKit"));
            FCollisionQueryParams StationQuery(SCENE_QUERY_STAT(A8bFangSpearStationDiagnostics), false, Target);
            FHitResult StationHit;
            const bool bStationTraceHit = GetWorld()->LineTraceSingleByChannel(
                StationHit, Target->GetPawnViewLocation(), Forge->GetActorLocation(), ECC_Visibility, StationQuery);
            const bool bStationReady = ForgeDistance <= 250.0
                && TargetCrafting->FindNearbyToolProgressionStation(TEXT("ForgeKit")) != nullptr
                && HasReadyFangSpearOption(ToolOptions);
            const bool bTargetReady = Wildlife->GetArchetype() == EKalmalaWildlifeArchetype::MoonBear
                && Wildlife->GetCreatureNicheId() == FName(FangSpearTargetId)
                && FMath::IsNearlyEqual(Wildlife->GetHealth(), 250.0f, 0.1f)
                && Wildlife->GetHealth() > 100.0f;
            if (!bStationReady || !bTargetReady)
            {
                const int32 FangTextStart = ToolOptions.Find(TEXT("Fang"));
                const FString FangText = FangTextStart == INDEX_NONE ? TEXT("<missing>")
                    : ToolOptions.Mid(FangTextStart, 180).Replace(TEXT("\n"), TEXT(" | "));
                Forge->Destroy();
                Wildlife->Destroy();
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Spear peer FAILED: station=%d visibleForge=%d distance=%.1f optionReady=%d target=%d"),
                    bStationReady ? 1 : 0,
                    TargetCrafting->FindNearbyToolProgressionStation(TEXT("ForgeKit")) ? 1 : 0,
                    ForgeDistance,
                    HasReadyFangSpearOption(ToolOptions) ? 1 : 0,
                    bTargetReady ? 1 : 0);
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Spear peer station diagnostics: Authority=%d ForgeAuthority=%d Kit=%s Id=%s TraceHit=%d HitActor=%s HitComponent=%s HitForge=%d ToolHeader=%d Ready=%d FangText=%s"),
                    Target->HasAuthority() ? 1 : 0, Forge->HasAuthority() ? 1 : 0,
                    *Forge->GetConstructionKit().ToString(), *Forge->GetConstructionId(),
                    bStationTraceHit ? 1 : 0,
                    bStationTraceHit && StationHit.GetActor() ? *StationHit.GetActor()->GetName() : TEXT("None"),
                    bStationTraceHit && StationHit.GetComponent() ? *StationHit.GetComponent()->GetName() : TEXT("None"),
                    bStationTraceHit && StationHit.GetActor() == Forge ? 1 : 0,
                    ToolOptions.Contains(TEXT("New tool: Fang Spear 1")) ? 1 : 0,
                    HasReadyFangSpearOption(ToolOptions) ? 1 : 0, *FangText);
                A8bFangSpearPeerStage = 99;
                return;
            }

            A8bFangSpearPeerTarget = Target;
            A8bFangSpearPeerWildlife = Wildlife;
            A8bFangSpearPeerBaselineWildlifeHealth = Wildlife->GetHealth();
            A8bFangSpearPeerResultSerial = TargetCrafting->GetResultSerial();
            TargetCrafting->PublishResult(FangSpearReadyResult, true);
            A8bFangSpearPeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Forge->ForceNetUpdate();
            Wildlife->ForceNetUpdate();
            A8bFangSpearPeerStage = 1;
            A8bFangSpearPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Spear peer fixture: Ready=1 RemoteOwner=1 WolfFang=4 Iron=10 Densewood=10 Forge=1 Target=MoonBear"));
            return;
        }
        if (A8bFangSpearPeerStage == 1)
        {
            AKalmalaCharacter* Target = A8bFangSpearPeerTarget.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCraftingComponent* TargetCrafting = Target
                ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
            UKalmalaCombatComponent* TargetCombat = Target
                ? Target->FindComponentByClass<UKalmalaCombatComponent>() : nullptr;
            if (!Target || !TargetInventory || !TargetCrafting || !TargetCombat)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer FAILED: remote owner disappeared before Forge result"));
                A8bFangSpearPeerStage = 99;
                return;
            }
            if (TargetCrafting->GetResultSerial() == A8bFangSpearPeerResultSerial)
            {
                if (A8bFangSpearPeerElapsed > 25.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer server: craft result timed out"));
                    A8bFangSpearPeerStage = 99;
                }
                return;
            }

            const FKalmalaToolState* Spear = FindFangSpear(Target);
            const FKalmalaDamageChannels Damage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(
                TEXT("FangSpear"), 1, TEXT("Basic"));
            const bool bProfileValid = FMath::IsNearlyEqual(Damage.Stab, 100.0f, 0.1f)
                && FMath::IsNearlyZero(Damage.Total() - Damage.Stab, 0.1f);
            const bool bCraftPassed = TargetCrafting->GetLastResult() == FangSpearProgressedResult
                && TargetCrafting->WasLastResultAccepted()
                && TargetInventory->GetQuantity(TEXT("WolfFang")) == A8bFangSpearPeerBaselineWolfFang
                && TargetInventory->GetQuantity(TEXT("Iron")) == A8bFangSpearPeerBaselineIron
                && TargetInventory->GetQuantity(TEXT("Densewood")) == A8bFangSpearPeerBaselineDensewood
                && TargetInventory->GetQuantity(TEXT("FangSpear")) == A8bFangSpearPeerBaselineSpear
                && CountFangSpears(Target) == 1 && Spear && Spear->InstanceGuid.IsValid()
                && Spear->ToolLevel == 1 && Spear->Durability == 200 && bProfileValid;
            if (!bCraftPassed)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("A8b Fang Spear peer server: Passed=0 Craft=%d Accepted=%d WolfFang=%d Iron=%d Densewood=%d Stack=%d Tools=%d Durability=%d Stab=%.1f"),
                    TargetCrafting->GetLastResult() == FangSpearProgressedResult ? 1 : 0,
                    TargetCrafting->WasLastResultAccepted() ? 1 : 0,
                    TargetInventory->GetQuantity(TEXT("WolfFang")), TargetInventory->GetQuantity(TEXT("Iron")),
                    TargetInventory->GetQuantity(TEXT("Densewood")), TargetInventory->GetQuantity(TEXT("FangSpear")),
                    CountFangSpears(Target), Spear ? Spear->Durability : INDEX_NONE, Damage.Stab);
                A8bFangSpearPeerStage = 99;
                return;
            }
            A8bFangSpearPeerItemGuid = Spear->InstanceGuid;
            A8bFangSpearPeerBaselineActionSerial = TargetCombat->GetActionSerial();
            A8bFangSpearPeerStage = 2;
            A8bFangSpearPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Spear peer server craft: Accepted=1 Payment=1 CarriedOutput=1 Guid=1 Durability=200 Stab=100"));
            return;
        }
        if (A8bFangSpearPeerStage == 2)
        {
            AKalmalaCharacter* Target = A8bFangSpearPeerTarget.Get();
            AKalmalaWildlifeSpawn* Wildlife = A8bFangSpearPeerWildlife.Get();
            UKalmalaInventoryComponent* TargetInventory = Target
                ? Target->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
            UKalmalaCombatComponent* TargetCombat = Target
                ? Target->FindComponentByClass<UKalmalaCombatComponent>() : nullptr;
            const float ExpectedHealth = A8bFangSpearPeerBaselineWildlifeHealth - 100.0f;
            const bool bAcceptedStab = Target && Wildlife && TargetInventory && TargetCombat
                && TargetCombat->GetActionSerial() > A8bFangSpearPeerBaselineActionSerial
                && TargetCombat->GetFeedback() == EKalmalaCombatFeedback::Hit
                && FMath::IsNearlyEqual(Wildlife->GetHealth(), ExpectedHealth, 0.1f)
                && Target->GetToolDurability(A8bFangSpearPeerItemGuid) == 200
                && TargetInventory->GetActiveItem() == TEXT("FangSpear")
                && TargetInventory->GetActiveInstanceGuid() == A8bFangSpearPeerItemGuid;
            if (!bAcceptedStab)
            {
                if (A8bFangSpearPeerElapsed > 20.0f)
                {
                    UE_LOG(LogTemp, Error,
                        TEXT("A8b Fang Spear peer server: Passed=0 Action=%u/%u Feedback=%d Health=%.1f/%.1f Durability=%d MainHand=%s"),
                        TargetCombat ? TargetCombat->GetActionSerial() : 0,
                        A8bFangSpearPeerBaselineActionSerial,
                        TargetCombat ? static_cast<int32>(TargetCombat->GetFeedback()) : INDEX_NONE,
                        Wildlife ? Wildlife->GetHealth() : -1.0f, ExpectedHealth,
                        Target ? Target->GetToolDurability(A8bFangSpearPeerItemGuid) : INDEX_NONE,
                        TargetInventory ? *TargetInventory->GetActiveItem().ToString() : TEXT("None"));
                    A8bFangSpearPeerStage = 99;
                }
                return;
            }
            if (UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>())
                TargetCrafting->PublishResult(FangSpearAttackResult, true);
            Target->ForceNetUpdate();
            Wildlife->ForceNetUpdate();
            A8bFangSpearPeerStage = 3;
            A8bFangSpearPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Spear peer server: Passed=1 Craft=1 Payment=1 Equip=1 Stab=100 Damage=100 Durability=200"));
            return;
        }
        if (A8bFangSpearPeerStage == 3)
        {
            const bool bHostStable = Inventory->GetQuantity(TEXT("WolfFang")) == A8bFangSpearPeerHostBaselineWolfFang
                && Inventory->GetQuantity(TEXT("Iron")) == A8bFangSpearPeerHostBaselineIron
                && Inventory->GetQuantity(TEXT("Densewood")) == A8bFangSpearPeerHostBaselineDensewood
                && Inventory->GetQuantity(TEXT("FangSpear")) == A8bFangSpearPeerHostBaselineSpear
                && CountFangSpears(Character) == A8bFangSpearPeerHostBaselineTools;
            if (!bHostStable)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer listen host: Passed=0 HostItems=Changed"));
                A8bFangSpearPeerStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("A8b Fang Spear peer listen host: Passed=1 HostItems=Stable"));
            A8bFangSpearPeerStage = 4;
        }
        return;
    }

    if (!Character->IsLocallyControlled()) return;
    UKalmalaCraftingComponent* OwnerCrafting = Character->FindComponentByClass<UKalmalaCraftingComponent>();
    UKalmalaCombatComponent* Combat = Character->FindComponentByClass<UKalmalaCombatComponent>();
    if (!OwnerCrafting || !Combat) return;

    AKalmalaConstructionActor* Forge = FindFangSpearForge(GetWorld());
    AKalmalaWildlifeSpawn* Wildlife = FindFangSpearTarget(GetWorld());
    if (Forge && !bA8bFangSpearPeerFixtureObserved)
    {
        int32 OwnerPlayerId = INDEX_NONE;
        if (!GetFangSpearOwnerId(Forge, OwnerPlayerId))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer observer: Passed=0 invalid Forge owner identity"));
            A8bFangSpearPeerStage = 99;
            return;
        }
        if (Character->GetPlayerState()->GetPlayerId() != OwnerPlayerId && Wildlife)
        {
            A8bFangSpearPeerObservedOwnerPlayerId = OwnerPlayerId;
            bA8bFangSpearPeerFixtureObserved = true;
            A8bFangSpearPeerElapsed = 0.0f;
        }
    }

    if (bA8bFangSpearPeerFixtureObserved)
    {
        if (A8bFangSpearPeerStage >= 4) return;
        AKalmalaCharacter* Owner = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            if (It->GetPlayerState()
                && It->GetPlayerState()->GetPlayerId() == A8bFangSpearPeerObservedOwnerPlayerId)
            { Owner = *It; break; }
        const UKalmalaInventoryComponent* OwnerInventory = Owner
            ? Owner->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        const UKalmalaPlayerStatusComponent* OwnerStatus = Owner
            ? Owner->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
        const bool bPublicHit = Wildlife
            && Wildlife->GetHealth() <= AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(
                EKalmalaWildlifeArchetype::MoonBear) - 99.9f;
        const bool bPrivate = Owner && !Owner->IsLocallyControlled() && OwnerInventory && OwnerStatus
            && OwnerInventory->GetStacks().IsEmpty() && OwnerInventory->GetEquippedGear().IsEmpty()
            && Owner->GetCarriedToolInventory().IsEmpty() && OwnerStatus->GetStatuses().IsEmpty()
            && Inventory->GetQuantity(TEXT("WolfFang")) == 0
            && Inventory->GetQuantity(TEXT("FangSpear")) == 0;
        if (bPublicHit && bPrivate)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Spear peer observer: Passed=1 Authority=0 PublicHealth=1 OwnerPrivate=1"));
            A8bFangSpearPeerStage = 4;
        }
        else if (A8bFangSpearPeerElapsed > 45.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Spear peer observer: Passed=0 OwnerFound=%d PublicHealth=%d OwnerPrivate=%d"),
                Owner ? 1 : 0, bPublicHit ? 1 : 0, bPrivate ? 1 : 0);
            A8bFangSpearPeerStage = 99;
        }
        return;
    }

    if (A8bFangSpearPeerStage == 0 && OwnerCrafting->GetLastResult() == FangSpearReadyResult)
    {
        const FString ToolOptions = OwnerCrafting->GetToolProgressionText(TEXT("ForgeKit"));
        if (!Wildlife || Inventory->GetQuantity(TEXT("WolfFang")) < 4
            || Inventory->GetQuantity(TEXT("Iron")) < 10 || Inventory->GetQuantity(TEXT("Densewood")) < 10
            || !HasReadyFangSpearOption(ToolOptions))
        {
            if (A8bFangSpearPeerElapsed < 20.0f) return;
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer owner: Passed=0 Forge, target or recipe inputs not ready"));
            A8bFangSpearPeerStage = 99;
            return;
        }

        A8bFangSpearPeerBaselineWolfFang = Inventory->GetQuantity(TEXT("WolfFang")) - 4;
        A8bFangSpearPeerBaselineIron = Inventory->GetQuantity(TEXT("Iron")) - 10;
        A8bFangSpearPeerBaselineDensewood = Inventory->GetQuantity(TEXT("Densewood")) - 10;
        A8bFangSpearPeerBaselineSpear = Inventory->GetQuantity(TEXT("FangSpear"));
        A8bFangSpearPeerResultSerial = OwnerCrafting->GetResultSerial();
        FString DirectReason;
        if (OwnerCrafting->ProgressToolFromServer(FangSpearItemId, DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer FAILED: direct client progression bypassed server authority"));
            A8bFangSpearPeerStage = 99;
            return;
        }
        OwnerCrafting->ServerProgressTool(FangSpearItemId);
        A8bFangSpearPeerStage = 1;
        A8bFangSpearPeerElapsed = 0.0f;
        return;
    }
    if (A8bFangSpearPeerStage == 1)
    {
        if (OwnerCrafting->GetResultSerial() == A8bFangSpearPeerResultSerial)
        {
            if (A8bFangSpearPeerElapsed > 25.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer owner: Passed=0 craft result timed out"));
                A8bFangSpearPeerStage = 99;
            }
            return;
        }
        const FKalmalaToolState* Spear = FindFangSpear(Character);
        const bool bCrafted = OwnerCrafting->GetLastResult() == FangSpearProgressedResult
            && OwnerCrafting->WasLastResultAccepted()
            && Inventory->GetQuantity(TEXT("WolfFang")) == A8bFangSpearPeerBaselineWolfFang
            && Inventory->GetQuantity(TEXT("Iron")) == A8bFangSpearPeerBaselineIron
            && Inventory->GetQuantity(TEXT("Densewood")) == A8bFangSpearPeerBaselineDensewood
            && Inventory->GetQuantity(TEXT("FangSpear")) == A8bFangSpearPeerBaselineSpear
            && CountFangSpears(Character) == 1 && Spear && Spear->InstanceGuid.IsValid()
            && Spear->ToolLevel == 1 && Spear->Durability == 200;
        if (!bCrafted)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Spear peer owner: Passed=0 Result=%s Accepted=%d WolfFang=%d Iron=%d Densewood=%d Stack=%d Tools=%d"),
                *OwnerCrafting->GetLastResult(), OwnerCrafting->WasLastResultAccepted() ? 1 : 0,
                Inventory->GetQuantity(TEXT("WolfFang")), Inventory->GetQuantity(TEXT("Iron")),
                Inventory->GetQuantity(TEXT("Densewood")), Inventory->GetQuantity(TEXT("FangSpear")),
                CountFangSpears(Character));
            A8bFangSpearPeerStage = 99;
            return;
        }
        A8bFangSpearPeerItemGuid = Spear->InstanceGuid;
        FString DirectReason;
        if (Inventory->SetMainHandFromServer(A8bFangSpearPeerItemGuid, DirectReason))
        {
            UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer FAILED: direct client Main Hand mutation was accepted"));
            A8bFangSpearPeerStage = 99;
            return;
        }
        A8bFangSpearPeerGearRequestSerial = Inventory->GetGearRequestResultSerial();
        Inventory->ServerSetMainHand(A8bFangSpearPeerItemGuid);
        A8bFangSpearPeerStage = 2;
        A8bFangSpearPeerElapsed = 0.0f;
        return;
    }
    if (A8bFangSpearPeerStage == 2)
    {
        if (Inventory->GetGearRequestResultSerial() == A8bFangSpearPeerGearRequestSerial)
        {
            if (A8bFangSpearPeerElapsed > 20.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("A8b Fang Spear peer owner: Main Hand RPC timed out"));
                A8bFangSpearPeerStage = 99;
            }
            return;
        }
        if (!Inventory->WasLastGearRequestAccepted()
            || Inventory->GetActiveItem() != TEXT("FangSpear")
            || Inventory->GetActiveInstanceGuid() != A8bFangSpearPeerItemGuid || !Wildlife)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Spear peer owner: Passed=0 MainHandAccepted=%d Item=%s Guid=%d Target=%d Reason=%s"),
                Inventory->WasLastGearRequestAccepted() ? 1 : 0, *Inventory->GetActiveItem().ToString(),
                Inventory->GetActiveInstanceGuid() == A8bFangSpearPeerItemGuid ? 1 : 0,
                Wildlife ? 1 : 0, *Inventory->GetLastGearRequestReason());
            A8bFangSpearPeerStage = 99;
            return;
        }
        A8bFangSpearPeerBaselineActionSerial = Combat->GetActionSerial();
        FaceFangSpearTarget(Character, Wildlife);
        Combat->ServerRequestAttack(8701);
        A8bFangSpearPeerStage = 3;
        A8bFangSpearPeerElapsed = 0.0f;
        return;
    }
    if (A8bFangSpearPeerStage == 3)
    {
        const FKalmalaToolState* Spear = FindFangSpear(Character);
        const bool bAttackPassed = OwnerCrafting->GetLastResult() == FangSpearAttackResult
            && OwnerCrafting->WasLastResultAccepted()
            && Combat->GetActionSerial() > A8bFangSpearPeerBaselineActionSerial
            && Combat->GetFeedback() == EKalmalaCombatFeedback::Hit && Wildlife
            && FMath::IsNearlyEqual(Wildlife->GetHealth(), 150.0f, 0.1f)
            && Spear && Spear->InstanceGuid == A8bFangSpearPeerItemGuid && Spear->Durability == 200
            && Inventory->GetQuantity(TEXT("WolfFang")) == A8bFangSpearPeerBaselineWolfFang
            && Inventory->GetQuantity(TEXT("Iron")) == A8bFangSpearPeerBaselineIron
            && Inventory->GetQuantity(TEXT("Densewood")) == A8bFangSpearPeerBaselineDensewood;
        if (bAttackPassed)
        {
            UE_LOG(LogTemp, Display,
                TEXT("A8b Fang Spear peer owner: Passed=1 Authority=0 Craft=1 Payment=1 MainHand=1 Stab=100 Damage=100 Durability=200"));
            A8bFangSpearPeerStage = 4;
        }
        else if (A8bFangSpearPeerElapsed > 25.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("A8b Fang Spear peer owner: Passed=0 Result=%s Action=%u/%u Feedback=%d Health=%.1f Durability=%d"),
                *OwnerCrafting->GetLastResult(), Combat->GetActionSerial(), A8bFangSpearPeerBaselineActionSerial,
                static_cast<int32>(Combat->GetFeedback()), Wildlife ? Wildlife->GetHealth() : -1.0f,
                Spear ? Spear->Durability : INDEX_NONE);
            A8bFangSpearPeerStage = 99;
        }
    }
}

#endif
