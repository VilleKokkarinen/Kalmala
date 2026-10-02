#include "KalmalaSurvivalStatusSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaCharacter.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanTravelFeedbackComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaWeatherActivityWidget.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolLifecycleContract.h"

void UKalmalaSurvivalStatusSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr) return;

    APlayerController* FoundController = LocalPlayer->GetPlayerController(World);
    if (LocalController.Get() != FoundController)
    {
        ReleaseController();
        LocalController = FoundController;
    }
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;

    if (StatusWidget == nullptr)
    {
        StatusWidget = CreateWidget<UKalmalaSurvivalStatusWidget>(FoundController,
            UKalmalaSurvivalStatusWidget::StaticClass());
        if (StatusWidget == nullptr) return;
        StatusWidget->ConfigureViewportPlacement();
        StatusWidget->AddToPlayerScreen(54);
    }

    if (!HotbarWidget)
    {
        HotbarWidget = CreateWidget<UKalmalaStatusHotbarWidget>(FoundController, UKalmalaStatusHotbarWidget::StaticClass());
        if (HotbarWidget) HotbarWidget->AddToPlayerScreen(55);
    }
    FKalmalaSurvivalStatusSnapshot Snapshot;
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(FoundController->GetPawn());
    Snapshot.bHasCharacter = Character != nullptr;
    if (Character != nullptr)
    {
        if (FeedbackPawn.Get() != Character)
        {
            FeedbackPawn = Character;
            LastOceanTravelFeedbackSerial = 0;
            LastLoggedOceanTravelFeedbackSerial = 0;
            OceanTravelFeedbackExpiry = 0.0f;
        }

        const UKalmalaOceanTravelFeedbackComponent* Feedback = Character->GetOceanTravelFeedbackComponent();
        if (Feedback != nullptr)
        {
            Snapshot.OceanTravelFeedback = Feedback->GetFeedback();
            if (Feedback->GetFeedbackSerial() != LastOceanTravelFeedbackSerial)
            {
                LastOceanTravelFeedbackSerial = Feedback->GetFeedbackSerial();
                OceanTravelFeedbackExpiry = World->GetTimeSeconds() + 8.0f;
            }
            Snapshot.bShowOceanTravelFeedback = Feedback->GetFeedbackSerial() > 0
                && World->GetTimeSeconds() <= OceanTravelFeedbackExpiry;
        }

        if (const AKalmalaOceanSkiff* Skiff = Cast<AKalmalaOceanSkiff>(Character->GetAttachParentActor()))
        {
            Snapshot.bInOceanSkiff = true;
            Snapshot.bAtOceanSkiffHelm = Skiff->GetHelmOccupant() == Character;
            Snapshot.OceanSkiffMode = Skiff->GetMode();
            Snapshot.OceanSkiffBlockReason = Skiff->GetBlockReason();
        }

        Snapshot.Exposure = Character->GetExposureState();
        if (const UKalmalaPlayerStatusComponent* Status = Character->GetStatusComponent())
        {
            Snapshot.Statuses = Status->GetStatuses();
        }
        if (const UKalmalaSupportMagicComponent* Support = Character->GetSupportMagicComponent())
        {
            Snapshot.ActiveSupportEffect = Support->GetActiveEffect();
            Snapshot.ActiveSupportEffectExpiry = Support->GetActiveEffectExpiry();
            Snapshot.HearthShieldStrength = Support->GetHearthShieldStrength();
            Snapshot.BearsVigorStrengthMultiplier = Support->GetBearsVigorStrengthMultiplier();
        }
    }

    if (const AKalmalaWorldGenerationGameState* GameState = World->GetGameState<AKalmalaWorldGenerationGameState>())
    {
        Snapshot.ServerTimeSeconds = GameState->GetServerWorldTimeSeconds();
        Snapshot.Weather = GameState->GetWeatherState();
        Snapshot.bHasWeatherState = Snapshot.Weather.IsValid();
    }
    else
    {
        Snapshot.ServerTimeSeconds = World->GetTimeSeconds();
    }

    StatusWidget->SetSnapshot(Snapshot,
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    StatusDetailsText = UKalmalaSurvivalStatusWidget::BuildStatusText(Snapshot);
    int32 HotbarScale = UKalmalaSettingsWidget::GetTextScalePercent();
    int32 HotbarContrast = UKalmalaSettingsWidget::GetContrastMode();
#if !UE_BUILD_SHIPPING
    FString CapturePrefix;
    if (Character && HotbarWidget && FParse::Value(FCommandLine::Get(), TEXT("KalmalaHotbarCapture="), CapturePrefix))
    {
        VerificationElapsed += DeltaTime;
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaHotbarScale="), HotbarScale);
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaHotbarContrast="), HotbarContrast);
        if (VerificationCapture == 0 && VerificationElapsed > 4)
        {
            UE_LOG(LogTemp, Display, TEXT("Hotbar owner snapshot: NetMode=%d Local=%d Pawn=%s Statuses=%d WeatherValid=%d"),
                static_cast<int32>(World->GetNetMode()), FoundController->IsLocalController(), *Character->GetName(), Snapshot.Statuses.Num(), Snapshot.bHasWeatherState);
            VerificationCapture = 1;
        }
        // Explicitly requested read-only UI fixture: never mutates pawn, weather, inventory, or server time.
        if (VerificationElapsed >= 6 && VerificationElapsed < 21)
        {
            Snapshot = FKalmalaSurvivalStatusSnapshot(); Snapshot.bHasCharacter = true;
            if (VerificationElapsed >= 11 && VerificationElapsed < 16)
            {
                Snapshot.Statuses.Add({UKalmalaPlayerStatusComponent::WetStatusId,31.2f});
                Snapshot.Statuses.Add({UKalmalaPlayerStatusComponent::SteadyMealStatusId,74});
                Snapshot.Exposure.HeatIntensity=.6f; Snapshot.Exposure.ColdIntensity=.5f; Snapshot.Exposure.Warmth=20;
                Snapshot.ActiveSupportEffect=EKalmalaSupportEffect::HearthShield; Snapshot.ActiveSupportEffectExpiry=80; Snapshot.ServerTimeSeconds=45;
                Snapshot.bHasWeatherState=true; Snapshot.Weather.ServerStartTimeSeconds=15;
                Snapshot.Weather.PrecipitationIntensity=.9f; Snapshot.Weather.WindStrength=.9f; Snapshot.Weather.RefreshActivityLevel();
            }
        }
        if ((VerificationCapture == 1 && VerificationElapsed >= 9) || (VerificationCapture == 2 && VerificationElapsed >= 14)
            || (VerificationCapture == 3 && VerificationElapsed >= 19))
        {
            const TCHAR* Phase = VerificationCapture==1 ? TEXT("empty") : VerificationCapture==2 ? TEXT("populated") : TEXT("expired");
            const auto& Geometry = HotbarWidget->GetCachedGeometry();
            const auto TL = Geometry.LocalToAbsolute(FVector2D::ZeroVector);
            const auto BR = Geometry.LocalToAbsolute(Geometry.GetLocalSize());
            UE_LOG(LogTemp, Display, TEXT("Hotbar fixture: Phase=%s Entries=%d ReadOnly=%d Bounds=%.0f,%.0f,%.0f,%.0f Scale=%d Dpi=%.4f"),
                Phase, UKalmalaStatusHotbarWidget::BuildEntries(Snapshot).Num(), !HotbarWidget->IsFocusable(), TL.X,TL.Y,BR.X,BR.Y,HotbarScale,
                UWidgetLayoutLibrary::GetViewportScale(this));
            FScreenshotRequest::RequestScreenshot(CapturePrefix+TEXT("-")+Phase+TEXT(".png"),true,false);
            ++VerificationCapture;
        }
        if (VerificationCapture == 4 && VerificationElapsed >= 22)
        {
            VerificationDetails = CreateWidget<UKalmalaSettingsWidget>(FoundController, UKalmalaSettingsWidget::StaticClass());
            VerificationDetails->AddToPlayerScreen(200);
            UKalmalaSettingsWidget::SetTextScalePercent(HotbarScale);
            UKalmalaSettingsWidget::SetContrastMode(HotbarContrast);
            VerificationDetails->OpenForVerification(FoundController,3);
            ++VerificationCapture;
        }
        if (VerificationCapture == 5 && VerificationElapsed >= 25)
        {
            UE_LOG(LogTemp,Display,TEXT("Hotbar details: Focusable=%d OwnerLocal=1 LiveDetails=%d"),
                VerificationDetails->HasFocusableContentForVerification(), !StatusDetailsText.IsEmpty());
            FScreenshotRequest::RequestScreenshot(CapturePrefix+TEXT("-details.png"),true,false);
            ++VerificationCapture;
        }
        if (VerificationCapture == 6 && VerificationElapsed >= 28)
        {
            VerificationDetails->Close(); VerificationDetails->RemoveFromParent(); VerificationDetails=nullptr;
            TArray<FKalmalaCatalogueRow> Rows;
            for (const auto& Item : UKalmalaItemCatalogue::Get()->Items) Rows.Add({Item.ItemId,Item.DisplayName});
            for (const auto& Tool : FKalmalaToolLifecycleContract::GetDefinitions()) Rows.Add({Tool.ToolId,Tool.ToolId.ToString()});
            for (const auto& Tool : FKalmalaToolLifecycleContract::GetTieredAxeDefinitions()) Rows.Add({Tool.ToolId,Tool.ToolId.ToString()});
            const FName Hammer = FKalmalaToolLifecycleContract::GetConstructionHammerDefinition().ToolId;
            Rows.Add({Hammer,Hammer.ToString()});
            const auto Viewport = UWidgetLayoutLibrary::GetViewportSize(this)/UWidgetLayoutLibrary::GetViewportScale(this);
            const float Width = (Viewport.X-48)/3;
            for (int32 Column=0; Column<3; ++Column)
            {
                auto* Gallery = CreateWidget<UKalmalaCatalogueRowsWidget>(FoundController,UKalmalaCatalogueRowsWidget::StaticClass());
                Gallery->SetVerificationBackground();
                Gallery->AddToPlayerScreen(220);
                TArray<FKalmalaCatalogueRow> Part;
                for (int32 Index=Column*16; Index<FMath::Min(Rows.Num(),(Column+1)*16); ++Index) Part.Add(Rows[Index]);
                Gallery->SetRows(Part,100,1);
                Gallery->SetDesiredSizeInViewport(FVector2D(Width,Viewport.Y-48));
                Gallery->SetPositionInViewport(FVector2D(24+Width*Column,24),false);
                VerificationGallery.Add(Gallery);
            }
            UE_LOG(LogTemp,Display,TEXT("Catalogue icon gallery: ItemsAndTools=%d ReadOnly=1"),Rows.Num());
            ++VerificationCapture;
        }
        if (VerificationCapture==7 && VerificationElapsed>=31)
        {
            FScreenshotRequest::RequestScreenshot(CapturePrefix+TEXT("-icons.png"),true,false);
            ++VerificationCapture;
        }
    }
#endif
    if (HotbarWidget) HotbarWidget->SetSnapshot(Snapshot, HotbarScale, HotbarContrast);

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanSkiffFeedbackTest"))
        && Character != nullptr && Snapshot.bShowOceanTravelFeedback
        && Snapshot.OceanTravelFeedback != EKalmalaOceanTravelFeedback::None
        && LastLoggedOceanTravelFeedbackSerial != LastOceanTravelFeedbackSerial)
    {
        LastLoggedOceanTravelFeedbackSerial = LastOceanTravelFeedbackSerial;
        UE_LOG(LogTemp, Display, TEXT("Ocean skiff feedback UI local: NetMode=%d Serial=%u Message=%s"),
            static_cast<int32>(World->GetNetMode()), LastOceanTravelFeedbackSerial,
            *UKalmalaSurvivalStatusWidget::BuildOceanTravelText(Snapshot));
    }
#endif
}

void UKalmalaSurvivalStatusSubsystem::ReleaseController()
{
    if (StatusWidget != nullptr) StatusWidget->RemoveFromParent();
    StatusWidget = nullptr;
    if (HotbarWidget) HotbarWidget->RemoveFromParent();
    HotbarWidget = nullptr;
    StatusDetailsText.Empty();
    VerificationElapsed=0; VerificationCapture=0;
    if (VerificationDetails) { VerificationDetails->Close(); VerificationDetails->RemoveFromParent(); }
    VerificationDetails=nullptr;
    for (auto Gallery : VerificationGallery) if (Gallery) Gallery->RemoveFromParent();
    VerificationGallery.Reset();
    FeedbackPawn = nullptr;
    LastOceanTravelFeedbackSerial = 0;
    LastLoggedOceanTravelFeedbackSerial = 0;
    OceanTravelFeedbackExpiry = 0.0f;
}

void UKalmalaSurvivalStatusSubsystem::Deinitialize()
{
    ReleaseController();
    LocalController = nullptr;
    Super::Deinitialize();
}
