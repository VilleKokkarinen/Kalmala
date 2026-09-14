#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInteractionGrid.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInteractionGridTest, "Kalmala.Gameplay.InteractionGrid.SurfaceMoisture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaInteractionGridTest::RunTest(const FString& Parameters)
{
    const FIntPoint Positive = FKalmalaInteractionGrid::ToCellKey(FVector(399.0f, 0.0f, 0.0f));
    const FIntPoint Negative = FKalmalaInteractionGrid::ToCellKey(FVector(-1.0f, -200.0f, 0.0f));
    TestEqual(TEXT("Grid keys floor positive world coordinates"), Positive, FIntPoint(1, 0));
    TestEqual(TEXT("Grid keys floor negative world coordinates"), Negative, FIntPoint(-1, -1));
    TestEqual(TEXT("Grid centre is deterministic"), FKalmalaInteractionGrid::ToCellCenter(FIntPoint(-1, 2)), FVector2D(-100.0f, 500.0f));

    FKalmalaInteractionCellState State;
    State.Material = EKalmalaInteractionMaterial::Ground;
    State.SurfaceWetness = 50.0f;
    FKalmalaInteractionGrid::AdvanceSurfaceMoisture(State, 1.0f, 2.0f);
    TestTrue(TEXT("Server rain increases transient surface moisture"), State.SurfaceWetness > 50.0f);
    FKalmalaInteractionGrid::AdvanceSurfaceMoisture(State, 0.0f, 1000.0f);
    TestEqual(TEXT("Drying clamps surface moisture"), State.SurfaceWetness, 0.0f);
    TestTrue(TEXT("Moisture state remains finite and bounded"), FKalmalaInteractionGrid::IsValid(State));

    State.SurfaceWetness = 101.0f;
    FKalmalaInteractionGrid::AdvanceSurfaceMoisture(State, 2.0f, -1.0f);
    TestTrue(TEXT("Malformed simulation input fails closed to bounded state"), FKalmalaInteractionGrid::IsValid(State));
    TestEqual(TEXT("The grid cap remains explicit"), FKalmalaInteractionGrid::MaxActiveCells, 1024);
    return true;
}

#endif
