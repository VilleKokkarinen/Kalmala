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
#include "KalmalaWeatherActivityWidget.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UKalmalaSurvivalStatusSubsystem::Tick(float)
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
    StatusWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

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
