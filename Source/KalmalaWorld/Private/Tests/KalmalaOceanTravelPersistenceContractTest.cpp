#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaOceanTravelPersistenceContract.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaOceanTravelPersistenceContractTest,
    "Kalmala.World.OceanTravel.PersistenceContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaOceanTravelPersistenceContractTest::RunTest(const FString& Parameters)
{
    const FKalmalaM7SaveIdentity WorldIdentity = FKalmalaM7SaveIdentity::ForWorld(418);
    UKalmalaOceanTravelPersistenceSaveGame* WorldSave = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
    WorldSave->Initialize(WorldIdentity);

    FKalmalaOceanTravelVesselState Vessel;
    Vessel.VesselId = TEXT("ocean-skiff:session-0001");
    Vessel.SafeLocation = FVector(5'000.0, -10'000.0, 250.0);
    Vessel.YawDegrees = 90.0f;
    TestTrue(TEXT("A world-scoped save accepts one bounded moored vessel snapshot"), WorldSave->SetVesselState(Vessel));
    TestTrue(TEXT("The accepted world save matches exact world and generator identity"), WorldSave->Matches(WorldIdentity));
    TestFalse(TEXT("A second vessel identity exceeds the one-skiff world bound"), WorldSave->SetVesselState({ TEXT("ocean-skiff:session-0002"), FVector::ZeroVector, 0.0f }));

    TArray<uint8> WorldBytes;
    TestTrue(TEXT("The world-scoped contract serializes to memory"), UGameplayStatics::SaveGameToMemory(WorldSave, WorldBytes));
    UKalmalaOceanTravelPersistenceSaveGame* ReloadedWorldSave =
        Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(WorldBytes));
    if (TestNotNull(TEXT("The world snapshot reloads as its versioned contract"), ReloadedWorldSave))
    {
        TestTrue(TEXT("The reloaded vessel retains exact world identity"), ReloadedWorldSave->Matches(WorldIdentity));
        TestEqual(TEXT("The reloaded world snapshot retains its stable vessel ID"),
            ReloadedWorldSave->GetVesselState().VesselId, Vessel.VesselId);
        TestTrue(TEXT("The reloaded world snapshot retains its last safe transform"),
            ReloadedWorldSave->GetVesselState().SafeLocation.Equals(Vessel.SafeLocation));
        TestEqual(TEXT("The reloaded world snapshot retains its accepted heading"),
            ReloadedWorldSave->GetVesselState().YawDegrees, Vessel.YawDegrees);
    }

    const FKalmalaM7SaveIdentity PlayerIdentity = FKalmalaM7SaveIdentity::ForPlayer(418, TEXT("player-alpha"));
    UKalmalaOceanTravelPersistenceSaveGame* PlayerSave = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
    PlayerSave->Initialize(PlayerIdentity);
    FKalmalaOceanTravelPassengerState Passenger;
    Passenger.VesselId = Vessel.VesselId;
    Passenger.Seat = EKalmalaOceanTravelSavedSeat::Passenger;
    TestTrue(TEXT("An authenticated player save accepts one seat association"), PlayerSave->SetPassengerState(Passenger));
    TestTrue(TEXT("The accepted player save matches the authenticated identity"), PlayerSave->Matches(PlayerIdentity));
    TestFalse(TEXT("A player record cannot contain a world vessel snapshot"), PlayerSave->SetVesselState(Vessel));

    TArray<uint8> PlayerBytes;
    TestTrue(TEXT("The player-scoped contract serializes to memory"), UGameplayStatics::SaveGameToMemory(PlayerSave, PlayerBytes));
    UKalmalaOceanTravelPersistenceSaveGame* ReloadedPlayerSave =
        Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(PlayerBytes));
    if (TestNotNull(TEXT("The player seat reloads as its versioned contract"), ReloadedPlayerSave))
    {
        TestTrue(TEXT("The reloaded seat retains the authenticated player identity"), ReloadedPlayerSave->Matches(PlayerIdentity));
        TestEqual(TEXT("The reloaded seat retains the server-selected vessel reference"),
            ReloadedPlayerSave->GetPassengerState().VesselId, Vessel.VesselId);
        TestEqual(TEXT("The reloaded seat retains its assigned role"),
            static_cast<uint8>(ReloadedPlayerSave->GetPassengerState().Seat),
            static_cast<uint8>(EKalmalaOceanTravelSavedSeat::Passenger));
        if (ReloadedWorldSave)
        {
            TestTrue(TEXT("A passenger association pairs only with the matching world vessel"),
                ReloadedPlayerSave->CanPairWithWorldSave(*ReloadedWorldSave));
        }
    }

    TestEqual(TEXT("Schema one loads without migration"),
        static_cast<uint8>(UKalmalaOceanTravelPersistenceSaveGame::EvaluateSchemaVersion(
            UKalmalaOceanTravelPersistenceSaveGame::CurrentSchemaVersion)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::AcceptCurrent));
    TestEqual(TEXT("Legacy unversioned data requires explicit migration"),
        static_cast<uint8>(UKalmalaOceanTravelPersistenceSaveGame::EvaluateSchemaVersion(0)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::MigrateBeforeLoad));
    TestEqual(TEXT("Future schema data fails closed"),
        static_cast<uint8>(UKalmalaOceanTravelPersistenceSaveGame::EvaluateSchemaVersion(
            UKalmalaOceanTravelPersistenceSaveGame::CurrentSchemaVersion + 1)),
        static_cast<uint8>(EKalmalaM7SchemaDecision::Reject));

    FKalmalaM7SaveIdentity WrongWorld = WorldIdentity;
    WrongWorld.WorldSeed = 419;
    TestFalse(TEXT("A different world seed cannot reuse the vessel snapshot"), WorldSave->Matches(WrongWorld));
    WrongWorld = WorldIdentity;
    WrongWorld.GeneratorRevision++;
    TestFalse(TEXT("A different generator revision cannot reuse the vessel snapshot"), WorldSave->Matches(WrongWorld));

    const FKalmalaM7SaveIdentity OtherPlayer = FKalmalaM7SaveIdentity::ForPlayer(418, TEXT("player-beta"));
    TestFalse(TEXT("A player seat cannot load under another authenticated identity"), PlayerSave->Matches(OtherPlayer));
    TestFalse(TEXT("A player-scoped save cannot masquerade as a world save"), PlayerSave->Matches(WorldIdentity));
    if (ReloadedPlayerSave && ReloadedWorldSave)
    {
        UKalmalaOceanTravelPersistenceSaveGame* OtherWorldSave = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
        OtherWorldSave->Initialize(FKalmalaM7SaveIdentity::ForWorld(419));
        OtherWorldSave->SetVesselState(Vessel);
        TestFalse(TEXT("A player seat cannot pair across world identities"), ReloadedPlayerSave->CanPairWithWorldSave(*OtherWorldSave));

        UKalmalaOceanTravelPersistenceSaveGame* DifferentVesselSave = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
        DifferentVesselSave->Initialize(WorldIdentity);
        DifferentVesselSave->SetVesselState({ TEXT("ocean-skiff:session-0003"), Vessel.SafeLocation, Vessel.YawDegrees });
        TestFalse(TEXT("A player seat cannot pair with a different vessel identity"), ReloadedPlayerSave->CanPairWithWorldSave(*DifferentVesselSave));
    }

    TestFalse(TEXT("Path-like vessel IDs are rejected"), FKalmalaOceanTravelVesselState::IsValidVesselId(TEXT("ocean-skiff:../slot")));
    TestFalse(TEXT("Vessel snapshots outside the finite world are rejected"),
        FKalmalaOceanTravelVesselState{ TEXT("ocean-skiff:bounded"), FVector(1'600'001.0, 0.0, 0.0), 0.0f }.IsValid());
    TestFalse(TEXT("Non-finite vessel coordinates are rejected"),
        FKalmalaOceanTravelVesselState{ TEXT("ocean-skiff:finite"), FVector(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0), 0.0f }.IsValid());
    TestFalse(TEXT("Non-finite heading is rejected"),
        FKalmalaOceanTravelVesselState{ TEXT("ocean-skiff:heading"), FVector::ZeroVector, std::numeric_limits<float>::infinity() }.IsValid());
    TestFalse(TEXT("Unknown saved seat values are rejected"),
        FKalmalaOceanTravelPassengerState{ Vessel.VesselId, static_cast<EKalmalaOceanTravelSavedSeat>(255) }.IsValid());

    UKalmalaOceanTravelPersistenceSaveGame* EmptySave = NewObject<UKalmalaOceanTravelPersistenceSaveGame>();
    EmptySave->Initialize(WorldIdentity);
    TestFalse(TEXT("An initialized but payload-free world save cannot be restored"), EmptySave->Matches(WorldIdentity));

    return true;
}

#endif
