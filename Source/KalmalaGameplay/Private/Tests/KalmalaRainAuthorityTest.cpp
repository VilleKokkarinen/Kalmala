#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRainAuthorityTest,
    "Kalmala.Gameplay.Hearth.AuthorityContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaRainAuthorityTest::RunTest(const FString& Parameters)
{
    // These state owners expose no client-to-server state mutation endpoint.
    for (const UClass* Class : {AKalmalaCampfire::StaticClass(), AKalmalaConstructionActor::StaticClass(), UKalmalaPlayerStatusComponent::StaticClass(), AKalmalaWorldGenerationGameState::StaticClass()})
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
    CheckReplicated(GetDefault<AKalmalaWorldGenerationGameState>(), TEXT("WeatherState"));
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWeatherMutationTest,
    "Kalmala.Gameplay.Hearth.WeatherMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWeatherMutationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* State = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Weather state spawned"), State)) { World->DestroyWorld(false); return false; }
    FKalmalaWeatherState Accepted;
    Accepted.WeatherCycleIndex = 7;
    Accepted.ServerStartTimeSeconds = 15.0f;
    Accepted.PrecipitationIntensity = 0.5f;
    Accepted.WindDirectionDegrees = 90;
    Accepted.WindStrength = 0.5f;
    State->SetWeatherStateFromServer(Accepted);
    auto CheckUnchanged = [&]() {
        const auto& Actual = State->GetWeatherState();
        TestEqual(TEXT("Cycle preserved"), Actual.WeatherCycleIndex, Accepted.WeatherCycleIndex);
        TestEqual(TEXT("Start preserved"), Actual.ServerStartTimeSeconds, Accepted.ServerStartTimeSeconds);
        TestEqual(TEXT("Duration preserved"), Actual.DurationSeconds, Accepted.DurationSeconds);
        TestEqual(TEXT("Rain preserved"), Actual.PrecipitationIntensity, Accepted.PrecipitationIntensity);
        TestEqual(TEXT("Wind direction preserved"), Actual.WindDirectionDegrees, Accepted.WindDirectionDegrees);
        TestEqual(TEXT("Wind strength preserved"), Actual.WindStrength, Accepted.WindStrength);
    };
    CheckUnchanged();
    State->SetRole(ROLE_SimulatedProxy);
    FKalmalaWeatherState Forged = Accepted;
    Forged.WeatherCycleIndex = 8; Forged.PrecipitationIntensity = 1.0f;
    State->SetWeatherStateFromServer(Forged);
    CheckUnchanged();
    State->SetRole(ROLE_Authority);
    for (int32 Case = 0; Case < 8; ++Case)
    {
        Forged = Accepted;
        switch (Case)
        {
        case 0: Forged.ServerStartTimeSeconds = NAN; break;
        case 1: Forged.ServerStartTimeSeconds = INFINITY; break;
        case 2: Forged.ServerStartTimeSeconds = -1.0f; break;
        case 3: Forged.PrecipitationIntensity = NAN; break;
        case 4: Forged.PrecipitationIntensity = 2.0f; break;
        case 5: Forged.DurationSeconds = 0.0f; break;
        case 6: Forged.WindDirectionDegrees = 1; break;
        case 7: Forged.WindStrength = -1.0f; break;
        }
        TestFalse(TEXT("Malformed weather fails validation"), Forged.IsValid());
        State->SetWeatherStateFromServer(Forged);
        CheckUnchanged();
    }
    Accepted.WeatherCycleIndex = 8;
    Accepted.PrecipitationIntensity = 1.0f;
    State->SetWeatherStateFromServer(Accepted);
    CheckUnchanged();
    World->DestroyWorld(false);
    return true;
}
#endif
