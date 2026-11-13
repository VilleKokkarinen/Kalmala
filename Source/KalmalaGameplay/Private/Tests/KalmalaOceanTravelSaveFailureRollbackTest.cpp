#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanTravelFeedbackComponent.h"
#include "KalmalaOceanTravelPersistenceContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SaveGame.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

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

    FString MakeOceanTravelWorldSlot(const FKalmalaM7SaveIdentity& Identity)
    {
        return FString::Printf(TEXT("KalmalaOceanTravelWorld_%llu_%d"),
            Identity.WorldSeed, Identity.GeneratorRevision);
    }

    FString MakeOceanTravelPlayerSlot(const FKalmalaM7SaveIdentity& Identity, const FString& PlayerIdentity)
    {
        return FString::Printf(TEXT("KalmalaOceanTravelPlayer_%llu_%d_%08x"),
            Identity.WorldSeed, Identity.GeneratorRevision, FCrc::StrCrc32(*PlayerIdentity));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanTravelSaveFailureRollbackTest,
    "Kalmala.Gameplay.OceanTravel.SaveFailureRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanTravelSaveFailureRollbackTest::RunTest(const FString& Parameters)
{
    if (!TestNotNull(TEXT("Engine is available for isolated travel worlds"), GEngine)) return false;

    const FGuid Guid = FGuid::NewGuid();
    const uint64 GeneratedWorldSeed = (static_cast<uint64>(Guid.A) << 32u) | Guid.B;
    const uint64 WorldSeed = GeneratedWorldSeed == 0 ? 1 : GeneratedWorldSeed;
    const FKalmalaWorldGenerationConfig Config{WorldSeed};
    const FKalmalaM7SaveIdentity WorldIdentity = FKalmalaM7SaveIdentity::ForWorld(WorldSeed);
    const FString WorldSlot = MakeOceanTravelWorldSlot(WorldIdentity);
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        FString::Printf(TEXT("rf26-%s"), *Guid.ToString(EGuidFormats::Digits)), FName(TEXT("KalmalaRF26Test")));
    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    const FKalmalaM7SaveIdentity PlayerIdentityScope = FKalmalaM7SaveIdentity::ForPlayer(WorldSeed, PlayerIdentity);
    const FString PlayerSlot = MakeOceanTravelPlayerSlot(PlayerIdentityScope, PlayerIdentity);
    const FString SavedDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    const FString PlayerSlotPath = FPaths::ConvertRelativePathToFull(SavedDirectory / (PlayerSlot + TEXT(".sav")));
    bool bOwnsTestSlots = false;
    TArray<UWorld*> FixtureWorlds;
    ON_SCOPE_EXIT
    {
        for (UWorld* World : FixtureWorlds)
        {
            if (World == nullptr) continue;
            if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Destroyed);
            World->DestroyWorld(false);
            if (GEngine != nullptr) GEngine->DestroyWorldContext(World);
        }
        if (bOwnsTestSlots)
        {
            UGameplayStatics::DeleteGameInSlot(WorldSlot, 0);
            UGameplayStatics::DeleteGameInSlot(PlayerSlot, 0);
        }
    };

    const FString WorldSlotPath = FPaths::ConvertRelativePathToFull(SavedDirectory / (WorldSlot + TEXT(".sav")));
    if (!TestTrue(TEXT("The isolated world travel slot is unused"), !UGameplayStatics::DoesSaveGameExist(WorldSlot, 0))
        || !TestTrue(TEXT("The isolated player travel slot is unused"), !UGameplayStatics::DoesSaveGameExist(PlayerSlot, 0))
        || !TestTrue(TEXT("The isolated world slot path is below the Windows limit"), WorldSlotPath.Len() < 260)
        || !TestTrue(TEXT("The isolated player slot path is below the Windows limit"), PlayerSlotPath.Len() < 260))
    {
        return false;
    }
    bOwnsTestSlots = true;

    FVector SafeLocation;
    if (!TestTrue(TEXT("The isolated world has a navigable ocean location"),
        FindNavigableOceanLocation(Config, SafeLocation))) return false;

    struct FTravelWorldFixture
    {
        UWorld* World = nullptr;
        AKalmalaGameMode* GameMode = nullptr;
    };
    const auto CreateTravelWorld = [this, &Config, &FixtureWorlds](const FString& Label) -> FTravelWorldFixture
    {
        FTravelWorldFixture Fixture;
        Fixture.World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!TestNotNull(Label + TEXT(" world created"), Fixture.World)) return Fixture;
        FixtureWorlds.Add(Fixture.World);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Fixture.World);
        Fixture.World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        Fixture.World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
        if (!TestTrue(Label + TEXT(" uses the authoritative GameMode"), Fixture.World->SetGameMode(FURL()))) return Fixture;
        Fixture.GameMode = Fixture.World->GetAuthGameMode<AKalmalaGameMode>();
        if (!TestNotNull(Label + TEXT(" GameMode created"), Fixture.GameMode)) return Fixture;

        AKalmalaWorldGenerationGameState* WorldState = Fixture.World->SpawnActor<AKalmalaWorldGenerationGameState>();
        if (!TestNotNull(Label + TEXT(" world identity state created"), WorldState)) return Fixture;
        Fixture.World->SetGameState(WorldState);
        if (!TestTrue(Label + TEXT(" world identity initialized"), WorldState->SelectWorldSeedFromServer(Config.WorldSeed)))
            return Fixture;
        Fixture.GameMode->WorldGenerationConfig = Config;
        Fixture.GameMode->LoadOceanTravelWorldSave();
        return Fixture;
    };

    FTravelWorldFixture FirstWorld = CreateTravelWorld(TEXT("Initial travel"));
    if (!TestNotNull(TEXT("Initial travel world is ready"), FirstWorld.World)
        || !TestNotNull(TEXT("Initial travel GameMode is ready"), FirstWorld.GameMode)) return false;

    APlayerState* PlayerState = FirstWorld.World->SpawnActor<APlayerState>();
    AKalmalaCharacter* Character = FirstWorld.World->SpawnActor<AKalmalaCharacter>(
        SafeLocation + FVector(200.0, 0.0, 100.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Authenticated launch requester state created"), PlayerState)
        || !TestNotNull(TEXT("Launch requester created"), Character)) return false;
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    Character->SetPlayerState(PlayerState);

    FActorSpawnParameters PatchParameters;
    PatchParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaGeneratedTerrainPatch* TerrainPatch = FirstWorld.World->SpawnActor<AKalmalaGeneratedTerrainPatch>(
        SafeLocation, FRotator::ZeroRotator, PatchParameters);
    if (!TestNotNull(TEXT("Generated shoreline target created"), TerrainPatch)) return false;
    TerrainPatch->Initialize(Config, FVector2D(SafeLocation));
    UBoxComponent* HitSurface = NewObject<UBoxComponent>(TerrainPatch, TEXT("RF26GeneratedHitSurface"));
    if (!TestNotNull(TEXT("Generated shoreline hit surface created"), HitSurface)) return false;
    TerrainPatch->AddInstanceComponent(HitSurface);
    HitSurface->SetupAttachment(TerrainPatch->GetRootComponent());
    HitSurface->SetBoxExtent(FVector(50.0));
    HitSurface->SetRelativeLocation(FVector(0.0, 0.0, 100.0));
    HitSurface->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HitSurface->SetCollisionObjectType(ECC_WorldStatic);
    HitSurface->RegisterComponent();
    // The hit is supplied directly below; keep generated terrain collision out of the launch spawn check.
    TerrainPatch->SetActorEnableCollision(false);

    FHitResult TerrainHit(TerrainPatch, HitSurface, SafeLocation, FVector::UpVector);
    TerrainHit.TraceStart = Character->GetActorLocation();
    TerrainHit.TraceEnd = SafeLocation;
    TerrainHit.Distance = FVector::Dist(Character->GetActorLocation(), SafeLocation);
    TerrainHit.bBlockingHit = true;

    // The failed world write must cancel the live launch and publish a save denial.
    FirstWorld.GameMode->OceanTravelSaveWriterForTests = [WorldSlot](USaveGame* SaveGame, const FString& Slot, int32 UserIndex)
    {
        return Slot != WorldSlot && UGameplayStatics::SaveGameToSlot(SaveGame, Slot, UserIndex);
    };
    TestNull(TEXT("A launch with an unavailable world save returns no skiff"),
        AKalmalaOceanSkiff::TryLaunchFromServer(Character, TerrainHit));
    TestEqual(TEXT("Failed launch reports the save denial, not success"),
        Character->GetOceanTravelFeedbackComponent()->GetFeedback(),
        EKalmalaOceanTravelFeedback::TravelActionSaveUnavailable);
    int32 SkiffCount = 0;
    for (TActorIterator<AKalmalaOceanSkiff> It(FirstWorld.World); It; ++It)
    {
        if (!It->IsActorBeingDestroyed()) ++SkiffCount;
    }
    TestEqual(TEXT("Failed launch leaves no session skiff"), SkiffCount, 0);

    FirstWorld.GameMode->OceanTravelSaveWriterForTests = [PlayerSlot](USaveGame* SaveGame,
        const FString& Slot, int32 UserIndex)
    {
        return Slot != PlayerSlot && UGameplayStatics::SaveGameToSlot(SaveGame, Slot, UserIndex);
    };
    FActorSpawnParameters SkiffParameters;
    SkiffParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaOceanSkiff* Skiff = FirstWorld.World->SpawnActor<AKalmalaOceanSkiff>(
        SafeLocation, FRotator::ZeroRotator, SkiffParameters);
    if (!TestNotNull(TEXT("Boarding rollback skiff created"), Skiff)
        || !TestTrue(TEXT("Boarding rollback skiff receives its persistent identity"),
            Skiff->InitializePersistentIdentityFromServer(TEXT("ocean-skiff:primary")))
        || !TestTrue(TEXT("The initial moored world state is saved"),
            FirstWorld.GameMode->OnOceanSkiffStateChanged(Skiff))) return false;

    TestFalse(TEXT("Passenger write failure rejects boarding"), Skiff->TryInteractFromServer(Character));
    TestNull(TEXT("Passenger write failure leaves the helm empty"), Skiff->GetHelmOccupant());
    TestNull(TEXT("Passenger write failure leaves the requester unattached"), Character->GetAttachParentActor());
    TestEqual(TEXT("Passenger write failure publishes a cancellation reason"),
        Character->GetOceanTravelFeedbackComponent()->GetFeedback(),
        EKalmalaOceanTravelFeedback::TravelActionSaveUnavailable);
    TestFalse(TEXT("Failed passenger write creates no player slot"),
        UGameplayStatics::DoesSaveGameExist(PlayerSlot, 0));

    const FKalmalaM7SaveIdentity WrongPlayerIdentity = FKalmalaM7SaveIdentity::ForPlayer(
        WorldSeed, PlayerIdentity + TEXT(":incompatible"));
    UKalmalaOceanTravelPersistenceSaveGame* IncompatibleSave =
        NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
    if (!TestNotNull(TEXT("Incompatible passenger record created"), IncompatibleSave)) return false;
    IncompatibleSave->Initialize(WrongPlayerIdentity);
    if (!TestTrue(TEXT("Incompatible passenger bytes are seeded"),
        UGameplayStatics::SaveGameToSlot(IncompatibleSave, PlayerSlot, 0))) return false;

    TArray<uint8> OriginalPlayerSlotBytes;
    if (!TestTrue(TEXT("Incompatible passenger bytes can be read"),
        FFileHelper::LoadFileToArray(OriginalPlayerSlotBytes, *PlayerSlotPath))) return false;
    FirstWorld.GameMode->OceanTravelSaveWriterForTests = [](USaveGame* SaveGame,
        const FString& Slot, int32 UserIndex)
    {
        return UGameplayStatics::SaveGameToSlot(SaveGame, Slot, UserIndex);
    };
    TestFalse(TEXT("Incompatible passenger slot rejects boarding"), Skiff->TryInteractFromServer(Character));
    TestNull(TEXT("Incompatible passenger slot leaves the seat empty"), Skiff->GetHelmOccupant());
    TestNull(TEXT("Incompatible passenger slot leaves the requester unattached"), Character->GetAttachParentActor());
    TestEqual(TEXT("Incompatible passenger slot reports a save denial"),
        Character->GetOceanTravelFeedbackComponent()->GetFeedback(),
        EKalmalaOceanTravelFeedback::TravelActionSaveUnavailable);
    TArray<uint8> RejectedPlayerSlotBytes;
    if (!TestTrue(TEXT("Incompatible passenger bytes remain readable"),
        FFileHelper::LoadFileToArray(RejectedPlayerSlotBytes, *PlayerSlotPath))) return false;
    TestTrue(TEXT("Incompatible passenger slot remains byte-identical"),
        OriginalPlayerSlotBytes == RejectedPlayerSlotBytes);

    TestTrue(TEXT("The incompatible slot is removed before the valid-seat case"),
        UGameplayStatics::DeleteGameInSlot(PlayerSlot, 0));
    TestTrue(TEXT("A valid passenger slot can be created after the incompatible record is removed"),
        Skiff->TryInteractFromServer(Character));
    TestEqual(TEXT("Successful boarding reports the helm seat"),
        Character->GetOceanTravelFeedbackComponent()->GetFeedback(),
        EKalmalaOceanTravelFeedback::HelmAssigned);
    TestTrue(TEXT("Successful boarding assigns and attaches the authenticated player"),
        Skiff->GetHelmOccupant() == Character && Character->GetAttachParentActor() == Skiff);

    FTravelWorldFixture RestartWorld = CreateTravelWorld(TEXT("Restart travel"));
    if (!TestNotNull(TEXT("Restart travel world is ready"), RestartWorld.World)
        || !TestNotNull(TEXT("Restart travel GameMode is ready"), RestartWorld.GameMode)) return false;
    TestTrue(TEXT("The current-format world record loads after restart"),
        RestartWorld.GameMode->bOceanTravelPersistenceWritable
            && RestartWorld.GameMode->OceanTravelWorldSave != nullptr);
    RestartWorld.GameMode->RestorePersistedOceanSkiff();
    AKalmalaOceanSkiff* RestoredSkiff = nullptr;
    for (TActorIterator<AKalmalaOceanSkiff> It(RestartWorld.World); It; ++It)
    {
        RestoredSkiff = *It;
        break;
    }
    if (!TestNotNull(TEXT("Persisted skiff restores after restart"), RestoredSkiff)) return false;

    APlayerController* RestoreController = RestartWorld.World->SpawnActor<APlayerController>();
    APlayerState* RestorePlayerState = RestartWorld.World->SpawnActor<APlayerState>();
    AKalmalaCharacter* RestoreCharacter = RestartWorld.World->SpawnActor<AKalmalaCharacter>(
        SafeLocation + FVector(300.0, 0.0, 100.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Restart player controller created"), RestoreController)
        || !TestNotNull(TEXT("Restart authenticated player state created"), RestorePlayerState)
        || !TestNotNull(TEXT("Restart authenticated character created"), RestoreCharacter)) return false;
    RestorePlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    RestoreController->SetPlayerState(RestorePlayerState);
    RestoreCharacter->SetPlayerState(RestorePlayerState);
    RestoreController->Possess(RestoreCharacter);
    RestartWorld.GameMode->RestoreOceanTravelForPlayer(RestoreController);

    TestTrue(TEXT("Restart restores the exact vessel identity and safe transform"),
        RestoredSkiff->GetPersistentVesselId() == TEXT("ocean-skiff:primary")
            && RestoredSkiff->GetActorLocation().Equals(SafeLocation, 0.1)
            && FMath::IsNearlyEqual(RestoredSkiff->GetActorRotation().Yaw, 0.0f));
    TestTrue(TEXT("Restart restores the authenticated helm seat and attachment"),
        RestoredSkiff->GetHelmOccupant() == RestoreCharacter
            && RestoreCharacter->GetAttachParentActor() == RestoredSkiff);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
