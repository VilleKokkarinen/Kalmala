#include "KalmalaInventorySubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
FString FindInputKeyLabel(const FName ActionName, const bool bGamepad)
{
    const UInputSettings* Settings = GetDefault<UInputSettings>();
    if (!Settings) return TEXT("Unbound");
    for (const FInputActionKeyMapping& Mapping : Settings->GetActionMappings())
        if (Mapping.ActionName == ActionName && Mapping.Key.IsGamepadKey() == bGamepad) return Mapping.Key.GetDisplayName().ToString();
    return TEXT("Unbound");
}

const TCHAR* InventorySupportEffectName(const EKalmalaSupportEffect Effect)
{
    switch (Effect)
    {
    case EKalmalaSupportEffect::Mending: return TEXT("Mending");
    case EKalmalaSupportEffect::HearthShield: return TEXT("Hearth Shield");
    case EKalmalaSupportEffect::BearsVigor: return TEXT("Bear's Vigor");
    case EKalmalaSupportEffect::DeerCall: return TEXT("Deer Call");
    default: return TEXT("None");
    }
}

void AppendCircle(TArray<FVector2D>& Points, const FVector2D Centre, const float Radius, const int32 Segments)
{
    Points.Reset(Segments + 1);
    for (int32 Index = 0; Index <= Segments; ++Index)
    {
        const float Angle = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
        Points.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
    }
}
}

void UKalmalaSupportGlyphWidget::SetGlyphState(const EKalmalaSupportGlyph InGlyph, const bool bInLearned, const bool bInSelected)
{
    if (Glyph == InGlyph && bLearned == bInLearned && bSelected == bInSelected) return;
    Glyph = InGlyph;
    bLearned = bInLearned;
    bSelected = bInSelected;
    Invalidate(EInvalidateWidget::Paint);
}

int32 UKalmalaSupportGlyphWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const float Scale = FMath::Min(Size.X, Size.Y) / 36.0f;
    if (Scale <= 0.0f) return DrawLayer;

    const FVector2D Centre = Size * 0.5f;
    const FLinearColor Ink = !bLearned
        ? FLinearColor(0.38f, 0.46f, 0.47f, 1.0f)
        : bSelected ? FLinearColor(1.0f, 0.74f, 0.38f, 1.0f) : FLinearColor(0.72f, 0.91f, 0.84f, 1.0f);
    const auto ToLocal = [Centre, Scale](const FVector2D Point)
    {
        return Centre + (Point - FVector2D(18.0f, 18.0f)) * Scale;
    };
    const auto DrawPath = [&OutDrawElements, &AllottedGeometry, DrawLayer, Ink, &ToLocal](const TArray<FVector2D>& Source, const bool bClosed, const float Thickness)
    {
        if (Source.Num() < 2) return;
        TArray<FVector2D> Points;
        Points.Reserve(Source.Num() + (bClosed ? 1 : 0));
        for (const FVector2D Point : Source) Points.Add(ToLocal(Point));
        if (bClosed)
        {
            const FVector2D FirstPoint = Points[0];
            Points.Add(FirstPoint);
        }
        FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Ink, true, Thickness);
    };
    const auto DrawCircle = [&DrawPath](const FVector2D CircleCentre, const float Radius, const int32 Segments, const float Thickness)
    {
        TArray<FVector2D> Points;
        AppendCircle(Points, CircleCentre, Radius, Segments);
        DrawPath(Points, false, Thickness);
    };
    const auto DrawDot = [&DrawCircle](const FVector2D DotCentre, const float Radius)
    {
        DrawCircle(DotCentre, Radius, 10, 1.8f);
    };

    if (bSelected)
    {
        TArray<FVector2D> SelectionRing;
        AppendCircle(SelectionRing, FVector2D(18.0f, 18.0f), 16.0f, 32);
        DrawPath(SelectionRing, false, 1.6f);
    }

    switch (Glyph)
    {
    case EKalmalaSupportGlyph::Mending:
        DrawCircle(FVector2D(18.0f, 18.0f), 10.0f, 24, 2.0f);
        DrawPath({ FVector2D(18.0f, 12.0f), FVector2D(18.0f, 24.0f) }, false, 2.2f);
        DrawPath({ FVector2D(12.0f, 18.0f), FVector2D(24.0f, 18.0f) }, false, 2.2f);
        break;
    case EKalmalaSupportGlyph::HearthShield:
        DrawPath({ FVector2D(18.0f, 5.0f), FVector2D(27.0f, 9.0f), FVector2D(25.0f, 20.0f),
            FVector2D(18.0f, 27.0f), FVector2D(11.0f, 20.0f), FVector2D(9.0f, 9.0f) }, true, 2.1f);
        DrawPath({ FVector2D(13.0f, 16.0f), FVector2D(17.0f, 20.0f), FVector2D(23.0f, 13.0f) }, false, 1.8f);
        break;
    case EKalmalaSupportGlyph::BearsVigor:
        DrawDot(FVector2D(9.0f, 11.0f), 2.3f);
        DrawDot(FVector2D(14.0f, 7.0f), 2.3f);
        DrawDot(FVector2D(21.0f, 7.0f), 2.3f);
        DrawDot(FVector2D(27.0f, 11.0f), 2.3f);
        DrawPath({ FVector2D(9.0f, 19.0f), FVector2D(12.0f, 15.0f), FVector2D(23.0f, 15.0f),
            FVector2D(27.0f, 19.0f), FVector2D(25.0f, 25.0f), FVector2D(11.0f, 25.0f) }, true, 2.0f);
        break;
    case EKalmalaSupportGlyph::DeerCall:
        DrawPath({ FVector2D(18.0f, 27.0f), FVector2D(18.0f, 19.0f), FVector2D(13.0f, 15.0f),
            FVector2D(12.0f, 10.0f), FVector2D(8.0f, 7.0f) }, false, 2.0f);
        DrawPath({ FVector2D(18.0f, 19.0f), FVector2D(23.0f, 15.0f), FVector2D(24.0f, 10.0f),
            FVector2D(28.0f, 7.0f) }, false, 2.0f);
        DrawPath({ FVector2D(12.0f, 10.0f), FVector2D(7.0f, 12.0f), FVector2D(5.0f, 17.0f) }, false, 1.7f);
        DrawPath({ FVector2D(24.0f, 10.0f), FVector2D(29.0f, 12.0f), FVector2D(31.0f, 17.0f) }, false, 1.7f);
        DrawPath({ FVector2D(13.0f, 15.0f), FVector2D(10.0f, 19.0f), FVector2D(9.0f, 23.0f) }, false, 1.7f);
        DrawPath({ FVector2D(23.0f, 15.0f), FVector2D(26.0f, 19.0f), FVector2D(27.0f, 23.0f) }, false, 1.7f);
        break;
    }
    return DrawLayer;
}

void UKalmalaInventoryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.04f, 0.9f));
    Border->SetPadding(FMargin(12));
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    SupportGlyphRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    const EKalmalaSupportGlyph GlyphKinds[] = { EKalmalaSupportGlyph::Mending, EKalmalaSupportGlyph::HearthShield,
        EKalmalaSupportGlyph::BearsVigor, EKalmalaSupportGlyph::DeerCall };
    const TCHAR* GlyphLabels[] = { TEXT("MEND"), TEXT("WARD"), TEXT("VIGOR"), TEXT("CALL") };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
        Card->SetBrushColor(FLinearColor(0.055f, 0.075f, 0.08f, 0.96f));
        Card->SetPadding(FMargin(3.0f));
        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>();
        USizeBox* GlyphBox = WidgetTree->ConstructWidget<USizeBox>();
        GlyphBox->SetWidthOverride(64.0f);
        GlyphBox->SetHeightOverride(42.0f);
        UKalmalaSupportGlyphWidget* GlyphWidget = WidgetTree->ConstructWidget<UKalmalaSupportGlyphWidget>();
        GlyphWidget->SetGlyphState(GlyphKinds[Index], false, false);
        GlyphBox->SetContent(GlyphWidget);
        CardContent->AddChild(GlyphBox);
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(GlyphLabels[Index]));
        Label->SetJustification(ETextJustify::Center);
        Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.87f, 0.84f, 1.0f)));
        Label->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 11));
        CardContent->AddChild(Label);
        Card->SetContent(CardContent);
        SupportGlyphRow->AddChildToHorizontalBox(Card)->SetPadding(FMargin(2.0f, 0.0f));
        SupportGlyphCards.Add(Card);
        SupportGlyphs.Add(GlyphWidget);
        SupportGlyphVisualStates.Add(0xff);
    }

    PackText = WidgetTree->ConstructWidget<UTextBlock>();
    PackText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    PackText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14));
    PackText->SetAutoWrapText(true);
    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Scroll->AddChild(PackText);
    Content->AddChildToVerticalBox(SupportGlyphRow)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
    Content->AddChildToVerticalBox(Scroll);
    Border->SetContent(Content);
    WidgetTree->RootWidget = Border;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaInventoryWidget::SetPackText(const FString& Text)
{
    if (PackText && PackText->GetText().ToString() != Text) PackText->SetText(FText::FromString(Text));
}

