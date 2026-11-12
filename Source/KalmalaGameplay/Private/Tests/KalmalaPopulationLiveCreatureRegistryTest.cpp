#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationLiveCreatureRegistryTest,
    "Kalmala.Gameplay.Population.LiveCreatureRegistry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationLiveCreatureRegistryTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Live population fixture world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Live population fixture installs its authoritative GameMode"),
        World->SetGameMode(FURL()))) return false;
    World->InitializeActorsForPlay(FURL());
    AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Live population fixture authoritative GameMode is available"), Mode)) return false;

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

    Destroyed->DispatchBeginPlay();
    const TWeakObjectPtr<AKalmalaWildlifeSpawn> DestroyedWeak = Destroyed;
    Destroyed->Destroy();
    TestFalse(TEXT("EndPlay removes destroyed actors from the registry"),
        Mode->LivePopulationCreatures.Contains(DestroyedWeak));
    TestEqual(TEXT("Destroyed actors do not remain in the live count"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, 5000.0f), 1);

    TestFalse(TEXT("Destroyed actor handle is invalid before the lazy-prune fixture"),
        DestroyedWeak.IsValid());
    TestTrue(TEXT("Defeated actor remains available for a stale weak-entry fixture"),
        IsValid(Moving) && Moving->IsDefeated());
    const TWeakObjectPtr<AKalmalaWildlifeSpawn> DefeatedWeak = Moving;
    // Simulate missed lifecycle cleanup by restoring stale handles directly,
    // bypassing RegisterLivePopulationCreature's eligibility checks.
    Mode->LivePopulationCreatures.Add(DestroyedWeak);
    Mode->LivePopulationCreatures.Add(DefeatedWeak);
    TestEqual(TEXT("The registry contains the healthy actor and two injected stale entries"),
        Mode->LivePopulationCreatures.Num(), 3);
    TestTrue(TEXT("The destroyed weak handle is present before the count query"),
        Mode->LivePopulationCreatures.Contains(DestroyedWeak));
    TestTrue(TEXT("The defeated weak handle is present before the count query"),
        Mode->LivePopulationCreatures.Contains(DefeatedWeak));
    TestEqual(TEXT("A count query excludes the invalid and defeated entries"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, 1500.0f), 1);
    TestEqual(TEXT("The count query lazily prunes both stale weak entries"),
        Mode->LivePopulationCreatures.Num(), 1);
    TestFalse(TEXT("Lazy pruning removes the invalid destroyed handle"),
        Mode->LivePopulationCreatures.Contains(DestroyedWeak));
    TestFalse(TEXT("Lazy pruning removes the defeated actor handle"),
        Mode->LivePopulationCreatures.Contains(DefeatedWeak));
    TestTrue(TEXT("The healthy actor remains registered after lazy pruning"),
        Mode->LivePopulationCreatures.Contains(TWeakObjectPtr<AKalmalaWildlifeSpawn>(First)));
    TestEqual(TEXT("The healthy actor retains its current-position count semantics"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, 1500.0f), 1);

    TestEqual(TEXT("Invalid query radii return no live count"),
        Mode->CountLivePopulationCreaturesWithinRadius(FVector::ZeroVector, -1.0f), 0);
    return true;
}

#endif
