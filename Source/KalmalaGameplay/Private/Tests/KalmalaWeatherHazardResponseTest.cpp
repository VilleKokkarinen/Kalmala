#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCampfireWeatherResponse.h"
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

    TestTrue(TEXT("Storm fuel wetting stays at baseline through the highly active threshold"),
        FMath::IsNearlyEqual(FKalmalaCampfireWeatherResponse::GetStormFuelWetnessMultiplier(FKalmalaWeatherState::HighlyActiveStormThreshold), 1.0f));
    TestTrue(TEXT("Maximum storm adds at most 25 percent fuel wetting"),
        FMath::IsNearlyEqual(FKalmalaCampfireWeatherResponse::GetStormFuelWetnessMultiplier(1.0f), 1.25f));
    TestTrue(TEXT("Non-finite storm intensity fails open to baseline fuel wetting"),
        FMath::IsNearlyEqual(FKalmalaCampfireWeatherResponse::GetStormFuelWetnessMultiplier(NAN), 1.0f));

    const float FullStormFuelWetness = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(0.0f, 1.0f, 1.0f, 1.0f, false);
    const float WindbreakFuelWetness = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(0.0f, 1.0f, 0.0f, 1.0f, false);
    const float RoofedFuelWetness = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(0.0f, 0.0f, 1.0f, 1.0f, false);
    TestTrue(TEXT("Full rain and wind storm accelerates exposed fuel wetting"), FMath::IsNearlyEqual(FullStormFuelWetness, 0.10f));
    TestTrue(TEXT("A windbreak removes the wind-derived storm surge"), FMath::IsNearlyEqual(WindbreakFuelWetness, 0.035f));
    TestEqual(TEXT("A roof removes direct-rain and storm wetting input"), RoofedFuelWetness, 0.0f);
    return true;
}
#endif
