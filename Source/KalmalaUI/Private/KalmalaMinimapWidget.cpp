#include "KalmalaMinimapWidget.h"

#include "KalmalaMinimapViewModel.h"
#include "KalmalaMinimapRaster.h"
#include "KalmalaUITheme.h"
#include "KalmalaSettingsWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Rendering/DrawElements.h"

void UKalmalaMinimapWidget::InitializeForLocalPlayer(APlayerController* InOwningPlayer, const float InitialZoom)
{
    if (InOwningPlayer == nullptr || !InOwningPlayer->IsLocalController())
    {
        return;
    }

    ViewModel = NewObject<UKalmalaMinimapViewModel>(this);
    ViewModel->Initialize(InOwningPlayer);
    CurrentZoom = ClampZoom(InitialZoom, MinZoom, MaxZoom);
    ViewModel->SetMapRadius(CurrentZoom);
    SetVisibility(ESlateVisibility::HitTestInvisible);
    ViewModel->Refresh();
    UpdateMapTexture();
    UpdateBiomeLabelLayout();
}

void UKalmalaMinimapWidget::ConfigureViewportPlacement()
{
    // Both size and position setters reset anchors in UE 5.8. Set anchors LAST.
    SetDesiredSizeInViewport(FVector2D(MapDiameter, MapDiameter + BiomeHeaderHeight));
    SetPositionInViewport(FVector2D(-ViewportInset, ViewportInset), false);
    SetAlignmentInViewport(FVector2D(1.0f, 0.0f));
    SetAnchorsInViewport(FAnchors(1.0f, 0.0f));
}

FVector2D UKalmalaMinimapWidget::GetStatusGroupViewportPosition() const
{
    if (UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get())
    {
        const FGameViewportWidgetSlot MapSlot = ViewportSubsystem->GetWidgetSlot(this);
        if (MapSlot.Anchors == FAnchors(1.0f, 0.0f)
            && MapSlot.Alignment == FVector2D(1.0f, 0.0f)
            && FMath::IsFinite(MapSlot.Offsets.Left)
            && FMath::IsFinite(MapSlot.Offsets.Right)
            && FMath::IsFinite(MapSlot.Offsets.Top)
            && MapSlot.Offsets.Right > 0.0f)
        {
            // Both widgets share the top-right viewport anchor. Derive the status
            // group's right edge from the actual map slot, including its size/margins.
            return FVector2D(MapSlot.Offsets.Left - MapSlot.Offsets.Right - StatusGroupGap,
                MapSlot.Offsets.Top + BiomeHeaderHeight);
        }
    }

    return GetDefaultStatusGroupViewportPosition(MapDiameter);
}

FVector2D UKalmalaMinimapWidget::GetDefaultStatusGroupViewportPosition(const float InMapDiameter)
{
    const float SafeDiameter = FMath::Max(0.0f, InMapDiameter);
    return FVector2D(-(ViewportInset + SafeDiameter + StatusGroupGap), ViewportInset + BiomeHeaderHeight);
}

void UKalmalaMinimapWidget::UpdateMapTexture()
{
    if (ViewModel == nullptr || !ViewModel->IsReady() || UploadedRevision == ViewModel->GetPresentationRevision()) return;
    TArray<FColor> Pixels = FKalmalaMinimapRaster::BuildPixels(ViewModel->GetTerrainSamples());
    const int32 Side = FMath::RoundToInt(FMath::Sqrt(static_cast<float>(Pixels.Num())));
    if (Pixels.IsEmpty()) return;
    if (MapTexture == nullptr)
    {
        MapTexture = UTexture2D::CreateTransient(Side, Side, PF_B8G8R8A8);
        if (MapTexture == nullptr) return;
        MapTexture->SRGB = true;
        MapTexture->Filter = TF_Bilinear;
        MapTexture->NeverStream = true;
        MapTexture->UpdateResource();
        MapBrush.SetResourceObject(MapTexture);
        MapBrush.ImageSize = FVector2D(Side, Side);
        MapBrush.DrawAs = ESlateBrushDrawType::Image;
    }
    // Null RHI has no resource and UpdateTextureRegions would not run cleanup.
    if (MapTexture->GetResource() == nullptr) return;
    const uint32 ByteCount = Pixels.Num() * sizeof(FColor);
    uint8* Upload = static_cast<uint8*>(FMemory::Malloc(ByteCount));
    FMemory::Memcpy(Upload, Pixels.GetData(), ByteCount);
    auto* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Side, Side);
    MapTexture->UpdateTextureRegions(0, 1, Region, Side * sizeof(FColor), sizeof(FColor), Upload,
        [](uint8* Data, const FUpdateTextureRegion2D* Regions) { FMemory::Free(Data); delete Regions; });
    UploadedRevision = ViewModel->GetPresentationRevision();
}

