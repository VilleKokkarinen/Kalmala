#include "KalmalaWorldMapWidget.h"

#include "Components/CanvasPanel.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Input/Reply.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaMinimapViewModel.h"
#include "Rendering/DrawElements.h"

void UKalmalaWorldMapWidget::InitializeForLocalPlayer(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr || !InOwningPlayer->IsLocalController()) return;
    SetOwningPlayer(InOwningPlayer);
    ViewModel = NewObject<UKalmalaMinimapViewModel>(this);
    ViewModel->Initialize(InOwningPlayer);
    ViewModel->SetMapRadius(MapZoom);
    SetIsFocusable(true);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UKalmalaWorldMapWidget::ConfigureViewportPlacement()
{
    SetAlignmentInViewport(FVector2D::ZeroVector);
    SetPositionInViewport(FVector2D::ZeroVector, false);
    SetDesiredSizeInViewport(FVector2D(1920.0f, 1080.0f));
    SetAnchorsInViewport(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
}

void UKalmalaWorldMapWidget::Open()
{
    if (GetOwningPlayer() == nullptr || ViewModel == nullptr) return;
    bMapOpen = true;
    Recenter();
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
    ViewModel->RecenterOnOwningPlayer();
    ViewModel->SetMapRadius(MapZoom);
    ViewModel->Refresh();
}

float UKalmalaWorldMapWidget::ClampMapZoom(const float RequestedZoom, const float InMinZoom, const float InMaxZoom)
{
    return FMath::Clamp(RequestedZoom, FMath::Min(InMinZoom, InMaxZoom), FMath::Max(InMinZoom, InMaxZoom));
}

void UKalmalaWorldMapWidget::UpdateMapTexture()
{
    if (ViewModel == nullptr || !ViewModel->IsReady() || UploadedRevision == ViewModel->GetPresentationRevision()) return;
    TArray<FColor> Pixels = FKalmalaMinimapRaster::BuildPixels(ViewModel->GetTerrainSamples());
    const int32 Side = FMath::RoundToInt(FMath::Sqrt(static_cast<float>(Pixels.Num())));
    if (Pixels.IsEmpty()) return;
    if (MapTexture == nullptr)
    {
        MapTexture = UTexture2D::CreateTransient(Side, Side, PF_B8G8R8A8);
        if (MapTexture == nullptr) return;
        MapTexture->SRGB = true; MapTexture->Filter = TF_Bilinear; MapTexture->NeverStream = true; MapTexture->UpdateResource();
        MapBrush.SetResourceObject(MapTexture); MapBrush.ImageSize = FVector2D(Side, Side); MapBrush.DrawAs = ESlateBrushDrawType::Image;
    }
    if (MapTexture->GetResource() == nullptr) return;
    const uint32 ByteCount = Pixels.Num() * sizeof(FColor);
    uint8* Upload = static_cast<uint8*>(FMemory::Malloc(ByteCount)); FMemory::Memcpy(Upload, Pixels.GetData(), ByteCount);
    auto* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Side, Side);
    MapTexture->UpdateTextureRegions(0, 1, Region, Side * sizeof(FColor), sizeof(FColor), Upload,
        [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
    UploadedRevision = ViewModel->GetPresentationRevision();
}

void UKalmalaWorldMapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bMapOpen || ViewModel == nullptr) return;
    RefreshAccumulator += InDeltaTime;
    if (RefreshAccumulator >= 0.10f) { RefreshAccumulator = 0.0f; ViewModel->Refresh(); UpdateMapTexture(); }
}

