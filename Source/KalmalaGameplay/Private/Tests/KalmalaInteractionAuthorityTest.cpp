#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInteractionTestActor.h"
#include "KalmalaHarvestNode.h"
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

#endif
