#include "KalmalaWorldMapWidget.h"
#include "KalmalaUITheme.h"
#include "KalmalaSettingsWidget.h"

#include "Async/Async.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaRegionalGeneration.h"
#include "HAL/IConsoleManager.h"

#include "KalmalaMinimapViewModel.h"
#include "KalmalaMapAwarenessComponent.h"
#include "KalmalaWorldMapExplorationSaveGame.h"
#include "KalmalaWorldMapPinsSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Rendering/DrawElements.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

namespace
{
    TAutoConsoleVariable<int32> RevealWorldMap(TEXT("kalmala.Map.RevealAll"), 1, TEXT("Debug: show all generated terrain without changing exploration."));
    TAutoConsoleVariable<int32> FitWorldMap(TEXT("kalmala.Map.FitWorldOnOpen"), 1, TEXT("Debug: open M at world origin and fit the full 16 km radius."));

    float DrawMapLabel(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry,
        FVector2D Position, const FString& Text, int32 SizeOffset, bool bHeading = false)
    {
        const auto& Theme = FKalmalaUITheme::Get();
        const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
        const auto Font = Theme.MakeFont(Theme.BodySize + SizeOffset, false, UKalmalaSettingsWidget::GetTextScalePercent());
        const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        Position.X = FMath::Clamp(Position.X, 20.0, FMath::Max(20.0, Geometry.GetLocalSize().X - 200.0));
        const float Width = FMath::Max(100.0, Geometry.GetLocalSize().X - Position.X - 20.0);
        TArray<FString> Words;
        Text.ParseIntoArrayWS(Words);
        FString Wrapped, Line;
        for (const FString& Word : Words)
        {
            const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
            if (!Line.IsEmpty() && Measure->Measure(Candidate, Font).X > Width)
            {
                Wrapped += Line + TEXT("\n");
                Line = Word;
            }
            else Line = Candidate;
        }
        Wrapped += Line;
        const FVector2D TextSize = Measure->Measure(Wrapped, Font);
        const float Y = FMath::Max(0.0, FMath::Min(Position.Y, Geometry.GetLocalSize().Y - TextSize.Y - 4.0));
        // Controls stay readable over terrain, independent of the selected theme image.
        FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(FVector2D(Width + 8, TextSize.Y + 4),
            FSlateLayoutTransform(FVector2D(Position.X - 4, Y - 2))), FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None, Contrast != 0 ? Theme.HighContrastPanel : Theme.Panel);
        FSlateDrawElement::MakeText(Elements, Layer + 1, Geometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(Position.X, Y))),
            Wrapped, Font, ESlateDrawEffect::None, Theme.TextColor(bHeading, Contrast));
        return Y + TextSize.Y + 6;
    }
}

void UKalmalaWorldMapWidget::InitializeForLocalPlayer(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr || !InOwningPlayer->IsLocalController()) return;
    SetOwningPlayer(InOwningPlayer);
    ViewModel = NewObject<UKalmalaMinimapViewModel>(this);
    ViewModel->Initialize(InOwningPlayer);
    ViewModel->SetMapRadius(MapZoom);
    ViewModel->SetMapSampleDimensions(FIntPoint(161, 91));
    SetIsFocusable(true);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaWorldMapWidget::ConfigureViewportPlacement()
{
    SetAlignmentInViewport(FVector2D::ZeroVector);
    SetPositionInViewport(FVector2D::ZeroVector, false);
    // With stretch anchors, desired size becomes right/bottom margins. A fixed
    // resolution here subtracts from the viewport and collapses the map.
    SetDesiredSizeInViewport(FVector2D::ZeroVector);
    SetAnchorsInViewport(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
}

void UKalmalaWorldMapWidget::Open()
{
    if (GetOwningPlayer() == nullptr || ViewModel == nullptr) return;
    bMapOpen = true;
    bMarkerFilterNavigationActive = false;
    HoveredMarkerCategoryIndex = INDEX_NONE;
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapProfile")))
    {
        DeveloperProfileOpenedAt = FPlatformTime::Seconds();
        DeveloperProfileWorkerSeconds = 0.0;
        DeveloperProfileMaxWorkerSeconds = 0.0;
        DeveloperProfileGameThreadSeconds = 0.0;
        DeveloperProfileMaxGameThreadSeconds = 0.0;
        DeveloperProfileGameThreadTicks = 0;
        bDeveloperProfileLogged = false;
    }
    if (!bHasOpenedMap)
    {
        // Preserve the established first-open default; later opens retain the
        // owner's transient centre, zoom and whole-world view mode.
        Recenter();
        bFitWholeWorld = FitWorldMap.GetValueOnGameThread() != 0;
        bHasOpenedMap = true;
    }
    SetVisibility(ESlateVisibility::Visible);
    APlayerController* Controller = GetOwningPlayer();
    Controller->SetShowMouseCursor(true);
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(TakeWidget());
    Controller->SetInputMode(InputMode);
    Controller->SetIgnoreMoveInput(true);
    Controller->SetIgnoreLookInput(true);
}

void UKalmalaWorldMapWidget::Close()
{
    if (!bMapOpen) return;
    bMapOpen = false;
    bDragging = false;
    bMarkerFilterNavigationActive = false;
    HoveredMarkerCategoryIndex = INDEX_NONE;
    SetVisibility(ESlateVisibility::Collapsed);
    if (APlayerController* Controller = GetOwningPlayer())
    {
        Controller->SetShowMouseCursor(false);
        Controller->SetInputMode(FInputModeGameOnly());
        Controller->SetIgnoreMoveInput(false);
        Controller->SetIgnoreLookInput(false);
    }
}

void UKalmalaWorldMapWidget::Recenter()
{
    if (ViewModel == nullptr) return;
    bFitWholeWorld = false;
    ViewModel->RecenterOnOwningPlayer();
    ViewModel->SetMapRadius(MapZoom);
    InvalidateOutstandingTileJobs();
}

void UKalmalaWorldMapWidget::RunDeveloperVerification()
{
    if (!bMapOpen || ViewModel == nullptr || bDeveloperVerificationLogged) return;
    Recenter();
    const FVector2D InitialCentre = ViewModel->GetMapCentre();
    ZoomAtScreenPosition(1000.0f, FVector2D(400.0f, 300.0f), FVector2D(800.0f, 600.0f));
    const bool bMinimumZoom = FMath::IsNearlyEqual(MapZoom, MinZoom);
    ZoomAtScreenPosition(-100000.0f, FVector2D(400.0f, 300.0f), FVector2D(800.0f, 600.0f));
    const bool bMaximumZoom = FMath::IsNearlyEqual(MapZoom, MaxZoom);
    PanByScreenDelta(FVector2D(160.0f, -120.0f), FVector2D(800.0f, 600.0f));
    const bool bPanChangedCentre = !ViewModel->GetMapCentre().Equals(InitialCentre, 1.0f);
    Recenter();
    const bool bRecentered = ViewModel->GetMapCentre().Equals(InitialCentre, 1.0f);
    MapZoom = 18000.0f;
    Recenter();
    bFitWholeWorld = FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldOverviewVerification"));
    bDeveloperVerificationLogged = true;
    const uint8 OriginalVisibilityMask = MarkerVisibilityMask;
    const int32 OriginalPinCount = LocalPins.Num();
    const int32 OriginalSelectedPin = SelectedPinIndex;
    MarkerVisibilityMask = 0x07;
    FocusedMarkerCategoryIndex = 0;

    bool bPointerTargets = true;
    const FVector2D ReviewSize(1280.0f, 720.0f);
    const FMarkerLegendLayout Legend = GetMarkerLegendLayout(ReviewSize);
    const float FirstFilterRowY = Legend.Position.Y + Legend.Padding + Legend.HeaderHeight + Legend.RowHeight * 1.5f;
    for (int32 Category = 0; Category < static_cast<int32>(EKalmalaWorldMapMarkerCategory::Count); ++Category)
    {
        const FVector2D RowPoint(Legend.Position.X + 18.0f, FirstFilterRowY + Category * Legend.RowHeight);
        bPointerTargets &= GetMarkerCategoryAtPosition(RowPoint, ReviewSize) == Category;
        const EKalmalaWorldMapMarkerCategory MarkerCategory = MarkerCategoryFromIndex(Category);
        ToggleMarkerCategory(MarkerCategory);
        bPointerTargets &= !IsMarkerCategoryVisible(MarkerCategory);
        ToggleMarkerCategory(MarkerCategory);
    }

    const bool bKeyboardFocus = HandleMarkerFilterKey(EKeys::F)
        && HandleMarkerFilterKey(EKeys::Down)
        && HandleMarkerFilterKey(EKeys::Enter)
        && !IsMarkerCategoryVisible(EKalmalaWorldMapMarkerCategory::CoopPlayers)
        && HandleMarkerFilterKey(EKeys::Escape);
    MarkerVisibilityMask = 0x07;
    FocusedMarkerCategoryIndex = 1;
    const bool bControllerFocus = HandleMarkerFilterKey(EKeys::Gamepad_LeftThumbstick)
        && HandleMarkerFilterKey(EKeys::Gamepad_DPad_Down)
        && HandleMarkerFilterKey(EKeys::Gamepad_FaceButton_Bottom)
        && !IsMarkerCategoryVisible(EKalmalaWorldMapMarkerCategory::CoopPings)
        && HandleMarkerFilterKey(EKeys::Gamepad_FaceButton_Right);
    const bool bPinsPreserved = LocalPins.Num() == OriginalPinCount && SelectedPinIndex == OriginalSelectedPin;
    MarkerVisibilityMask = OriginalVisibilityMask;
    bMarkerFilterNavigationActive = true;
    FocusedMarkerCategoryIndex = 0;
    UE_LOG(LogTemp, Display, TEXT("World map marker filters: PointerTargets=%d Keyboard=%d Controller=%d PinsPreserved=%d."),
        bPointerTargets ? 1 : 0, bKeyboardFocus ? 1 : 0, bControllerFocus ? 1 : 0, bPinsPreserved ? 1 : 0);
    UE_LOG(LogTemp, Display, TEXT("World map verification: Open=%d Input=%d ZoomMin=%d ZoomMax=%d Pan=%d Recenter=%d."),
        bMapOpen ? 1 : 0, GetOwningPlayer() && GetOwningPlayer()->IsMoveInputIgnored() && GetOwningPlayer()->IsLookInputIgnored() ? 1 : 0,
        bMinimumZoom ? 1 : 0, bMaximumZoom ? 1 : 0, bPanChangedCentre ? 1 : 0, bRecentered ? 1 : 0);
}

float UKalmalaWorldMapWidget::FullWorldZoom(float AspectRatio)
{
    return FKalmalaWorldBounds::Radius * 1.04 / FMath::Min(1.0f, FMath::Clamp(AspectRatio, 0.5f, 3.0f));
}

float UKalmalaWorldMapWidget::ChooseTileWorldSize(FVector2D Extent)
{
    float Size = TileWorldSize;
    // At most eight tiles along the longer axis, including both edge tiles.
    while (Size * 6 < FMath::Max(Extent.X, Extent.Y) * 2) Size *= 2;
    return Size;
}

float UKalmalaWorldMapWidget::ClampMapZoom(const float RequestedZoom, const float InMinZoom, const float InMaxZoom)
{
    return FMath::Clamp(RequestedZoom, FMath::Min(InMinZoom, InMaxZoom), FMath::Max(InMinZoom, InMaxZoom));
}

bool UKalmalaWorldMapWidget::IsWithinLocalRevealRadius(const FVector2D WorldPosition, const FVector2D OwningPawnLocation, const float RevealRadius)
{
    return RevealRadius > 0.0f && FMath::IsFinite(WorldPosition.X) && FMath::IsFinite(WorldPosition.Y)
        && FMath::IsFinite(OwningPawnLocation.X) && FMath::IsFinite(OwningPawnLocation.Y)
        && FVector2D::DistSquared(WorldPosition, OwningPawnLocation) <= FMath::Square(RevealRadius);
}

bool UKalmalaWorldMapWidget::CanShowCoopLocation(FVector2D WorldPosition, FVector2D OwningPawnLocation,
    const UKalmalaWorldMapExplorationSaveGame* Exploration)
{
    if (!FMath::IsFinite(WorldPosition.X) || !FMath::IsFinite(WorldPosition.Y)) return false;
    return IsWithinLocalRevealRadius(WorldPosition, OwningPawnLocation, LocalRevealRadius)
        || (Exploration && Exploration->IsExplored(WorldPosition));
}

void UKalmalaWorldMapWidget::SendMapPing(FVector2D Location)
{
    auto* Awareness = GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UKalmalaMapAwarenessComponent>() : nullptr;
    FVector2D PawnLocation;
    FKalmalaWorldGenerationConfig Config;
    if (!Awareness || !Awareness->IsSharingEnabled()) { PingFeedback = TEXT("Enable co-op sharing first."); return; }
    if (!ViewModel || !ViewModel->GetPresentationInputs(Config, PawnLocation)
        || !UKalmalaMapAwarenessComponent::IsLocationInRange(Location, PawnLocation))
    {
        PingFeedback = TEXT("Ping must be within 65m of you.");
        return;
    }
    Awareness->RequestPing(Location);
    PingFeedback = TEXT("Ping requested (6s lifetime; 2s cooldown; nearby opted-in peers only).");
}

UKalmalaWorldMapWidget::FMarkerLegendLayout UKalmalaWorldMapWidget::GetMarkerLegendLayout(const FVector2D& WidgetSize) const
{
    FMarkerLegendLayout Layout;
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    const FSlateFontInfo Font = Theme.MakeFont(Theme.BodySize - 1, false, UKalmalaSettingsWidget::GetTextScalePercent());
    const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    const float TextHeight = Measure->Measure(TEXT("Ag"), Font).Y;
    const float LongestRowWidth = Measure->Measure(TEXT("Co-op players 256 [FILTERED]"), Font).X;
    const float TitleWidth = Measure->Measure(TEXT("MAP SYMBOLS"), Theme.MakeFont(Theme.BodySize, true,
        UKalmalaSettingsWidget::GetTextScalePercent())).X;
    Layout.HeaderHeight = TextHeight + 9.0f;
    Layout.RowHeight = FMath::Max(28.0f, TextHeight + 10.0f);
    const float Width = FMath::Max(220.0f, FMath::Max(TitleWidth, LongestRowWidth + 56.0f + Layout.Padding * 2.0f));
    Layout.Size.X = FMath::Min(Width, FMath::Max(180.0f, WidgetSize.X - 24.0f));
    Layout.Size.Y = Layout.Padding * 2.0f + Layout.HeaderHeight + Layout.RowHeight * 4.0f;
    Layout.Position.X = FMath::Max(12.0f, WidgetSize.X - Layout.Size.X - 18.0f);
    Layout.Position.Y = FMath::Clamp(72.0f, 12.0f, FMath::Max(12.0f, WidgetSize.Y - Layout.Size.Y - 12.0f));
    return Layout;
}

int32 UKalmalaWorldMapWidget::GetMarkerCategoryAtPosition(const FVector2D& WidgetPosition, const FVector2D& WidgetSize) const
{
    const FMarkerLegendLayout Layout = GetMarkerLegendLayout(WidgetSize);
    const FVector2D Local = WidgetPosition - Layout.Position;
    const float CategoryStart = Layout.Padding + Layout.HeaderHeight + Layout.RowHeight;
    if (Local.X < Layout.Padding || Local.X > Layout.Size.X - Layout.Padding || Local.Y < CategoryStart) return INDEX_NONE;
    const int32 Index = FMath::FloorToInt((Local.Y - CategoryStart) / Layout.RowHeight);
    return Index >= 0 && Index < static_cast<int32>(EKalmalaWorldMapMarkerCategory::Count) ? Index : INDEX_NONE;
}

bool UKalmalaWorldMapWidget::IsInsideMarkerLegend(const FVector2D& WidgetPosition, const FVector2D& WidgetSize) const
{
    const FMarkerLegendLayout Layout = GetMarkerLegendLayout(WidgetSize);
    return WidgetPosition.X >= Layout.Position.X && WidgetPosition.X <= Layout.Position.X + Layout.Size.X
        && WidgetPosition.Y >= Layout.Position.Y && WidgetPosition.Y <= Layout.Position.Y + Layout.Size.Y;
}

EKalmalaWorldMapMarkerCategory UKalmalaWorldMapWidget::MarkerCategoryFromIndex(const int32 Index)
{
    return static_cast<EKalmalaWorldMapMarkerCategory>(FMath::Clamp(Index, 0, static_cast<int32>(EKalmalaWorldMapMarkerCategory::Count) - 1));
}

bool UKalmalaWorldMapWidget::IsMarkerCategoryVisible(const EKalmalaWorldMapMarkerCategory Category) const
{
    const uint8 CategoryIndex = static_cast<uint8>(Category);
    return CategoryIndex < static_cast<uint8>(EKalmalaWorldMapMarkerCategory::Count)
        && (MarkerVisibilityMask & (1u << CategoryIndex)) != 0;
}

void UKalmalaWorldMapWidget::ToggleMarkerCategory(const EKalmalaWorldMapMarkerCategory Category)
{
    const uint8 CategoryIndex = static_cast<uint8>(Category);
    if (CategoryIndex < static_cast<uint8>(EKalmalaWorldMapMarkerCategory::Count))
    {
        MarkerVisibilityMask ^= static_cast<uint8>(1u << CategoryIndex);
        Invalidate(EInvalidateWidget::Paint);
    }
}

bool UKalmalaWorldMapWidget::HandleMarkerFilterKey(const FKey& Key)
{
    if (!bMarkerFilterNavigationActive)
    {
        if (Key == EKeys::F || Key == EKeys::Gamepad_LeftThumbstick)
        {
            bMarkerFilterNavigationActive = true;
            FocusedMarkerCategoryIndex = FMath::Clamp(FocusedMarkerCategoryIndex, 0,
                static_cast<int32>(EKalmalaWorldMapMarkerCategory::Count) - 1);
            Invalidate(EInvalidateWidget::Paint);
            return true;
        }
        return false;
    }

    const int32 CategoryCount = static_cast<int32>(EKalmalaWorldMapMarkerCategory::Count);
    if (Key == EKeys::F || Key == EKeys::Gamepad_LeftThumbstick)
    {
        bMarkerFilterNavigationActive = false;
        Invalidate(EInvalidateWidget::Paint);
        return true;
    }
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
    {
        bMarkerFilterNavigationActive = false;
        Invalidate(EInvalidateWidget::Paint);
        return true;
    }
    if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up)
    {
        FocusedMarkerCategoryIndex = (FocusedMarkerCategoryIndex + CategoryCount - 1) % CategoryCount;
        Invalidate(EInvalidateWidget::Paint);
        return true;
    }
    if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down)
    {
        FocusedMarkerCategoryIndex = (FocusedMarkerCategoryIndex + 1) % CategoryCount;
        Invalidate(EInvalidateWidget::Paint);
        return true;
    }
    if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        ToggleMarkerCategory(MarkerCategoryFromIndex(FocusedMarkerCategoryIndex));
        return true;
    }
    return false;
}

