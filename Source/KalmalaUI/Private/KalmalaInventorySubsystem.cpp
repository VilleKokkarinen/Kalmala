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
    FString Text = TEXT("Pack\n");
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
    Widget->SetDesiredSizeInViewport(FVector2D(280, 60 + (Inventory ? FMath::Max(1, Inventory->GetStacks().Num()) : 1) * 22));
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
