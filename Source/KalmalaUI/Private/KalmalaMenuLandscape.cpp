#include "KalmalaMenuLandscape.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

void UKalmalaMenuLandscape::AddToCanvas(UWidgetTree& Tree, UCanvasPanel& Canvas)
{
    auto* Landscape = Tree.ConstructWidget<UKalmalaMenuLandscape>();
    Landscape->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* Slot = Canvas.AddChildToCanvas(Landscape);
    Slot->SetAnchors(FAnchors(0, 0, 1, 1));
    Slot->SetOffsets(FMargin(0));
}

void UKalmalaMenuLandscape::FitPanel(UBorder* Panel, const float ViewportWidth)
{
    if (!Panel) return;
    if (auto* Slot = Cast<UCanvasPanelSlot>(Panel->Slot))
    {
        Slot->SetAnchors(FAnchors(0, 0, 0, 1));
        Slot->SetOffsets(FMargin(16, 16, FMath::Clamp(ViewportWidth - 32.f, 0.f, 452.f), 16));
    }
}

int32 UKalmalaMenuLandscape::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bEnabled) const
{
    Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, bEnabled);
    const FVector2f Size(Geometry.GetLocalSize());
    TArray<FSlateGradientStop> Sky;
    Sky.Emplace(FVector2f(0, 0), FLinearColor(.025f, .065f, .105f));
    Sky.Emplace(FVector2f(0, Size.Y * .58f), FLinearColor(.43f, .24f, .13f));
    Sky.Emplace(FVector2f(0, Size.Y), FLinearColor(.015f, .035f, .035f));
    FSlateDrawElement::MakeGradient(Elements, ++Layer, Geometry.ToPaintGeometry(), Sky, Orient_Vertical);

    // A single bounded mesh: layered ridgelines, lake, fir silhouettes and a hearth.
    TArray<FSlateVertex> Vertices;
    TArray<SlateIndex> Indices;
    const auto Point = [&Size](float X, float Y) { return FVector2f(X * Size.X, Y * Size.Y); };
    const auto Triangle = [&](FVector2f A, FVector2f B, FVector2f C, FColor Color)
    {
        const SlateIndex Start = static_cast<SlateIndex>(Vertices.Num());
        for (const FVector2f P : { A, B, C })
            Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
                Geometry.GetAccumulatedRenderTransform(), P, FVector2f(.5f, .5f), Color));
        Indices.Append({ Start, static_cast<SlateIndex>(Start + 1), static_cast<SlateIndex>(Start + 2) });
    };
    const auto Quad = [&](float X, float Y, float W, float H, FColor Color)
    {
        Triangle(Point(X,Y), Point(X+W,Y), Point(X+W,Y+H), Color);
        Triangle(Point(X,Y), Point(X+W,Y+H), Point(X,Y+H), Color);
    };
    const auto Disc = [&](float X, float Y, float Radius, FColor Color)
    {
        const FVector2f Center = Point(X, Y);
        for (int32 I = 0; I < 48; ++I)
        {
            const float A = I * 2.f * PI / 48.f, B = (I + 1) * 2.f * PI / 48.f;
            Triangle(Center, Center + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Radius * Size.Y,
                Center + FVector2f(FMath::Cos(B), FMath::Sin(B)) * Radius * Size.Y, Color);
        }
    };
    Disc(.73f, .39f, .105f, FColor(225, 167, 100, 12));
    Disc(.73f, .39f, .075f, FColor(235, 183, 119, 18));
    Disc(.73f, .39f, .043f, FColor(242, 205, 151));
    for (int32 Ridge = 0; Ridge < 4; ++Ridge)
    {
        const FColor Colors[] = { FColor(63,78,83), FColor(41,64,69), FColor(23,48,51), FColor(12,32,35) };
        for (int32 I = 0; I < 32; ++I)
        {
            const float X = I / 32.f, NextX = (I+1) / 32.f;
            const auto Height = [Ridge](float T)
            { return .48f + Ridge * .10f + .065f * FMath::Sin(T * 12.f + Ridge * 2.f)
                + .023f * FMath::Sin(T * 27.f + Ridge); };
            Triangle(Point(X,Height(X)), Point(NextX,Height(NextX)), Point(NextX,1), Colors[Ridge]);
            Triangle(Point(X,Height(X)), Point(NextX,1), Point(X,1), Colors[Ridge]);
        }
    }
    Triangle(Point(.72f,.59f), Point(.40f,1), Point(.92f,1), FColor(45,66,65));
    for (int32 I = 0; I < 25; ++I)
    {
        const float Y = .62f + I * .016f, W = .012f + I * .0025f;
        Quad(.72f-W, Y, W*2, .0012f, FColor(205,164,107, static_cast<uint8>(75-I*2)));
    }
    const auto Tree = [&](float X, float Bottom, float Height, FColor Color)
    {
        Quad(X-.002f, Bottom-Height, .004f, Height, Color);
        for (int32 Tier = 0; Tier < 6; ++Tier)
        {
            const float T = Tier / 6.f, Y = Bottom - Height + T * Height * .7f;
            const float HalfWidth = Height * (.10f + T * .20f) * Size.Y / FMath::Max(Size.X, 1.f);
            Triangle(Point(X,Y), Point(X-HalfWidth,Y+Height*.30f), Point(X+HalfWidth,Y+Height*.30f), Color);
        }
    };
    for (int32 I = 0; I < 42; ++I)
    {
        const float X = I / 41.f;
        if (X > .55f && X < .83f) continue;
        Tree(X, .77f + .035f * FMath::Sin(I*2.f), .12f + .08f * FMath::Abs(FMath::Sin(I*7.f)), FColor(16,40,42));
    }
    for (int32 I = 0; I < 10; ++I)
    {
        const float X = I < 5 ? I * .045f : .86f + (I-5)*.04f;
        Tree(X, 1.06f, .51f + .24f * FMath::Abs(FMath::Cos(I*3.f)), FColor(6,22,25));
    }
    Disc(.80f,.85f,.055f,FColor(237,140,58,18));
    Disc(.80f,.85f,.025f,FColor(237,140,58,35));
    Triangle(Point(.79f,.86f),Point(.803f,.81f),Point(.811f,.86f),FColor(237,150,65));
    Triangle(Point(.796f,.86f),Point(.799f,.836f),Point(.805f,.86f),FColor(255,223,151));
    const auto Resource = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*FCoreStyle::Get().GetBrush("WhiteBrush"));
    FSlateDrawElement::MakeCustomVerts(Elements, ++Layer, Resource, Vertices, Indices, nullptr, 0, 0);
    return Layer;
}
