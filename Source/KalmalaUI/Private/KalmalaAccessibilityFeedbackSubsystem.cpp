#include "KalmalaAccessibilityFeedbackSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaSupportMagicComponent.h"

namespace
{
FString MarkerLine(const TCHAR* Marker, const FString& Value)
{
    return FString::Printf(TEXT("[%s] %s\n"), Marker, *Value);
}
}

void UKalmalaAccessibilityFeedbackWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);

    Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AccessibilityFeedbackBackground"));
    Background->SetPadding(FMargin(12.0f));
    FeedbackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AccessibilityFeedbackText"));
    FeedbackText->SetAutoWrapText(true);
    FeedbackText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 16));
    Background->SetContent(FeedbackText);
    WidgetTree->RootWidget = Background;
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaAccessibilityFeedbackWidget::SetFeedbackText(const FString& Text)
{
    if (Background == nullptr || FeedbackText == nullptr) return;

    const bool bHighContrast = UKalmalaSettingsWidget::GetContrastMode() != 0;
    Background->SetBrushColor(bHighContrast
        ? FLinearColor(0.0f, 0.0f, 0.0f, 0.98f)
        : FLinearColor(0.025f, 0.035f, 0.04f, 0.94f));
    FeedbackText->SetColorAndOpacity(FSlateColor(bHighContrast
        ? FLinearColor::White
        : FLinearColor(0.9f, 0.95f, 0.92f, 1.0f)));
    FeedbackText->SetText(FText::FromString(Text));
}

FString UKalmalaAccessibilityFeedbackSubsystem::BuildFeedbackText(APawn* Pawn) const
{
    if (Pawn == nullptr) return FString();

    FString Text = TEXT("COLOUR-INDEPENDENT FEEDBACK — Text + markers\n");
    if (const UKalmalaCraftingComponent* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>())
    {
        Text += MarkerLine(TEXT("HEARTH"), Crafting->GetNearbyFireText());
        Text += MarkerLine(TEXT("CONSTRUCTION"), Crafting->GetNearbyConstructionText());
    }

    if (const UKalmalaCombatComponent* Combat = Pawn->FindComponentByClass<UKalmalaCombatComponent>())
    {
        const TCHAR* Phase = Combat->GetActionPhase() == EKalmalaCombatActionPhase::Windup
            ? TEXT("WINDUP") : Combat->GetActionPhase() == EKalmalaCombatActionPhase::Recovery ? TEXT("RECOVERING") : TEXT("READY");
        Text += MarkerLine(TEXT("COMBAT"), FString::Printf(TEXT("phase %s"), Phase));
        switch (Combat->GetFeedback())
        {
        case EKalmalaCombatFeedback::Hit: Text += MarkerLine(TEXT("HIT"), TEXT("confirmed")); break;
        case EKalmalaCombatFeedback::Defeat: Text += MarkerLine(TEXT("DEFEAT"), TEXT("confirmed")); break;
        case EKalmalaCombatFeedback::Unavailable: Text += MarkerLine(TEXT("UNAVAILABLE"), TEXT("move closer or wait")); break;
        default: break;
        }
    }

    if (const UKalmalaDiscoveryProgressComponent* Discovery = Pawn->FindComponentByClass<UKalmalaDiscoveryProgressComponent>();
        Discovery != nullptr && Discovery->GetFeedbackSerial() > 0)
    {
        const TCHAR* Result = TEXT("unavailable");
        switch (Discovery->GetFeedback())
        {
        case EKalmalaDiscoveryFeedback::LandmarkFound: Result = TEXT("landmark found"); break;
        case EKalmalaDiscoveryFeedback::ScrollFound: Result = TEXT("scroll found"); break;
        case EKalmalaDiscoveryFeedback::AlreadyFound: Result = TEXT("already found"); break;
        default: break;
        }
        Text += MarkerLine(TEXT("DISCOVERY"), Result);
    }

    if (const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Pawn))
    {
        if (const UKalmalaSupportMagicComponent* Support = Character->GetSupportMagicComponent())
        {
            const TCHAR* Result = Support->GetFeedbackSerial() == 0
                ? TEXT("no server result yet")
                : Support->GetFeedback() == EKalmalaSupportFeedback::Accepted ? TEXT("accepted") : TEXT("unavailable");
            Text += MarkerLine(TEXT("SUPPORT"), Result);

        }
    }

    return Text;
}

void UKalmalaAccessibilityFeedbackSubsystem::Tick(float DeltaTime)
{
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetLocalPlayer()) return;

    APlayerController* FoundController = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (Controller.Get() != FoundController)
    {
        ReleaseWidget();
        Controller = FoundController;
    }
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;

    if (UKalmalaSettingsWidget::GetFeedbackMode() == 0)
    {
        if (Widget != nullptr) Widget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (Widget == nullptr)
    {
        Widget = CreateWidget<UKalmalaAccessibilityFeedbackWidget>(FoundController,
            UKalmalaAccessibilityFeedbackWidget::StaticClass());
        if (Widget == nullptr) return;
        Widget->SetDesiredSizeInViewport(FVector2D(520.0f, 210.0f));
        Widget->SetPositionInViewport(FVector2D(24.0f, 500.0f), false);
        Widget->AddToPlayerScreen(120);
    }

    if (FoundController->GetPawn() == nullptr)
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    Widget->SetFeedbackText(BuildFeedbackText(FoundController->GetPawn()));
    Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaAccessibilityFeedbackSubsystem::ReleaseWidget()
{
    if (Widget != nullptr) Widget->RemoveFromParent();
    Widget = nullptr;
}

void UKalmalaAccessibilityFeedbackSubsystem::Deinitialize()
{
    ReleaseWidget();
    Controller = nullptr;
    Super::Deinitialize();
}
