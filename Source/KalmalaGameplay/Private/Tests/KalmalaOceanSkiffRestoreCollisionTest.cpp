#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanTravelPersistenceContract.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

namespace
{
    bool FindNavigableOceanLocation(const FKalmalaWorldGenerationConfig& Config, FVector& OutLocation)
    {
        constexpr int32 SearchStepCm = 10000;
        constexpr int32 SearchRadiusCm = 1400000;
        constexpr int32 DirectionCount = 48;
        for (int32 Radius = 0; Radius <= SearchRadiusCm; Radius += SearchStepCm)
        {
            for (int32 DirectionIndex = 0; DirectionIndex < DirectionCount; ++DirectionIndex)
            {
                const float Angle = DirectionIndex * (2.0f * PI / DirectionCount);
                const FVector2D Candidate(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle));
                const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, Candidate);
                if (Ocean.bIsValid && Ocean.WaterDepth >= 100.0f
                    && AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(Config, Candidate, 0.0f))
                {
                    OutLocation = FVector(Candidate.X, Candidate.Y, 0.0);
                    return true;
                }
            }
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanSkiffRestoreCollisionTest,
    "Kalmala.Gameplay.OceanTravel.SkiffRestoreCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanSkiffRestoreCollisionTest::RunTest(const FString& Parameters)
{
    const FGuid Guid = FGuid::NewGuid();
    const uint64 GeneratedWorldSeed = (static_cast<uint64>(Guid.A) << 32u) | Guid.B;
    const uint64 WorldSeed = GeneratedWorldSeed == 0 ? 1 : GeneratedWorldSeed;
    const FKalmalaWorldGenerationConfig Config{WorldSeed};
    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForWorld(WorldSeed);
    const FString Slot = FString::Printf(TEXT("KalmalaOceanTravelWorld_%llu_%d"),
        Identity.WorldSeed, Identity.GeneratorRevision);
    bool bOwnsSlot = false;
    UWorld* BlockedWorld = nullptr;
    UWorld* RestartWorld = nullptr;
    ON_SCOPE_EXIT
    {
        if (BlockedWorld != nullptr)
        {
            if (BlockedWorld->HasBegunPlay()) BlockedWorld->EndPlay(EEndPlayReason::Destroyed);
            BlockedWorld->DestroyWorld(false);
            if (GEngine != nullptr) GEngine->DestroyWorldContext(BlockedWorld);
        }
        if (RestartWorld != nullptr)
        {
            if (RestartWorld->HasBegunPlay()) RestartWorld->EndPlay(EEndPlayReason::Destroyed);
            RestartWorld->DestroyWorld(false);
            if (GEngine != nullptr) GEngine->DestroyWorldContext(RestartWorld);
        }
        if (bOwnsSlot) UGameplayStatics::DeleteGameInSlot(Slot, 0);
    };

    if (!TestTrue(TEXT("The generated world slot is unused"), !UGameplayStatics::DoesSaveGameExist(Slot, 0))) return false;
    const FString SlotPath = FPaths::ConvertRelativePathToFull(
        FPaths::ProjectSavedDir() / TEXT("SaveGames") / (Slot + TEXT(".sav")));
    if (!TestTrue(TEXT("The isolated slot path stays below the Windows path limit"), SlotPath.Len() < 260)) return false;

    FVector SafeLocation;
    if (!TestTrue(TEXT("The isolated seed has a navigable ocean location"),
        FindNavigableOceanLocation(Config, SafeLocation))) return false;

    UKalmalaOceanTravelPersistenceSaveGame* WorldSave = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
    if (!TestNotNull(TEXT("World travel save created"), WorldSave)) return false;
    WorldSave->Initialize(Identity);
    FKalmalaOceanTravelVesselState Vessel;
    Vessel.VesselId = TEXT("ocean-skiff:primary");
    Vessel.SafeLocation = SafeLocation;
    Vessel.YawDegrees = 0.0f;
    if (!TestTrue(TEXT("The original vessel snapshot is valid"), WorldSave->SetVesselState(Vessel))) return false;

    bOwnsSlot = true;
    if (!TestTrue(TEXT("The original vessel snapshot is written to its isolated world slot"),
        UGameplayStatics::SaveGameToSlot(WorldSave, Slot, 0))) return false;
    TArray<uint8> OriginalSlotBytes;
    if (!TestTrue(TEXT("The original slot bytes are readable"),
        FFileHelper::LoadFileToArray(OriginalSlotBytes, *SlotPath))) return false;

    BlockedWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Blocked startup world created"), BlockedWorld)) return false;
    if (GEngine != nullptr)
    {
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(BlockedWorld);
        BlockedWorld->SetGameInstance(NewObject<UGameInstance>(GEngine));
    }
    BlockedWorld->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Blocked startup uses the authoritative GameMode"), BlockedWorld->SetGameMode(FURL()))) return false;
    AKalmalaGameMode* BlockedMode = BlockedWorld->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Blocked startup GameMode created"), BlockedMode)) return false;
    BlockedMode->WorldGenerationConfig = Config;

    FActorSpawnParameters BlockerParameters;
    BlockerParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Blocker = BlockedWorld->SpawnActor<AActor>(SafeLocation, FRotator::ZeroRotator, BlockerParameters);
    if (!TestNotNull(TEXT("Saved-location blocker created"), Blocker)) return false;
    UBoxComponent* BlockerCollision = NewObject<UBoxComponent>(Blocker, TEXT("SavedVesselBlocker"));
    if (!TestNotNull(TEXT("Saved-location blocker collision created"), BlockerCollision)) return false;
    Blocker->SetRootComponent(BlockerCollision);
    BlockerCollision->SetBoxExtent(FVector(600.0f, 600.0f, 300.0f));
    BlockerCollision->SetCollisionProfileName(TEXT("BlockAll"));
    BlockerCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BlockerCollision->RegisterComponent();

    BlockedMode->LoadOceanTravelWorldSave();
    if (!TestNotNull(TEXT("Production loader accepts the saved vessel"), BlockedMode->OceanTravelWorldSave.Get())) return false;
    BlockedMode->RestorePersistedOceanSkiff();
    TestFalse(TEXT("A collision-blocked saved vessel makes world persistence read-only"),
        BlockedMode->bOceanTravelPersistenceWritable);
    TestTrue(TEXT("A collision-blocked saved vessel reserves the primary launch slot"),
        BlockedMode->bOceanTravelSavedVesselRestoreBlocked);
    TestFalse(TEXT("A primary skiff cannot claim the reserved launch slot"),
        BlockedMode->CanLaunchPrimaryOceanSkiff());
    TestFalse(TEXT("Launch admission rejects the reserved slot"),
        AKalmalaOceanSkiff::IsLaunchAllowed(true, true, true, true, true,
            BlockedMode->CanLaunchPrimaryOceanSkiff()));
    int32 BlockedStartupSkiffCount = 0;
    for (TActorIterator<AKalmalaOceanSkiff> It(BlockedWorld); It; ++It) ++BlockedStartupSkiffCount;
    TestEqual(TEXT("Blocked startup creates no duplicate skiff"), BlockedStartupSkiffCount, 0);

    FActorSpawnParameters CandidateParameters;
    CandidateParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaOceanSkiff* Candidate = BlockedWorld->SpawnActor<AKalmalaOceanSkiff>(
        SafeLocation, FRotator::ZeroRotator, CandidateParameters);
    if (!TestNotNull(TEXT("Replacement candidate created for the blocked-write check"), Candidate)) return false;
    TestFalse(TEXT("A replacement vessel cannot be persisted over the reserved slot"),
        BlockedMode->PersistOceanTravelState(Candidate));

    TArray<uint8> BlockedSlotBytes;
    if (!TestTrue(TEXT("The world slot remains readable after blocked restore and rejected replacement"),
        FFileHelper::LoadFileToArray(BlockedSlotBytes, *SlotPath))) return false;
    TestTrue(TEXT("The accepted world slot bytes are unchanged"), OriginalSlotBytes == BlockedSlotBytes);

    Candidate->Destroy();
    Blocker->Destroy();
    BlockedWorld->DestroyWorld(false);
    if (GEngine != nullptr) GEngine->DestroyWorldContext(BlockedWorld);
    BlockedWorld = nullptr;

    RestartWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Fresh restart world created"), RestartWorld)) return false;
    if (GEngine != nullptr)
    {
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(RestartWorld);
        RestartWorld->SetGameInstance(NewObject<UGameInstance>(GEngine));
    }
    RestartWorld->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Fresh startup uses the authoritative GameMode"), RestartWorld->SetGameMode(FURL()))) return false;
    AKalmalaGameMode* RestartMode = RestartWorld->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Fresh restart GameMode created"), RestartMode)) return false;
    RestartMode->WorldGenerationConfig = Config;
    RestartMode->LoadOceanTravelWorldSave();
    RestartMode->RestorePersistedOceanSkiff();

    AKalmalaOceanSkiff* Restored = nullptr;
    int32 RestoredSkiffCount = 0;
    for (TActorIterator<AKalmalaOceanSkiff> It(RestartWorld); It; ++It)
    {
        ++RestoredSkiffCount;
        Restored = *It;
    }
    TestEqual(TEXT("Fresh startup restores exactly one skiff"), RestoredSkiffCount, 1);
    if (TestNotNull(TEXT("Fresh startup restores the original skiff"), Restored))
    {
        TestEqual(TEXT("Fresh startup retains the original vessel ID"), Restored->GetPersistentVesselId(), Vessel.VesselId);
        TestTrue(TEXT("Fresh startup retains the exact accepted safe location"),
            Restored->GetActorLocation().Equals(Vessel.SafeLocation, 0.01));
        TestTrue(TEXT("Fresh startup retains the accepted heading"),
            FMath::IsNearlyEqual(Restored->GetActorRotation().Yaw, Vessel.YawDegrees, 0.01f));
    }
    return true;
}

#endif
