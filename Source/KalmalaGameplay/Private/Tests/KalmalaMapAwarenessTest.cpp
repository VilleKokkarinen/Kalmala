#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaMapAwarenessComponent.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMapAwarenessTest, "Kalmala.Gameplay.MapAwareness.Authority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMapAwarenessTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("NaN world location is rejected"),
        UKalmalaMapAwarenessComponent::IsValidPingLocation(FVector2D(std::numeric_limits<double>::quiet_NaN(), 0)));
    TestFalse(TEXT("Infinity world location is rejected"),
        UKalmalaMapAwarenessComponent::IsValidPingLocation(FVector2D(0, std::numeric_limits<double>::infinity())));
    TestFalse(TEXT("Location beyond the world boundary is rejected"),
        UKalmalaMapAwarenessComponent::IsValidPingLocation(FVector2D(1600001, 0)));
    TestTrue(TEXT("Location beyond the former 65m range is allowed"),
        UKalmalaMapAwarenessComponent::IsValidPingLocation(FVector2D(6501, 0)));
    TestTrue(TEXT("Remote world location inside the boundary is allowed"),
        UKalmalaMapAwarenessComponent::IsValidPingLocation(FVector2D(1000000, 0)));
    TestTrue(TEXT("World boundary location is allowed"),
        UKalmalaMapAwarenessComponent::IsValidPingLocation(FVector2D(1600000, 0)));
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
