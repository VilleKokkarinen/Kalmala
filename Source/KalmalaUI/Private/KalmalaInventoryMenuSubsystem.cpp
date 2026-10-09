#include "KalmalaInventoryMenuSubsystem.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaCatalogueRowsWidget.h"
#include "KalmalaItemDetailWidget.h"
#include "KalmalaIconWidget.h"
#include "KalmalaThemedButton.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

void UKalmalaInventoryMenuSubsystem::Tick(float DeltaTime)
{
    if (GetLocalPlayer() == nullptr || GetWorld() == nullptr || !GetWorld()->IsGameWorld()) return;

    APlayerController* FoundController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (LocalController != FoundController) ReleaseController();
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;

    LocalController = FoundController;
    BindLocalInput(LocalController);
    if (InventoryWidget != nullptr && InventoryWidget->IsMenuOpen())
    {
        InventoryWidget->RefreshOwnerInventory();
    }
#if !UE_BUILD_SHIPPING
    TickMenuReview(DeltaTime);
#endif
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

#if !UE_BUILD_SHIPPING
void UKalmalaInventoryMenuSubsystem::TickMenuReview(const float DeltaTime)
{
    FString Capture;
    if (ReviewStage >= 13 || !FParse::Value(FCommandLine::Get(), TEXT("KalmalaInventoryMenuCapture="), Capture)) return;
    const auto* Character = Cast<AKalmalaCharacter>(LocalController->GetPawn());
    const auto* Pack = Character ? Character->GetInventoryComponent() : nullptr;
    if (!Pack) return;
    const bool bHost = Character->HasAuthority();
    const int32 InitialWood = bHost ? 7 : 23;
    FString Food(TEXT("HearthBroth"));
    FParse::Value(FCommandLine::Get(), TEXT("KalmalaInventoryMenuFood="), Food);
    if (ReviewStage == 0)
    {
        if (Pack->GetQuantity(TEXT("Wood")) != InitialWood || Pack->GetQuantity(FName(*Food)) != 2) return;
        int32 Scale = 100, Contrast = 0, InterfaceScale = 100;
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperTextScale="), Scale);
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperContrast="), Contrast);
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperInterfaceScale="), InterfaceScale);
        UKalmalaSettingsWidget::SetTextScalePercent(Scale);
        UKalmalaSettingsWidget::SetContrastMode(Contrast);
        UKalmalaSettingsWidget::SetInterfaceScalePercent(InterfaceScale);
        UKalmalaSettingsWidget::SetReducedMotionEnabled(FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryMenuReducedMotion")));
        // Open the subsystem-owned production instance via its bound toggle.
        ToggleInventoryMenu();
        if (!InventoryWidget || !InventoryWidget->IsMenuOpen()) return;
        ReviewStage = 1;
        ReviewElapsed = 0.0f;
    }
    ReviewElapsed += DeltaTime;
    if (ReviewElapsed < 1.0f) return;
    auto* Menu = InventoryWidget.Get();
    if (!Menu || !Menu->WidgetTree) return;
    UEditableTextBox* Search = nullptr;
    UKalmalaCatalogueRowsWidget* Rows = nullptr;
    UKalmalaItemDetailWidget* Details = nullptr;
    UKalmalaThemedButton* Eat = nullptr;
    TArray<UScrollBox*> Scrolls;
    Menu->WidgetTree->ForEachWidget([&](UWidget* Widget)
    {
        if (auto* Value = Cast<UEditableTextBox>(Widget)) Search = Value;
        if (auto* Value = Cast<UKalmalaCatalogueRowsWidget>(Widget)) Rows = Value;
        if (auto* Value = Cast<UKalmalaItemDetailWidget>(Widget)) Details = Value;
        if (auto* Value = Cast<UScrollBox>(Widget)) Scrolls.Add(Value);
        if (auto* Button = Cast<UKalmalaThemedButton>(Widget))
            if (const auto* Label = Cast<UTextBlock>(Button->GetContent()); Label && Label->GetText().ToString() == TEXT("Eat one serving")) Eat = Button;
    });
    if (!Search || !Rows || !Details || !Eat || Scrolls.Num() != 2) return;
    const auto CaptureStage = [&](const TCHAR* Name, const bool bPassed)
    {
        bool bSquareIcon = true;
        if (Details->GetVisibility() != ESlateVisibility::Collapsed)
        {
            bSquareIcon = false;
            Details->WidgetTree->ForEachWidget([&](UWidget* Widget) {
                if (const auto* Icon = Cast<UKalmalaIconWidget>(Widget))
                    bSquareIcon = Icon->HasCatalogueTexture()
                        && Icon->GetCachedGeometry().GetLocalSize().Equals(FVector2D(64.0f, 64.0f), 0.5f);
            });
        }
        bool bPrivate = true;
        if (!bHost)
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
                if (*It != Character) bPrivate &= It->GetInventoryComponent()->GetStacks().IsEmpty()
                    && It->GetCarriedToolInventory().IsEmpty();
        UE_LOG(LogTemp, Display, TEXT("Inventory menu review: Stage=%s Passed=%d Host=%d Private=%d IconSquare=%d Wood=%d Selected=%s"),
            Name, bPassed && bPrivate && bSquareIcon, bHost, bPrivate, bSquareIcon, Pack->GetQuantity(TEXT("Wood")), *Menu->GetSelectedItemForVerification().ToString());
        FScreenshotRequest::RequestScreenshot(Capture + TEXT("-") + Name + TEXT(".png"), true, false);
        ReviewElapsed = 0.0f;
        ++ReviewStage;
    };
    // Screenshot requests are fulfilled at the end of this frame. Change the
    // next view only on the following tick so each PNG matches its marker.
    const auto NextFrame = [&](TFunction<void()> Action)
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(MoveTemp(Action));
    };
    switch (ReviewStage)
    {
    case 1:
        CaptureStage(TEXT("filled"), Rows->GetFilledSlotCount() == 3 && Rows->GetCarriedToolCount() == Character->GetCarriedToolInventory().Num()
            && Menu->GetSelectedItemForVerification() == TEXT("Wood"));
        NextFrame([Scrolls] { Scrolls[0]->ScrollToEnd(); });
        break;
    case 2:
        CaptureStage(TEXT("details"), Menu->IsMenuOpen() && Details->GetVisibility() != ESlateVisibility::Collapsed);
        NextFrame([Menu, Scrolls] {
            Menu->SetInventoryBrowseForVerification(TEXT(""), 2, 0);
            Menu->StepSelectionForVerification(1);
            Menu->SetInventoryScrollOffsetForVerification(10000.0f);
            Scrolls[1]->ScrollToEnd();
        });
        break;
    case 3:
        CaptureStage(TEXT("equipment"), Rows->GetCarriedToolCount() == Character->GetCarriedToolInventory().Num() && Rows->GetFilledSlotCount() == 0);
        NextFrame([Menu, Search, Scrolls] {
            Menu->SetInventoryScrollOffsetForVerification(0.0f);
            Search->SetText(FText::FromString(TEXT("absent owner item")));
            Search->OnTextChanged.Broadcast(Search->GetText());
            Scrolls[0]->ScrollToStart();
        });
        break;
    case 4:
        CaptureStage(TEXT("no-results"), Menu->GetVisibleItemIdsForVerification().IsEmpty()
            && Menu->GetSelectedItemForVerification().IsNone() && Details->GetVisibility() == ESlateVisibility::Collapsed);
        NextFrame([Search, Menu] {
            Search->SetText(FText::GetEmpty());
            Search->OnTextChanged.Broadcast(Search->GetText());
            Menu->SetInventoryBrowseForVerification(TEXT(""), 0, 0);
        });
        break;
    case 5:
        CaptureStage(TEXT("recovered"), Rows->GetFilledSlotCount() == 3 && Menu->HasBrowseFocusTargetsForVerification());
        NextFrame([Search, Food] {
            Search->SetText(FText::FromString(UKalmalaItemCatalogue::Get()->FindItem(FName(*Food))->DisplayName));
            Search->OnTextChanged.Broadcast(Search->GetText());
        });
        break;
    case 6:
        CaptureStage(TEXT("food-ready"), Menu->GetSelectedItemForVerification() == FName(*Food) && Eat->GetIsEnabled());
        NextFrame([Eat, Scrolls] { Eat->OnClicked.Broadcast(); Scrolls[0]->ScrollToEnd(); });
        break;
    case 7:
        if (Pack->GetQuantity(FName(*Food)) != 1 || !Character->GetStatusComponent()->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId)) return;
        CaptureStage(TEXT("food-accepted"), !Eat->GetIsEnabled());
        // Exercise a repeated menu action while its meal slot is occupied.
        NextFrame([Eat] { Eat->OnClicked.Broadcast(); });
        break;
    case 8:
        CaptureStage(TEXT("food-repeat"), Pack->GetQuantity(FName(*Food)) == 1 && !Eat->GetIsEnabled());
        NextFrame([Menu, Scrolls] {
            Menu->SetInventoryBrowseForVerification(TEXT("Wood"), 0, 0);
            Scrolls[0]->ScrollToEnd();
        });
        break;
    case 9:
        if (Pack->GetQuantity(TEXT("Wood")) != InitialWood - 3) return;
        CaptureStage(TEXT("live"), Menu->GetSelectedItemForVerification() == TEXT("Wood") && Menu->IsMenuOpen());
        break;
    case 10:
        if (!Pack->GetStacks().IsEmpty()) return;
        Menu->SetInventoryBrowseForVerification(TEXT(""), 1, 0);
        Menu->SetInventoryScrollOffsetForVerification(0.0f);
        Scrolls[0]->ScrollToStart(); Scrolls[1]->ScrollToStart();
        ReviewStage = 11; ReviewElapsed = 0.0f;
        break;
    case 11:
        CaptureStage(TEXT("empty"), Rows->GetSlotCapacity() == 16 && Rows->GetEmptySlotCount() == 16
            && Rows->GetFilledSlotCount() == 0 && Menu->GetSelectedItemForVerification().IsNone()
            && Details->GetVisibility() == ESlateVisibility::Collapsed);
        NextFrame([Menu, Scrolls] {
            Menu->SetInventoryScrollOffsetForVerification(10000.0f);
            Scrolls[0]->ScrollToEnd(); Scrolls[1]->ScrollToEnd();
        });
        break;
    case 12:
        CaptureStage(TEXT("empty-bottom"), Rows->GetEmptySlotCount() == 16);
        NextFrame([this, bHost, Food] {
            CloseIfOpen();
            UE_LOG(LogTemp, Display, TEXT("Inventory menu review complete: Passed=%d Host=%d Food=%s"),
                !IsMenuOpen() && !LocalController->IsMoveInputIgnored() && !LocalController->IsLookInputIgnored(), bHost, *Food);
        });
        break;
    }
}
#endif
