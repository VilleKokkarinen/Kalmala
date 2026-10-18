#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaRegionalGeneration.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/AutomationTest.h"

// Search actual spline centers rather than a coarse lattice that can miss a channel.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaHighGroundRiverCenterTest,
    "Kalmala.World.Regional.HighGroundRiverCenter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaHighGroundRiverCenterTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig Config{418ull};
    constexpr int32 SearchRadius = 24;
    for (int32 Y = -SearchRadius; Y <= SearchRadius; ++Y)
    for (int32 X = -SearchRadius; X <= SearchRadius; ++X)
    for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
    {
        if (Segment.bStream) continue;
        const FVector2D Position((Segment.A + Segment.B) * 0.5);
        const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
        const float SourceHeight = (Fields.Elevation - T::SeaElevation) * 2000.0f;
        if (SourceHeight < 400.0f) continue;
        const auto Sample = G::Sample(Fields);
        // These biome weights are computed before hydrology. Zero lake/mire
        // eligibility excludes all basin support, including its outer rim,
        // without selecting the fixture by its resulting water level or bed.
        if (Sample.Weights[1] + Sample.Weights[3] > 0.0f) continue;

        TestTrue(TEXT("Fixture source terrain is at least four metres above sea"), SourceHeight >= 400.0f);
        TestTrue(TEXT("Generated segment midpoint has full river influence"), Sample.RiverWeight > 0.999f);
        TestEqual(TEXT("Fixture has no lake basin influence"), Sample.BasinWeight, 0.0f);
        TestEqual(TEXT("High-ground river water stays at Z=0"), Sample.WaterLevel, 0.0f);
        TestTrue(TEXT("High-ground river bed is below its water plane"), Sample.Height < Sample.WaterLevel);
        TestTrue(TEXT("Land clamp cannot refill the configured river bed"),
            Sample.Height <= -float(T::RiverBedDepth) + 0.01f);
        TestTrue(TEXT("High-ground river center reports water"), Sample.bHasWater);
        AddInfo(FString::Printf(TEXT("High-ground river seed=418 position=(%.3f,%.3f) source=%.3f bed=%.3f water=%.3f"),
            Position.X, Position.Y, SourceHeight, Sample.Height, Sample.WaterLevel));
        return true;
    }
    AddError(TEXT("No basin-free high-ground river center found in the bounded seed-418 search"));
    return false;
}

