#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaToolProgressionContract.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOpalHelmTest, "Kalmala.Gameplay.Inventory.OpalHelm",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaOpalHelmTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
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
    auto* Controller = World->SpawnActor<APlayerController>();
    Controller->Possess(Pawn);
    Source->SetActorLocation(FVector(-1000,0,0));
    auto* Forge = World->SpawnActor<AKalmalaConstructionActor>(FVector(140,0,0), FRotator::ZeroRotator);
    Forge->InitializeFromServer(TEXT("ForgeKit"), TEXT("OpalForge"));
    auto* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    TestFalse(TEXT("Forge one cannot perform a level-two upgrade"), Crafting->MaintainOpalHelmFromServer(Guid, true, Reason));
    TestEqual(TEXT("Wrong station level preserves payment"), Inventory->GetQuantity(TEXT("Iron")), 10);
    auto* Anvil = World->SpawnActor<AKalmalaConstructionActor>(FVector(140,100,0), FRotator::ZeroRotator);
    Anvil->InitializeFromServer(TEXT("ForgeAnvilKit"), TEXT("OpalAnvil"));
    TestEqual(TEXT("Anvil creates effective Forge two"), FKalmalaToolProgressionContract::GetEffectiveStationLevel(Forge), 2);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(OpalForgeTest), false, Pawn);
    FHitResult Trace;
    const bool bVisible = World->LineTraceSingleByChannel(Trace, Pawn->GetPawnViewLocation(), Forge->GetActorLocation(), ECC_Visibility, Query);
    AddInfo(FString::Printf(TEXT("Forge fixture: Controller=%d Config=%d Trace=%d Hit=%s View=%s Forge=%s"), Pawn->GetController()!=nullptr,
        World->GetGameState<AKalmalaWorldGenerationGameState>()->GetWorldGenerationConfig().IsValid(), bVisible, *GetNameSafe(Trace.GetActor()),
        *Pawn->GetPawnViewLocation().ToCompactString(), *Forge->GetActorLocation().ToCompactString()));
    if (!TestTrue(TEXT("Nearby Forge two upgrades through the gameplay maintenance route"), Crafting->MaintainOpalHelmFromServer(Guid, true, Reason)))
    { AddInfo(Reason); World->DestroyWorld(false); return false; }
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
    Source->SetActorLocation(Pawn->GetActorLocation()+FVector(0,100,0));
    Pawn->ApplyWildlifeDamageFromServer(Source, Hit);
    TestEqual(TEXT("Damage resolves upgraded Armor"), Pawn->GetHealth(), 93.f);
    Gear = Inventory->FindEquippedGear(EKalmalaGearSlot::Head);
    TestTrue(TEXT("Accepted hit spends one wearable condition"), Gear && Gear->ConditionSpent == 1);
    Forge->SetActorLocation(FVector(1000,0,0));
    TestFalse(TEXT("Distant Forge cannot repair the helm"), Crafting->MaintainOpalHelmFromServer(Guid, false, Reason));
    Forge->SetActorLocation(FVector(140,0,0));
    TestTrue(TEXT("Nearby Forge repairs through the gameplay route"), Crafting->MaintainOpalHelmFromServer(Guid, false, Reason));
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