int32 UKalmalaWorldMapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FMargin Margin(44.0f);
    const FVector2D MapSize(FMath::Max(1.0f, Size.X - Margin.Left - Margin.Right), FMath::Max(1.0f, Size.Y - Margin.Top - Margin.Bottom));
    const FPaintGeometry MapGeometry = AllottedGeometry.ToPaintGeometry(MapSize, FSlateLayoutTransform(FVector2D(Margin.Left, Margin.Top)));
    FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer, AllottedGeometry.ToPaintGeometry(), FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None, FLinearColor(0.008f, 0.018f, 0.025f, 0.96f));
    if (MapTexture != nullptr && ViewModel != nullptr && ViewModel->IsReady())
    {
        FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 1, MapGeometry, &MapBrush, ESlateDrawEffect::None, FLinearColor::White);
    }
    FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 2, MapGeometry, FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None, FLinearColor(0.55f, 0.75f, 0.68f, 0.9f));
    const FVector2D Centre = FVector2D(Margin.Left, Margin.Top) + MapSize * 0.5f;
    TArray<FVector2D> Cross;
    Cross.Add(Centre - FVector2D(12.0f, 0.0f)); Cross.Add(Centre + FVector2D(12.0f, 0.0f));
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 3, AllottedGeometry.ToPaintGeometry(), Cross, ESlateDrawEffect::None, FLinearColor::White, true, 2.0f);
    Cross = { Centre - FVector2D(0.0f, 12.0f), Centre + FVector2D(0.0f, 12.0f) };
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 3, AllottedGeometry.ToPaintGeometry(), Cross, ESlateDrawEffect::None, FLinearColor::White, true, 2.0f);
    const FString Hint = FString::Printf(TEXT("MAP  |  %.0fm  |  Drag to pan · Wheel to zoom · R to recenter · M / Esc to close"), MapZoom / 100.0f);
    FSlateDrawElement::MakeText(OutDrawElements, DrawLayer + 4, AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(FVector2D(20.0f, 18.0f))), Hint,
        FCoreStyle::GetDefaultFontStyle("Regular", 16), ESlateDrawEffect::None, FLinearColor(0.85f, 0.91f, 0.87f, 1.0f));
    return DrawLayer + 4;
}

void UKalmalaWorldMapWidget::PanByScreenDelta(const FVector2D& ScreenDelta, const FVector2D& MapSize)
{
    if (ViewModel == nullptr || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const FVector2D WorldDelta(-ScreenDelta.X / MapSize.X * 2.0f * MapZoom, -ScreenDelta.Y / MapSize.Y * 2.0f * MapZoom);
    ViewModel->SetMapCentre(ViewModel->GetMapCentre() + WorldDelta);
    ViewModel->Refresh();
}

void UKalmalaWorldMapWidget::ZoomAtScreenPosition(const float WheelDelta, const FVector2D& ScreenPosition, const FVector2D& MapSize)
{
    if (ViewModel == nullptr || FMath::IsNearlyZero(WheelDelta) || MapSize.X <= 0.0f || MapSize.Y <= 0.0f) return;
    const FVector2D Normalized((ScreenPosition.X / MapSize.X - 0.5f) * 2.0f, (ScreenPosition.Y / MapSize.Y - 0.5f) * 2.0f);
    const FVector2D PinnedWorld = ViewModel->GetMapCentre() + Normalized * MapZoom;
    MapZoom = ClampMapZoom(MapZoom * (1.0f - WheelDelta * ZoomStep), MinZoom, MaxZoom);
    ViewModel->SetMapRadius(MapZoom);
    ViewModel->SetMapCentre(PinnedWorld - Normalized * MapZoom);
    ViewModel->Refresh();
}

FReply UKalmalaWorldMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bMapOpen && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) { bDragging = true; LastDragPosition = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()); return FReply::Handled().CaptureMouse(TakeWidget()); }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UKalmalaWorldMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) { bDragging = false; return FReply::Handled().ReleaseMouseCapture(); }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UKalmalaWorldMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bDragging) return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
    const FVector2D Position = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    PanByScreenDelta(Position - LastDragPosition, InGeometry.GetLocalSize() - FVector2D(88.0f)); LastDragPosition = Position;
    return FReply::Handled();
}

FReply UKalmalaWorldMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bMapOpen) return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
    ZoomAtScreenPosition(InMouseEvent.GetWheelDelta(), InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition()) - FVector2D(44.0f), InGeometry.GetLocalSize() - FVector2D(88.0f));
    return FReply::Handled();
}
