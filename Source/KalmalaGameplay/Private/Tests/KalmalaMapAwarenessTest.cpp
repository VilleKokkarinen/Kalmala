#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMapAwarenessComponent.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMapAwarenessTest, "Kalmala.Gameplay.MapAwareness.Authority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMapAwarenessTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("NaN coordinate is not in range"),
        UKalmalaMapAwarenessComponent::IsLocationInRange(FVector2D(std::numeric_limits<double>::quiet_NaN(), 0), FVector2D::ZeroVector));
    TestFalse(TEXT("Infinity coordinate is not in range"),
        UKalmalaMapAwarenessComponent::IsLocationInRange(FVector2D(0, std::numeric_limits<double>::infinity()), FVector2D::ZeroVector));
    TestFalse(TEXT("Out-of-bound coordinate is not in range"),
        UKalmalaMapAwarenessComponent::IsLocationInRange(FVector2D(1.0e9, 0), FVector2D::ZeroVector));
    TestFalse(TEXT("Distant coordinate is not in range"),
        UKalmalaMapAwarenessComponent::IsLocationInRange(FVector2D(6501, 0), FVector2D::ZeroVector));
    TestTrue(TEXT("Exact range boundary allowed"), UKalmalaMapAwarenessComponent::IsLocationInRange(FVector2D(6500, 0), FVector2D::ZeroVector));
    FKalmalaMapPing A, B, C;
    A.IssuedAt = B.IssuedAt = C.IssuedAt = 10;
    A.SenderId = B.SenderId = 1; C.SenderId = 2;
    A.Sequence = 1; B.Sequence = 2; C.Sequence = 1;
    TArray<FKalmalaMapPing> Ordered{C, B, A};
    Ordered.Sort(UKalmalaMapAwarenessComponent::PingLess);
    TestTrue(TEXT("Equal timestamps use sender then sequence"), Ordered[0].Sequence == 1 && Ordered[1].Sequence == 2 && Ordered[2].SenderId == 2);
    return true;
}
#endif
