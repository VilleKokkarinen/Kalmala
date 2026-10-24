#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaWildlifeSpawn.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationAreaCapsTest,
    "Kalmala.Gameplay.Population.AreaCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationAreaCapsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population cap fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population cap fixture GameMode created"), Mode)) return false;

    const FIntPoint HomeKey(0, 0);
    Mode->ActivePopulationSpatialKeys.Add(HomeKey);
    Mode->PopulationKeyWildlifeActorBudgets.Add(HomeKey, 20);

    const auto SpawnCreature = [World, Mode](const uint64 Seed,
        const EKalmalaWildlifeArchetype Archetype, const FVector& Location, const FIntPoint& SpawnKey)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
            AKalmalaWildlifeSpawn::StaticClass(), Location, FRotator::ZeroRotator, SpawnParameters);
        if (Creature == nullptr) return Creature;

        FKalmalaWorldPopulationSpawn Descriptor;
        Descriptor.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Descriptor.SpatialKey = SpawnKey;
        Descriptor.SpawnSeed = Seed;
        Descriptor.Location = Location;
        Creature->InitializeServer(Descriptor, Archetype);
        Mode->RegisterLivePopulationCreature(Creature);
        return Creature;
    };

    const TArray<TPair<EKalmalaWildlifeArchetype, int32>> ExpectedCaps = {
        { EKalmalaWildlifeArchetype::Boar, 4 }, { EKalmalaWildlifeArchetype::Deer, 4 },
        { EKalmalaWildlifeArchetype::Rat, 4 }, { EKalmalaWildlifeArchetype::Badger, 4 },
        { EKalmalaWildlifeArchetype::Viper, 4 }, { EKalmalaWildlifeArchetype::Mireling, 4 },
        { EKalmalaWildlifeArchetype::DeathAdder, 4 }, { EKalmalaWildlifeArchetype::MosquitoSwarm, 4 },
        { EKalmalaWildlifeArchetype::Lynx, 4 }, { EKalmalaWildlifeArchetype::HornedGoat, 4 },
        { EKalmalaWildlifeArchetype::MoonBear, 1 }, { EKalmalaWildlifeArchetype::GiantSnake, 1 },
        { EKalmalaWildlifeArchetype::PeatHusk, 1 }, { EKalmalaWildlifeArchetype::PolarBear, 1 },
        { EKalmalaWildlifeArchetype::ThunderSentinel, 1 }, { EKalmalaWildlifeArchetype::ReefMaw, 1 },
        { EKalmalaWildlifeArchetype::Wolf, 6 }
    };
    for (const TPair<EKalmalaWildlifeArchetype, int32>& Expected : ExpectedCaps)
    {
        TestEqual(TEXT("Archetype uses its accepted population area cap"),
            Mode->GetPopulationSpeciesAreaCap(Expected.Key), Expected.Value);
    }

    for (int32 Index = 0; Index < 2; ++Index)
    {
        TestNotNull(TEXT("Nearby Boar fixture created"), SpawnCreature(1000 + Index,
            EKalmalaWildlifeArchetype::Boar, FVector(Index * 200.0f, 0.0f, 0.0f), HomeKey));
    }
    TestNotNull(TEXT("Out-of-radius Boar fixture created"), SpawnCreature(1003,
        EKalmalaWildlifeArchetype::Boar,
        FVector(FKalmalaMonsterSpawnTuning::CountRadiusCm + 1.0f, 0.0f, 0.0f), HomeKey));
    TestTrue(TEXT("An actor outside the 96 m neighborhood does not consume the area cap"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, HomeKey, EKalmalaWildlifeArchetype::Boar));
    TestNotNull(TEXT("Inclusive 96 m boundary Boar fixture created"), SpawnCreature(1004,
        EKalmalaWildlifeArchetype::Boar, FVector(FKalmalaMonsterSpawnTuning::CountRadiusCm, 0.0f, 0.0f), HomeKey));
    TestNotNull(TEXT("Fourth nearby Boar fixture created"), SpawnCreature(1005,
        EKalmalaWildlifeArchetype::Boar, FVector(600.0f, 0.0f, 0.0f), HomeKey));
    TestFalse(TEXT("Four nearby Boars including the exact 96 m boundary fill the common-species cap"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, HomeKey, EKalmalaWildlifeArchetype::Boar));

    for (int32 Index = 0; Index < 5; ++Index)
    {
        TestNotNull(TEXT("Wolf individual fixture created"), SpawnCreature(2000 + Index,
            EKalmalaWildlifeArchetype::Wolf, FVector(Index * 100.0f, 1000.0f, 0.0f), HomeKey));
    }
    TestTrue(TEXT("Five nearby Wolf individuals leave one slot"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, HomeKey, EKalmalaWildlifeArchetype::Wolf));
    TestNotNull(TEXT("Sixth Wolf individual fixture created"), SpawnCreature(2005,
        EKalmalaWildlifeArchetype::Wolf, FVector(500.0f, 1000.0f, 0.0f), HomeKey));
    TestFalse(TEXT("Six nearby Wolf individuals fill the Wolf cap"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, HomeKey, EKalmalaWildlifeArchetype::Wolf));

    TestNotNull(TEXT("Rare hard enemy fixture created"), SpawnCreature(3000,
        EKalmalaWildlifeArchetype::MoonBear, FVector(0.0f, 2000.0f, 0.0f), HomeKey));
    TestFalse(TEXT("One nearby rare hard enemy fills its species cap"),
        Mode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, HomeKey, EKalmalaWildlifeArchetype::MoonBear));

    UWorld* BudgetWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population budget fixture world created"), BudgetWorld)) return false;
    ON_SCOPE_EXIT { BudgetWorld->DestroyWorld(false); };
    AKalmalaGameMode* BudgetMode = BudgetWorld->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population budget fixture GameMode created"), BudgetMode)) return false;
    const FIntPoint FirstKey(0, 0);
    const FIntPoint SecondKey(1, 0);
    const FIntPoint InactiveKey(8, 8);
    BudgetMode->ActivePopulationSpatialKeys.Add(FirstKey);
    BudgetMode->ActivePopulationSpatialKeys.Add(SecondKey);
    BudgetMode->PopulationKeyWildlifeActorBudgets.Add(FirstKey, 1);
    BudgetMode->PopulationKeyWildlifeActorBudgets.Add(SecondKey, 10);
    const auto SpawnBudgetCreature = [BudgetWorld, BudgetMode](const uint64 Seed,
        const EKalmalaWildlifeArchetype Archetype, const FIntPoint& SpawnKey)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaWildlifeSpawn* Creature = BudgetWorld->SpawnActor<AKalmalaWildlifeSpawn>(
            AKalmalaWildlifeSpawn::StaticClass(), FVector(50000.0f + Seed, 50000.0f, 0.0f),
            FRotator::ZeroRotator, SpawnParameters);
        if (Creature == nullptr) return Creature;
        FKalmalaWorldPopulationSpawn Descriptor;
        Descriptor.SpatialKey = SpawnKey;
        Descriptor.SpawnSeed = Seed;
        Descriptor.Location = Creature->GetActorLocation();
        Creature->InitializeServer(Descriptor, Archetype);
        BudgetMode->RegisterLivePopulationCreature(Creature);
        return Creature;
    };

    TestNotNull(TEXT("Creature charged to first key created"),
        SpawnBudgetCreature(4000, EKalmalaWildlifeArchetype::Boar, FirstKey));
    TestFalse(TEXT("A full home-key budget blocks a different species candidate"),
        BudgetMode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, FirstKey,
            EKalmalaWildlifeArchetype::Badger));
    for (int32 Index = 0; Index < 11; ++Index)
    {
        TestNotNull(TEXT("Unbudgeted pre-existing actor fixture created"),
            SpawnBudgetCreature(4100 + Index, EKalmalaWildlifeArchetype::Deer, InactiveKey));
    }
    TestFalse(TEXT("The sum of active-key budgets blocks over-budget live actors"),
        BudgetMode->CanAdmitPopulationCreatureWithinCaps(FVector::ZeroVector, SecondKey,
            EKalmalaWildlifeArchetype::Boar));
    return true;
}

#endif
