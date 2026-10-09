#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaRawFuelContract.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaSkillProgressionComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EngineUtils.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaShelterSampler.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaHarvestNode.h"

void UKalmalaCraftingComponent::RunVerification(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    auto* C = GetCharacter(); if (!C || !C->GetPlayerState()) return;
    auto* I = C->FindComponentByClass<UKalmalaInventoryComponent>(); if (!I) return;
    VerificationElapsed += DeltaTime;
    if (C->HasAuthority() && VerificationStage == 0 && VerificationElapsed > 3)
    {
        int32 Players=0;
        for(TActorIterator<AKalmalaCharacter> It(GetWorld());It;++It) if(It->GetPlayerState()) ++Players;
        if(Players<2) return;
        auto Check = [&](bool Passed, const TCHAR* Label) {
            bVerificationPassed &= Passed;
            if (!Passed) UE_LOG(LogTemp, Error, TEXT("Crafting fixture FAILED: %s"), Label);
        };
        FString Reason;
        FActorSpawnParameters RepairStoneSpawn;
        RepairStoneSpawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaConstructionActor* RepairStone = GetWorld()->SpawnActor<AKalmalaConstructionActor>(
            AKalmalaConstructionActor::StaticClass(), FTransform::Identity, RepairStoneSpawn);
        bool bRepairStoneVisible = false;
        bool bAcceptedOnce = false;
        bool bNoExtraMutation = false;
        bool bActionOnlyFeedback = false;
        bool bNoMenu = false;
        if (RepairStone != nullptr && C->GetController() != nullptr)
        {
            RepairStone->InitializeFromServer(TEXT("GrindingStoneKit"), FString::Printf(
                TEXT("m12-repair-all-%d"), C->GetPlayerState()->GetPlayerId()));
            const FRotator PreviousViewRotation = C->GetController()->GetControlRotation();
            const FRotator PreviousActorRotation = C->GetActorRotation();
            for (int32 Turn = 0; Turn < 16 && !bRepairStoneVisible; ++Turn)
            {
                const FRotator TestViewRotation(0.0f, Turn * 22.5f, 0.0f);
                C->GetController()->SetControlRotation(TestViewRotation);
                C->SetActorRotation(TestViewRotation);
                FVector ViewLocation;
                FRotator ViewRotation;
                C->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
                RepairStone->SetActorLocation(ViewLocation + ViewRotation.Vector() * 160.0f);
                bRepairStoneVisible = RepairStone->CanInteract_Implementation(C);
            }

            const TArray<FKalmalaToolState> ToolsBefore = C->GetCarriedToolInventory();
            const TArray<FKalmalaInventoryStack> PackBefore = I->GetStacks();
            const uint32 ResultSerialBefore = GetResultSerial();
            AKalmalaConstructionActor* ContextActorBefore = GetLastStationContextActor();
            const FName ContextKitBefore = GetLastStationContextKit();
            const FString ContextIdBefore = GetLastStationContextConstructionId();
            const uint32 ContextSerialBefore = GetStationContextInteractionSerial();
            const FName CookingKitBefore = GetLastInteractedCookingStationKit();
            const uint32 CookingSerialBefore = GetCookingStationInteractionSerial();
            const bool bHadStorageBefore = HasStorageView();
            const double NextRequestTimeBefore = NextRequestTime;

            NextRequestTime = 0.0;
            if (bRepairStoneVisible) C->ServerRequestInteract(NAME_None, 0);
            NextRequestTime = NextRequestTimeBefore;

            const uint32 ExpectedResultSerial = ResultSerialBefore == TNumericLimits<uint32>::Max()
                ? 1 : ResultSerialBefore + 1;
            bAcceptedOnce = bRepairStoneVisible && GetResultSerial() == ExpectedResultSerial
                && WasLastResultAccepted();
            bool bToolsUnchanged = C->GetCarriedToolInventory().Num() == ToolsBefore.Num();
            for (int32 Index = 0; bToolsUnchanged && Index < ToolsBefore.Num(); ++Index)
            {
                const FKalmalaToolState& Before = ToolsBefore[Index];
                const FKalmalaToolState& After = C->GetCarriedToolInventory()[Index];
                bToolsUnchanged = Before.ToolId == After.ToolId && Before.ToolLevel == After.ToolLevel
                    && Before.Durability == After.Durability;
            }
            bool bPackUnchanged = I->GetStacks().Num() == PackBefore.Num();
            for (int32 Index = 0; bPackUnchanged && Index < PackBefore.Num(); ++Index)
            {
                bPackUnchanged = I->GetStacks()[Index].ItemId == PackBefore[Index].ItemId
                    && I->GetStacks()[Index].Quantity == PackBefore[Index].Quantity;
            }
            bNoExtraMutation = bToolsUnchanged && bPackUnchanged;
            bActionOnlyFeedback = GetLastResult() == TEXT("All carried tools are already at full condition");
            bNoMenu = GetStationContextInteractionSerial() == ContextSerialBefore
                && GetLastStationContextActor() == ContextActorBefore
                && GetLastStationContextKit() == ContextKitBefore
                && GetLastStationContextConstructionId() == ContextIdBefore
                && GetLastInteractedCookingStationKit() == CookingKitBefore
                && GetCookingStationInteractionSerial() == CookingSerialBefore
                && HasStorageView() == bHadStorageBefore;
            C->SetActorRotation(PreviousActorRotation);
            C->GetController()->SetControlRotation(PreviousViewRotation);
        }
        if (RepairStone != nullptr) RepairStone->Destroy();
        Check(bAcceptedOnce && bNoExtraMutation && bActionOnlyFeedback && bNoMenu,
            TEXT("Grinding Stone Interact accepts one Repair All without extra mutation or menu"));
        UE_LOG(LogTemp, Display, TEXT("Grinding Stone interaction: AcceptedOnce=%d NoExtraMutation=%d ActionOnlyFeedback=%d NoMenu=%d"),
            bAcceptedOnce ? 1 : 0, bNoExtraMutation ? 1 : 0, bActionOnlyFeedback ? 1 : 0, bNoMenu ? 1 : 0);

        Check(!PlaceFromServer(Reason), TEXT("No ingredients cannot create a hearth"));
        Check(!CraftFromServer(TEXT("Campfire"),1,Reason), TEXT("Campfire cannot be crafted into an inventory item"));
        Check(!CraftFromServer(TEXT("Forged"),1,Reason), TEXT("Unknown recipe"));
        for (int32 Batch : {MIN_int32,-1,0,MAX_int32}) Check(!CraftFromServer(TEXT("Workbench"),Batch,Reason),TEXT("Malformed batch"));
        Check(I->TryGrantFromServer(TEXT("Wood"),50) && I->TryGrantFromServer(TEXT("Stone"),40)
            && I->TryGrantFromServer(TEXT("Fibre"),50),TEXT("Seed bounded test materials"));
        const FVector StationProbeOrigin=C->GetActorLocation();
        C->SetActorLocation(StationProbeOrigin+FVector(0,0,10000));
        Check(!CraftFromServer(TEXT("Floor"),1,Reason),TEXT("Missing station rejects craft"));
        C->SetActorLocation(StationProbeOrigin);
        auto* Recipes=GetMutableDefault<UKalmalaRecipeCatalogue>();
        auto& FuelRecipe=Recipes->Recipes[0]; const bool Enabled=FuelRecipe.bEnabled; FuelRecipe.bEnabled=false;
        Check(!CraftFromServer(FuelRecipe.RecipeId,1,Reason),TEXT("Disabled recipe rejects craft")); FuelRecipe.bEnabled=Enabled;
        Check(I->TryGrantFromServer(TEXT("WorkbenchKit"),5),TEXT("Fill output stack"));
        const int32 WoodBefore=I->GetQuantity(TEXT("Wood"));
        Check(!CraftFromServer(TEXT("Workbench"),1,Reason) && I->GetQuantity(TEXT("Wood"))==WoodBefore,TEXT("Full output cannot consume inputs"));
        Check(!CraftFromServer(TEXT("Campfire"),1,Reason),TEXT("Normal crafting cannot create a Campfire inventory item"));
        Check(GetRecipeAvailability(TEXT("Campfire")) == TEXT("Ready"),TEXT("Campfire direct build checks recipe materials and raw fuel"));
        const int32 HearthWoodBefore = I->GetQuantity(TEXT("Wood"));
        const int32 HearthStoneBefore = I->GetQuantity(TEXT("Stone"));

        FVector Original=C->GetActorLocation(); const FRotator OriginalRotation=C->GetActorRotation();
        TSet<AKalmalaCampfire*> Existing;
        for(TActorIterator<AKalmalaCampfire> It(GetWorld());It;++It) Existing.Add(*It);
        bool Placed=false;
        for(int32 Turn=0; Turn<8 && !Placed; ++Turn)
        {
            C->SetActorRotation(FRotator(0,Turn*45,0));
            Placed=PlaceFromServer(Reason);
        }
        // A restored camp can occupy every direction around the shared player start.
        // Search bounded nearby terrain for this test's new paid camp; never remove
        // restored actors or relax the production placement checks to make room.
        if (!Placed)
        {
            UE_LOG(LogTemp, Display, TEXT("Crafting fixture seeking clear ground: Player=%d Origin=%s Reason=%s"),
                C->GetPlayerState()->GetPlayerId(), *Original.ToCompactString(), *Reason);
            const FVector SearchOrigin = Original;
            FCollisionQueryParams Query(SCENE_QUERY_STAT(CraftingFixtureGround), false, C);
            for (int32 Site = 0; Site < 24 && !Placed; ++Site)
            {
                const FVector Probe = SearchOrigin + FRotator(0, (Site % 8) * 45, 0).Vector() * (600 + (Site / 8) * 600);
                FHitResult Ground;
                if (!GetWorld()->LineTraceSingleByChannel(Ground, Probe + FVector(0,0,1000), Probe - FVector(0,0,2000), ECC_Visibility, Query)
                    || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor())) continue;
                C->SetActorLocation(Ground.ImpactPoint + FVector(0,0,C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2));
                for (int32 Turn = 0; Turn < 8 && !Placed; ++Turn)
                {
                    C->SetActorRotation(FRotator(0, Turn * 45, 0));
                    Placed = PlaceFromServer(Reason);
                }
            }
            if (Placed)
            {
                Original = C->GetActorLocation();
                C->GetCharacterMovement()->StopMovementImmediately();
                UE_LOG(LogTemp, Display, TEXT("Crafting fixture found clear ground: Player=%d Location=%s"),
                    C->GetPlayerState()->GetPlayerId(), *Original.ToCompactString());
            }
            else C->SetActorLocation(SearchOrigin);
        }
        Check(Placed,TEXT("Paid placement on actual generated collision"));
        Check(Reason.StartsWith(TEXT("Campfire placed")), TEXT("Placed result is named Campfire"));
        for(TActorIterator<AKalmalaCampfire> It(GetWorld());It;++It) if(!Existing.Contains(*It)) VerificationFire=*It;
        if (!VerificationFire) { VerificationStage=99; return; }
        auto* Fire=VerificationFire.Get(); Fire->SetActorTickEnabled(false);
        const auto AreStacksEqual = [](const TArray<FKalmalaInventoryStack>& Left, const TArray<FKalmalaInventoryStack>& Right)
        {
            if (Left.Num() != Right.Num()) return false;
            for (int32 Index = 0; Index < Left.Num(); ++Index)
                if (Left[Index].ItemId != Right[Index].ItemId || Left[Index].Quantity != Right[Index].Quantity) return false;
            return true;
        };
        const auto InteractWithFire = [C, Fire](const bool bInRange)
        {
            if (!C->GetController()) return false;
            const FRotator PreviousViewRotation = C->GetController()->GetControlRotation();
            const FRotator PreviousActorRotation = C->GetActorRotation();
            const FTransform PreviousFireTransform = Fire->GetActorTransform();
            const float Distance = bInRange ? 160.0f : 350.0f;
            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CraftingFireInteractFixture), false, C);
            const auto UpdateFixtureViewpoint = [C]()
            {
                if (APlayerController* PlayerController = Cast<APlayerController>(C->GetController());
                    PlayerController != nullptr && PlayerController->PlayerCameraManager != nullptr)
                {
                    FMinimalViewInfo ViewInfo = PlayerController->PlayerCameraManager->GetCameraCacheView();
                    ViewInfo.Location = C->GetPawnViewLocation();
                    ViewInfo.Rotation = PlayerController->GetControlRotation();
                    PlayerController->PlayerCameraManager->SetCameraCachePOV(ViewInfo);
                    PlayerController->PlayerCameraManager->SetLastFrameCameraCachePOV(ViewInfo);
                }
            };
            for (int32 Turn = 0; Turn < 16; ++Turn)
            {
                const FRotator Facing(0.0f, PreviousViewRotation.Yaw + Turn * 22.5f, 0.0f);
                const FVector TestLocation = C->GetActorLocation() + Facing.Vector() * Distance
                    + FVector(0.0f, 0.0f, bInRange ? 80.0f : 24.0f);
                Fire->SetActorLocation(TestLocation);
                UpdateFixtureViewpoint();
                FVector ViewLocation;
                FRotator ViewRotation;
                C->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
                const FRotator AimRotation = (TestLocation - ViewLocation).Rotation();
                C->SetActorRotation(FRotator(0.0f, AimRotation.Yaw, 0.0f));
                C->GetController()->SetControlRotation(AimRotation);
                UpdateFixtureViewpoint();
                C->GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
                FHitResult Hit;
                const bool bHasHit = C->GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation,
                    ViewLocation + ViewRotation.Vector() * C->GetInteractionRange(), ECC_Visibility, QueryParams);
                if ((bInRange && (!bHasHit || Hit.GetActor() != Fire))
                    || (!bInRange && bHasHit && Hit.GetActor() != Fire))
                {
                    continue;
                }
                C->ServerRequestInteract(NAME_None, 0);
                Fire->SetActorTransform(PreviousFireTransform);
                C->SetActorRotation(PreviousActorRotation);
                C->GetController()->SetControlRotation(PreviousViewRotation);
                return true;
            }
            Fire->SetActorTransform(PreviousFireTransform);
            C->SetActorRotation(PreviousActorRotation);
            C->GetController()->SetControlRotation(PreviousViewRotation);
            return false;
        };
        Check(I->GetQuantity(TEXT("CampfireKit"))==0 && I->GetQuantity(TEXT("WorkbenchKit"))==5
            && I->GetQuantity(TEXT("Wood"))==HearthWoodBefore-4 && I->GetQuantity(TEXT("Stone"))==HearthStoneBefore-5,
            TEXT("Placement charges raw hearth materials and one raw-fuel item exactly once without creating a kit"));
        const int32 WoodBeforeOverlap = I->GetQuantity(TEXT("Wood"));
        const int32 StoneBeforeOverlap = I->GetQuantity(TEXT("Stone"));
        const int32 FuelBeforeOverlap = I->GetQuantity(TEXT("WorkbenchKit"));
        Check(!PlaceFromServer(Reason) && I->GetQuantity(TEXT("CampfireKit"))==0
            && I->GetQuantity(TEXT("WorkbenchKit"))==FuelBeforeOverlap && I->GetQuantity(TEXT("Wood"))==WoodBeforeOverlap
            && I->GetQuantity(TEXT("Stone"))==StoneBeforeOverlap,TEXT("Overlap rejects placement without raw-material payment"));
        Check(!CraftFromServer(TEXT("Floor"),1,Reason) && GetRecipeAvailability(TEXT("Floor")) == TEXT("Ready"),
            TEXT("Floor is built directly from materials and no longer crafts into a kit"));
        Fire->SetOwner(nullptr); Fire->SetSharedFromServer(false);
        // Move away from any other player's eligible shared hearth while checking this lock.
        const FVector FireOrigin=Fire->GetActorLocation();
        C->SetActorLocation(Original+FVector(0,0,1000)); Fire->SetActorLocation(C->GetActorLocation()+FVector(100,0,0));
        Check(!CraftFromServer(TEXT("Floor"),1,Reason),TEXT("Direct build materials cannot be converted into a kit by RPC"));
        Fire->SetOwner(C->GetController()); Fire->SetSharedFromServer(true);
        Fire->SetActorLocation(FireOrigin);
        Check(!CraftFromServer(TEXT("Floor"),1,Reason) && !Fire->TryRefuelFromServer(C),TEXT("Distant station and refuel"));
        C->SetActorLocation(Original); C->SetActorRotation(OriginalRotation);
        bool ConstructionPlaced = false;
        TSet<AKalmalaConstructionActor*> ExistingConstruction;
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It) ExistingConstruction.Add(*It);
        for (int32 Turn = 0; Turn < 8 && !ConstructionPlaced; ++Turn)
        {
            C->SetActorRotation(FRotator(0, Turn * 45, 0));
            ConstructionPlaced = PlaceConstructionFromServer(TEXT("FloorKit"), Reason);
        }
        const FVector ConstructionSearchOrigin = C->GetActorLocation();
        if (!ConstructionPlaced)
        {
            UE_LOG(LogTemp, Display, TEXT("Crafting fixture seeking clear construction ground: Player=%d Origin=%s Reason=%s"),
                C->GetPlayerState()->GetPlayerId(), *ConstructionSearchOrigin.ToCompactString(), *Reason);
            FCollisionQueryParams Query(SCENE_QUERY_STAT(CraftingFixtureConstructionGround), false, C);
            for (int32 Site = 0; Site < 24 && !ConstructionPlaced; ++Site)
            {
                const FVector Probe = ConstructionSearchOrigin + FRotator(0, (Site % 8) * 45, 0).Vector() * (600 + (Site / 8) * 600);
                FHitResult Ground;
                if (!GetWorld()->LineTraceSingleByChannel(Ground, Probe + FVector(0,0,1000), Probe - FVector(0,0,2000), ECC_Visibility, Query)
                    || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor())) continue;
                C->SetActorLocation(Ground.ImpactPoint + FVector(0,0,C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2));
                for (int32 Turn = 0; Turn < 8 && !ConstructionPlaced; ++Turn)
                {
                    C->SetActorRotation(FRotator(0, Turn * 45, 0));
                    ConstructionPlaced = PlaceConstructionFromServer(TEXT("FloorKit"), Reason);
                }
            }
            if (ConstructionPlaced)
            {
                C->GetCharacterMovement()->StopMovementImmediately();
                UE_LOG(LogTemp, Display, TEXT("Crafting fixture found clear construction ground: Player=%d Location=%s"),
                    C->GetPlayerState()->GetPlayerId(), *C->GetActorLocation().ToCompactString());
            }
            C->SetActorLocation(ConstructionSearchOrigin);
        }
        if (APlayerController* PlayerController = Cast<APlayerController>(C->GetController());
            PlayerController != nullptr)
        {
            PlayerController->SetViewTarget(C);
            if (PlayerController->PlayerCameraManager != nullptr)
            {
                FMinimalViewInfo ViewInfo = PlayerController->PlayerCameraManager->GetCameraCacheView();
                ViewInfo.Location = C->GetPawnViewLocation();
                ViewInfo.Rotation = PlayerController->GetControlRotation();
                PlayerController->PlayerCameraManager->SetCameraCachePOV(ViewInfo);
                PlayerController->PlayerCameraManager->SetLastFrameCameraCachePOV(ViewInfo);
            }
        }
        Check(ConstructionPlaced, TEXT("Server construction placement ignores local preview and pays once"));
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It) if (!ExistingConstruction.Contains(*It))
        {
            It->AdvanceRainWearFromServer(1000.0f, 1.0f);
            Check(It->GetHealth() == AKalmalaConstructionActor::RainHealthFloor, TEXT("Construction feedback rain-wear floor"));
            UE_LOG(LogTemp, Display, TEXT("Construction accepted: Id=%s Kit=%s"), *It->GetConstructionId(), *It->GetConstructionKit().ToString());
        }
        C->SetActorRotation(OriginalRotation);
        for(int32 N=0; N<4; ++N) Check(Fire->TryRefuelFromServer(C),TEXT("Bounded refuel"));
        const TArray<FKalmalaInventoryStack> PackBeforeFullInteract = I->GetStacks();
        const float FuelBeforeFullInteract = Fire->GetFuelSeconds();
        const bool bFullInteractRejected = !Fire->CanInteract_Implementation(C) && InteractWithFire(true)
            && !Fire->TryRefuelFromServer(C) && Fire->GetFuelSeconds() == FuelBeforeFullInteract
            && AreStacksEqual(PackBeforeFullInteract, I->GetStacks());
        Check(bFullInteractRejected, TEXT("Full hearth rejects direct Interact without consuming fuel"));
        Fire->TryLightFromServer(C); Fire->AdvanceFromServer(300,0,0);
        Check(Fire->GetFuelSeconds()==0 && !Fire->IsLit() && Fire->GetEffectiveWarmth()==0,TEXT("Fuel exhaustion extinguishes warmth"));
        const TArray<FKalmalaInventoryStack> PackBeforeNoFuelInteract = I->GetStacks();
        bool bRemovedAllRawFuel = true;
        for (const FName FuelItemId : FKalmalaRawFuelContract::GetFuelItemIds())
        {
            const int32 Quantity = I->GetQuantity(FuelItemId);
            if (Quantity > 0) bRemovedAllRawFuel = I->TryConsumeFromServer(FuelItemId, Quantity) && bRemovedAllRawFuel;
        }
        const TArray<FKalmalaInventoryStack> EmptyFuelPack = I->GetStacks();
        const float FuelBeforeNoFuelInteract = Fire->GetFuelSeconds();
        const bool bNoFuelInteractRejected = bRemovedAllRawFuel && !Fire->CanInteract_Implementation(C)
            && InteractWithFire(true) && Fire->GetFuelSeconds() == FuelBeforeNoFuelInteract
            && AreStacksEqual(EmptyFuelPack, I->GetStacks());
        Check(bNoFuelInteractRejected, TEXT("No raw fuel rejects direct Interact without changing fuel or pack"));
        const bool bRawFuelRestored = I->TryCommitStacksFromServer(EmptyFuelPack, PackBeforeNoFuelInteract);
        Check(bRawFuelRestored, TEXT("Restore raw fuel after no-fuel interaction rejection"));

        const TArray<FKalmalaInventoryStack> PackBeforeRangeInteract = I->GetStacks();
        const float FuelBeforeRangeInteract = Fire->GetFuelSeconds();
        const bool bRangeInteractRejected = InteractWithFire(false) && Fire->GetFuelSeconds() == FuelBeforeRangeInteract
            && AreStacksEqual(PackBeforeRangeInteract, I->GetStacks());
        Check(bRangeInteractRejected, TEXT("Out-of-range direct Interact cannot refuel"));

        const TArray<FKalmalaInventoryStack> PackBeforeDirectRefuel = I->GetStacks();
        TArray<FKalmalaInventoryStack> ExpectedFuelCost;
        FString FuelReason;
        const bool bExpectedFuelSelection = FKalmalaRawFuelContract::AddCosts(PackBeforeDirectRefuel, 1, ExpectedFuelCost, FuelReason);
        TArray<FKalmalaInventoryStack> ExpectedPackAfterRefuel;
        const bool bExpectedFuelExchange = bExpectedFuelSelection
            && UKalmalaInventoryComponent::BuildExchange(PackBeforeDirectRefuel, ExpectedFuelCost, NAME_None, 0,
                ExpectedPackAfterRefuel, FuelReason);
        const float FuelBeforeDirectRefuel = Fire->GetFuelSeconds();
        const bool bDirectRefuelAccepted = InteractWithFire(true) && bExpectedFuelExchange
            && AreStacksEqual(ExpectedPackAfterRefuel, I->GetStacks())
            && FMath::IsNearlyEqual(Fire->GetFuelSeconds(), FuelBeforeDirectRefuel + AKalmalaCampfire::FuelSecondsPerItem);
        const bool bDirectRefuelDidNotLight = Fire->GetHearthState() == EKalmalaHearthState::Extinguished;
        Check(bDirectRefuelAccepted && bDirectRefuelDidNotLight,
            TEXT("Campfire Interact consumes exactly one priority raw fuel item without lighting"));
        UE_LOG(LogTemp, Display, TEXT("Campfire interaction: AddedOne=%d NoLighting=%d FullRejected=%d NoFuelRejected=%d RangeRejected=%d"),
            bDirectRefuelAccepted ? 1 : 0, bDirectRefuelDidNotLight ? 1 : 0, bFullInteractRejected ? 1 : 0,
            bNoFuelInteractRejected ? 1 : 0, bRangeInteractRejected ? 1 : 0);
        // Real server shelter traces must shield the fire; no authored protection volume.
        auto* WorldState=GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
        const auto SavedWeather=WorldState->GetWeatherState(); auto Storm=SavedWeather;
        Storm.PrecipitationIntensity=1; Storm.WindStrength=1; Storm.WindDirectionDegrees=0;
        WorldState->SetWeatherStateFromServer(Storm);
        auto MakeShelter=[&](FVector Offset,FVector Extent,const TCHAR* Tag) {
            auto* Actor=GetWorld()->SpawnActor<AActor>();
            auto* Box=NewObject<UBoxComponent>(Actor); Actor->SetRootComponent(Box); Actor->AddInstanceComponent(Box);
            Box->SetBoxExtent(Extent); Box->SetCollisionProfileName(TEXT("BlockAll")); Box->RegisterComponent();
            Actor->SetActorLocation(Fire->GetActorLocation()+Offset); Actor->Tags.Add(FName(Tag)); return Actor;
        };
        auto* Roof=MakeShelter(FVector(0,0,200),FVector(120,120,10),TEXT("KalmalaShelterRoof"));
        auto* Wall=MakeShelter(FVector(-200,0,60),FVector(10,120,180),TEXT("KalmalaShelterWindbreak"));
        Fire->TryLightFromServer(C); Fire->Tick(1);
        Check(Fire->HasRoof() && Fire->HasWindbreak() && Fire->IsLit() && Fire->GetFuelWetness()==0
            && Fire->GetEffectiveWarmth()==1,TEXT("Server roof and windbreak traces protect fire"));
        Roof->Destroy(); Wall->Destroy(); WorldState->SetWeatherStateFromServer(SavedWeather);
        Fire->AdvanceFromServer(300,0,0); Check(Fire->TryRefuelFromServer(C),TEXT("Prepare rain fixture"));
        Fire->TryLightFromServer(C); Fire->AdvanceFromServer(12,1,1);
        Check(Fire->GetHearthState()==EKalmalaHearthState::Smouldering && Fire->GetEffectiveWarmth()==0,TEXT("Exposed rain smoulders"));
        Check(!Fire->TryLightFromServer(C),TEXT("Wet lighting rejected"));
        const float WetBefore=Fire->GetFuelWetness(); Check(Fire->TryRefuelFromServer(C) && Fire->GetFuelWetness()==WetBefore,TEXT("Refuel preserves wetness"));
        Fire->AdvanceFromServer(100,0,0); Fire->TryLightFromServer(C); Fire->AdvanceFromServer(300,0,0);
        Check(Fire->TryRefuelFromServer(C),TEXT("Prepare replicated fire"));
        Fire->TryLightFromServer(C); Fire->AdvanceFromServer(0,0,0);
        Check(Fire->IsLit() && Fire->GetFuelSeconds()==60 && Fire->GetEffectiveWarmth()==1,TEXT("Dry replicated fixture"));
        const auto Stacks=I->GetStacks(); for(const auto& Stack:Stacks) I->TryConsumeFromServer(Stack.ItemId,Stack.Quantity);
        Check(I->TryGrantFromServer(TEXT("Wood"),18) && I->TryGrantFromServer(TEXT("Fibre"),12) && I->TryGrantFromServer(TEXT("Stone"),4),TEXT("Seed real raw-material RPC transactions"));
        UE_LOG(LogTemp,Display,TEXT("Crafting server gates: Passed=%d Player=%d Placement=1 Atomic=1 Malformed=1 Locked=1 Distant=1 Fuel=1 Rain=1"),bVerificationPassed,C->GetPlayerState()->GetPlayerId());
        UE_LOG(LogTemp,Display,TEXT("Crafting fire server: Name=%s Fuel=60 Lit=1 Wet=0 Warmth=1 State=1"),*Fire->GetName());
        PublishResult(TEXT("Verification ready"), true); VerificationStage=1; VerificationElapsed=0;
    }
    if (C->HasAuthority() && VerificationStage==1 && VerificationElapsed>8)
    {
        if(VerificationFire) {
            VerificationFire->AdvanceFromServer(12,1,1);
            UE_LOG(LogTemp,Display,TEXT("Crafting fire server: Name=%s Fuel=48 Lit=0 Wet=96 Warmth=0 State=2"),*VerificationFire->GetName());
        }
        VerificationStage=2;
    }
    if (C->HasAuthority() && VerificationStage==2 && VerificationElapsed>12)
    {
        const bool Passed=I->GetQuantity(TEXT("WorkbenchKit"))==2 && I->GetQuantity(TEXT("Wood"))==0 && I->GetQuantity(TEXT("Fibre"))==0 && I->GetQuantity(TEXT("Stone"))==0 && I->GetStacks().Num()==1;
        UE_LOG(LogTemp,Display,TEXT("Crafting server final: Passed=%d Player=%d WorkbenchKit=%d Slots=%d"),Passed,C->GetPlayerState()->GetPlayerId(),I->GetQuantity(TEXT("WorkbenchKit")),I->GetStacks().Num());
        VerificationStage=3;
    }
    if (!C->IsLocallyControlled()) return;
    LocalVerificationElapsed+=DeltaTime;
    if(LocalVerificationStage==0 && LastResult==TEXT("Verification ready") && I->GetQuantity(TEXT("Wood"))==18)
    {
        ServerCraft(TEXT("Workbench"),1); ServerCraft(TEXT("Workbench"),1); // immediate duplicate is rate-limited
        LocalVerificationStage=1; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==1 && LocalVerificationElapsed>2 && I->GetQuantity(TEXT("WorkbenchKit"))==1)
    {
        ServerCraft(TEXT("Workbench"),1); LocalVerificationStage=2; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==2 && LocalVerificationElapsed>2 && I->GetQuantity(TEXT("WorkbenchKit"))==2)
    {
        ServerCraft(TEXT("Workbench"),1); LocalVerificationStage=3; LocalVerificationElapsed=0; // insufficient
    }
    else if(LocalVerificationStage==3 && LocalVerificationElapsed>2)
    {
        ServerCraft(TEXT("Forged"),1); LocalVerificationStage=4; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==4 && LocalVerificationElapsed>1)
    {
        ServerCraft(TEXT("Workbench"),MAX_int32); LocalVerificationStage=5; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==5 && LocalVerificationElapsed>1)
    {
        ServerPlaceCampfire(); LocalVerificationStage=6; LocalVerificationElapsed=0;
    }
    else if(LocalVerificationStage==6 && LocalVerificationElapsed>1)
    {
        FString Reason;
        bool Rejected=C->HasAuthority() || (!CraftFromServer(TEXT("Workbench"),1,Reason) && !PlaceFromServer(Reason));
        if(!C->HasAuthority()) if(auto* Fire=FindNearbyFire(false))
        {
            const float FuelBefore=Fire->GetFuelSeconds(); Fire->AdvanceFromServer(1000,1,1);
            Rejected &= Fire->GetFuelSeconds()==FuelBefore && !Fire->TryRefuelFromServer(C) && !Fire->CanInteract_Implementation(C);
        }
        const bool Passed=Rejected && I->GetQuantity(TEXT("WorkbenchKit"))==2 && I->GetStacks().Num()==1;
        UE_LOG(LogTemp,Display,TEXT("Crafting owner final: Passed=%d Authority=%d WorkbenchKit=%d Slots=%d"),Passed,C->HasAuthority(),I->GetQuantity(TEXT("WorkbenchKit")),I->GetStacks().Num());
        LocalVerificationStage = 10;
    }
