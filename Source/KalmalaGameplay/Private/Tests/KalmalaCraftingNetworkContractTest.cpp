#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCraftingComponent.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCraftingNetworkContractTest,
    "Kalmala.Gameplay.Crafting.NetworkContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaCraftingNetworkContractTest::RunTest(const FString& Parameters)
{
    const UClass* CraftingClass = UKalmalaCraftingComponent::StaticClass();
    const_cast<UClass*>(CraftingClass)->SetUpRuntimeReplicationData();
    const FArrayProperty* Receipts = FindFProperty<FArrayProperty>(CraftingClass, TEXT("AcceptedCraftingActionReceipts"));
    if (TestNotNull(TEXT("Accepted action receipt queue exists"), Receipts))
    {
        TestTrue(TEXT("Accepted action receipts replicate"), Receipts->HasAnyPropertyFlags(CPF_Net));
        TestFalse(TEXT("Accepted action receipts are not included in SaveGame data"), Receipts->HasAnyPropertyFlags(CPF_SaveGame));

        TArray<FLifetimeProperty> Lifetime;
        GetDefault<UKalmalaCraftingComponent>()->GetLifetimeReplicatedProps(Lifetime);
        TestTrue(TEXT("Accepted action receipts replicate only to the owner"), Lifetime.ContainsByPredicate(
            [Receipts](const FLifetimeProperty& Entry)
            {
                return Entry.RepIndex == Receipts->RepIndex && Entry.Condition == COND_OwnerOnly;
            }));
    }

    const UFunction* Craft = CraftingClass->FindFunctionByName(TEXT("ServerCraft"));
    if (TestNotNull(TEXT("Craft intent exists"), Craft))
    {
        TestTrue(TEXT("Craft intent is an owning-client server RPC"), Craft->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Craft intent has only recipe and batch parameters"), int32(Craft->NumParms), 2);
        TestNotNull(TEXT("Craft intent supplies a recipe identity"), Craft->FindPropertyByName(TEXT("RecipeId")));
        TestNotNull(TEXT("Craft intent supplies a bounded batch request"), Craft->FindPropertyByName(TEXT("Batch")));
    }

    const UFunction* ConsumeFood = CraftingClass->FindFunctionByName(TEXT("ServerConsumeFood"));
    if (TestNotNull(TEXT("Food consumption intent exists"), ConsumeFood))
    {
        TestTrue(TEXT("Food consumption is a reliable owning-client server RPC"),
            ConsumeFood->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
        TestEqual(TEXT("Food consumption accepts only one item identity"), int32(ConsumeFood->NumParms), 1);
        TestNotNull(TEXT("Food request carries the allowlisted item identity"), ConsumeFood->FindPropertyByName(TEXT("FoodItemId")));
    }

    for (const FName Intent : {FName(TEXT("ServerPlaceCampfire")), FName(TEXT("ServerRefuel")), FName(TEXT("ServerLight"))})
    {
        const UFunction* Function = CraftingClass->FindFunctionByName(Intent);
        if (!TestNotNull(TEXT("No-payload campfire intent exists"), Function)) continue;
        TestTrue(TEXT("Campfire intent is an owning-client server RPC"), Function->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Campfire intent cannot provide a target or authoritative state"), int32(Function->NumParms), 0);
    }
    const UFunction* Repair = CraftingClass->FindFunctionByName(TEXT("ServerRepairTool"));
    if (TestNotNull(TEXT("Repair intent exists"), Repair))
    {
        TestTrue(TEXT("Repair is an owning-client server RPC"), Repair->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Repair intent accepts only the allowlisted tool identity"), int32(Repair->NumParms), 1);
        TestNotNull(TEXT("Repair intent cannot provide condition, materials, or result"), Repair->FindPropertyByName(TEXT("ToolId")));
    }

    const UFunction* Construction = CraftingClass->FindFunctionByName(TEXT("ServerPlaceConstruction"));
    if (TestNotNull(TEXT("Construction placement intent exists"), Construction))
    {
        TestTrue(TEXT("Construction placement is an owning-client server RPC"), Construction->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Construction placement accepts only a buildable identity"), int32(Construction->NumParms), 1);
        TestNotNull(TEXT("Construction placement cannot provide a transform or state"), Construction->FindPropertyByName(TEXT("BuildableId")));
    }
    return true;
}
#endif