void UKalmalaMinimapWidget::UpdateBiomeLabelLayout()
{
    if (ViewModel == nullptr || !FSlateApplication::IsInitialized()) return;
    const FString& BiomeLabel = ViewModel->GetCurrentBiomeLabel();
    const int32 TextScale = UKalmalaSettingsWidget::GetTextScalePercent();
    if (BiomeLabel == MeasuredBiomeLabel && TextScale == BiomeLabelTextScale
        && (BiomeLabel.IsEmpty() || (BiomeLabelFont.Size > 0.0f && BiomeLabelTextSize.X > 0.0f))) return;

    MeasuredBiomeLabel = BiomeLabel;
    BiomeLabelTextScale = TextScale;
    BiomeLabelTextSize = FVector2D::ZeroVector;
    if (BiomeLabel.IsEmpty()) return;

    FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
    if (Renderer == nullptr) return;

    const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
    BiomeLabelFont = Theme.MakeFont(Theme.BodySize, true, TextScale);
    const TSharedRef<FSlateFontMeasure> FontMeasure = Renderer->GetFontMeasureService();
    BiomeLabelTextSize = FontMeasure->Measure(BiomeLabel, BiomeLabelFont);
    const float MaximumTextWidth = FMath::Max(1.0f, MapDiameter - 8.0f);
    while ((BiomeLabelTextSize.X > MaximumTextWidth || BiomeLabelTextSize.Y > BiomeLabelHeight - 2.0f)
        && BiomeLabelFont.Size > 8.0f)
    {
        BiomeLabelFont.Size -= 1.0f;
        BiomeLabelTextSize = FontMeasure->Measure(BiomeLabel, BiomeLabelFont);
    }
}

bool UKalmalaMinimapWidget::IsInsideCircularMap(const FVector2D& NormalizedMapPosition)
{
    return NormalizedMapPosition.SizeSquared() <= 1.0f;
}

bool UKalmalaMinimapWidget::IsTopRightPlacementValid(const FVector2D& ViewportSize, const float InMapDiameter, const float Margin, const float UiScale)
{
    if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f || InMapDiameter <= 0.0f || Margin < 0.0f || UiScale <= 0.0f)
    {
        return false;
    }

    const float ScaledDiameter = InMapDiameter * UiScale;
    const float ScaledMargin = Margin * UiScale;
    const FVector2D MapTopLeft(ViewportSize.X - ScaledMargin - ScaledDiameter,
        ScaledMargin + BiomeHeaderHeight * UiScale);
    const FVector2D MapBottomRight = MapTopLeft + FVector2D(ScaledDiameter, ScaledDiameter);
    const float WidgetTop = MapTopLeft.Y - BiomeHeaderHeight * UiScale;
    return MapTopLeft.X >= 0.0f && WidgetTop >= 0.0f && MapBottomRight.X <= ViewportSize.X
        && MapBottomRight.Y <= ViewportSize.Y;
}

float UKalmalaMinimapWidget::ClampZoom(const float RequestedZoom, const float InMinZoom, const float InMaxZoom)
{
    const float SafeMinZoom = FMath::Max(100.0f, FMath::Min(InMinZoom, InMaxZoom));
    const float SafeMaxZoom = FMath::Max(SafeMinZoom, FMath::Max(InMinZoom, InMaxZoom));
    return FMath::Clamp(RequestedZoom, SafeMinZoom, SafeMaxZoom);
}

bool UKalmalaMinimapWidget::ShouldAcceptZoomInput(const bool bCanProcessNormalGameInput)
{
    return bCanProcessNormalGameInput;
}

void UKalmalaMinimapWidget::AdjustZoom(const float WheelDelta)
{
    if (ViewModel == nullptr || FMath::IsNearlyZero(WheelDelta))
    {
        return;
    }

    CurrentZoom = ClampZoom(CurrentZoom - WheelDelta * ZoomStep, MinZoom, MaxZoom);
    ViewModel->SetMapRadius(CurrentZoom);
    ViewModel->Refresh();
    UpdateMapTexture();
}

void UKalmalaMinimapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
#if !UE_BUILD_SHIPPING
    if (bLoggedPaint && !bRequestedVerificationScreenshot)
    {
        VerificationElapsed += InDeltaTime;
        FString ScreenshotPath;
        if (VerificationElapsed >= 3.0f && FParse::Value(FCommandLine::Get(), TEXT("KalmalaMinimapScreenshot="), ScreenshotPath))
        {
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
            bRequestedVerificationScreenshot = true;
        }
    }
#endif
    RefreshAccumulator += InDeltaTime;
    if (ViewModel != nullptr && RefreshAccumulator >= 0.10f)
    {
        RefreshAccumulator = 0.0f;
        ViewModel->Refresh();
        UpdateMapTexture();
        UpdateBiomeLabelLayout();
    }

}

