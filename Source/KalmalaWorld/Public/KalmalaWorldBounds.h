#pragma once

#include "KalmalaWorldGenerationConfig.h"

/** Revisioned finite wilderness, centred at world XY zero. Units are centimetres. */
struct FKalmalaWorldBounds
{
    static constexpr double Radius = 1600000.0;
    static bool IsBounded(const FKalmalaWorldGenerationConfig& C) { return C.GeneratorRevision >= 5; }
    static bool Contains(const FKalmalaWorldGenerationConfig& C, FVector2D P, double Margin = 0)
    {
        return !P.ContainsNaN() && (!IsBounded(C) || P.SizeSquared() <= FMath::Square(Radius - Margin));
    }
    static FVector2D Constrain(const FKalmalaWorldGenerationConfig& C, FVector2D P, double Margin = 0)
    {
        return Contains(C, P, Margin) ? P : P.GetSafeNormal() * (Radius - Margin);
    }
    static bool IntersectsPatch(const FKalmalaWorldGenerationConfig& C, FVector2D Centre, double HalfSize)
    {
        return Contains(C, FVector2D(FMath::Max(0.0, FMath::Abs(Centre.X) - HalfSize),
            FMath::Max(0.0, FMath::Abs(Centre.Y) - HalfSize)));
    }
    /** Clip small terrain/water triangles to the circle's signed-distance contour.
     * Interpolated edge points lie inside the circle; patch-edge intersections agree.
     * Interior triangles retain their original collision plane and winding. */
    static void ClipMesh(const FKalmalaWorldGenerationConfig& C, FVector2D Origin,
        TArray<FVector>& Vertices, TArray<int32>& Triangles)
    {
        if (!IsBounded(C)) return;
        TArray<FVector> Clipped;
        TArray<int32> Indices;
        for (int32 I = 0; I < Triangles.Num(); I += 3)
        {
            TArray<FVector, TInlineAllocator<4>> Polygon;
            for (int32 Edge = 0; Edge < 3; ++Edge)
            {
                const FVector A = Vertices[Triangles[I + Edge]], B = Vertices[Triangles[I + (Edge + 1) % 3]];
                const double DA = Radius - (Origin + FVector2D(A)).Size();
                const double DB = Radius - (Origin + FVector2D(B)).Size();
                if (DA >= 0) Polygon.Add(A);
                if ((DA >= 0) != (DB >= 0)) Polygon.Add(FMath::Lerp(A, B, DA / (DA - DB)));
            }
            for (int32 V = 1; V + 1 < Polygon.Num(); ++V)
            {
                const int32 Start = Clipped.Num();
                Clipped.Append({Polygon[0], Polygon[V], Polygon[V + 1]});
                Indices.Append({Start, Start + 1, Start + 2});
            }
        }
        Vertices = MoveTemp(Clipped);
        Triangles = MoveTemp(Indices);
    }
};
