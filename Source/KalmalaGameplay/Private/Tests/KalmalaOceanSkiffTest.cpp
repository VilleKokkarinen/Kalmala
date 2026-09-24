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
#endif
