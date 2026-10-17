#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCombatComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerModelComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/ScopeExit.h"
#include <limits>
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCombatLoopTest, "Kalmala.Gameplay.Combat.BasicAttack.AuthorityAndCooldown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaCombatLoopTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Client role cannot begin an attack"), UKalmalaCombatComponent::IsAttackRequestAllowed(false, true, 1, true, true));
    TestFalse(TEXT("Zero sequence is rejected"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 0, true, true));
    TestFalse(TEXT("Repeated sequence is rejected"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, false, 2, true, true));
    TestFalse(TEXT("Busy attack cannot begin another action"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 3, false, true));
    TestFalse(TEXT("Server cannot accept a missing target"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 4, true, false));
    TestTrue(TEXT("Validated server intent begins one committed action"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 5, true, true));
    TestFalse(TEXT("Construction Hammer is build-only"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("ConstructionHammer")));
    TestFalse(TEXT("Bow remains outside the melee attack loop"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("Bow")));
    TestTrue(TEXT("Axe remains attack-capable"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("StoneAxe")));
    TestTrue(TEXT("Unarmed combat fallback remains available"), UKalmalaCombatComponent::IsAttackItemAllowed(NAME_None));
    TestTrue(TEXT("Combat recovery remains a positive committed window"), UKalmalaCombatComponent::GetRecoverySeconds() > 0.0f);
    TestEqual(TEXT("M5 combat recovery uses the tuned 0.36-second window"), UKalmalaCombatComponent::GetRecoverySeconds(), 0.36f);
    TestFalse(TEXT("Clients cannot apply wildlife combat damage"), AKalmalaWildlifeSpawn::IsDefeatAllowed(false, false));
    TestNotEqual(TEXT("Hit feedback is distinguishable from defeat"), uint8(EKalmalaCombatFeedback::Hit), uint8(EKalmalaCombatFeedback::Defeat));
    TestNotEqual(TEXT("Unavailable feedback is distinguishable from hit"), uint8(EKalmalaCombatFeedback::Unavailable), uint8(EKalmalaCombatFeedback::Hit));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaAxeSwingTest, "Kalmala.Gameplay.Combat.AxeSwing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaAxeSwingTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Usable Stone Axe can swing without a target"),
        UKalmalaCombatComponent::CanSwingAxeWithoutTarget(TEXT("StoneAxe"), 1, 50));
    TestFalse(TEXT("Broken axe cannot begin a targetless swing"),
        UKalmalaCombatComponent::CanSwingAxeWithoutTarget(TEXT("StoneAxe"), 1, 0));
    TestFalse(TEXT("Forged condition cannot authorize a swing"),
        UKalmalaCombatComponent::CanSwingAxeWithoutTarget(TEXT("StoneAxe"), 1, MAX_int32));
    TestFalse(TEXT("Unknown item cannot authorize a swing"),
        UKalmalaCombatComponent::CanSwingAxeWithoutTarget(TEXT("ForgedAxe"), 1, 50));
    TestFalse(TEXT("Hammer remains build-only"),
        UKalmalaCombatComponent::CanSwingAxeWithoutTarget(TEXT("ConstructionHammer"), 1, 100));
    TestFalse(TEXT("Pick does not enter targetless axe path"),
        UKalmalaCombatComponent::CanSwingAxeWithoutTarget(TEXT("StonePick"), 1, 20));

    float Pitch = 8.0f;
    TestTrue(TEXT("Swing starts at gait"), UKalmalaPlayerModelComponent::EvaluateAttackArmPose(0, 8, Pitch));
    TestEqual(TEXT("Initial pose has no snap"), Pitch, 8.0f);
    TestTrue(TEXT("Windup pose is available"), UKalmalaPlayerModelComponent::EvaluateAttackArmPose(0.18f, 8, Pitch));
    TestEqual(TEXT("Arm raises visibly"), Pitch, -115.0f);
    TestTrue(TEXT("Strike pose is available"), UKalmalaPlayerModelComponent::EvaluateAttackArmPose(0.30f, 8, Pitch));
    TestEqual(TEXT("Arm strikes down"), Pitch, 35.0f);
    TestTrue(TEXT("Swing blends back to gait"), UKalmalaPlayerModelComponent::EvaluateAttackArmPose(0.539f, 8, Pitch));
    TestTrue(TEXT("Recovery approaches gait"), FMath::IsNearlyEqual(Pitch, 8.0f, 0.2f));
    TestFalse(TEXT("Animation ends at bounded duration"), UKalmalaPlayerModelComponent::EvaluateAttackArmPose(0.54f, 8, Pitch));
    TestFalse(TEXT("Invalid time is rejected"), UKalmalaPlayerModelComponent::EvaluateAttackArmPose(
        std::numeric_limits<float>::quiet_NaN(), 8, Pitch));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Swing fixture world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Swing fixture pawn exists"), Pawn)) return false;
    Pawn->DispatchBeginPlay();
    auto* Inventory = Pawn->GetInventoryComponent();
    const auto Tools = Pawn->GetCarriedToolInventory();
    const auto* Axe = Tools.FindByPredicate([](const auto& Tool) { return Tool.ItemId == TEXT("StoneAxe"); });
    if (!TestNotNull(TEXT("Fresh pawn carries its starter axe"), Axe)) return false;
    Inventory->ServerSetMainHand(Axe->InstanceGuid);
    TestEqual(TEXT("Axe is actually equipped"), Inventory->GetActiveItem(), FName(TEXT("StoneAxe")));
    auto* Combat = Pawn->GetCombatComponent();
    Combat->ServerRequestAttack(1);
    TestEqual(TEXT("Ordinary intent starts an air swing"), Combat->GetActionPhase(), EKalmalaCombatActionPhase::Windup);
    TestEqual(TEXT("Accepted swing publishes one cosmetic serial"), Combat->GetActionSerial(), uint32(1));
    auto* Model = Pawn->FindComponentByClass<UKalmalaPlayerModelComponent>();
    if (!TestNotNull(TEXT("Pawn has its actual procedural player model"), Model)) return false;
    Model->TickComponent(0.18f, LEVELTICK_All, nullptr);
    Model->TickComponent(0.01f, LEVELTICK_All, nullptr);
    TArray<UProceduralMeshComponent*> MeshParts;
    Pawn->GetComponents(MeshParts);
    const auto* Arm = MeshParts.FindByPredicate([](const auto* Part) { return Part->GetFName() == TEXT("PlayerRightArm"); });
    if (TestNotNull(TEXT("Animated right arm exists"), Arm ? *Arm : nullptr))
        TestTrue(TEXT("Accepted action drives actual arm geometry"),
            (*Arm)->GetRelativeRotation().Quaternion().Equals(FRotator(-115.0f, 0, 0).Quaternion(), 0.001f));
    Combat->ServerRequestAttack(2);
    TestEqual(TEXT("Busy intent cannot restart animation"), Combat->GetActionSerial(), uint32(1));
    World->Tick(LEVELTICK_TimeOnly, 0.20f);
    Combat->TickComponent(0.20f, LEVELTICK_All, nullptr);
    TestEqual(TEXT("Air swing enters recovery"), Combat->GetActionPhase(), EKalmalaCombatActionPhase::Recovery);
    TestEqual(TEXT("Miss does not report attack unavailable"), Combat->GetFeedback(), EKalmalaCombatFeedback::None);
    TestEqual(TEXT("Miss spends no tool condition"), Pawn->GetToolDurability(TEXT("StoneAxe")), Axe->Durability);
    return true;
}
#endif