int32 UKalmalaMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    const FVector2D Size(MapDiameter, MapDiameter);
    const FGeometry MapGeometry = AllottedGeometry.MakeChild(Size,
        FSlateLayoutTransform(FVector2D(0.0f, BiomeHeaderHeight)));
    const float Radius = FMath::Min(Size.X, Size.Y) * 0.5f;
    const FVector2D Centre = Size * 0.5f;

    const FString BiomeLabel = ViewModel != nullptr ? ViewModel->GetCurrentBiomeLabel() : FString();
    if (!BiomeLabel.IsEmpty() && BiomeLabelTextSize.X > 0.0f && BiomeLabelFont.Size > 0.0f
        && FSlateApplication::IsInitialized())
    {
        const FKalmalaUITheme& Theme = FKalmalaUITheme::Get();
        const int32 Contrast = UKalmalaSettingsWidget::GetContrastMode();
        const FVector2D TextPosition(MapDiameter - BiomeLabelTextSize.X,
            FMath::Max(0.0f, (BiomeLabelHeight - BiomeLabelTextSize.Y) * 0.5f));
        const FGeometry TextGeometry = AllottedGeometry.MakeChild(BiomeLabelTextSize,
            FSlateLayoutTransform(TextPosition));
        const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();
        FSlateDrawElement::MakeText(OutDrawElements, DrawLayer,
            AllottedGeometry.MakeChild(BiomeLabelTextSize, FSlateLayoutTransform(TextPosition + FVector2D(1.0f))).ToPaintGeometry(), BiomeLabel, BiomeLabelFont,
            ESlateDrawEffect::None, FLinearColor::Black * Tint);
        FSlateDrawElement::MakeText(OutDrawElements, DrawLayer + 1,
            TextGeometry.ToPaintGeometry(), BiomeLabel, BiomeLabelFont, ESlateDrawEffect::None, Theme.TextColor(true, Contrast) * Tint);
    }

    if (MapTexture != nullptr && ViewModel != nullptr && ViewModel->IsReady())
    {
        FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 2, MapGeometry.ToPaintGeometry(),
            &MapBrush, ESlateDrawEffect::None, InWidgetStyle.GetColorAndOpacityTint());
        if (!bLoggedPaint && Radius > 0.0f && FParse::Param(FCommandLine::Get(), TEXT("KalmalaMinimapVerification")))
        {
            const FVector2D TopLeft = MapGeometry.LocalToAbsolute(FVector2D::ZeroVector);
            const FVector2D BottomRight = MapGeometry.LocalToAbsolute(Size);
            UE_LOG(LogTemp, Display, TEXT("Minimap painted: LocalPlayer=%s Size=%.0fx%.0f Bounds=%.0f,%.0f,%.0f,%.0f Samples=%d Biome=%s"),
                *GetNameSafe(GetOwningPlayer()), Size.X, Size.Y, TopLeft.X, TopLeft.Y, BottomRight.X, BottomRight.Y,
                ViewModel->GetTerrainSamples().Num(), *BiomeLabel);
            bLoggedPaint = true;
        }
    }

    TArray<FVector2D> CirclePoints;
    constexpr int32 CircleSegments = 48;
    CirclePoints.Reserve(CircleSegments + 1);
    for (int32 Index = 0; Index <= CircleSegments; ++Index)
    {
        const float Angle = (2.0f * PI * Index) / CircleSegments;
        CirclePoints.Add(Centre + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
    }
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 3, MapGeometry.ToPaintGeometry(), CirclePoints,
        ESlateDrawEffect::None, FLinearColor(0.75f, 0.88f, 0.85f, 0.95f), true, 2.0f);

    const float FacingRadians = ViewModel != nullptr ? FMath::DegreesToRadians(ViewModel->GetPlayerFacingDegrees()) : 0.0f;
    const FVector2D Forward(FMath::Cos(FacingRadians), FMath::Sin(FacingRadians));
    const FVector2D Right(-Forward.Y, Forward.X);
    TArray<FVector2D> MarkerLines;
    MarkerLines.Add(Centre + Forward * 13.0f);
    MarkerLines.Add(Centre - Forward * 8.0f + Right * 7.0f);
    MarkerLines.Add(Centre - Forward * 8.0f - Right * 7.0f);
    MarkerLines.Add(Centre + Forward * 13.0f);
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 4, MapGeometry.ToPaintGeometry(), MarkerLines,
        ESlateDrawEffect::None, FLinearColor(0.025f, 0.035f, 0.04f, 1.0f), true, 5.0f);
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 5, MapGeometry.ToPaintGeometry(), MarkerLines,
        ESlateDrawEffect::None, FLinearColor::White, true, 2.5f);
    return DrawLayer + 5;
}