bool UKalmalaWorldMapWidget::IsPersonalPinOnCurrentView(const FKalmalaWorldMapPersonalPin& Pin) const
{
    if (!Pin.bVisible || ViewModel == nullptr) return false;
    const FVector2D Normalized = WorldToMapNormalized(Pin.WorldLocation, ViewModel->GetMapCentre(), ViewModel->GetMapExtent());
    return Normalized.X >= 0.0f && Normalized.X <= 1.0f && Normalized.Y >= 0.0f && Normalized.Y <= 1.0f;
}

int32 UKalmalaWorldMapWidget::CountEligiblePersonalPins() const
{
    int32 Count = 0;
    for (const FKalmalaWorldMapPersonalPin& Pin : LocalPins) if (IsPersonalPinOnCurrentView(Pin)) ++Count;
    return Count;
}

UKalmalaWorldMapWidget::FMarkerCounts UKalmalaWorldMapWidget::DrawCoopAwareness(const FGeometry& Geometry, FVector2D MapSize, int32 LayerId,
    FSlateWindowElementList& Elements) const
{
    FMarkerCounts Counts;
    const auto* Awareness = GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UKalmalaMapAwarenessComponent>() : nullptr;
    if (!Awareness) return Counts;
    const float FooterY = Geometry.GetLocalSize().Y - FKalmalaUITheme::Get().ScaledFontSize(
        FKalmalaUITheme::Get().BodySize - 1, UKalmalaSettingsWidget::GetTextScalePercent()) * 5 - 30;
    const float NextY = DrawMapLabel(Elements, LayerId, Geometry, FVector2D(20, FooterY), Awareness->GetStatusText(), -1);
    const float FeedbackY = DrawMapLabel(Elements, LayerId, Geometry, FVector2D(20, NextY),
        TEXT("Share map · Ping · Ping centre"), -1);
    if (!PingFeedback.IsEmpty()) DrawMapLabel(Elements, LayerId, Geometry, FVector2D(20, FeedbackY), PingFeedback, -1);
    FVector2D Here;
    FKalmalaWorldGenerationConfig Config;
    if (!ViewModel || !ViewModel->GetPresentationInputs(Config, Here)) return Counts;
    const auto* Coverage = ExplorationSave && ExplorationSave->MatchesWorld(Config) ? ExplorationSave.Get() : nullptr;
    Elements.PushClip(FSlateClippingZone(Geometry.ToPaintGeometry(MapSize, FSlateLayoutTransform(FVector2D(44)))));
    const auto DrawMarker = [&](FVector2D Location, const FString& Text, const bool bPing)
    {
        if (!CanShowCoopLocation(Location, Here, Coverage)) return false;
        const FVector2D N = WorldToMapNormalized(Location, ViewModel->GetMapCentre(), ViewModel->GetMapExtent());
        if (N.X < 0 || N.X > 1 || N.Y < 0 || N.Y > 1) return false;
        if (bPing) ++Counts.CoopPings;
        else ++Counts.CoopPlayers;
        if (!IsMarkerCategoryVisible(bPing ? EKalmalaWorldMapMarkerCategory::CoopPings : EKalmalaWorldMapMarkerCategory::CoopPlayers)) return true;
        const FVector2D P = FVector2D(44) + N * MapSize;
        const TArray<FVector2D> Shape = bPing
            ? TArray<FVector2D>{P + FVector2D(-8, -8), P + FVector2D(8, 8), P, P + FVector2D(-8, 8), P + FVector2D(8, -8)}
            : TArray<FVector2D>{P + FVector2D(0, -9), P + FVector2D(9, 0), P + FVector2D(0, 9), P + FVector2D(-9, 0), P + FVector2D(0, -9)};
        FSlateDrawElement::MakeLines(Elements, LayerId, Geometry.ToPaintGeometry(), Shape, ESlateDrawEffect::None,
            bPing ? FLinearColor(1, 0.75f, 0.4f) : FLinearColor(0.7f, 0.9f, 1), true, 2.5f);
        FSlateDrawElement::MakeText(Elements, LayerId, Geometry.ToPaintGeometry(FSlateLayoutTransform(P + FVector2D(12, bPing ? 14 : -14))),
            Text, FKalmalaUITheme::Get().MakeFont(FKalmalaUITheme::Get().BodySize - 1, false, UKalmalaSettingsWidget::GetTextScalePercent()), ESlateDrawEffect::None, FLinearColor::White);
        return true;
    };
    for (const auto& Peer : Awareness->GetPeerMarkers()) DrawMarker(Peer.Location, FString::Printf(TEXT("Peer %d"), Peer.PlayerId), false);
    for (const auto& Ping : Awareness->GetVisiblePings()) DrawMarker(Ping.Location, FString::Printf(TEXT("Ping %d:%u (temporary)"), Ping.SenderId, Ping.Sequence), true);
    Elements.PopClip();
    return Counts;
}

