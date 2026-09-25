#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaOceanSkiff.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanSkiffAuthorityContractTest,
    "Kalmala.Gameplay.OceanTravel.SkiffAuthorityContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanSkiffAuthorityContractTest::RunTest(const FString& Parameters)
{
    using Seat = EKalmalaOceanSkiffSeat;

    TestTrue(TEXT("Server launches only from a generated, nearby, bounded deep-ocean surface when session capacity is free"),
        AKalmalaOceanSkiff::IsLaunchAllowed(true, true, true, true, true, true));
    TestFalse(TEXT("Client launch requests are rejected"), AKalmalaOceanSkiff::IsLaunchAllowed(false, true, true, true, true, true));
    TestFalse(TEXT("Non-generated launch targets are rejected"), AKalmalaOceanSkiff::IsLaunchAllowed(true, false, true, true, true, true));
    TestFalse(TEXT("Distant launch targets are rejected"), AKalmalaOceanSkiff::IsLaunchAllowed(true, true, false, true, true, true));
    TestFalse(TEXT("Shallow-water launch targets are rejected"), AKalmalaOceanSkiff::IsLaunchAllowed(true, true, true, false, true, true));
    TestFalse(TEXT("Out-of-world launch targets are rejected"), AKalmalaOceanSkiff::IsLaunchAllowed(true, true, true, true, false, true));
    TestFalse(TEXT("A second session skiff is rejected"), AKalmalaOceanSkiff::IsLaunchAllowed(true, true, true, true, true, false));

    TestEqual(TEXT("The first accepted player receives the helm"), AKalmalaOceanSkiff::ChooseSeat(false, false), Seat::Helm);
    TestEqual(TEXT("The next accepted player receives the passenger seat"), AKalmalaOceanSkiff::ChooseSeat(true, false), Seat::Passenger);
    TestEqual(TEXT("Occupied seats reject another player"), AKalmalaOceanSkiff::ChooseSeat(true, true), Seat::None);

    TestTrue(TEXT("An occupant may leave at a safe stopped position"), AKalmalaOceanSkiff::IsDisembarkAllowed(true, true, 50.0f, true));
    TestFalse(TEXT("Clients cannot choose a disembark outcome"), AKalmalaOceanSkiff::IsDisembarkAllowed(false, true, 0.0f, true));
    TestFalse(TEXT("Non-occupants cannot disembark"), AKalmalaOceanSkiff::IsDisembarkAllowed(true, false, 0.0f, true));
    TestFalse(TEXT("Disembark is rejected above the stopped-speed limit"), AKalmalaOceanSkiff::IsDisembarkAllowed(true, true, 50.01f, true));
    TestFalse(TEXT("Malformed speed is rejected"), AKalmalaOceanSkiff::IsDisembarkAllowed(true, true, NAN, true));
    TestFalse(TEXT("Disembark is rejected without a safe capsule placement"), AKalmalaOceanSkiff::IsDisembarkAllowed(true, true, 0.0f, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanSkiffSteeringContractTest,
    "Kalmala.Gameplay.OceanTravel.SkiffSteeringContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanSkiffSteeringContractTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("The current helm may submit finite bounded sequenced input"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(true, true, 1.0f, -1.0f, 1, 0, 10.0, 0.0, false));
    TestFalse(TEXT("Clients cannot decide accepted vessel steering"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(false, true, 0.5f, 0.0f, 1, 0, 10.0, 0.0, false));
    TestFalse(TEXT("Passengers cannot steer"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(true, false, 0.5f, 0.0f, 1, 0, 10.0, 0.0, false));
    TestFalse(TEXT("Throttle outside the intent bounds is rejected"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(true, true, 1.01f, 0.0f, 1, 0, 10.0, 0.0, false));
    TestFalse(TEXT("Non-finite rudder input is rejected"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(true, true, 0.0f, NAN, 1, 0, 10.0, 0.0, false));
    TestFalse(TEXT("Replayed sequences are rejected"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(true, true, 0.5f, 0.0f, 4, 4, 10.2, 10.0, true));
    TestFalse(TEXT("Inputs faster than ten updates per second are rejected"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(true, true, 0.5f, 0.0f, 5, 4, 10.05, 10.0, true));
    TestTrue(TEXT("A new sequence is accepted at the ten-hertz interval"),
        AKalmalaOceanSkiff::IsSteeringIntentAllowed(true, true, 0.5f, 0.0f, 5, 4, 10.1, 10.0, true));

    TestTrue(TEXT("The last input remains fresh through the half-second boundary"),
        AKalmalaOceanSkiff::IsInputFresh(10.5, 10.0, true));
    TestFalse(TEXT("Steering expires after half a second without a fresh sequence"),
        AKalmalaOceanSkiff::IsInputFresh(10.5001, 10.0, true));
    TestFalse(TEXT("A disconnected or never-accepted helm has no fresh input"),
        AKalmalaOceanSkiff::IsInputFresh(10.0, 10.0, false));

    TestTrue(TEXT("Forward acceleration is capped at 100 cm/s^2"),
        FMath::IsNearlyEqual(AKalmalaOceanSkiff::AdvanceSpeed(0.0f, 1.0f, 0.25f), 25.0f));
    TestTrue(TEXT("Reverse acceleration is capped at 100 cm/s^2"),
        FMath::IsNearlyEqual(AKalmalaOceanSkiff::AdvanceSpeed(0.0f, -1.0f, 0.25f), -25.0f));

    float Speed = 0.0f;
    for (int32 Step = 0; Step < 40; ++Step)
    {
        Speed = AKalmalaOceanSkiff::AdvanceSpeed(Speed, 1.0f, 0.25f);
    }
    TestTrue(TEXT("Forward speed clamps to 700 cm/s"), FMath::IsNearlyEqual(Speed, 700.0f));
    Speed = AKalmalaOceanSkiff::AdvanceSpeed(Speed, -1.0f, 0.25f);
    TestTrue(TEXT("Reverse intent changes speed only within the acceleration cap"), FMath::IsNearlyEqual(Speed, 675.0f));
    for (int32 Step = 0; Step < 80; ++Step)
    {
        Speed = AKalmalaOceanSkiff::AdvanceSpeed(Speed, -1.0f, 0.25f);
    }
    TestTrue(TEXT("Reverse speed clamps to -200 cm/s"), FMath::IsNearlyEqual(Speed, -200.0f));
    TestTrue(TEXT("Expired input decelerates toward zero"),
        FMath::IsNearlyEqual(AKalmalaOceanSkiff::AdvanceSpeed(100.0f, 0.0f, 0.25f), 75.0f));
    return true;
}
#endif
