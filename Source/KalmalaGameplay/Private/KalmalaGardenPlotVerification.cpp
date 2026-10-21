#include "KalmalaCraftingComponent.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UKalmalaCraftingComponent::RunGardenPlotVerification(const float DeltaTime)
{
    AKalmalaCharacter* Character = GetCharacter();
    UWorld* World = GetWorld();
    if (!Character || !Character->HasAuthority() || !Character->GetController() || !World) return;
    if (GardenPlotVerificationStage == 99 || GardenPlotVerificationStage >= 2) return;

    GardenPlotVerificationElapsed += FMath::Clamp(DeltaTime, 0.0f, 0.25f);
    const int32 OwnerId = Character->GetPlayerState() ? Character->GetPlayerState()->GetPlayerId() : -1;
    const auto Fail = [this, OwnerId](const FString& Reason)
    {
        GardenPlotVerificationStage = 99;
        UE_LOG(LogTemp, Error, TEXT("Garden Plot server: Owner=%d Passed=0 Reason=%s"), OwnerId, *Reason);
    };

    if (GardenPlotVerificationStage == 0)
    {
        int32 ControllerCount = 0;
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) ++ControllerCount;
        if (ControllerCount < 2)
        {
            if (GardenPlotVerificationElapsed > 60.0f) Fail(TEXT("Host/client fixture did not reach two peers"));
            return;
        }

        UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
        if (!Inventory)
        {
            Fail(TEXT("Owner inventory is unavailable"));
            return;
        }

        const FName CropPaymentItems[] = {
            TEXT("CarrotSeed"), TEXT("Carrot"), TEXT("Potato"), TEXT("RutabagaSeed"),
            TEXT("Rutabaga"), TEXT("OnionSeed"), TEXT("Onion")
        };
        for (const FName ItemId : CropPaymentItems)
        {
            if (!Inventory->TryGrantFromServer(ItemId, 1))
            {
                Fail(FString::Printf(TEXT("Could not seed crop-payment item %s"), *ItemId.ToString()));
                return;
            }
        }

        FKalmalaToolState* Trowel = Character->CarriedTools.FindByPredicate(
            [](const FKalmalaToolState& Tool) { return Tool.ItemId == TEXT("PlantingTrowel"); });
        if (!Trowel)
        {
            FKalmalaToolState NewTrowel;
            NewTrowel.ItemId = TEXT("PlantingTrowel");
            NewTrowel.ToolLevel = 1;
            NewTrowel.Durability = 100;
            NewTrowel.InstanceGuid = FGuid::NewGuid();
            Character->CarriedTools.Add(NewTrowel);
            Trowel = &Character->CarriedTools.Last();
        }
        Trowel->ToolLevel = 1;
        Trowel->Durability = 100;
        if (!Trowel->InstanceGuid.IsValid()) Trowel->InstanceGuid = FGuid::NewGuid();
        GardenPlotVerificationTrowelGuid = Trowel->InstanceGuid;
        Character->EnsureToolInstanceGuidsFromServer();
        Inventory->SynchronizeGridFromServer();

        FString MainHandReason;
        if (!Inventory->SetMainHandFromServer(GardenPlotVerificationTrowelGuid, MainHandReason))
        {
            Fail(FString::Printf(TEXT("Could not select Planting Trowel: %s"), *MainHandReason));
            return;
        }

        if (!UKalmalaRecipeCatalogue::BuildDirectMaterialCost(
                TEXT("GardenPlotKit"), GardenPlotVerificationPlacementCost, MainHandReason)
            || GardenPlotVerificationPlacementCost.IsEmpty())
        {
            Fail(FString::Printf(TEXT("Garden Plot direct-material recipe unavailable: %s"), *MainHandReason));
            return;
        }
        for (const FKalmalaInventoryStack& Cost : GardenPlotVerificationPlacementCost)
        {
            if (!Inventory->TryGrantFromServer(Cost.ItemId, Cost.Quantity))
            {
                Fail(FString::Printf(TEXT("Could not seed placement material %s"), *Cost.ItemId.ToString()));
                return;
            }
            GardenPlotVerificationCostBefore.Add(Inventory->GetQuantity(Cost.ItemId));
        }
        GardenPlotVerificationStartingCarrotSeeds = Inventory->GetQuantity(TEXT("CarrotSeed"));
        GardenPlotVerificationStartingTrowelCondition = Character->GetToolDurability(GardenPlotVerificationTrowelGuid);

        AKalmalaGameMode* GameMode = World->GetAuthGameMode<AKalmalaGameMode>();
        if (!GameMode || !GameMode->PersistPlayerStateFromServer(Character))
        {
            Fail(TEXT("Could not persist the verification Planting Trowel"));
            return;
        }

        TSet<AKalmalaConstructionActor*> ExistingConstructions;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It) ExistingConstructions.Add(*It);
        const FVector OriginalLocation = Character->GetActorLocation();
        const FRotator OriginalRotation = Character->GetActorRotation();
        FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(GardenPlotVerificationGround), false, Character);
        FString PlacementReason;
        bool bPlaced = false;
        const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        for (int32 Ring = 0; Ring < 6 && !bPlaced; ++Ring)
        {
            const float Radius = 600.0f + Ring * 500.0f;
            for (int32 DirectionIndex = 0; DirectionIndex < 8 && !bPlaced; ++DirectionIndex)
            {
                const float Yaw = DirectionIndex * 45.0f;
                const FVector Direction = FRotator(0.0f, Yaw, 0.0f).Vector();
                const FVector Probe = OriginalLocation + Direction * Radius;
                FHitResult CandidateGround;
                if (!World->LineTraceSingleByChannel(CandidateGround, Probe + FVector(0.0f, 0.0f, 1000.0f),
                        Probe - FVector(0.0f, 0.0f, 2000.0f), ECC_Visibility, GroundQuery)
                    || !Cast<AKalmalaGeneratedTerrainPatch>(CandidateGround.GetActor()))
                {
                    continue;
                }

                Character->SetActorLocation(CandidateGround.ImpactPoint - Direction * 165.0f
                    + FVector(0.0f, 0.0f, CapsuleHalfHeight + 2.0f), false, nullptr, ETeleportType::TeleportPhysics);
                Character->SetActorRotation(FRotator(0.0f, Yaw, 0.0f));
                bPlaced = PlaceConstructionFromServer(TEXT("GardenPlotKit"), PlacementReason);
            }
        }
        if (!bPlaced)
        {
            Character->SetActorLocation(OriginalLocation, false, nullptr, ETeleportType::TeleportPhysics);
            Character->SetActorRotation(OriginalRotation);
            Fail(FString::Printf(TEXT("Direct-material placement failed: %s"), *PlacementReason));
            return;
        }

        AKalmalaConstructionActor* Plot = nullptr;
        for (TActorIterator<AKalmalaConstructionActor> It(World); It; ++It)
        {
            if (!ExistingConstructions.Contains(*It) && It->GetConstructionKit() == TEXT("GardenPlotKit"))
            {
                Plot = *It;
                break;
            }
        }
        if (!Plot)
        {
            Fail(TEXT("Placed Garden Plot actor was not found"));
            return;
        }

        bool bPaidPlacementCost = true;
        for (int32 Index = 0; Index < GardenPlotVerificationPlacementCost.Num(); ++Index)
        {
            const FKalmalaInventoryStack& Cost = GardenPlotVerificationPlacementCost[Index];
            bPaidPlacementCost &= GardenPlotVerificationCostBefore.IsValidIndex(Index)
                && Inventory->GetQuantity(Cost.ItemId) == GardenPlotVerificationCostBefore[Index] - Cost.Quantity;
        }
        if (!bPaidPlacementCost || Inventory->GetQuantity(TEXT("GardenPlotKit")) != 0)
        {
            Fail(TEXT("Placement did not charge its direct recipe or created a portable kit"));
            return;
        }

        GardenPlotVerificationActor = Plot;
        InteractWithConstructionFromServer(Plot);
        GardenPlotVerificationStartingResultSerial = ResultSerial;
        const bool bContextReady = IsStationContextTargetCurrent(
            Plot, TEXT("GardenPlotKit"), Plot->GetConstructionId());
        if (!bContextReady)
        {
            Fail(TEXT("Server could not establish the nearby usable Garden Plot context"));
            return;
        }

        Character->ForceNetUpdate();
        GardenPlotVerificationStage = 1;
        GardenPlotVerificationElapsed = 0.0f;
        UE_LOG(LogTemp, Display,
            TEXT("Garden Plot server: Owner=%d Stage=Prepared PaidDirectCost=1 TrowelMainHand=1 Context=1"), OwnerId);
        return;
    }

    AKalmalaConstructionActor* Plot = GardenPlotVerificationActor.Get();
    UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
    if (!Plot || !Inventory)
    {
        Fail(TEXT("Verification plot or owner inventory expired"));
        return;
    }

    const FKalmalaGardenPlotState& State = Plot->GetGardenPlotState();
    if (State.CropMode == EKalmalaGardenCropMode::CarrotFood && State.IsPlanted()
        && ResultSerial != GardenPlotVerificationStartingResultSerial && WasLastResultAccepted())
    {
        bool bPaidPlacementCost = true;
        for (int32 Index = 0; Index < GardenPlotVerificationPlacementCost.Num(); ++Index)
        {
            const FKalmalaInventoryStack& Cost = GardenPlotVerificationPlacementCost[Index];
            bPaidPlacementCost &= GardenPlotVerificationCostBefore.IsValidIndex(Index)
                && Inventory->GetQuantity(Cost.ItemId) == GardenPlotVerificationCostBefore[Index] - Cost.Quantity;
        }
        const int32 CurrentCondition = Character->GetToolDurability(GardenPlotVerificationTrowelGuid);
        const bool bPlantedPayment = Inventory->GetQuantity(TEXT("CarrotSeed"))
            == GardenPlotVerificationStartingCarrotSeeds - 1;
        const bool bTrowelUsed = CurrentCondition == GardenPlotVerificationStartingTrowelCondition - 1;
        const bool bGrowing = State.RemainingActiveSeconds > 500.0f
            && State.RemainingActiveSeconds <= FKalmalaGardenPlotState::CropDurationSeconds;
        const bool bNoPortableKit = Inventory->GetQuantity(TEXT("GardenPlotKit")) == 0;
        const bool bPassed = bPaidPlacementCost && bPlantedPayment && bTrowelUsed
            && bGrowing && bNoPortableKit
            && LastResult.Contains(TEXT("Carrot plant"));
        if (!bPassed)
        {
            Fail(FString::Printf(TEXT("Transaction mismatch: Paid=%d Seed=%d Trowel=%d Growing=%d NoKit=%d Result=%s"),
                bPaidPlacementCost, bPlantedPayment, bTrowelUsed, bGrowing, bNoPortableKit, *LastResult));
            return;
        }

        GardenPlotVerificationStage = 2;
        UE_LOG(LogTemp, Display,
            TEXT("Garden Plot server: Owner=%d Passed=1 DirectCost=1 SeedSpent=1 TrowelSpent=1 Crop=CarrotFood ActiveSeconds=%.0f NoPortableKit=1"),
            OwnerId, State.RemainingActiveSeconds);
        return;
    }

    if (GardenPlotVerificationElapsed > 60.0f)
    {
        Fail(FString::Printf(TEXT("Timed out waiting for owner planting RPC; CropMode=%d ResultSerial=%u Accepted=%d Result=%s"),
            static_cast<int32>(State.CropMode), ResultSerial, WasLastResultAccepted(), *LastResult));
    }
}

#endif