void UKalmalaWorldMapWidget::DrawMarkerLegend(const FGeometry& Geometry, const int32 LayerId, const FMarkerCounts& Counts,
    FSlateWindowElementList& Elements) const
{
    const FMarkerLegendLayout Layout = GetMarkerLegendLayout(Geometry.GetLocalSize());
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
    const bool bHighContrast = Contrast != 0;
    const FSlateFontInfo Font = Theme.MakeFont(Theme.BodySize - 1, false, UKalmalaSettingsWidget::GetTextScalePercent());
    const FSlateFontInfo SmallFont = Theme.MakeFont(Theme.BodySize - 2, false, UKalmalaSettingsWidget::GetTextScalePercent());
    const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    const float TextHeight = Measure->Measure(TEXT("Ag"), Font).Y;
    const FLinearColor TextColour = Theme.TextColor(false, Contrast);
    const FLinearColor FocusColour = bHighContrast ? FLinearColor::White : Theme.ButtonFocused;
    const FLinearColor MapPanelColour = bHighContrast ? Theme.HighContrastPanel : Theme.Panel;
    const FVector2D Position = Layout.Position;
    const FVector2D PanelSize = Layout.Size;
    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
    FSlateDrawElement::MakeBox(Elements, LayerId,
        Geometry.ToPaintGeometry(PanelSize, FSlateLayoutTransform(Position)), WhiteBrush, ESlateDrawEffect::None, MapPanelColour);
    const TArray<FVector2D> PanelOutline = { Position, Position + FVector2D(PanelSize.X, 0.0f), Position + PanelSize,
        Position + FVector2D(0.0f, PanelSize.Y), Position };
    FSlateDrawElement::MakeLines(Elements, LayerId + 1, Geometry.ToPaintGeometry(), PanelOutline,
        ESlateDrawEffect::None, bHighContrast ? FLinearColor::White : Theme.BorderColor, true, bHighContrast ? 2.0f : 1.0f);

    const float TextX = Position.X + Layout.Padding + 48.0f;
    float RowY = Position.Y + Layout.Padding;
    FSlateDrawElement::MakeText(Elements, LayerId + 2,
        Geometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(Position.X + Layout.Padding, RowY))),
        FString(TEXT("MAP SYMBOLS")), Theme.MakeFont(Theme.BodySize, true, UKalmalaSettingsWidget::GetTextScalePercent()),
        ESlateDrawEffect::None, Theme.TextColor(true, Contrast));
    RowY += Layout.HeaderHeight;

    const auto DrawSymbol = [&](const FVector2D& Centre, const EKalmalaWorldMapMarkerCategory Category, const bool bOwningPlayer)
    {
        TArray<FVector2D> Points;
        FLinearColor Colour = bHighContrast ? FLinearColor::White : FLinearColor(0.7f, 0.9f, 1.0f);
        if (!bHighContrast && bOwningPlayer) Colour = FLinearColor(0.86f, 0.96f, 0.9f);
        else if (!bHighContrast && Category == EKalmalaWorldMapMarkerCategory::PersonalPins) Colour = GetPinColour(EKalmalaWorldMapPinStyle::Cairn);
        else if (!bHighContrast && Category == EKalmalaWorldMapMarkerCategory::CoopPings) Colour = FLinearColor(1.0f, 0.75f, 0.4f);

        if (bOwningPlayer)
            Points = { Centre + FVector2D(0.0f, -8.0f), Centre + FVector2D(7.0f, 6.0f), Centre + FVector2D(-7.0f, 6.0f), Centre + FVector2D(0.0f, -8.0f) };
        else if (Category == EKalmalaWorldMapMarkerCategory::CoopPings)
            Points = { Centre + FVector2D(-6.0f, -6.0f), Centre + FVector2D(6.0f, 6.0f), Centre, Centre + FVector2D(-6.0f, 6.0f), Centre + FVector2D(6.0f, -6.0f) };
        else
            Points = { Centre + FVector2D(0.0f, -7.0f), Centre + FVector2D(7.0f, 0.0f), Centre + FVector2D(0.0f, 7.0f),
                Centre + FVector2D(-7.0f, 0.0f), Centre + FVector2D(0.0f, -7.0f) };
        FSlateDrawElement::MakeLines(Elements, LayerId + 3, Geometry.ToPaintGeometry(), Points,
            ESlateDrawEffect::None, Colour, true, 2.5f);
    };

    const auto DrawRowText = [&](const FString& Text, const float Y, const int32 DrawLayer)
    {
        FSlateDrawElement::MakeText(Elements, DrawLayer,
            Geometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(TextX, Y + (Layout.RowHeight - TextHeight) * 0.5f))),
            Text, Font, ESlateDrawEffect::None, TextColour);
    };
    DrawSymbol(FVector2D(Position.X + 22.0f, RowY + Layout.RowHeight * 0.5f), EKalmalaWorldMapMarkerCategory::PersonalPins, true);
    DrawRowText(TEXT("You (facing)"), RowY, LayerId + 3);
    RowY += Layout.RowHeight;

    const auto DrawFilterRow = [&](const EKalmalaWorldMapMarkerCategory Category, const int32 Index, const TCHAR* Name,
        const int32 Count, const FVector2D& Centre)
    {
        const bool bVisible = IsMarkerCategoryVisible(Category);
        const bool bFocused = bMarkerFilterNavigationActive && FocusedMarkerCategoryIndex == Index;
        const bool bHovered = HoveredMarkerCategoryIndex == Index;
        const FVector2D RowPosition(Position.X + 5.0f, RowY);
        const FVector2D RowSize(PanelSize.X - 10.0f, Layout.RowHeight);
        if (bFocused || bHovered)
        {
            const FLinearColor Fill = bHighContrast ? FLinearColor(0.1f, 0.1f, 0.1f, 1.0f)
                : (bHovered ? Theme.ButtonHovered : Theme.ButtonFocused);
            FSlateDrawElement::MakeBox(Elements, LayerId + 2,
                Geometry.ToPaintGeometry(RowSize, FSlateLayoutTransform(RowPosition)), WhiteBrush, ESlateDrawEffect::None, Fill);
            const TArray<FVector2D> Outline = { RowPosition, RowPosition + FVector2D(RowSize.X, 0.0f), RowPosition + RowSize,
                RowPosition + FVector2D(0.0f, RowSize.Y), RowPosition };
            FSlateDrawElement::MakeLines(Elements, LayerId + 3, Geometry.ToPaintGeometry(), Outline,
                ESlateDrawEffect::None, bHighContrast ? FLinearColor::White : FocusColour, true,
                bHighContrast ? FMath::Max(2.0f, Theme.FocusBorderWidth) : Theme.FocusBorderWidth);
        }

        const FLinearColor CheckFill = bHighContrast ? FLinearColor::Black : (bVisible ? Theme.ButtonSelected : Theme.ButtonNormal);
        const FVector2D CheckPosition(Position.X + Layout.Padding, RowY + (Layout.RowHeight - 15.0f) * 0.5f);
        FSlateDrawElement::MakeBox(Elements, LayerId + 3,
            Geometry.ToPaintGeometry(FVector2D(15.0f), FSlateLayoutTransform(CheckPosition)), WhiteBrush, ESlateDrawEffect::None, CheckFill);
        const TArray<FVector2D> CheckOutline = { CheckPosition, CheckPosition + FVector2D(15.0f, 0.0f),
            CheckPosition + FVector2D(15.0f), CheckPosition + FVector2D(0.0f, 15.0f), CheckPosition };
        FSlateDrawElement::MakeLines(Elements, LayerId + 4, Geometry.ToPaintGeometry(), CheckOutline,
            ESlateDrawEffect::None, bHighContrast ? FLinearColor::White : Theme.BorderColor, true, bHighContrast ? 1.5f : 1.0f);
        if (bVisible)
        {
            FSlateDrawElement::MakeLines(Elements, LayerId + 5, Geometry.ToPaintGeometry(),
                { CheckPosition + FVector2D(3.0f, 8.0f), CheckPosition + FVector2D(6.0f, 11.0f), CheckPosition + FVector2D(12.0f, 3.0f) },
                ESlateDrawEffect::None, bHighContrast ? FLinearColor::White : TextColour, true, 1.8f);
        }
        DrawSymbol(Centre, Category, false);
        const FString Label = FString::Printf(TEXT("%s %d [%s]"), Name, Count, bVisible ? TEXT("SHOWN") : TEXT("FILTERED"));
        DrawRowText(Label, RowY, LayerId + 5);
        RowY += Layout.RowHeight;
    };

    DrawFilterRow(EKalmalaWorldMapMarkerCategory::PersonalPins, 0, TEXT("Personal pins"), Counts.PersonalPins,
        FVector2D(Position.X + 22.0f, RowY + Layout.RowHeight * 0.5f));
    DrawFilterRow(EKalmalaWorldMapMarkerCategory::CoopPlayers, 1, TEXT("Co-op players"), Counts.CoopPlayers,
        FVector2D(Position.X + 22.0f, RowY + Layout.RowHeight * 0.5f));
    DrawFilterRow(EKalmalaWorldMapMarkerCategory::CoopPings, 2, TEXT("Map pings"), Counts.CoopPings,
        FVector2D(Position.X + 22.0f, RowY + Layout.RowHeight * 0.5f));

}

