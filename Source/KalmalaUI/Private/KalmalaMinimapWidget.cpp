#include "KalmalaMinimapWidget.h"

#include "KalmalaMinimapViewModel.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void UKalmalaMinimapWidget::InitializeForLocalPlayer(APlayerController* InOwningPlayer)
{
    if (InOwningPlayer == nullptr || !InOwningPlayer->IsLocalController())
    {
        return;
    }

    ViewModel = NewObject<UKalmalaMinimapViewModel>(this);
    ViewModel->Initialize(InOwningPlayer);
    SetDesiredSizeInViewport(FVector2D(MapDiameter, MapDiameter));
}

bool UKalmalaMinimapWidget::IsInsideCircularMap(const FVector2D& NormalizedMapPosition)
{
    return NormalizedMapPosition.SizeSquared() <= 1.0f;
}

void UKalmalaMinimapWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshAccumulator += InDeltaTime;
    if (ViewModel != nullptr && RefreshAccumulator >= 0.10f)
    {
        RefreshAccumulator = 0.0f;
        ViewModel->Refresh();
    }
}

int32 UKalmalaMinimapWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, const bool bParentEnabled) const
{
    const int32 DrawLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled) + 1;
    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const float Radius = FMath::Min(Size.X, Size.Y) * 0.5f;
    const FVector2D Centre = Size * 0.5f;
    const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));

    if (ViewModel != nullptr && ViewModel->IsReady())
    {
        const float DotSize = FMath::Max(2.0f, Radius * 0.12f);
        for (const FKalmalaMinimapTerrainSample& Sample : ViewModel->GetTerrainSamples())
        {
            if (!IsInsideCircularMap(Sample.MapPosition))
            {
                continue;
            }

            const FVector2D Point = Centre + Sample.MapPosition * (Radius - DotSize);
            const FLinearColor TerrainColour = Sample.bIsWater
                ? FLinearColor(0.08f, 0.42f, 0.63f, 0.96f)
                : FLinearColor(0.18f, 0.34f + FMath::Clamp(Sample.TerrainHeight / 5500.0f, 0.0f, 0.18f), 0.16f, 0.96f);
            FSlateDrawElement::MakeBox(OutDrawElements, DrawLayer + 1,
                AllottedGeometry.ToPaintGeometry(FVector2f(DotSize, DotSize), FSlateLayoutTransform(FVector2f(Point - FVector2D(DotSize * 0.5f)))),
                WhiteBrush, ESlateDrawEffect::None, TerrainColour);
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
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 2, AllottedGeometry.ToPaintGeometry(), CirclePoints,
        ESlateDrawEffect::None, FLinearColor(0.75f, 0.88f, 0.85f, 0.95f), true, 2.0f);

    const float FacingRadians = ViewModel != nullptr ? FMath::DegreesToRadians(ViewModel->GetPlayerFacingDegrees()) : 0.0f;
    const FVector2D Forward(FMath::Cos(FacingRadians), FMath::Sin(FacingRadians));
    const FVector2D Right(-Forward.Y, Forward.X);
    TArray<FVector2D> MarkerLines;
    MarkerLines.Add(Centre + Forward * 13.0f);
    MarkerLines.Add(Centre - Forward * 8.0f + Right * 7.0f);
    MarkerLines.Add(Centre - Forward * 8.0f - Right * 7.0f);
    MarkerLines.Add(Centre + Forward * 13.0f);
    FSlateDrawElement::MakeLines(OutDrawElements, DrawLayer + 3, AllottedGeometry.ToPaintGeometry(), MarkerLines,
        ESlateDrawEffect::None, FLinearColor::White, true, 2.5f);
    return DrawLayer + 3;
}
