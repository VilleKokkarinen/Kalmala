#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationWolfReservationTest,
    "Kalmala.Gameplay.Population.WolfPackReservations",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationWolfReservationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Wolf reservation fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Wolf reservation fixture GameMode created"), Mode)) return false;

    const FIntPoint HomeKey(0, 0);
    Mode->ActivePopulationSpatialKeys.Add(HomeKey);
    Mode->PopulationKeyWildlifeActorBudgets.Add(HomeKey, 20);
    const auto SpawnWolf = [World, Mode, HomeKey](const uint64 Seed, const FVector& Location)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
            AKalmalaWildlifeSpawn::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
        if (Creature == nullptr) return Creature;
        FKalmalaWorldPopulationSpawn Descriptor;
        Descriptor.SpatialKey = HomeKey;
        Descriptor.SpawnSeed = Seed;
        Descriptor.Location = Location;
        Creature->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::Wolf);
        Mode->RegisterLivePopulationCreature(Creature);
        return Creature;
    };

    for (int32 Index = 0; Index < 6; ++Index)
    {
        TestNotNull(TEXT("Existing Wolf outside the pack area was created"),
            SpawnWolf(1000 + Index, FVector(70000.0f + Index * 100.0f, 0.0f, 0.0f)));
    }

    const TArray<FVector> FirstPack = {
        FVector::ZeroVector, FVector(100.0f, 0.0f, 0.0f), FVector(200.0f, 0.0f, 0.0f)
    };
    const TArray<FVector> SecondPack = {
        FVector(300.0f, 0.0f, 0.0f), FVector(400.0f, 0.0f, 0.0f), FVector(500.0f, 0.0f, 0.0f)
    };
    FGuid FirstReservation;
    FGuid SecondReservation;
    TestTrue(TEXT("A three-Wolf pack reserves all members together"),
        Mode->TryReserveWolfPackCapacity(FirstPack, HomeKey, FirstReservation));
    TestTrue(TEXT("Successful pack reservation returns a valid release token"), FirstReservation.IsValid());
    const TArray<FKalmalaPopulationCreatureReservation>* FirstMembers =
        Mode->PendingPopulationCreatureReservations.Find(FirstReservation);
    TestNotNull(TEXT("First pack reservation is retained"), FirstMembers);
    if (FirstMembers != nullptr)
    {
        TestEqual(TEXT("The reservation holds exactly three Wolf individuals"), FirstMembers->Num(), 3);
    }
    TestTrue(TEXT("A second overlapping pack can use the remaining Wolf area slots"),
        Mode->TryReserveWolfPackCapacity(SecondPack, HomeKey, SecondReservation));

    TArray<FGuid> ReservationIds;
    Mode->PendingPopulationCreatureReservations.GetKeys(ReservationIds);
    TestEqual(TEXT("Two complete packs reserve six Wolf individuals"), ReservationIds.Num(), 2);
    TestFalse(TEXT("Six reserved nearby Wolves fill the individual area cap"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, HomeKey, EKalmalaWildlifeArchetype::Wolf));

    const TArray<FVector> ThirdPack = {
        FVector(600.0f, 0.0f, 0.0f), FVector(700.0f, 0.0f, 0.0f), FVector(800.0f, 0.0f, 0.0f)
    };
    FGuid RejectedReservation;
    TestFalse(TEXT("A pack that cannot fit all three Wolf slots is rejected"),
        Mode->TryReserveWolfPackCapacity(ThirdPack, HomeKey, RejectedReservation));
    TestFalse(TEXT("A rejected pack has no release token"), RejectedReservation.IsValid());
    Mode->PendingPopulationCreatureReservations.GetKeys(ReservationIds);
    TestEqual(TEXT("Rejected pack admission leaves both prior packs intact"), ReservationIds.Num(), 2);

    TestTrue(TEXT("A successful reservation can be released explicitly"),
        Mode->ReleasePopulationCreatureReservation(FirstReservation));
    TestFalse(TEXT("Releasing the same pack twice has no effect"),
        Mode->ReleasePopulationCreatureReservation(FirstReservation));
    TestTrue(TEXT("Release returns the nearby Wolf capacity to the cap query"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, HomeKey, EKalmalaWildlifeArchetype::Wolf));

    const TArray<FVector> PartialFailurePack = {
        FVector(40000.0f, 0.0f, 0.0f), FVector(50000.0f, 0.0f, 0.0f), FVector(70000.0f, 0.0f, 0.0f)
    };
    FGuid RolledBackReservation;
    TestFalse(TEXT("A full member area rejects the whole prospective pack"),
        Mode->TryReserveWolfPackCapacity(PartialFailurePack, HomeKey, RolledBackReservation));
    TestFalse(TEXT("A rolled-back pack exposes no reservation token"), RolledBackReservation.IsValid());
    Mode->PendingPopulationCreatureReservations.GetKeys(ReservationIds);
    TestEqual(TEXT("Capacity held by earlier provisional members is rolled back"), ReservationIds.Num(), 1);
    TestTrue(TEXT("The failed provisional pack leaves its clear member area available"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector(40000.0f, 0.0f, 0.0f), HomeKey,
            EKalmalaWildlifeArchetype::Wolf));

    TestTrue(TEXT("The remaining complete pack is explicitly released"),
        Mode->ReleasePopulationCreatureReservation(SecondReservation));
    TestEqual(TEXT("Explicit releases remove all pending pack capacity"),
        Mode->PendingPopulationCreatureReservations.Num(), 0);

    UWorld* BudgetWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Reservation budget fixture world created"), BudgetWorld)) return false;
    ON_SCOPE_EXIT { BudgetWorld->DestroyWorld(false); };
    AKalmalaGameMode* BudgetMode = BudgetWorld->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Reservation budget fixture GameMode created"), BudgetMode)) return false;
    const FIntPoint FirstKey(0, 0);
    const FIntPoint SecondKey(1, 0);
    const FIntPoint InactiveKey(8, 8);
    BudgetMode->ActivePopulationSpatialKeys.Add(FirstKey);
    BudgetMode->ActivePopulationSpatialKeys.Add(SecondKey);
    BudgetMode->PopulationKeyWildlifeActorBudgets.Add(FirstKey, 3);
    BudgetMode->PopulationKeyWildlifeActorBudgets.Add(SecondKey, 6);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaWildlifeSpawn* Existing = BudgetWorld->SpawnActor<AKalmalaWildlifeSpawn>(
            AKalmalaWildlifeSpawn::StaticClass(), FVector(100000.0f + Index * 100.0f, 100000.0f, 0.0f),
            FRotator::ZeroRotator, SpawnParameters);
        TestNotNull(TEXT("Existing actor for aggregate budget fixture was created"), Existing);
        if (Existing != nullptr)
        {
            FKalmalaWorldPopulationSpawn Descriptor;
            Descriptor.SpatialKey = InactiveKey;
            Descriptor.SpawnSeed = 2000 + Index;
            Descriptor.Location = Existing->GetActorLocation();
            Existing->InitializeServer(Descriptor, EKalmalaWildlifeArchetype::Deer);
            BudgetMode->RegisterLivePopulationCreature(Existing);
        }
    }

    const TArray<FVector> FirstKeyPack = {
        FVector::ZeroVector, FVector(100.0f, 0.0f, 0.0f), FVector(200.0f, 0.0f, 0.0f)
    };
    const TArray<FVector> SecondKeyPack = {
        FVector(30000.0f, 0.0f, 0.0f), FVector(30100.0f, 0.0f, 0.0f), FVector(30200.0f, 0.0f, 0.0f)
    };
    FGuid FirstKeyReservation;
    FGuid SecondKeyReservation;
    TestTrue(TEXT("First key can reserve its existing three-actor budget"),
        BudgetMode->TryReserveWolfPackCapacity(FirstKeyPack, FirstKey, FirstKeyReservation));
    TestTrue(TEXT("Second key can reserve within its existing budget"),
        BudgetMode->TryReserveWolfPackCapacity(SecondKeyPack, SecondKey, SecondKeyReservation));
    TestFalse(TEXT("Pending packs count against the aggregate active-key budget"),
        BudgetMode->CanAdmitPopulationCreatureWithinCaps(FVector(30000.0f, 0.0f, 0.0f), SecondKey,
            EKalmalaWildlifeArchetype::Badger));
    TestFalse(TEXT("Pending pack members count against their home-key budget"),
        BudgetMode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, FirstKey,
            EKalmalaWildlifeArchetype::Badger));
    TestTrue(TEXT("Releasing capacity permits a later different-species candidate"),
        BudgetMode->ReleasePopulationCreatureReservation(FirstKeyReservation)
        && BudgetMode->CanAdmitPopulationCreatureWithinCaps(FVector(30000.0f, 0.0f, 0.0f), SecondKey,
            EKalmalaWildlifeArchetype::Badger));
    TestTrue(TEXT("Final reservation is released"), BudgetMode->ReleasePopulationCreatureReservation(SecondKeyReservation));
    return true;
}

#endif
