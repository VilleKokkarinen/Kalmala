#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRainAuthorityTest,
    "Kalmala.Gameplay.Hearth.AuthorityContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaRainAuthorityTest::RunTest(const FString& Parameters)
{
    // These state owners expose no client-to-server state mutation endpoint.
    for (const UClass* Class : {AKalmalaCampfire::StaticClass(), AKalmalaConstructionActor::StaticClass(), UKalmalaPlayerStatusComponent::StaticClass()})
    {
        for (TFieldIterator<UFunction> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
        {
            TestFalse(*FString::Printf(TEXT("%s.%s cannot accept a server RPC"), *Class->GetName(), *It->GetName()),
                It->HasAnyFunctionFlags(FUNC_NetServer));
        }
    }
    auto CheckReplicated = [&](const AActor* Actor, const TCHAR* Name)
    {
        Actor->GetClass()->SetUpRuntimeReplicationData();
        const FProperty* Property = Actor->GetClass()->FindPropertyByName(Name);
        if (!TestNotNull(Name, Property)) return;
        TestTrue(TEXT("Authoritative state has replication metadata"), Property->HasAnyPropertyFlags(CPF_Net));
        TestFalse(TEXT("Transient rain state is not a save field"), Property->HasAnyPropertyFlags(CPF_SaveGame));
        TArray<FLifetimeProperty> Lifetime;
        Actor->GetLifetimeReplicatedProps(Lifetime);
        TestTrue(TEXT("State is registered for ordinary peer replication"), Lifetime.ContainsByPredicate([&](const FLifetimeProperty& Entry) {
            return Entry.RepIndex == Property->RepIndex && Entry.Condition == COND_None;
        }));
    };
    CheckReplicated(GetDefault<AKalmalaConstructionActor>(), TEXT("Health"));
    for (const TCHAR* Name : {TEXT("HearthState"), TEXT("FuelSeconds"), TEXT("EffectiveWarmth"), TEXT("bRoofProtected"), TEXT("bWindProtected")})
        CheckReplicated(GetDefault<AKalmalaCampfire>(), Name);

    // Preserve the approved schema: only accepted identity, kit and transform persist.
    int32 RecordFields = 0;
    for (TFieldIterator<FProperty> It(FKalmalaConstructionSaveRecord::StaticStruct()); It; ++It)
    {
        ++RecordFields;
        TestTrue(TEXT("Construction persistence contains no health, rain or fire claim"),
            It->GetFName() == TEXT("ConstructionId") || It->GetFName() == TEXT("KitId") || It->GetFName() == TEXT("Transform"));
    }
    TestEqual(TEXT("Construction save contract remains three fields"), RecordFields, 3);
    TestEqual(TEXT("Construction schema remains unchanged"), UKalmalaConstructionSaveGame::CurrentSchemaVersion, 1);
    return true;
}
#endif
