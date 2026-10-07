#include "KalmalaInventoryMenuSubsystem.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaSettingsWidget.h"

void UKalmalaInventoryMenuSubsystem::Tick(float)
{
    if (GetLocalPlayer() == nullptr || GetWorld() == nullptr || !GetWorld()->IsGameWorld()) return;

    APlayerController* FoundController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (LocalController != FoundController) ReleaseController();
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;

    LocalController = FoundController;
    BindLocalInput(LocalController);
}

void UKalmalaInventoryMenuSubsystem::Deinitialize()
{
    ReleaseController();
    Super::Deinitialize();
}

bool UKalmalaInventoryMenuSubsystem::IsMenuOpen() const
{
    return InventoryWidget != nullptr && InventoryWidget->IsMenuOpen();
}

void UKalmalaInventoryMenuSubsystem::ReleaseController()
{
    if (UInputComponent* Input = BoundInputComponent.Get())
    {
        for (int32 Index = Input->GetNumActionBindings() - 1; Index >= 0; --Index)
        {
            if (Input->GetActionBinding(Index).ActionDelegate.IsBoundToObject(this))
            {
                Input->RemoveActionBinding(Index);
            }
        }
    }
    BoundInputComponent.Reset();
    if (InventoryWidget != nullptr)
    {
        InventoryWidget->Close();
        InventoryWidget->RemoveFromParent();
        InventoryWidget = nullptr;
    }
    LocalController = nullptr;
}

void UKalmalaInventoryMenuSubsystem::BindLocalInput(APlayerController* InLocalController)
{
    if (InLocalController == nullptr || InLocalController->InputComponent == nullptr
        || BoundInputComponent.Get() == InLocalController->InputComponent) return;

    if (UInputComponent* Previous = BoundInputComponent.Get())
    {
        for (int32 Index = Previous->GetNumActionBindings() - 1; Index >= 0; --Index)
        {
            if (Previous->GetActionBinding(Index).ActionDelegate.IsBoundToObject(this))
            {
                Previous->RemoveActionBinding(Index);
            }
        }
    }

    FInputActionBinding& Binding = InLocalController->InputComponent->BindAction(
        TEXT("InventoryMenu"), IE_Pressed, this, &ThisClass::ToggleInventoryMenu);
    Binding.bConsumeInput = true;
    BoundInputComponent = InLocalController->InputComponent;
    UKalmalaSettingsWidget::ApplySavedInputBindings(InLocalController);
}

void UKalmalaInventoryMenuSubsystem::ToggleInventoryMenu()
{
    if (LocalController == nullptr || !LocalController->IsLocalController()) return;

    if (IsMenuOpen())
    {
        if (InventoryWidget->HasTextEntryFocus()) return;
        InventoryWidget->Close();
        return;
    }

    // Other modal menus own movement and look input. Do not stack Inventory
    // over crafting, Settings, the map, or another presentation modal.
    if (LocalController->IsMoveInputIgnored() || LocalController->IsLookInputIgnored()) return;

    if (InventoryWidget == nullptr)
    {
        InventoryWidget = CreateWidget<UKalmalaInventoryMenuWidget>(
            LocalController, UKalmalaInventoryMenuWidget::StaticClass());
        if (InventoryWidget == nullptr) return;
        InventoryWidget->AddToPlayerScreen(250);
    }

    InventoryWidget->Open();
}

bool UKalmalaInventoryMenuSubsystem::CloseIfOpen()
{
    if (InventoryWidget == nullptr || !InventoryWidget->IsMenuOpen()) return false;
    InventoryWidget->Close();
    return true;
}
