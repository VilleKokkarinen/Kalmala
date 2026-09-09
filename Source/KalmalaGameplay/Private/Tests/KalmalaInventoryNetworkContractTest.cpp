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
        TestEqual(TEXT("Client cannot supply a target, item ID, quantity or outcome"), int32(Intent->NumParms), 0);
    }
    for (UClass* Class : { UKalmalaInventoryComponent::StaticClass(), AKalmalaHarvestNode::StaticClass() })
    {
        for (TFieldIterator<UFunction> Function(Class, EFieldIteratorFlags::ExcludeSuper); Function; ++Function)
        {
            TestFalse(*FString::Printf(TEXT("No client-callable mutation RPC: %s"), *Function->GetName()),
                Function->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        }
    }
    auto* Inventory = NewObject<UKalmalaInventoryComponent>();
    TestTrue(TEXT("New inventory is empty"), Inventory->GetStacks().IsEmpty());
    TestFalse(TEXT("An ownerless component cannot mint items"), Inventory->TryGrantFromServer(TEXT("Wood"), 1));
    TestFalse(TEXT("An ownerless component cannot consume items"), Inventory->TryConsumeFromServer(TEXT("Wood"), 1));
    return true;
}
#endif
