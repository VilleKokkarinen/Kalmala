#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOpalHelmTest, "Kalmala.Gameplay.Inventory.OpalHelm",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaOpalHelmTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    auto* Source = World->SpawnActor<AKalmalaCharacter>();
    if (!Pawn || !Source) { World->DestroyWorld(false); return false; }
    auto* Inventory = Pawn->GetInventoryComponent();
    FString Reason;
    Inventory->TryGrantFromServer(TEXT("OpalHelm"), 1);
    const auto* Stack = Inventory->GetStacks().FindByPredicate([](const auto& Entry) { return Entry.ItemId == TEXT("OpalHelm"); });
    if (!TestNotNull(TEXT("Helm fixture has a physical item"), Stack)) { World->DestroyWorld(false); return false; }
    const FGuid Guid = Stack->ItemGuids[0];
    if (!TestTrue(TEXT("Helm equips"), Inventory->EquipGearFromServer(Guid, Reason))) { World->DestroyWorld(false); return false; }
    Inventory->TryGrantFromServer(TEXT("Iron"), 10);
    Inventory->TryGrantFromServer(TEXT("DeerHide"), 4);
    TestFalse(TEXT("Unowned GUID cannot spend materials"), Inventory->UpgradeEquippedOpalFromServer(FGuid::NewGuid(), Reason));
    TestEqual(TEXT("Rejected request preserves materials"), Inventory->GetQuantity(TEXT("Iron")), 10);
    TestFalse(TEXT("No-controller/no-station gameplay request fails closed"),
        Pawn->FindComponentByClass<UKalmalaCraftingComponent>()->MaintainOpalHelmFromServer(Guid, true, Reason));
    TestTrue(TEXT("Trusted server transaction upgrades helm"), Inventory->UpgradeEquippedOpalFromServer(Guid, Reason));
    const auto* Gear = Inventory->FindEquippedGear(EKalmalaGearSlot::Head);
    TestTrue(TEXT("Upgrade retains exact item identity"), Gear && Gear->ItemGuid == Guid && Gear->Level == 2);
    TestEqual(TEXT("Upgrade spends authored Iron"), Inventory->GetQuantity(TEXT("Iron")), 5);
    TestEqual(TEXT("Upgrade spends authored hide"), Inventory->GetQuantity(TEXT("DeerHide")), 2);
    FKalmalaGearLevelStats Stats;
    UKalmalaItemCatalogue::Get()->GetGearLevelStats(TEXT("OpalHelm"), 2, Stats);
    TestEqual(TEXT("Level two adds one Armor"), Stats.Armor, 13);
    TestEqual(TEXT("Level two adds ten condition"), Stats.MaxDurability, 160);
    TestFalse(TEXT("Next doubled payment cannot be partially spent"), Inventory->UpgradeEquippedOpalFromServer(Guid, Reason));
    TestEqual(TEXT("Failed scaled payment preserves Iron"), Inventory->GetQuantity(TEXT("Iron")), 5);
    FKalmalaDamageChannels Hit; Hit.Slash = 20.f;
    Pawn->ApplyWildlifeDamageFromServer(Source, Hit);
    TestEqual(TEXT("Damage resolves upgraded Armor"), Pawn->GetHealth(), 93.f);
    Gear = Inventory->FindEquippedGear(EKalmalaGearSlot::Head);
    TestTrue(TEXT("Accepted hit spends one wearable condition"), Gear && Gear->ConditionSpent == 1);
    TestTrue(TEXT("Trusted repair succeeds"), Inventory->RepairEquippedOpalFromServer(Guid, Reason));
    TestEqual(TEXT("Repair is free"), Inventory->GetQuantity(TEXT("Iron")), 5);
    TestEqual(TEXT("Repair removes wear"), Inventory->FindEquippedGear(EKalmalaGearSlot::Head)->ConditionSpent, 0);
    TestTrue(TEXT("Levelled helm returns to pack"), Inventory->UnequipGearFromServer(EKalmalaGearSlot::Head, Reason));
    TestTrue(TEXT("Same GUID can re-equip"), Inventory->EquipGearFromServer(Guid, Reason));
    TestEqual(TEXT("Same-pawn re-equip retains level"), Inventory->FindEquippedGear(EKalmalaGearSlot::Head)->Level, 2);
    TestFalse(TEXT("Full condition rejects unnecessary repair"), Inventory->RepairEquippedOpalFromServer(Guid, Reason));
    World->DestroyWorld(false);
    return true;
}
#endif
