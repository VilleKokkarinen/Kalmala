#include "KalmalaCraftingSubsystem.h"

#if !UE_BUILD_SHIPPING

#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaStationContextWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"

bool UKalmalaCraftingWidget::VerifyGardenPlotPresentationForTest(const bool bExpectGrowing)
{
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(GetOwningPlayerPawn());
    UKalmalaCraftingComponent* Crafting = Model();
    UKalmalaInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
    AKalmalaConstructionActor* Plot = ContextStationActor.Get();
    if (!bOpen || !bEmbeddedContext || !bCultivationContext || !Character || !Crafting || !Inventory
        || !IsValid(Plot) || Plot->GetConstructionKit() != TEXT("GardenPlotKit")
        || StationFilterKit != TEXT("GardenPlotKit")
        || !StationContextSection.Equals(TEXT("Cultivate"), ESearchCase::IgnoreCase)
        || !IsStationContextValid()) return false;

    RefreshCultivationState(Crafting);
    const FString Status = CultivationContextStatusText ? CultivationContextStatusText->GetText().ToString() : FString();
    const FString Choices = CultivationChoicesText ? CultivationChoicesText->GetText().ToString() : FString();
    const FGuid MainHandGuid = Inventory->GetMainHandGuid();
    const FKalmalaToolState* Trowel = Character->GetCarriedToolInventory().FindByPredicate(
        [&MainHandGuid](const FKalmalaToolState& Tool)
        { return Tool.InstanceGuid == MainHandGuid && Tool.ItemId == TEXT("PlantingTrowel"); });
    const bool bTrowelReady = Trowel && Trowel->Durability > 0 && Inventory->GetActiveItem() == TEXT("PlantingTrowel");
    const bool bAllModesListed = Choices.Contains(TEXT("Carrot")) && Choices.Contains(TEXT("Potato"))
        && Choices.Contains(TEXT("Rutabaga")) && Choices.Contains(TEXT("Onion"));
    const bool bActionsVisible = CultivationContextPanel
        && CultivationContextPanel->GetVisibility() == ESlateVisibility::Visible
        && PlantCarrotFoodButton && PlantCarrotFoodButton->GetVisibility() == ESlateVisibility::Visible
        && PlantCarrotSeedButton && PlantCarrotSeedButton->GetVisibility() == ESlateVisibility::Visible
        && PlantPotatoTuberButton && PlantPotatoTuberButton->GetVisibility() == ESlateVisibility::Visible
        && PlantRutabagaFoodButton && PlantRutabagaFoodButton->GetVisibility() == ESlateVisibility::Visible
        && PlantRutabagaSeedButton && PlantRutabagaSeedButton->GetVisibility() == ESlateVisibility::Visible
        && PlantOnionFoodButton && PlantOnionFoodButton->GetVisibility() == ESlateVisibility::Visible
        && PlantOnionSeedButton && PlantOnionSeedButton->GetVisibility() == ESlateVisibility::Visible
        && HarvestGardenCropButton && HarvestGardenCropButton->GetVisibility() == ESlateVisibility::Visible;

    if (!bTrowelReady || !bAllModesListed || !bActionsVisible) return false;
    if (bExpectGrowing)
    {
        const FKalmalaGardenPlotState& State = Plot->GetGardenPlotState();
        return State.CropMode == EKalmalaGardenCropMode::CarrotFood && State.IsPlanted()
            && Status.Contains(TEXT("Carrot food growing"))
            && Status.Contains(TEXT("active seconds remaining"))
            && !PlantCarrotFoodButton->GetIsEnabled() && !PlantCarrotSeedButton->GetIsEnabled()
            && !PlantPotatoTuberButton->GetIsEnabled() && !PlantRutabagaFoodButton->GetIsEnabled()
            && !PlantRutabagaSeedButton->GetIsEnabled() && !PlantOnionFoodButton->GetIsEnabled()
            && !PlantOnionSeedButton->GetIsEnabled() && !HarvestGardenCropButton->GetIsEnabled();
    }

    return Plot->GetGardenPlotState().IsEmpty() && Status.StartsWith(TEXT("Empty plot"))
        && PlantCarrotFoodButton->GetIsEnabled() && PlantCarrotSeedButton->GetIsEnabled()
        && PlantPotatoTuberButton->GetIsEnabled() && PlantRutabagaFoodButton->GetIsEnabled()
        && PlantRutabagaSeedButton->GetIsEnabled() && PlantOnionFoodButton->GetIsEnabled()
        && PlantOnionSeedButton->GetIsEnabled() && !HarvestGardenCropButton->GetIsEnabled();
}

