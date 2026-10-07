#include "KalmalaSupportSelectionSubsystem.h"

#include "KalmalaSupportMagicComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaSettingsWidget.h"
#include "KalmalaUITheme.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
const EKalmalaSupportGlyph GlyphKinds[] = {
    EKalmalaSupportGlyph::Mending, EKalmalaSupportGlyph::HearthShield,
    EKalmalaSupportGlyph::BearsVigor, EKalmalaSupportGlyph::DeerCall
};
const EKalmalaSupportEffect Effects[] = {
    EKalmalaSupportEffect::Mending, EKalmalaSupportEffect::HearthShield,
    EKalmalaSupportEffect::BearsVigor, EKalmalaSupportEffect::DeerCall
};
const TCHAR* GlyphLabels[] = { TEXT("MEND"), TEXT("WARD"), TEXT("VIGOR"), TEXT("CALL") };

const TCHAR* EffectName(const int32 Index)
{
    switch (Index)
    {
    case 0: return TEXT("Mending");
    case 1: return TEXT("Hearth Shield");
    case 2: return TEXT("Bear's Vigor");
    case 3: return TEXT("Deer Call");
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

void UKalmalaSupportGlyphWidget::SetGlyphState(
    const EKalmalaSupportGlyph InGlyph, const bool bInLearned, const bool bInSelected)
{
    if (Glyph == InGlyph && bLearned == bInLearned && bSelected == bInSelected) return;
    Glyph = InGlyph;
    bLearned = bInLearned;
    bSelected = bInSelected;
    Invalidate(EInvalidateWidget::Paint);
}

void UKalmalaSupportGlyphWidget::SetGlyphContrast(const int32 ContrastMode)
{
    const int32 Bounded = UKalmalaSettingsWidget::ClampContrastMode(ContrastMode);
    if (Contrast == Bounded) return;
    Contrast = Bounded;
    Invalidate(EInvalidateWidget::Paint);
}

int32 UKalmalaSupportGlyphWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, const int32 LayerId,
    const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
        OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const float Scale = FMath::Min(Size.X, Size.Y) / 36.0f;
    if (Scale <= 0.0f) return DrawLayer;

    const FVector2D Centre = Size * 0.5f;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    const FLinearColor Ink = Contrast != 0 ? FLinearColor::White : Theme.TextColor(bSelected || !bLearned, 0);
    const auto ToLocal = [Centre, Scale](const FVector2D Point)
    {
        return Centre + (Point - FVector2D(18.0f, 18.0f)) * Scale;
    };
    const auto DrawPath = [&OutDrawElements, &AllottedGeometry, DrawLayer, Ink, &ToLocal]
        (const TArray<FVector2D>& Source, const bool bClosed, const float Thickness)
    {
        if (Source.Num() < 2) return;
        TArray<FVector2D> Points;
        Points.Reserve(Source.Num() + (bClosed ? 1 : 0));
        for (const FVector2D Point : Source) Points.Add(ToLocal(Point));
        if (bClosed) Points.Add(Points[0]);
        FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Ink, true, Thickness);
    };
    const auto DrawCircle = [&DrawPath](const FVector2D CircleCentre, const float Radius,
        const int32 Segments, const float Thickness)
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
        DrawDot(FVector2D(9.0f, 11.0f), 2.3f); DrawDot(FVector2D(14.0f, 7.0f), 2.3f);
        DrawDot(FVector2D(21.0f, 7.0f), 2.3f); DrawDot(FVector2D(27.0f, 11.0f), 2.3f);
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

void UKalmalaSupportSelectionWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    SetIsFocusable(false);
    Background = WidgetTree->ConstructWidget<UBorder>();
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    Theme.ApplyPanel(*Background, UKalmalaSettingsWidget::GetContrastMode());
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Heading = WidgetTree->ConstructWidget<UTextBlock>();
    Heading->SetText(FText::FromString(TEXT("SUPPORT")));
    Content->AddChildToVerticalBox(Heading);
    GlyphRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Effects); ++Index)
    {
        UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
        Card->SetPadding(FMargin(Theme.SlotPadding));
        UVerticalBox* CardContent = WidgetTree->ConstructWidget<UVerticalBox>();
        USizeBox* GlyphBox = WidgetTree->ConstructWidget<USizeBox>();
        Theme.ApplyIconSlot(*GlyphBox);
        UKalmalaSupportGlyphWidget* Glyph = WidgetTree->ConstructWidget<UKalmalaSupportGlyphWidget>();
        Glyph->SetGlyphState(GlyphKinds[Index], false, false);
        GlyphBox->SetContent(Glyph);
        CardContent->AddChild(GlyphBox);
        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(GlyphLabels[Index]));
        Label->SetJustification(ETextJustify::Center);
        Label->SetAutoWrapText(false);
        CardContent->AddChild(Label);
        Card->SetContent(CardContent);
        GlyphRow->AddChildToHorizontalBox(Card)->SetPadding(FMargin(2.0f, 0.0f));
        Cards.Add(Card);
        Glyphs.Add(Glyph);
        Labels.Add(Label);
        VisualStates.Add(0xff);
    }
    Content->AddChildToVerticalBox(GlyphRow)->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 2.0f));
    SelectedText = WidgetTree->ConstructWidget<UTextBlock>();
    SelectedText->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(SelectedText);
    Background->SetContent(Content);
    WidgetTree->RootWidget = Background;
    ApplyAccessibility(UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

FString UKalmalaSupportSelectionWidget::BuildSelectedText(const int32 SelectedIndex, const bool bLearned)
{
    if (SelectedIndex < 0 || SelectedIndex >= UE_ARRAY_COUNT(Effects)) return TEXT("No support selected");
    return FString::Printf(TEXT("Selected: %s · %s"), EffectName(SelectedIndex), bLearned ? TEXT("learned") : TEXT("not learned"));
}

FString UKalmalaSupportSelectionWidget::GetSelectionSummary() const
{
    return SelectedText != nullptr ? SelectedText->GetText().ToString() : FString();
}

void UKalmalaSupportSelectionWidget::ApplyAccessibility(const int32 TextScale, const int32 Contrast)
{
    const int32 BoundedScale = UKalmalaSettingsWidget::ClampTextScale(TextScale);
    const int32 BoundedContrast = UKalmalaSettingsWidget::ClampContrastMode(Contrast);
    if (BoundedScale == LastTextScale && BoundedContrast == LastContrast) return;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    if (Background) Theme.ApplyPanel(*Background, BoundedContrast);
    if (Heading) Theme.ApplyText(*Heading, Theme.HeadingSize, true, BoundedScale, BoundedContrast);
    if (SelectedText) Theme.ApplyText(*SelectedText, Theme.BodySize, false, BoundedScale, BoundedContrast);
    for (UTextBlock* Label : Labels)
        if (Label) Theme.ApplyText(*Label, Theme.BodySize - 2, true, BoundedScale, BoundedContrast);
    for (UKalmalaSupportGlyphWidget* Glyph : Glyphs)
        if (Glyph) Glyph->SetGlyphContrast(BoundedContrast);
    LastTextScale = BoundedScale;
    LastContrast = BoundedContrast;
}

void UKalmalaSupportSelectionWidget::SetSnapshot(
    const int32 SelectedIndex, const uint8 LearnedMask, const int32 TextScale, const int32 Contrast)
{
    const int32 BoundedContrast = UKalmalaSettingsWidget::ClampContrastMode(Contrast);
    ApplyAccessibility(TextScale, BoundedContrast);
    for (int32 Index = 0; Index < Cards.Num(); ++Index)
    {
        const bool bLearned = (LearnedMask & (1u << Index)) != 0;
        const bool bSelected = Index == SelectedIndex;
        const uint8 State = static_cast<uint8>((bLearned ? 1 : 0) | (bSelected ? 2 : 0));
        if (VisualStates[Index] != State)
        {
            FKalmalaUITheme::Get().ApplySelectablePanel(*Cards[Index], bSelected, false, !bLearned, BoundedContrast);
            VisualStates[Index] = State;
        }
        Glyphs[Index]->SetGlyphState(GlyphKinds[Index], bLearned, bSelected);
    }
    if (SelectedText)
        SelectedText->SetText(FText::FromString(BuildSelectedText(
            SelectedIndex, SelectedIndex >= 0 && SelectedIndex < 4 && (LearnedMask & (1u << SelectedIndex)) != 0)));
}

void UKalmalaSupportSelectionSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    if (World == nullptr || !World->IsGameWorld() || LocalPlayer == nullptr) return;
    APlayerController* FoundController = LocalPlayer->GetPlayerController(World);
    if (Controller.Get() != FoundController)
    {
        ReleaseWidget();
        Controller = FoundController;
    }
    if (FoundController == nullptr || !FoundController->IsLocalController()) return;

    if (Widget == nullptr)
    {
        Widget = CreateWidget<UKalmalaSupportSelectionWidget>(FoundController,
            UKalmalaSupportSelectionWidget::StaticClass());
        if (Widget == nullptr) return;
        Widget->SetAlignmentInViewport(FVector2D(0.5f, 0.0f));
        Widget->AddToPlayerScreen(56);
    }

    APawn* Pawn = FoundController->GetPawn();
    const AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Pawn);
    const UKalmalaSupportMagicComponent* Support = Character != nullptr ? Character->GetSupportMagicComponent() : nullptr;
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(Widget)
        / FMath::Max(0.1f, UWidgetLayoutLibrary::GetViewportScale(Widget));
    if (ViewportSize.X < 640.0f || ViewportSize.Y < 360.0f || FoundController->IsMoveInputIgnored()
        || Character == nullptr || Support == nullptr)
    {
        Widget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    constexpr float PanelWidth = 264.0f;
    constexpr float PanelHeight = 112.0f;
    if (!ViewportSize.Equals(LastViewportSize, 0.5f))
    {
        Widget->SetDesiredSizeInViewport(FVector2D(PanelWidth, PanelHeight));
        Widget->SetPositionInViewport(FVector2D(ViewportSize.X * 0.5f, 24.0f), false);
        LastViewportSize = ViewportSize;
    }

    int32 SelectedIndex = static_cast<int32>(Character->GetSelectedSupportEffect()) - 1;
    if (SelectedIndex < 0 || SelectedIndex >= UE_ARRAY_COUNT(Effects)) SelectedIndex = INDEX_NONE;
    uint8 LearnedMask = 0;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Effects); ++Index)
        if (Support->HasLearnedEffect(Effects[Index])) LearnedMask |= static_cast<uint8>(1u << Index);
    Widget->SetSnapshot(SelectedIndex, LearnedMask,
        UKalmalaSettingsWidget::GetTextScalePercent(), UKalmalaSettingsWidget::GetContrastMode());
    Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UKalmalaSupportSelectionSubsystem::ReleaseWidget()
{
    if (Widget) Widget->RemoveFromParent();
    Widget = nullptr;
    LastViewportSize = FVector2D::ZeroVector;
}

void UKalmalaSupportSelectionSubsystem::Deinitialize()
{
    ReleaseWidget();
    Controller = nullptr;
    Super::Deinitialize();
}
