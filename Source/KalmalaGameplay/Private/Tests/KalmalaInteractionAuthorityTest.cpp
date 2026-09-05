#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInteractionTestActor.h"
#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaCampfireWeatherResponse.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaHazardSpawn.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaInteractionAuthorityTest,
    "Kalmala.Gameplay.Interaction.ServerOnlyRangeValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaInteractionAuthorityTest::RunTest(const FString& Parameters)
{
    const FVector TargetLocation = FVector::ZeroVector;
    int32 ReplicatedCounter = 0;

    const bool bInvalidRequest = AKalmalaInteractionTestActor::IsInteractionAllowed(true, FVector(1000.0f, 0.0f, 0.0f), TargetLocation, 250.0f);
    if (bInvalidRequest)
    {
        ++ReplicatedCounter;
    }
    TestFalse(TEXT("An out-of-range request is rejected"), bInvalidRequest);
    TestEqual(TEXT("A rejected request cannot change interaction state"), ReplicatedCounter, 0);

    const bool bClientRequest = AKalmalaInteractionTestActor::IsInteractionAllowed(false, FVector(100.0f, 0.0f, 0.0f), TargetLocation, 250.0f);
    TestFalse(TEXT("A non-authoritative request is rejected"), bClientRequest);

    const bool bValidRequest = AKalmalaInteractionTestActor::IsInteractionAllowed(true, FVector(100.0f, 0.0f, 0.0f), TargetLocation, 250.0f);
    if (bValidRequest)
    {
        ++ReplicatedCounter;
    }
    TestTrue(TEXT("An in-range server request is accepted"), bValidRequest);
    TestEqual(TEXT("Only the accepted server request changes interaction state"), ReplicatedCounter, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaHarvestNodeAuthorityTest,
    "Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaHarvestNodeAuthorityTest::RunTest(const FString& Parameters)
{
    const FVector NodeLocation = FVector::ZeroVector;
    TestFalse(TEXT("Clients cannot harvest a generated node"), AKalmalaHarvestNode::IsHarvestAllowed(false, false, FVector(100.0f, 0.0f, 0.0f), NodeLocation));
    TestFalse(TEXT("The server rejects a distant harvester"), AKalmalaHarvestNode::IsHarvestAllowed(true, false, FVector(1000.0f, 0.0f, 0.0f), NodeLocation));
    TestFalse(TEXT("The server rejects a depleted node"), AKalmalaHarvestNode::IsHarvestAllowed(true, true, FVector(100.0f, 0.0f, 0.0f), NodeLocation));
    TestTrue(TEXT("The server accepts an in-range harvester for an available node"), AKalmalaHarvestNode::IsHarvestAllowed(true, false, FVector(100.0f, 0.0f, 0.0f), NodeLocation));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaCampfireWeatherTest,
    "Kalmala.Gameplay.Campfire.ServerWeatherResponse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaCampfireWeatherTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("A client cannot light a campfire"), AKalmalaCampfire::IsLightingAllowed(false, 0.0f));
    TestFalse(TEXT("The server rejects soaked fuel"), AKalmalaCampfire::IsLightingAllowed(true, FKalmalaCampfireWeatherResponse::ExtinguishWetness));
    TestTrue(TEXT("The server can light dry fuel"), AKalmalaCampfire::IsLightingAllowed(true, 0.0f));

    const float DryWarmth = FKalmalaCampfireWeatherResponse::CalculateEffectiveWarmth(true, 0.0f, 0.0f, 0.0f);
    const float StormWarmth = FKalmalaCampfireWeatherResponse::CalculateEffectiveWarmth(true, 0.35f, 1.0f, 1.0f);
    const float WetFuel = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(0.0f, 1.0f, 1.0f, 10.0f, true);
    const float DryingFuel = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(0.5f, 0.0f, 0.0f, 10.0f, true);
    TestEqual(TEXT("A dry sheltered campfire produces its full warmth contribution"), DryWarmth, 1.0f);
    TestTrue(TEXT("Rain, wind, and damp fuel reduce effective campfire warmth"), StormWarmth < DryWarmth);
    TestTrue(TEXT("Rain and wind soak exposed campfire fuel"), WetFuel > 0.0f);
    TestTrue(TEXT("A lit campfire dries fuel when rain stops"), DryingFuel < 0.5f);
    TestFalse(TEXT("Soaked fuel extinguishes a lit campfire"), FKalmalaCampfireWeatherResponse::ShouldRemainLit(true, FKalmalaCampfireWeatherResponse::ExtinguishWetness));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaExposureConsequenceTest,
    "Kalmala.Gameplay.Exposure.RecoverableTravelPenalty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaExposureConsequenceTest::RunTest(const FString& Parameters)
{
    const float ExposedWetness = FKalmalaExposureResponse::AdvanceWetness(0.0f, 1.0f, 0.8f, 1.0f, 0.0f, 0.0f, 120.0f);
    const float ExposedWarmth = FKalmalaExposureResponse::AdvanceWarmth(100.0f, -12.0f, ExposedWetness, 1.0f, 0.0f, 0.0f, 120.0f);
    const float ExposedSpeed = FKalmalaExposureResponse::GetTravelSpeedMultiplier(ExposedWarmth);
    const float RecoveredWetness = FKalmalaExposureResponse::AdvanceWetness(ExposedWetness, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 60.0f);
    const float RecoveredWarmth = FKalmalaExposureResponse::AdvanceWarmth(ExposedWarmth, -12.0f, RecoveredWetness, 0.0f, 1.0f, 1.0f, 60.0f);

    TestTrue(TEXT("Prolonged exposed rain produces wetness"), ExposedWetness > 0.0f);
    TestTrue(TEXT("Prolonged cold exposure reduces warmth"), ExposedWarmth < 50.0f);
    TestTrue(TEXT("Low warmth applies a visible reversible travel penalty"), ExposedSpeed < 1.0f);
    TestTrue(TEXT("Shelter and a lit fire dry the player"), RecoveredWetness < ExposedWetness);
    TestTrue(TEXT("Shelter and a lit fire recover warmth"), RecoveredWarmth > ExposedWarmth);
    TestEqual(TEXT("Recovered warmth removes the travel penalty"), FKalmalaExposureResponse::GetTravelSpeedMultiplier(RecoveredWarmth), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaExposureAuthorityTest,
    "Kalmala.Gameplay.Exposure.ServerAuthoritativeReplicationContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaExposureAuthorityTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("A client cannot update replicated weather"), AKalmalaWorldGenerationGameState::IsWeatherUpdateAllowed(false));
    TestTrue(TEXT("Only the server can update replicated weather"), AKalmalaWorldGenerationGameState::IsWeatherUpdateAllowed(true));
    TestFalse(TEXT("A client cannot update replicated exposure"), AKalmalaCharacter::IsExposureUpdateAllowed(false));
    TestTrue(TEXT("Only the server can update replicated exposure"), AKalmalaCharacter::IsExposureUpdateAllowed(true));
    TestFalse(TEXT("A client cannot light a replicated campfire"), AKalmalaCampfire::IsLightingAllowed(false, 0.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaWildlifeSpawnAuthorityTest,
    "Kalmala.Gameplay.WildlifeSpawn.ServerOnlyDefeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWildlifeSpawnAuthorityTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Clients cannot defeat a generated wildlife spawn"), AKalmalaWildlifeSpawn::IsDefeatAllowed(false, false));
    TestFalse(TEXT("The server cannot defeat an already defeated wildlife spawn"), AKalmalaWildlifeSpawn::IsDefeatAllowed(true, true));
    TestTrue(TEXT("The server can record the first validated wildlife defeat"), AKalmalaWildlifeSpawn::IsDefeatAllowed(true, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaHazardSpawnAuthorityTest,
    "Kalmala.Gameplay.HazardSpawn.ServerOnlyDefeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaHazardSpawnAuthorityTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Clients cannot defeat a generated hazard spawn"), AKalmalaHazardSpawn::IsDefeatAllowed(false, false));
    TestFalse(TEXT("The server cannot defeat an already defeated hazard spawn"), AKalmalaHazardSpawn::IsDefeatAllowed(true, true));
    TestTrue(TEXT("The server can record the first validated hazard defeat"), AKalmalaHazardSpawn::IsDefeatAllowed(true, false));
    return true;
}

#endif
