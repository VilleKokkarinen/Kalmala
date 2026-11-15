#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaSkillProgressionContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace
{
    constexpr TCHAR SkillPrivacyPeerCauldronPrefix[] = TEXT("RF45SkillCauldron_");
    constexpr TCHAR SkillPrivacyPeerReady[] = TEXT("RF45 Skill privacy peer ready");
    constexpr TCHAR SkillPrivacyPeerRecipe[] = TEXT("HearthBrothRecipe");
    constexpr int32 SkillPrivacyPeerCraftCount = 10;
    constexpr int32 SkillPrivacyPeerExperiencePerCraft = 10;

    bool HasExpectedSkillStates(const TArray<FKalmalaSkillState>& States, const int32 CookingExperience)
    {
        const TArray<EKalmalaSkill> AllowedSkills = FKalmalaSkillProgressionContract::GetAllowlistedSkills();
        if (States.Num() != AllowedSkills.Num() || AllowedSkills.Num() != 6) return false;

        for (const EKalmalaSkill Skill : AllowedSkills)
        {
            const FKalmalaSkillState* State = States.FindByPredicate([Skill](const FKalmalaSkillState& Entry)
            {
                return Entry.Skill == Skill;
            });
            const int32 ExpectedExperience = Skill == EKalmalaSkill::Cooking ? CookingExperience : 0;
            const int32 ExpectedLevel = FKalmalaSkillProgressionContract::GetLevelForExperience(ExpectedExperience);
            const uint8 ExpectedUnlockMask = FKalmalaSkillProgressionContract::GetUnlockMaskForLevel(ExpectedLevel);
            if (!State || !State->IsValid()
                || State->Experience != ExpectedExperience || State->Level != ExpectedLevel
                || State->UnlockMask != ExpectedUnlockMask) return false;
        }
        return true;
    }

    bool HasExpectedServerLedger(const FKalmalaSkillProgressionLedger& Ledger, const int32 CookingExperience)
    {
        return Ledger.IsValid() && HasExpectedSkillStates(Ledger.Skills, CookingExperience);
    }

    bool HasExpectedSkillPeerBadge(
        const UKalmalaSkillProgressionComponent* Progression,
        const int32 CookingExperience)
    {
        if (!Progression) return false;
        const FKalmalaSkillPeerPresentationState& Badge = Progression->GetPeerPresentation();
        const int32 CookingLevel = FKalmalaSkillProgressionContract::GetLevelForExperience(CookingExperience);
        const uint8 ExpectedHighestLevel = static_cast<uint8>(FMath::Max(1, CookingLevel));
        const uint8 ExpectedUnlockMask = FKalmalaSkillProgressionContract::GetUnlockMaskForLevel(ExpectedHighestLevel);
        return Badge.IsValid() && Badge.HighestLevel == ExpectedHighestLevel
            && Badge.HighestUnlockMask == ExpectedUnlockMask;
    }

    AKalmalaConstructionActor* FindSkillPrivacyPeerCauldron(UWorld* World, int32& OutOwnerPlayerId)
    {
        OutOwnerPlayerId = INDEX_NONE;
        if (!World) return nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            const FString Id = It->GetConstructionId();
            if (!Id.StartsWith(SkillPrivacyPeerCauldronPrefix)) continue;
            const FString Suffix = Id.RightChop(UE_ARRAY_COUNT(SkillPrivacyPeerCauldronPrefix) - 1);
            if (!Suffix.IsNumeric()) continue;
            OutOwnerPlayerId = FCString::Atoi(*Suffix);
            return *It;
        }
        return nullptr;
    }

    AKalmalaCharacter* FindSkillPrivacyPeerCharacter(UWorld* World, const int32 OwnerPlayerId)
    {
        if (!World || OwnerPlayerId == INDEX_NONE) return nullptr;
        for (TActorIterator<AKalmalaCharacter> It(World); It; ++It)
        {
            if (It->GetPlayerState() && It->GetPlayerState()->GetPlayerId() == OwnerPlayerId) return *It;
        }
        return nullptr;
    }
}