FVector2D UKalmalaWorldMapWidget::WorldToMapNormalized(const FVector2D WorldPosition, const FVector2D MapCentre, const FVector2D MapExtent)
{
    if (!FMath::IsFinite(WorldPosition.X) || !FMath::IsFinite(WorldPosition.Y) || MapExtent.X <= 0.0f || MapExtent.Y <= 0.0f)
    {
        return FVector2D::ZeroVector;
    }
    return (WorldPosition - (MapCentre - MapExtent)) / (MapExtent * 2.0f);
}

FVector2D UKalmalaWorldMapWidget::MapNormalizedToWorld(const FVector2D NormalizedPosition, const FVector2D MapCentre, const FVector2D MapExtent)
{
    if (!FMath::IsFinite(NormalizedPosition.X) || !FMath::IsFinite(NormalizedPosition.Y)
        || MapExtent.X <= 0.0f || MapExtent.Y <= 0.0f)
    {
        return FVector2D::ZeroVector;
    }
    return MapCentre + (NormalizedPosition * 2.0f - FVector2D(1.0f, 1.0f)) * MapExtent;
}

FVector2D UKalmalaWorldMapWidget::GetFacingDirection(const float FacingDegrees)
{
    const float FacingRadians = FMath::DegreesToRadians(FMath::IsFinite(FacingDegrees) ? FacingDegrees : 0.0f);
    return FVector2D(FMath::Cos(FacingRadians), FMath::Sin(FacingRadians));
}

float UKalmalaWorldMapWidget::ChooseGridSpacing(const float InMapRadius)
{
    const float SafeRadius = FMath::Max(0.0f, InMapRadius);
    if (SafeRadius > 50000.0f) return FMath::Pow(10.0f, FMath::FloorToFloat(FMath::LogX(10.0f, SafeRadius)));
    return SafeRadius <= 7000.0f ? 2500.0f : SafeRadius <= 20000.0f ? 5000.0f : 10000.0f;
}

FString UKalmalaWorldMapWidget::SanitizePinLabel(FString Label)
{
    Label.TrimStartAndEndInline();
    FString Sanitized;
    Sanitized.Reserve(FMath::Min(Label.Len(), MaxPinLabelLength));
    for (const TCHAR Character : Label)
    {
        if (Sanitized.Len() >= MaxPinLabelLength) break;
        if (FChar::IsAlnum(Character) || Character == TEXT(' ') || Character == TEXT('-') || Character == TEXT('\'')) Sanitized.AppendChar(Character);
    }
    Sanitized.TrimStartAndEndInline();
    return Sanitized;
}

bool UKalmalaWorldMapWidget::IsValidPinLabel(const FString& Label)
{
    return !Label.IsEmpty() && Label.Len() <= MaxPinLabelLength && Label == SanitizePinLabel(Label);
}

FString UKalmalaWorldMapWidget::GetPinAccessibilityLabel(const FKalmalaWorldMapPersonalPin& Pin)
{
    const TCHAR* StyleName = Pin.Style == EKalmalaWorldMapPinStyle::Lantern ? TEXT("Lantern")
        : Pin.Style == EKalmalaWorldMapPinStyle::Thread ? TEXT("Thread") : TEXT("Cairn");
    return FString::Printf(TEXT("%s marker: %s; %s; %s"), StyleName, *Pin.Label,
        Pin.bComplete ? TEXT("complete") : TEXT("active"), Pin.bVisible ? TEXT("shown") : TEXT("hidden"));
}

FColor UKalmalaWorldMapWidget::GetFogTreatmentColor(const EKalmalaWorldMapFogTreatment Treatment)
{
    switch (Treatment)
    {
    case EKalmalaWorldMapFogTreatment::CurrentPersonal:
        return FColor(0, 0, 0, 0);
    case EKalmalaWorldMapFogTreatment::RememberedPersonal:
        // Sea-glass teal keeps personal memory visible without making it read as current sight.
        return FColor(20, 78, 70, 112);
    case EKalmalaWorldMapFogTreatment::ReservedShared:
        // Warm lichen-ember is intentionally distinct; no shared data is rendered yet.
        return FColor(106, 68, 32, 112);
    default:
        return FColor(8, 18, 24, 255);
    }
}

TArray<FColor> UKalmalaWorldMapWidget::BuildFogPixels(const FVector2D MapCentre, const FVector2D MapExtent,
    const FVector2D OwningPawnLocation, const FIntPoint Dimensions, const UKalmalaWorldMapExplorationSaveGame* Exploration,
    const bool bRevealAll, const bool bBoundedWorld)
{
    TArray<FColor> Pixels;
    if (Dimensions.X <= 0 || Dimensions.Y <= 0 || MapExtent.X <= 0.0f || MapExtent.Y <= 0.0f) return Pixels;
    Pixels.Reserve(Dimensions.X * Dimensions.Y);
    for (int32 Y = 0; Y < Dimensions.Y; ++Y) for (int32 X = 0; X < Dimensions.X; ++X)
    {
        const FVector2D Normalized(
            Dimensions.X > 1 ? static_cast<float>(X) / static_cast<float>(Dimensions.X - 1) * 2.0f - 1.0f : 0.0f,
            Dimensions.Y > 1 ? static_cast<float>(Y) / static_cast<float>(Dimensions.Y - 1) * 2.0f - 1.0f : 0.0f);
        const FVector2D WorldPosition = MapCentre + Normalized * MapExtent;
        if (bBoundedWorld && WorldPosition.SizeSquared() > FMath::Square(FKalmalaWorldBounds::Radius))
        {
            Pixels.Add(FColor(8, 18, 24, 255));
            continue;
        }
        const bool bCurrentlyVisible = bRevealAll || IsWithinLocalRevealRadius(WorldPosition, OwningPawnLocation, LocalRevealRadius);
        const bool bPersonallyExplored = !bCurrentlyVisible && Exploration != nullptr && Exploration->IsExplored(WorldPosition);
        // Fully opaque pixels disclose neither sampled terrain nor water treatment. Personal
        // memory is tinted separately from current sight; shared coverage has no source yet.
        Pixels.Add(GetFogTreatmentColor(bCurrentlyVisible ? EKalmalaWorldMapFogTreatment::CurrentPersonal
            : bPersonallyExplored ? EKalmalaWorldMapFogTreatment::RememberedPersonal : EKalmalaWorldMapFogTreatment::Unexplored));
    }
    return Pixels;
}

TArray<FColor> UKalmalaWorldMapWidget::BuildTilePixels(const FKalmalaWorldGenerationConfig Config, const FIntPoint TileCoordinate, const float WorldSize)
{
    const int32 SamplesPerAxis = WorldSize > TileWorldSize ? 65 : TileSamplesPerAxis;
    const FVector2D Centre = (FVector2D(TileCoordinate) + FVector2D(0.5f)) * WorldSize;
    if (WorldSize > TileWorldSize)
    {
        // Overview pixels span hundreds of metres: sample the same generator
        // once per pixel instead of resolving sub-metre collision triangles.
        TArray<FKalmalaMinimapTerrainSample> Samples;
        for (int32 Y = 0; Y < SamplesPerAxis; ++Y) for (int32 X = 0; X < SamplesPerAxis; ++X)
        {
            auto& S = Samples.AddDefaulted_GetRef();
            S.MapPosition = FVector2D(X, Y) * (2.0 / (SamplesPerAxis - 1)) - FVector2D(1);
            const FVector2D P = Centre + S.MapPosition * (WorldSize * 0.5);
            if (!FKalmalaWorldBounds::Contains(Config, P)) { S.TerrainColour = FLinearColor(0.003f, 0.006f, 0.009f); continue; }
            const auto R = FKalmalaRegionalGeneration::Sample(Config, P);
            S.TerrainHeight = R.Height;
            S.bIsWater = R.Height < 0 || R.bHasWater;
            const auto Biome = static_cast<EKalmalaBiome>(R.Biome);
            S.TerrainColour = FKalmalaMinimapRaster::SampleBiomeTexture(S.bIsWater ? EKalmalaBiome::Ocean : Biome, P);
            if (S.bIsWater && Biome == EKalmalaBiome::ShimmeringLakes) S.TerrainColour *= FLinearColor(1.3f, 1.8f, 1.6f);
        }
        return FKalmalaMinimapRaster::BuildPixels(Samples, FIntPoint(SamplesPerAxis));
    }
    return FKalmalaMinimapRaster::BuildPixels(UKalmalaMinimapViewModel::BuildTerrainSamples(
        Config, Centre, FVector2D(WorldSize * 0.5f), FIntPoint(TileSamplesPerAxis, TileSamplesPerAxis)),
        FIntPoint(TileSamplesPerAxis, TileSamplesPerAxis));
}

void UKalmalaWorldMapWidget::InvalidateOutstandingTileJobs()
{
    ++TileEpoch;
    if (TileEpoch == 0) ++TileEpoch;
    // The old view can have a full pending set. Drop those local handles so
    // a pan or zoom cannot accumulate multiple capped request sets; workers
    // retain their epoch and their output is never uploaded into the new view.
    Tiles.Reset();
}

void UKalmalaWorldMapWidget::StartTile(const FIntPoint& TileCoordinate, const FKalmalaWorldGenerationConfig& Config)
{
    FWorldMapTile& Tile = Tiles.FindOrAdd(TileCoordinate);
    if (Tile.PendingPixels.IsValid() || Tile.Texture != nullptr) return;
    Tile.RequestEpoch = TileEpoch;
    Tile.PendingPixels = Async(EAsyncExecution::ThreadPool, [Config, TileCoordinate, WorldSize = ActiveTileWorldSize]()
    {
        FWorldMapTile::FBuildResult Result;
        const double StartedAt = FPlatformTime::Seconds();
        Result.Pixels = BuildTilePixels(Config, TileCoordinate, WorldSize);
        Result.WorkerSeconds = FPlatformTime::Seconds() - StartedAt;
        return Result;
    });
}

