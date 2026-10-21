#include "KalmalaPlayerModelComponent.h"
#include "Misc/AutomationTest.h"
#include <limits>
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaHeldToolModelTest, "Kalmala.Gameplay.Tools.HeldModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaHeldToolModelTest::RunTest(const FString& Parameters)
{
    const TArray<FName> Ids = {TEXT("ConstructionHammer"), TEXT("StoneAxe"), TEXT("BronzeAxe"), TEXT("IronAxe"),
        TEXT("StoneKnife"), TEXT("StonePick"), TEXT("StoneSpear"), TEXT("StoneMace"), TEXT("Bow"), TEXT("CultivationRake")};
    const TArray<EKalmalaHeldToolVisual> Expected = {EKalmalaHeldToolVisual::Hammer, EKalmalaHeldToolVisual::Axe,
        EKalmalaHeldToolVisual::Axe, EKalmalaHeldToolVisual::Axe, EKalmalaHeldToolVisual::Knife,
        EKalmalaHeldToolVisual::Pick, EKalmalaHeldToolVisual::Spear, EKalmalaHeldToolVisual::Mace,
        EKalmalaHeldToolVisual::Bow, EKalmalaHeldToolVisual::Rake};
    for (int32 Index = 0; Index < Ids.Num(); ++Index)
        TestEqual(TEXT("Known equipment maps to its actual family"),
            UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(Ids[Index]), Expected[Index]);
    TestEqual(TEXT("Empty hand has no tool"), UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(NAME_None), EKalmalaHeldToolVisual::None);
    TestEqual(TEXT("Unknown items never display a hammer"), UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(TEXT("Forged")), EKalmalaHeldToolVisual::None);
    const auto* VisualProperty = FindFProperty<FEnumProperty>(UKalmalaPlayerModelComponent::StaticClass(), TEXT("HeldToolVisual"));
    TestTrue(TEXT("Remote cosmetic state is a replicated enum, not an inventory payload"),
        VisualProperty && VisualProperty->HasAnyPropertyFlags(CPF_Net));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Held-model fixture world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Fixture character exists"), Pawn)) return false;
    Pawn->DispatchBeginPlay();
    auto* Model = Pawn->FindComponentByClass<UKalmalaPlayerModelComponent>();
    auto* Inventory = Pawn->GetInventoryComponent();
    if (!TestNotNull(TEXT("Player model exists"), Model)) return false;
    const auto Tools = Pawn->GetCarriedToolInventory();
    FString Reason;
    for (int32 Cycle = 0; Cycle < 2; ++Cycle)
    {
        for (const auto& Tool : Tools)
        {
            if (Inventory->GetActiveItem() != Tool.ItemId)
                TestTrue(TEXT("Normal authoritative equip succeeds"), Inventory->SetMainHandFromServer(Tool.InstanceGuid, Reason));
            Model->TickComponent(0.016f, LEVELTICK_All, nullptr);
            TestEqual(TEXT("Actual held model tracks equipped item"), Model->GetHeldToolVisual(),
                UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(Tool.ItemId));
            TArray<UProceduralMeshComponent*> Meshes;
            Pawn->GetComponents(Meshes);
            TestEqual(TEXT("Equipment swaps retain a fixed component count"), Meshes.Num(), 16);
            int32 Visible = 0;
            for (auto* Mesh : Meshes)
            {
                TestEqual(TEXT("Body and weapon visuals remain non-colliding"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
                if (Mesh->GetName().StartsWith(TEXT("HeldToolPart")) && Mesh->IsVisible()) ++Visible;
            }
            const int32 ExpectedVisible = Tool.ItemId == TEXT("StoneAxe") || Tool.ItemId == TEXT("StonePick") ? 3
                : Tool.ItemId == TEXT("CultivationRake") ? 7 : 2;
            TestEqual(TEXT("Only this weapon's parts are visible"), Visible, ExpectedVisible);
        }
        TestTrue(TEXT("Unequip succeeds"), Inventory->SetMainHandFromServer(FGuid(), Reason));
        Model->TickComponent(0.016f, LEVELTICK_All, nullptr);
        TestEqual(TEXT("Clearing Main Hand clears the visible weapon"), Model->GetHeldToolVisual(), EKalmalaHeldToolVisual::None);
        TArray<UProceduralMeshComponent*> Meshes;
        Pawn->GetComponents(Meshes);
        TestFalse(TEXT("No leftover hammer or weapon is visible"), Meshes.ContainsByPredicate([](const auto* Mesh)
            { return Mesh->GetName().StartsWith(TEXT("HeldToolPart")) && Mesh->IsVisible(); }));
    }
    // Exercise the same repnotify used by a simulated peer, with no inventory lookup.
    auto* MutableVisualProperty = FindFProperty<FEnumProperty>(UKalmalaPlayerModelComponent::StaticClass(), TEXT("HeldToolVisual"));
    if (MutableVisualProperty)
    {
        for (const auto Visual : Expected)
        {
            MutableVisualProperty->GetUnderlyingProperty()->SetIntPropertyValue(
                MutableVisualProperty->ContainerPtrToValuePtr<void>(Model), static_cast<uint64>(Visual));
            Model->ProcessEvent(Model->FindFunctionChecked(TEXT("OnRep_HeldToolVisual")), nullptr);
            TArray<UProceduralMeshComponent*> Meshes;
            Pawn->GetComponents(Meshes);
            const int32 Visible = Meshes.FilterByPredicate([](const auto* Mesh)
                { return Mesh->GetName().StartsWith(TEXT("HeldToolPart")) && Mesh->IsVisible(); }).Num();
            TestEqual(TEXT("Repnotify alone reconstructs remote tool geometry"), Visible,
                Visual == EKalmalaHeldToolVisual::Axe || Visual == EKalmalaHeldToolVisual::Pick
                    || Visual == EKalmalaHeldToolVisual::Bow ? 3
                    : Visual == EKalmalaHeldToolVisual::Rake ? 7 : 2);
            TestTrue(TEXT("Remote appearance does not populate private inventory selection"), Inventory->GetActiveItem().IsNone());
            auto* Handle = *Meshes.FindByPredicate([](const auto* Mesh)
                { return Mesh->GetName() == TEXT("HeldToolPart0"); });
            auto* Arm = Handle->GetAttachParent();
            const FVector HandGrip(0, 0, -52);
            const FVector GripInHandle = Handle->GetRelativeTransform().InverseTransformPosition(HandGrip);
            TestTrue(TEXT("The palm intersects the actual handle, not empty space beside it"),
                FMath::Abs(GripInHandle.X) < 0.15f && FMath::Abs(GripInHandle.Y) < 0.15f
                && FMath::Abs(GripInHandle.Z) <= 1.01f);
            const FVector ShaftAxis = Handle->GetRelativeRotation().RotateVector(FVector::UpVector);
            if (Visual == EKalmalaHeldToolVisual::Bow)
            {
                TestTrue(TEXT("Bow stays upright with the hand at its central limb join"),
                    ShaftAxis.Z > 0.9f && GripInHandle.Z < -0.9f);
                auto* String = *Meshes.FindByPredicate([](const auto* Mesh)
                    { return Mesh->GetName() == TEXT("HeldToolPart2"); });
                TestTrue(TEXT("Bow string joins limb tips behind the grip"),
                    String->GetRelativeLocation().X < HandGrip.X - 10.0f);
            }
            else
            {
                TestTrue(TEXT("Tool shaft projects forward across the palm, perpendicular to the forearm"), ShaftAxis.X > 0.99f);
                if (Visual == EKalmalaHeldToolVisual::Axe)
                {
                    auto* Edge = *Meshes.FindByPredicate([](const auto* Mesh)
                        { return Mesh->GetName() == TEXT("HeldToolPart2"); });
                    TestTrue(TEXT("Axe cutting edge points down from the handle for the chopping stroke"),
                        Edge->GetRelativeLocation().Z < Handle->GetRelativeLocation().Z - 10.0f);
                }
            }
            for (const float Pitch : {0.0f, -25.0f, -115.0f, 35.0f})
            {
                Arm->SetRelativeRotation(FRotator(Pitch, 0, 0));
                TestTrue(TEXT("Grip stays in the hand through idle, gait and attack arm poses"),
                    Handle->GetComponentTransform().TransformPosition(GripInHandle).Equals(
                        Arm->GetComponentTransform().TransformPosition(HandGrip), 0.01f));
            }
            Arm->SetRelativeRotation(FRotator::ZeroRotator);
        }
    }
    return true;
}

#endif
