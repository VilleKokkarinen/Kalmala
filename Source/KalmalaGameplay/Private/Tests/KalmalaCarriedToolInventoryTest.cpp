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
    TestEqual(TEXT("Starting inventory contains three gathering tools and the construction hammer"), InitialTools.Num(), 4);
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
    TestTrue(TEXT("The starter construction hammer is carried at level one"), InitialTools.ContainsByPredicate(
        [](const FKalmalaToolState& State) { return State.ToolId == TEXT("ConstructionHammer") && State.ToolLevel == 1; }));
    TestEqual(TEXT("The hammer has no use durability cost"),
        FKalmalaToolLifecycleContract::GetConstructionHammerDefinition().DurabilityCost, 0);

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
