#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryGridWidget.h"
#include "Misc/AutomationTest.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Components/Border.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryGridWidgetTest, "Kalmala.UI.Inventory.NumberedHotbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaInventoryGridWidgetTest::RunTest(const FString& Parameters)
{
    TArray<FName> Slots;
    Slots.Init(NAME_None, 40);
    TestEqual(TEXT("Inventory always retains forty cells"), UKalmalaInventoryGridWidget::VisibleSlots(Slots, false).Num(), 40);
    TestTrue(TEXT("Empty hotbar hides every cell"), UKalmalaInventoryGridWidget::VisibleSlots(Slots, true).IsEmpty());
    Slots[0] = TEXT("StoneKnife"); Slots[3] = TEXT("HearthBroth"); Slots[9] = TEXT("StoneAxe"); Slots[10] = TEXT("Wood");
    const auto Visible = UKalmalaInventoryGridWidget::VisibleSlots(Slots, true);
    TestTrue(TEXT("Hotbar omits empty cells and lower rows without renumbering"), Visible == TArray<int32>({0, 3, 9}));
    TestEqual(TEXT("First cell is key one"), UKalmalaInventoryGridWidget::SlotLabel(0), FString(TEXT("1")));
    TestEqual(TEXT("Tenth cell is key zero, after nine"), UKalmalaInventoryGridWidget::SlotLabel(9), FString(TEXT("0")));
    TestTrue(TEXT("Lower rows have no hotbar number"), UKalmalaInventoryGridWidget::SlotLabel(10).IsEmpty());
    Slots[3] = NAME_None;
    TestTrue(TEXT("Consumed hotbar item removes only its HUD cell"),
        UKalmalaInventoryGridWidget::VisibleSlots(Slots, true) == TArray<int32>({0, 9}));
    TestTrue(TEXT("Equipment state does not remove the hotbar item"),
        UKalmalaInventoryGridWidget::VisibleSlots(Slots,true)==TArray<int32>({0,9}));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaEquippedHotbarTintTest,"Kalmala.UI.Inventory.EquippedHotbarTint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaEquippedHotbarTintTest::RunTest(const FString& Parameters)
{
    auto* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!World) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    auto* Pawn=World->SpawnActor<AKalmalaCharacter>();
    auto* Controller=World->SpawnActor<APlayerController>();
    Controller->SetPlayer(NewObject<ULocalPlayer>(GEngine)); Controller->Possess(Pawn);
    auto& Tools=const_cast<TArray<FKalmalaToolState>&>(Pawn->GetCarriedToolInventory());
    FKalmalaToolState Axe; Axe.ItemId=TEXT("StoneAxe"); Axe.Durability=50; Axe.InstanceGuid=FGuid::NewGuid();
    auto Other=Axe; Other.InstanceGuid=FGuid::NewGuid(); Tools={Axe,Other};
    auto* Inventory=Pawn->GetInventoryComponent(); Inventory->SynchronizeGridFromServer();
    Inventory->MoveSlotByInstanceFromServer(0,2,Axe.InstanceGuid,FGuid());
    Inventory->MoveSlotByInstanceFromServer(1,3,Other.InstanceGuid,FGuid());
    auto* Widget=CreateWidget<UKalmalaInventoryGridWidget>(Controller);
    if(!TestNotNull(TEXT("Actual grid widget initializes"),Widget)) return false;
    Widget->Refresh(Inventory,false,NAME_None);
    auto* Field=FindFProperty<FArrayProperty>(Widget->GetClass(),TEXT("Backgrounds"));
    auto* Backgrounds=Field->ContainerPtrToValuePtr<TArray<TObjectPtr<UBorder>>>(Widget);
    if(!TestEqual(TEXT("Forty real cell backgrounds exist"),Backgrounds->Num(),40)) return false;
    const auto Color=[&](int32 Slot){return CastChecked<UBorder>((*Backgrounds)[Slot]->GetContent())->GetBrushColor();};
    const auto Base=Color(2);
    FString Reason; Inventory->SetMainHandFromServer(Axe.InstanceGuid,Reason);
    Widget->Refresh(Inventory,false,NAME_None);
    TestTrue(TEXT("Equipped slot is subtly blue"),Color(2).B>Color(2).R && Color(2)!=Base);
    TestEqual(TEXT("Duplicate unequipped axe retains base fill"),Color(3),Base);
    TestEqual(TEXT("Blue axe stays in key-three cell"),Inventory->GetSlotInstanceGuid(2),Axe.InstanceGuid);
    Inventory->SetMainHandFromServer(Other.InstanceGuid,Reason); Widget->Refresh(Inventory,false,NAME_None);
    TestEqual(TEXT("Old slot tint clears when switching"),Color(2),Base);
    TestTrue(TEXT("New exact equipped copy turns blue"),Color(3).B>Color(3).R && Color(3)!=Base);
    Inventory->SetMainHandFromServer(FGuid(),Reason); Widget->Refresh(Inventory,false,NAME_None);
    TestEqual(TEXT("Unequipping restores normal fill"),Color(3),Base);
    TestEqual(TEXT("Unequipping leaves axe in key-three cell"),Inventory->GetSlotInstanceGuid(2),Axe.InstanceGuid);
    return true;
}

#endif
