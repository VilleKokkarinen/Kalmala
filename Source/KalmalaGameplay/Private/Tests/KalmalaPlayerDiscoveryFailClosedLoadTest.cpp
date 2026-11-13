#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerDiscoveryFailClosedLoadTest,
    "Kalmala.Gameplay.Persistence.PlayerDiscoveryFailClosedLoad",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlayerDiscoveryFailClosedLoadTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Player-save fixture world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    };

    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Player-save fixture world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("World state accepts the authored fixture seed"),
            WorldState->SelectWorldSeedFromServer(418))) return false;

    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Player-save fixture selects the authoritative GameMode"), World->SetGameMode(FURL())))
        return false;
    AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Player-save fixture GameMode created"), Mode)) return false;

    AKalmalaCharacter* Player = World->SpawnActor<AKalmalaCharacter>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Authenticated player pawn created"), Player)
        || !TestNotNull(TEXT("Authenticated player state created"), PlayerState)) return false;
    Player->DispatchBeginPlay();
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("rf22-player-discovery-owner"), FName(TEXT("KalmalaRF22Test")));
    PlayerState->SetUniqueId(FUniqueNetIdRepl(*NetId));
    PlayerState->SetPlayerId(1);
    Player->SetPlayerState(PlayerState);
    const FString PlayerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    const FKalmalaWorldGenerationConfig WorldConfig = WorldState->GetWorldGenerationConfig();
    Mode->WorldGenerationConfig = WorldConfig;

    const FString ExpectedSlot = FString::Printf(TEXT("KalmalaPlayerDiscoveries_%llu_%08x"),
        WorldConfig.WorldSeed, FCrc::StrCrc32(*PlayerIdentity));
    TMap<FString, TArray<uint8>> PlayerSlotBytes;
    int32 LoadAttempts = 0;
    int32 WriteAttempts = 0;
    Mode->PlayerDiscoverySaveLoaderForTests = [this, &PlayerSlotBytes, &LoadAttempts, ExpectedSlot](
        const FString& Slot, const int32 UserIndex, USaveGame*& OutSaveGame)
    {
        ++LoadAttempts;
        TestEqual(TEXT("GameMode loads the exact authenticated player slot"), Slot, ExpectedSlot);
        TestEqual(TEXT("GameMode reads local user zero"), UserIndex, 0);
        const TArray<uint8>* Bytes = PlayerSlotBytes.Find(Slot);
        if (Bytes == nullptr) return false;
        OutSaveGame = UGameplayStatics::LoadGameFromMemory(*Bytes);
        return true;
    };
    Mode->PlayerDiscoverySaveWriterForTests = [&WriteAttempts](
        UKalmalaPlayerDiscoverySaveGameV2*, const FString&, int32)
    {
        ++WriteAttempts;
        return false;
    };

    UKalmalaSupportMagicComponent* Support = Player->GetSupportMagicComponent();
    UKalmalaInventoryComponent* Inventory = Player->GetInventoryComponent();
    if (!TestNotNull(TEXT("Player has server support state"), Support)
        || !TestNotNull(TEXT("Player has server inventory"), Inventory)) return false;
    const int32 MendingScrollsBefore = Inventory->GetQuantity(TEXT("MendingScroll"));

    TArray<FKalmalaToolState>& RuntimeTools =
        const_cast<TArray<FKalmalaToolState>&>(Player->GetCarriedToolInventory());
    FKalmalaToolState SentinelTool;
    SentinelTool.ItemId = TEXT("StoneKnife");
    SentinelTool.ToolLevel = 1;
    SentinelTool.Durability = 7;
    SentinelTool.InstanceGuid = FGuid::NewGuid();
    const auto ResetRuntimeState = [&RuntimeTools, &SentinelTool]()
    {
        RuntimeTools.Reset();
        RuntimeTools.Add(SentinelTool);
    };
    const auto CheckRuntimeStateUnchanged = [this, Player, Support, Inventory, MendingScrollsBefore,
        &SentinelTool](const TCHAR* Label)
    {
        const TArray<FKalmalaToolState>& Tools = Player->GetCarriedToolInventory();
        const FString Prefix = FString(Label);
        TestTrue(Prefix + TEXT(" invalid slot restores no carried tools"), Tools.Num() == 1);
        if (Tools.Num() == 1)
        {
            TestTrue(Prefix + TEXT(" keeps the pre-load tool instance"),
                Tools[0].ItemId == SentinelTool.ItemId && Tools[0].InstanceGuid == SentinelTool.InstanceGuid
                    && Tools[0].Durability == SentinelTool.Durability);
        }
        TestFalse(Prefix + TEXT(" invalid slot restores no learned effect"),
            Support->HasLearnedEffect(EKalmalaSupportEffect::Mending));
        TestTrue(Prefix + TEXT(" invalid slot grants no support scroll"),
            Inventory->GetQuantity(TEXT("MendingScroll")) == MendingScrollsBefore);
    };

    const FGuid SavedToolGuid = FGuid::NewGuid();
    const FString DiscoveryId(TEXT("Poi:1:lakes-island-cache:-2,4:1"));
    const auto BuildCurrentSave = [Mode, &SavedToolGuid, &DiscoveryId](
        const FKalmalaWorldGenerationConfig& SaveWorld, const FString& SaveOwner,
        const FString& OptionalM9Claim, const bool bIncludeLearnedEffect)
        -> UKalmalaPlayerDiscoverySaveGameV2*
    {
        UKalmalaPlayerDiscoverySaveGameV2* Save = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(Mode);
        Save->InitializeForPlayer(SaveWorld, SaveOwner);
        FKalmalaPlayerToolSaveRecord Record;
        Record.ItemId = TEXT("StoneKnife");
        Record.ToolLevel = 1;
        Record.Condition = 9;
        Record.InstanceGuid = SavedToolGuid;
        if (!Save->AddDiscovery(DiscoveryId) || !Save->AddToolRecord(Record)) return nullptr;
        if (bIncludeLearnedEffect && !Save->AddLearnedEffect(TEXT("Effect:mending"))) return nullptr;
        if (!OptionalM9Claim.IsEmpty() && !Save->AddM9Claim(OptionalM9Claim)) return nullptr;
        return Save;
    };

    const auto SerializeSave = [this](UKalmalaPlayerDiscoverySaveGameV2* Save,
        const TCHAR* Label, TArray<uint8>& OutBytes)
    {
        TestNotNull(FString::Printf(TEXT("%s save fixture created"), Label), Save);
        if (Save == nullptr) return false;
        TestTrue(FString::Printf(TEXT("%s player slot serializes in memory"), Label),
            UGameplayStatics::SaveGameToMemory(Save, OutBytes));
        return !OutBytes.IsEmpty();
    };

    const auto AssertRejectedSlot = [this, Mode, Player, &ResetRuntimeState,
        &CheckRuntimeStateUnchanged, &PlayerSlotBytes, &ExpectedSlot, &PlayerIdentity,
        &LoadAttempts, &WriteAttempts, &SerializeSave](const TCHAR* Label,
            UKalmalaPlayerDiscoverySaveGameV2* InvalidSave)
    {
        TArray<uint8> InvalidBytes;
        if (!SerializeSave(InvalidSave, Label, InvalidBytes)) return;
        PlayerSlotBytes.Add(ExpectedSlot, InvalidBytes);
        Mode->PlayerDiscoverySaves.Remove(PlayerIdentity);
        Mode->RejectedPlayerDiscoverySaveIdentities.Remove(PlayerIdentity);
        ResetRuntimeState();
        const TArray<uint8> BeforeBytes = PlayerSlotBytes.FindChecked(ExpectedSlot);
        const int32 ReadsBefore = LoadAttempts;

        FString LoadedIdentity;
        UKalmalaPlayerDiscoverySaveGameV2* Restored = Mode->GetPlayerDiscoverySave(Player, LoadedIdentity);
        TestNull(FString::Printf(TEXT("%s slot returns no player save"), Label), Restored);
        TestEqual(FString::Printf(TEXT("%s rejection retains the authenticated identity"), Label),
            LoadedIdentity, PlayerIdentity);
        CheckRuntimeStateUnchanged(Label);
        TestFalse(FString::Printf(TEXT("%s subsequent server persistence remains rejected"), Label),
            Mode->PersistPlayerStateFromServer(Player));
        TestEqual(FString::Printf(TEXT("%s calls the exact slot loader once"), Label), LoadAttempts, ReadsBefore + 1);
        TestEqual(FString::Printf(TEXT("%s never reaches the player slot writer"), Label), WriteAttempts, 0);
        TestTrue(FString::Printf(TEXT("%s preserves the original exact slot bytes"), Label),
            PlayerSlotBytes.FindChecked(ExpectedSlot) == BeforeBytes);
        TestFalse(FString::Printf(TEXT("%s is not cached as a valid player save"), Label),
            Mode->PlayerDiscoverySaves.Contains(PlayerIdentity));
        TestTrue(FString::Printf(TEXT("%s permanently blocks later slot access in this session"), Label),
            Mode->RejectedPlayerDiscoverySaveIdentities.Contains(PlayerIdentity));
    };

    UKalmalaPlayerDiscoverySaveGameV2* InvalidSchema =
        BuildCurrentSave(WorldConfig, PlayerIdentity, FString(), false);
    if (!TestNotNull(TEXT("Invalid-schema slot fixture built"), InvalidSchema)) return false;
    InvalidSchema->SchemaVersion = 0;
    TestFalse(TEXT("Schema-zero fixture is rejected by the save model"),
        InvalidSchema->MatchesPlayer(WorldConfig, PlayerIdentity));
    AssertRejectedSlot(TEXT("Invalid-schema"), InvalidSchema);

    const FString DifferentOwner = TEXT("KalmalaRF22Test:another-owner");
    UKalmalaPlayerDiscoverySaveGameV2* WrongOwner =
        BuildCurrentSave(WorldConfig, DifferentOwner, FString(), false);
    if (!TestNotNull(TEXT("Wrong-owner slot fixture built"), WrongOwner)) return false;
    TestTrue(TEXT("Wrong-owner fixture is otherwise a valid current-format save"),
        WrongOwner->MatchesPlayer(WorldConfig, DifferentOwner));
    AssertRejectedSlot(TEXT("Wrong-owner"), WrongOwner);

    FKalmalaWorldGenerationConfig DifferentSeed = WorldConfig;
    ++DifferentSeed.WorldSeed;
    UKalmalaPlayerDiscoverySaveGameV2* WrongSeed =
        BuildCurrentSave(DifferentSeed, PlayerIdentity, FString(), false);
    if (!TestNotNull(TEXT("Wrong-seed slot fixture built"), WrongSeed)) return false;
    TestTrue(TEXT("Wrong-seed fixture is otherwise a valid current-format save"),
        WrongSeed->MatchesPlayer(DifferentSeed, PlayerIdentity));
    AssertRejectedSlot(TEXT("Wrong-seed"), WrongSeed);

    FKalmalaWorldGenerationConfig DifferentRevision = WorldConfig;
    ++DifferentRevision.GeneratorRevision;
    UKalmalaPlayerDiscoverySaveGameV2* WrongRevision =
        BuildCurrentSave(DifferentRevision, PlayerIdentity, FString(), false);
    if (!TestNotNull(TEXT("Wrong-revision slot fixture built"), WrongRevision)) return false;
    TestTrue(TEXT("Wrong-revision fixture is otherwise a valid current-format save"),
        WrongRevision->MatchesPlayer(DifferentRevision, PlayerIdentity));
    AssertRejectedSlot(TEXT("Wrong-revision"), WrongRevision);

    const FString StaleM9Claim = FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
        TEXT("lakes-rillworn-marker"), FIntPoint(50000, 50000));
    if (!TestTrue(TEXT("Stale M9 fixture uses a well-formed claim identity"),
            UKalmalaPlayerDiscoverySaveGameV2::IsValidM9ClaimId(StaleM9Claim))) return false;
    UKalmalaPlayerDiscoverySaveGameV2* StaleClaim =
        BuildCurrentSave(WorldConfig, PlayerIdentity, StaleM9Claim, false);
    if (!TestNotNull(TEXT("Stale-claim slot fixture built"), StaleClaim)) return false;
    TestTrue(TEXT("Stale-claim fixture is valid to the save model"),
        StaleClaim->MatchesPlayer(WorldConfig, PlayerIdentity));
    TestFalse(TEXT("Stale claim no longer matches a generated world descriptor"),
        Mode->IsCurrentM9ClaimId(StaleM9Claim));
    AssertRejectedSlot(TEXT("Stale-M9-claim"), StaleClaim);

    UKalmalaPlayerDiscoverySaveGameV2* ValidCurrent =
        BuildCurrentSave(WorldConfig, PlayerIdentity, FString(), true);
    if (!TestNotNull(TEXT("Valid current-format slot fixture built"), ValidCurrent)) return false;
    TArray<uint8> ValidBytes;
    if (!SerializeSave(ValidCurrent, TEXT("Valid-current-format"), ValidBytes)) return false;
    PlayerSlotBytes.Add(ExpectedSlot, ValidBytes);
    Mode->PlayerDiscoverySaves.Remove(PlayerIdentity);
    Mode->RejectedPlayerDiscoverySaveIdentities.Remove(PlayerIdentity);
    ResetRuntimeState();
    FString ValidIdentity;
    UKalmalaPlayerDiscoverySaveGameV2* RestoredValid = Mode->GetPlayerDiscoverySave(Player, ValidIdentity);
    if (!TestNotNull(TEXT("Valid current-format slot loads through GameMode"), RestoredValid)) return false;
    TestEqual(TEXT("Valid load returns its authenticated owner identity"), ValidIdentity, PlayerIdentity);
    TestTrue(TEXT("Valid loaded save retains exact seed, revision, scope, and owner"),
        RestoredValid->MatchesPlayer(WorldConfig, PlayerIdentity));
    TestTrue(TEXT("Valid loaded save retains its discovery"), RestoredValid->HasDiscovery(DiscoveryId));
    TestTrue(TEXT("Valid load restores the saved tool instance"),
        Player->GetCarriedToolInventory().Num() == 1
            && Player->GetCarriedToolInventory()[0].InstanceGuid == SavedToolGuid
            && Player->GetCarriedToolInventory()[0].Durability == 9);
    TestTrue(TEXT("Valid load restores its learned support entitlement"),
        Support->HasLearnedEffect(EKalmalaSupportEffect::Mending));
    TestEqual(TEXT("Valid load rebuilds the support scroll from its saved entitlement"),
        Inventory->GetQuantity(TEXT("MendingScroll")), MendingScrollsBefore + 1);
    TestEqual(TEXT("Valid load does not invoke the slot writer"), WriteAttempts, 0);
    TestFalse(TEXT("Valid identity is not rejected"), Mode->RejectedPlayerDiscoverySaveIdentities.Contains(PlayerIdentity));
    return true;
}

#endif
