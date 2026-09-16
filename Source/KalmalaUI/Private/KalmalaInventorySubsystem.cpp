#include "KalmalaInventorySubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Styling/CoreStyle.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "GameFramework/InputSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UKalmalaInventoryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.04f, 0.9f));
    Border->SetPadding(FMargin(12));
    PackText = WidgetTree->ConstructWidget<UTextBlock>();
    PackText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    PackText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
    PackText->SetAutoWrapText(true);
    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Scroll->AddChild(PackText);
    Border->SetContent(Scroll);
    WidgetTree->RootWidget = Border;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaInventoryWidget::SetPackText(const FString& Text)
{
    if (PackText && PackText->GetText().ToString() != Text) PackText->SetText(FText::FromString(Text));
}

void UKalmalaInventorySubsystem::Tick(float DeltaTime)
{
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetLocalPlayer()) return;
    APlayerController* Controller = GetLocalPlayer()->GetPlayerController(GetWorld());
    if (Widget && Widget->GetOwningPlayer() != Controller)
    {
        Widget->RemoveFromParent();
        Widget = nullptr;
        bVerified = false;
    }
    if (!Controller || !Controller->IsLocalController()) return;
    if (!Widget)
    {
        Widget = CreateWidget<UKalmalaInventoryWidget>(Controller);
        if (!Widget) return;
        Widget->SetDesiredSizeInViewport(FVector2D(280, 420));
        Widget->SetPositionInViewport(FVector2D(24, 24), false);
        Widget->AddToPlayerScreen(40);
    }
    const APawn* Pawn = Controller->GetPawn();
    const UKalmalaInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    FString CraftKey = TEXT("Unbound");
    for (const auto& Mapping : GetDefault<UInputSettings>()->GetActionMappings())
        if (Mapping.ActionName == TEXT("CraftMenu") && !Mapping.Key.IsGamepadKey()) { CraftKey=Mapping.Key.GetDisplayName().ToString(); break; }
    const UKalmalaPlayerStatusComponent* Status = Pawn ? Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    const UKalmalaCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UKalmalaCombatComponent>() : nullptr;
    const UKalmalaDiscoveryProgressComponent* Discovery = Pawn ? Pawn->FindComponentByClass<UKalmalaDiscoveryProgressComponent>() : nullptr;
    FString Text;
    if (Status && Status->HasStatus(UKalmalaPlayerStatusComponent::WetStatusId))
    {
        Text = FString::Printf(TEXT("WET | %d s remaining\nMovement -%.0f%%\nStamina use +%.0f%%\nDry off near a lit campfire.\n\n"),
            FMath::CeilToInt(Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WetStatusId)),
            (1.0f - UKalmalaPlayerStatusComponent::WetMovementMultiplier) * 100.0f,
            (UKalmalaPlayerStatusComponent::WetStaminaUseMultiplier - 1.0f) * 100.0f);
    }
    else if (Status)
    {
        Text = TEXT("Wet: inactive\n\n");
    }
    if (Combat)
    {
        const TCHAR* Phase = Combat->GetActionPhase() == EKalmalaCombatActionPhase::Windup ? TEXT("WINDUP") : Combat->GetActionPhase() == EKalmalaCombatActionPhase::Recovery ? TEXT("RECOVERING") : TEXT("READY");
        Text += FString::Printf(TEXT("Combat: %s\n"), Phase);
        switch (Combat->GetFeedback())
        {
        case EKalmalaCombatFeedback::Hit: Text += TEXT("Attack result: HIT confirmed\n"); break;
        case EKalmalaCombatFeedback::Defeat: Text += TEXT("Attack result: DEFEAT confirmed\n"); break;
        case EKalmalaCombatFeedback::Unavailable: Text += TEXT("Attack unavailable: move closer or wait.\n"); break;
        default: Text += TEXT("Attack result: none\n"); break;
        }
        Text += TEXT("\n");
    }
    if (Discovery && Discovery->GetFeedbackSerial() > 0)
    {
        // Explicit text keeps discovery acknowledgement accessible without relying on marker colour.
        switch (Discovery->GetFeedback())
        {
        case EKalmalaDiscoveryFeedback::LandmarkFound: Text += TEXT("Discovery: landmark found\n"); break;
        case EKalmalaDiscoveryFeedback::ScrollFound: Text += TEXT("Discovery: scroll found\n"); break;
        case EKalmalaDiscoveryFeedback::AlreadyFound: Text += TEXT("Discovery: already found\n"); break;
        case EKalmalaDiscoveryFeedback::Unavailable: Text += TEXT("Discovery: unavailable\n"); break;
        default: break;
        }
        if (!Discovery->GetFeedbackLabel().IsEmpty()) Text += Discovery->GetFeedbackLabel() + TEXT("\n");
        Text += TEXT("\n");
    }
    int32 StatusLines = 0;
    for (const TCHAR Character : Text) if (Character == TEXT('\n')) ++StatusLines;
    Text += TEXT("Pack | Craft: ") + CraftKey + TEXT("\n");
    if (!Inventory) Text += TEXT("Waiting for player");
    else if (Inventory->GetStacks().IsEmpty()) Text += TEXT("Empty");
    else
    {
        for (const auto& Stack : Inventory->GetStacks())
        {
            const auto* Item = GetDefault<UKalmalaItemCatalogue>()->FindItem(Stack.ItemId);
            Text += FString::Printf(TEXT("%s: %d\n"), Item ? *Item->DisplayName : *Stack.ItemId.ToString(), Stack.Quantity);
        }
    }
    Widget->SetPackText(Text);
    Widget->SetDesiredSizeInViewport(FVector2D(280, 60 + (StatusLines + (Inventory ? FMath::Max(1, Inventory->GetStacks().Num()) : 1)) * 22));
#if !UE_BUILD_SHIPPING
    if (!bVerified && Inventory && Inventory->GetQuantity(TEXT("Wood")) == 7
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryTest")))
    {
        bVerified = true;
        UE_LOG(LogTemp, Display, TEXT("Inventory presentation: Owner=1 Wood=7 ReadOnly=%d"), !Widget->IsFocusable());
    }
#endif
}

void UKalmalaInventorySubsystem::Deinitialize()
{
    if (Widget) Widget->RemoveFromParent();
    Widget = nullptr;
    bVerified = false;
    Super::Deinitialize();
}