// Compare the real sampler against the same land shaping with courses omitted.
// Eligibility is decided before assertions, never by the observed carved height.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverBankCrossSectionTest,
    "Kalmala.World.Regional.RiverBankCrossSection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverBankCrossSectionTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig Config{418ull};
    struct FPoint { FVector2D Position; FKalmalaRegionalSample Base; double Influence; };
    constexpr int32 Steps = 64;
    for (int32 Y = -24; Y <= 24; ++Y)
    for (int32 X = -24; X <= 24; ++X)
    for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
    {
        if (Segment.bStream) continue;
        const FVector2D Center((Segment.A + Segment.B) * 0.5);
        const auto CenterBase = G::SampleInternal(FKalmalaWorldFieldSampler::Sample(Config, Center), false, true);
        if (CenterBase.Height < 400 || CenterBase.Weights[1] + CenterBase.Weights[3] > 0) continue;
        const FVector2D Direction = FVector2D(Segment.B - Segment.A).GetSafeNormal();
        if (Direction.IsNearlyZero()) continue;
        const FVector2D Normal(-Direction.Y, Direction.X);
        const double Radius = Segment.Width + T::RiverBankExtent;
        TArray<FPoint> Points;
        bool bEligible = true;
        // Include the entire shoulder plus a substantial unchanged exterior.
        for (int32 I = -Steps; I <= Steps; ++I)
        {
            const FVector2D P = Center + Normal * (1.5 * Radius * I / Steps);
            const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, P);
            const auto Base = G::SampleInternal(Fields, false, true);
            if (Base.Weights[1] + Base.Weights[3] > 0 || Base.Height < 400)
            { bEligible = false; break; }
            double Influence = 0;
            const FIntPoint Cell(FMath::FloorToInt(P.X / T::GridCell), FMath::FloorToInt(P.Y / T::GridCell));
            for (const auto& S : G::GetHydrology(Config, Cell))
            {
                if (S.bStream) continue;
                const FVector2D A(S.A), Delta = FVector2D(S.B) - A;
                const double U = FMath::Clamp(FVector2D::DotProduct(P - A, Delta) / Delta.SizeSquared(), 0.0, 1.0);
                const double Distance = (P - A - Delta * U).Size();
                const double Taper = FMath::Clamp((Distance - S.Width * 0.2)
                    / (S.Width + T::RiverBankExtent - S.Width * 0.2), 0.0, 1.0);
                Influence = FMath::Max(Influence, 1 - Taper * Taper * (3 - 2 * Taper));
            }
            Points.Add({P, Base, Influence});
        }
        if (!bEligible || Points.Num() != 2 * Steps + 1) continue;
        // Avoid neighboring bends/courses that hide the outer edge of either bank.
        if (Points[0].Influence != 0 || Points.Last().Influence != 0) continue;
        bool bMonotone = true;
        for (int32 I = 1; I <= Steps; ++I)
            bMonotone &= Points[Steps - I].Influence <= Points[Steps - I + 1].Influence
                && Points[Steps + I].Influence <= Points[Steps + I - 1].Influence;
        if (!bMonotone) continue;

        int32 Shoulder[2] = {}, Exterior[2] = {};
        for (int32 I = 0; I < Points.Num(); ++I)
        {
            const auto& P = Points[I];
            const auto Actual = G::Sample(Config, P.Position);
            const float Expected = FMath::Lerp(P.Base.Height, -float(T::RiverBedDepth), float(P.Influence));
            TestTrue(TEXT("Both banks follow the continuous cubic shoulder"),
                FMath::IsNearlyEqual(Actual.Height, Expected, 0.02f));
            const int32 Side = I < Steps ? 0 : 1;
            if (P.Influence > 0 && P.Influence < 1) ++Shoulder[Side];
            if (P.Influence == 0)
            {
                ++Exterior[Side];
                TestEqual(TEXT("Beyond either bank terrain is exactly unchanged"), Actual.Height, P.Base.Height);
                TestEqual(TEXT("Beyond either bank water output is unchanged"), Actual.WaterLevel, P.Base.WaterLevel);
                TestEqual(TEXT("Beyond either bank water presence is unchanged"), Actual.bHasWater, P.Base.bHasWater);
            }
            // Tiny physical steps test continuity throughout each bank and its return.
            for (const double Sign : {-1.0, 1.0})
            {
                const auto Nearby = G::Sample(Config, P.Position + Normal * (Sign * 0.01));
                TestTrue(TEXT("Bank and exterior have no height step"), FMath::Abs(Nearby.Height - Actual.Height) < 0.1f);
            }
        }
        for (int32 Side = 0; Side < 2; ++Side)
        {
            TestTrue(TEXT("Each bank includes multiple shoulder samples"), Shoulder[Side] > 8);
            TestTrue(TEXT("Each bank includes multiple unchanged exterior samples"), Exterior[Side] > 4);
        }
        AddInfo(FString::Printf(TEXT("River cross-section seed=418 center=(%.3f,%.3f) radius=%.3f samples=%d"),
            Center.X, Center.Y, Radius, Points.Num()));
        return true;
    }
    AddError(TEXT("No high-ground basin-free two-bank cross-section found in bounded seed-418 search"));
    return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverSeedRepeatabilityTest,
    "Kalmala.World.Regional.RiverSeedRepeatability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverSeedRepeatabilityTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    const FKalmalaWorldGenerationConfig Config{418ull}, OtherConfig{419ull};
    TArray<FVector2D> Positions;
    TSet<uint64> Courses;
    G::ClearHydrologyCache();
    // Use actual distinct courses so narrow rivers cannot fall between grid probes.
    for (int32 Y = -24; Y <= 24 && Courses.Num() < 8; ++Y)
    for (int32 X = -24; X <= 24 && Courses.Num() < 8; ++X)
    for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
    {
        if (Segment.bStream || Courses.Contains(Segment.Id)) continue;
        const FVector2D Direction = FVector2D(Segment.B - Segment.A).GetSafeNormal();
        if (Direction.IsNearlyZero()) continue;
        const FVector2D Center((Segment.A + Segment.B) * 0.5);
        if (G::Sample(Config, Center).RiverWeight < 0.999f) continue;
        Courses.Add(Segment.Id);
        const FVector2D Normal(-Direction.Y, Direction.X);
        for (const double Offset : {-1.5, -0.5, 0.0, 0.5, 1.5})
            Positions.Add(Center + Normal * ((Segment.Width + T::RiverBankExtent) * Offset));
        if (Courses.Num() == 8) break;
    }
    if (!TestEqual(TEXT("Bounded search supplies eight distinct river courses"), Courses.Num(), 8)) return false;

    TArray<FKalmalaRegionalSample> Baseline;
    for (const FVector2D& P : Positions) Baseline.Add(G::Sample(Config, P));
    const auto CheckRepeat = [this](const FKalmalaRegionalSample& A, const FKalmalaRegionalSample& B)
    {
        TestEqual(TEXT("Same seed reproduces exact river height"), B.Height, A.Height);
        TestEqual(TEXT("Same seed reproduces exact water level"), B.WaterLevel, A.WaterLevel);
        TestEqual(TEXT("Same seed reproduces water presence"), B.bHasWater, A.bHasWater);
        TestEqual(TEXT("Same seed reproduces river influence"), B.RiverWeight, A.RiverWeight);
    };
    for (int32 I = 0; I < Positions.Num(); ++I)
        CheckRepeat(Baseline[I], G::Sample(Config, Positions[I]));

    G::ClearHydrologyCache();
    int32 HeightChanges = 0, RiverChanges = 0;
    // Reverse traversal after cache disposal and interleave a different seed.
    // Compare identical world positions, not independently relocated fixtures.
    for (int32 I = Positions.Num() - 1; I >= 0; --I)
    {
        const auto Other = G::Sample(OtherConfig, Positions[I]);
        HeightChanges += FMath::Abs(Other.Height - Baseline[I].Height) > 0.01f;
        RiverChanges += FMath::Abs(Other.RiverWeight - Baseline[I].RiverWeight) > 0.001f;
        CheckRepeat(Baseline[I], G::Sample(Config, Positions[I]));
    }
    TestTrue(TEXT("Different seed changes river-area terrain at fixed positions"), HeightChanges > 0);
    TestTrue(TEXT("Different seed moves river influence at fixed positions"), RiverChanges > 0);
    // Sea-level water is deliberately invariant; seed variation must not require
    // its plane to move. Height, wet/dry presence and water level all repeat above.
    AddInfo(FString::Printf(TEXT("River repeatability seeds=418/419 courses=%d samples=%d heightChanges=%d riverChanges=%d"),
        Courses.Num(), Positions.Num(), HeightChanges, RiverChanges));
    G::ClearHydrologyCache();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverUnchangedControlsTest,
    "Kalmala.World.Regional.RiverUnchangedControls",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverUnchangedControlsTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    for (const uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        int32 Land = 0, Sea = 0, CoastLand = 0, CoastSea = 0, Lakes = 0;
        // A bounded 50 m lattice covers both sides of the source coast and the
        // lake-eligible ring. Fixture selection never examines carved outputs.
        for (int32 Y = -120; Y <= 120; ++Y)
        for (int32 X = -120; X <= 120; ++X)
        {
            if (Land >= 8 && Sea >= 8 && CoastLand >= 8 && CoastSea >= 8 && Lakes >= 8) break;
            const FVector2D P(X * 5000.0, Y * 5000.0);
            const auto Fields = FKalmalaWorldFieldSampler::Sample(Config, P);
            const auto Base = G::SampleInternal(Fields, false, true);
            const float Source = (Fields.Elevation - T::SeaElevation) * 2000.0f;
            bool bOutside = true;
            const FIntPoint Cell(FMath::FloorToInt(P.X / T::GridCell), FMath::FloorToInt(P.Y / T::GridCell));
            for (const auto& S : G::GetHydrology(Config, Cell))
            {
                const FVector2D A(S.A), Delta = FVector2D(S.B) - A;
                const double U = FMath::Clamp(FVector2D::DotProduct(P - A, Delta) / Delta.SizeSquared(), 0.0, 1.0);
                if ((P - A - Delta * U).Size() <= S.Width + (S.bStream ? 0.0 : T::RiverBankExtent))
                    bOutside = false;
            }
            // BasinWeight is measured before courses; a positive value and a
            // wet baseline identify an inland bowl independently of carving.
            if (Lakes < 8 && Base.BasinWeight > 0 && Base.WaterLevel > 0 && Base.bHasWater)
            {
                const auto Actual = G::Sample(Config, P);
                TestEqual(TEXT("Enclosed lake retains its established water level"), Actual.WaterLevel, Base.WaterLevel);
                TestTrue(TEXT("Enclosed lake remains wet"), Actual.bHasWater);
                if (bOutside || Base.BasinWeight == 1.0f)
                    TestEqual(TEXT("Uninfluenced or fully supported lake retains its bed"), Actual.Height, Base.Height);
                ++Lakes;
            }
            if (!bOutside || Base.BasinWeight > 0) continue;
            int32& Count = FMath::Abs(Source) <= 20.0f
                ? (Source > 0 ? CoastLand : CoastSea) : (Source > 0 ? Land : Sea);
            if (Count >= 8) continue;
            const auto Actual = G::Sample(Config, P);
            TestEqual(TEXT("Outside course envelopes terrain is exactly unchanged"), Actual.Height, Base.Height);
            TestEqual(TEXT("Outside course envelopes water level is exactly unchanged"), Actual.WaterLevel, Base.WaterLevel);
            TestEqual(TEXT("Outside course envelopes wet state is unchanged"), Actual.bHasWater, Base.bHasWater);
            TestEqual(TEXT("Source coast retains its land/sea terrain sign"), Actual.Height > 0, Source > 0);
            TestEqual(TEXT("Source coast retains its biome identity"), Actual.Biome, Base.Biome);
            for (int32 I = 0; I < 7; ++I)
                TestEqual(TEXT("Control biome weights remain unchanged"), Actual.Weights[I], Base.Weights[I]);
            ++Count;
        }
        TestEqual(TEXT("Away-from-river land controls exist"), Land, 8);
        TestEqual(TEXT("Away-from-river sea controls exist"), Sea, 8);
        TestEqual(TEXT("Source coast has eight land-side controls within 20 cm"), CoastLand, 8);
        TestEqual(TEXT("Source coast has eight sea-side controls within 20 cm"), CoastSea, 8);
        TestEqual(TEXT("Eight wet enclosed-lake controls exist"), Lakes, 8);
        AddInfo(FString::Printf(TEXT("River controls seed=%llu land=%d sea=%d coastLand=%d coastSea=%d lakes=%d"),
            Seed, Land, Sea, CoastLand, CoastSea, Lakes));
    }
    G::ClearHydrologyCache();
    return true;
}

