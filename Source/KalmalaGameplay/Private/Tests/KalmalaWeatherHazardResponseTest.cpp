#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaExposureResponse.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWeatherHazardResponseTest,
    "Kalmala.Gameplay.Exposure.WeatherHazardResponse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWeatherHazardResponseTest::RunTest(const FString& Parameters)
{
    constexpr float NormalRainTriggerSeconds = 10.0f;
    const float CalmTrigger = FKalmalaExposureResponse::GetUnroofedRainTriggerSeconds(NormalRainTriggerSeconds, false);
    const float ActiveTrigger = FKalmalaExposureResponse::GetUnroofedRainTriggerSeconds(NormalRainTriggerSeconds, false);
    const float HighlyActiveTrigger = FKalmalaExposureResponse::GetUnroofedRainTriggerSeconds(NormalRainTriggerSeconds, true);

    TestTrue(TEXT("Calm rain retains its existing trigger"), FMath::IsNearlyEqual(CalmTrigger, 10.0f));
    TestTrue(TEXT("Active rain retains its existing trigger"), FMath::IsNearlyEqual(ActiveTrigger, 10.0f));
    TestTrue(TEXT("Highly Active rain applies Wet after 7.5 seconds"), FMath::IsNearlyEqual(HighlyActiveTrigger, 7.5f));
    TestTrue(TEXT("Storm pressure remains bounded to a 25 percent trigger reduction"),
        HighlyActiveTrigger >= NormalRainTriggerSeconds * 0.75f && HighlyActiveTrigger <= NormalRainTriggerSeconds);
    TestTrue(TEXT("Nonpositive configured trigger cannot become negative"),
        FMath::IsNearlyEqual(FKalmalaExposureResponse::GetUnroofedRainTriggerSeconds(-2.0f, true), 0.0f));
    return true;
}
#endif