void UKalmalaWorldMapWidget::UploadCompletedTiles()
{
    const int32 Resolution = ActiveTileWorldSize > TileWorldSize ? 65 : TileSamplesPerAxis;
    for (TPair<FIntPoint, FWorldMapTile>& Pair : Tiles)
    {
        FWorldMapTile& Tile = Pair.Value;
        if (!Tile.PendingPixels.IsValid() || !Tile.PendingPixels.IsReady()) continue;
        FWorldMapTile::FBuildResult BuildResult = Tile.PendingPixels.Get();
        Tile.PendingPixels = {};
        // A stale worker never reaches the render resource after pan, zoom,
        // identity, or player-centre changes.
        if (Tile.RequestEpoch != TileEpoch || BuildResult.Pixels.Num() != Resolution * Resolution) continue;
        DeveloperProfileWorkerSeconds += BuildResult.WorkerSeconds;
        DeveloperProfileMaxWorkerSeconds = FMath::Max(DeveloperProfileMaxWorkerSeconds, BuildResult.WorkerSeconds);
        TArray<FColor>& Pixels = BuildResult.Pixels;
        Tile.PixelHash = FCrc::MemCrc32(Pixels.GetData(), Pixels.Num() * sizeof(FColor));
        Tile.Texture = TStrongObjectPtr<UTexture2D>(UTexture2D::CreateTransient(Resolution, Resolution, PF_B8G8R8A8));
        if (Tile.Texture == nullptr) continue;
        Tile.Texture->SRGB = true; Tile.Texture->Filter = TF_Bilinear; Tile.Texture->NeverStream = true; Tile.Texture->UpdateResource();
        const uint32 ByteCount = Pixels.Num() * sizeof(FColor);
        uint8* Upload = static_cast<uint8*>(FMemory::Malloc(ByteCount)); FMemory::Memcpy(Upload, Pixels.GetData(), ByteCount);
        auto* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Resolution, Resolution);
        Tile.Texture->UpdateTextureRegions(0, 1, Region, Resolution * sizeof(FColor), sizeof(FColor), Upload,
            [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
    }
}

void UKalmalaWorldMapWidget::UpdateFogTexture(const FVector2D MapCentre, const FVector2D MapExtent,
    const FVector2D OwningPawnLocation, const FIntPoint Dimensions)
{
    const TArray<FColor> Pixels = BuildFogPixels(MapCentre, MapExtent, OwningPawnLocation, Dimensions, ExplorationSave, RevealWorldMap.GetValueOnGameThread() != 0, FKalmalaWorldBounds::IsBounded(TileConfig));
    if (Pixels.Num() != Dimensions.X * Dimensions.Y) return;
    if (FogTexture != nullptr && (FogTexture->GetSizeX() != Dimensions.X || FogTexture->GetSizeY() != Dimensions.Y))
    {
        FogBrush.SetResourceObject(nullptr);
        FogTexture = nullptr;
    }
    if (FogTexture == nullptr)
    {
        FogTexture = TStrongObjectPtr<UTexture2D>(UTexture2D::CreateTransient(Dimensions.X, Dimensions.Y, PF_B8G8R8A8));
        if (FogTexture == nullptr) return;
        FogTexture->SRGB = true; FogTexture->Filter = TF_Bilinear; FogTexture->NeverStream = true; FogTexture->UpdateResource();
        FogBrush.SetResourceObject(FogTexture.Get()); FogBrush.ImageSize = FVector2D(Dimensions); FogBrush.DrawAs = ESlateBrushDrawType::Image;
    }
    if (FogTexture->GetResource() == nullptr) return;
    const uint32 ByteCount = Pixels.Num() * sizeof(FColor);
    uint8* Upload = static_cast<uint8*>(FMemory::Malloc(ByteCount)); FMemory::Memcpy(Upload, Pixels.GetData(), ByteCount);
    auto* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Dimensions.X, Dimensions.Y);
    FogTexture->UpdateTextureRegions(0, 1, Region, Dimensions.X * sizeof(FColor), sizeof(FColor), Upload,
        [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
}

FString UKalmalaWorldMapWidget::GetExplorationSaveSlot(const FKalmalaWorldGenerationConfig& Config) const
{
    const int32 PlayerIndex = GetOwningPlayer() != nullptr ? GetOwningPlayer()->GetLocalPlayer()->GetControllerId() : 0;
    return FString::Printf(TEXT("KalmalaPersonalMapCoverage_v1_%llu_%d"), Config.WorldSeed, PlayerIndex);
}

FString UKalmalaWorldMapWidget::GetPinsSaveSlot(const FKalmalaWorldGenerationConfig& Config) const
{
    const int32 PlayerIndex = GetOwningPlayer() != nullptr ? GetOwningPlayer()->GetLocalPlayer()->GetControllerId() : 0;
    return FString::Printf(TEXT("KalmalaPersonalMapPins_v1_%llu_%d"), Config.WorldSeed, PlayerIndex);
}

void UKalmalaWorldMapWidget::EnsureExplorationForWorld(const FKalmalaWorldGenerationConfig& Config)
{
    if (ExplorationSave != nullptr && ExplorationSave->MatchesWorld(Config)) return;
    ExplorationSave = Cast<UKalmalaWorldMapExplorationSaveGame>(UGameplayStatics::LoadGameFromSlot(GetExplorationSaveSlot(Config), 0));
    bExplorationLoadedForWorld = ExplorationSave != nullptr && ExplorationSave->MatchesWorld(Config);
    if (!bExplorationLoadedForWorld)
    {
        ExplorationSave = NewObject<UKalmalaWorldMapExplorationSaveGame>(this);
        ExplorationSave->InitializeForWorld(Config);
    }
}

void UKalmalaWorldMapWidget::RecordLocalExploration(const FVector2D OwningPawnLocation)
{
    if (ExplorationSave != nullptr && ExplorationSave->RecordReveal(OwningPawnLocation, LocalRevealRadius))
    {
        UGameplayStatics::SaveGameToSlot(ExplorationSave, GetExplorationSaveSlot(TileConfig), 0);
    }
}

void UKalmalaWorldMapWidget::TickExploration(const float DeltaTime)
{
    ExplorationAccumulator += DeltaTime;
    if (ViewModel == nullptr || ExplorationAccumulator < 0.5f) return;
    FKalmalaWorldGenerationConfig Config;
    FVector2D PawnLocation;
    if (!ViewModel->GetPresentationInputs(Config, PawnLocation)) return;
    ExplorationAccumulator = 0.0f;
    if (!(Config == TileConfig)) { TileConfig = Config; InvalidateOutstandingTileJobs(); }
    EnsureExplorationForWorld(Config);
    RecordLocalExploration(PawnLocation);
    if (!bMapOpen && !bDeveloperClosedExplorationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
    {
        bDeveloperClosedExplorationLogged = true;
        UE_LOG(LogTemp, Display, TEXT("World map gameplay exploration: Closed=1 Cells=%d Tiles=%d."),
            ExplorationSave->GetExploredCellCount(), Tiles.Num());
    }
}

void UKalmalaWorldMapWidget::EnsurePinsForWorld(const FKalmalaWorldGenerationConfig& Config)
{
    if (PinsSave != nullptr && PinsSave->MatchesWorld(Config)) return;
    PinsSave = Cast<UKalmalaWorldMapPinsSaveGame>(UGameplayStatics::LoadGameFromSlot(GetPinsSaveSlot(Config), 0));
    if (PinsSave != nullptr && PinsSave->MatchesWorld(Config))
    {
        LocalPins = PinsSave->GetPins();
        if (!LocalPins.IsValidIndex(SelectedPinIndex)) SelectedPinIndex = INDEX_NONE;
        return;
    }
    PinsSave = NewObject<UKalmalaWorldMapPinsSaveGame>(this);
    PinsSave->InitializeForWorld(Config);
    LocalPins.Reset();
    SelectedPinIndex = INDEX_NONE;
}

void UKalmalaWorldMapWidget::PersistPins()
{
    if (PinsSave != nullptr && PinsSave->MatchesWorld(TileConfig) && PinsSave->SetPins(LocalPins))
    {
        UGameplayStatics::SaveGameToSlot(PinsSave, GetPinsSaveSlot(TileConfig), 0);
    }
}

void UKalmalaWorldMapWidget::EvictUnusedTiles()
{
    if (Tiles.Num() <= MaxCachedTiles) return;
    TArray<FIntPoint> Evictable;
    for (const TPair<FIntPoint, FWorldMapTile>& Pair : Tiles)
    {
        if (Pair.Value.LastUsedEpoch != TileEpoch && !Pair.Value.PendingPixels.IsValid()) Evictable.Add(Pair.Key);
    }
    Evictable.Sort([](const FIntPoint& Left, const FIntPoint& Right) { return Left.X == Right.X ? Left.Y < Right.Y : Left.X < Right.X; });
    for (const FIntPoint& Key : Evictable)
    {
        if (Tiles.Num() <= MaxCachedTiles) break;
        Tiles.Remove(Key);
    }
}

TArray<FIntPoint> UKalmalaWorldMapWidget::BuildPrioritizedTileCoordinates(const FVector2D& Centre, const FVector2D& Extent, const float WorldSize)
{
    const FIntPoint Min(FMath::FloorToInt((Centre.X - Extent.X) / WorldSize), FMath::FloorToInt((Centre.Y - Extent.Y) / WorldSize));
    const FIntPoint Max(FMath::FloorToInt((Centre.X + Extent.X) / WorldSize), FMath::FloorToInt((Centre.Y + Extent.Y) / WorldSize));
    TArray<FIntPoint> Coordinates;
    for (int32 Y = Min.Y; Y <= Max.Y; ++Y) for (int32 X = Min.X; X <= Max.X; ++X) Coordinates.Add(FIntPoint(X, Y));
    Coordinates.Sort([Centre, WorldSize](const FIntPoint& Left, const FIntPoint& Right)
    {
        const FVector2D LeftCentre = (FVector2D(Left) + FVector2D(0.5f)) * WorldSize;
        const FVector2D RightCentre = (FVector2D(Right) + FVector2D(0.5f)) * WorldSize;
        const double LeftDistance = FVector2D::DistSquared(LeftCentre, Centre);
        const double RightDistance = FVector2D::DistSquared(RightCentre, Centre);
        if (!FMath::IsNearlyEqual(LeftDistance, RightDistance)) return LeftDistance < RightDistance;
        return Left.X == Right.X ? Left.Y < Right.Y : Left.X < Right.X;
    });
    Coordinates.SetNum(FMath::Min(Coordinates.Num(), MaxCachedTiles));
    return Coordinates;
}

void UKalmalaWorldMapWidget::LogDeveloperTileFingerprint()
{
    if (bDeveloperTileFingerprintLogged || !bDeveloperVerificationLogged || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification"))) return;
    TArray<FIntPoint> Coordinates;
    uint32 Digest = 0;
    for (const TPair<FIntPoint, FWorldMapTile>& Pair : Tiles)
    {
        if (Pair.Value.LastUsedEpoch != TileEpoch) continue;
        if (Pair.Value.Texture == nullptr) return;
        Coordinates.Add(Pair.Key);
    }
    if (Coordinates.IsEmpty() || !ViewModel || Coordinates.Num() != BuildPrioritizedTileCoordinates(ViewModel->GetMapCentre(), ViewModel->GetMapExtent(), ActiveTileWorldSize).Num()) return;
    Coordinates.Sort([](const FIntPoint& Left, const FIntPoint& Right) { return Left.X == Right.X ? Left.Y < Right.Y : Left.X < Right.X; });
    for (const FIntPoint& Coordinate : Coordinates)
    {
        const FWorldMapTile& Tile = Tiles.FindChecked(Coordinate);
        Digest = FCrc::MemCrc32(&Coordinate, sizeof(Coordinate), Digest);
        Digest = FCrc::MemCrc32(&Tile.PixelHash, sizeof(Tile.PixelHash), Digest);
    }
    bDeveloperTileFingerprintLogged = true;
    UE_LOG(LogTemp, Display, TEXT("World map tile presentation: Seed=%llu Tiles=%d Fingerprint=%u PollOnly=1."),
        TileConfig.WorldSeed, Coordinates.Num(), Digest);
}

void UKalmalaWorldMapWidget::LogDeveloperProfile()
{
    if (bDeveloperProfileLogged || !bDeveloperVerificationLogged || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapProfile"))) return;
    int32 ReadyTiles = 0;
    for (const TPair<FIntPoint, FWorldMapTile>& Pair : Tiles)
    {
        if (Pair.Value.LastUsedEpoch != TileEpoch) continue;
        if (Pair.Value.Texture == nullptr) return;
        ++ReadyTiles;
    }
    if (ReadyTiles == 0) return;
    bDeveloperProfileLogged = true;
    const double OpenMilliseconds = (FPlatformTime::Seconds() - DeveloperProfileOpenedAt) * 1000.0;
    const int32 Resolution = ActiveTileWorldSize > TileWorldSize ? 65 : TileSamplesPerAxis;
    const SIZE_T CacheBytes = static_cast<SIZE_T>(ReadyTiles) * Resolution * Resolution * sizeof(FColor);
    UE_LOG(LogTemp, Display, TEXT("World map profile: OpenMs=%.3f WorkerTotalMs=%.3f WorkerMaxMs=%.3f GameThreadTotalMs=%.3f GameThreadMaxMs=%.3f Ticks=%d Tiles=%d CacheBytes=%llu."),
        OpenMilliseconds, DeveloperProfileWorkerSeconds * 1000.0, DeveloperProfileMaxWorkerSeconds * 1000.0,
        DeveloperProfileGameThreadSeconds * 1000.0, DeveloperProfileMaxGameThreadSeconds * 1000.0,
        DeveloperProfileGameThreadTicks, ReadyTiles, static_cast<uint64>(CacheBytes));
}

void UKalmalaWorldMapWidget::RefreshTiles(const FVector2D& MapSize)
{
    if (ViewModel == nullptr || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    FKalmalaWorldGenerationConfig Config;
    FVector2D PawnLocation;
    if (!ViewModel->GetPresentationInputs(Config, PawnLocation))
    {
        if (!bDeveloperTileInputsUnavailableLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
        {
            bDeveloperTileInputsUnavailableLogged = true;
            UE_LOG(LogTemp, Display, TEXT("World map tile inputs are not ready."));
        }
        return;
    }
    if (!(Config == TileConfig)) { TileConfig = Config; InvalidateOutstandingTileJobs(); }
    const FVector2D Centre = ViewModel->GetMapCentre();
    const FVector2D Extent = ViewModel->GetMapExtent();
    const float NewTileSize = ChooseTileWorldSize(Extent);
    if (NewTileSize != ActiveTileWorldSize) { ActiveTileWorldSize = NewTileSize; InvalidateOutstandingTileJobs(); }
    EnsureExplorationForWorld(Config);
    EnsurePinsForWorld(Config);
    const FIntPoint FogDimensions = FKalmalaWorldBounds::IsBounded(Config)
        ? FIntPoint(513, FMath::Clamp(FMath::RoundToInt(513.0 * MapSize.Y / MapSize.X), 171, 1026))
        : ViewModel->GetMapSampleDimensions();
    UpdateFogTexture(Centre, Extent, PawnLocation, FogDimensions);
    if (!bDeveloperFogVerificationLogged && bDeveloperVerificationLogged && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
    {
        const FVector2D RemoteProbe = PawnLocation + FVector2D(LocalRevealRadius * 2.0f + 1.0f, 0.0f);
        const bool bRemoteExplored = ExplorationSave != nullptr && ExplorationSave->IsExplored(RemoteProbe);
        bDeveloperFogVerificationLogged = true;
        UE_LOG(LogTemp, Display, TEXT("World map fog presentation: Seed=%llu Cells=%d Loaded=%d Remote=%d."),
            Config.WorldSeed, ExplorationSave != nullptr ? ExplorationSave->GetExploredCellCount() : 0,
            bExplorationLoadedForWorld ? 1 : 0, bRemoteExplored ? 1 : 0);
    }
    int32 PendingCount = 0;
    for (const auto& Pair : Tiles) if (Pair.Value.PendingPixels.IsValid()) ++PendingCount;
    for (const FIntPoint& Key : BuildPrioritizedTileCoordinates(Centre, Extent, ActiveTileWorldSize))
    {
        FWorldMapTile* ExistingTile = Tiles.Find(Key);
        if (ExistingTile == nullptr && (Tiles.Num() >= MaxCachedTiles || PendingCount >= 2)) continue;
        if (ExistingTile == nullptr) ++PendingCount;
        FWorldMapTile& Tile = Tiles.FindOrAdd(Key); Tile.LastUsedEpoch = TileEpoch; StartTile(Key, Config);
    }
    UploadCompletedTiles();
    EvictUnusedTiles();
}

void UKalmalaWorldMapWidget::TickTilePresentation(const float DeltaTime)
{
    const double TickStartedAt = FPlatformTime::Seconds();
    if (!bMapOpen || ViewModel == nullptr) return;
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    if (APlayerController* Controller = GetOwningPlayer()) Controller->GetViewportSize(ViewportWidth, ViewportHeight);
    // Paint and pointer input use Slate units, including the local player's DPI
    // scale. Keep the generated aspect ratio in that same coordinate space.
    const FVector2D PaintedSize = GetCachedGeometry().GetLocalSize();
    const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), UE_SMALL_NUMBER);
    const FVector2D SurfaceSize = PaintedSize.X > 88.0f && PaintedSize.Y > 88.0f
        ? PaintedSize : FVector2D(ViewportWidth, ViewportHeight) / ViewportScale;
    const FVector2D MapSize = SurfaceSize - FVector2D(88.0f);
    if (MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const float AspectRatio = MapSize.Y > 0.0f ? MapSize.X / MapSize.Y : 1.0f;
    ViewModel->SetMapAspectRatio(AspectRatio);
    if (bFitWholeWorld)
    {
        const float FitZoom = FullWorldZoom(AspectRatio);
        if (MapZoom != FitZoom) { MapZoom = FitZoom; InvalidateOutstandingTileJobs(); }
        ViewModel->SetMapRadius(MapZoom);
        ViewModel->SetMapCentre(FVector2D::ZeroVector);
    }
    ViewModel->SetMapSampleDimensions(FIntPoint(161, FMath::Clamp(FMath::RoundToInt(161.0f / AspectRatio), 61, 129)));
    RefreshAccumulator += DeltaTime;
    if (RefreshAccumulator >= 0.10f) { RefreshAccumulator = 0.0f; RefreshTiles(MapSize); LogDeveloperTileFingerprint(); LogDeveloperProfile(); }
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapProfile")))
    {
        const double TickSeconds = FPlatformTime::Seconds() - TickStartedAt;
        DeveloperProfileGameThreadSeconds += TickSeconds;
        DeveloperProfileMaxGameThreadSeconds = FMath::Max(DeveloperProfileMaxGameThreadSeconds, TickSeconds);
        ++DeveloperProfileGameThreadTicks;
    }
}

void UKalmalaWorldMapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    TickTilePresentation(InDeltaTime);
    if (bDeveloperVerificationLogged && !bRequestedVerificationScreenshot)
    {
        VerificationElapsed += InDeltaTime;
        FString ScreenshotPath;
        if (VerificationElapsed >= 3.0f && bDeveloperPaintVerified && FParse::Value(FCommandLine::Get(), TEXT("KalmalaWorldMapScreenshot="), ScreenshotPath))
        {
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
            bRequestedVerificationScreenshot = true;
        }
    }
}

int32 UKalmalaWorldMapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FMargin Margin(44.0f);
    const FVector2D MapSize(FMath::Max(1.0f, Size.X - Margin.Left - Margin.Right), FMath::Max(1.0f, Size.Y - Margin.Top - Margin.Bottom));
    const FPaintGeometry MapGeometry = AllottedGeometry.ToPaintGeometry(MapSize, FSlateLayoutTransform(FVector2D(Margin.Left, Margin.Top)));
    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    const FSlateBrush PanelBrush = Theme.MakePanelBrush(UKalmalaSettingsWidget::GetContrastMode(), &Theme.WorldMapPanelImage);
    FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), &PanelBrush,
        ESlateDrawEffect::None, PanelBrush.TintColor.GetSpecifiedColor());
    FMarkerCounts MarkerCounts;
    if (ViewModel != nullptr)
    {
        // Edge tiles extend past the view. Clip terrain and fog together so
        // unexplored terrain cannot leak into the surrounding controls.
        OutDrawElements.PushClip(FSlateClippingZone(MapGeometry));
        const FVector2D Centre = ViewModel->GetMapCentre();
        const FVector2D Extent = ViewModel->GetMapExtent();
        const float GridSpacing = ChooseGridSpacing(MapZoom);
        const FVector2D WorldMinimum = Centre - Extent;
        const FVector2D WorldMaximum = Centre + Extent;
        const FLinearColor GridColour(0.42f, 0.72f, 0.67f, 0.16f);
        for (float X = FMath::FloorToFloat(WorldMinimum.X / GridSpacing) * GridSpacing; X <= WorldMaximum.X; X += GridSpacing)
        {
            const float NormalizedX = WorldToMapNormalized(FVector2D(X, Centre.Y), Centre, Extent).X;
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 1, MapGeometry,
                { FVector2D(NormalizedX * MapSize.X, 0.0f), FVector2D(NormalizedX * MapSize.X, MapSize.Y) }, ESlateDrawEffect::None, GridColour, true, 1.0f);
        }
        for (float Y = FMath::FloorToFloat(WorldMinimum.Y / GridSpacing) * GridSpacing; Y <= WorldMaximum.Y; Y += GridSpacing)
        {
            const float NormalizedY = WorldToMapNormalized(FVector2D(Centre.X, Y), Centre, Extent).Y;
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 1, MapGeometry,
                { FVector2D(0.0f, NormalizedY * MapSize.Y), FVector2D(MapSize.X, NormalizedY * MapSize.Y) }, ESlateDrawEffect::None, GridColour, true, 1.0f);
        }
        for (const TPair<FIntPoint, FWorldMapTile>& Pair : Tiles) if (Pair.Value.Texture != nullptr)
        {
            const FVector2D WorldMin = FVector2D(Pair.Key) * ActiveTileWorldSize;
            const FVector2D Offset = (WorldMin - (Centre - Extent)) / (Extent * 2.0f);
            const FVector2D TileFraction(ActiveTileWorldSize / (Extent.X * 2.0f), ActiveTileWorldSize / (Extent.Y * 2.0f));
            const FPaintGeometry TileGeometry = AllottedGeometry.ToPaintGeometry(MapSize * TileFraction,
                FSlateLayoutTransform(FVector2D(Margin.Left, Margin.Top) + Offset * MapSize));
            FSlateBrush TileBrush;
            TileBrush.SetResourceObject(Pair.Value.Texture.Get()); TileBrush.ImageSize = FVector2D(Pair.Value.Texture->GetSizeX()); TileBrush.DrawAs = ESlateBrushDrawType::Image;
            FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 1, TileGeometry, &TileBrush, ESlateDrawEffect::None, FLinearColor::White);
        }
        if (FogTexture != nullptr)
        {
            FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 2, MapGeometry, &FogBrush, ESlateDrawEffect::None, FLinearColor::White);
        }
        OutDrawElements.PopClip();
        MarkerCounts.PersonalPins = DrawPins(AllottedGeometry, MapSize, DrawLayer + 3, OutDrawElements);
        const FMarkerCounts CoopCounts = DrawCoopAwareness(AllottedGeometry, MapSize, DrawLayer + 6, OutDrawElements);
        MarkerCounts.CoopPlayers = CoopCounts.CoopPlayers;
        MarkerCounts.CoopPings = CoopCounts.CoopPings;
        if (!bDeveloperPaintVerified && bDeveloperVerificationLogged && FogTexture != nullptr
            && FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldMapVerification")))
        {
            const FGeometry PlayerScreen = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(GetOwningPlayer());
            int32 ReadyTiles = 0;
            for (const auto& Pair : Tiles) if (Pair.Value.Texture != nullptr) ++ReadyTiles;
            const bool bFillsPlayerScreen = Size.X > 88.0f && Size.Y > 88.0f
                && Size.Equals(PlayerScreen.GetLocalSize(), 1.0f)
                && AllottedGeometry.LocalToAbsolute(FVector2D::ZeroVector).Equals(PlayerScreen.LocalToAbsolute(FVector2D::ZeroVector), 1.0f);
            const int32 ExpectedTiles = BuildPrioritizedTileCoordinates(Centre, Extent, ActiveTileWorldSize).Num();
            if (bFillsPlayerScreen && ReadyTiles > 0 && ReadyTiles == ExpectedTiles)
            {
                bDeveloperPaintVerified = true;
                if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldOverviewVerification")))
                {
                    const bool Fits = Centre.IsNearlyZero() && Extent.X > FKalmalaWorldBounds::Radius && Extent.Y > FKalmalaWorldBounds::Radius;
                    UE_LOG(LogTemp, Display, TEXT("World overview: Fits=%d Reveal=%d Tiles=%d Expected=%d Radius=1600000."),
                        Fits ? 1 : 0, RevealWorldMap.GetValueOnGameThread() != 0 ? 1 : 0, ReadyTiles, ExpectedTiles);
                }
                UE_LOG(LogTemp, Display, TEXT("World map paint verification: FullViewport=1 Size=%.0fx%.0f Map=%.0fx%.0f ReadyTiles=%d Fog=1."),
                    Size.X, Size.Y, MapSize.X, MapSize.Y, ReadyTiles);
            }
        }
    }
    FVector2D PawnLocation;
    FKalmalaWorldGenerationConfig PresentationConfig;
    if (ViewModel != nullptr && ViewModel->GetPresentationInputs(PresentationConfig, PawnLocation))
    {
        const FVector2D NormalizedPawn = WorldToMapNormalized(PawnLocation, ViewModel->GetMapCentre(), ViewModel->GetMapExtent());
        if (NormalizedPawn.X >= 0.0f && NormalizedPawn.X <= 1.0f && NormalizedPawn.Y >= 0.0f && NormalizedPawn.Y <= 1.0f)
        {
            const FVector2D MarkerCentre = FVector2D(Margin.Left, Margin.Top) + NormalizedPawn * MapSize;
            const FVector2D Forward = GetFacingDirection(ViewModel->GetPlayerFacingDegrees());
            const FVector2D Right(-Forward.Y, Forward.X);
            const TArray<FVector2D> MarkerLines = { MarkerCentre + Forward * 15.0f, MarkerCentre - Forward * 9.0f + Right * 8.0f,
                MarkerCentre - Forward * 9.0f - Right * 8.0f, MarkerCentre + Forward * 15.0f };
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 4, AllottedGeometry.ToPaintGeometry(), MarkerLines,
                ESlateDrawEffect::None, FLinearColor(0.02f, 0.04f, 0.05f, 1.0f), true, 5.0f);
            FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 5, AllottedGeometry.ToPaintGeometry(), MarkerLines,
                ESlateDrawEffect::None, FLinearColor(0.86f, 0.96f, 0.9f, 1.0f), true, 2.5f);
        }
    }
    DrawMarkerLegend(AllottedGeometry, DrawLayer + 9, MarkerCounts, OutDrawElements);
    const FString Hint = FString::Printf(TEXT("MAP  |  %.0fm  |  Grid %.0fm"),
        MapZoom / 100.0f, ChooseGridSpacing(MapZoom) / 100.0f);
    const float HeaderBottom = DrawMapLabel(OutDrawElements, DrawLayer + 5, AllottedGeometry, FVector2D(20, 18), Hint, 3);
    if (LocalPins.IsValidIndex(SelectedPinIndex))
    {
        FString SelectedLabel = GetPinAccessibilityLabel(LocalPins[SelectedPinIndex]);
        if (LocalPins[SelectedPinIndex].bVisible && !IsMarkerCategoryVisible(EKalmalaWorldMapMarkerCategory::PersonalPins))
            SelectedLabel += TEXT("; filtered from map");
        const FString SelectedHint = FString::Printf(TEXT("SELECTED: %s"), *SelectedLabel);
        DrawMapLabel(OutDrawElements, DrawLayer + 6, AllottedGeometry, FVector2D(20, HeaderBottom), SelectedHint, 1, true);
    }
    if (bPinLabelEntry)
    {
        const FString Draft = PendingPinLabel.IsEmpty() ? TEXT("Name marker…") : PendingPinLabel;
        const FString Prompt = FString::Printf(TEXT("MARKER: %s  |  Cairn · Lantern · Thread · Save"), *Draft);
        DrawMapLabel(OutDrawElements, DrawLayer + 8, AllottedGeometry, FVector2D(20, Size.Y - 38), Prompt, 1, true);
    }
    return DrawLayer + 16;
}

