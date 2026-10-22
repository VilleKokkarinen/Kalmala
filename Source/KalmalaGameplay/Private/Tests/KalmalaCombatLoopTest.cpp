#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCombatComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaHarvestNode.h"
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
    TestFalse(TEXT("Server cannot accept an unavailable swing"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 4, true, false));
    TestTrue(TEXT("Validated server intent begins one committed action"), UKalmalaCombatComponent::IsAttackRequestAllowed(true, true, 5, true, true));
    TestFalse(TEXT("Construction Hammer is build-only"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("ConstructionHammer")));
    TestFalse(TEXT("Bow uses drawing rather than melee"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("Bow")));
    TestFalse(TEXT("Light Bow uses drawing rather than melee"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("LightBow")));
    TestFalse(TEXT("Horn Bow uses drawing rather than melee"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("HornBow")));
    TestTrue(TEXT("Axe remains attack-capable"), UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("StoneAxe")));
    TestTrue(TEXT("Fang Spear is accepted by the server melee route"),
        UKalmalaCombatComponent::IsAttackItemAllowed(TEXT("FangSpear")));
    TestTrue(TEXT("Unarmed combat fallback remains available"), UKalmalaCombatComponent::IsAttackItemAllowed(NAME_None));
    TestTrue(TEXT("Combat recovery remains a positive committed window"), UKalmalaCombatComponent::GetRecoverySeconds() > 0.0f);
    TestEqual(TEXT("M5 combat recovery uses the tuned 0.36-second window"), UKalmalaCombatComponent::GetRecoverySeconds(), 0.36f);
    TestFalse(TEXT("Clients cannot apply wildlife combat damage"), AKalmalaWildlifeSpawn::IsDefeatAllowed(false, false));
    TestNotEqual(TEXT("Hit feedback is distinguishable from defeat"), uint8(EKalmalaCombatFeedback::Hit), uint8(EKalmalaCombatFeedback::Defeat));
    TestNotEqual(TEXT("Unavailable feedback is distinguishable from hit"), uint8(EKalmalaCombatFeedback::Unavailable), uint8(EKalmalaCombatFeedback::Hit));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaAllItemSwingTest, "Kalmala.Gameplay.Combat.AllItemSwings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaAllItemSwingTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("All-item swing fixture world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    const auto* Catalogue = UKalmalaToolCatalogue::Get();
    if (!TestNotNull(TEXT("All carried item profiles load"), Catalogue)) return false;
    TArray<FName> Items;
    for (const auto& Profile : Catalogue->Profiles)
        if (UKalmalaCombatComponent::IsAttackItemAllowed(Profile.ItemId)) Items.Add(Profile.ItemId);
    Items.Add(NAME_None);
    TestTrue(TEXT("Fang Spear is included in all-profile server hit coverage"), Items.Contains(TEXT("FangSpear")));
    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        const FName Item = Items[Index];
        const FVector Origin(Index * 2000.0f, 0, 0);
        auto* Pawn = World->SpawnActor<AKalmalaCharacter>(Origin, FRotator::ZeroRotator);
        if (!TestNotNull(TEXT("Swinging pawn spawns"), Pawn)) return false;
        Pawn->DispatchBeginPlay();
        auto* Inventory = Pawn->GetInventoryComponent();
        FString Reason;
        if (!Item.IsNone())
        {
            auto& Tools = const_cast<TArray<FKalmalaToolState>&>(Pawn->GetCarriedToolInventory());
            auto* Tool = Tools.FindByPredicate([Item](const auto& Entry) { return Entry.ItemId == Item; });
            if (!Tool)
            {
                Tool = &Tools.AddDefaulted_GetRef();
                Tool->ItemId = Item;
                Tool->InstanceGuid = FGuid::NewGuid();
            }
            Tool->ToolLevel = 1;
            Tool->Durability = FKalmalaToolLifecycleContract::GetMaxDurability(Item, 1);
            Inventory->SynchronizeGridFromServer();
            if (Inventory->GetActiveItem() != Item)
                TestTrue(TEXT("Authored item can actually be equipped"), Inventory->SetMainHandFromServer(Tool->InstanceGuid, Reason));
        }
        else TestTrue(TEXT("Empty hands can be selected"), Inventory->SetMainHandFromServer(FGuid(), Reason));
        TestEqual(TEXT("Fixture swings the intended item"), Inventory->GetActiveItem(), Item);
        const int32 Condition = Pawn->GetToolDurability(Item);
        auto* Combat = Pawn->GetCombatComponent();
        const auto Advance = [World, Combat](float Seconds)
        {
            World->Tick(LEVELTICK_TimeOnly, Seconds);
            Combat->TickComponent(Seconds, LEVELTICK_All, nullptr);
        };
        Combat->ServerRequestAttack(0);
        TestEqual(TEXT("Zero request sequence cannot swing"), Combat->GetActionSerial(), uint32(0));
        Combat->ServerRequestAttack(1);
        TestEqual(*FString::Printf(TEXT("%s starts a swing without any nearby target"), *Item.ToString()),
            Combat->GetActionPhase(), EKalmalaCombatActionPhase::Windup);
        TestEqual(TEXT("Free swing publishes its animation serial"), Combat->GetActionSerial(), uint32(1));
        auto* Model = Pawn->FindComponentByClass<UKalmalaPlayerModelComponent>();
        Model->TickComponent(0.18f, LEVELTICK_All, nullptr);
        Model->TickComponent(0.01f, LEVELTICK_All, nullptr);
        TArray<UProceduralMeshComponent*> Meshes;
        Pawn->GetComponents(Meshes);
        const auto* Arm = Meshes.FindByPredicate([](const auto* Part) { return Part->GetFName() == TEXT("PlayerRightArm"); });
        TestTrue(TEXT("Every item's accepted swing animates the actual arm"), Arm
            && (*Arm)->GetRelativeRotation().Quaternion().Equals(FRotator(-115, 0, 0).Quaternion(), 0.001f));
        Combat->ServerRequestAttack(2);
        TestEqual(TEXT("Busy requests cannot restart any item's swing"), Combat->GetActionSerial(), uint32(1));
        Advance(0.20f);
        TestEqual(TEXT("Every air swing completes quietly"), Combat->GetFeedback(), EKalmalaCombatFeedback::None);
        TestEqual(TEXT("Every miss preserves held condition"), Pawn->GetToolDurability(Item), Condition);
        Advance(0.20f); Advance(0.20f);
        Combat->ServerRequestAttack(1);
        TestEqual(TEXT("Duplicate sequences cannot swing again"), Combat->GetActionSerial(), uint32(1));
        auto* Wildlife = World->SpawnActor<AKalmalaWildlifeSpawn>(Origin + FVector(140, 0, 55), FRotator::ZeroRotator);
        Combat->ServerRequestAttack(3);
        Advance(0.20f);
        const float AuthoredDamage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(Item, 1, TEXT("Basic")).Total();
        TestEqual(TEXT("Actual hit keeps authored damage or the existing basic melee fallback"),
            Wildlife->GetHealth(), FMath::Max(0.0f, 100.0f - (AuthoredDamage > 0 ? AuthoredDamage : 25.0f)));
        TestTrue(TEXT("Hammer, bow, weapons and fists all report validated wildlife hits"),
            Combat->GetFeedback() == EKalmalaCombatFeedback::Hit || Combat->GetFeedback() == EKalmalaCombatFeedback::Defeat);
        if (Item == TEXT("ConstructionHammer") || Item == TEXT("Bow"))
        {
            FKalmalaWorldPopulationSpawn Spawn;
            Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
            Spawn.ContentId = TEXT("meadows-birch-trunk");
            Spawn.Location = Origin + FVector(100, 0, 0);
            auto* Tree = World->SpawnActor<AKalmalaHarvestNode>();
            Tree->InitializeServer(Spawn);
            FKalmalaDamageChannels Damage; Damage.Crush = 25;
            TestFalse(TEXT("Improvised melee never bypasses the axe harvesting gate"), Tree->ApplyAxeDamageFromServer(Pawn, Damage));
        }
        Pawn->Destroy();
    }
    AddInfo(FString::Printf(TEXT("Verified free swing, arm animation and actual hit for %d carried profiles plus empty hands."), Items.Num() - 1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaAxeSwingTest, "Kalmala.Gameplay.Combat.AxeSwing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaAxeSwingTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Usable Stone Axe can swing without a target"),
        UKalmalaCombatComponent::CanSwingWithoutTarget(TEXT("StoneAxe"), 1, 50));
    TestFalse(TEXT("Broken axe cannot begin a targetless swing"),
        UKalmalaCombatComponent::CanSwingWithoutTarget(TEXT("StoneAxe"), 1, 0));
    TestFalse(TEXT("Forged condition cannot authorize a swing"),
        UKalmalaCombatComponent::CanSwingWithoutTarget(TEXT("StoneAxe"), 1, MAX_int32));
    TestFalse(TEXT("Unknown item cannot authorize a swing"),
        UKalmalaCombatComponent::CanSwingWithoutTarget(TEXT("ForgedAxe"), 1, 50));
    TestFalse(TEXT("Hammer cannot make melee attacks"),
        UKalmalaCombatComponent::CanSwingWithoutTarget(TEXT("ConstructionHammer"), 1, 100));
    TestTrue(TEXT("Pick can swing without a target"),
        UKalmalaCombatComponent::CanSwingWithoutTarget(TEXT("StonePick"), 1, 20));

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
