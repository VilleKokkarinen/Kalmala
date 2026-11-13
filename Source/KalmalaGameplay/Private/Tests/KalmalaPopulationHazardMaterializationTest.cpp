#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaBiomeClassifier.h"
#include "KalmalaBiomeExpansionContract.h"
#include "KalmalaGameMode.h"
#include "KalmalaHazardSpawn.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationHazardMaterializationTest,
    "Kalmala.Gameplay.Population.HazardMaterialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationHazardMaterializationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Hazard materialization fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Hazard materialization GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig.WorldSeed = 418;

    UKalmalaWorldPopulationSaveGame* Save = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Hazard population save fixture created"), Save)) return false;
    Save->InitializeForWorld(Mode->WorldGenerationConfig);
    Mode->PopulationSaveGame = Save;

    TMap<FIntPoint, TArray<FKalmalaWorldPopulationSpawn>> EligibleHazardKeys;
    FIntPoint FirstKey = FIntPoint::ZeroValue;
    FIntPoint SecondKey = FIntPoint::ZeroValue;
    TArray<FKalmalaWorldPopulationSpawn> FirstHazards;
    TArray<FKalmalaWorldPopulationSpawn> SecondHazards;
    bool bFoundAdjacentKeys = false;
    for (int32 X = -128; X <= 128 && !bFoundAdjacentKeys; ++X)
    {
        for (int32 Y = -128; Y <= 128 && !bFoundAdjacentKeys; ++Y)
        {
            const FIntPoint Key(X, Y);
            const FVector2D KeyCenter = (FVector2D(Key) + FVector2D(0.5f, 0.5f))
                * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
            if (!FKalmalaWorldBounds::Contains(Mode->WorldGenerationConfig, KeyCenter)) continue;

            TArray<FKalmalaWorldPopulationSpawn> Hazards =
                FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(Mode->WorldGenerationConfig,
                    Key, EKalmalaWorldPopulationKind::Hazard);
            const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                FKalmalaWorldFieldSampler::Sample(Mode->WorldGenerationConfig, KeyCenter));
            if (KeyBiome == EKalmalaBiome::ShimmeringLakes || KeyBiome == EKalmalaBiome::Elderwood
                || KeyBiome == EKalmalaBiome::MossyMire || KeyBiome == EKalmalaBiome::FreezingTundra
                || KeyBiome == EKalmalaBiome::ThunderMountains)
            {
                Hazards.SetNum(FMath::Min(Hazards.Num(), FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                    Hazards.Num(), EKalmalaWorldPopulationKind::Hazard, KeyBiome)));
            }
            if (Hazards.IsEmpty()) continue;

            if (const TArray<FKalmalaWorldPopulationSpawn>* Neighbor = EligibleHazardKeys.Find(FIntPoint(X - 1, Y)))
            {
                FirstKey = FIntPoint(X - 1, Y);
                FirstHazards = *Neighbor;
                SecondKey = Key;
                SecondHazards = MoveTemp(Hazards);
                bFoundAdjacentKeys = true;
                continue;
            }
            if (const TArray<FKalmalaWorldPopulationSpawn>* Neighbor = EligibleHazardKeys.Find(FIntPoint(X, Y - 1)))
            {
                FirstKey = FIntPoint(X, Y - 1);
                FirstHazards = *Neighbor;
                SecondKey = Key;
                SecondHazards = MoveTemp(Hazards);
                bFoundAdjacentKeys = true;
                continue;
            }
            EligibleHazardKeys.Add(Key, MoveTemp(Hazards));
        }
    }
    if (!TestTrue(TEXT("Seed 418 has adjacent keys with materializable hazard descriptors"), bFoundAdjacentKeys)) return false;

    const auto FindHazardAt = [](UWorld* InWorld, const FKalmalaWorldPopulationSpawn& Descriptor) -> AKalmalaHazardSpawn*
    {
        for (TActorIterator<AKalmalaHazardSpawn> It(InWorld); It; ++It)
        {
            if (It->GetActorLocation().Equals(Descriptor.Location, 0.1f)) return *It;
        }
        return nullptr;
    };
    const auto CountHazards = [](UWorld* InWorld)
    {
        int32 Count = 0;
        for (TActorIterator<AKalmalaHazardSpawn> It(InWorld); It; ++It) ++Count;
        return Count;
    };

    Mode->ActivatePopulationKey(FirstKey);
    Mode->ActivatePopulationKey(SecondKey);
    const int32 ExpectedHazardCount = FirstHazards.Num() + SecondHazards.Num();
    TestEqual(TEXT("Activation materializes every deterministic hazard descriptor"),
        CountHazards(World), ExpectedHazardCount);

    TArray<FKalmalaWorldPopulationSpawn> AllHazards = FirstHazards;
    AllHazards.Append(SecondHazards);
    TArray<FString> HazardIds;
    HazardIds.Reserve(AllHazards.Num());
    for (const FKalmalaWorldPopulationSpawn& Descriptor : AllHazards)
    {
        const FString Id = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptor);
        HazardIds.Add(Id);
        AKalmalaHazardSpawn* Hazard = FindHazardAt(World, Descriptor);
        TestNotNull(TEXT("Generated hazard is found at its descriptor location"), Hazard);
        TestTrue(TEXT("Generated hazard binds the production GameMode defeat callback"),
            IsValid(Hazard) && Hazard->OnDefeated.IsBoundToObject(Mode));
    }

    const FString DefeatedId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(FirstHazards[0]);
    AKalmalaHazardSpawn* DefeatedHazard = FindHazardAt(World, FirstHazards[0]);
    if (!TestNotNull(TEXT("Selected generated hazard is available for server defeat"), DefeatedHazard)) return false;

    TArray<uint8> PersistedBytes;
    int32 SaveWriterCalls = 0;
    bool bSerializedInMemory = false;
    Mode->PopulationSaveWriterForTests = [&PersistedBytes, &SaveWriterCalls, &bSerializedInMemory](
        UKalmalaWorldPopulationSaveGame* WrittenSave, const FString&, int32)
    {
        ++SaveWriterCalls;
        bSerializedInMemory = UGameplayStatics::SaveGameToMemory(WrittenSave, PersistedBytes);
    };

    TestTrue(TEXT("The server accepts the generated hazard defeat"), DefeatedHazard->DefeatServer());
    TestTrue(TEXT("Accepted defeat hides the generated hazard"), DefeatedHazard->IsHidden());
    TestFalse(TEXT("The same generated hazard cannot be defeated twice"), DefeatedHazard->DefeatServer());
    TestEqual(TEXT("Accepted defeat invokes one isolated persistence write"), SaveWriterCalls, 1);
    TestTrue(TEXT("The isolated writer serializes the live population save in memory"), bSerializedInMemory);
    TestTrue(TEXT("The production callback records the exact stable descriptor ID"), Save->IsDefeated(DefeatedId));
    for (const FString& Id : HazardIds)
    {
        TestEqual(TEXT("Only the accepted hazard ID is depleted in the sparse save"),
            Save->IsDefeated(Id), Id == DefeatedId);
    }

    UKalmalaWorldPopulationSaveGame* ReloadedSave =
        Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromMemory(PersistedBytes));
    if (!TestNotNull(TEXT("In-memory population save reloads"), ReloadedSave)) return false;
    TestTrue(TEXT("Reloaded save retains the accepted stable hazard ID"), ReloadedSave->IsDefeated(DefeatedId));
    for (const FString& Id : HazardIds)
    {
        TestEqual(TEXT("Reloaded sparse save retains no unrelated hazard depletion"),
            ReloadedSave->IsDefeated(Id), Id == DefeatedId);
    }

    UWorld* FreshWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Fresh hazard activation world created"), FreshWorld)) return false;
    ON_SCOPE_EXIT { FreshWorld->DestroyWorld(false); };
    AKalmalaGameMode* FreshMode = FreshWorld->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Fresh hazard activation GameMode created"), FreshMode)) return false;
    FreshMode->WorldGenerationConfig.WorldSeed = 418;
    FreshMode->PopulationSaveGame = ReloadedSave;
    FreshMode->ActivatePopulationKey(FirstKey);
    FreshMode->ActivatePopulationKey(SecondKey);

    TestEqual(TEXT("Fresh activation suppresses exactly the defeated hazard slot"),
        CountHazards(FreshWorld), ExpectedHazardCount - 1);
    for (const FKalmalaWorldPopulationSpawn& Descriptor : AllHazards)
    {
        const FString Id = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptor);
        AKalmalaHazardSpawn* Hazard = FindHazardAt(FreshWorld, Descriptor);
        if (Id == DefeatedId)
        {
            TestNull(TEXT("Fresh activation omits the permanently defeated hazard"), Hazard);
        }
        else
        {
            TestNotNull(TEXT("Fresh activation still materializes its undefeated neighbor"), Hazard);
            TestTrue(TEXT("Fresh neighbor has its production defeat callback bound"),
                IsValid(Hazard) && Hazard->OnDefeated.IsBoundToObject(FreshMode));
        }
    }
    return true;
}

#endif