FVector2D UKalmalaWorldMapWidget::ScreenToWorld(const FVector2D& ScreenPosition, const FVector2D& MapSize) const
{
    if (ViewModel == nullptr || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return FVector2D::ZeroVector;
    return MapNormalizedToWorld(ScreenPosition / MapSize, ViewModel->GetMapCentre(), ViewModel->GetMapExtent());
}

int32 UKalmalaWorldMapWidget::FindVisiblePinAtScreenPosition(const FVector2D& ScreenPosition, const FVector2D& MapSize) const
{
    if (ViewModel == nullptr || !IsMarkerCategoryVisible(EKalmalaWorldMapMarkerCategory::PersonalPins)) return INDEX_NONE;
    const FVector2D Centre = ViewModel->GetMapCentre();
    const FVector2D Extent = ViewModel->GetMapExtent();
    for (int32 Index = LocalPins.Num() - 1; Index >= 0; --Index)
    {
        const FKalmalaWorldMapPersonalPin& Pin = LocalPins[Index];
        if (!IsPersonalPinOnCurrentView(Pin)) continue;
        const FVector2D PinScreen = WorldToMapNormalized(Pin.WorldLocation, Centre, Extent) * MapSize;
        if (FVector2D::DistSquared(PinScreen, ScreenPosition) <= FMath::Square(16.0f)) return Index;
    }
    return INDEX_NONE;
}

void UKalmalaWorldMapWidget::BeginPinPlacement(const FVector2D& WorldLocation)
{
    bDragging = false;
    bPinLabelEntry = true;
    PendingPinLocation = WorldLocation;
    PendingPinLabel.Reset();
    PendingPinStyle = EKalmalaWorldMapPinStyle::Cairn;
}

void UKalmalaWorldMapWidget::CommitPinPlacement()
{
    const FString Label = SanitizePinLabel(PendingPinLabel);
    if (!IsValidPinLabel(Label)) return;
    while (LocalPins.Num() >= UKalmalaWorldMapPinsSaveGame::MaxPersonalPins) LocalPins.RemoveAt(0);
    FKalmalaWorldMapPersonalPin& Pin = LocalPins.AddDefaulted_GetRef();
    Pin.WorldLocation = PendingPinLocation;
    Pin.Label = Label;
    Pin.Style = PendingPinStyle;
    SelectedPinIndex = LocalPins.Num() - 1;
    bPinLabelEntry = false;
    PendingPinLabel.Reset();
    PersistPins();
}

void UKalmalaWorldMapWidget::SelectNextPin()
{
    if (LocalPins.IsEmpty()) { SelectedPinIndex = INDEX_NONE; return; }
    SelectedPinIndex = LocalPins.IsValidIndex(SelectedPinIndex) ? (SelectedPinIndex + 1) % LocalPins.Num() : 0;
}

bool UKalmalaWorldMapWidget::ToggleSelectedPinCompletion()
{
    if (!LocalPins.IsValidIndex(SelectedPinIndex)) return false;
    LocalPins[SelectedPinIndex].bComplete = !LocalPins[SelectedPinIndex].bComplete;
    PersistPins();
    return true;
}

bool UKalmalaWorldMapWidget::ToggleSelectedPinVisibility()
{
    if (!LocalPins.IsValidIndex(SelectedPinIndex)) return false;
    LocalPins[SelectedPinIndex].bVisible = !LocalPins[SelectedPinIndex].bVisible;
    PersistPins();
    return true;
}

bool UKalmalaWorldMapWidget::RemoveSelectedPin()
{
    if (!LocalPins.IsValidIndex(SelectedPinIndex)) return false;
    LocalPins.RemoveAt(SelectedPinIndex);
    SelectedPinIndex = LocalPins.IsEmpty() ? INDEX_NONE : FMath::Min(SelectedPinIndex, LocalPins.Num() - 1);
    PersistPins();
    return true;
}

void UKalmalaWorldMapWidget::PanByKeyboardDelta(const FVector2D& ScreenDelta)
{
    const FVector2D MapSize = GetCachedGeometry().GetLocalSize() - FVector2D(88.0f);
    PanByScreenDelta(ScreenDelta, MapSize);
}

void UKalmalaWorldMapWidget::ZoomAtMapCentre(const float WheelDelta)
{
    const FVector2D MapSize = GetCachedGeometry().GetLocalSize() - FVector2D(88.0f);
    ZoomAtScreenPosition(WheelDelta, MapSize * 0.5f, MapSize);
}

void UKalmalaWorldMapWidget::CancelPinPlacement()
{
    bPinLabelEntry = false;
    PendingPinLabel.Reset();
}

FLinearColor UKalmalaWorldMapWidget::GetPinColour(const EKalmalaWorldMapPinStyle Style)
{
    switch (Style)
    {
    case EKalmalaWorldMapPinStyle::Lantern: return FLinearColor(0.93f, 0.68f, 0.28f, 1.0f);
    case EKalmalaWorldMapPinStyle::Thread: return FLinearColor(0.7f, 0.5f, 0.82f, 1.0f);
    default: return FLinearColor(0.48f, 0.84f, 0.73f, 1.0f);
    }
}

int32 UKalmalaWorldMapWidget::DrawPins(const FGeometry& AllottedGeometry, const FVector2D& MapSize, const int32 LayerId,
    FSlateWindowElementList& OutDrawElements) const
{
    if (ViewModel == nullptr) return 0;
    const FVector2D Margin(44.0f, 44.0f);
    const FVector2D Centre = ViewModel->GetMapCentre();
    const FVector2D Extent = ViewModel->GetMapExtent();
    int32 EligibleCount = 0;
    for (int32 Index = 0; Index < LocalPins.Num(); ++Index)
    {
        const FKalmalaWorldMapPersonalPin& Pin = LocalPins[Index];
        if (!Pin.bVisible) continue;
        const FVector2D Normalized = WorldToMapNormalized(Pin.WorldLocation, Centre, Extent);
        if (Normalized.X < 0.0f || Normalized.X > 1.0f || Normalized.Y < 0.0f || Normalized.Y > 1.0f) continue;
        ++EligibleCount;
        if (!IsMarkerCategoryVisible(EKalmalaWorldMapMarkerCategory::PersonalPins)) continue;
        const FVector2D Position = Margin + Normalized * MapSize;
        const TArray<FVector2D> Diamond = { Position + FVector2D(0.0f, -8.0f), Position + FVector2D(7.0f, 0.0f),
            Position + FVector2D(0.0f, 8.0f), Position + FVector2D(-7.0f, 0.0f), Position + FVector2D(0.0f, -8.0f) };
        const FLinearColor Colour = GetPinColour(Pin.Style).CopyWithNewOpacity(Pin.bComplete ? 0.45f : 1.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Diamond, ESlateDrawEffect::None, FLinearColor::Black, true, 4.0f);
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Diamond, ESlateDrawEffect::None, Colour, true, 2.0f);
        if (Pin.bComplete) FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
            { Position + FVector2D(-4.0f, 0.0f), Position + FVector2D(-1.0f, 3.0f), Position + FVector2D(5.0f, -4.0f) }, ESlateDrawEffect::None, FLinearColor::White, true, 1.5f);
        if (Index == SelectedPinIndex) FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
            AllottedGeometry.ToPaintGeometry(FVector2D(22.0f, 22.0f), FSlateLayoutTransform(Position - FVector2D(11.0f))),
            FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, 0.2f));
        FSlateDrawElement::MakeText(OutDrawElements, LayerId + 4, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(Position + FVector2D(10.0f, -8.0f))),
            GetPinAccessibilityLabel(Pin),
            FKalmalaUITheme::Get().MakeFont(FKalmalaUITheme::Get().BodySize - 1, false, UKalmalaSettingsWidget::GetTextScalePercent()), ESlateDrawEffect::None, FLinearColor::White);
    }
    return EligibleCount;
}

