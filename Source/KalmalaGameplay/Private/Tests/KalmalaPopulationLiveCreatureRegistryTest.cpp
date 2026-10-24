#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationLiveCreatureRegistryTest,
    "Kalmala.Gameplay.Population.LiveCreatureRegistry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationLiveCreatureRegistryTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Live population fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Live population fixture GameMode created"), Mode)) return false;

    const auto SpawnCreature = [World, Mode](const uint64 SpawnSeed, const FVector& Location)
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
            AKalmalaWildlifeSpawn::StaticClass(), Location, FRotator::ZeroRotator, Parameters);
        if (Creature == nullptr) return Creature;

        FKalmalaWorldPopulationSpawn Descriptor;
        Descriptor.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Descriptor.SpatialKey = FIntPoint(0, 0);
        Descriptor.SpawnSeed = SpawnSeed;
        Descriptor.Location = Location;
        Descriptor.ContentId = FName(TEXT("meadow-grazer"));
        Creature->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::Boar);
        Mode->RegisterLivePopulationCreature(Creature);
        return Creature;
    };

    AKalmalaWildlifeSpawn* First = SpawnCreature(1001, FVector::ZeroVector);
    AKalmalaWildlifeSpawn* Moving = SpawnCreature(1002, FVector(1500.0, 0.0, 0.0));
    AKalmalaWildlifeSpawn* Destroyed = SpawnCreature(1003, FVector(4000.0, 0.0, 0.0));
    if (!TestTrue(TEXT("All authoritative wildlife fixtures were created"),
        IsValid(First) && IsValid(Moving) && IsValid(Destroyed)))
    {
        return false;
    }

    Mode->RegisterLivePopulationCreature(First);
    TestEqual(TEXT("Registering an actor twice keeps one registry entry"),
        Mode->LivePopulationCreatures.Num(), 3);
    TestEqual(TEXT("Current positions inside the inclusive 15 m radius are counted"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, 1500.0f), 2);

    Moving->SetActorLocation(FVector(2500.0, 0.0, 0.0));
    TestEqual(TEXT("The count follows current actor position after roaming"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, 1500.0f), 1);

    Moving->SetActorLocation(FVector(1000.0, 0.0, 0.0));
    TestTrue(TEXT("A valid server defeat succeeds for the Boar fixture"), Moving->DefeatServer());
    TestFalse(TEXT("Defeated actors are immediately unregistered"),
        Mode->LivePopulationCreatures.Contains(TWeakObjectPtr<AKalmalaWildlifeSpawn>(Moving)));
    TestEqual(TEXT("Defeated actors no longer contribute to live counts"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, 1500.0f), 1);

    const TWeakObjectPtr<AKalmalaWildlifeSpawn> DestroyedWeak = Destroyed;
    Destroyed->Destroy();
    TestFalse(TEXT("EndPlay removes destroyed actors from the registry"),
        Mode->LivePopulationCreatures.Contains(DestroyedWeak));
    TestEqual(TEXT("Destroyed actors do not remain in the live count"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, 5000.0f), 1);

    TestEqual(TEXT("Invalid query radii return no live count"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, -1.0f), 0);
    return true;
}

#endif
