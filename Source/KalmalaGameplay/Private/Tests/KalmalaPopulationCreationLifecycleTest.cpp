#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationCreationTest,
    "Kalmala.Gameplay.Population.CreationLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationCreationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population creation fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population creation GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig.WorldSeed = 418;

    UKalmalaWorldPopulationSaveGame* Save = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Population save fixture created"), Save)) return false;
    Save->InitializeForWorld(Mode->WorldGenerationConfig);
    Mode->PopulationSaveGame = Save;

    FKalmalaWorldPopulationSpawn Descriptor;
    Descriptor.Kind = EKalmalaWorldPopulationKind::Wildlife;
    Descriptor.SpatialKey = FIntPoint(-1, 2);
    Descriptor.SpawnSeed = 987654321;
    Descriptor.Location = FVector(-6400.0, 12800.0, 0.0);
    const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptor);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), Descriptor.Location,
        FRotator::ZeroRotator, SpawnParameters);
    if (!TestNotNull(TEXT("Living wildlife fixture created"), Creature)) return false;
    Creature->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::Rat);
    Mode->RegisterLivePopulationCreature(Creature);
    TestTrue(TEXT("An initialized stable spawn ID is present in the live registry"),
        Mode->HasLivePopulationCreatureWithSpawnId(PersistentSpawnId));

    FKalmalaPopulationSpawnAttempt RejectedAttempt;
    RejectedAttempt.Descriptor = Descriptor;
    int32 WildlifeCountBeforeRejectedAttempt = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
    {
        ++WildlifeCountBeforeRejectedAttempt;
    }
    TestFalse(TEXT("A descriptor without all nine passing gate results cannot create an actor"),
        Mode->TryCreatePopulationSpawnTransaction(RejectedAttempt, 10.0));
    int32 WildlifeCountAfterRejectedAttempt = 0;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(World); It; ++It)
    {
        ++WildlifeCountAfterRejectedAttempt;
    }
    TestEqual(TEXT("Rejected attempts leave the actor population unchanged"),
        WildlifeCountAfterRejectedAttempt, WildlifeCountBeforeRejectedAttempt);

    FActorSpawnParameters PlayerSpawnParameters;
    PlayerSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>(
        Descriptor.Location + FVector(FKalmalaMonsterSpawnTuning::CountRadiusCm, 0.0, 0.0),
        FRotator::ZeroRotator, PlayerSpawnParameters);
    if (!TestNotNull(TEXT("Living player fixture created"), Player)) return false;
    const TArray<AKalmalaCharacter*> LivingPlayers = { Player };

    Mode->UpdatePopulationCreatureUnloads(10.0, LivingPlayers);
    TestTrue(TEXT("A roaming creature at exactly 96 m remains loaded"), IsValid(Creature));
    TestFalse(TEXT("The exact 96 m boundary does not start an unload cooldown"),
        Mode->PopulationCreatureReactivationDueTimes.Contains(PersistentSpawnId));

    Player->SetActorLocation(Player->GetActorLocation() + FVector(1.0, 0.0, 0.0));
    Mode->UpdatePopulationCreatureUnloads(11.0, LivingPlayers);
    TestFalse(TEXT("A creature outside every living player's 96 m radius unloads"), IsValid(Creature));
    TestFalse(TEXT("Unloaded wildlife is removed from the live registry"),
        Mode->HasLivePopulationCreatureWithSpawnId(PersistentSpawnId));
    TestEqual(TEXT("Unload preserves the stable slot through its 30-second cooldown"),
        Mode->PopulationCreatureReactivationDueTimes.FindRef(PersistentSpawnId), 41.0);
    TestFalse(TEXT("Unload does not mark the persistent slot defeated"),
        Save->IsDefeated(PersistentSpawnId));
    TestFalse(TEXT("Reactivation is blocked immediately before cooldown expiry"),
        Mode->IsPopulationSpawnReactivationReady(PersistentSpawnId, 40.999));
    TestTrue(TEXT("Reactivation is allowed at the inclusive cooldown expiry"),
        Mode->IsPopulationSpawnReactivationReady(PersistentSpawnId, 41.0));

    FActorSpawnParameters ReloadParameters;
    ReloadParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* ReloadedCreature = World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), Descriptor.Location,
        FRotator::ZeroRotator, ReloadParameters);
    if (!TestNotNull(TEXT("The cooled-down descriptor can be reloaded"), ReloadedCreature)) return false;
    ReloadedCreature->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::Rat);
    Mode->RegisterLivePopulationCreature(ReloadedCreature);
    TestEqual(TEXT("Reload preserves the descriptor's stable spawn identity"),
        ReloadedCreature->GetPersistentSpawnId(), PersistentSpawnId);

    Player->SetActorLocation(Descriptor.Location + FVector(
        FKalmalaMonsterSpawnTuning::CountRadiusCm, 0.0, 0.0));
    Mode->UpdatePopulationCreatureUnloads(41.0, LivingPlayers);
    TestTrue(TEXT("The reloaded living actor remains tracked at the inclusive player radius"),
        IsValid(ReloadedCreature)
        && Mode->HasLivePopulationCreatureWithSpawnId(PersistentSpawnId));
    TestFalse(TEXT("Cooldown state clears after the same slot is reactivated"),
        Mode->PopulationCreatureReactivationDueTimes.Contains(PersistentSpawnId));
    TestFalse(TEXT("Reload does not turn an intentional unload into a defeat"),
        Save->IsDefeated(PersistentSpawnId));

    return true;
}

#endif