void UKalmalaWorldMapWidget::PanByScreenDelta(const FVector2D& ScreenDelta, const FVector2D& MapSize)
{
    if (ViewModel == nullptr || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const FVector2D Extent = ViewModel->GetMapExtent();
    const FVector2D WorldDelta(-ScreenDelta.X / MapSize.X * 2.0f * Extent.X, -ScreenDelta.Y / MapSize.Y * 2.0f * Extent.Y);
    bFitWholeWorld = false;
    ViewModel->SetMapCentre(ViewModel->GetMapCentre() + WorldDelta);
    InvalidateOutstandingTileJobs();
}

void UKalmalaWorldMapWidget::ZoomAtScreenPosition(const float WheelDelta, const FVector2D& ScreenPosition, const FVector2D& MapSize)
{
    if (ViewModel == nullptr || FMath::IsNearlyZero(WheelDelta) || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const FVector2D Normalized((ScreenPosition.X / MapSize.X - 0.5f) * 2.0f, (ScreenPosition.Y / MapSize.Y - 0.5f) * 2.0f);
    const FVector2D PinnedWorld = ViewModel->GetMapCentre() + Normalized * ViewModel->GetMapExtent();
    bFitWholeWorld = false;
    MapZoom = ClampMapZoom(MapZoom * (1.0f - WheelDelta * ZoomStep), MinZoom, MaxZoom);
    ViewModel->SetMapRadius(MapZoom);
    ViewModel->SetMapCentre(PinnedWorld - Normalized * ViewModel->GetMapExtent());
    InvalidateOutstandingTileJobs();
}

FReply UKalmalaWorldMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bMapOpen) return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    const FVector2D WidgetPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    if (IsInsideMarkerLegend(WidgetPosition, InGeometry.GetLocalSize()))
    {
        const int32 CategoryIndex = GetMarkerCategoryAtPosition(WidgetPosition, InGeometry.GetLocalSize());
        if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CategoryIndex != INDEX_NONE)
        {
            FocusedMarkerCategoryIndex = CategoryIndex;
            HoveredMarkerCategoryIndex = CategoryIndex;
            bMarkerFilterNavigationActive = false;
            ToggleMarkerCategory(MarkerCategoryFromIndex(CategoryIndex));
            Invalidate(EInvalidateWidget::Paint);
        }
        return FReply::Handled();
    }
    const FVector2D MapSize = InGeometry.GetLocalSize() - FVector2D(88.0f);
    const FVector2D MapPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f);
    if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
    {
        if (!bPinLabelEntry && MapPosition.X >= 0 && MapPosition.Y >= 0 && MapPosition.X <= MapSize.X && MapPosition.Y <= MapSize.Y)
            SendMapPing(ScreenToWorld(MapPosition, MapSize));
        return FReply::Handled();
    }
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (const int32 PinIndex = FindVisiblePinAtScreenPosition(MapPosition, MapSize); PinIndex != INDEX_NONE)
        {
            LocalPins.RemoveAt(PinIndex);
            SelectedPinIndex = LocalPins.IsEmpty() ? INDEX_NONE : FMath::Min(PinIndex, LocalPins.Num() - 1);
            PersistPins();
        }
        return FReply::Handled();
    }
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (const int32 PinIndex = FindVisiblePinAtScreenPosition(MapPosition, MapSize); PinIndex != INDEX_NONE)
        {
            SelectedPinIndex = PinIndex;
            if (InMouseEvent.IsControlDown()) LocalPins[PinIndex].bVisible = !LocalPins[PinIndex].bVisible;
            else LocalPins[PinIndex].bComplete = !LocalPins[PinIndex].bComplete;
            PersistPins();
            return FReply::Handled();
        }
        if (InMouseEvent.IsShiftDown()) { BeginPinPlacement(ScreenToWorld(MapPosition, MapSize)); return FReply::Handled(); }
        bDragging = true; LastDragPosition = MapPosition; return FReply::Handled().CaptureMouse(TakeWidget());
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UKalmalaWorldMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) { bDragging = false; return FReply::Handled().ReleaseMouseCapture(); }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UKalmalaWorldMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bDragging)
    {
        const FVector2D WidgetPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        const int32 HoveredIndex = IsInsideMarkerLegend(WidgetPosition, InGeometry.GetLocalSize())
            ? GetMarkerCategoryAtPosition(WidgetPosition, InGeometry.GetLocalSize()) : INDEX_NONE;
        if (HoveredIndex != HoveredMarkerCategoryIndex)
        {
            HoveredMarkerCategoryIndex = HoveredIndex;
            Invalidate(EInvalidateWidget::Paint);
        }
        return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
    }
    const FVector2D Position = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f);
    PanByScreenDelta(Position - LastDragPosition, InGeometry.GetLocalSize() - FVector2D(88.0f)); LastDragPosition = Position;
    return FReply::Handled();
}