bool UKalmalaCraftingWidget::RequestGardenPlotPlantingForTest()
{
    if (!VerifyGardenPlotPresentationForTest(false) || !PlantCarrotFoodButton->GetIsEnabled()) return false;
    PlantCarrotFood();
    return true;
}

void UKalmalaCraftingSubsystem::RunGardenPlotUiVerification(const float DeltaTime)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaGardenPlotTest")) || GardenPlotUiTestStage >= 2
        || GardenPlotUiTestStage == 99) return;
    GardenPlotUiTestElapsed += FMath::Clamp(DeltaTime, 0.0f, 0.25f);

    APlayerController* PC = Controller;
    AKalmalaCharacter* Character = PC ? Cast<AKalmalaCharacter>(PC->GetPawn()) : nullptr;
    UKalmalaCraftingComponent* Crafting = Character
        ? Character->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    const int32 OwnerId = Character && Character->GetPlayerState() ? Character->GetPlayerState()->GetPlayerId() : -1;
    const auto Fail = [this, OwnerId](const FString& Reason)
    {
        GardenPlotUiTestStage = 99;
        UE_LOG(LogTemp, Error, TEXT("Garden Plot UI: Owner=%d Passed=0 Reason=%s"), OwnerId, *Reason);
    };
    const auto HasCultivationContext = [this, Crafting]()
    {
        return Crafting && Widget && Widget->IsOpen()
            && Widget->GetStationContextKit() == TEXT("GardenPlotKit")
            && Widget->GetStationContextSection().Equals(TEXT("Cultivate"), ESearchCase::IgnoreCase);
    };

    if (Crafting && PC && !HasCultivationContext())
    {
        AKalmalaConstructionActor* Plot = Crafting->GetLastStationContextActor();
        const FName Kit = Crafting->GetLastStationContextKit();
        const FString& ConstructionId = Crafting->GetLastStationContextConstructionId();
        if (Kit == TEXT("GardenPlotKit")
            && Crafting->IsStationContextTargetCurrent(Plot, Kit, ConstructionId))
        {
            if (!Widget)
            {
                Widget = CreateWidget<UKalmalaCraftingWidget>(PC);
                if (Widget) Widget->AddToPlayerScreen(160);
            }
            if (!StationContextWidget)
            {
                StationContextWidget = CreateWidget<UKalmalaStationContextWidget>(PC);
                if (StationContextWidget) StationContextWidget->AddToPlayerScreen(170);
            }
            if (Widget && StationContextWidget)
                StationContextWidget->OpenForStation(Plot, TEXT("Cultivate"), Widget);
        }
    }

    if (GardenPlotUiTestStage == 0)
    {
        if (HasCultivationContext() && Widget->VerifyGardenPlotPresentationForTest(false))
        {
            const double WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
            if (GardenPlotUiTestReadyTime < 0.0)
            {
                GardenPlotUiTestReadyTime = WorldTime;
                return;
            }
            if (WorldTime - GardenPlotUiTestReadyTime < 0.3) return;

            UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
            const FGuid MainHandGuid = Inventory ? Inventory->GetMainHandGuid() : FGuid();
            const FKalmalaToolState* Trowel = Character->GetCarriedToolInventory().FindByPredicate(
                [&MainHandGuid](const FKalmalaToolState& Tool)
                { return Tool.InstanceGuid == MainHandGuid && Tool.ItemId == TEXT("PlantingTrowel"); });
            if (!Inventory || !Trowel)
            {
                Fail(TEXT("Owner Planting Trowel or inventory is unavailable"));
                return;
            }
            GardenPlotUiTestStartingCarrotSeeds = Inventory->GetQuantity(TEXT("CarrotSeed"));
            GardenPlotUiTestStartingTrowelCondition = Trowel->Durability;
            GardenPlotUiTestResultSerial = Crafting->GetResultSerial();
            if (!Widget->RequestGardenPlotPlantingForTest())
            {
                Fail(TEXT("Cultivation UI did not enable the Carrot action"));
                return;
            }
            GardenPlotUiTestStage = 1;
            GardenPlotUiTestElapsed = 0.0f;
            UE_LOG(LogTemp, Display,
                TEXT("Garden Plot UI: Owner=%d View=Empty Passed=1 Modes=7 MainHandTrowel=1"), OwnerId);
            return;
        }
        if (GardenPlotUiTestElapsed > 60.0f)
        {
            const FString WidgetState = Widget
                ? FString::Printf(TEXT("WidgetOpen=%d Kit=%s Section=%s ContextValid=%d"),
                    Widget->IsOpen(), *Widget->GetStationContextKit().ToString(),
                    *Widget->GetStationContextSection(), Widget->IsStationContextValid())
                : TEXT("Widget=null");
            const FString OverlayState = StationContextWidget
                ? FString::Printf(TEXT("OverlayOpen=%d TargetValid=%d"),
                    StationContextWidget->IsOpen(), StationContextWidget->IsTargetValid())
                : TEXT("Overlay=null");
            const FString InputState = PC
                ? FString::Printf(TEXT("MoveIgnored=%d LookIgnored=%d"),
                    PC->IsMoveInputIgnored(), PC->IsLookInputIgnored())
                : TEXT("Controller=null");
            Fail(FString::Printf(TEXT("Cultivation panel did not become ready: HasContext=%d %s %s %s"),
                HasCultivationContext(), *WidgetState, *OverlayState, *InputState));
        }
        return;
    }

    if (GardenPlotUiTestStage == 1)
    {
        if (!Crafting)
        {
            if (GardenPlotUiTestElapsed > 60.0f) Fail(TEXT("Owner crafting component expired"));
            return;
        }
        if (Crafting->GetResultSerial() == GardenPlotUiTestResultSerial)
        {
            if (GardenPlotUiTestElapsed > 60.0f) Fail(TEXT("Planting request did not return to the owner"));
            return;
        }
        AKalmalaConstructionActor* Plot = Widget ? Widget->GetStationContextActor() : nullptr;
        UKalmalaInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
        const FKalmalaToolState* Trowel = Character
            ? Character->GetCarriedToolInventory().FindByPredicate([](const FKalmalaToolState& Tool)
                { return Tool.ItemId == TEXT("PlantingTrowel"); }) : nullptr;
        const bool bPayment = Inventory
            && Inventory->GetQuantity(TEXT("CarrotSeed")) == GardenPlotUiTestStartingCarrotSeeds - 1;
        const bool bTrowelWear = Trowel
            && Trowel->Durability == GardenPlotUiTestStartingTrowelCondition - 1;
        const bool bCorrectResult = Crafting->GetLastResult().StartsWith(TEXT("Carrot planted"));
        if (!Crafting->WasLastResultAccepted() || !bCorrectResult || !bPayment || !bTrowelWear)
        {
            Fail(FString::Printf(TEXT("Growing state mismatch: Accepted=%d Result=%d Payment=%d Trowel=%d Message=%s"),
                Crafting->WasLastResultAccepted(), bCorrectResult, bPayment, bTrowelWear, *Crafting->GetLastResult()));
            return;
        }
        if (!Plot || !Plot->GetGardenPlotState().IsPlanted() || !HasCultivationContext()
            || !Widget->VerifyGardenPlotPresentationForTest(true))
        {
            if (GardenPlotUiTestElapsed <= 5.0f) return;
            const bool bReplicatedCrop = Plot && Plot->GetGardenPlotState().IsPlanted();
            Fail(FString::Printf(TEXT("Growing view did not replicate: Crop=%d HasContext=%d ContextValid=%d"),
                bReplicatedCrop, HasCultivationContext(), Widget->IsStationContextValid()));
            return;
        }

        FString CapturePath;
        if (FParse::Value(FCommandLine::Get(), TEXT("KalmalaGardenPlotCapture="), CapturePath)
            && !CapturePath.IsEmpty())
        {
            FScreenshotRequest::RequestScreenshot(CapturePath, true, false);
            bGardenPlotUiCaptureRequested = true;
        }
        GardenPlotUiTestStage = 2;
        UE_LOG(LogTemp, Display,
            TEXT("Garden Plot UI: Owner=%d View=Growing Passed=1 Payment=1 TrowelWear=1 CaptureRequested=%d"),
            OwnerId, bGardenPlotUiCaptureRequested);
        return;
    }

}

#endif