#endif
}

namespace
{
    bool GatherPersistedCampMaterials(AKalmalaCharacter* Character, UKalmalaInventoryComponent* Inventory, const TMap<FName, int32>& Required)
    {
        for (uint64 SpawnSeed = 1; SpawnSeed <= 2048; ++SpawnSeed)
        {
            bool bComplete = true;
            for (const TPair<FName, int32>& Entry : Required) bComplete &= Inventory->GetQuantity(Entry.Key) >= Entry.Value;
            if (bComplete) return true;
            FActorSpawnParameters Parameters;
            Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaHarvestNode* Node = Character->GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Character->GetActorLocation() + FVector(80, 0, 0), FRotator::ZeroRotator, Parameters);
            if (Node == nullptr) return false;
            FKalmalaWorldPopulationSpawn Spawn;
            Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
            Spawn.SpawnSeed = SpawnSeed;
            Spawn.Location = Node->GetActorLocation();
            Node->InitializeServer(Spawn);
            const FName ItemId = Node->GetHarvestItemId();
            const int32* Target = Required.Find(ItemId);
            if (Target == nullptr || Inventory->GetQuantity(ItemId) >= *Target)
            {
                Node->Destroy();
                continue;
            }
            const int32 Before = Inventory->GetQuantity(ItemId);
            Node->Interact_Implementation(Character);
            const bool bAccepted = Inventory->GetQuantity(ItemId) == Before + 1;
            Node->Destroy();
            if (!bAccepted) return false;
        }
        return false;
    }

    bool PlacePersistedCampfireNearTerrain(UKalmalaCraftingComponent* Crafting, AKalmalaCharacter* Character, FString& OutReason)
    {
        const FVector Original = Character->GetActorLocation(); const FRotator OriginalRotation = Character->GetActorRotation(); bool bPlaced = false;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(PersistedCampFixtureGround), false, Character);
        for (int32 Site = 0; Site < 24 && !bPlaced; ++Site)
        {
            const FVector Probe = Original + FRotator(0, float(Site % 8) * 45, 0).Vector() * (600 + float(Site / 8) * 600);
            FHitResult Ground;
            if (!Character->GetWorld()->LineTraceSingleByChannel(Ground, Probe + FVector(0, 0, 1000), Probe - FVector(0, 0, 2000), ECC_Visibility, Query) || !Cast<AKalmalaGeneratedTerrainPatch>(Ground.GetActor())) continue;
            Character->SetActorLocation(Ground.ImpactPoint + FVector(0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2));
            for (int32 Turn = 0; Turn < 8 && !bPlaced; ++Turn) { Character->SetActorRotation(FRotator(0, Turn * 45, 0)); bPlaced = Crafting->PlaceFromServer(OutReason); }
        }
        if (!bPlaced) { Character->SetActorLocation(Original); Character->SetActorRotation(OriginalRotation); }
        return bPlaced;
    }

    bool PlacePersistedCampConstruction(UKalmalaCraftingComponent* Crafting, AKalmalaCharacter* Character, FName KitId, FString& OutReason, AKalmalaConstructionActor*& OutConstruction)
    {
        TSet<AKalmalaConstructionActor*> Existing;
        for (TActorIterator<AKalmalaConstructionActor> It(Character->GetWorld()); It; ++It) Existing.Add(*It);
        const FVector Original = Character->GetActorLocation(); const FRotator OriginalRotation = Character->GetActorRotation();
        bool bPlaced = false;
        for (int32 Site = 0; Site < 16 && !bPlaced; ++Site)
        {
            Character->SetActorLocation(Original + FRotator(0, Site * 22.5f, 0).Vector() * (Site < 8 ? 70.0f : 140.0f));
            for (int32 Turn = 0; Turn < 16 && !bPlaced; ++Turn)
            {
                Character->SetActorRotation(FRotator(0, Turn * 22.5f, 0));
                bPlaced = Crafting->PlaceConstructionFromServer(KitId, OutReason);
            }
        }
        Character->SetActorLocation(Original); Character->SetActorRotation(OriginalRotation);
        if (bPlaced) for (TActorIterator<AKalmalaConstructionActor> It(Character->GetWorld()); It; ++It)
            if (!Existing.Contains(*It) && It->GetConstructionKit() == KitId) { OutConstruction = *It; break; }
        return bPlaced && OutConstruction != nullptr;
    }

    bool OpenPersistedCampStorage(UKalmalaCraftingComponent* Crafting, AKalmalaCharacter* Character, AKalmalaConstructionActor* Storage)
    {
        const FVector Original = Character->GetActorLocation();
        bool bOpened = false;
        for (int32 Turn = 0; Turn < 16 && !bOpened; ++Turn)
        {
            Character->SetActorLocation(Storage->GetActorLocation() + FRotator(0, Turn * 22.5f, 0).Vector() * 150.0f + FVector(0, 0, 40));
            bOpened = Crafting->OpenStorageFromServer(Storage);
        }
        if (!bOpened) Character->SetActorLocation(Original);
        return bOpened;
    }

    const AKalmalaCampfire* FindOwnedPersistedCampfire(const UWorld* World, const AKalmalaCharacter* Character)
    {
        if (World == nullptr || Character == nullptr) return nullptr;
        for (TActorIterator<AKalmalaCampfire> It(World); It; ++It)
        {
            if (It->GetOwner() == Character->GetController()) return *It;
        }
        return nullptr;
    }
}

