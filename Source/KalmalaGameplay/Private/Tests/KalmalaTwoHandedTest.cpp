#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaTwoHandedTest,
    "Kalmala.Gameplay.Inventory.TwoHanded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaTwoHandedTest::RunTest(const FString& Parameters)
{
    const auto* Items = UKalmalaItemCatalogue::Get();
    const auto* Tools = UKalmalaToolCatalogue::Get();
    const auto* Recipes = UKalmalaRecipeCatalogue::Get();
    if (!TestNotNull(TEXT("Items load"), Items) || !TestNotNull(TEXT("Tools load"), Tools)
        || !TestNotNull(TEXT("Recipes load"), Recipes)) return false;
    TestTrue(TEXT("Bow requires both hands"), FKalmalaToolLifecycleContract::IsTwoHanded(TEXT("Bow")));
    TestFalse(TEXT("Hammer remains one handed"), FKalmalaToolLifecycleContract::IsTwoHanded(TEXT("ConstructionHammer")));
    TestFalse(TEXT("Unknown item cannot claim both hands"), FKalmalaToolLifecycleContract::IsTwoHanded(TEXT("ForgedBow")));
    for (const auto& Expected : {TPair<FName, int32>(TEXT("String"), 20), TPair<FName, int32>(TEXT("WoodenArrow"), 100)})
    {
        const auto* Item = Items->FindItem(Expected.Key);
        if (!TestNotNull(TEXT("New stackable item exists"), Item)) return false;
        TestEqual(TEXT("Authored stack limit"), Item->MaxStack, Expected.Value);
        TArray<FKalmalaInventoryStack> Granted;
        FString Reason;
        TestTrue(TEXT("Grant splits across stack boundaries"), UKalmalaInventoryComponent::BuildGrant(
            {}, Expected.Key, Expected.Value * 2 + 1, Granted, Reason));
        if (TestEqual(TEXT("Two full stacks and one remainder"), Granted.Num(), 3))
        {
            TestEqual(TEXT("First full stack"), Granted[0].Quantity, Expected.Value);
            TestEqual(TEXT("Second full stack"), Granted[1].Quantity, Expected.Value);
            TestEqual(TEXT("Remainder stack"), Granted[2].Quantity, 1);
            TSet<FGuid> Units;
            for (const auto& Stack : Granted) for (const FGuid& Guid : Stack.ItemGuids) Units.Add(Guid);
            TestEqual(TEXT("Every unit retains distinct identity"), Units.Num(), Expected.Value * 2 + 1);
        }
    }
    const auto* BowRecipe = Tools->Recipes.FindByPredicate([](const FKalmalaToolRecipeData& Entry)
        { return Entry.ItemId == TEXT("Bow"); });
    if (!TestNotNull(TEXT("Bow acquisition exists"), BowRecipe)) return false;
    TestEqual(TEXT("Bow crafted at Workbench"), BowRecipe->RequiredStation, FString(TEXT("Workbench")));
    TestEqual(TEXT("Bow has exactly two ingredients"), BowRecipe->Ingredients.Num(), 2);
    TestTrue(TEXT("Bow costs five normal Wood and two String"),
        BowRecipe->Ingredients.ContainsByPredicate([](const auto& Cost) { return Cost.ItemId == TEXT("Wood") && Cost.Quantity == 5; })
        && BowRecipe->Ingredients.ContainsByPredicate([](const auto& Cost) { return Cost.ItemId == TEXT("String") && Cost.Quantity == 2; }));
    const auto* ArrowRecipe = Recipes->Recipes.FindByPredicate([](const FKalmalaRecipe& Entry)
        { return Entry.Output == TEXT("WoodenArrow"); });
    if (!TestNotNull(TEXT("Arrow recipe exists"), ArrowRecipe)) return false;
    TestEqual(TEXT("Arrow batch yields twenty"), ArrowRecipe->OutputCount, 20);
    TestTrue(TEXT("Arrow batch costs only five normal Wood"), ArrowRecipe->Ingredients.Num() == 1
        && ArrowRecipe->Ingredients[0].ItemId == TEXT("Wood") && ArrowRecipe->Ingredients[0].Quantity == 5);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Isolated authority world exists"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    auto* Controller = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Pawn spawned without profile loading"), Pawn)
        || !TestNotNull(TEXT("Hotbar owner controller exists"), Controller)) return false;
    Controller->Possess(Pawn);
    auto* Inventory = Pawn->GetInventoryComponent();
    if (!TestNotNull(TEXT("Equipment inventory exists"), Inventory)) return false;
    auto UseNumber = [&](const int32 Slot)
    {
        World->Tick(LEVELTICK_TimeOnly, 0.25f);
        Inventory->ServerUseHotbarSlot(Slot);
    };
    const FArrayProperty* Property = FindFProperty<FArrayProperty>(Pawn->GetClass(), TEXT("CarriedTools"));
    if (!TestNotNull(TEXT("Carried fixture field exists"), Property)) return false;
    const FStructProperty* Record = CastField<FStructProperty>(Property->Inner);
    if (!TestTrue(TEXT("Fixture record type matches"), Record && Record->Struct == FKalmalaToolState::StaticStruct())) return false;
    FKalmalaToolState Hammer, Bow;
    Hammer.ItemId = TEXT("ConstructionHammer");
    Hammer.Durability = FKalmalaToolLifecycleContract::GetMaxDurability(Hammer.ItemId, 1);
    Hammer.InstanceGuid = FGuid::NewGuid();
    Bow.ItemId = TEXT("Bow");
    Bow.Durability = FKalmalaToolLifecycleContract::GetMaxDurability(Bow.ItemId, 1);
    Bow.InstanceGuid = FGuid::NewGuid();
    *Property->ContainerPtrToValuePtr<TArray<FKalmalaToolState>>(Pawn) = {Hammer, Bow};
    Inventory->SynchronizeGridFromServer();
    FString Reason;
    if (!TestTrue(TEXT("Wield hammer"), Inventory->SetMainHandFromServer(Hammer.InstanceGuid, Reason))
        || !TestTrue(TEXT("Grant shield"), Inventory->TryGrantFromServer(TEXT("WoodShield"), 1))) return false;
    const FGuid ShieldGuid = Inventory->GetStacks()[0].ItemGuids[0];
    if (!TestTrue(TEXT("Equip shield alongside hammer"), Inventory->EquipGearFromServer(ShieldGuid, Reason))) return false;
    Inventory->WearEquippedGearFromServer();
    TArray<FKalmalaInventoryStack> FullPack;
    for (int32 Index = 0; Index < 39; ++Index)
    {
        FKalmalaInventoryStack Stack;
        Stack.ItemId = TEXT("BoarHideBoots"); Stack.Quantity = 1;
        Stack.InstanceGuid = FGuid::NewGuid(); Stack.ItemGuids.Add(FGuid::NewGuid());
        FullPack.Add(Stack);
    }
    const auto Empty = Inventory->GetStacks();
    if (!TestTrue(TEXT("Fill forty pack cells including packed bow"), Inventory->TryCommitStacksFromServer(Empty, FullPack))) return false;
    const auto Before = Inventory->GetGridInstanceGuids();
    TestFalse(TEXT("Full pack rejects returning two held items for bow"), Inventory->SetMainHandFromServer(Bow.InstanceGuid, Reason));
    const int32 BowSlot = Inventory->GetGridInstanceGuids().IndexOfByKey(Bow.InstanceGuid);
    if (!TestTrue(TEXT("Packed bow has a numbered cell"), BowSlot >= 0 && BowSlot < Inventory->Columns)) return false;
    UseNumber(BowSlot);
    TestTrue(TEXT("Rejected equip preserves hammer"), Inventory->GetMainHandGuid() == Hammer.InstanceGuid);
    TestTrue(TEXT("Rejected equip preserves complete layout"), Inventory->GetGridInstanceGuids() == Before);
    TestEqual(TEXT("Rejected hotbar swap preserves all stack records"), Inventory->GetStacks().Num(), 39);
    const auto* Shield = Inventory->FindEquippedGear(EKalmalaGearSlot::Shield);
    if (!TestNotNull(TEXT("Rejected equip preserves worn shield"), Shield)) return false;
    TestTrue(TEXT("Rejected equip preserves shield GUID and wear"), Shield->ItemGuid == ShieldGuid && Shield->ConditionSpent == 1);

    if (!TestTrue(TEXT("Free exactly one cell"), Inventory->TryConsumeFromServer(TEXT("BoarHideBoots"), 1))) return false;
    TestTrue(TEXT("Freeing capacity preserves bow's numbered cell"), Inventory->GetSlotInstanceGuid(BowSlot) == Bow.InstanceGuid);
    UseNumber(BowSlot);
    if (!TestTrue(TEXT("One free cell accepts hotbar two-hand swap"), Inventory->GetMainHandGuid() == Bow.InstanceGuid)) return false;
    TestNull(TEXT("Bow cannot coexist with worn shield"), Inventory->FindEquippedGear(EKalmalaGearSlot::Shield));
    TestTrue(TEXT("Old weapon returned without identity change"), Inventory->GetGridInstanceGuids().Contains(Hammer.InstanceGuid));
    TestTrue(TEXT("Shield returned with original physical identity"), Inventory->GetStacks().ContainsByPredicate(
        [ShieldGuid](const auto& Stack) { return Stack.ItemGuids.Contains(ShieldGuid); }));
    TestFalse(TEXT("Bow has no pack alias"), Inventory->GetGridInstanceGuids().Contains(Bow.InstanceGuid));
    TestFalse(TEXT("Full pack rejects unwielding bow"), Inventory->SetMainHandFromServer(FGuid(), Reason));
    TestTrue(TEXT("Rejected unwield preserves bow"), Inventory->GetMainHandGuid() == Bow.InstanceGuid);
    const auto BeforeOrdinary = Inventory->GetGridInstanceGuids();
    const int32 BootsSlot = Inventory->GetGridSlots().IndexOfByKey(TEXT("BoarHideBoots"));
    if (!TestTrue(TEXT("Ordinary fixture has a numbered cell"), BootsSlot >= 0 && BootsSlot < Inventory->Columns)) return false;
    UseNumber(BootsSlot);
    TestTrue(TEXT("Full-pack ordinary hotbar selection preserves bow"), Inventory->GetMainHandGuid() == Bow.InstanceGuid);
    TestTrue(TEXT("Rejected ordinary selection preserves all cells"), Inventory->GetGridInstanceGuids() == BeforeOrdinary);
    for (const int32 Slot : {-1, Inventory->Columns, MAX_int32}) UseNumber(Slot);
    TestTrue(TEXT("Invalid hotbar numbers preserve two-handed wielding"), Inventory->GetMainHandGuid() == Bow.InstanceGuid);
    TestTrue(TEXT("Invalid hotbar numbers preserve full layout"), Inventory->GetGridInstanceGuids() == BeforeOrdinary);

    if (!TestTrue(TEXT("Equipping packed shield returns bow using shield's vacated cell"),
        Inventory->EquipGearFromServer(ShieldGuid, Reason))) return false;
    TestFalse(TEXT("Shield selection clears two-handed wielding"), Inventory->GetMainHandGuid().IsValid());
    TestTrue(TEXT("Bow returns to the pack"), Inventory->GetGridInstanceGuids().Contains(Bow.InstanceGuid));
    Shield = Inventory->FindEquippedGear(EKalmalaGearSlot::Shield);
    if (!TestNotNull(TEXT("Shield is equipped"), Shield)) return false;
    TestEqual(TEXT("Swap preserved shield condition"), Shield->ConditionSpent, 1);
    const int32 ReturnedBowSlot = Inventory->GetGridInstanceGuids().IndexOfByKey(Bow.InstanceGuid);
    if (!TestTrue(TEXT("Returned bow has a numbered cell"), ReturnedBowSlot >= 0 && ReturnedBowSlot < Inventory->Columns)) return false;
    UseNumber(ReturnedBowSlot);
    if (!TestTrue(TEXT("Shield-only full pack accepts bow through hotbar"), Inventory->GetMainHandGuid() == Bow.InstanceGuid)) return false;
    TestNull(TEXT("Shield again returned"), Inventory->FindEquippedGear(EKalmalaGearSlot::Shield));
    if (!TestTrue(TEXT("Free room for unwield"), Inventory->TryConsumeFromServer(TEXT("BoarHideBoots"), 1))) return false;
    TestTrue(TEXT("Unwield bow with one free cell"), Inventory->SetMainHandFromServer(FGuid(), Reason));
    TestFalse(TEXT("Both hands clear after unwield"), Inventory->GetMainHandGuid().IsValid());
    TestTrue(TEXT("Unwield preserves physical bow"), Inventory->GetGridInstanceGuids().Contains(Bow.InstanceGuid));
    for (const FKalmalaToolState& Original : {Hammer, Bow})
    {
        const auto* Current = Pawn->GetCarriedToolInventory().FindByPredicate(
            [&](const auto& Tool) { return Tool.InstanceGuid == Original.InstanceGuid; });
        if (TestNotNull(TEXT("Hotbar swaps retain original tool record"), Current))
        {
            TestEqual(TEXT("Hotbar swaps preserve item identity"), Current->ItemId, Original.ItemId);
            TestEqual(TEXT("Hotbar swaps preserve level"), Current->ToolLevel, Original.ToolLevel);
            TestEqual(TEXT("Hotbar swaps preserve condition"), Current->Durability, Original.Durability);
        }
    }
    Pawn->SetRole(ROLE_SimulatedProxy);
    TestFalse(TEXT("Client role cannot wield bow"), Inventory->SetMainHandFromServer(Bow.InstanceGuid, Reason));
    Pawn->SetRole(ROLE_Authority);
    return true;
}
#endif
