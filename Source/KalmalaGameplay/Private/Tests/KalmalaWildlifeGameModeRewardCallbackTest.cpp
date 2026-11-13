#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"

namespace
{
struct FDefeatBroadcastObservation
{
    int32 BroadcastCount = 0;
    int32 ExpectedWritesBeforeBroadcast = 0;
    bool bSaveWasCommittedBeforeBroadcast = false;
};

bool TryDecodeClaim(const FString& ClaimId, const EKalmalaWildlifeArchetype Archetype,
    FString& OutSpawnId, bool& bOutOptionalOutcome)
{
    bOutOptionalOutcome = false;
    switch (Archetype)
    {
    case EKalmalaWildlifeArchetype::Wolf:
        return UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bOutOptionalOutcome);
    case EKalmalaWildlifeArchetype::Lynx:
        return UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bOutOptionalOutcome);
    case EKalmalaWildlifeArchetype::PolarBear:
        return UKalmalaWorldPopulationSaveGame::TryGetPolarBearSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId);
    case EKalmalaWildlifeArchetype::HornedGoat:
        return UKalmalaWorldPopulationSaveGame::TryGetHornedGoatSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId);
    case EKalmalaWildlifeArchetype::ThunderSentinel:
        return UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bOutOptionalOutcome);
    case EKalmalaWildlifeArchetype::ReefMaw:
        return UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bOutOptionalOutcome);
    default:
        return false;
    }
}

FString MakeClaimId(const EKalmalaWildlifeArchetype Archetype, const FString& SpawnId)
{
    switch (Archetype)
    {
    case EKalmalaWildlifeArchetype::Wolf:
        return UKalmalaWorldPopulationSaveGame::MakeWolfPendingRewardClaimId(SpawnId, false);
    case EKalmalaWildlifeArchetype::Lynx:
        return UKalmalaWorldPopulationSaveGame::MakeLynxPendingRewardClaimId(SpawnId, false);
    case EKalmalaWildlifeArchetype::PolarBear:
        return UKalmalaWorldPopulationSaveGame::MakePolarBearPendingRewardClaimId(SpawnId);
    case EKalmalaWildlifeArchetype::HornedGoat:
        return UKalmalaWorldPopulationSaveGame::MakeHornedGoatPendingRewardClaimId(SpawnId);
    case EKalmalaWildlifeArchetype::ThunderSentinel:
        return UKalmalaWorldPopulationSaveGame::MakeThunderSentinelPendingRewardClaimId(SpawnId, false);
    case EKalmalaWildlifeArchetype::ReefMaw:
        return UKalmalaWorldPopulationSaveGame::MakeReefMawPendingRewardClaimId(SpawnId, false);
    default:
        return FString();
    }
}

int32 CountClaimsForSpawn(const UKalmalaWorldPopulationSaveGame* Save,
    const EKalmalaWildlifeArchetype Archetype, const FString& SpawnId)
{
    if (Save == nullptr) return 0;
    int32 Count = 0;
    for (const FKalmalaPendingWildlifeReward& Claim : Save->GetPendingWildlifeRewards())
    {
        FString DecodedSpawnId;
        bool bOptionalOutcome = false;
        if (TryDecodeClaim(Claim.PersistentSpawnId, Archetype, DecodedSpawnId, bOptionalOutcome)
            && DecodedSpawnId == SpawnId)
        {
            ++Count;
        }
    }
    return Count;
}