void UKalmalaCraftingComponent::RunSkillPrivacyPeerVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character
        ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    UKalmalaSkillProgressionComponent* Progression = Character
        ? Character->GetSkillProgressionComponent() : nullptr;
    if (!Character || !Character->GetPlayerState() || !Inventory || !Progression || !GetWorld()) return;
    SkillPrivacyPeerElapsed += DeltaTime;
    SkillPrivacyPeerLocalElapsed += DeltaTime;

    const auto Fail = [this](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Error, TEXT("Skill privacy peer FAILED: %s"), Reason);
        SkillPrivacyPeerStage = 99;
        SkillPrivacyPeerLocalStage = 99;
    };

    if (Character->HasAuthority())
    {
        if (!Character->IsLocallyControlled() || SkillPrivacyPeerStage >= 3) return;
        if (SkillPrivacyPeerStage == 0)
        {
            if (SkillPrivacyPeerElapsed < 3.0f) return;
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

            UKalmalaInventoryComponent* TargetInventory = Target->GetInventoryComponent();
            UKalmalaCraftingComponent* TargetCrafting = Target->FindComponentByClass<UKalmalaCraftingComponent>();
            UKalmalaSkillProgressionComponent* TargetProgression = Target->GetSkillProgressionComponent();
            if (!TargetInventory || !TargetCrafting || !TargetProgression
                || !HasExpectedServerLedger(TargetProgression->GetServerLedger(), 0))
            {
                Fail(TEXT("remote owner or fresh six-skill server ledger is invalid"));
                return;
            }

            SkillPrivacyPeerBaselineDeerMeat = TargetInventory->GetQuantity(TEXT("DeerMeat"));
            SkillPrivacyPeerBaselineOnion = TargetInventory->GetQuantity(TEXT("Onion"));
            SkillPrivacyPeerBaselineHearthBroth = TargetInventory->GetQuantity(TEXT("HearthBroth"));
            SkillPrivacyPeerBaselineWood = TargetInventory->GetQuantity(TEXT("Wood"));
            if (!TargetInventory->TryGrantFromServer(TEXT("DeerMeat"), SkillPrivacyPeerCraftCount)
                || !TargetInventory->TryGrantFromServer(TEXT("Onion"), SkillPrivacyPeerCraftCount)
                || !TargetInventory->TryGrantFromServer(TEXT("Wood"), 1))
            {
                Fail(TEXT("server could not stage the authored recipe ingredients and hearth fuel"));
                return;
            }

            const FVector CauldronLocation = Target->GetActorLocation() + FVector(0.0f, 150.0f, 0.0f);
            const FVector FireLocation = CauldronLocation + FVector(150.0f, 0.0f, 0.0f);
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaConstructionActor* Cauldron = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
                AKalmalaConstructionActor::StaticClass(), FTransform(FRotator::ZeroRotator, CauldronLocation),
                SpawnParameters);
            AKalmalaCampfire* Fire = GetWorld()->SpawnActor<AKalmalaCampfire>(
                AKalmalaCampfire::StaticClass(), FTransform(FRotator::ZeroRotator, FireLocation), SpawnParameters);
            if (!Cauldron || !Fire)
            {
                if (Cauldron) Cauldron->Destroy();
                if (Fire) Fire->Destroy();
                Fail(TEXT("test Cauldron or hearth did not spawn"));
                return;
            }

            const int32 OwnerPlayerId = Target->GetPlayerState()->GetPlayerId();
            Cauldron->InitializeFromServer(TEXT("CauldronKit"), FString::Printf(TEXT("%s%d"),
                SkillPrivacyPeerCauldronPrefix, OwnerPlayerId));
            Fire->SetSharedFromServer(true);
            AKalmalaWorldGenerationGameState* WorldState =
                GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
            if (!WorldState)
            {
                Cauldron->Destroy();
                Fire->Destroy();
                Fail(TEXT("world-generation state is unavailable"));
                return;
            }
            FKalmalaWeatherState DryWeather = WorldState->GetWeatherState();
            DryWeather.PrecipitationIntensity = 0.0f;
            DryWeather.WindStrength = 0.0f;
            WorldState->SetWeatherStateFromServer(DryWeather);

            const bool bFireReady = Fire->TryRefuelFromServer(Target) && Fire->TryLightFromServer(Target);
            const float StationHeat = Fire->GetWarmthContributionAt(Cauldron->GetActorLocation());
            const bool bStationReady = FVector::DistSquared(Target->GetActorLocation(), Cauldron->GetActorLocation())
                    <= FMath::Square(250.0f)
                && FVector::DistSquared(Fire->GetActorLocation(), Cauldron->GetActorLocation())
                    <= FMath::Square(250.0f)
                && FMath::IsFinite(StationHeat) && StationHeat > 0.0f
                && TargetCrafting->GetRecipeAvailability(SkillPrivacyPeerRecipe) == TEXT("Ready");
            if (!bFireReady || !bStationReady)
            {
                Cauldron->Destroy();
                Fire->Destroy();
                Fail(TEXT("real Hearth Broth recipe fixture is not ready at a lit Cauldron"));
                return;
            }

            SkillPrivacyPeerTarget = Target;
            SkillPrivacyPeerCauldron = Cauldron;
            SkillPrivacyPeerFire = Fire;
            TargetCrafting->PublishInteractionResultFromServer(SkillPrivacyPeerReady, true);
            SkillPrivacyPeerResultSerial = TargetCrafting->GetResultSerial();
            Target->ForceNetUpdate();
            Cauldron->ForceNetUpdate();
            Fire->ForceNetUpdate();
            SkillPrivacyPeerStage = 1;
            SkillPrivacyPeerElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("Skill privacy peer fixture: Ready=1 RemoteOwner=1 Recipe=HearthBrothRecipe Cauldron=1 Heat=1"));
            return;
        }

        AKalmalaCharacter* Target = SkillPrivacyPeerTarget.Get();
        UKalmalaInventoryComponent* TargetInventory = Target ? Target->GetInventoryComponent() : nullptr;
        UKalmalaCraftingComponent* TargetCrafting = Target
            ? Target->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
        UKalmalaSkillProgressionComponent* TargetProgression = Target
            ? Target->GetSkillProgressionComponent() : nullptr;
        if (!Target || !TargetInventory || !TargetCrafting || !TargetProgression
            || !SkillPrivacyPeerCauldron.IsValid() || !SkillPrivacyPeerFire.IsValid())
        {
            if (SkillPrivacyPeerElapsed >= 60.0f) Fail(TEXT("remote owner or cooking fixture disappeared"));
            return;
        }
        if (SkillPrivacyPeerStage == 1)
        {
            const uint32 ExpectedResultSerial = SkillPrivacyPeerResultSerial + SkillPrivacyPeerCraftCount;
            if (TargetCrafting->GetResultSerial() < ExpectedResultSerial)
            {
                if (SkillPrivacyPeerElapsed >= 90.0f) Fail(TEXT("owner did not complete ten accepted cooking RPCs"));
                return;
            }
            const bool bPaymentExact = TargetInventory->GetQuantity(TEXT("DeerMeat")) == SkillPrivacyPeerBaselineDeerMeat
                && TargetInventory->GetQuantity(TEXT("Onion")) == SkillPrivacyPeerBaselineOnion
                && TargetInventory->GetQuantity(TEXT("Wood")) == SkillPrivacyPeerBaselineWood;
            const bool bOutputExact = TargetInventory->GetQuantity(TEXT("HearthBroth"))
                == SkillPrivacyPeerBaselineHearthBroth + SkillPrivacyPeerCraftCount;
            const bool bResultsExact = TargetCrafting->GetResultSerial() == ExpectedResultSerial
                && TargetCrafting->WasLastResultAccepted()
                && TargetCrafting->GetLastResult() == TEXT("Crafted 1 Hearth Broth");
            if (!bPaymentExact || !bOutputExact || !bResultsExact
                || !HasExpectedServerLedger(TargetProgression->GetServerLedger(), 100)
                || !HasExpectedSkillPeerBadge(TargetProgression, 100))
            {
                Fail(TEXT("accepted crafts did not produce exact server payment, output, six-skill state, and derived badge"));
                return;
            }
            UE_LOG(LogTemp, Display,
                TEXT("Skill privacy peer server: Passed=1 AcceptedCrafts=10 ResultDelta=10 Payment=1 Output=10 Skills=6 CookingExperience=100 Level=2 Unlock=1"));
            SkillPrivacyPeerStage = 2;
        }
        return;
    }

    if (!Character->IsLocallyControlled() || SkillPrivacyPeerLocalStage >= 3) return;

    if (SkillPrivacyPeerLocalStage == 0 && GetLastResult() == SkillPrivacyPeerReady)
    {
        SkillPrivacyPeerLocalStage = 20;
        SkillPrivacyPeerLocalElapsed = 0.0f;
    }

    if (SkillPrivacyPeerLocalStage == 20)
    {
        if (!HasExpectedSkillStates(Progression->GetDetailedProgression(), 0)
            || !HasExpectedSkillPeerBadge(Progression, 0))
        {
            if (SkillPrivacyPeerLocalElapsed >= 30.0f)
                Fail(TEXT("owner did not receive the complete fresh detailed six-skill baseline"));
            return;
        }
        const bool bClientWriteAccepted = Progression->AwardExperienceFromAcceptedServerAction(
            EKalmalaSkill::Survival, FKalmalaSkillProgressionContract::MaxAwardPerAcceptedAction);
        if (bClientWriteAccepted || !HasExpectedSkillStates(Progression->GetDetailedProgression(), 0)
            || !HasExpectedSkillPeerBadge(Progression, 0))
        {
            Fail(TEXT("client-authored skill progression changed or was accepted"));
            return;
        }
        UE_LOG(LogTemp, Display,
            TEXT("Skill privacy peer owner write guard: Rejected=1 DetailedState=Unchanged Badge=Unchanged"));
        SkillPrivacyPeerLocalResultSerial = GetResultSerial();
        ServerCraft(SkillPrivacyPeerRecipe, 1);
        SkillPrivacyPeerLocalAcceptedCrafts = 0;
        SkillPrivacyPeerLocalStage = 1;
        SkillPrivacyPeerLocalElapsed = 0.0f;
        return;
    }

    if (SkillPrivacyPeerLocalStage == 1)
    {
        const uint32 CurrentResultSerial = GetResultSerial();
        if (CurrentResultSerial == SkillPrivacyPeerLocalResultSerial)
        {
            if (SkillPrivacyPeerLocalElapsed >= 30.0f) Fail(TEXT("owner cooking RPC timed out"));
            return;
        }
        if (CurrentResultSerial != SkillPrivacyPeerLocalResultSerial + 1
            || !WasLastResultAccepted() || GetLastResult() != TEXT("Crafted 1 Hearth Broth"))
        {
            Fail(TEXT("owner cooking RPC was rejected or returned an unexpected result"));
            return;
        }
        const int32 ExpectedCookingExperience =
            (SkillPrivacyPeerLocalAcceptedCrafts + 1) * SkillPrivacyPeerExperiencePerCraft;
        if (!HasExpectedSkillStates(Progression->GetDetailedProgression(), ExpectedCookingExperience)
            || !HasExpectedSkillPeerBadge(Progression, ExpectedCookingExperience))
        {
            if (SkillPrivacyPeerLocalElapsed >= 30.0f)
                Fail(TEXT("owner did not receive the exact detailed state and derived badge for an accepted craft"));
            return;
        }

        SkillPrivacyPeerLocalResultSerial = CurrentResultSerial;
        ++SkillPrivacyPeerLocalAcceptedCrafts;
        if (SkillPrivacyPeerLocalAcceptedCrafts == SkillPrivacyPeerCraftCount)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Skill privacy peer owner: Passed=1 Authority=0 AcceptedCrafts=10 Skills=6 CookingExperience=100 Level=2 Unlock=1 Badge=2:1 ClientWriteRejected=1"));
            SkillPrivacyPeerLocalStage = 3;
            return;
        }
        SkillPrivacyPeerLocalStage = 2;
        SkillPrivacyPeerLocalElapsed = 0.0f;
    }

    if (SkillPrivacyPeerLocalStage == 2 && SkillPrivacyPeerLocalElapsed >= 0.35f)
    {
        SkillPrivacyPeerLocalResultSerial = GetResultSerial();
        ServerCraft(SkillPrivacyPeerRecipe, 1);
        SkillPrivacyPeerLocalStage = 1;
        SkillPrivacyPeerLocalElapsed = 0.0f;
        return;
    }

    if (SkillPrivacyPeerLocalStage == 0)
    {
        int32 OwnerPlayerId = INDEX_NONE;
        if (FindSkillPrivacyPeerCauldron(GetWorld(), OwnerPlayerId))
        {
            if (OwnerPlayerId == Character->GetPlayerState()->GetPlayerId()) return;
            SkillPrivacyPeerObservedOwnerPlayerId = OwnerPlayerId;
            SkillPrivacyPeerLocalStage = 10;
            SkillPrivacyPeerLocalElapsed = 0.0f;
        }
        else if (SkillPrivacyPeerLocalElapsed >= 120.0f)
        {
            Fail(TEXT("observer did not discover the skill privacy peer fixture"));
            return;
        }
    }

    if (SkillPrivacyPeerLocalStage == 10 || SkillPrivacyPeerLocalStage == 11)
    {
        AKalmalaCharacter* Owner = FindSkillPrivacyPeerCharacter(GetWorld(), SkillPrivacyPeerObservedOwnerPlayerId);
        UKalmalaSkillProgressionComponent* OwnerProgression = Owner
            ? Owner->GetSkillProgressionComponent() : nullptr;
        if (!Owner || !OwnerProgression)
        {
            if (SkillPrivacyPeerLocalElapsed >= 120.0f) Fail(TEXT("observer could not resolve the remote skill owner"));
            return;
        }
        const FKalmalaSkillPeerPresentationState& Badge = OwnerProgression->GetPeerPresentation();
        if (!OwnerProgression->GetDetailedProgression().IsEmpty())
        {
            Fail(TEXT("observer received the remote owner's detailed six-skill ledger"));
            return;
        }
        if (!Badge.IsValid() || Badge.HighestLevel > 2 || (Badge.HighestLevel == 2
            && Badge.HighestUnlockMask != FKalmalaSkillProgressionContract::GetUnlockMaskForLevel(2)))
        {
            Fail(TEXT("observer received an invalid or client-authored aggregate skill badge"));
            return;
        }
        if (SkillPrivacyPeerLocalStage == 10 && Badge.HighestLevel == 2
            && Badge.HighestUnlockMask == FKalmalaSkillProgressionContract::GetUnlockMaskForLevel(2))
        {
            SkillPrivacyPeerLocalStage = 11;
            SkillPrivacyPeerLocalElapsed = 0.0f;
        }
        if (SkillPrivacyPeerLocalStage == 11 && SkillPrivacyPeerLocalElapsed >= 1.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Skill privacy peer observer: Passed=1 Authority=0 TargetDetailed=0 Badge=2:1"));
            SkillPrivacyPeerLocalStage = 12;
            return;
        }
        if (SkillPrivacyPeerLocalElapsed >= 120.0f)
            Fail(TEXT("observer did not receive the expected derived level-two badge"));
    }
}

#endif // !UE_BUILD_SHIPPING
