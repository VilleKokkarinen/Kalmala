#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaCarriedToolInventoryTest,
    "Kalmala.Gameplay.Tools.CarriedToolInventoryContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaCarriedToolInventoryTest::RunTest(const FString& Parameters)
{
    const TArray<FKalmalaToolState> InitialTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
    TestEqual(TEXT("Starting inventory is bounded to the three first-wave tools"), InitialTools.Num(), 3);
    TestTrue(TEXT("Starting tool records stay within the carried-tool cap"),
        InitialTools.Num() <= FKalmalaToolLifecycleContract::MaxCarriedToolRecords);

    for (const FKalmalaToolState& State : InitialTools)
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(State.ToolId);
        TestNotNull(TEXT("Starting carried tool has an authored definition"), Definition);
        TestEqual(TEXT("Tool level starts at its server-authored baseline"), State.ToolLevel, 1);
        if (Definition != nullptr)
            TestEqual(TEXT("Starting carried tool has full condition"), State.Durability, Definition->MaxDurability);
    }

    UClass* CharacterClass = AKalmalaCharacter::StaticClass();
    CharacterClass->SetUpRuntimeReplicationData();
    const FArrayProperty* InventoryProperty = FindFProperty<FArrayProperty>(CharacterClass, TEXT("CarriedTools"));
    if (TestNotNull(TEXT("Character exposes the carried-tool record array"), InventoryProperty))
    {
        TestTrue(TEXT("Carried-tool state is replicated"), InventoryProperty->HasAnyPropertyFlags(CPF_Net));
        TestFalse(TEXT("Carried-tool progression remains transient"), InventoryProperty->HasAnyPropertyFlags(CPF_SaveGame));

        TArray<FLifetimeProperty> Lifetime;
        GetDefault<AKalmalaCharacter>()->GetLifetimeReplicatedProps(Lifetime);
        TestTrue(TEXT("Detailed carried-tool records replicate only to their owner"), Lifetime.ContainsByPredicate(
            [InventoryProperty](const FLifetimeProperty& Entry)
            {
                return Entry.RepIndex == InventoryProperty->RepIndex && Entry.Condition == COND_OwnerOnly;
            }));
    }

    TestNotNull(TEXT("Tool progression is represented by its own record field"),
        FKalmalaToolState::StaticStruct()->FindPropertyByName(TEXT("ToolLevel")));
    TestNull(TEXT("Tool level is not a player's skill level field"),
        FKalmalaToolState::StaticStruct()->FindPropertyByName(TEXT("SkillLevel")));

    return true;
}

#endif
