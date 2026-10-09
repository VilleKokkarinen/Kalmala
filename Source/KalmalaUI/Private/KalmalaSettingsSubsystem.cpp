#include "KalmalaSettingsSubsystem.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "HighResScreenshot.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "KalmalaCharacter.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"
#include "KalmalaWorldMapSubsystem.h"
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/LocalPlayer.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/UserInterfaceSettings.h"

#if !UE_BUILD_SHIPPING
namespace
{
    bool HasActionMapping(const UPlayerInput* PlayerInput, const FName ActionName, const FKey& Key)
    {
        return PlayerInput != nullptr && PlayerInput->ActionMappings.ContainsByPredicate(
            [ActionName, Key](const FInputActionKeyMapping& Mapping)
            {
                return Mapping.ActionName == ActionName && Mapping.Key == Key;
            });
    }

    bool HasEscapeSettingsMapping(const UPlayerInput* PlayerInput)
    {
        return HasActionMapping(PlayerInput, TEXT("SettingsMenu"), EKeys::Escape);
    }

    void RequestSettingsScreenshot(const TCHAR* TabName)
    {
        FString BasePath;
        if (!FParse::Value(FCommandLine::Get(), TEXT("KalmalaSettingsScreenshot="), BasePath)) return;
        const FString Path = FString::Printf(TEXT("%s-%s.png"), *BasePath, TabName);
        FScreenshotRequest::RequestScreenshot(Path, true, false);
        UE_LOG(LogTemp, Display, TEXT("Settings accessibility capture requested: Tab=%s Path=%s"), TabName, *Path);
    }
}
#endif

void UKalmalaSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UKalmalaSettingsWidget::ApplySavedMasterVolume();
    UKalmalaSettingsWidget::ApplySavedInterfaceScale();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaSettingsAccessibilityTest")))
    {
        UE_LOG(LogTemp, Display, TEXT("Settings accessibility subsystem initialized."));
    }
#endif
}

void UKalmalaSettingsSubsystem::Tick(float DeltaTime)
{
    if (GetLocalPlayer() == nullptr || GetWorld() == nullptr || !GetWorld()->IsGameWorld()) return;
    APlayerController* FoundController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (LocalController != FoundController) ReleaseController();
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;
    LocalController = FoundController;
    BindLocalInput(LocalController);
#if !UE_BUILD_SHIPPING
    RunDeveloperSettingsVerification(DeltaTime);
#endif
}

void UKalmalaSettingsSubsystem::Deinitialize() { ReleaseController(); Super::Deinitialize(); }

void UKalmalaSettingsSubsystem::ReleaseController()
{
    if (UInputComponent* Input = BoundInputComponent.Get())
    {
        for (int32 Index = Input->GetNumActionBindings() - 1; Index >= 0; --Index)
        {
            if (Input->GetActionBinding(Index).ActionDelegate.IsBoundToObject(this)) Input->RemoveActionBinding(Index);
        }
    }
    BoundInputComponent.Reset();
    if (SettingsWidget != nullptr) { SettingsWidget->Close(); SettingsWidget->RemoveFromParent(); SettingsWidget = nullptr; }
    LocalController = nullptr;
}

void UKalmalaSettingsSubsystem::BindLocalInput(APlayerController* InLocalController)
{
    if (InLocalController == nullptr || InLocalController->InputComponent == nullptr || BoundInputComponent.Get() == InLocalController->InputComponent) return;
    if (UInputComponent* Previous = BoundInputComponent.Get())
    {
        for (int32 Index = Previous->GetNumActionBindings() - 1; Index >= 0; --Index)
        {
            if (Previous->GetActionBinding(Index).ActionDelegate.IsBoundToObject(this)) Previous->RemoveActionBinding(Index);
        }
    }
    FInputActionBinding& Binding = InLocalController->InputComponent->BindAction(TEXT("SettingsMenu"), IE_Pressed, this, &ThisClass::HandleSettingsMenu);
    Binding.bConsumeInput = true;
    BoundInputComponent = InLocalController->InputComponent;
    UKalmalaSettingsWidget::ApplySavedInputBindings(InLocalController);
}

void UKalmalaSettingsSubsystem::HandleSettingsMenu()
{
    if (LocalController == nullptr) return;
    if (auto* Crafting = GetLocalPlayer()->GetSubsystem<UKalmalaCraftingSubsystem>(); Crafting && Crafting->CloseIfOpen()) return;
    if (UKalmalaWorldMapSubsystem* MapSubsystem = GetLocalPlayer()->GetSubsystem<UKalmalaWorldMapSubsystem>(); MapSubsystem != nullptr && MapSubsystem->CloseMapIfOpen()) return;
    if (SettingsWidget == nullptr)
    {
        SettingsWidget = CreateWidget<UKalmalaSettingsWidget>(LocalController, UKalmalaSettingsWidget::StaticClass());
        if (SettingsWidget == nullptr) return;
        SettingsWidget->AddToPlayerScreen(200);
    }
    if (SettingsWidget->IsMenuOpen()) SettingsWidget->Close(); else SettingsWidget->Open(LocalController);
}

