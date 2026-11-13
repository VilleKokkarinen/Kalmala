#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Crc.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaMirelingBossScrollRecoveryTest,
    "Kalmala.Gameplay.Wildlife.MirelingBossScrollRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaMirelingBossScrollRecoveryTest::RunTest(const FString& Parameters)
{
    if (!TestNotNull(TEXT("Engine is available for isolated scroll-recovery worlds"), GEngine)) return false;

    FKalmalaWorldGenerationConfig WorldConfig;
    WorldConfig.WorldSeed = 19418;
    FKalmalaWorldPopulationSpawn BossSpawn;
    FString BossSpawnId;
    bool bFoundBossSpawn = false;
    for (int32 Y = -64; Y <= 64 && !bFoundBossSpawn; ++Y)
    {
        for (int32 X = -64; X <= 64 && !bFoundBossSpawn; ++X)
        {
            for (const FKalmalaWorldPopulationSpawn& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                    WorldConfig, FIntPoint(X, Y), EKalmalaWorldPopulationKind::Wildlife))
            {
                const FString CandidateId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (Spawn.Archetype != EKalmalaWildlifeArchetype::Mireling
                    || !AKalmalaGameMode::IsMirelingBossRewardId(CandidateId)) continue;
                BossSpawn = Spawn;
                BossSpawnId = CandidateId;
                bFoundBossSpawn = true;
                break;
            }
        }
    }
    if (!TestTrue(TEXT("Fixture finds an eligible generated Mireling boss"), bFoundBossSpawn)) return false;

    const FUniqueNetIdStringRef OwnerNetId = FUniqueNetIdString::Create(
        TEXT("mireling-scroll-recovery-owner"), FName(TEXT("KalmalaMirelingScrollRecoveryTest")));
    const FUniqueNetIdStringRef ObserverNetId = FUniqueNetIdString::Create(
        TEXT("mireling-scroll-recovery-observer"), FName(TEXT("KalmalaMirelingScrollRecoveryTest")));
    const FString OwnerIdentity = OwnerNetId->GetType().ToString() + TEXT(":") + OwnerNetId->ToString();
    const FString ScrollId = AKalmalaGameMode::GetMirelingBossScrollId(WorldConfig.WorldSeed);
    const FString ScrollDefinition = AKalmalaGameMode::GetMirelingBossScrollDefinition(WorldConfig.WorldSeed);
    const EKalmalaSupportEffect ScrollEffect =
        UKalmalaSupportMagicComponent::FromScrollDefinition(ScrollDefinition);
    const FString EffectId = UKalmalaSupportMagicComponent::CanonicalId(ScrollEffect);
    const FString PendingScrollClaimId =
        UKalmalaWorldPopulationSaveGame::MakeMirelingBossScrollPendingRewardClaimId(BossSpawnId);

    TArray<UWorld*> FixtureWorlds;
    ON_SCOPE_EXIT
    {
        for (UWorld* World : FixtureWorlds)
        {
            if (World == nullptr) continue;
            if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Destroyed);
            World->DestroyWorld(false);
            GEngine->DestroyWorldContext(World);
        }
    };

    struct FScrollFixture
    {
        UWorld* World = nullptr;
        AKalmalaGameMode* GameMode = nullptr;
        AKalmalaCharacter* Owner = nullptr;
        APlayerController* OwnerController = nullptr;
        APlayerState* OwnerState = nullptr;
        AKalmalaCharacter* Observer = nullptr;
        APlayerState* ObserverState = nullptr;
    };

    const auto CreateFixture = [this, &FixtureWorlds, &WorldConfig, &BossSpawn, &OwnerNetId, &ObserverNetId](
        const TCHAR* Label, const TArray<uint8>* PersistedPopulationBytes) -> FScrollFixture
    {
        FScrollFixture Fixture;
        const FString Prefix(Label);
        Fixture.World = UWorld::CreateWorld(EWorldType::Game, false);
        if (!TestNotNull(Prefix + TEXT(" world created"), Fixture.World)) return Fixture;
        FixtureWorlds.Add(Fixture.World);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(Fixture.World);
        Fixture.World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        Fixture.World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
        AKalmalaWorldGenerationGameState* WorldState =
            Fixture.World->SpawnActor<AKalmalaWorldGenerationGameState>();
        if (!TestNotNull(Prefix + TEXT(" world identity state created"), WorldState)) return Fixture;
        Fixture.World->SetGameState(WorldState);
        if (!TestTrue(Prefix + TEXT(" world seed initialized"),
                WorldState->SelectWorldSeedFromServer(WorldConfig.WorldSeed))) return Fixture;
        if (!TestTrue(Prefix + TEXT(" selects the authoritative GameMode"), Fixture.World->SetGameMode(FURL())))
            return Fixture;
        Fixture.GameMode = Fixture.World->GetAuthGameMode<AKalmalaGameMode>();
        if (!TestNotNull(Prefix + TEXT(" GameMode created"), Fixture.GameMode)) return Fixture;
        Fixture.GameMode->WorldGenerationConfig = WorldConfig;
        if (PersistedPopulationBytes != nullptr)
        {
            Fixture.GameMode->PopulationSaveGame = Cast<UKalmalaWorldPopulationSaveGame>(
                UGameplayStatics::LoadGameFromMemory(*PersistedPopulationBytes));
            if (!TestTrue(Prefix + TEXT(" restores the durable defeated-source queue"),
                    Fixture.GameMode->PopulationSaveGame != nullptr
                        && Fixture.GameMode->PopulationSaveGame->MatchesWorld(WorldConfig))) return Fixture;
        }
        else
        {
            Fixture.GameMode->PopulationSaveGame = NewObject<UKalmalaWorldPopulationSaveGame>(Fixture.GameMode);
            if (Fixture.GameMode->PopulationSaveGame != nullptr)
                Fixture.GameMode->PopulationSaveGame->InitializeForWorld(WorldConfig);
        }

        Fixture.OwnerController = Fixture.World->SpawnActor<APlayerController>();
        Fixture.OwnerState = Fixture.World->SpawnActor<APlayerState>();
        Fixture.Owner = Fixture.World->SpawnActor<AKalmalaCharacter>(
            BossSpawn.Location + FVector(100.0, 0.0, 0.0), FRotator::ZeroRotator);
        APlayerController* ObserverController = Fixture.World->SpawnActor<APlayerController>();
        Fixture.ObserverState = Fixture.World->SpawnActor<APlayerState>();
        Fixture.Observer = Fixture.World->SpawnActor<AKalmalaCharacter>(
            BossSpawn.Location + FVector(150.0, 0.0, 0.0), FRotator::ZeroRotator);
        if (!TestNotNull(Prefix + TEXT(" owner controller created"), Fixture.OwnerController)
            || !TestNotNull(Prefix + TEXT(" owner PlayerState created"), Fixture.OwnerState)
            || !TestNotNull(Prefix + TEXT(" owner pawn created"), Fixture.Owner)
            || !TestNotNull(Prefix + TEXT(" observer controller created"), ObserverController)
            || !TestNotNull(Prefix + TEXT(" observer PlayerState created"), Fixture.ObserverState)
            || !TestNotNull(Prefix + TEXT(" observer pawn created"), Fixture.Observer)) return Fixture;

        Fixture.World->AddController(Fixture.OwnerController);
        Fixture.OwnerState->SetUniqueId(FUniqueNetIdRepl(*OwnerNetId));
        Fixture.OwnerState->SetPlayerId(1);
        Fixture.OwnerController->SetPlayerState(Fixture.OwnerState);
        Fixture.OwnerController->Possess(Fixture.Owner);
        Fixture.Owner->SetPlayerState(Fixture.OwnerState);

        Fixture.World->AddController(ObserverController);
        Fixture.ObserverState->SetUniqueId(FUniqueNetIdRepl(*ObserverNetId));
        Fixture.ObserverState->SetPlayerId(2);
        ObserverController->SetPlayerState(Fixture.ObserverState);
        ObserverController->Possess(Fixture.Observer);
        Fixture.Observer->SetPlayerState(Fixture.ObserverState);
        return Fixture;
    };

    TArray<uint8> PopulationBytes;
    int32 FirstWorldPopulationWrites = 0;
    FScrollFixture Initial = CreateFixture(TEXT("Failed player-save load"), nullptr);
    if (!TestNotNull(TEXT("Initial GameMode is ready"), Initial.GameMode)
        || !TestNotNull(TEXT("Initial owner is ready"), Initial.Owner)
        || !TestNotNull(TEXT("Initial observer is ready"), Initial.Observer)) return false;
    Initial.GameMode->PopulationSaveWriteAttemptForTests = [&PopulationBytes, &FirstWorldPopulationWrites](
        UKalmalaWorldPopulationSaveGame* Candidate, const FString&, int32)
    {
        ++FirstWorldPopulationWrites;
        return Candidate != nullptr && UGameplayStatics::SaveGameToMemory(Candidate, PopulationBytes);
    };

    UKalmalaPlayerDiscoverySaveGame* IncompatiblePlayerSave = NewObject<UKalmalaPlayerDiscoverySaveGame>();
    if (!TestNotNull(TEXT("An older player-save fixture is available"), IncompatiblePlayerSave)) return false;
    IncompatiblePlayerSave->InitializeForPlayer(WorldConfig, OwnerIdentity);
    TArray<uint8> IncompatiblePlayerBytes;
    if (!TestTrue(TEXT("Incompatible player slot serializes in memory"),
            UGameplayStatics::SaveGameToMemory(IncompatiblePlayerSave, IncompatiblePlayerBytes))) return false;
    const FString ExpectedOwnerSlot = FString::Printf(TEXT("KalmalaPlayerDiscoveries_%llu_%08x"),
        WorldConfig.WorldSeed, FCrc::StrCrc32(*OwnerIdentity));
    int32 InitialPlayerLoadAttempts = 0;
    int32 InitialPlayerWriteAttempts = 0;
    Initial.GameMode->PlayerDiscoverySaveLoaderForTests = [this, &InitialPlayerLoadAttempts,
        &IncompatiblePlayerBytes, ExpectedOwnerSlot](const FString& Slot, const int32 UserIndex,
            USaveGame*& OutSaveGame)
    {
        ++InitialPlayerLoadAttempts;
        TestEqual(TEXT("Failed scroll recovery reads the exact owner slot"), Slot, ExpectedOwnerSlot);
        TestEqual(TEXT("Failed scroll recovery reads local user zero"), UserIndex, 0);
        if (Slot != ExpectedOwnerSlot || UserIndex != 0) return false;
        OutSaveGame = UGameplayStatics::LoadGameFromMemory(IncompatiblePlayerBytes);
        return OutSaveGame != nullptr;
    };
    Initial.GameMode->PlayerDiscoverySaveWriterForTests = [&InitialPlayerWriteAttempts](
        UKalmalaPlayerDiscoverySaveGameV2*, const FString&, int32)
    {
        ++InitialPlayerWriteAttempts;
        return false;
    };

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaWildlifeSpawn* Mireling = Initial.World->SpawnActor<AKalmalaWildlifeSpawn>(
        AKalmalaWildlifeSpawn::StaticClass(), BossSpawn.Location, FRotator::ZeroRotator, SpawnParameters);
    if (!TestNotNull(TEXT("Eligible Mireling actor created"), Mireling)) return false;
    Mireling->InitializeServer(BossSpawn);
    TestEqual(TEXT("Fixture actor keeps the selected Mireling archetype"),
        Mireling->GetArchetype(), EKalmalaWildlifeArchetype::Mireling);
    TestEqual(TEXT("Fixture actor keeps the eligible persistent identity"),
        Mireling->GetPersistentSpawnId(), BossSpawnId);
    if (UKalmalaResistanceComponent* Resistance = Mireling->FindComponentByClass<UKalmalaResistanceComponent>())
        Resistance->Profile.Armor = 0.0f;
    else
    {
        TestTrue(TEXT("Eligible Mireling has its server resistance component"), false);
        return false;
    }

    const uint32 InitialOwnerFeedbackSerial = Initial.Owner->GetDiscoveryProgressComponent()->GetFeedbackSerial();
    TestTrue(TEXT("Production defeat callback accepts the owner even when player-save load fails"),
        Mireling->ApplyCombatDamageFromServer(100.0f, Initial.Owner));
    TestTrue(TEXT("Mireling remains defeated after the player-save load failure"), Mireling->IsDefeated());
    TestEqual(TEXT("A player-save load failure publishes owner-only unavailable feedback"),
        Initial.Owner->GetDiscoveryProgressComponent()->GetFeedback(), EKalmalaDiscoveryFeedback::Unavailable);
    TestEqual(TEXT("The initial load failure publishes one result"),
        Initial.Owner->GetDiscoveryProgressComponent()->GetFeedbackSerial(), InitialOwnerFeedbackSerial + 1);
    TestEqual(TEXT("An incompatible player slot is never overwritten"), InitialPlayerWriteAttempts, 0);
    TestEqual(TEXT("Failed player load is attempted once before the save is fail-closed"), InitialPlayerLoadAttempts, 1);
    TestEqual(TEXT("The defeated source is durably recorded before its scroll claim"), FirstWorldPopulationWrites, 1);
    TestTrue(TEXT("The persistent queue retains the owner-bound boss scroll"),
        Initial.GameMode->PopulationSaveGame->GetPendingWildlifeRewards().ContainsByPredicate(
            [&PendingScrollClaimId, &OwnerIdentity](const FKalmalaPendingWildlifeReward& Reward)
            {
                return Reward.PersistentSpawnId == PendingScrollClaimId
                    && Reward.PlayerIdentity == OwnerIdentity && Reward.GemItemId.IsNone();
            }));
    TestFalse(TEXT("The failed load records no claimed scroll"),
        Initial.Owner->GetSupportMagicComponent()->HasLearnedEffect(ScrollEffect));
    TestEqual(TEXT("The observer receives no private scroll feedback"),
        Initial.Observer->GetDiscoveryProgressComponent()->GetFeedbackSerial(), 0u);

    const FString GemClaimId = TEXT("mireling-gem|") + BossSpawnId;
    TestTrue(TEXT("The regression isolates boss-scroll recovery from the separate gem reward"),
        Initial.GameMode->PopulationSaveGame->RemovePendingWildlifeReward(GemClaimId));
    TestTrue(TEXT("The isolated defeated-source queue serializes in memory"),
        UGameplayStatics::SaveGameToMemory(Initial.GameMode->PopulationSaveGame, PopulationBytes));

    Initial.GameMode->DeliverPendingWildlifeRewards();
    TestTrue(TEXT("A retryable scroll entry survives repeated incompatible player loads"),
        Initial.GameMode->PopulationSaveGame->GetPendingWildlifeRewards().ContainsByPredicate(
            [&PendingScrollClaimId](const FKalmalaPendingWildlifeReward& Reward)
            { return Reward.PersistentSpawnId == PendingScrollClaimId; }));
    TestEqual(TEXT("The observer still has no learned boss effect"),
        Initial.Observer->GetSupportMagicComponent()->HasLearnedEffect(ScrollEffect), false);
    TestTrue(TEXT("The durable population queue can be reloaded for recovery"), !PopulationBytes.IsEmpty());

    int32 RecoveryPopulationWrites = 0;
    FScrollFixture Recovery = CreateFixture(TEXT("Recovered player-save write"), &PopulationBytes);
    if (!TestNotNull(TEXT("Recovery GameMode is ready"), Recovery.GameMode)
        || !TestNotNull(TEXT("Recovery owner is ready"), Recovery.Owner)
        || !TestNotNull(TEXT("Recovery observer is ready"), Recovery.Observer)) return false;
    Recovery.GameMode->PopulationSaveWriteAttemptForTests = [&PopulationBytes, &RecoveryPopulationWrites](
        UKalmalaWorldPopulationSaveGame* Candidate, const FString&, int32)
    {
        ++RecoveryPopulationWrites;
        return Candidate != nullptr && UGameplayStatics::SaveGameToMemory(Candidate, PopulationBytes);
    };

    TMap<FString, TArray<uint8>> PlayerSaveSlots;
    int32 RecoveryPlayerLoadAttempts = 0;
    int32 RecoveryPlayerWriteAttempts = 0;
    bool bFailNextPlayerWrite = true;
    Recovery.GameMode->PlayerDiscoverySaveLoaderForTests = [this, &PlayerSaveSlots,
        &RecoveryPlayerLoadAttempts, ExpectedOwnerSlot](const FString& Slot, const int32 UserIndex,
            USaveGame*& OutSaveGame)
    {
        ++RecoveryPlayerLoadAttempts;
        TestEqual(TEXT("Recovery loads the exact authenticated owner slot"), Slot, ExpectedOwnerSlot);
        TestEqual(TEXT("Recovery loads local user zero"), UserIndex, 0);
        if (Slot != ExpectedOwnerSlot || UserIndex != 0) return false;
        const TArray<uint8>* SavedBytes = PlayerSaveSlots.Find(Slot);
        if (SavedBytes == nullptr) return false;
        OutSaveGame = UGameplayStatics::LoadGameFromMemory(*SavedBytes);
        return OutSaveGame != nullptr;
    };
    Recovery.GameMode->PlayerDiscoverySaveWriterForTests = [this, &PlayerSaveSlots,
        &RecoveryPlayerWriteAttempts, &bFailNextPlayerWrite, ExpectedOwnerSlot](
            UKalmalaPlayerDiscoverySaveGameV2* Candidate, const FString& Slot, const int32 UserIndex)
    {
        ++RecoveryPlayerWriteAttempts;
        TestEqual(TEXT("Recovery writes the exact authenticated owner slot"), Slot, ExpectedOwnerSlot);
        TestEqual(TEXT("Recovery writes local user zero"), UserIndex, 0);
        if (bFailNextPlayerWrite)
        {
            bFailNextPlayerWrite = false;
            return false;
        }
        if (Candidate == nullptr || Slot != ExpectedOwnerSlot || UserIndex != 0) return false;
        TArray<uint8> SavedBytes;
        if (!UGameplayStatics::SaveGameToMemory(Candidate, SavedBytes)) return false;
        PlayerSaveSlots.Add(Slot, MoveTemp(SavedBytes));
        return true;
    };

    const int32 RecoveryOwnerFeedbackBeforeWriteFailure =
        Recovery.Owner->GetDiscoveryProgressComponent()->GetFeedbackSerial();
    Recovery.GameMode->DeliverPendingWildlifeRewards();
    TestEqual(TEXT("Injected player-save write failure publishes unavailable to the owner"),
        Recovery.Owner->GetDiscoveryProgressComponent()->GetFeedback(), EKalmalaDiscoveryFeedback::Unavailable);
    TestEqual(TEXT("Injected player-save write failure publishes one owner result"),
        Recovery.Owner->GetDiscoveryProgressComponent()->GetFeedbackSerial(),
        RecoveryOwnerFeedbackBeforeWriteFailure + 1);
    TestEqual(TEXT("Injected player-save writer fails exactly once before recovery"),
        RecoveryPlayerWriteAttempts, 1);
    TestEqual(TEXT("Recovery loads the player's slot once before retrying its write"),
        RecoveryPlayerLoadAttempts, 1);
    TestFalse(TEXT("Failed player-save write does not deliver the learned effect"),
        Recovery.Owner->GetSupportMagicComponent()->HasLearnedEffect(ScrollEffect));
    TestTrue(TEXT("Failed player-save write preserves the pending owner claim"),
        Recovery.GameMode->PopulationSaveGame->GetPendingWildlifeRewards().ContainsByPredicate(
            [&PendingScrollClaimId](const FKalmalaPendingWildlifeReward& Reward)
            { return Reward.PersistentSpawnId == PendingScrollClaimId; }));

    Recovery.GameMode->DeliverPendingWildlifeRewards();
    TestEqual(TEXT("Recovered persistence publishes the canonical scroll result"),
        Recovery.Owner->GetDiscoveryProgressComponent()->GetFeedback(), EKalmalaDiscoveryFeedback::ScrollFound);
    TestEqual(TEXT("Scroll result follows the single unavailable response"),
        Recovery.Owner->GetDiscoveryProgressComponent()->GetFeedbackSerial(),
        RecoveryOwnerFeedbackBeforeWriteFailure + 2);
    TestTrue(TEXT("Recovered owner learns the canonical world effect"),
        Recovery.Owner->GetSupportMagicComponent()->HasLearnedEffect(ScrollEffect));
    TestFalse(TEXT("Observer receives no private scroll feedback"),
        Recovery.Observer->GetDiscoveryProgressComponent()->GetFeedbackSerial() != 0);
    TestFalse(TEXT("Observer does not learn the private boss effect"),
        Recovery.Observer->GetSupportMagicComponent()->HasLearnedEffect(ScrollEffect));
    TestEqual(TEXT("One failed and one recovered write are attempted"), RecoveryPlayerWriteAttempts, 2);
    TestTrue(TEXT("Durable owner save contains the world-canonical scroll claim and learned effect"),
        PlayerSaveSlots.Contains(ExpectedOwnerSlot));
    UKalmalaPlayerDiscoverySaveGameV2* PersistedOwnerSave = Cast<UKalmalaPlayerDiscoverySaveGameV2>(
        UGameplayStatics::LoadGameFromMemory(PlayerSaveSlots.FindChecked(ExpectedOwnerSlot)));
    if (!TestNotNull(TEXT("Recovered player save reloads as the current schema"), PersistedOwnerSave)) return false;
    TestTrue(TEXT("Recovered save binds the canonical entitlement to its authenticated owner"),
        PersistedOwnerSave->MatchesPlayer(WorldConfig, OwnerIdentity)
            && PersistedOwnerSave->HasDiscovery(ScrollId)
            && PersistedOwnerSave->HasLearnedEffect(EffectId));
    TestFalse(TEXT("Durable scroll queue is cleared only after player persistence"),
        Recovery.GameMode->PopulationSaveGame->GetPendingWildlifeRewards().ContainsByPredicate(
            [&PendingScrollClaimId](const FKalmalaPendingWildlifeReward& Reward)
            { return Reward.PersistentSpawnId == PendingScrollClaimId; }));

    const int32 WritesBeforeReplay = RecoveryPlayerWriteAttempts + RecoveryPopulationWrites;
    const int32 LearnedEffectCount = PersistedOwnerSave->GetLearnedEffectIds().Num();
    Recovery.GameMode->DeliverPendingWildlifeRewards();
    TestEqual(TEXT("Completed queue replay grants no additional learned state"),
        PersistedOwnerSave->GetLearnedEffectIds().Num(), LearnedEffectCount);
    TestEqual(TEXT("Completed queue replay writes no further player or population saves"),
        RecoveryPlayerWriteAttempts + RecoveryPopulationWrites, WritesBeforeReplay);

    Recovery.OwnerController->UnPossess();
    Recovery.Owner->Destroy();
    AKalmalaCharacter* ReconnectedOwner = Recovery.World->SpawnActor<AKalmalaCharacter>(
        BossSpawn.Location + FVector(100.0, 0.0, 0.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Authenticated owner pawn can reconnect in the recovered world"), ReconnectedOwner)) return false;
    ReconnectedOwner->SetPlayerState(Recovery.OwnerState);
    Recovery.OwnerController->Possess(ReconnectedOwner);
    FString ReconnectedIdentity;
    UKalmalaPlayerDiscoverySaveGameV2* ReconnectedSave =
        Recovery.GameMode->GetPlayerDiscoverySave(ReconnectedOwner, ReconnectedIdentity);
    TestTrue(TEXT("Same-session reconnect reuses the exact owner entitlement"),
        ReconnectedSave != nullptr && ReconnectedIdentity == OwnerIdentity
            && ReconnectedSave->HasDiscovery(ScrollId));
    TestTrue(TEXT("Same-session reconnect reapplies its saved support effect"),
        ReconnectedOwner->GetSupportMagicComponent()->HasLearnedEffect(ScrollEffect));
    TestFalse(TEXT("Observer never receives the recovered entitlement"),
        Recovery.Observer->GetSupportMagicComponent()->HasLearnedEffect(ScrollEffect));
    return true;
}

#endif