// Locate real course/grid intersections; arbitrary seam probes can miss rivers.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverHydrologyGridSeamTest,
    "Kalmala.World.Regional.RiverHydrologyGridSeam",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverHydrologyGridSeamTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    for (const uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        bool Found[2] = {};
        for (int32 Y = -24; Y <= 24 && !(Found[0] && Found[1]); ++Y)
        for (int32 X = -24; X <= 24 && !(Found[0] && Found[1]); ++X)
        for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
        {
            if (Segment.bStream) continue;
            const FVector2D A(Segment.A), Delta = FVector2D(Segment.B) - A;
            for (int32 Axis = 0; Axis < 2; ++Axis)
            {
                if (Found[Axis]) continue;
                const double Start = Axis == 0 ? A.X : A.Y;
                const double Span = Axis == 0 ? Delta.X : Delta.Y;
                if (FMath::Abs(Span) < 0.001) continue;
                const double Boundary = (FMath::FloorToInt(FMath::Min(Start, Start + Span) / T::GridCell) + 1) * T::GridCell;
                const double U = (Boundary - Start) / Span;
                if (U <= 0 || U >= 1) continue;
                FVector2D Center = A + Delta * U;
                if (Axis == 0) Center.X = Boundary; else Center.Y = Boundary;
                const FVector2D Normal = Axis == 0 ? FVector2D(1, 0) : FVector2D(0, 1);
                // Pre-hydrology biome eligibility excludes lake support without
                // filtering on the expected channel height or water results.
                bool bEligible = true;
                for (const double Offset : {-0.1, 0.0, 0.1})
                {
                    const auto R = G::Sample(Config, Center + Normal * Offset);
                    if (R.Weights[1] + R.Weights[3] > 0) bEligible = false;
                }
                if (!bEligible) continue;
                const auto CellAt = [](FVector2D P)
                { return FIntPoint(FMath::FloorToInt(P.X / T::GridCell), FMath::FloorToInt(P.Y / T::GridCell)); };
                TestTrue(TEXT("River fixture straddles distinct hydrology cells"),
                    CellAt(Center - Normal * 0.01) != CellAt(Center + Normal * 0.01));
                const auto Middle = G::Sample(Config, Center);
                TestTrue(TEXT("Grid seam intersects the river center"), Middle.RiverWeight > 0.999f);
                TestEqual(TEXT("Seam center water is sea level"), Middle.WaterLevel, 0.0f);
                TestTrue(TEXT("Seam center is submerged"), Middle.bHasWater && Middle.Height < Middle.WaterLevel);
                for (const double Epsilon : {0.01, 0.1})
                {
                    G::ClearHydrologyCache();
                    const auto Before = G::Sample(Config, Center - Normal * Epsilon);
                    const auto After = G::Sample(Config, Center + Normal * Epsilon);
                    TestTrue(TEXT("Channel height is continuous across hydrology grid"), FMath::Abs(Before.Height - After.Height) < 0.5f);
                    TestEqual(TEXT("Channel water level agrees across hydrology grid"), Before.WaterLevel, After.WaterLevel);
                    TestEqual(TEXT("Both sides retain sea-level water"), Before.WaterLevel, 0.0f);
                    TestTrue(TEXT("Both sides retain submerged channel water"), Before.bHasWater && After.bHasWater
                        && Before.Height < Before.WaterLevel && After.Height < After.WaterLevel);
                    TestTrue(TEXT("River signal is continuous across hydrology grid"), FMath::Abs(Before.RiverWeight - After.RiverWeight) < 0.001f);
                    G::ClearHydrologyCache();
                    const auto ReverseAfter = G::Sample(Config, Center + Normal * Epsilon);
                    const auto ReverseBefore = G::Sample(Config, Center - Normal * Epsilon);
                    TestEqual(TEXT("Reverse query order reproduces before height"), ReverseBefore.Height, Before.Height);
                    TestEqual(TEXT("Reverse query order reproduces after height"), ReverseAfter.Height, After.Height);
                    TestEqual(TEXT("Reverse query order reproduces before water"), ReverseBefore.WaterLevel, Before.WaterLevel);
                    TestEqual(TEXT("Reverse query order reproduces after water"), ReverseAfter.WaterLevel, After.WaterLevel);
                    TestEqual(TEXT("Reverse query order reproduces before wet flag"), ReverseBefore.bHasWater, Before.bHasWater);
                    TestEqual(TEXT("Reverse query order reproduces after wet flag"), ReverseAfter.bHasWater, After.bHasWater);
                }
                Found[Axis] = true;
                AddInfo(FString::Printf(TEXT("River grid seam seed=%llu axis=%d position=(%.3f,%.3f)"), Seed, Axis, Center.X, Center.Y));
            }
        }
        TestTrue(TEXT("Bounded search finds an X-grid river crossing"), Found[0]);
        TestTrue(TEXT("Bounded search finds a Y-grid river crossing"), Found[1]);
    }
    G::ClearHydrologyCache();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRiverTerrainPatchSeamTest,
    "Kalmala.World.Regional.RiverTerrainPatchSeam",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRiverTerrainPatchSeamTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    using T = FKalmalaRegionalTuning;
    using L = FKalmalaTerrainPatchLayout;
    constexpr double Size = L::PatchSize;
    constexpr double Half = Size * 0.5;
    constexpr double Step = Size / FKalmalaWaterSurfaceMesh::CellsPerSide;
    for (const uint64 Seed : {418ull, 419ull})
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        bool Found[2] = {};
        for (int32 Y = -24; Y <= 24 && !(Found[0] && Found[1]); ++Y)
        for (int32 X = -24; X <= 24 && !(Found[0] && Found[1]); ++X)
        for (const auto& Segment : G::GetHydrology(Config, FIntPoint(X, Y)))
        {
            if (Segment.bStream) continue;
            const FVector2D A(Segment.A), Delta = FVector2D(Segment.B) - A;
            for (int32 Axis = 0; Axis < 2; ++Axis)
            {
                if (Found[Axis]) continue;
                const double Start = Axis == 0 ? A.X : A.Y;
                const double Span = Axis == 0 ? Delta.X : Delta.Y;
                if (FMath::Abs(Span) < 0.001) continue;
                // Zero-origin patches have edges at n*Size + Half.
                const double Boundary = (FMath::FloorToInt((FMath::Min(Start, Start + Span) - Half) / Size) + 1) * Size + Half;
                const double U = (Boundary - Start) / Span;
                if (U <= 0 || U >= 1) continue;
                FVector2D Center = A + Delta * U;
                if (Axis == 0) Center.X = Boundary; else Center.Y = Boundary;
                const FVector2D Normal = Axis == 0 ? FVector2D(1, 0) : FVector2D(0, 1);
                const FIntPoint Key = L::GetPatchCoordinate(FVector2D::ZeroVector, Center - Normal * 0.1);
                const FVector2D LeftCenter = L::GetPatchCenter(FVector2D::ZeroVector, Key.X, Key.Y);
                const FVector2D RightCenter = LeftCenter + Normal * Size;
                // Exclude basin support at the two seam lattice endpoints and
                // their neighboring columns, without filtering carved outputs.
                const double Along = Axis == 0 ? Center.Y : Center.X;
                const double AlongStart = FMath::FloorToDouble((Along + Half) / Step) * Step - Half;
                bool bEligible = true;
                for (const double Across : {-Step, 0.0, Step})
                for (const double Tangent : {AlongStart, AlongStart + Step})
                {
                    const FVector2D P = Axis == 0 ? FVector2D(Boundary + Across, Tangent) : FVector2D(Tangent, Boundary + Across);
                    const auto R = G::Sample(Config, P);
                    if (R.Weights[1] + R.Weights[3] > 0) bEligible = false;
                }
                if (!bEligible) continue;
                TestTrue(TEXT("Fixture crosses distinct terrain patches"), Key != L::GetPatchCoordinate(FVector2D::ZeroVector, Center + Normal * 0.1));
                const auto Before = G::Sample(Config, Center - Normal * 0.01);
                const auto After = G::Sample(Config, Center + Normal * 0.01);
                TestTrue(TEXT("Channel height is continuous across patch seam"), FMath::Abs(Before.Height - After.Height) < 0.5f);
                TestTrue(TEXT("Both patch sides retain submerged sea-level channel"), Before.Height < 0 && After.Height < 0
                    && Before.bHasWater && After.bHasWater && Before.WaterLevel == 0 && After.WaterLevel == 0);
                // Explicit origins exercise the actual collision triangle lattice
                // from both adjacent patches, including queries on either side.
                for (const double Offset : {-0.01, 0.0, 0.01})
                {
                    const FVector2D P = Center + Normal * Offset;
                    const auto Left = FKalmalaOceanSampler::Sample(Config, P, LeftCenter - FVector2D(Half, Half));
                    const auto Right = FKalmalaOceanSampler::Sample(Config, P, RightCenter - FVector2D(Half, Half));
                    TestTrue(TEXT("Both patch triangle samples are valid and wet"), Left.IsWater() && Right.IsWater());
                    TestEqual(TEXT("Adjacent patch origins reproduce channel triangle height"), Left.TerrainHeight, Right.TerrainHeight);
                    TestEqual(TEXT("Adjacent patch origins reproduce sea depth"), Left.WaterDepth, Right.WaterDepth);
                }
                const auto Left = FKalmalaWaterSurfaceMesh::BuildPatch(Config, LeftCenter, false);
                const auto Right = FKalmalaWaterSurfaceMesh::BuildPatch(Config, RightCenter, false);
                const auto Edge = [&](const FKalmalaWaterMesh& Mesh, FVector2D Origin)
                {
                    TArray<FVector> Result;
                    for (const FVector& V : Mesh.Vertices)
                    {
                        const FVector World = V + FVector(Origin, 0);
                        const double Across = Axis == 0 ? World.X : World.Y;
                        const double Tangent = Axis == 0 ? World.Y : World.X;
                        if (FMath::Abs(Across - Boundary) < 0.001 && Tangent >= AlongStart - 0.001 && Tangent <= AlongStart + Step + 0.001)
                            Result.AddUnique(World);
                    }
                    return Result;
                };
                const auto LeftEdge = Edge(Left, LeftCenter), RightEdge = Edge(Right, RightCenter);
                TestTrue(TEXT("River crossing supplies nonempty clipped water edges on both patches"), LeftEdge.Num() >= 2 && RightEdge.Num() >= 2);
                TestEqual(TEXT("Shared river edge vertex counts match"), LeftEdge.Num(), RightEdge.Num());
                for (const FVector& V : LeftEdge)
                {
                    TestEqual(TEXT("Clipped river patch edge stays at sea level"), V.Z, 0.0);
                    TestTrue(TEXT("Adjacent river water edges have identical world intersections"), RightEdge.ContainsByPredicate(
                        [&V](const FVector& R) { return V.Equals(R, 0.001); }));
                }
                Found[Axis] = true;
                AddInfo(FString::Printf(TEXT("River patch seam seed=%llu axis=%d position=(%.3f,%.3f) edgeVertices=%d"),
                    Seed, Axis, Center.X, Center.Y, LeftEdge.Num()));
            }
        }
        TestTrue(TEXT("Bounded search finds an X-patch river crossing"), Found[0]);
        TestTrue(TEXT("Bounded search finds a Y-patch river crossing"), Found[1]);
    }
    G::ClearHydrologyCache();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRegionalGenerationTest, "Kalmala.World.Regional.Integrated",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaRegionalGenerationTest::RunTest(const FString& Parameters)
{
    using G = FKalmalaRegionalGeneration;
    constexpr int32 Side = 161;
    constexpr double Step = 10000;
    TArray<uint8> Previous;
    for (uint64 Seed : {418ull, 419ull})
    {
        FKalmalaWorldGenerationConfig C{Seed};
        TArray<uint8> Biomes;
        int32 Counts[7] = {}, RiverCount = 0, StreamCount = 0, Edges = 0, Different = 0;
        double MaxJump = 0, MaxWeightJump = 0;
        FVector2D WaterFixture = FVector2D::ZeroVector;
        float BestBasinDepth = 0;
        bool bWaterFixture = false;
        FVector2D RiverFixture = FVector2D::ZeroVector;
        for (int32 Y = 0; Y < Side; ++Y) for (int32 X = 0; X < Side; ++X)
        {
            const FVector2D P((X - Side / 2) * Step, (Y - Side / 2) * Step);
            auto Fields = FKalmalaWorldFieldSampler::Sample(C, P);
            const auto R = G::Sample(Fields);
            Biomes.Add(R.Biome);
            ++Counts[R.Biome];
            RiverCount += R.RiverWeight > 0;
            if (R.RiverWeight > 0) RiverFixture = P;
            StreamCount += R.StreamWeight > 0;
            if (R.BasinWeight > 0.99f && R.WaterLevel - R.Height > BestBasinDepth)
            { WaterFixture = P; bWaterFixture = true; BestBasinDepth = R.WaterLevel - R.Height; }
            if (!Previous.IsEmpty()) Different += Previous[Biomes.Num() - 1] != R.Biome;
            if (X > 0) Edges += R.Biome != Biomes[Biomes.Num() - 2];
            if (Y > 0) Edges += R.Biome != Biomes[Biomes.Num() - 1 - Side];
            float Total = 0;
            for (float W : R.Weights) { TestTrue(TEXT("Finite normalized biome weight"), FMath::IsFinite(W) && W >= 0 && W <= 1); Total += W; }
            TestTrue(TEXT("Weights sum to one"), FMath::IsNearlyEqual(Total, 1.0f, 0.0001f));
            if (Fields.Elevation < 0.22f) TestEqual(TEXT("Source sea floor stays ocean"), R.Biome, uint8(6));
            if (X % 8 == 0 && Y % 8 == 0)
            {
                Fields.Flora = 0;
                const auto Clearing = G::Sample(Fields);
                Fields.Flora = 1;
                const auto Dense = G::Sample(Fields);
                TestEqual(TEXT("Local Flora cannot fragment any regional biome"), Clearing.Biome, Dense.Biome);
                TestEqual(TEXT("Local Flora does not reshape terrain"), Clearing.Height, Dense.Height);
                const auto East = G::Sample(C, P + FVector2D(0.1, 0));
                MaxJump = FMath::Max(MaxJump, double(FMath::Abs(East.Height - R.Height)));
                for (int32 I = 0; I < 7; ++I) MaxWeightJump = FMath::Max(MaxWeightJump, double(FMath::Abs(East.Weights[I] - R.Weights[I])));
                TestEqual(TEXT("Same identity reproduces shaped height"), G::Sample(C, P).Height, R.Height);
            }
        }
        for (int32 I = 0; I < 7; ++I) TestTrue(FString::Printf(TEXT("Seed %llu contains biome %d"), Seed, I), Counts[I] > 5);
        TestTrue(TEXT("Large area contains rivers and no small streams"), RiverCount > 10 && StreamCount == 0);
        TestTrue(TEXT("Sub-centimetre terrain samples are continuous"), MaxJump < 2);
        TestTrue(TEXT("Biome blends are continuous"), MaxWeightJump < 0.005);
        const double BoundaryDensity = double(Edges) / (2 * Side * (Side - 1));
        TestTrue(TEXT("Biome boundaries do not return to confetti"), BoundaryDensity < 0.18);
        TArray<bool> Visited; Visited.Init(false, Biomes.Num());
        TArray<int32> Areas;
        int32 Small = 0;
        for (int32 Start = 0; Start < Biomes.Num(); ++Start)
        {
            if (Visited[Start]) continue;
            TArray<int32> Queue{Start}; Visited[Start] = true;
            for (int32 Q = 0; Q < Queue.Num(); ++Q)
            {
                const int32 Index = Queue[Q], X = Index % Side, Y = Index / Side;
                const int32 LocalNeighbours[] = { X > 0 ? Index - 1 : -1, X + 1 < Side ? Index + 1 : -1, Y > 0 ? Index - Side : -1, Y + 1 < Side ? Index + Side : -1 };
                for (int32 N : LocalNeighbours) if (N >= 0 && !Visited[N] && Biomes[N] == Biomes[Start]) { Visited[N] = true; Queue.Add(N); }
            }
            Areas.Add(Queue.Num());
            Small += Queue.Num() <= 2;
        }
        Areas.Sort();
        TestTrue(TEXT("At most two percent of broad-world samples form tiny components"), Small < Biomes.Num() / 50);
        AddInfo(FString::Printf(TEXT("Regional seed=%llu coverage=%d,%d,%d,%d,%d,%d,%d boundaries=%.4f components=%d medianCells=%d tiny=%d rivers=%d streams=%d maxHeightJump=%.5f"),
            Seed, Counts[0], Counts[1], Counts[2], Counts[3], Counts[4], Counts[5], Counts[6], BoundaryDensity, Areas.Num(), Areas[Areas.Num()/2], Small, RiverCount, StreamCount, MaxJump));
        if (!Previous.IsEmpty()) TestTrue(TEXT("Different seeds meaningfully move regions"), Different > Biomes.Num() / 4);
        Previous = Biomes;

        // The spatial index must return the same curves regardless of cache/query order.
        const FIntPoint Index(int32(FMath::FloorToInt(RiverFixture.X / FKalmalaRegionalTuning::GridCell)), int32(FMath::FloorToInt(RiverFixture.Y / FKalmalaRegionalTuning::GridCell)));
        const auto A = G::GetHydrology(C, Index);
        TestTrue(TEXT("Spline index comparison is nonempty"), !A.IsEmpty());
        G::ClearHydrologyCache();
        G::GetHydrology(C, Index + FIntPoint(1, 0));
        const auto B = G::GetHydrology(C, Index);
        TestEqual(TEXT("Spline rebuild count"), A.Num(), B.Num());
        for (int32 I = 0; I < FMath::Min(A.Num(), B.Num()); ++I)
        {
            TestEqual(TEXT("Spline identity"), A[I].Id, B[I].Id);
            TestEqual(TEXT("Spline start"), A[I].A, B[I].A);
            TestEqual(TEXT("Spline end"), A[I].B, B[I].B);
            TestTrue(TEXT("No broken spline step"), FVector::Dist(A[I].A, A[I].B) < 500);
        }
        for (int32 I = -4; I < 4; ++I)
        {
            const FVector2D P(I * FKalmalaRegionalTuning::GridCell, 7000);
            const auto West = G::Sample(C, P - FVector2D(0.01, 0));
            const auto East = G::Sample(C, P + FVector2D(0.01, 0));
            TestTrue(TEXT("Hydrology GridCell seam is continuous"), FMath::Abs(West.Height - East.Height) < 0.5);
        }
        // Verify the depth query uses both shaped collision-triangle planes.
        for (const FVector2D Origin : { FVector2D(-3000, -6000), FVector2D(3000, 6000) })
        for (const bool Upper : { false, true })
        {
            const FVector2D P = Origin + FVector2D(Upper ? 250.0 / 3 : 125.0 / 3);
            const float H = (G::Sample(C, Origin + FVector2D(125, 0)).Height
                + G::Sample(C, Origin + FVector2D(0, 125)).Height
                + G::Sample(C, Origin + (Upper ? FVector2D(125) : FVector2D::ZeroVector)).Height) / 3;
            const auto Ocean = FKalmalaOceanSampler::Sample(C, P, Origin);
            TestTrue(TEXT("Depth query uses shaped collision triangles"), FMath::IsNearlyEqual(Ocean.TerrainHeight, H, 0.01f));
            TestEqual(TEXT("Equivalent patch origins preserve shaped depth"), Ocean.TerrainHeight, FKalmalaOceanSampler::Sample(C, P, Origin + FVector2D(3000, 0)).TerrainHeight);
        }
        TestTrue(TEXT("Water seam fixture exists"), bWaterFixture);
        if (bWaterFixture)
        {
            const auto Left = FKalmalaWaterSurfaceMesh::BuildPatch(C, WaterFixture, true);
            const auto Right = FKalmalaWaterSurfaceMesh::BuildPatch(C, WaterFixture + FVector2D(3000, 0), true);
            int32 Matches = 0;
            for (const FVector& V : Left.Vertices) if (FMath::IsNearlyEqual(V.X, 1500.0, 0.01))
            {
                TestTrue(TEXT("Adjacent water patches share exact physical edge"), Right.Vertices.ContainsByPredicate([&](const FVector& W) { return (V - FVector(3000, 0, 0)).Equals(W, 0.01); }));
                ++Matches;
            }
            AddInfo(FString::Printf(TEXT("Water patch seam vertices=%d"), Matches));
            TestTrue(TEXT("Water seam comparison is nonempty"), Matches > 0);
        }
    }
    return true;
}
#endif
