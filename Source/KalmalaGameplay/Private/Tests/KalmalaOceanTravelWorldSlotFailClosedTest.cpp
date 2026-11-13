#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanTravelPersistenceContract.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"
#include <limits>

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaOceanTravelWorldSlotFailClosedTest,
    "Kalmala.Gameplay.OceanTravel.WorldSlotFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaOceanTravelWorldSlotFailClosedTest::RunTest(const FString& Parameters)
{
    const FGuid Guid = FGuid::NewGuid();
    const uint64 GeneratedWorldSeed = (static_cast<uint64>(Guid.A) << 32u) | Guid.B;
    const uint64 WorldSeed = GeneratedWorldSeed == 0 ? 1 : GeneratedWorldSeed;
    const FKalmalaWorldGenerationConfig Config{WorldSeed};
    const FKalmalaM7SaveIdentity WorldIdentity = FKalmalaM7SaveIdentity::ForWorld(WorldSeed);
    const FString WorldSlot = FString::Printf(TEXT("KalmalaOceanTravelWorld_%llu_%d"),
        WorldIdentity.WorldSeed, WorldIdentity.GeneratorRevision);
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(TEXT("owner-01"), FName(TEXT("KalmalaRF25Test")));
    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    const FKalmalaM7SaveIdentity PlayerSaveIdentity = FKalmalaM7SaveIdentity::ForPlayer(WorldSeed, PlayerIdentity);
    const FString PlayerSlot = FString::Printf(TEXT("KalmalaOceanTravelPlayer_%llu_%d_%08x"),
        PlayerSaveIdentity.WorldSeed, PlayerSaveIdentity.GeneratorRevision, FCrc::StrCrc32(*PlayerIdentity));
    bool bOwnsWorldSlot = false;
    bool bOwnsPlayerSlot = false;
    UWorld* InvalidWorld = nullptr;
    UWorld* ValidWorld = nullptr;
    ON_SCOPE_EXIT
    {
        if (InvalidWorld != nullptr)
        {
            if (InvalidWorld->HasBegunPlay()) InvalidWorld->EndPlay(EEndPlayReason::Destroyed);
            InvalidWorld->DestroyWorld(false);
            if (GEngine != nullptr) GEngine->DestroyWorldContext(InvalidWorld);
        }
        if (ValidWorld != nullptr)
        {
            if (ValidWorld->HasBegunPlay()) ValidWorld->EndPlay(EEndPlayReason::Destroyed);
            ValidWorld->DestroyWorld(false);
            if (GEngine != nullptr) GEngine->DestroyWorldContext(ValidWorld);
        }
        if (bOwnsWorldSlot) UGameplayStatics::DeleteGameInSlot(WorldSlot, 0);
        if (bOwnsPlayerSlot) UGameplayStatics::DeleteGameInSlot(PlayerSlot, 0);
    };

    const FString SavedDirectory = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    const FString WorldSlotPath = FPaths::ConvertRelativePathToFull(SavedDirectory / (WorldSlot + TEXT(".sav")));
    const FString PlayerSlotPath = FPaths::ConvertRelativePathToFull(SavedDirectory / (PlayerSlot + TEXT(".sav")));
    if (!TestTrue(TEXT("The isolated world slot is unused"), !UGameplayStatics::DoesSaveGameExist(WorldSlot, 0))
        || !TestTrue(TEXT("The isolated player slot is unused"), !UGameplayStatics::DoesSaveGameExist(PlayerSlot, 0))
        || !TestTrue(TEXT("The isolated world slot path is below the Windows limit"), WorldSlotPath.Len() < 260)
        || !TestTrue(TEXT("The isolated player slot path is below the Windows limit"), PlayerSlotPath.Len() < 260))
    {
        return false;
    }
    bOwnsWorldSlot = true;
    bOwnsPlayerSlot = true;

    FVector SafeLocation;
    if (!TestTrue(TEXT("The isolated world has a navigable ocean restore location"),
        FindNavigableOceanLocation(Config, SafeLocation))) return false;

    const auto MakeValidWorldSave = [this, &WorldIdentity, &SafeLocation]()
        -> UKalmalaOceanTravelPersistenceSaveGame*
    {
        UKalmalaOceanTravelPersistenceSaveGame* Save = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
        if (!TestNotNull(TEXT("Current-format world save created"), Save)) return nullptr;
        Save->Initialize(WorldIdentity);
        FKalmalaOceanTravelVesselState Vessel;
        Vessel.VesselId = TEXT("ocean-skiff:primary");
        Vessel.SafeLocation = SafeLocation;
        Vessel.YawDegrees = 0.0f;
        if (!TestTrue(TEXT("Current-format vessel payload is valid"), Save->SetVesselState(Vessel))) return nullptr;
        return Save;
    };

    const auto CreateWorldMode = [this, &Config](UWorld* World, const FString& Label) -> AKalmalaGameMode*
    {
        if (World == nullptr) return nullptr;
        if (GEngine != nullptr)
        {
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        }
        World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
        if (!TestTrue(Label + TEXT(" uses the authoritative GameMode"), World->SetGameMode(FURL()))) return nullptr;
        AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
        if (!TestNotNull(Label + TEXT(" GameMode created"), Mode)) return nullptr;
        Mode->WorldGenerationConfig = Config;
        return Mode;
    };

    InvalidWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Invalid-slot fixture world created"), InvalidWorld)) return false;
    AKalmalaGameMode* InvalidMode = CreateWorldMode(InvalidWorld, TEXT("Invalid-slot fixture"));
    if (!TestNotNull(TEXT("Invalid-slot fixture GameMode available"), InvalidMode)) return false;

    APlayerState* PlayerState = InvalidWorld->SpawnActor<APlayerState>();
    AKalmalaCharacter* Character = InvalidWorld->SpawnActor<AKalmalaCharacter>(
        SafeLocation + FVector(5000.0, 0.0, 300.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Authenticated passenger player state created"), PlayerState)
        || !TestNotNull(TEXT("Authoritative passenger character created"), Character)) return false;
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    Character->SetPlayerState(PlayerState);

    const auto CheckRejectedRecord = [this, InvalidWorld, InvalidMode, Character, &WorldSlot, &WorldSlotPath,
        &PlayerSlot, &SafeLocation](const FString& CaseName,
        UKalmalaOceanTravelPersistenceSaveGame* Record) -> bool
    {
        if (!TestNotNull(CaseName + TEXT(" record created"), Record)) return false;
        const FString Prefix = CaseName + TEXT(": ");
        if (!TestTrue(Prefix + TEXT("incompatible bytes are seeded in the production world slot"),
                UGameplayStatics::SaveGameToSlot(Record, WorldSlot, 0))) return false;

        TArray<uint8> OriginalSlotBytes;
        if (!TestTrue(Prefix + TEXT("original bytes are readable"),
                FFileHelper::LoadFileToArray(OriginalSlotBytes, *WorldSlotPath))) return false;

        InvalidMode->LoadOceanTravelWorldSave();
        InvalidMode->RestorePersistedOceanSkiff();
        TestFalse(Prefix + TEXT("production loader blocks world travel persistence"),
            InvalidMode->bOceanTravelPersistenceWritable);
        TestNull(Prefix + TEXT("production loader retains no incompatible cache"),
            InvalidMode->OceanTravelWorldSave.Get());

        int32 RestoredCount = 0;
        for (TActorIterator<AKalmalaOceanSkiff> It(InvalidWorld); It; ++It) ++RestoredCount;
        TestEqual(Prefix + TEXT("production restore spawns no skiff"), RestoredCount, 0);

        FActorSpawnParameters CandidateParameters;
        CandidateParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaOceanSkiff* Candidate = InvalidWorld->SpawnActor<AKalmalaOceanSkiff>(
            SafeLocation, FRotator::ZeroRotator, CandidateParameters);
        if (!TestNotNull(Prefix + TEXT("world persistence candidate created"), Candidate)) return false;
        if (!TestTrue(Prefix + TEXT("candidate receives the stable server vessel ID"),
                Candidate->InitializePersistentIdentityFromServer(TEXT("ocean-skiff:primary")))) return false;
        TestFalse(Prefix + TEXT("world travel persistence rejects the candidate"),
            InvalidMode->PersistOceanTravelState(Candidate));
        TestFalse(Prefix + TEXT("authenticated passenger persistence is rejected"),
            InvalidMode->PersistOceanTravelPassenger(Character, TEXT("ocean-skiff:primary"),
                EKalmalaOceanSkiffSeat::Helm));
        TestFalse(Prefix + TEXT("rejected passenger persistence creates no player slot"),
            UGameplayStatics::DoesSaveGameExist(PlayerSlot, 0));
        Candidate->Destroy();

        TArray<uint8> RejectedSlotBytes;
        if (!TestTrue(Prefix + TEXT("slot bytes remain readable after rejected writes"),
                FFileHelper::LoadFileToArray(RejectedSlotBytes, *WorldSlotPath))) return false;
        TestTrue(Prefix + TEXT("exact original world-slot bytes are unchanged"),
            OriginalSlotBytes == RejectedSlotBytes);
        return true;
    };

    UKalmalaOceanTravelPersistenceSaveGame* FutureSchema = MakeValidWorldSave();
    if (FutureSchema == nullptr) return false;
    FutureSchema->SchemaVersion = UKalmalaOceanTravelPersistenceSaveGame::CurrentSchemaVersion + 1;
    if (!CheckRejectedRecord(TEXT("Unknown future schema"), FutureSchema)) return false;

    UKalmalaOceanTravelPersistenceSaveGame* Unversioned = MakeValidWorldSave();
    if (Unversioned == nullptr) return false;
    Unversioned->SchemaVersion = 0;
    if (!CheckRejectedRecord(TEXT("Unversioned save schema"), Unversioned)) return false;

    UKalmalaOceanTravelPersistenceSaveGame* WrongSeed = MakeValidWorldSave();
    if (WrongSeed == nullptr) return false;
    WrongSeed->Identity.WorldSeed ^= 1ull;
    if (!CheckRejectedRecord(TEXT("Wrong world seed"), WrongSeed)) return false;

    UKalmalaOceanTravelPersistenceSaveGame* WrongRevision = MakeValidWorldSave();
    if (WrongRevision == nullptr) return false;
    ++WrongRevision->Identity.GeneratorRevision;
    if (!CheckRejectedRecord(TEXT("Wrong generator revision"), WrongRevision)) return false;

    UKalmalaOceanTravelPersistenceSaveGame* MissingPayload = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
    if (!TestNotNull(TEXT("Payload-free world save created"), MissingPayload)) return false;
    MissingPayload->Initialize(WorldIdentity);
    if (!CheckRejectedRecord(TEXT("Missing vessel payload"), MissingPayload)) return false;

    UKalmalaOceanTravelPersistenceSaveGame* InvalidVesselId = MakeValidWorldSave();
    if (InvalidVesselId == nullptr) return false;
    InvalidVesselId->VesselState.VesselId = TEXT("not-a-vessel");
    if (!CheckRejectedRecord(TEXT("Invalid vessel identity"), InvalidVesselId)) return false;

    UKalmalaOceanTravelPersistenceSaveGame* InvalidLocation = MakeValidWorldSave();
    if (InvalidLocation == nullptr) return false;
    InvalidLocation->VesselState.SafeLocation.X = std::numeric_limits<double>::quiet_NaN();
    if (!CheckRejectedRecord(TEXT("Malformed vessel location"), InvalidLocation)) return false;

    UKalmalaOceanTravelPersistenceSaveGame* InvalidHeading = MakeValidWorldSave();
    if (InvalidHeading == nullptr) return false;
    InvalidHeading->VesselState.YawDegrees = std::numeric_limits<float>::infinity();
    if (!CheckRejectedRecord(TEXT("Malformed vessel heading"), InvalidHeading)) return false;

    InvalidWorld->DestroyWorld(false);
    if (GEngine != nullptr) GEngine->DestroyWorldContext(InvalidWorld);
    InvalidWorld = nullptr;

    UKalmalaOceanTravelPersistenceSaveGame* ValidSave = MakeValidWorldSave();
    if (ValidSave == nullptr) return false;
    if (!TestTrue(TEXT("Current-format record written to isolated world slot"),
        UGameplayStatics::SaveGameToSlot(ValidSave, WorldSlot, 0))) return false;

    ValidWorld = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Current-format restart world created"), ValidWorld)) return false;
    AKalmalaGameMode* ValidMode = CreateWorldMode(ValidWorld, TEXT("Current-format restart"));
    if (!TestNotNull(TEXT("Current-format restart GameMode available"), ValidMode)) return false;
    ValidMode->LoadOceanTravelWorldSave();
    TestTrue(TEXT("Current-format world record remains writable"), ValidMode->bOceanTravelPersistenceWritable);
    TestTrue(TEXT("Current-format record is cached by the production loader"),
        ValidMode->OceanTravelWorldSave != nullptr
            && ValidMode->OceanTravelWorldSave->Matches(WorldIdentity));
    ValidMode->RestorePersistedOceanSkiff();

    AKalmalaOceanSkiff* Restored = nullptr;
    int32 RestoredCount = 0;
    for (TActorIterator<AKalmalaOceanSkiff> It(ValidWorld); It; ++It)
    {
        ++RestoredCount;
        Restored = *It;
    }
    TestEqual(TEXT("Current-format startup restores exactly one skiff"), RestoredCount, 1);
    if (TestNotNull(TEXT("Current-format startup restores the saved skiff"), Restored))
    {
        TestEqual(TEXT("Current-format restore retains the stable vessel ID"),
            Restored->GetPersistentVesselId(), TEXT("ocean-skiff:primary"));
        TestTrue(TEXT("Current-format restore retains the exact safe location"),
            Restored->GetActorLocation().Equals(SafeLocation, 0.01));
    }
    return true;
}

#endif
