#include "KalmalaWorldMapSubsystem.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaWorldMapWidget.h"
#include "Misc/CommandLine.h"

void UKalmalaWorldMapSubsystem::Tick(float DeltaTime)
{
    if (GetLocalPlayer() == nullptr || GetWorld() == nullptr || !GetWorld()->IsGameWorld()) return;
    APlayerController* FoundController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (LocalController != FoundController) ReleaseController();
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;
    LocalController = FoundController; BindLocalInput(LocalController);
    // Create the collapsed map at possession so walking reveals coverage before
    // the first M press. The companion minimap stays on its normal HUD layer.
    if (MapWidget == nullptr)
    {
        MapWidget = CreateWidget<UKalmalaWorldMapWidget>(LocalController, UKalmalaWorldMapWidget::StaticClass());
        if (MapWidget == nullptr) return;
        MapWidget->InitializeForLocalPlayer(LocalController); MapWidget->ConfigureViewportPlacement(); MapWidget->AddToPlayerScreen(150);
    }
    MapWidget->TickExploration(DeltaTime);
    if (!bDeveloperVerificationStarted && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")) && LocalController->GetPawn() != nullptr)
    {
        bDeveloperVerificationStarted = true;
        ToggleMap();
        if (MapWidget != nullptr) MapWidget->RunDeveloperVerification();
    }
    if (MapWidget != nullptr && MapWidget->IsMapOpen()) MapWidget->TickTilePresentation(DeltaTime);
}

void UKalmalaWorldMapSubsystem::Deinitialize() { ReleaseController(); Super::Deinitialize(); }

void UKalmalaWorldMapSubsystem::ReleaseController()
{
    if (UInputComponent* Input = BoundInputComponent.Get()) for (int32 Index = Input->GetNumActionBindings() - 1; Index >= 0; --Index)
        if (Input->GetActionBinding(Index).ActionDelegate.IsBoundToObject(this)) Input->RemoveActionBinding(Index);
    BoundInputComponent.Reset();
    if (MapWidget != nullptr) { MapWidget->Close(); MapWidget->RemoveFromParent(); MapWidget = nullptr; }
    LocalController = nullptr;
}

void UKalmalaWorldMapSubsystem::BindLocalInput(APlayerController* InLocalController)
{
    if (InLocalController == nullptr || InLocalController->InputComponent == nullptr || BoundInputComponent.Get() == InLocalController->InputComponent) return;
    FInputActionBinding& Binding = InLocalController->InputComponent->BindAction(TEXT("WorldMap"), IE_Pressed, this, &ThisClass::ToggleMap);
    Binding.bConsumeInput = true;
    FInputActionBinding& RecenterBinding = InLocalController->InputComponent->BindAction(TEXT("WorldMapRecenter"), IE_Pressed, this, &ThisClass::RecenterMap);
    RecenterBinding.bConsumeInput = true; BoundInputComponent = InLocalController->InputComponent;
}

void UKalmalaWorldMapSubsystem::ToggleMap()
{
    if (LocalController == nullptr) return;
    if (LocalController->IsMoveInputIgnored() && (MapWidget == nullptr || !MapWidget->IsMapOpen())) return;
    if (MapWidget == nullptr) return;
    if (MapWidget->IsMapOpen()) MapWidget->Close(); else MapWidget->Open();
}

bool UKalmalaWorldMapSubsystem::CloseMapIfOpen()
{
    if (MapWidget == nullptr || !MapWidget->IsMapOpen()) return false;
    MapWidget->Close(); return true;
}

void UKalmalaWorldMapSubsystem::RecenterMap()
{
    if (MapWidget != nullptr && MapWidget->IsMapOpen()) MapWidget->Recenter();
}