void UKalmalaCraftingComponent::RunPersistedCampVerification(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    AKalmalaCharacter* Character = GetCharacter();
    UKalmalaInventoryComponent* Inventory = Character ? Character->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    if (Character == nullptr || Inventory == nullptr || Character->GetPlayerState() == nullptr) return;
    PersistedCampVerificationElapsed += DeltaTime;
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaPersistedCampRestoreTest")))
    {
        if (Character->HasAuthority() && PersistedCampVerificationStage == 0 && PersistedCampVerificationElapsed > 3.0f)
        {
            AKalmalaConstructionActor* Storage = nullptr;
            int32 ConstructionCount = 0;
            for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
            {
                if (It->GetConstructionKit() != NAME_None) ++ConstructionCount;
                if (Storage == nullptr && It->GetConstructionKit() == TEXT("StorageKit")) Storage = *It;
            }
            const bool bOpened = Storage != nullptr && OpenPersistedCampStorage(this, Character, Storage);
            const bool bStorage = bOpened && HasStorageView() && StorageView.Num() == 1
                && StorageView[0].ItemId == TEXT("Wood") && StorageView[0].Quantity == 1;
            const bool bPassed = ConstructionCount == 10 && bStorage;
            UE_LOG(LogTemp, Display, TEXT("Persisted camp restore server: Passed=%d Player=%d Constructions=%d StorageWood=%d"), bPassed, Character->GetPlayerState()->GetPlayerId(), ConstructionCount, bStorage);
            PersistedCampVerificationStage = bPassed ? 1 : 99;
            PersistedCampVerificationElapsed = 0.0f;
        }
        if (!Character->IsLocallyControlled() || bPersistedCampOwnerReported || PersistedCampVerificationElapsed < 6.0f) return;
        int32 ConstructionCount = 0;
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It) if (It->GetConstructionKit() != NAME_None) ++ConstructionCount;
        const bool bStorage = HasStorageView() && StorageView.Num() == 1 && StorageView[0].ItemId == TEXT("Wood") && StorageView[0].Quantity == 1;
        const bool bPassed = ConstructionCount == 10 && bStorage;
        UE_LOG(LogTemp, Display, TEXT("Persisted camp restore owner: Passed=%d Authority=%d Player=%d Constructions=%d StorageWood=%d"), bPassed, Character->HasAuthority(), Character->GetPlayerState()->GetPlayerId(), ConstructionCount, bStorage);
        bPersistedCampOwnerReported = true;
        return;
    }
    if (Character->HasAuthority() && PersistedCampVerificationStage == 0 && PersistedCampVerificationElapsed > 3)
    {
        int32 PlayerCount = 0;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It) PlayerCount += (*It && It->GetPlayerState()) ? 1 : 0;
        if (PlayerCount < 2) return;
        FString Reason;
        // These are deliberately harvested from initialized nodes instead of granted: one complete
        // personal camp needs direct Wood, Stone, and Fibre costs, including raw hearth fuel.
        const TMap<FName, int32> Required = { { TEXT("Wood"), 37 }, { TEXT("Stone"), 7 }, { TEXT("Fibre"), 32 } };
        const bool bGathered = GatherPersistedCampMaterials(Character, Inventory, Required);
        const bool bHearthMaterialsReady = bGathered;
        const bool bHearthPlaced = bHearthMaterialsReady && PlacePersistedCampfireNearTerrain(this, Character, Reason);
        const bool bKitsCrafted = bHearthPlaced && CraftFromServer(TEXT("Workbench"), 1, Reason)
            && CraftFromServer(TEXT("Storage"), 1, Reason);
        AKalmalaConstructionActor *Floor = nullptr, *Wall = nullptr, *Roof = nullptr, *Workbench = nullptr, *Storage = nullptr;
        const bool bBuilt = bKitsCrafted
            && PlacePersistedCampConstruction(this, Character, TEXT("FloorKit"), Reason, Floor)
            && PlacePersistedCampConstruction(this, Character, TEXT("WallKit"), Reason, Wall)
            && PlacePersistedCampConstruction(this, Character, TEXT("RoofKit"), Reason, Roof)
            && PlacePersistedCampConstruction(this, Character, TEXT("WorkbenchKit"), Reason, Workbench)
            && PlacePersistedCampConstruction(this, Character, TEXT("StorageKit"), Reason, Storage);
        const bool bStorage = bBuilt && Inventory->TryGrantFromServer(TEXT("Wood"), 1) && OpenPersistedCampStorage(this, Character, Storage)
            && TransferStorageFromServer(TEXT("Wood"), true, Reason) && HasStorageView() && StorageView.Num() == 1 && StorageView[0].ItemId == TEXT("Wood") && StorageView[0].Quantity == 1;
        const bool bPaid = bStorage && Inventory->GetStacks().IsEmpty();
        const bool bPassed = bGathered && bHearthMaterialsReady && bHearthPlaced && bKitsCrafted && bBuilt && bStorage && bPaid;
        const AKalmalaCampfire* OwnedFire = FindOwnedPersistedCampfire(GetWorld(), Character);
        UE_LOG(LogTemp, Display, TEXT("Persisted camp build server: Passed=%d Player=%d Gathered=%d Hearth=%d Kits=%d Built=%d Storage=%d Paid=%d Fuel=%.0f"), bPassed, Character->GetPlayerState()->GetPlayerId(), bGathered, bHearthPlaced, bKitsCrafted, bBuilt, bStorage, bPaid, OwnedFire ? OwnedFire->GetFuelSeconds() : -1.0f);
        if (bBuilt)
        {
            // The construction actor alone owns this opaque ID.  This development
            // evidence lets the restart runner compare restored replication without
            // introducing an ID into any client request or save mutation.
            for (const AKalmalaConstructionActor* Construction : { Floor, Wall, Roof, Workbench, Storage })
            {
                check(Construction != nullptr);
                UE_LOG(LogTemp, Display, TEXT("Persisted camp construction: Id=%s Kit=%s Player=%d"), *Construction->GetConstructionId(), *Construction->GetConstructionKit().ToString(), Character->GetPlayerState()->GetPlayerId());
            }
        }
        if (bPassed)
        {
            // The fixture selects its weather and only seeds the replicated state on
            // the server. Subsequent changes still come from the production GameMode
            // exposure tick, which samples the accepted construction geometry.
            AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
            check(WorldState != nullptr);
            FKalmalaWeatherState CampWeather = WorldState->GetWeatherState();
            CampWeather.WeatherCycleIndex = 77;
            CampWeather.ServerStartTimeSeconds = GetWorld()->GetTimeSeconds();
            CampWeather.DurationSeconds = 120.0f;
            CampWeather.PrecipitationIntensity = 0.75f;
            CampWeather.WindDirectionDegrees = 0;
            CampWeather.WindStrength = 1.0f;
            WorldState->SetWeatherStateFromServer(CampWeather);
            FKalmalaExposureState InitialExposure;
            InitialExposure.Wetness = 45.0f;
            InitialExposure.Warmth = 40.0f;
            InitialExposure.TravelSpeedMultiplier = FKalmalaExposureResponse::GetTravelSpeedMultiplier(InitialExposure.Warmth);
            Character->SetExposureStateFromServer(InitialExposure);
        }
        PersistedCampVerificationStage = bPassed ? 1 : 99; PersistedCampVerificationElapsed = 0;
    }
    if (!Character->IsLocallyControlled() || bPersistedCampOwnerReported) return;
    // Placement probes deliberately reposition the authoritative fixture pawn.
    // Its matching client must validate the replicated owned hearth identity, not
    // infer proximity from a client-side test teleport.
    const AKalmalaCampfire* Fire = FindOwnedPersistedCampfire(GetWorld(), Character);
    int32 ConstructionCount = 0;
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It) if (It->GetConstructionKit() != NAME_None) ++ConstructionCount;
    const bool bStorageVisible = HasStorageView() && StorageView.Num() == 1 && StorageView[0].ItemId == TEXT("Wood") && StorageView[0].Quantity == 1;
    // Exercise the actual replicated client copies rather than an actor-role unit
    // fixture. These are deliberately direct calls, not RPCs: none may write a
    // local authoritative-looking value or reach the server save owners.
    if (!Character->HasAuthority() && !bPersistedCampAuthorityProbeReported && PersistedCampVerificationElapsed >= 6.0f)
    {
        AKalmalaConstructionActor* Floor = nullptr;
        AKalmalaConstructionActor* Roof = nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
        {
            if (It->GetConstructionKit() == TEXT("FloorKit")) Floor = *It;
            if (It->GetConstructionKit() == TEXT("RoofKit")) Roof = *It;
        }
        UKalmalaPlayerStatusComponent* Statuses = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
        AKalmalaWorldGenerationGameState* MutableWorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
        const float WetBefore = Statuses ? Statuses->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WetStatusId) : -1.0f;
        const float HealthBefore = Floor ? Floor->GetHealth() : -1.0f;
        const EKalmalaHearthState FireBefore = Fire ? Fire->GetHearthState() : EKalmalaHearthState::Extinguished;
        const float FuelBefore = Fire ? Fire->GetFuelSeconds() : -1.0f;
        const FKalmalaWeatherState WeatherBefore = MutableWorldState ? MutableWorldState->GetWeatherState() : FKalmalaWeatherState{};
        if (Statuses) { Statuses->ApplyWetFromServer(); Statuses->AdvanceFromServer(120.0f); }
        if (Floor) Floor->AdvanceRainWearFromServer(1000.0f, 1.0f);
        if (Roof) Roof->AdvanceRainWearFromServer(1000.0f, 1.0f);
        if (Fire) const_cast<AKalmalaCampfire*>(Fire)->AdvanceFromServer(1000.0f, 1.0f, 1.0f);
        if (MutableWorldState)
        {
            FKalmalaWeatherState Forged = WeatherBefore;
            Forged.WeatherCycleIndex += 1;
            Forged.PrecipitationIntensity = 1.0f;
            MutableWorldState->SetWeatherStateFromServer(Forged);
        }
        const FKalmalaWeatherState WeatherAfter = MutableWorldState ? MutableWorldState->GetWeatherState() : FKalmalaWeatherState{};
        const bool bUnchanged = Statuses && Floor && Roof && Fire && MutableWorldState
            && FMath::IsNearlyEqual(Statuses->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WetStatusId), WetBefore)
            && FMath::IsNearlyEqual(Floor->GetHealth(), HealthBefore) && FMath::IsNearlyEqual(Roof->GetHealth(), AKalmalaConstructionActor::MaximumHealth)
            && Fire->GetHearthState() == FireBefore && FMath::IsNearlyEqual(Fire->GetFuelSeconds(), FuelBefore)
            && WeatherAfter.WeatherCycleIndex == WeatherBefore.WeatherCycleIndex
            && FMath::IsNearlyEqual(WeatherAfter.PrecipitationIntensity, WeatherBefore.PrecipitationIntensity)
            && GetWorld()->GetAuthGameMode() == nullptr;
        UE_LOG(LogTemp, Display, TEXT("Persisted camp client authority probe: Passed=%d Wet=%d FloorHealth=%.1f RoofHealth=%.1f FireState=%d Fuel=%.0f Weather=%d/%.2f SaveOwner=0"),
            bUnchanged, FMath::RoundToInt(WetBefore), Floor ? Floor->GetHealth() : -1.0f, Roof ? Roof->GetHealth() : -1.0f,
            Fire ? static_cast<int32>(Fire->GetHearthState()) : -1, Fire ? Fire->GetFuelSeconds() : -1.0f,
            WeatherAfter.WeatherCycleIndex, WeatherAfter.PrecipitationIntensity);
        bPersistedCampAuthorityProbeReported = true;
    }
    PersistedCampClientDiagnosticElapsed += DeltaTime;
    if (PersistedCampClientDiagnosticElapsed >= 5.0f)
    {
        UE_LOG(LogTemp, Display, TEXT("Persisted camp client progress: Player=%d Fire=%d Fuel=%.0f Constructions=%d EmptyPack=%d Storage=%d Stage=%d."),
            Character->GetPlayerState()->GetPlayerId(), Fire != nullptr, Fire ? Fire->GetFuelSeconds() : -1.0f,
            ConstructionCount, Inventory->GetStacks().IsEmpty(), bStorageVisible, PersistedCampVerificationStage);
        PersistedCampClientDiagnosticElapsed = 0.0f;
    }
    if (PersistedCampVerificationElapsed < 6) return;
    if (Fire == nullptr || !Inventory->GetStacks().IsEmpty() || ConstructionCount < 10 || !bStorageVisible) return;
    const AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldState == nullptr || WorldState->GetWeatherState().WeatherCycleIndex != 77) return;
    const FKalmalaWeatherState& Weather = WorldState->GetWeatherState();
    const FKalmalaExposureState& Exposure = Character->GetExposureState();
    const bool bWeather = FMath::IsNearlyEqual(Weather.PrecipitationIntensity, 0.75f) && Weather.WindDirectionDegrees == 0 && FMath::IsNearlyEqual(Weather.WindStrength, 1.0f);
    // In the selected storm the regular server tick must advance the seeded
    // state, even when the freely placed pieces do not form a full enclosure.
    const bool bExposure = Exposure.Wetness > 45.0f && Exposure.Warmth < 40.0f;
    float Shelter = -1.0f;
    if (Character->HasAuthority())
    {
        const FKalmalaEnvironmentalExposureSample Environment = FKalmalaEnvironmentalExposureSampler::Sample(WorldState->GetWorldGenerationConfig(), FVector2D(Character->GetActorLocation()));
        Shelter = FKalmalaShelterSampler::Sample(GetWorld(), Character, Environment.NaturalCover, Weather.WindDirectionDegrees).Shelter;
    }
    const bool bPassed = Fire != nullptr && FMath::IsNearlyEqual(Fire->GetFuelSeconds(), 60.0f) && ConstructionCount >= 10 && bStorageVisible && bWeather && bExposure;
    UE_LOG(LogTemp, Display, TEXT("Persisted camp build owner: Passed=%d Authority=%d Player=%d EmptyPack=%d Fuel=%.0f Constructions=%d StorageWood=1 Weather=%d/%.2f/%d/%.2f Shelter=%.2f Wetness=%.2f Warmth=%.2f"), bPassed, Character->HasAuthority(), Character->GetPlayerState()->GetPlayerId(), Inventory->GetStacks().IsEmpty(), Fire ? Fire->GetFuelSeconds() : -1.0f, ConstructionCount, Weather.WeatherCycleIndex, Weather.PrecipitationIntensity, Weather.WindDirectionDegrees, Weather.WindStrength, Shelter, Exposure.Wetness, Exposure.Warmth);
    bPersistedCampOwnerReported = true;
