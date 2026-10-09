#include "KalmalaInventoryMenuSubsystem.h"

#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "KalmalaInventoryMenuWidget.h"
#include "KalmalaInventoryGridWidget.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaCharacter.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaThemedButton.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Algo/Count.h"

void UKalmalaInventoryMenuSubsystem::Tick(float DeltaTime)
{
    if (GetLocalPlayer() == nullptr || GetWorld() == nullptr || !GetWorld()->IsGameWorld()) return;

    APlayerController* FoundController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (LocalController != FoundController) ReleaseController();
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;

    LocalController = FoundController;
    BindLocalInput(LocalController);
    if (!HotbarWidget)
    {
        HotbarWidget = CreateWidget<UKalmalaInventoryGridWidget>(LocalController, UKalmalaInventoryGridWidget::StaticClass());
        if (HotbarWidget) HotbarWidget->AddToPlayerScreen(55);
    }
    if (HotbarWidget)
    {
        APawn* Pawn = LocalController->GetPawn().Get();
        auto* Inventory = Pawn ? Pawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
        HotbarWidget->Refresh(Inventory, true);
        const int32 Count = Inventory ? UKalmalaInventoryGridWidget::VisibleSlots(Inventory->GetGridSlots(), true).Num() : 0;
        int32 Width = 0, Height = 0;
        LocalController->GetViewportSize(Width, Height);
        const float Cell = FMath::Clamp(float(Width) * .04f, 28.0f, 56.0f);
        const FVector2D Size(FMath::Max(1, Count) * Cell, Cell);
        if (Size != LastHotbarSize)
        {
            LastHotbarSize = Size;
            HotbarWidget->SetDesiredSizeInViewport(Size);
            HotbarWidget->SetPositionInViewport(FVector2D(16.0f), false);
            HotbarWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
            HotbarWidget->SetAnchorsInViewport(FAnchors(0.0f));
        }
        HotbarWidget->SetVisibility(Count > 0 && !IsMenuOpen() && !LocalController->IsMoveInputIgnored()
            ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
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
    if (HotbarWidget) { HotbarWidget->RemoveFromParent(); HotbarWidget = nullptr; }
    LastHotbarSize = FVector2D::ZeroVector;
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
    if (ReviewStage >= 11 || !FParse::Value(FCommandLine::Get(), TEXT("KalmalaInventoryMenuCapture="), Capture)) return;
    auto* Character = Cast<AKalmalaCharacter>(LocalController->GetPawn());
    auto* Pack = Character ? Character->GetInventoryComponent() : nullptr;
    if (!Pack) return;
    const bool bHost = Character->HasAuthority();
    const int32 InitialWood = bHost ? 7 : 23;
    const FName Food(TEXT("HearthBroth"));
    if (ReviewStage == 0)
    {
        if (Pack->GetQuantity(TEXT("Wood")) != InitialWood || Pack->GetQuantity(Food) != 2 || Pack->GetGridSlots().Num() != 40) return;
        int32 Scale = 100, Contrast = 0, InterfaceScale = 100;
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperTextScale="), Scale);
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperContrast="), Contrast);
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaUIDeveloperInterfaceScale="), InterfaceScale);
        UKalmalaSettingsWidget::SetTextScalePercent(Scale);
        UKalmalaSettingsWidget::SetContrastMode(Contrast);
        UKalmalaSettingsWidget::SetInterfaceScalePercent(InterfaceScale);
        ToggleInventoryMenu();
        if (!InventoryWidget || !IsMenuOpen()) return;
        ReviewStage = 1;
        ReviewElapsed = 0.0f;
    }
    ReviewElapsed += DeltaTime;
    if (ReviewElapsed < 1.0f) return;
    auto* Menu = InventoryWidget.Get();
    UKalmalaThemedButton* Eat = nullptr;
    UScrollBox* Scroll = nullptr;
    if (!Menu || !Menu->WidgetTree) return;
    Menu->WidgetTree->ForEachWidget([&](UWidget* Widget)
    {
        if (auto* Value = Cast<UScrollBox>(Widget)) Scroll = Value;
        if (auto* Button = Cast<UKalmalaThemedButton>(Widget))
            if (const auto* Label = Cast<UTextBlock>(Button->GetContent()); Label && Label->GetText().ToString() == TEXT("Eat one serving")) Eat = Button;
    });
    if (!Eat || !Scroll) return;
    const auto CaptureStage = [&](const TCHAR* Name, const bool bPassed)
    {
        bool bPrivate = true;
        if (!bHost)
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
                if (*It != Character) bPrivate &= It->GetInventoryComponent()->GetStacks().IsEmpty()
                    && It->GetInventoryComponent()->GetGridSlots().IsEmpty() && It->GetCarriedToolInventory().IsEmpty();
        UE_LOG(LogTemp, Display, TEXT("Inventory menu review: Stage=%s Passed=%d Host=%d Private=%d Wood=%d Selected=%s"),
            Name, bPassed && bPrivate, bHost, bPrivate, Pack->GetQuantity(TEXT("Wood")), *Menu->GetSelectedItemForVerification().ToString());
        FScreenshotRequest::RequestScreenshot(Capture + TEXT("-") + Name + TEXT(".png"), true, false);
        ReviewElapsed = 0.0f;
        ++ReviewStage;
    };
    const auto NextFrame = [&](TFunction<void()> Action)
    { GetWorld()->GetTimerManager().SetTimerForNextTick(MoveTemp(Action)); };
    switch (ReviewStage)
    {
    case 1:
        CaptureStage(TEXT("filled"), Pack->GetGridSlots().Num() == 40 && HotbarWidget->GetVisibility() == ESlateVisibility::Collapsed);
        NextFrame([Scroll] { Scroll->ScrollToEnd(); });
        break;
    case 2:
        CaptureStage(TEXT("details"), Menu->IsMenuOpen() && !Menu->GetSelectedItemForVerification().IsNone());
        NextFrame([Pack, Scroll] {
            const int32 Source = Pack->GetGridSlots().IndexOfByKey(FName(TEXT("Wood")));
            Pack->ServerMoveSlot(Source, 9, TEXT("Wood"), Pack->GetSlotItem(9));
            Scroll->ScrollToStart();
        });
        break;
    case 3:
        if (Pack->GetSlotItem(9) != TEXT("Wood")) return;
        CaptureStage(TEXT("hotbar-assigned"), UKalmalaInventoryGridWidget::SlotLabel(9) == TEXT("0") && Pack->GetQuantity(TEXT("Wood")) == InitialWood);
        NextFrame([Pack] { Pack->ServerMoveSlot(9, 10, TEXT("Wood"), Pack->GetSlotItem(10)); });
        break;
    case 4:
        if (Pack->GetSlotItem(10) != TEXT("Wood") || Pack->GetSlotItem(9) == TEXT("Wood")) return;
        CaptureStage(TEXT("hotbar-removed"), Pack->GetQuantity(TEXT("Wood")) == InitialWood);
        NextFrame([Menu, Food, Scroll] { Menu->SelectItemForVerification(Food); Scroll->ScrollToEnd(); });
        break;
    case 5:
        CaptureStage(TEXT("food-ready"), Menu->GetSelectedItemForVerification() == Food && Eat->GetIsEnabled());
        NextFrame([Eat] { Eat->OnClicked.Broadcast(); });
        break;
    case 6:
        if (Pack->GetQuantity(Food) != 1 || !Character->GetStatusComponent()->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId)) return;
        CaptureStage(TEXT("food-accepted"), !Eat->GetIsEnabled());
        NextFrame([Eat] { Eat->OnClicked.Broadcast(); });
        break;
    case 7:
        CaptureStage(TEXT("food-repeat"), Pack->GetQuantity(Food) == 1 && !Eat->GetIsEnabled());
        NextFrame([Menu] { Menu->SelectItemForVerification(TEXT("Wood")); });
        break;
    case 8:
        if (Pack->GetQuantity(TEXT("Wood")) != InitialWood - 3) return;
        CaptureStage(TEXT("live"), Pack->GetSlotItem(10) == TEXT("Wood") && Menu->IsMenuOpen());
        break;
    case 9:
        if (!Pack->GetStacks().IsEmpty()) return;
        Scroll->ScrollToStart();
        CaptureStage(TEXT("materials-empty"), Pack->GetGridSlots().Num() == 40 && Pack->GetSlotItem(10).IsNone()
            && Algo::CountIf(Pack->GetGridSlots(), [](FName Id) { return !Id.IsNone(); }) == Character->GetCarriedToolInventory().Num());
        NextFrame([this] { CloseIfOpen(); });
        break;
    case 10:
        CaptureStage(TEXT("hud"), !IsMenuOpen() && !LocalController->IsMoveInputIgnored() && !LocalController->IsLookInputIgnored()
            && HotbarWidget && HotbarWidget->GetVisibility() == ESlateVisibility::HitTestInvisible);
        UE_LOG(LogTemp, Display, TEXT("Inventory menu review complete: Passed=%d Host=%d Food=HearthBroth"),
            !IsMenuOpen() && !LocalController->IsMoveInputIgnored() && !LocalController->IsLookInputIgnored(), bHost);
        break;
    }
}
#endif
