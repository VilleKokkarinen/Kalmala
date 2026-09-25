#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanTravelFeedbackComponent.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldPlayerStartResolver.h"
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

    TestEqual(TEXT("Non-generated surface launch gives coast guidance"),
        AKalmalaOceanSkiff::GetLaunchDenialFeedback(false, true, true, true, true, true),
        EKalmalaOceanTravelFeedback::LaunchObstructed);
    TestEqual(TEXT("Distant launch gives reach guidance"),
        AKalmalaOceanSkiff::GetLaunchDenialFeedback(true, false, true, true, true, true),
        EKalmalaOceanTravelFeedback::OutOfReach);
    TestEqual(TEXT("An occupied session slot is explained"),
        AKalmalaOceanSkiff::GetLaunchDenialFeedback(true, true, true, true, false, true),
        EKalmalaOceanTravelFeedback::SessionSkiffExists);
    TestEqual(TEXT("The finite-world edge has its own launch reason"),
        AKalmalaOceanSkiff::GetLaunchDenialFeedback(true, true, true, false, true, true),
        EKalmalaOceanTravelFeedback::WorldEdge);
    TestEqual(TEXT("Shallow launch position reports access guidance"),
        AKalmalaOceanSkiff::GetLaunchDenialFeedback(true, true, false, true, true, true),
        EKalmalaOceanTravelFeedback::ShallowLaunch);
    TestEqual(TEXT("A shallow hull corner reports its own launch reason"),
        AKalmalaOceanSkiff::GetLaunchDenialFeedback(true, true, true, true, true, false),
        EKalmalaOceanTravelFeedback::ShallowHull);
    TestEqual(TEXT("A valid launch has no denial reason"),
        AKalmalaOceanSkiff::GetLaunchDenialFeedback(true, true, true, true, true, true),
        EKalmalaOceanTravelFeedback::None);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanSkiffRestoreContractTest,
    "Kalmala.Gameplay.OceanTravel.SkiffRestoreContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanSkiffRestoreContractTest::RunTest(const FString& Parameters)
{
    using Feedback = EKalmalaOceanTravelFeedback;
    TestTrue(TEXT("A server restores an authenticated player into their matching free saved seat"),
        AKalmalaOceanSkiff::IsSeatRestoreAllowed(true, true, true, true, true, false, false));
    TestFalse(TEXT("A client cannot restore an ocean-travel seat"),
        AKalmalaOceanSkiff::IsSeatRestoreAllowed(false, true, true, true, true, false, false));
    TestFalse(TEXT("Unauthenticated players cannot restore saved seats"),
        AKalmalaOceanSkiff::IsSeatRestoreAllowed(true, false, true, true, true, false, false));
    TestFalse(TEXT("A seat from another vessel cannot be restored"),
        AKalmalaOceanSkiff::IsSeatRestoreAllowed(true, true, false, true, true, false, false));
    TestFalse(TEXT("An occupied saved seat is not assigned twice"),
        AKalmalaOceanSkiff::IsSeatRestoreAllowed(true, true, true, true, false, false, false));
    TestFalse(TEXT("A duplicate player cannot occupy two seats"),
        AKalmalaOceanSkiff::IsSeatRestoreAllowed(true, true, true, true, true, true, false));
    TestFalse(TEXT("An already attached player cannot be duplicated on a saved skiff"),
        AKalmalaOceanSkiff::IsSeatRestoreAllowed(true, true, true, true, true, false, true));
    TestTrue(TEXT("Failed saved-seat cleanup explains why disembarking was cancelled"),
        UKalmalaOceanTravelFeedbackComponent::GetFeedbackText(Feedback::TravelSaveUnavailable)
            .Contains(TEXT("disembarking was cancelled safely")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanTravelFeedbackContractTest,
    "Kalmala.Gameplay.OceanTravel.FeedbackAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanTravelFeedbackContractTest::RunTest(const FString& Parameters)
{
    using Feedback = EKalmalaOceanTravelFeedback;
    TestTrue(TEXT("A server may publish a known requester feedback reason"),
        UKalmalaOceanTravelFeedbackComponent::IsFeedbackAllowed(true, Feedback::ShallowLaunch));
    TestFalse(TEXT("A client cannot choose an ocean travel result"),
        UKalmalaOceanTravelFeedbackComponent::IsFeedbackAllowed(false, Feedback::HelmAssigned));
    TestFalse(TEXT("No-result is not a published feedback outcome"),
        UKalmalaOceanTravelFeedbackComponent::IsFeedbackAllowed(true, Feedback::None));
    TestTrue(TEXT("Shallow launch text explains depth and how to reach launch water"),
        UKalmalaOceanTravelFeedbackComponent::GetFeedbackText(Feedback::ShallowLaunch)
            .Contains(TEXT("Wade or swim farther out")));
    TestTrue(TEXT("Stop-before-exit text states the released-control speed limit"),
        UKalmalaOceanTravelFeedbackComponent::GetFeedbackText(Feedback::StopBeforeDisembarking)
            .Contains(TEXT("0.5 m/s or slower")));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanSkiffWeatherPressureTest,
    "Kalmala.Gameplay.OceanTravel.SkiffWeatherPressure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanSkiffWeatherPressureTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Calm weather and head or following wind do not yaw the hull"),
        FMath::IsNearlyZero(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, 0.0f, 1.0f, 700.0f))
        && FMath::IsNearlyZero(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, 180.0f, 1.0f, 700.0f))
        && FMath::IsNearlyZero(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, 90.0f, 0.0f, 700.0f)));
    TestTrue(TEXT("Full beam wind adds at most eight degrees per second at full speed"),
        FMath::IsNearlyEqual(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, 90.0f, 1.0f, 700.0f), 8.0f));
    TestTrue(TEXT("Opposite crosswinds turn in opposite directions"),
        FMath::IsNearlyEqual(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, 270.0f, 1.0f, 700.0f), -8.0f));
    TestTrue(TEXT("Crosswind pressure scales with replicated wind strength and vessel speed"),
        FMath::IsNearlyEqual(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, 90.0f, 0.5f, 350.0f), 2.0f));
    TestTrue(TEXT("Helm counter-steering can overcome the bounded crosswind turn"),
        AKalmalaOceanSkiff::CalculateWeatherYawRate(-1.0f, 0.0f, 90.0f, 1.0f, 700.0f) < 0.0f);
    TestTrue(TEXT("Weather cannot exceed the existing yaw-rate cap"),
        FMath::IsNearlyEqual(AKalmalaOceanSkiff::CalculateWeatherYawRate(1.0f, 0.0f, 90.0f, 1.0f, 700.0f), 35.0f));
    TestTrue(TEXT("Malformed weather or movement values create no yaw pressure"),
        FMath::IsNearlyZero(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, NAN, 1.0f, 700.0f))
        && FMath::IsNearlyZero(AKalmalaOceanSkiff::CalculateWeatherYawRate(0.0f, 0.0f, 90.0f, NAN, 700.0f)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanSkiffCoastlineAccessTest,
    "Kalmala.Gameplay.OceanTravel.SkiffCoastlineAccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanSkiffCoastlineAccessTest::RunTest(const FString& Parameters)
{
    constexpr uint64 RepresentativeSeeds[] = {418, 999, 1337};
    constexpr float CoarseCoastStep = 10000.0f;
    constexpr float MaximumScanRadius = 1400000.0f;
    constexpr float CoastCandidateStep = 100.0f;
    constexpr float InteractionRange = 250.0f;
    constexpr float MaximumSwimAccessDistance = 30000.0f;
    constexpr float SafeExitOffset = 190.0f;
    constexpr double CapsuleWorldMargin = 44.0;

    for (const uint64 Seed : RepresentativeSeeds)
    {
        const FKalmalaWorldGenerationConfig Config{Seed};
        const FVector2D Start(FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation());
        bool bFoundCoast = false;
        bool bFoundShallowRejection = false;
        bool bFoundAccessAndExit = false;
        int32 CoastSegments = 0;
        FVector2D AcceptedLaunch = FVector2D::ZeroVector;
        FVector2D AcceptedExit = FVector2D::ZeroVector;
        float AcceptedDepth = 0.0f;
        float AcceptedShoreDistance = 0.0f;
        float AcceptedExitDepth = 0.0f;

        for (int32 DirectionIndex = 0; DirectionIndex < 32; ++DirectionIndex)
        {
            const float Angle = DirectionIndex * (2.0f * PI / 32.0f);
            const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
            float PreviousRadius = 0.0f;
            FKalmalaOceanSample Previous = FKalmalaOceanSampler::Sample(Config, Start);
            if (!Previous.bIsValid || Previous.TerrainHeight < 0.0f) continue;

            for (float Radius = CoarseCoastStep; Radius <= MaximumScanRadius; Radius += CoarseCoastStep)
            {
                const FVector2D Probe = Start + Direction * Radius;
                if (!FKalmalaWorldBounds::Contains(Config, Probe, 160.0)) break;

                const FKalmalaOceanSample Current = FKalmalaOceanSampler::Sample(Config, Probe);
                if (!Current.bIsValid) break;
                const FVector2D PreviousPosition = Start + Direction * PreviousRadius;
                if (Previous.TerrainHeight >= 0.0f && Current.WaterDepth > 0.0f
                    && !FKalmalaShimmeringLakeSampler::IsWater(Config, PreviousPosition)
                    && !FKalmalaShimmeringLakeSampler::IsWater(Config, Probe))
                {
                    ++CoastSegments;
                    bFoundCoast = true;
                    float LandRadius = PreviousRadius;
                    float SeaRadius = Radius;
                    for (int32 Refine = 0; Refine < 12 && SeaRadius - LandRadius > 1.0f; ++Refine)
                    {
                        const float MidRadius = (LandRadius + SeaRadius) * 0.5f;
                        const FKalmalaOceanSample Mid = FKalmalaOceanSampler::Sample(
                            Config, Start + Direction * MidRadius);
                        if (Mid.bIsValid && Mid.WaterDepth > 0.0f) SeaRadius = MidRadius;
                        else LandRadius = MidRadius;
                    }

                    const FVector2D Shore = Start + Direction * ((LandRadius + SeaRadius) * 0.5f);
                    for (float Offset = 0.0f; Offset <= MaximumSwimAccessDistance; Offset += CoastCandidateStep)
                    {
                        const FVector2D Candidate = Start + Direction * (SeaRadius + Offset);
                        const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, Candidate);
                        if (!Ocean.bIsValid || FKalmalaShimmeringLakeSampler::IsWater(Config, Candidate)) continue;

                        if (Offset <= InteractionRange && Ocean.WaterDepth > 0.0f && Ocean.WaterDepth < 100.0f)
                        {
                            bool bAllHeadingsRejected = true;
                            for (int32 HeadingIndex = 0; HeadingIndex < 12; ++HeadingIndex)
                            {
                                bAllHeadingsRejected &= !AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(
                                    Config, Candidate, HeadingIndex * 15.0f);
                            }
                            bFoundShallowRejection |= bAllHeadingsRejected;
                        }

                        if (Ocean.WaterDepth < 100.0f || Ocean.WaterDepth > InteractionRange)
                        {
                            continue;
                        }
                        const bool bLaunchGatesPass = AKalmalaOceanSkiff::IsLaunchAllowed(true, true, true,
                            Ocean.WaterDepth >= 100.0f, FKalmalaWorldBounds::Contains(Config, Candidate, 160.0), true);
                        if (!bLaunchGatesPass) continue;

                        for (int32 HeadingIndex = 0; HeadingIndex < 12 && !bFoundAccessAndExit; ++HeadingIndex)
                        {
                            if (!AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(
                                Config, Candidate, HeadingIndex * 15.0f)) continue;

                            for (int32 ExitDirection = 0; ExitDirection < 8; ++ExitDirection)
                            {
                                const float ExitAngle = ExitDirection * (2.0f * PI / 8.0f);
                                const FVector2D Exit = Candidate
                                    + FVector2D(FMath::Cos(ExitAngle), FMath::Sin(ExitAngle)) * SafeExitOffset;
                                if (AKalmalaOceanSkiff::IsSafeExitSurfaceForConfig(Config, Exit, CapsuleWorldMargin))
                                {
                                    AcceptedLaunch = Candidate;
                                    AcceptedExit = Exit;
                                    AcceptedDepth = Ocean.WaterDepth;
                                    AcceptedShoreDistance = (Candidate - Shore).Size();
                                    AcceptedExitDepth = FKalmalaOceanSampler::Sample(Config, Exit).WaterDepth;
                                    bFoundAccessAndExit = true;
                                    break;
                                }
                            }
                        }
                        if (bFoundAccessAndExit) break;
                    }
                    break;
                }

                Previous = Current;
                PreviousRadius = Radius;
            }
        }

        TestTrue(FString::Printf(TEXT("Seed %llu has a generated mainland-to-ocean coastline"), Seed), bFoundCoast);
        TestTrue(FString::Printf(TEXT("Seed %llu rejects shallow hull footprints"), Seed), bFoundShallowRejection);
        TestTrue(FString::Printf(TEXT("Seed %llu has a launch candidate within 300 m of the coast and a safe exit"), Seed),
            bFoundAccessAndExit);
        AddInfo(FString::Printf(TEXT("Coast seed %llu: coast rays=%d shallow-rejected=%s access=%s launch-depth=%.1f cm shore-distance=%.1f m exit-water-depth=%.1f cm launch=(%.0f, %.0f) exit=(%.0f, %.0f)"),
            Seed, CoastSegments, bFoundShallowRejection ? TEXT("yes") : TEXT("no"),
            bFoundAccessAndExit ? TEXT("yes") : TEXT("no"), AcceptedDepth, AcceptedShoreDistance / 100.0f, AcceptedExitDepth,
            AcceptedLaunch.X, AcceptedLaunch.Y, AcceptedExit.X, AcceptedExit.Y));
    }

    const FKalmalaWorldGenerationConfig Config{418};
    TestFalse(TEXT("A hull footprint outside the finite playable world is rejected"),
        AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(Config, FVector2D(FKalmalaWorldBounds::Radius + 500.0, 0.0), 0.0f));
    TestFalse(TEXT("A safe-exit candidate outside the finite playable world is rejected"),
        AKalmalaOceanSkiff::IsSafeExitSurfaceForConfig(Config, FVector2D(FKalmalaWorldBounds::Radius + 500.0, 0.0)));
    TestFalse(TEXT("Negative exit clearance is malformed"),
        AKalmalaOceanSkiff::IsSafeExitSurfaceForConfig(Config, FVector2D::ZeroVector, -1.0));
    return true;
}
#endif
