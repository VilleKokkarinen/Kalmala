#include "KalmalaPlayerModelComponent.h"
#include "Misc/AutomationTest.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaToolSharpeningPresentationTest,
    "Kalmala.Gameplay.Tools.SharpeningPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaToolSharpeningPresentationTest::RunTest(const FString& Parameters)
{
    const UKalmalaPlayerModelComponent* DefaultModel = GetDefault<UKalmalaPlayerModelComponent>();
    const UFunction* CosmeticMulticast = UKalmalaPlayerModelComponent::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKalmalaPlayerModelComponent, MulticastPlayGrindingStoneSharpening));
    TestTrue(TEXT("Player model component replicates its cosmetic multicast"), DefaultModel && DefaultModel->GetIsReplicated());
    TestTrue(TEXT("Sharpening multicast carries no payload"), CosmeticMulticast
        && (CosmeticMulticast->FunctionFlags & FUNC_NetMulticast) != 0 && CosmeticMulticast->NumParms == 0);

    float LeftPitch = 0.0f;
    float RightPitch = 0.0f;
    constexpr float RestLeft = 8.0f;
    constexpr float RestRight = -6.0f;

    TestTrue(TEXT("Sharpening pose evaluates at animation start"),
        UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(0.0f, RestLeft, RestRight, LeftPitch, RightPitch));
    TestTrue(TEXT("Pose starts at the normal gait pose"),
        FMath::IsNearlyEqual(LeftPitch, RestLeft) && FMath::IsNearlyEqual(RightPitch, RestRight));

    TestTrue(TEXT("Sharpening pose evaluates during its stroke"),
        UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(0.3f, RestLeft, RestRight, LeftPitch, RightPitch));
    const float DrawStrokePitch = RightPitch;
    TestTrue(TEXT("Left arm braces toward the stone"), FMath::IsNearlyEqual(LeftPitch, -38.0f));

    TestTrue(TEXT("Sharpening pose evaluates on the return stroke"),
        UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(0.5f, RestLeft, RestRight, LeftPitch, RightPitch));
    TestTrue(TEXT("Right arm alternates across three short strokes"), RightPitch > DrawStrokePitch + 25.0f);

    TestTrue(TEXT("Sharpening pose evaluates as it returns to gait"),
        UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(
            UKalmalaPlayerModelComponent::ToolSharpeningDurationSeconds - 0.001f,
            RestLeft, RestRight, LeftPitch, RightPitch));
    TestTrue(TEXT("Pose blends back to its current gait at the end"),
        FMath::IsNearlyEqual(LeftPitch, RestLeft, 0.3f) && FMath::IsNearlyEqual(RightPitch, RestRight, 0.4f));

    TestFalse(TEXT("Negative animation time is rejected"),
        UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(-0.01f, RestLeft, RestRight, LeftPitch, RightPitch));
    TestFalse(TEXT("Non-finite animation time is rejected"),
        UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(std::numeric_limits<float>::quiet_NaN(),
            RestLeft, RestRight, LeftPitch, RightPitch));
    TestFalse(TEXT("Pose stops at the bounded animation duration"),
        UKalmalaPlayerModelComponent::EvaluateToolSharpeningArmPose(
            UKalmalaPlayerModelComponent::ToolSharpeningDurationSeconds, RestLeft, RestRight, LeftPitch, RightPitch));
    return true;
}

#endif