FReply UKalmalaWorldMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (bPinLabelEntry)
    {
        if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom) { CommitPinPlacement(); return FReply::Handled(); }
        if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right) { CancelPinPlacement(); return FReply::Handled(); }
        if (Key == EKeys::BackSpace) { PendingPinLabel.LeftChopInline(1); return FReply::Handled(); }
        if (Key == EKeys::One) { PendingPinStyle = EKalmalaWorldMapPinStyle::Cairn; return FReply::Handled(); }
        if (Key == EKeys::Two) { PendingPinStyle = EKalmalaWorldMapPinStyle::Lantern; return FReply::Handled(); }
        if (Key == EKeys::Three) { PendingPinStyle = EKalmalaWorldMapPinStyle::Thread; return FReply::Handled(); }
        return FReply::Unhandled();
    }
    if (!bMapOpen) return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    if (HandleMarkerFilterKey(Key)) return FReply::Handled();
    if (Key == EKeys::C || Key == EKeys::Gamepad_Special_Right)
    {
        if (!InKeyEvent.IsRepeat())
            if (auto* Awareness = GetOwningPlayer() ? GetOwningPlayer()->FindComponentByClass<UKalmalaMapAwarenessComponent>() : nullptr)
                Awareness->SetSharingEnabled(!Awareness->IsSharingRequested());
        return FReply::Handled();
    }
    if (Key == EKeys::Q || Key == EKeys::Gamepad_RightThumbstick)
    {
        if (!InKeyEvent.IsRepeat() && ViewModel) SendMapPing(ViewModel->GetMapCentre());
        return FReply::Handled();
    }
    if (Key == EKeys::Tab || Key == EKeys::Gamepad_FaceButton_Left) { SelectNextPin(); return FReply::Handled(); }
    if (Key == EKeys::P) { BeginPinPlacement(ScreenToWorld((InGeometry.GetLocalSize() - FVector2D(88.0f)) * 0.5f, InGeometry.GetLocalSize() - FVector2D(88.0f))); return FReply::Handled(); }
    if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        if (!ToggleSelectedPinCompletion()) BeginPinPlacement(ScreenToWorld((InGeometry.GetLocalSize() - FVector2D(88.0f)) * 0.5f, InGeometry.GetLocalSize() - FVector2D(88.0f)));
        return FReply::Handled();
    }
    if (Key == EKeys::H || Key == EKeys::Gamepad_FaceButton_Right) { ToggleSelectedPinVisibility(); return FReply::Handled(); }
    if (Key == EKeys::Delete || Key == EKeys::Gamepad_LeftTrigger) { RemoveSelectedPin(); return FReply::Handled(); }
    if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Top) { Recenter(); return FReply::Handled(); }
    if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left) { PanByKeyboardDelta(FVector2D(64.0f, 0.0f)); return FReply::Handled(); }
    if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right) { PanByKeyboardDelta(FVector2D(-64.0f, 0.0f)); return FReply::Handled(); }
    if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up) { PanByKeyboardDelta(FVector2D(0.0f, -64.0f)); return FReply::Handled(); }
    if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down) { PanByKeyboardDelta(FVector2D(0.0f, 64.0f)); return FReply::Handled(); }
    if (Key == EKeys::PageUp || Key == EKeys::Gamepad_RightShoulder) { ZoomAtMapCentre(1.0f); return FReply::Handled(); }
    if (Key == EKeys::PageDown || Key == EKeys::Gamepad_LeftShoulder) { ZoomAtMapCentre(-1.0f); return FReply::Handled(); }
    return FReply::Unhandled();
}

FReply UKalmalaWorldMapWidget::NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
    if (!bPinLabelEntry) return Super::NativeOnKeyChar(InGeometry, InCharacterEvent);
    const TCHAR Character = InCharacterEvent.GetCharacter();
    if (PendingPinLabel.Len() < MaxPinLabelLength && (FChar::IsAlnum(Character) || Character == TEXT(' ') || Character == TEXT('-') || Character == TEXT('\'')))
    {
        PendingPinLabel.AppendChar(Character);
    }
    return FReply::Handled();
}

FReply UKalmalaWorldMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bMapOpen) return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    ZoomAtScreenPosition(InMouseEvent.GetWheelDelta(), InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f), InGeometry.GetLocalSize() - FVector2D(88.0f));
    return FReply::Handled();
}
