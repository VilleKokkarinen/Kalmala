#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryNetworkContractTest,
    "Kalmala.Gameplay.Inventory.NetworkContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryNetworkContractTest::RunTest(const FString& Parameters)
{
    const UFunction* Intent = AKalmalaCharacter::StaticClass()->FindFunctionByName(TEXT("ServerRequestInteract"));
    if (TestNotNull(TEXT("Harvest uses the existing interaction intent"), Intent))
    {
        TestTrue(TEXT("Interaction intent is a server RPC"), Intent->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Interaction accepts only the selected tool and action"), int32(Intent->NumParms), 2);
        const FProperty* ToolParameter = Intent->FindPropertyByName(TEXT("ClientItemId"));
        const FProperty* ActionParameter = Intent->FindPropertyByName(TEXT("ClientAction"));
        TestTrue(TEXT("Tool intent is an item-independent name"), CastField<FNameProperty>(ToolParameter) != nullptr);
        TestTrue(TEXT("Action intent is a bounded byte enum value"), CastField<FByteProperty>(ActionParameter) != nullptr);
        for (TFieldIterator<FProperty> Property(Intent); Property; ++Property)
        {
            const FString Name = Property->GetName();
            TestTrue(*FString::Printf(TEXT("Intent excludes targets, rewards, quantities, and outcomes (%s)"), *Name),
                Name == TEXT("ClientItemId") || Name == TEXT("ClientAction") || !Property->HasAnyPropertyFlags(CPF_Parm));
        }
    }
    for (UClass* Class : { UKalmalaInventoryComponent::StaticClass(), AKalmalaHarvestNode::StaticClass() })
    {
        for (TFieldIterator<UFunction> Function(Class, EFieldIteratorFlags::ExcludeSuper); Function; ++Function)
        {
            const bool bLayoutIntent = Class == UKalmalaInventoryComponent::StaticClass()
                && (Function->GetFName() == TEXT("ServerMoveSlot")
                    || Function->GetFName() == TEXT("ServerMoveSlotByInstance")
                    || Function->GetFName() == TEXT("ServerUseHotbarSlot")
                    || Function->GetFName() == TEXT("ServerSetMainHand")
                    || Function->GetFName() == TEXT("ServerEquipGear")
                    || Function->GetFName() == TEXT("ServerUnequipGear"));
            TestTrue(*FString::Printf(TEXT("Only validated inventory intents are client-callable: %s"), *Function->GetName()),
                !Function->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer) || bLayoutIntent);
        }
    }
    const UFunction* MainHand = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ServerSetMainHand"));
    if (TestNotNull(TEXT("Main Hand intent exists"), MainHand))
    {
        TestTrue(TEXT("Main Hand intent is a reliable server RPC"),
            MainHand->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
        const FStructProperty* ItemGuid = CastField<FStructProperty>(MainHand->FindPropertyByName(TEXT("ItemGuid")));
        TestEqual(TEXT("Main Hand intent carries only one physical identity"), int32(MainHand->NumParms), 1);
        TestTrue(TEXT("Main Hand intent excludes client-authored stats and item types"),
            ItemGuid && ItemGuid->Struct == TBaseStructure<FGuid>::Get());
    }
    const UFunction* Hotbar = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ServerUseHotbarSlot"));
    if (TestNotNull(TEXT("Numbered hotbar intent exists"), Hotbar))
    {
        TestTrue(TEXT("Hotbar intent is a reliable server RPC"),
            Hotbar->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
        TestEqual(TEXT("Hotbar sends only the numbered cell"), int32(Hotbar->NumParms), 1);
        TestNotNull(TEXT("Server resolves the item from an integer cell index"),
            CastField<FIntProperty>(Hotbar->FindPropertyByName(TEXT("Slot"))));
    }
    const UFunction* Result = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ClientGearRequestResult"));
    if (TestNotNull(TEXT("Gear result channel exists"), Result))
    {
        TestTrue(TEXT("Gear result returns reliably to the owning client"),
            Result->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient | FUNC_NetReliable));
        TestFalse(TEXT("Gear result is never broadcast to peers"), Result->HasAnyFunctionFlags(FUNC_NetMulticast));
    }
    const UFunction* EquipGear = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ServerEquipGear"));
    if (TestNotNull(TEXT("Wearable equip RPC exists"), EquipGear))
    {
        const FStructProperty* ItemGuid = CastField<FStructProperty>(EquipGear->FindPropertyByName(TEXT("ItemGuid")));
        TestEqual(TEXT("Equip sends one exact physical item identity"), int32(EquipGear->NumParms), 1);
        TestTrue(TEXT("Equip intent carries a GUID rather than a client item ID"),
            ItemGuid && ItemGuid->Struct == TBaseStructure<FGuid>::Get());
    }
    const UFunction* UnequipGear = UKalmalaInventoryComponent::StaticClass()->FindFunctionByName(TEXT("ServerUnequipGear"));
    if (TestNotNull(TEXT("Wearable unequip RPC exists"), UnequipGear))
    {
        const FEnumProperty* Slot = CastField<FEnumProperty>(UnequipGear->FindPropertyByName(TEXT("Slot")));
        TestEqual(TEXT("Unequip sends one bounded slot identity"), int32(UnequipGear->NumParms), 1);
        TestTrue(TEXT("Unequip intent uses the authored gear-slot enum"),
            Slot && Slot->GetEnum() == StaticEnum<EKalmalaGearSlot>());
    }
    UClass* InventoryClass = UKalmalaInventoryComponent::StaticClass();
    InventoryClass->SetUpRuntimeReplicationData();
    TArray<FLifetimeProperty> Lifetime;
    GetDefault<UKalmalaInventoryComponent>()->GetLifetimeReplicatedProps(Lifetime);
    for (const FName Name : { FName(TEXT("Stacks")), FName(TEXT("EquippedGear")),
        FName(TEXT("GridSlots")), FName(TEXT("GridInstanceGuids")),
        FName(TEXT("ActiveItem")), FName(TEXT("ActiveInstanceGuid")) })
    {
        const FProperty* Property = FindFProperty<FProperty>(InventoryClass, Name);
        if (!TestNotNull(*FString::Printf(TEXT("Owner inventory field exists: %s"), *Name.ToString()), Property)) continue;
        TestTrue(*FString::Printf(TEXT("Inventory field replicates: %s"), *Name.ToString()),
            Property->HasAnyPropertyFlags(CPF_Net));
        TestTrue(*FString::Printf(TEXT("Inventory field is owner-only: %s"), *Name.ToString()),
            Lifetime.ContainsByPredicate([Property](const FLifetimeProperty& Entry)
                { return Entry.RepIndex == Property->RepIndex && Entry.Condition == COND_OwnerOnly; }));
        TestFalse(*FString::Printf(TEXT("Inventory location is not a save field: %s"), *Name.ToString()),
            Property->HasAnyPropertyFlags(CPF_SaveGame));
    }
    auto* Inventory = NewObject<UKalmalaInventoryComponent>();
    TestTrue(TEXT("New inventory is empty"), Inventory->GetStacks().IsEmpty());
    TestFalse(TEXT("An ownerless component cannot mint items"), Inventory->TryGrantFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("An ownerless component cannot consume items"), Inventory->TryConsumeFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("An ownerless component cannot move slots"), Inventory->MoveSlotFromServer(0, 1, TEXT("Wood"), NAME_None));
    FString Reason;
    TestFalse(TEXT("An ownerless component cannot equip Main Hand"),
        Inventory->SetMainHandFromServer(FGuid::NewGuid(), Reason));
    TestFalse(TEXT("An ownerless component cannot return Main Hand"),
        Inventory->SetMainHandFromServer(FGuid(), Reason));
    return true;
}
#endif
