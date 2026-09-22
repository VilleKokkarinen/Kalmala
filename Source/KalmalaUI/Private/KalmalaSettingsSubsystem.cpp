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
#include "KalmalaWorldMapSubsystem.h"
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/LocalPlayer.h"

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
        bDeveloperScreenshotPending = false;
        static const TCHAR* ScreenshotTabs[] = { TEXT("settings"), TEXT("controls"), TEXT("audio") };
        if (ScreenshotTabs[0] != nullptr && DeveloperSettingsScreenshotTab >= 0
            && DeveloperSettingsScreenshotTab < UE_ARRAY_COUNT(ScreenshotTabs))
        {
            RequestSettingsScreenshot(ScreenshotTabs[DeveloperSettingsScreenshotTab]);
        }
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
        const UPlayerInput* PlayerInput = LocalController->PlayerInput;
        const bool bInputApplied = bKeyboardSet && bGamepadSet
            && HasActionMapping(PlayerInput, TEXT("Interact"), EKeys::F)
            && HasActionMapping(PlayerInput, TEXT("Interact"), EKeys::Gamepad_FaceButton_Right)
            && HasEscapeSettingsMapping(PlayerInput);
        const bool bLocalRoundTrip = FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetStoredMasterVolume(), 0.5f)
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Ambient), 0.25f)
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Music), 0.5f)
            && FMath::IsNearlyEqual(UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::InteractionCombat), 0.75f)
            && UKalmalaSettingsWidget::GetTextScalePercent() == 150
            && UKalmalaSettingsWidget::GetContrastMode() == 1
            && UKalmalaSettingsWidget::GetFeedbackMode() == 1;
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=Settings Open=%d Focused=%d FocusTargets=%d MoveIgnored=%d LookIgnored=%d LocalRoundTrip=%d InputApplied=%d TextScale=%d Contrast=%d Feedback=%d"),
            Character->HasAuthority() ? 1 : 0, SettingsWidget->IsMenuOpen() ? 1 : 0,
            SettingsWidget->HasAnyUserFocus() ? 1 : 0, SettingsWidget->HasFocusableContentForVerification() ? 1 : 0,
            LocalController->IsMoveInputIgnored() ? 1 : 0,
            LocalController->IsLookInputIgnored() ? 1 : 0, bLocalRoundTrip ? 1 : 0, bInputApplied ? 1 : 0,
            UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode(),
            UKalmalaSettingsWidget::GetFeedbackMode());
        if (!bLocalRoundTrip || !bInputApplied)
        {
            UE_LOG(LogTemp, Error, TEXT("Settings accessibility: FAIL local option round-trip or local input application."));
            bDeveloperSettingsVerificationCompleted = true;
            return;
        }
        DeveloperSettingsScreenshotTab = 0;
        bDeveloperScreenshotPending = true;
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
        SettingsWidget->SetVerificationTab(1);
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=Controls Open=%d Focused=%d FocusTargets=%d FocusableControls=%d Keyboard=%s Controller=%s Escape=%d"),
            Character->HasAuthority() ? 1 : 0, SettingsWidget->IsMenuOpen() ? 1 : 0,
            SettingsWidget->HasAnyUserFocus() ? 1 : 0,
            SettingsWidget->HasFocusableContentForVerification() ? 1 : 0,
            SettingsWidget->HasFocusableControlsForVerification() ? 1 : 0,
            *UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), false).ToString(),
            *UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), true).ToString(),
            HasEscapeSettingsMapping(LocalController->PlayerInput) ? 1 : 0);
        DeveloperSettingsScreenshotTab = 1;
        bDeveloperScreenshotPending = true;
        ++DeveloperSettingsVerificationStage;
        return;
    }

    if (DeveloperSettingsVerificationStage == 1)
    {
        SettingsWidget->SetVerificationTab(0);
        UE_LOG(LogTemp, Display,
            TEXT("Settings accessibility: Authority=%d Stage=Audio Open=%d Focused=%d FocusTargets=%d Master=%.2f Ambient=%.2f Music=%.2f InteractionCombat=%.2f"),
            Character->HasAuthority() ? 1 : 0, SettingsWidget->IsMenuOpen() ? 1 : 0,
            SettingsWidget->HasAnyUserFocus() ? 1 : 0, SettingsWidget->HasFocusableContentForVerification() ? 1 : 0,
            UKalmalaSettingsWidget::GetStoredMasterVolume(),
            UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Ambient),
            UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::Music),
            UKalmalaSettingsWidget::GetAudioCategoryVolume(EKalmalaAudioCategory::InteractionCombat));
        DeveloperSettingsScreenshotTab = 2;
        bDeveloperScreenshotPending = true;
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