#endif
}

void UKalmalaCraftingComponent::RunRainVerticalSliceVerification(float DeltaTime)
{
#if !UE_BUILD_SHIPPING
    AKalmalaCharacter* Character = GetCharacter();
    if (bRainVerticalSliceClientReported || Character == nullptr || Character->HasAuthority() || !Character->IsLocallyControlled()) return;
    const AKalmalaWorldGenerationGameState* WorldState = GetWorld() ? GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>() : nullptr;
    if (WorldState == nullptr || WorldState->GetWeatherState().WeatherCycleIndex != 82) return;
    // Cycle 82 is the server's final observation marker. Let the ordinary
    // replicated hearth, construction, and status updates arrive before this
    // client-only verifier reads them.
    RainVerticalSliceClientObservationSeconds += FMath::Max(0.0f, DeltaTime);
    if (RainVerticalSliceClientObservationSeconds < 2.0f) return;
    const AKalmalaCampfire* Fire = nullptr;
    const AKalmalaConstructionActor* Exposed = nullptr;
    const AKalmalaConstructionActor* Roofed = nullptr;
    const AKalmalaConstructionActor* Roof = nullptr;
    for (TActorIterator<AKalmalaCampfire> It(GetWorld()); It; ++It)
        if (FVector::DistSquared(It->GetActorLocation(), Character->GetActorLocation()) <= FMath::Square(600.0f)) { Fire = *It; break; }
    for (TActorIterator<AKalmalaConstructionActor> It(GetWorld()); It; ++It)
    {
        if (It->GetConstructionId() == TEXT("RainSliceExposedFloor")) Exposed = *It;
        if (It->GetConstructionId() == TEXT("RainSliceRoofedFloor")) Roofed = *It;
        if (It->GetConstructionId() == TEXT("RainSliceFloorRoof")) Roof = *It;
    }
    const UKalmalaPlayerStatusComponent* Statuses = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    const bool bPassed = Fire != nullptr && Exposed != nullptr && Roofed != nullptr && Roof != nullptr && Statuses != nullptr
        && Fire->GetHearthState() == EKalmalaHearthState::Lit && Fire->HasRoof() && !Statuses->HasStatus(UKalmalaPlayerStatusComponent::WetStatusId)
        && FMath::IsNearlyEqual(Exposed->GetHealth(), AKalmalaConstructionActor::RainHealthFloor)
        && FMath::IsNearlyEqual(Roofed->GetHealth(), AKalmalaConstructionActor::MaximumHealth)
        && FMath::IsNearlyEqual(Roof->GetHealth(), AKalmalaConstructionActor::MaximumHealth);
    if (bPassed)
    {
        UE_LOG(LogTemp, Display, TEXT("Rain vertical slice client: Passed=1 Wet=0 ExposedHealth=%.1f RoofedHealth=%.1f RoofHealth=%.1f FireState=%d Roofed=%d"), Exposed->GetHealth(), Roofed->GetHealth(), Roof->GetHealth(), static_cast<int32>(Fire->GetHearthState()), Fire->HasRoof());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Rain vertical slice client: Passed=0"));
    }
    bRainVerticalSliceClientReported = true;
#endif
}
