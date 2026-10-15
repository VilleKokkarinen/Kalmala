#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaInventoryComponent.h"
#include "Misc/AutomationTest.h"
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
                    || Function->GetFName() == TEXT("ServerEquipGear")
                    || Function->GetFName() == TEXT("ServerUnequipGear"));
            TestTrue(*FString::Printf(TEXT("Only validated inventory intents are client-callable: %s"), *Function->GetName()),
                !Function->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer) || bLayoutIntent);
        }
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
    const FProperty* EquippedGearProperty = FindFProperty<FProperty>(
        UKalmalaInventoryComponent::StaticClass(), TEXT("EquippedGear"));
    TestTrue(TEXT("Equipped gear is a replicated inventory field"),
        EquippedGearProperty && EquippedGearProperty->HasAnyPropertyFlags(CPF_Net));
    auto* Inventory = NewObject<UKalmalaInventoryComponent>();
    TestTrue(TEXT("New inventory is empty"), Inventory->GetStacks().IsEmpty());
    TestFalse(TEXT("An ownerless component cannot mint items"), Inventory->TryGrantFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("An ownerless component cannot consume items"), Inventory->TryConsumeFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("An ownerless component cannot move slots"), Inventory->MoveSlotFromServer(0, 1, TEXT("Wood"), NAME_None));
    return true;
}
#endif
