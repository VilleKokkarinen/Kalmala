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
        const FProperty* ToolParameter = Intent->FindPropertyByName(TEXT("ClientToolId"));
        const FProperty* ActionParameter = Intent->FindPropertyByName(TEXT("ClientAction"));
        TestTrue(TEXT("Tool intent is an item-independent name"), CastField<FNameProperty>(ToolParameter) != nullptr);
        TestTrue(TEXT("Action intent is a bounded byte enum value"), CastField<FByteProperty>(ActionParameter) != nullptr);
        for (TFieldIterator<FProperty> Property(Intent); Property; ++Property)
        {
            const FString Name = Property->GetName();
            TestTrue(*FString::Printf(TEXT("Intent excludes targets, rewards, quantities, and outcomes (%s)"), *Name),
                Name == TEXT("ClientToolId") || Name == TEXT("ClientAction") || !Property->HasAnyPropertyFlags(CPF_Parm));
        }
    }
    for (UClass* Class : { UKalmalaInventoryComponent::StaticClass(), AKalmalaHarvestNode::StaticClass() })
    {
        for (TFieldIterator<UFunction> Function(Class, EFieldIteratorFlags::ExcludeSuper); Function; ++Function)
        {
            const bool bLayoutIntent = Class == UKalmalaInventoryComponent::StaticClass()
                && (Function->GetFName() == TEXT("ServerMoveSlot") || Function->GetFName() == TEXT("ServerUseHotbarSlot"));
            TestTrue(*FString::Printf(TEXT("Only validated grid/hotbar intents are client-callable: %s"), *Function->GetName()),
                !Function->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer) || bLayoutIntent);
        }
    }
    auto* Inventory = NewObject<UKalmalaInventoryComponent>();
    TestTrue(TEXT("New inventory is empty"), Inventory->GetStacks().IsEmpty());
    TestFalse(TEXT("An ownerless component cannot mint items"), Inventory->TryGrantFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("An ownerless component cannot consume items"), Inventory->TryConsumeFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("An ownerless component cannot move slots"), Inventory->MoveSlotFromServer(0, 1, TEXT("Wood"), NAME_None));
    return true;
}
#endif
