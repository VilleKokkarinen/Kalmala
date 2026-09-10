#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCraftingComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCraftingNetworkContractTest,
    "Kalmala.Gameplay.Crafting.NetworkContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaCraftingNetworkContractTest::RunTest(const FString& Parameters)
{
    const UClass* CraftingClass = UKalmalaCraftingComponent::StaticClass();
    const UFunction* Craft = CraftingClass->FindFunctionByName(TEXT("ServerCraft"));
    if (TestNotNull(TEXT("Craft intent exists"), Craft))
    {
        TestTrue(TEXT("Craft intent is an owning-client server RPC"), Craft->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Craft intent has only recipe and batch parameters"), int32(Craft->NumParms), 2);
        TestNotNull(TEXT("Craft intent supplies a recipe identity"), Craft->FindPropertyByName(TEXT("RecipeId")));
        TestNotNull(TEXT("Craft intent supplies a bounded batch request"), Craft->FindPropertyByName(TEXT("Batch")));
    }

    for (const FName Intent : {FName(TEXT("ServerPlaceCampfire")), FName(TEXT("ServerRefuel")), FName(TEXT("ServerLight"))})
    {
        const UFunction* Function = CraftingClass->FindFunctionByName(Intent);
        if (!TestNotNull(TEXT("No-payload campfire intent exists"), Function)) continue;
        TestTrue(TEXT("Campfire intent is an owning-client server RPC"), Function->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Campfire intent cannot provide a target or authoritative state"), int32(Function->NumParms), 0);
    }
    const UFunction* Construction = CraftingClass->FindFunctionByName(TEXT("ServerPlaceConstruction"));
    if (TestNotNull(TEXT("Construction placement intent exists"), Construction))
    {
        TestTrue(TEXT("Construction placement is an owning-client server RPC"), Construction->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Construction placement accepts only a kit identity"), int32(Construction->NumParms), 1);
        TestNotNull(TEXT("Construction placement cannot provide a transform or state"), Construction->FindPropertyByName(TEXT("KitId")));
    }
    return true;
}
#endif