void UKalmalaInventoryWidget::SetSupportGlyphState(const int32 Index, const EKalmalaSupportGlyph Glyph, const bool bLearned, const bool bSelected)
{
    if (!SupportGlyphCards.IsValidIndex(Index) || !SupportGlyphs.IsValidIndex(Index) || !SupportGlyphVisualStates.IsValidIndex(Index)) return;
    const uint8 VisualState = static_cast<uint8>((bLearned ? 1 : 0) | (bSelected ? 2 : 0));
    if (SupportGlyphVisualStates[Index] != VisualState)
    {
        const FLinearColor CardColour = bSelected ? FLinearColor(0.26f, 0.17f, 0.08f, 0.98f)
            : bLearned ? FLinearColor(0.055f, 0.12f, 0.105f, 0.96f) : FLinearColor(0.055f, 0.075f, 0.08f, 0.96f);
        SupportGlyphCards[Index]->SetBrushColor(CardColour);
        SupportGlyphVisualStates[Index] = VisualState;
    }
    SupportGlyphs[Index]->SetGlyphState(Glyph, bLearned, bSelected);
}

void UKalmalaInventoryWidget::SetSupportGlyphsVisible(const bool bVisible)
{
    const ESlateVisibility DesiredVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    if (SupportGlyphRow && SupportGlyphRow->GetVisibility() != DesiredVisibility) SupportGlyphRow->SetVisibility(DesiredVisibility);
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
        Widget->SetDesiredSizeInViewport(FVector2D(340, 480));
        Widget->SetPositionInViewport(FVector2D(24, 24), false);
        Widget->AddToPlayerScreen(40);
    }
    APawn* Pawn = Controller->GetPawn();
    const UKalmalaInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UKalmalaInventoryComponent>() : nullptr;
    FString CraftKey = TEXT("Unbound");
    for (const auto& Mapping : GetDefault<UInputSettings>()->GetActionMappings())
        if (Mapping.ActionName == TEXT("CraftMenu") && !Mapping.Key.IsGamepadKey()) { CraftKey=Mapping.Key.GetDisplayName().ToString(); break; }
    const UKalmalaPlayerStatusComponent* Status = Pawn ? Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
    const UKalmalaCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UKalmalaCombatComponent>() : nullptr;
    const UKalmalaDiscoveryProgressComponent* Discovery = Pawn ? Pawn->FindComponentByClass<UKalmalaDiscoveryProgressComponent>() : nullptr;
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Pawn);
    const UKalmalaSupportMagicComponent* Support = Character ? Character->GetSupportMagicComponent() : nullptr;
    const UKalmalaCharacterMovementComponent* Movement = Pawn ? Cast<UKalmalaCharacterMovementComponent>(Pawn->GetMovementComponent()) : nullptr;
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
    if (Support && Character && Movement)
    {
        Widget->SetSupportGlyphsVisible(true);
        const FString MendingKey = FindInputKeyLabel(TEXT("SupportSelectMending"), false);
        const FString HearthKey = FindInputKeyLabel(TEXT("SupportSelectHearthShield"), false);
        const FString VigorKey = FindInputKeyLabel(TEXT("SupportSelectBearsVigor"), false);
        const FString DeerKey = FindInputKeyLabel(TEXT("SupportSelectDeerCall"), false);
        const FString MendingGamepadKey = FindInputKeyLabel(TEXT("SupportSelectMending"), true);
        const FString HearthGamepadKey = FindInputKeyLabel(TEXT("SupportSelectHearthShield"), true);
        const FString VigorGamepadKey = FindInputKeyLabel(TEXT("SupportSelectBearsVigor"), true);
        const FString DeerGamepadKey = FindInputKeyLabel(TEXT("SupportSelectDeerCall"), true);
        const FString ActivateKey = FindInputKeyLabel(TEXT("SupportActivate"), false);
        const float ServerNow = GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
        Text += FString::Printf(TEXT("SUPPORT | %s / %s / %s / %s select; %s use\n"), *MendingKey, *HearthKey, *VigorKey, *DeerKey, *ActivateKey);
        Text += FString::Printf(TEXT("Controller: D-pad select; %s use\n"), *FindInputKeyLabel(TEXT("SupportActivate"), true));
        const EKalmalaSupportEffect Effects[] = { EKalmalaSupportEffect::Mending, EKalmalaSupportEffect::HearthShield, EKalmalaSupportEffect::BearsVigor, EKalmalaSupportEffect::DeerCall };
        const FString Keys[] = { MendingKey, HearthKey, VigorKey, DeerKey };
        const FString GamepadKeys[] = { MendingGamepadKey, HearthGamepadKey, VigorGamepadKey, DeerGamepadKey };
        for (int32 Index = 0; Index < 4; ++Index)
        {
            const EKalmalaSupportEffect Effect = Effects[Index];
            const bool bLearned = Support->HasLearnedEffect(Effect);
            const bool bSelected = Character->GetSelectedSupportEffect() == Effect;
            Widget->SetSupportGlyphState(Index, static_cast<EKalmalaSupportGlyph>(Index), bLearned, bSelected);
            const TCHAR* Marker = Character->GetSelectedSupportEffect() == Effect ? TEXT(">") : TEXT(" ");
            Text += FString::Printf(TEXT("%s [K:%s | Pad:%s] %s — %s\n"), Marker, *Keys[Index], *GamepadKeys[Index], InventorySupportEffectName(Effect), bLearned ? TEXT("LEARNED") : TEXT("UNAVAILABLE"));
        }
        const float CooldownRemaining = FMath::Max(0.0f, Support->GetCooldownExpiry() - ServerNow);
        Text += FString::Printf(TEXT("Stamina: %.0f / %.0f | Cooldown: %s\n"), Movement->GetStamina(), Movement->GetMaximumStamina(), CooldownRemaining > 0.0f ? *FString::Printf(TEXT("%.1f s"), CooldownRemaining) : TEXT("READY"));
        if (Support->GetActiveEffect() != EKalmalaSupportEffect::None && Support->GetActiveEffectExpiry() > ServerNow)
            Text += FString::Printf(TEXT("Active: %s | Expiry: %.1f s\n"), InventorySupportEffectName(Support->GetActiveEffect()), Support->GetActiveEffectExpiry() - ServerNow);
        else
            Text += TEXT("Active: none\n");
        if (Support->GetFeedbackSerial() == 0) Text += TEXT("Server result: none yet\n\n");
        else Text += FString::Printf(TEXT("Server result: %s\n\n"), Support->GetFeedback() == EKalmalaSupportFeedback::Accepted ? TEXT("ACCEPTED") : TEXT("UNAVAILABLE"));
    }
    else
    {
        Widget->SetSupportGlyphsVisible(false);
    }
    int32 StatusLines = 0;
    for (const TCHAR CurrentChar : Text) if (CurrentChar == TEXT('\n')) ++StatusLines;
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
    Widget->SetDesiredSizeInViewport(FVector2D(340, 60 + (Support && Character && Movement ? 68 : 0)
        + (StatusLines + (Inventory ? FMath::Max(1, Inventory->GetStacks().Num()) : 1)) * 22));
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
