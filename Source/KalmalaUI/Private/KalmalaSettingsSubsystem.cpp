#include "KalmalaSettingsSubsystem.h"

#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaWorldMapSubsystem.h"
#include "Engine/LocalPlayer.h"

void UKalmalaSettingsSubsystem::Tick(float DeltaTime)
{
    if (GetLocalPlayer() == nullptr || GetWorld() == nullptr || !GetWorld()->IsGameWorld()) return;
    APlayerController* FoundController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (LocalController != FoundController) ReleaseController();
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;
    LocalController = FoundController;
    BindLocalInput(LocalController);
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
}

void UKalmalaSettingsSubsystem::HandleSettingsMenu()
{
    if (LocalController == nullptr) return;
    if (UKalmalaWorldMapSubsystem* MapSubsystem = GetLocalPlayer()->GetSubsystem<UKalmalaWorldMapSubsystem>(); MapSubsystem != nullptr && MapSubsystem->CloseMapIfOpen()) return;
    if (SettingsWidget == nullptr)
    {
        SettingsWidget = CreateWidget<UKalmalaSettingsWidget>(LocalController, UKalmalaSettingsWidget::StaticClass());
        if (SettingsWidget == nullptr) return;
        SettingsWidget->AddToPlayerScreen(200);
    }
    if (SettingsWidget->IsMenuOpen()) SettingsWidget->Close(); else SettingsWidget->Open(LocalController);
}