bool FindClaimForSpawn(const UKalmalaWorldPopulationSaveGame* Save,
    const EKalmalaWildlifeArchetype Archetype, const FString& SpawnId,
    FKalmalaPendingWildlifeReward& OutClaim)
{
    if (Save == nullptr) return false;
    for (const FKalmalaPendingWildlifeReward& Claim : Save->GetPendingWildlifeRewards())
    {
        FString DecodedSpawnId;
        bool bOptionalOutcome = false;
        if (TryDecodeClaim(Claim.PersistentSpawnId, Archetype, DecodedSpawnId, bOptionalOutcome)
            && DecodedSpawnId == SpawnId)
        {
            OutClaim = Claim;
            return true;
        }
    }
    return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeGameModeRewardCallbackTest,
    "Kalmala.Gameplay.Wildlife.GameModeRewardCallbacks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeGameModeRewardCallbackTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Wildlife callback fixture world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Wildlife callback fixture world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("Wildlife callback fixture selects a stable seed"),
            WorldState->SelectWorldSeedFromServer(19418))) return false;
    const FKalmalaWorldGenerationConfig Config = WorldState->GetWorldGenerationConfig();

    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Wildlife callback fixture installs its authoritative GameMode"),
            World->SetGameMode(FURL()))) return false;
    AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Wildlife callback fixture GameMode is available"), Mode)) return false;
    Mode->WorldGenerationConfig = Config;
    UKalmalaWorldPopulationSaveGame* InitialSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Wildlife callback fixture population save created"), InitialSave)) return false;
    InitialSave->InitializeForWorld(Config);
    Mode->PopulationSaveGame = InitialSave;

    AKalmalaCharacter* Attacker = World->SpawnActor<AKalmalaCharacter>(FVector::ZeroVector,
        FRotator::ZeroRotator);
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Wildlife callback owner character created"), Attacker)
        || !TestNotNull(TEXT("Wildlife callback owner PlayerState created"), PlayerState)) return false;
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("wildlife-callback-owner"), FName(TEXT("KalmalaWildlifeCallbackTest")));
    const FUniqueNetIdRef BaseNetId = NetId;
    PlayerState->SetUniqueId(FUniqueNetIdRepl(BaseNetId));
    Attacker->SetPlayerState(PlayerState);
    const FString OwnerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    if (!TestFalse(TEXT("Authenticated callback owner has a stable identity"), OwnerIdentity.IsEmpty())) return false;

    TArray<uint8> DurablePopulationBytes;
    if (!TestTrue(TEXT("Initial population save serializes in memory"),
            UGameplayStatics::SaveGameToMemory(InitialSave, DurablePopulationBytes))) return false;
    int32 PopulationWriteAttempts = 0;
    Mode->PopulationSaveWriteAttemptForTests = [&DurablePopulationBytes, &PopulationWriteAttempts](
        UKalmalaWorldPopulationSaveGame* Candidate, const FString&, int32)
    {
        ++PopulationWriteAttempts;
        return UGameplayStatics::SaveGameToMemory(Candidate, DurablePopulationBytes);
    };

    const auto SpawnCreature = [this, World](const EKalmalaWildlifeArchetype Archetype,
        const int32 Slot, AKalmalaCharacter* CreditedAttacker) -> AKalmalaWildlifeSpawn*
    {
        const FVector SpawnLocation(static_cast<double>(Slot) * 1000.0, 0.0, 500.0);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
            AKalmalaWildlifeSpawn::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
        if (Creature == nullptr)
        {
            TestTrue(TEXT("Wildlife callback actor created"), false);
            return nullptr;
        }

        FKalmalaWorldPopulationSpawn Descriptor;
        Descriptor.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Descriptor.SpatialKey = FIntPoint(Slot, 0);
        Descriptor.SpawnSeed = 19418ull + static_cast<uint64>(Slot);
        Descriptor.Location = SpawnLocation;
        Descriptor.ContentId = FName(TEXT("wildlife-game-mode-callback"));
        Creature->InitializeServer(Descriptor, Archetype);
        Creature->Health = 1.0f;
        if (UKalmalaResistanceComponent* Resistance = Creature->FindComponentByClass<UKalmalaResistanceComponent>())
            Resistance->Profile.Armor = 0.0f;
        else
        {
            TestTrue(TEXT("Wildlife callback actor resistance component exists"), false);
            return nullptr;
        }
        if (CreditedAttacker != nullptr)
            CreditedAttacker->SetActorLocation(Creature->GetActorLocation() + FVector(100.0, 0.0, 0.0));
        return Creature;
    };

    const auto BindSaveOrderObserver = [this, Mode, &DurablePopulationBytes, &PopulationWriteAttempts](
        AKalmalaWildlifeSpawn* Creature, const EKalmalaWildlifeArchetype Archetype,
        const FString& SpawnId, const FString& SpeciesName, const FString& ExpectedOwner,
        const bool bExpectClaim, const int32 ExpectedWritesBeforeBroadcast)
        -> TSharedRef<FDefeatBroadcastObservation>
    {
        TSharedRef<FDefeatBroadcastObservation> Observation = MakeShared<FDefeatBroadcastObservation>();
        Observation->ExpectedWritesBeforeBroadcast = ExpectedWritesBeforeBroadcast;
        Creature->OnDefeated.AddLambda([this, Observation, &DurablePopulationBytes,
            &PopulationWriteAttempts, Archetype, SpawnId, SpeciesName, ExpectedOwner,
            bExpectClaim](const FString& BroadcastSpawnId)
        {
            ++Observation->BroadcastCount;
            UKalmalaWorldPopulationSaveGame* Durable = Cast<UKalmalaWorldPopulationSaveGame>(
                UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
            FKalmalaPendingWildlifeReward Claim;
            const bool bHasExpectedClaim = FindClaimForSpawn(Durable, Archetype, SpawnId, Claim)
                && (ExpectedOwner.IsEmpty() || Claim.PlayerIdentity == ExpectedOwner);
            const bool bClaimsMatch = bExpectClaim ? bHasExpectedClaim
                : CountClaimsForSpawn(Durable, Archetype, SpawnId) == 0;
            Observation->bSaveWasCommittedBeforeBroadcast = BroadcastSpawnId == SpawnId
                && PopulationWriteAttempts == Observation->ExpectedWritesBeforeBroadcast
                && Durable != nullptr && Durable->IsDefeated(SpawnId) && bClaimsMatch;
            TestTrue(FString::Printf(TEXT("%s writes its defeated ID and reward state before OnDefeated"),
                *SpeciesName), Observation->bSaveWasCommittedBeforeBroadcast);
        });
        Creature->OnDefeated.AddUObject(Mode, &AKalmalaGameMode::RecordDefeatedSpawn);
        return Observation;
    };

    const auto RepeatCallback = [Mode](const EKalmalaWildlifeArchetype Archetype,
        const FString& SpawnId)
    {
        switch (Archetype)
        {
        case EKalmalaWildlifeArchetype::Wolf: return Mode->RecordWolfDefeatReward(SpawnId, nullptr);
        case EKalmalaWildlifeArchetype::Lynx: return Mode->RecordLynxDefeatReward(SpawnId, nullptr);
        case EKalmalaWildlifeArchetype::PolarBear: return Mode->RecordPolarBearDefeatReward(SpawnId, nullptr);
        case EKalmalaWildlifeArchetype::HornedGoat: return Mode->RecordHornedGoatDefeatReward(SpawnId, nullptr);
        case EKalmalaWildlifeArchetype::ThunderSentinel: return Mode->RecordThunderSentinelDefeatReward(SpawnId, nullptr);
        case EKalmalaWildlifeArchetype::ReefMaw: return Mode->RecordReefMawDefeatReward(SpawnId, nullptr);
        default: return false;
        }
    };

    struct FSpeciesCase
    {
        EKalmalaWildlifeArchetype Archetype;
        const TCHAR* Name;
    };
    const FSpeciesCase SpeciesCases[] = {
        { EKalmalaWildlifeArchetype::Wolf, TEXT("Wolf") },
        { EKalmalaWildlifeArchetype::Lynx, TEXT("Lynx") },
        { EKalmalaWildlifeArchetype::PolarBear, TEXT("Polar Bear") },
        { EKalmalaWildlifeArchetype::HornedGoat, TEXT("Horned Goat") },
        { EKalmalaWildlifeArchetype::ThunderSentinel, TEXT("Thunder Sentinel") },
        { EKalmalaWildlifeArchetype::ReefMaw, TEXT("Reef Maw") }
    };

    int32 Slot = 1;
    for (const FSpeciesCase& Species : SpeciesCases)
    {
        AKalmalaWildlifeSpawn* Creature = SpawnCreature(Species.Archetype, Slot++, Attacker);
        if (!TestNotNull(FString::Printf(TEXT("%s callback actor created"), Species.Name), Creature)) return false;
        const FString SpawnId = Creature->GetPersistentSpawnId();
        if (!TestTrue(FString::Printf(TEXT("%s callback has a stable actor identity"), Species.Name),
                !SpawnId.IsEmpty())) return false;
        if (Species.Archetype == EKalmalaWildlifeArchetype::Wolf)
            TestTrue(TEXT("Wolf callback uses the exact first pack-member identity"),
                SpawnId.EndsWith(TEXT("/wolf/0")));

        const FString ConflictingClaimId = MakeClaimId(Species.Archetype, SpawnId);
        if (!TestTrue(FString::Printf(TEXT("%s conflicting claim ID is valid"), Species.Name),
                !ConflictingClaimId.IsEmpty())
            || !TestTrue(FString::Printf(TEXT("%s starts with a conflicting owner-bound claim"), Species.Name),
                Mode->PopulationSaveGame->TryQueuePendingWildlifeReward(
                    ConflictingClaimId, TEXT("other-player:reserved")))) return false;
        TArray<uint8> QueueRejectLiveBytes;
        if (!TestTrue(FString::Printf(TEXT("%s queue-rejection baseline serializes"), Species.Name),
                UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, QueueRejectLiveBytes))
            || !TestTrue(FString::Printf(TEXT("%s queue-rejection durable baseline serializes"), Species.Name),
                UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, DurablePopulationBytes))) return false;
        const TArray<uint8> QueueRejectDurableBytes = DurablePopulationBytes;
        const int32 WritesBeforeQueueReject = PopulationWriteAttempts;
        const TSharedRef<FDefeatBroadcastObservation> Observation = BindSaveOrderObserver(
            Creature, Species.Archetype, SpawnId, FString(Species.Name), OwnerIdentity, true,
            WritesBeforeQueueReject + 1);
        TestFalse(FString::Printf(TEXT("%s owner-mismatch queue rejects the lethal defeat"), Species.Name),
            Creature->ApplyCombatDamageFromServer(100.0f, Attacker));
        TestFalse(FString::Printf(TEXT("%s queue rejection does not accept the actor"), Species.Name),
            Creature->IsDefeated());
        TestTrue(FString::Printf(TEXT("%s queue rejection restores health"), Species.Name),
            Creature->GetHealth() > 0.0f);
        TestEqual(FString::Printf(TEXT("%s queue rejection does not broadcast defeat"), Species.Name),
            Observation->BroadcastCount, 0);
        TestEqual(FString::Printf(TEXT("%s queue rejection performs no slot write"), Species.Name),
            PopulationWriteAttempts, WritesBeforeQueueReject);
        TArray<uint8> QueueRejectLiveAfter;
        TestTrue(FString::Printf(TEXT("%s live state serializes after queue rejection"), Species.Name),
            UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, QueueRejectLiveAfter));
        TestTrue(FString::Printf(TEXT("%s queue rejection leaves live state unchanged"), Species.Name),
            QueueRejectLiveAfter == QueueRejectLiveBytes);
        TestTrue(FString::Printf(TEXT("%s queue rejection leaves durable bytes unchanged"), Species.Name),
            DurablePopulationBytes == QueueRejectDurableBytes);
        TestFalse(FString::Printf(TEXT("%s queue rejection adds no defeated ID"), Species.Name),
            Mode->PopulationSaveGame->IsDefeated(SpawnId));
        UKalmalaWorldPopulationSaveGame* QueueRejectReload = Cast<UKalmalaWorldPopulationSaveGame>(
            UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
        if (!TestNotNull(FString::Printf(TEXT("%s queue-rejection snapshot reloads"), Species.Name),
                QueueRejectReload)) return false;
        TestFalse(FString::Printf(TEXT("%s rejected queue does not persist a defeated ID"), Species.Name),
            QueueRejectReload->IsDefeated(SpawnId));
        TestEqual(FString::Printf(TEXT("%s rejected queue retains exactly its original claim"), Species.Name),
            CountClaimsForSpawn(QueueRejectReload, Species.Archetype, SpawnId), 1);
        FKalmalaPendingWildlifeReward ConflictingClaim;
        TestTrue(FString::Printf(TEXT("%s queue rejection preserves only the conflicting owner's claim"),
            Species.Name), FindClaimForSpawn(QueueRejectReload, Species.Archetype, SpawnId, ConflictingClaim));
        TestEqual(FString::Printf(TEXT("%s claim remains bound to its original owner"), Species.Name),
            ConflictingClaim.PlayerIdentity, FString(TEXT("other-player:reserved")));

        TestTrue(FString::Printf(TEXT("%s conflicting claim can be cleared before retry"), Species.Name),
            Mode->PopulationSaveGame->RemovePendingWildlifeReward(ConflictingClaimId));
        if (!TestTrue(FString::Printf(TEXT("%s retry baseline writes through the isolated seam"), Species.Name),
                Mode->WritePopulationSaveToSlot())) return false;

        if (Species.Archetype == EKalmalaWildlifeArchetype::Lynx)
        {
            const FKalmalaWorldGenerationConfig OriginalConfig = Mode->WorldGenerationConfig;
            const int32 WritesBeforeWorldReject = PopulationWriteAttempts;
            TArray<uint8> WorldRejectLiveBytes;
            if (!TestTrue(TEXT("Lynx world-mismatch baseline serializes"),
                    UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, WorldRejectLiveBytes))) return false;
            const TArray<uint8> WorldRejectDurableBytes = DurablePopulationBytes;
            Mode->WorldGenerationConfig.WorldSeed = OriginalConfig.WorldSeed + 1;
            TestFalse(TEXT("Lynx callback rejects a population save from another world"),
                Creature->ApplyCombatDamageFromServer(100.0f, Attacker));
            Mode->WorldGenerationConfig = OriginalConfig;
            TestFalse(TEXT("Lynx world rejection leaves the actor undefeated"), Creature->IsDefeated());
            TestTrue(TEXT("Lynx world rejection restores actor health"), Creature->GetHealth() > 0.0f);
            TestEqual(TEXT("Lynx world rejection does not broadcast defeat"), Observation->BroadcastCount, 0);
            TestEqual(TEXT("Lynx world rejection performs no slot write"),
                PopulationWriteAttempts, WritesBeforeWorldReject);
            TArray<uint8> WorldRejectLiveAfter;
            TestTrue(TEXT("Lynx live state serializes after world rejection"),
                UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, WorldRejectLiveAfter));
            TestTrue(TEXT("Lynx world rejection leaves live save unchanged"),
                WorldRejectLiveAfter == WorldRejectLiveBytes);
            TestTrue(TEXT("Lynx world rejection leaves durable bytes unchanged"),
                DurablePopulationBytes == WorldRejectDurableBytes);
            TestFalse(TEXT("Lynx world rejection does not add a defeated ID"),
                Mode->PopulationSaveGame->IsDefeated(SpawnId));
            TestEqual(TEXT("Lynx world rejection creates no reward claim"),
                CountClaimsForSpawn(Mode->PopulationSaveGame, Species.Archetype, SpawnId), 0);
            UKalmalaWorldPopulationSaveGame* WorldRejectReload = Cast<UKalmalaWorldPopulationSaveGame>(
                UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
            if (!TestNotNull(TEXT("Lynx world-rejection snapshot reloads"), WorldRejectReload)) return false;
            TestFalse(TEXT("Lynx world rejection leaves the durable defeated ledger unchanged"),
                WorldRejectReload->IsDefeated(SpawnId));
            TestEqual(TEXT("Lynx world rejection leaves the durable claim queue unchanged"),
                CountClaimsForSpawn(WorldRejectReload, Species.Archetype, SpawnId), 0);
        }

        const int32 WritesBeforeAccept = PopulationWriteAttempts;
        Observation->ExpectedWritesBeforeBroadcast = WritesBeforeAccept + 1;
        TestTrue(FString::Printf(TEXT("%s actual DefeatServer dispatch accepts a valid owner"), Species.Name),
            Creature->ApplyCombatDamageFromServer(100.0f, Attacker));
        TestTrue(FString::Printf(TEXT("%s accepts the species-specific defeat"), Species.Name),
            Creature->IsDefeated());
        TestEqual(FString::Printf(TEXT("%s broadcasts accepted defeat exactly once"), Species.Name),
            Observation->BroadcastCount, 1);
        TestTrue(FString::Printf(TEXT("%s reward save is durable before defeat broadcast"), Species.Name),
            Observation->bSaveWasCommittedBeforeBroadcast);
        TestEqual(FString::Printf(TEXT("%s writes reward candidate then generic callback"), Species.Name),
            PopulationWriteAttempts, WritesBeforeAccept + 2);
        TestTrue(FString::Printf(TEXT("%s live save contains its defeated ID"), Species.Name),
            Mode->PopulationSaveGame->IsDefeated(SpawnId));
        TestEqual(FString::Printf(TEXT("%s live save contains one owner-bound claim"), Species.Name),
            CountClaimsForSpawn(Mode->PopulationSaveGame, Species.Archetype, SpawnId), 1);
        FKalmalaPendingWildlifeReward Claim;
        if (!TestTrue(FString::Printf(TEXT("%s claim can be decoded to its exact stable ID"), Species.Name),
                FindClaimForSpawn(Mode->PopulationSaveGame, Species.Archetype, SpawnId, Claim))) return false;
        TestEqual(FString::Printf(TEXT("%s claim owner is the authenticated attacker"), Species.Name),
            Claim.PlayerIdentity, OwnerIdentity);
        FString DecodedSpawnId;
        bool bStoredOptionalOutcome = false;
        TestTrue(FString::Printf(TEXT("%s claim stores a valid stable member identity and outcome"), Species.Name),
            TryDecodeClaim(Claim.PersistentSpawnId, Species.Archetype, DecodedSpawnId, bStoredOptionalOutcome));
        TestEqual(FString::Printf(TEXT("%s claim uses the actor's exact persistent identity"), Species.Name),
            DecodedSpawnId, SpawnId);

        UKalmalaWorldPopulationSaveGame* DurableReload = Cast<UKalmalaWorldPopulationSaveGame>(
            UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
        if (!TestNotNull(FString::Printf(TEXT("%s durable population save reloads"), Species.Name),
                DurableReload)) return false;
        TestTrue(FString::Printf(TEXT("%s durable save retains its defeated ID"), Species.Name),
            DurableReload->IsDefeated(SpawnId));
        TestEqual(FString::Printf(TEXT("%s durable save retains exactly one claim"), Species.Name),
            CountClaimsForSpawn(DurableReload, Species.Archetype, SpawnId), 1);
        FKalmalaPendingWildlifeReward DurableClaim;
        TestTrue(FString::Printf(TEXT("%s durable claim reloads"), Species.Name),
            FindClaimForSpawn(DurableReload, Species.Archetype, SpawnId, DurableClaim));
        TestEqual(FString::Printf(TEXT("%s durable claim retains its owner"), Species.Name),
            DurableClaim.PlayerIdentity, OwnerIdentity);
        TestEqual(FString::Printf(TEXT("%s durable claim retains the encoded roll outcome"), Species.Name),
            DurableClaim.PersistentSpawnId, Claim.PersistentSpawnId);

        TestTrue(FString::Printf(TEXT("%s direct repeat remains idempotent"), Species.Name),
            RepeatCallback(Species.Archetype, SpawnId));
        FKalmalaPendingWildlifeReward RepeatedClaim;
        TestTrue(FString::Printf(TEXT("%s repeat retains its original claim"), Species.Name),
            FindClaimForSpawn(Mode->PopulationSaveGame, Species.Archetype, SpawnId, RepeatedClaim));
        TestEqual(FString::Printf(TEXT("%s repeat does not reassign its owner"), Species.Name),
            RepeatedClaim.PlayerIdentity, Claim.PlayerIdentity);
        TestEqual(FString::Printf(TEXT("%s repeat does not reroll its optional outcome"), Species.Name),
            RepeatedClaim.PersistentSpawnId, Claim.PersistentSpawnId);
        TestEqual(FString::Printf(TEXT("%s repeat does not duplicate its claim"), Species.Name),
            CountClaimsForSpawn(Mode->PopulationSaveGame, Species.Archetype, SpawnId), 1);
    }

    APlayerState* UnauthenticatedState = World->SpawnActor<APlayerState>();
    AKalmalaCharacter* UnauthenticatedAttacker = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Unauthenticated test PlayerState created"), UnauthenticatedState)
        || !TestNotNull(TEXT("Unauthenticated test attacker created"), UnauthenticatedAttacker)) return false;
    TestFalse(TEXT("Unauthenticated PlayerState has no stable owner identity"),
        UnauthenticatedState->GetUniqueId().IsValid());
    UnauthenticatedAttacker->SetPlayerState(UnauthenticatedState);
    AKalmalaWildlifeSpawn* UncreditedGoat = SpawnCreature(
        EKalmalaWildlifeArchetype::HornedGoat, Slot, UnauthenticatedAttacker);
    if (!TestNotNull(TEXT("Uncredited Horned Goat actor created"), UncreditedGoat)) return false;
    const FString UncreditedSpawnId = UncreditedGoat->GetPersistentSpawnId();
    const TSharedRef<FDefeatBroadcastObservation> UncreditedObservation = BindSaveOrderObserver(
        UncreditedGoat, EKalmalaWildlifeArchetype::HornedGoat, UncreditedSpawnId,
        TEXT("Uncredited Horned Goat"), FString(), false, PopulationWriteAttempts + 1);
    TestTrue(TEXT("An unauthenticated attacker can defeat without creating a misowned claim"),
        UncreditedGoat->ApplyCombatDamageFromServer(100.0f, UnauthenticatedAttacker));
    TestEqual(TEXT("Uncredited defeat broadcasts exactly once"), UncreditedObservation->BroadcastCount, 1);
    TestTrue(TEXT("Uncredited defeat persists its stable defeated ID"),
        Mode->PopulationSaveGame->IsDefeated(UncreditedSpawnId));
    TestEqual(TEXT("Uncredited defeat creates no owner-bound claim"),
        CountClaimsForSpawn(Mode->PopulationSaveGame, EKalmalaWildlifeArchetype::HornedGoat, UncreditedSpawnId), 0);
    TestTrue(TEXT("Uncredited defeat still persists before broadcasting"),
        UncreditedObservation->bSaveWasCommittedBeforeBroadcast);
    UKalmalaWorldPopulationSaveGame* UncreditedReload = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Uncredited defeat snapshot reloads"), UncreditedReload)) return false;
    TestTrue(TEXT("Uncredited defeat persists its stable ID to the durable save"),
        UncreditedReload->IsDefeated(UncreditedSpawnId));
    TestEqual(TEXT("Uncredited durable save contains no owner-bound claim"),
        CountClaimsForSpawn(UncreditedReload, EKalmalaWildlifeArchetype::HornedGoat, UncreditedSpawnId), 0);

    return true;
}

#endif
