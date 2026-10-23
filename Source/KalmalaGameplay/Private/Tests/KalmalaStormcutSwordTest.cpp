#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaPlayerModelComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStormcutSwordTest, "Kalmala.Gameplay.Combat.StormcutSword",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaStormcutSwordTest::RunTest(const FString& Parameters)
{
    const auto* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StormcutSword"));
    if (!TestNotNull(TEXT("Sword has a runtime lifecycle"), Definition)) return false;
    TestEqual(TEXT("Sword has its own supported kind"), Definition->Kind, EKalmalaToolKind::StormcutSword);
    TestEqual(TEXT("Sword is combat only"), Definition->Action, EKalmalaToolAction::None);
    TestFalse(TEXT("Sword uses one hand"), FKalmalaToolLifecycleContract::IsTwoHanded(TEXT("StormcutSword")));
    TestEqual(TEXT("Held silhouette is a sword"), UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(TEXT("StormcutSword")), EKalmalaHeldToolVisual::Sword);
    const auto Damage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(TEXT("StormcutSword"), 1, TEXT("Basic"));
    TestEqual(TEXT("Basic has seventy Slash"), Damage.Slash, 70.f);
    TestEqual(TEXT("No extra elemental/style damage"), Damage.Total(), 70.f);
    TestEqual(TEXT("Starting condition"), FKalmalaToolLifecycleContract::GetMaxDurability(TEXT("StormcutSword"), 1), 220);
    const TArray<FKalmalaInventoryStack> Materials = {{TEXT("Iron"),25}, {TEXT("Densewood"),3}, {TEXT("DeerHide"),2}, {TEXT("StormSapphire"),1}};
    TArray<FKalmalaToolState> Crafted, Upgraded;
    TArray<FKalmalaInventoryStack> AfterCraft, AfterUpgrade;
    FString Reason;
    const auto StartingTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
    TestFalse(TEXT("Client cannot craft a sword"), FKalmalaToolProgressionContract::BuildServerUpgrade(
        false, nullptr, TEXT("ForgeKit"), 1, TEXT("StormcutSword"), StartingTools, Materials, Crafted, AfterCraft, Reason));
    if (!TestTrue(TEXT("Paid Forge acquisition succeeds"), FKalmalaToolProgressionContract::BuildServerUpgrade(
        true, nullptr, TEXT("ForgeKit"), 1, TEXT("StormcutSword"), StartingTools, Materials, Crafted, AfterCraft, Reason))) return false;
    const auto* Sword = Crafted.FindByPredicate([](const auto& Tool) { return Tool.ItemId == TEXT("StormcutSword"); });
    if (!TestNotNull(TEXT("Craft creates a carried sword"), Sword)) return false;
    const FGuid Guid = Sword->InstanceGuid;
    TestTrue(TEXT("Paid Forge upgrade succeeds"), FKalmalaToolProgressionContract::BuildServerUpgrade(
        true, nullptr, TEXT("ForgeKit"), 2, TEXT("StormcutSword"), Crafted, AfterCraft, Upgraded, AfterUpgrade, Reason));
    const auto* LevelTwo = Upgraded.FindByPredicate([](const auto& Tool) { return Tool.ItemId == TEXT("StormcutSword"); });
    if (!TestNotNull(TEXT("Upgrade retains sword"), LevelTwo)) return false;
    TestTrue(TEXT("Upgrade retains physical identity and current condition"), LevelTwo->InstanceGuid == Guid && LevelTwo->ToolLevel == 2 && LevelTwo->Durability == 220);
    TestEqual(TEXT("Upgrade raises maximum condition by thirty"), FKalmalaToolLifecycleContract::GetMaxDurability(TEXT("StormcutSword"), 2), 250);
    TestTrue(TEXT("Exact acquisition and upgrade materials consumed"), AfterUpgrade.IsEmpty());
    TestEqual(TEXT("Upgrade adds four Slash"), FKalmalaToolLifecycleContract::GetAttackDamageBonuses(TEXT("StormcutSword"), 2, TEXT("Basic")).Slash, 74.f);
    auto Worn = *LevelTwo; Worn.Durability = 0;
    FKalmalaToolState Repaired;
    TestTrue(TEXT("Normal free station repair supports sword"), FKalmalaToolLifecycleContract::BuildServerFreeRepair(true, true, Worn, Repaired));
    TestEqual(TEXT("Repair restores level maximum"), Repaired.Durability, 250);
    return true;
}
#endif