#if !UE_BUILD_SHIPPING
void UKalmalaSettingsSubsystem::RunDeveloperSettingsVerification(const float DeltaTime)
{
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaSettingsPreferenceReloadTest")))
    {
        if (bDeveloperSettingsVerificationCompleted || LocalController == nullptr
            || LocalController->GetPawn() == nullptr)
        {
            return;
        }
        int32 ExpectedInterfaceScale = 100;
        int32 ExpectedReducedMotion = 0;
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaExpectedInterfaceScale="), ExpectedInterfaceScale);
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaExpectedReducedMotion="), ExpectedReducedMotion);
        const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(LocalController->GetPawn());
        const int32 InterfaceScale = UKalmalaSettingsWidget::GetInterfaceScalePercent();
        const bool bReducedMotion = UKalmalaSettingsWidget::IsReducedMotionEnabled();
        const float ExpectedApplicationScale = UKalmalaSettingsWidget::GetThemeDefaultApplicationScale()
            * InterfaceScale / 100.0f;
        const bool bScaleApplied = FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAppliedApplicationScale(),
            ExpectedApplicationScale, 0.001f);
        const bool bPreferencesRestored = Character != nullptr && InterfaceScale == ExpectedInterfaceScale
            && bReducedMotion == (ExpectedReducedMotion != 0) && bScaleApplied;
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility reload: Authority=%d Completed=%d InterfaceScale=%d ReducedMotion=%d AppliedScale=%.3f ScaleApplied=%d"),
            Character != nullptr && Character->HasAuthority() ? 1 : 0, bPreferencesRestored ? 1 : 0,
            InterfaceScale, bReducedMotion ? 1 : 0, UKalmalaSettingsWidget::GetAppliedApplicationScale(),
            bScaleApplied ? 1 : 0);
        if (!bPreferencesRestored)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility reload: FAIL local accessibility preferences did not reload."));
        }
        bDeveloperSettingsVerificationCompleted = true;
        return;
    }

    if (bDeveloperSettingsVerificationCompleted
        || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaSettingsAccessibilityTest"))
        || LocalController == nullptr || LocalController->GetPawn() == nullptr
        || GetWorld() == nullptr || SettingsWidget != nullptr && !SettingsWidget->IsValidLowLevel())
    {
        return;
    }

    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(LocalController->GetPawn());
    AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>();
    if (Character == nullptr || WorldState == nullptr || !WorldState->GetWorldGenerationConfig().IsValid()) return;

    if (bDeveloperScreenshotPending)
    {
        DeveloperScreenshotDelay += DeltaTime;
        if (DeveloperScreenshotDelay >= 0.25f)
        {
            bDeveloperScreenshotPending = false;
            DeveloperScreenshotDelay = 0.0f;
            static const TCHAR* ScreenshotTabs[] = { TEXT("settings"), TEXT("controls"), TEXT("audio"), TEXT("escape"), TEXT("video"), TEXT("motion"), TEXT("hud") };
            if (ScreenshotTabs[0] != nullptr && DeveloperSettingsScreenshotTab >= 0
                && DeveloperSettingsScreenshotTab < UE_ARRAY_COUNT(ScreenshotTabs))
            {
                RequestSettingsScreenshot(ScreenshotTabs[DeveloperSettingsScreenshotTab]);
            }
        }
    }

    if (bDeveloperOpeningProbePending && SettingsWidget != nullptr && SettingsWidget->IsMenuOpen())
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        const bool bShouldAnimate = Theme.ShouldAnimateOptionsOpening();
        const float InitialOffset = Theme.OptionsOpeningOffset(0.0f);
        const bool bOpeningStartWasLogged = bDeveloperOpeningProbeStartLogged;
        if (!bDeveloperOpeningProbeStartLogged)
        {
            ++DeveloperOpeningFocusFrameCount;
            if (SettingsWidget->HasFocusedContentForVerification())
            {
                bDeveloperOpeningProbeStartLogged = true;
                UE_LOG(LogTemp, Display,
                    TEXT("Settings accessibility: Authority=%d Stage=OpeningFocus Focused=1 FocusTargets=%d FocusDuringAnimation=%d Frame=%d"),
                    Character->HasAuthority() ? 1 : 0,
                    SettingsWidget->HasFocusableContentForVerification() ? 1 : 0,
                    (!bShouldAnimate || SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()) ? 1 : 0,
                    DeveloperOpeningFocusFrameCount);
            }
            else if (DeveloperOpeningFocusFrameCount >= 3)
            {
                UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL opening focus was not acquired during the transition."));
                bDeveloperSettingsVerificationCompleted = true;
                bDeveloperOpeningProbePending = false;
                return;
            }
            else
            {
                return;
            }
        }
        if (bOpeningStartWasLogged) DeveloperOpeningProbeElapsed += DeltaTime;
        const bool bReopenWasFocused = bDeveloperOpeningProbeReopenFocused;
        if (bDeveloperOpeningProbeReopened && !bDeveloperOpeningProbeReopenFocused
            && SettingsWidget->HasFocusedContentForVerification())
        {
            bDeveloperOpeningProbeReopenFocused = true;
            DeveloperOpeningReopenElapsed = 0.0f;
            bDeveloperOpeningProbeReopenFocusDuringAnimation = !bShouldAnimate
                || SettingsWidget->IsOptionsOpeningAnimationActiveForVerification();
            UE_LOG(LogTemp, Display,
                TEXT("Settings accessibility: Authority=%d Stage=OpeningCloseReopen Interrupted=%d Reopened=%d CloseReset=%d Focused=1 FocusDuringAnimation=%d CenterAnchored=%d"),
                Character->HasAuthority() ? 1 : 0, bDeveloperOpeningProbeInterrupted ? 1 : 0,
                bDeveloperOpeningProbeReopened ? 1 : 0, bDeveloperOpeningProbeCloseReset ? 1 : 0,
                bDeveloperOpeningProbeReopenFocusDuringAnimation ? 1 : 0,
                SettingsWidget->IsOptionsPanelCenterAnchoredForVerification() ? 1 : 0);
        }
        if (bReopenWasFocused) DeveloperOpeningReopenElapsed += DeltaTime;
        const float CurrentPanelY = SettingsWidget->GetOptionsPanelPositionYForVerification();
        if (bShouldAnimate && SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()
            && CurrentPanelY < -0.5f && CurrentPanelY > InitialOffset + 0.5f)
        {
            bDeveloperOpeningProbeObservedIntermediate = true;
        }

        if (bDeveloperOpeningProbeReopened && bDeveloperOpeningProbeReopenFocused && !bDeveloperOpeningProbeResizeAttempted
            && DeveloperOpeningReopenElapsed >= 0.04f)
        {
            if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
            {
                bDeveloperOpeningProbeResizeAttempted = true;
                const FVector2D InitialViewport = UWidgetLayoutLibrary::GetViewportSize(SettingsWidget);
                DeveloperOpeningOriginalResolution = FIntPoint(FMath::RoundToInt(InitialViewport.X),
                    FMath::RoundToInt(InitialViewport.Y));
                const FIntPoint TestResolution = DeveloperOpeningOriginalResolution == FIntPoint(1600, 900)
                    ? FIntPoint(1920, 1080) : FIntPoint(1600, 900);
                UserSettings->SetScreenResolution(TestResolution);
                UserSettings->ApplySettings(false);
                const FVector2D AppliedViewport = UWidgetLayoutLibrary::GetViewportSize(SettingsWidget);
                bDeveloperOpeningProbeResizeApplied = FMath::IsNearlyEqual(AppliedViewport.X, TestResolution.X, 2.0f)
                    && FMath::IsNearlyEqual(AppliedViewport.Y, TestResolution.Y, 2.0f);
                UE_LOG(LogTemp, Display,
                    TEXT("Settings accessibility: Authority=%d Stage=OpeningResize Applied=%d Viewport=%.0fx%.0f PanelY=%.2f CenterAnchored=%d"),
                    Character->HasAuthority() ? 1 : 0, bDeveloperOpeningProbeResizeApplied ? 1 : 0,
                    AppliedViewport.X, AppliedViewport.Y, SettingsWidget->GetOptionsPanelPositionYForVerification(),
                    SettingsWidget->IsOptionsPanelCenterAnchoredForVerification() ? 1 : 0);
            }
        }

        const bool bReadyToInterrupt = bShouldAnimate
            ? bDeveloperOpeningProbeObservedIntermediate && SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()
            : DeveloperOpeningProbeElapsed >= 0.08f;
        if (!bDeveloperOpeningProbeReopened && bReadyToInterrupt)
        {
            const bool bWasMidOpening = !bShouldAnimate || bDeveloperOpeningProbeObservedIntermediate
                || (SettingsWidget->IsOptionsOpeningAnimationActiveForVerification() && CurrentPanelY < -0.5f);
            SettingsWidget->Close();
            const bool bCloseReset = !SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()
                && FMath::IsNearlyZero(SettingsWidget->GetOptionsPanelPositionYForVerification(), 0.1f)
                && !LocalController->IsMoveInputIgnored() && !LocalController->IsLookInputIgnored();
            SettingsWidget->OpenForVerification(LocalController, 2);
            const float ReopenedY = SettingsWidget->GetOptionsPanelPositionYForVerification();
            const bool bReopenedAtStart = FMath::IsNearlyEqual(ReopenedY, InitialOffset, 0.1f)
                && LocalController->IsMoveInputIgnored()
                && LocalController->IsLookInputIgnored();
            bDeveloperOpeningProbeCloseReset = bCloseReset;
            bDeveloperOpeningProbeInterrupted = bWasMidOpening && bCloseReset;
            bDeveloperOpeningProbeReopened = bReopenedAtStart
                && SettingsWidget->IsOptionsPanelCenterAnchoredForVerification();
            DeveloperOpeningReopenElapsed = 0.0f;
            bDeveloperOpeningProbeReopenFocused = false;
            bDeveloperOpeningProbeReopenFocusDuringAnimation = false;
        }

        if (bDeveloperOpeningProbeResizeAttempted && !bDeveloperOpeningProbeResizeRestoreAttempted
            && DeveloperOpeningReopenElapsed >= 0.08f)
        {
            if (UGameUserSettings* UserSettings = UGameUserSettings::GetGameUserSettings())
            {
                bDeveloperOpeningProbeResizeRestoreAttempted = true;
                UserSettings->SetScreenResolution(DeveloperOpeningOriginalResolution);
                UserSettings->ApplySettings(false);
                const FVector2D RestoredViewport = UWidgetLayoutLibrary::GetViewportSize(SettingsWidget);
                bDeveloperOpeningProbeResizeRestored = FMath::IsNearlyEqual(RestoredViewport.X,
                    DeveloperOpeningOriginalResolution.X, 2.0f)
                    && FMath::IsNearlyEqual(RestoredViewport.Y, DeveloperOpeningOriginalResolution.Y, 2.0f);
                UE_LOG(LogTemp, Display,
                    TEXT("Settings accessibility: Authority=%d Stage=OpeningResizeRestore Restored=%d Viewport=%.0fx%.0f PanelY=%.2f CenterAnchored=%d"),
                    Character->HasAuthority() ? 1 : 0, bDeveloperOpeningProbeResizeRestored ? 1 : 0,
                    RestoredViewport.X, RestoredViewport.Y,
                    SettingsWidget->GetOptionsPanelPositionYForVerification(),
                    SettingsWidget->IsOptionsPanelCenterAnchoredForVerification() ? 1 : 0);
            }
        }

        if (bDeveloperOpeningProbeReopened)
        {
            const float ReopenedPanelY = SettingsWidget->GetOptionsPanelPositionYForVerification();
            if (bShouldAnimate && SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()
                && ReopenedPanelY < -0.5f && ReopenedPanelY > InitialOffset + 0.5f)
            {
                bDeveloperOpeningProbeReopenObservedIntermediate = true;
            }
            const bool bAnimationFinished = bShouldAnimate
                ? DeveloperOpeningReopenElapsed >= Theme.OptionsOpeningDuration + 0.04f
                    && !SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()
                : DeveloperOpeningReopenElapsed >= 0.04f
                    && !SettingsWidget->IsOptionsOpeningAnimationActiveForVerification();
            if (bAnimationFinished)
            {
                const bool bFinalPosition = FMath::IsNearlyZero(ReopenedPanelY, 0.1f);
                const bool bMotionEvidence = !bShouldAnimate || (bDeveloperOpeningProbeObservedIntermediate
                    && bDeveloperOpeningProbeReopenObservedIntermediate);
                bDeveloperOpeningProbePassed = bFinalPosition && bMotionEvidence
                    && bDeveloperOpeningProbeInterrupted && bDeveloperOpeningProbeResizeApplied
                    && bDeveloperOpeningProbeResizeRestored && bDeveloperOpeningProbeStartLogged
                    && bDeveloperOpeningProbeReopenFocused && SettingsWidget->HasFocusedContentForVerification()
                    && bDeveloperOpeningProbeReopenFocusDuringAnimation
                    && SettingsWidget->HasFocusableContentForVerification()
                    && SettingsWidget->IsOptionsPanelCenterAnchoredForVerification()
                    && LocalController->IsMoveInputIgnored() && LocalController->IsLookInputIgnored();
                UE_LOG(LogTemp, Display,
                    TEXT("Settings accessibility: Authority=%d Stage=OpeningAnimation Completed=%d Animated=%d Intermediate=%d Interrupted=%d Resize=1600x900 Restored=%d FinalY=%.2f Focused=%d CenterAnchored=%d"),
                    Character->HasAuthority() ? 1 : 0, bDeveloperOpeningProbePassed ? 1 : 0,
                    bShouldAnimate ? 1 : 0,
                    (!bShouldAnimate || (bDeveloperOpeningProbeObservedIntermediate
                        && bDeveloperOpeningProbeReopenObservedIntermediate)) ? 1 : 0,
                    bDeveloperOpeningProbeInterrupted ? 1 : 0, bDeveloperOpeningProbeResizeRestored ? 1 : 0,
                    ReopenedPanelY, SettingsWidget->HasFocusedContentForVerification() ? 1 : 0,
                    SettingsWidget->IsOptionsPanelCenterAnchoredForVerification() ? 1 : 0);
                if (!bDeveloperOpeningProbePassed)
                {
                    UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL opening animation, resize, interruption, focus, or reduced-motion path."));
                    bDeveloperSettingsVerificationCompleted = true;
                }
                bDeveloperOpeningProbePending = false;
            }
        }

        if (bDeveloperOpeningProbePending) return;
    }

    if (!bDeveloperSettingsVerificationStarted)
    {
        bDeveloperSettingsVerificationStarted = true;
        DeveloperSettingsInitialLocation = Character->GetActorLocation();
        DeveloperSettingsInitialHealth = Character->GetHealth();
        DeveloperSettingsInitialWorldSeed = WorldState->GetWorldGenerationConfig().WorldSeed;

        UKalmalaSettingsWidget::SetMasterVolume(0.5f);
        UKalmalaSettingsWidget::SetAudioCategoryVolume(EKalmalaAudioCategory::Ambient, 0.25f);
        UKalmalaSettingsWidget::SetAudioCategoryVolume(EKalmalaAudioCategory::Music, 0.5f);
        UKalmalaSettingsWidget::SetAudioCategoryVolume(EKalmalaAudioCategory::InteractionCombat, 0.75f);
        int32 ExpectedInterfaceScale = Character->HasAuthority() ? 90 : 110;
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaSettingsInterfaceScale="), ExpectedInterfaceScale);
        UKalmalaSettingsWidget::SetInterfaceScalePercent(ExpectedInterfaceScale);
        UKalmalaSettingsWidget::SetReducedMotionEnabled(false);
        UKalmalaSettingsWidget::SetTextScalePercent(150);
        UKalmalaSettingsWidget::SetContrastMode(1);
        UKalmalaSettingsWidget::SetFeedbackMode(1);
        const bool bKeyboardSet = UKalmalaSettingsWidget::SetLocalInputBinding(TEXT("Interact"), false, EKeys::F);
        const bool bGamepadSet = UKalmalaSettingsWidget::SetLocalInputBinding(
            TEXT("Interact"), true, EKeys::Gamepad_FaceButton_Right);
        UKalmalaSettingsWidget::ApplySavedInputBindings(LocalController);

        if (SettingsWidget == nullptr)
        {
            SettingsWidget = CreateWidget<UKalmalaSettingsWidget>(LocalController, UKalmalaSettingsWidget::StaticClass());
            if (SettingsWidget != nullptr) SettingsWidget->AddToPlayerScreen(200);
        }
        if (SettingsWidget == nullptr) return;

        SettingsWidget->OpenForVerification(LocalController, 2);
        const int32 InterfaceScaleBeforeTabMemory = UKalmalaSettingsWidget::GetInterfaceScalePercent();
        const int32 TextScaleBeforeTabMemory = UKalmalaSettingsWidget::GetTextScalePercent();
        const float MasterVolumeBeforeTabMemory = UKalmalaSettingsWidget::GetStoredMasterVolume();
        SettingsWidget->SetVerificationTab(5); // Return to the main shell.
        SettingsWidget->SetVerificationTab(4); // Reopen Options in the same local session.
        const bool bRememberedOptionsTab = SettingsWidget->GetRememberedOptionsTabForVerification() == 3
            && SettingsWidget->HasFocusedContentForVerification()
            && UKalmalaSettingsWidget::GetInterfaceScalePercent() == InterfaceScaleBeforeTabMemory
            && UKalmalaSettingsWidget::GetTextScalePercent() == TextScaleBeforeTabMemory
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetStoredMasterVolume(), MasterVolumeBeforeTabMemory);
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=RememberedOptionsTab Selected=3 Restored=%d Focused=%d ValuesUnchanged=%d"),
            Character->HasAuthority() ? 1 : 0,
            SettingsWidget->GetRememberedOptionsTabForVerification(),
            SettingsWidget->HasFocusedContentForVerification() ? 1 : 0,
            (UKalmalaSettingsWidget::GetInterfaceScalePercent() == InterfaceScaleBeforeTabMemory
                && UKalmalaSettingsWidget::GetTextScalePercent() == TextScaleBeforeTabMemory
                && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetStoredMasterVolume(), MasterVolumeBeforeTabMemory)) ? 1 : 0);
        if (!bRememberedOptionsTab)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL remembered Options tab did not restore cleanly."));
            bDeveloperSettingsVerificationCompleted = true;
            return;
        }
        const FKalmalaUITheme& OpeningTheme = FKalmalaUITheme::Get();
        const float ExpectedOpeningY = OpeningTheme.OptionsOpeningOffset(0.0f);
        bDeveloperOpeningProbeAnimated = SettingsWidget->IsOptionsOpeningAnimationActiveForVerification();
        bDeveloperOpeningProbePending = true;
        DeveloperOpeningProbeElapsed = 0.0f;
        const bool bOpeningStartedCorrectly = FMath::IsNearlyEqual(
                SettingsWidget->GetOptionsPanelPositionYForVerification(), ExpectedOpeningY, 0.1f)
            && SettingsWidget->HasFocusableContentForVerification()
            && SettingsWidget->IsOptionsPanelCenterAnchoredForVerification()
            && LocalController->IsMoveInputIgnored() && LocalController->IsLookInputIgnored()
            && bDeveloperOpeningProbeAnimated == OpeningTheme.ShouldAnimateOptionsOpening();
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=OpeningStart Animated=%d PanelY=%.2f ExpectedY=%.2f Duration=%.2f Travel=%.2f FocusQueued=1 MoveIgnored=%d LookIgnored=%d CenterAnchored=%d"),
            Character->HasAuthority() ? 1 : 0, bDeveloperOpeningProbeAnimated ? 1 : 0,
            SettingsWidget->GetOptionsPanelPositionYForVerification(), ExpectedOpeningY,
            OpeningTheme.OptionsOpeningDuration, OpeningTheme.OptionsOpeningTravel,
            LocalController->IsMoveInputIgnored() ? 1 : 0,
            LocalController->IsLookInputIgnored() ? 1 : 0,
            SettingsWidget->IsOptionsPanelCenterAnchoredForVerification() ? 1 : 0);
        if (!bOpeningStartedCorrectly)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL immediate opening focus, animation start, or centering."));
            bDeveloperSettingsVerificationCompleted = true;
            return;
        }
        const UPlayerInput* PlayerInput = LocalController->PlayerInput;
        const bool bInputApplied = bKeyboardSet && bGamepadSet
            && HasActionMapping(PlayerInput, TEXT("Interact"), EKeys::F)
            && HasActionMapping(PlayerInput, TEXT("Interact"), EKeys::Gamepad_FaceButton_Right)
            && HasEscapeSettingsMapping(PlayerInput);
        const bool bLocalRoundTrip = FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetStoredMasterVolume(), 0.5f)
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Ambient), 0.25f)
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Music), 0.5f)
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::InteractionCombat), 0.75f)
            && UKalmalaSettingsWidget::GetInterfaceScalePercent() == ExpectedInterfaceScale
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAppliedApplicationScale(),
                UKalmalaSettingsWidget::GetThemeDefaultApplicationScale() * ExpectedInterfaceScale / 100.0f, 0.001f)
            && !UKalmalaSettingsWidget::IsReducedMotionEnabled()
            && UKalmalaSettingsWidget::GetTextScalePercent() == 150
            && UKalmalaSettingsWidget::GetContrastMode() == 1
            && UKalmalaSettingsWidget::GetFeedbackMode() == 1;
        UKalmalaSettingsWidget::SetContrastMode(0);
        static const int32 OptionViews[] = { 5, 4, 0, 1, 2 };
        bool bOptionPanelImagesLoaded = true;
        for (const int32 ViewIndex : OptionViews)
        {
            SettingsWidget->SetVerificationTab(ViewIndex);
            bOptionPanelImagesLoaded &= !SettingsWidget->GetPanelImagePathForVerification().IsEmpty();
        }
        UKalmalaSettingsWidget::SetContrastMode(1);
        SettingsWidget->SetVerificationTab(2);
        const bool bHighContrastSuppressesImage = SettingsWidget->GetPanelImagePathForVerification().IsEmpty();
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=Settings Open=%d Focused=%d FocusTargets=%d MoveIgnored=%d LookIgnored=%d LocalRoundTrip=%d InputApplied=%d InterfaceScale=%d ReducedMotion=%d AppliedScale=%.3f TextScale=%d Contrast=%d Feedback=%d PanelImages=%d ContrastImageSuppressed=%d"),
            Character->HasAuthority() ? 1 : 0, SettingsWidget->IsMenuOpen() ? 1 : 0,
            SettingsWidget->HasFocusedContentForVerification() ? 1 : 0, SettingsWidget->HasFocusableContentForVerification() ? 1 : 0,
            LocalController->IsMoveInputIgnored() ? 1 : 0,
            LocalController->IsLookInputIgnored() ? 1 : 0, bLocalRoundTrip ? 1 : 0, bInputApplied ? 1 : 0,
            UKalmalaSettingsWidget::GetInterfaceScalePercent(), UKalmalaSettingsWidget::IsReducedMotionEnabled() ? 1 : 0,
            UKalmalaSettingsWidget::GetAppliedApplicationScale(),
            UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode(),
            UKalmalaSettingsWidget::GetFeedbackMode(), bOptionPanelImagesLoaded ? 1 : 0,
            bHighContrastSuppressesImage ? 1 : 0);
        UKalmalaSettingsWidget::SetContrastMode(0);
        SettingsWidget->SetVerificationTab(2);
        const bool bStandardBackgroundLoaded = !SettingsWidget->GetPanelImagePathForVerification().IsEmpty();
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=StandardBackgroundCapture Loaded=%d Contrast=%d"),
            Character->HasAuthority() ? 1 : 0, bStandardBackgroundLoaded ? 1 : 0,
            UKalmalaSettingsWidget::GetContrastMode());
        if (!bLocalRoundTrip || !bInputApplied || !bOptionPanelImagesLoaded
            || !bHighContrastSuppressesImage || !bStandardBackgroundLoaded)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL local options, page backgrounds, high contrast, or local input application."));
            bDeveloperSettingsVerificationCompleted = true;
            return;
        }
        DeveloperSettingsScreenshotTab = 0;
        bDeveloperScreenshotPending = true;
        DeveloperScreenshotDelay = 0.0f;
    }

    DeveloperSettingsVerificationElapsed += DeltaTime;
    if (DeveloperSettingsVerificationElapsed < 1.0f) return;
    DeveloperSettingsVerificationElapsed = 0.0f;

    if (DeveloperSettingsVerificationStage == 0)
    {
        if (!bDeveloperGameplayBaselineCaptured)
        {
            DeveloperSettingsInitialLocation = Character->GetActorLocation();
            DeveloperSettingsInitialHealth = Character->GetHealth();
            DeveloperSettingsInitialWorldSeed = WorldState->GetWorldGenerationConfig().WorldSeed;
            bDeveloperGameplayBaselineCaptured = true;
        }
        UKalmalaSettingsWidget::SetContrastMode(1);
        SettingsWidget->SetVerificationTab(1);
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=Controls Open=%d Focused=%d FocusTargets=%d FocusableControls=%d Keyboard=%s Controller=%s Escape=%d"),
            Character->HasAuthority() ? 1 : 0, SettingsWidget->IsMenuOpen() ? 1 : 0,
            SettingsWidget->HasFocusedContentForVerification() ? 1 : 0,
            SettingsWidget->HasFocusableContentForVerification() ? 1 : 0,
            SettingsWidget->HasFocusableControlsForVerification() ? 1 : 0,
            *UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), false).ToString(),
            *UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), true).ToString(),
            HasEscapeSettingsMapping(LocalController->PlayerInput) ? 1 : 0);
        DeveloperSettingsScreenshotTab = 1;
        bDeveloperScreenshotPending = true;
        DeveloperScreenshotDelay = 0.0f;
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (DeveloperSettingsVerificationStage == 1)
    {
        SettingsWidget->SetVerificationTab(0);
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=Audio Open=%d Focused=%d FocusTargets=%d Master=%.2f Ambient=%.2f Music=%.2f InteractionCombat=%.2f"),
            Character->HasAuthority() ? 1 : 0, SettingsWidget->IsMenuOpen() ? 1 : 0,
            SettingsWidget->HasFocusedContentForVerification() ? 1 : 0, SettingsWidget->HasFocusableContentForVerification() ? 1 : 0,
            UKalmalaSettingsWidget::GetStoredMasterVolume(),
            UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Ambient),
            UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Music),
            UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::InteractionCombat));
        DeveloperSettingsScreenshotTab = 2;
        bDeveloperScreenshotPending = true;
        DeveloperScreenshotDelay = 0.0f;
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (DeveloperSettingsVerificationStage == 2)
    {
        UKalmalaSettingsWidget::SetContrastMode(0);
        SettingsWidget->SetVerificationTab(5);
        const bool bImageLoaded = !SettingsWidget->GetPanelImagePathForVerification().IsEmpty();
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=EscapeBackgroundCapture Loaded=%d Contrast=%d FocusTargets=%d"),
            Character->HasAuthority() ? 1 : 0, bImageLoaded ? 1 : 0,
            UKalmalaSettingsWidget::GetContrastMode(), SettingsWidget->HasFocusableContentForVerification() ? 1 : 0);
        DeveloperSettingsScreenshotTab = 3;
        bDeveloperScreenshotPending = true;
        DeveloperScreenshotDelay = 0.0f;
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (DeveloperSettingsVerificationStage == 3)
    {
        SettingsWidget->SetVerificationTab(4);
        const bool bImageLoaded = !SettingsWidget->GetPanelImagePathForVerification().IsEmpty();
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=VideoBackgroundCapture Loaded=%d Contrast=%d FocusTargets=%d"),
            Character->HasAuthority() ? 1 : 0, bImageLoaded ? 1 : 0,
            UKalmalaSettingsWidget::GetContrastMode(), SettingsWidget->HasFocusableContentForVerification() ? 1 : 0);
        DeveloperSettingsScreenshotTab = 4;
        bDeveloperScreenshotPending = true;
        DeveloperScreenshotDelay = 0.0f;
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (DeveloperSettingsVerificationStage == 4)
    {
        UKalmalaSettingsWidget::SetContrastMode(1);
        SettingsWidget->SetVerificationTab(1);
        const bool bHighContrastSuppressesImage = SettingsWidget->GetPanelImagePathForVerification().IsEmpty();
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=ContrastRestore Mode=%d ImageSuppressed=%d"),
            Character->HasAuthority() ? 1 : 0, UKalmalaSettingsWidget::GetContrastMode(),
            bHighContrastSuppressesImage ? 1 : 0);
        if (!bHighContrastSuppressesImage)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL high contrast was not restored after standard captures."));
            bDeveloperSettingsVerificationCompleted = true;
            return;
        }
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (DeveloperSettingsVerificationStage == 5)
    {
        const bool bExpectedReducedMotion = Character->HasAuthority();
        UKalmalaSettingsWidget::SetReducedMotionEnabled(bExpectedReducedMotion);
        if (SettingsWidget != nullptr && SettingsWidget->IsMenuOpen()) SettingsWidget->Close();
        SettingsWidget->OpenForVerification(LocalController, 2);
        const bool bShouldAnimate = FKalmalaUITheme::Get().ShouldAnimateOptionsOpening();
        const bool bAnimationPathMatches = bShouldAnimate
            ? SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()
            : !SettingsWidget->IsOptionsOpeningAnimationActiveForVerification()
                && FMath::IsNearlyZero(SettingsWidget->GetOptionsPanelPositionYForVerification(), 0.1f);
        const bool bPanelFits = SettingsWidget->DoesOptionsPanelFitViewportForVerification();
        const FVector2D PanelSize = SettingsWidget->GetOptionsPanelSizeForVerification();
        const FVector2D LayoutSize = SettingsWidget->GetLayoutViewportSizeForVerification();
        const bool bMotionPreferencePassed = UKalmalaSettingsWidget::IsReducedMotionEnabled() == bExpectedReducedMotion
            && bAnimationPathMatches && SettingsWidget->HasFocusedContentForVerification() && bPanelFits;
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=MotionPreference ReducedMotion=%d Animated=%d Focused=%d PanelFits=%d InterfaceScale=%d Panel=%.0fx%.0f LayoutViewport=%.0fx%.0f"),
            Character->HasAuthority() ? 1 : 0, UKalmalaSettingsWidget::IsReducedMotionEnabled() ? 1 : 0,
            SettingsWidget->IsOptionsOpeningAnimationActiveForVerification() ? 1 : 0,
            SettingsWidget->HasFocusedContentForVerification() ? 1 : 0, bPanelFits ? 1 : 0,
            UKalmalaSettingsWidget::GetInterfaceScalePercent(), PanelSize.X, PanelSize.Y, LayoutSize.X, LayoutSize.Y);
        if (!bMotionPreferencePassed)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL reduced-motion override, focus, or scaled panel fit."));
            bDeveloperSettingsVerificationCompleted = true;
            return;
        }
        DeveloperSettingsScreenshotTab = 5;
        bDeveloperScreenshotPending = true;
        DeveloperScreenshotDelay = 0.0f;
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (DeveloperSettingsVerificationStage == 6)
    {
        if (SettingsWidget != nullptr && SettingsWidget->IsMenuOpen()) SettingsWidget->Close();
        const bool bInputRestored = !LocalController->IsMoveInputIgnored() && !LocalController->IsLookInputIgnored();
        const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(SettingsWidget);
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=HUDScale InterfaceScale=%d ReducedMotion=%d InputRestored=%d Viewport=%.0fx%.0f"),
            Character->HasAuthority() ? 1 : 0, UKalmalaSettingsWidget::GetInterfaceScalePercent(),
            UKalmalaSettingsWidget::IsReducedMotionEnabled() ? 1 : 0, bInputRestored ? 1 : 0,
            ViewportSize.X, ViewportSize.Y);
        if (!bInputRestored)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL scaled settings close did not restore gameplay input."));
            bDeveloperSettingsVerificationCompleted = true;
            return;
        }
        DeveloperSettingsScreenshotTab = 6;
        bDeveloperScreenshotPending = true;
        DeveloperScreenshotDelay = 0.0f;
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (SettingsWidget != nullptr && SettingsWidget->IsMenuOpen()) SettingsWidget->Close();
    const bool bGameplayStable = Character->GetHealth() == DeveloperSettingsInitialHealth
        && WorldState->GetWorldGenerationConfig().WorldSeed == DeveloperSettingsInitialWorldSeed
        && Character->GetActorLocation().Equals(DeveloperSettingsInitialLocation, 1.0f);
    UE_LOG(LogTemp, Display,
        TEXT("Settings accessibility: Authority=%d Completed=1 GameplayStable=%d InputRestored=%d WorldSeed=%llu"),
        Character->HasAuthority() ? 1 : 0, bGameplayStable ? 1 : 0,
        (!LocalController->IsMoveInputIgnored() && !LocalController->IsLookInputIgnored()) ? 1 : 0,
        DeveloperSettingsInitialWorldSeed);
    bDeveloperSettingsVerificationCompleted = true;
}
#endif
