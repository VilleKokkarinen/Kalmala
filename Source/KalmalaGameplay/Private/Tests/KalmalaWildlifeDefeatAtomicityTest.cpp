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
bool TryDecodeDefeatClaim(const FString& ClaimId, const EKalmalaWildlifeArchetype Archetype,
    FString& OutSpawnId)
{
    bool bStoredOutcome = false;
    switch (Archetype)
    {
    case EKalmalaWildlifeArchetype::Rat:
        OutSpawnId = ClaimId;
        return !OutSpawnId.IsEmpty();
    case EKalmalaWildlifeArchetype::Wolf:
        return UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bStoredOutcome);
    case EKalmalaWildlifeArchetype::Lynx:
        return UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bStoredOutcome);
    case EKalmalaWildlifeArchetype::PolarBear:
        return UKalmalaWorldPopulationSaveGame::TryGetPolarBearSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId);
    case EKalmalaWildlifeArchetype::HornedGoat:
        return UKalmalaWorldPopulationSaveGame::TryGetHornedGoatSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId);
    case EKalmalaWildlifeArchetype::ThunderSentinel:
        return UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bStoredOutcome);
    case EKalmalaWildlifeArchetype::ReefMaw:
        return UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            ClaimId, OutSpawnId, bStoredOutcome);
    default:
        return false;
    }
}

int32 CountDefeatClaims(const UKalmalaWorldPopulationSaveGame* Save,
    const EKalmalaWildlifeArchetype Archetype, const FString& SpawnId)
{
    if (Save == nullptr) return 0;
    int32 Count = 0;
    for (const FKalmalaPendingWildlifeReward& Claim : Save->GetPendingWildlifeRewards())
    {
        FString DecodedSpawnId;
        if (TryDecodeDefeatClaim(Claim.PersistentSpawnId, Archetype, DecodedSpawnId)
            && DecodedSpawnId == SpawnId)
        {
            ++Count;
        }
    }
    return Count;
}

bool FindDefeatClaim(const UKalmalaWorldPopulationSaveGame* Save,
    const EKalmalaWildlifeArchetype Archetype, const FString& SpawnId,
    FKalmalaPendingWildlifeReward& OutClaim)
{
    if (Save == nullptr) return false;
    for (const FKalmalaPendingWildlifeReward& Claim : Save->GetPendingWildlifeRewards())
    {
        FString DecodedSpawnId;
        if (TryDecodeDefeatClaim(Claim.PersistentSpawnId, Archetype, DecodedSpawnId)
            && DecodedSpawnId == SpawnId)
        {
            OutClaim = Claim;
            return true;
        }
    }
    return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeDefeatAtomicityTest,
    "Kalmala.Gameplay.Wildlife.DefeatCallbackAtomicity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaWildlifeDefeatAtomicityTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Wildlife defeat fixture world created"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Wildlife defeat fixture world state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    if (!TestTrue(TEXT("Wildlife defeat fixture selects a stable world seed"),
            WorldState->SelectWorldSeedFromServer(9417))) return false;
    const FKalmalaWorldGenerationConfig Config = WorldState->GetWorldGenerationConfig();

    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!TestTrue(TEXT("Wildlife defeat fixture installs its authoritative GameMode"),
            World->SetGameMode(FURL()))) return false;
    AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Wildlife defeat fixture GameMode is available"), Mode)) return false;
    Mode->WorldGenerationConfig = Config;
    UKalmalaWorldPopulationSaveGame* InitialSave = NewObject<UKalmalaWorldPopulationSaveGame>(Mode);
    if (!TestNotNull(TEXT("Wildlife defeat fixture population save created"), InitialSave)) return false;
    InitialSave->InitializeForWorld(Config);
    Mode->PopulationSaveGame = InitialSave;

    AKalmalaCharacter* Attacker = World->SpawnActor<AKalmalaCharacter>(FVector::ZeroVector,
        FRotator::ZeroRotator);
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Wildlife defeat owner character created"), Attacker)
        || !TestNotNull(TEXT("Wildlife defeat owner PlayerState created"), PlayerState)) return false;
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        TEXT("wildlife-defeat-atomicity-owner"), FName(TEXT("KalmalaDefeatAtomicity")));
    const FUniqueNetIdRef BaseNetId = NetId;
    PlayerState->SetUniqueId(FUniqueNetIdRepl(BaseNetId));
    PlayerState->SetPlayerId(1);
    Attacker->SetPlayerState(PlayerState);
    const FString OwnerIdentity = NetId->GetType().ToString() + TEXT(":") + NetId->ToString();
    if (!TestTrue(TEXT("Wildlife defeat fixture has an authenticated owner"), !OwnerIdentity.IsEmpty())) return false;

    TArray<uint8> DurablePopulationBytes;
    if (!TestTrue(TEXT("Initial population save serializes in memory"),
            UGameplayStatics::SaveGameToMemory(InitialSave, DurablePopulationBytes))) return false;
    int32 PopulationWriteAttempts = 0;
    bool bFailNextPopulationWrite = false;
    Mode->PopulationSaveWriteAttemptForTests = [&DurablePopulationBytes, &PopulationWriteAttempts,
        &bFailNextPopulationWrite](UKalmalaWorldPopulationSaveGame* Candidate, const FString&, int32)
    {
        ++PopulationWriteAttempts;
        if (bFailNextPopulationWrite)
        {
            bFailNextPopulationWrite = false;
            return false;
        }
        return UGameplayStatics::SaveGameToMemory(Candidate, DurablePopulationBytes);
    };

    const auto SpawnCreature = [this, World, Mode, Attacker](
        const EKalmalaWildlifeArchetype Archetype, const int32 Slot) -> AKalmalaWildlifeSpawn*
    {
        const FVector SpawnLocation(static_cast<double>(Slot) * 1000.0, 0.0, 500.0);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaWildlifeSpawn* Creature = World->SpawnActor<AKalmalaWildlifeSpawn>(
            AKalmalaWildlifeSpawn::StaticClass(), SpawnLocation, FRotator::ZeroRotator, SpawnParameters);
        if (Creature == nullptr)
        {
            TestNotNull(TEXT("Wildlife defeat actor created"), Creature);
            return nullptr;
        }

        FKalmalaWorldPopulationSpawn Descriptor;
        Descriptor.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Descriptor.SpatialKey = FIntPoint(Slot, 0);
        Descriptor.SpawnSeed = 9417ull + static_cast<uint64>(Slot);
        Descriptor.Location = SpawnLocation;
        Descriptor.ContentId = FName(TEXT("wildlife-defeat-atomicity"));
        Creature->InitializeServer(Descriptor, Archetype);
        Creature->Health = 1.0f;
        if (UKalmalaResistanceComponent* Resistance = Creature->FindComponentByClass<UKalmalaResistanceComponent>())
            Resistance->Profile.Armor = 0.0f;
        else
        {
            TestTrue(TEXT("Wildlife actor resistance component exists"), false);
            return nullptr;
        }
        Creature->OnDefeated.AddUObject(Mode, &AKalmalaGameMode::RecordDefeatedSpawn);
        Attacker->SetActorLocation(Creature->GetActorLocation() + FVector(100.0, 0.0, 0.0));
        return Creature;
    };

    AKalmalaWildlifeSpawn* Rat = SpawnCreature(EKalmalaWildlifeArchetype::Rat, 1);
    if (!TestNotNull(TEXT("Rat defeat actor created"), Rat)) return false;
    const FString RatSpawnId = Rat->GetPersistentSpawnId();
    int32 RatDefeatBroadcastCount = 0;
    Rat->OnDefeated.AddLambda([&RatDefeatBroadcastCount](const FString&) { ++RatDefeatBroadcastCount; });
    Mode->RegisterLivePopulationCreature(Rat);
    TestTrue(TEXT("Rat registers with the live wildlife registry before defeat"),
        Mode->LivePopulationCreatures.Contains(TWeakObjectPtr<AKalmalaWildlifeSpawn>(Rat)));
    const int32 RatNeighborhoodLiveCount = Mode->CountLivePopulationCreaturesWithinRadius(
        Rat->GetActorLocation(), 1.0f);
    TestTrue(TEXT("Rat initially consumes one live creature cap"), RatNeighborhoodLiveCount >= 1);
    if (!TestTrue(TEXT("Rat defeat actor has a stable identity"), !RatSpawnId.IsEmpty())
        || !TestTrue(TEXT("Existing Rat claim is reserved to a different owner"),
            Mode->PopulationSaveGame->TryQueuePendingWildlifeReward(RatSpawnId, TEXT("other-player:reserved"))))
        return false;
    if (!TestTrue(TEXT("Rat owner-mismatch fixture bytes serialize before rejection"),
            UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, DurablePopulationBytes))) return false;

    TArray<uint8> RatRejectedLiveBytes;
    if (!TestTrue(TEXT("Rat owner-mismatch baseline serializes"),
            UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, RatRejectedLiveBytes))) return false;
    const TArray<uint8> RatRejectedDurableBytes = DurablePopulationBytes;
    UKalmalaWorldPopulationSaveGame* RatLiveSaveBeforeReject = Mode->PopulationSaveGame;
    Rat->Behaviour = EKalmalaWildlifeBehaviour::Flee;
    Rat->BehaviourSecondsRemaining = 1.0f;
    TestFalse(TEXT("Rat lethal hit is rejected when its stable claim belongs to another owner"),
        Rat->ApplyCombatDamageFromServer(100.0f, Attacker));
    TestFalse(TEXT("Rejected Rat remains alive and unaccepted"), Rat->IsDefeated());
    TestTrue(TEXT("Rejected Rat waits in an inactive pending-defeat state"), Rat->bPendingDefeat);
    TestEqual(TEXT("Rejected Rat has no active health while its claim is pending"), Rat->GetHealth(), 0.0f);
    TestTrue(TEXT("Rejected Rat is hidden while its defeat is pending"), Rat->IsHidden());
    const float PendingRatBehaviourSeconds = Rat->BehaviourSecondsRemaining;
    TestEqual(TEXT("Pending Rat retains the original authenticated owner identity"),
        Rat->PendingDefeatRewardContext.CreditedPlayerIdentity, OwnerIdentity);
    TestFalse(TEXT("Pending Rat rejects later hits instead of changing the credited owner"),
        Rat->ApplyCombatDamageFromServer(100.0f, Attacker));
    TestNull(TEXT("Rejected Rat does not retain an unaccepted attacker"), Rat->LastValidatedAttacker.Get());
    TestEqual(TEXT("Rejected Rat has not broadcast an accepted defeat"), RatDefeatBroadcastCount, 0);
    const FVector PendingRatLocation = Rat->GetActorLocation();
    Rat->Tick(0.5f);
    TestFalse(TEXT("Rat remains unaccepted while its conflicting owner claim exists"), Rat->IsDefeated());
    TestTrue(TEXT("Rat stays inactive while a retry is rejected"), Rat->IsHidden());
    TestEqual(TEXT("Pending Rat server behavior does not advance"),
        Rat->BehaviourSecondsRemaining, PendingRatBehaviourSeconds);
    TestTrue(TEXT("Rejected retry does not move the pending Rat"),
        Rat->GetActorLocation().Equals(PendingRatLocation));
    TestEqual(TEXT("Rejected retry does not broadcast a defeat"), RatDefeatBroadcastCount, 0);
    Rat->Tick(0.5f);
    TestFalse(TEXT("Repeated rejected retry still leaves the Rat pending"), Rat->IsDefeated());
    TestEqual(TEXT("Repeated rejected retry does not broadcast another defeat"), RatDefeatBroadcastCount, 0);
    TestTrue(TEXT("Pending Rat identity remains reserved against duplicate spawn activation"),
        Mode->HasLivePopulationCreatureWithSpawnId(RatSpawnId));
    TestEqual(TEXT("Pending Rat does not consume a live creature cap"),
        Mode->CountLivePopulationCreaturesWithinRadius(Rat->GetActorLocation(), 1.0f),
        RatNeighborhoodLiveCount - 1);
    TestTrue(TEXT("Pending Rat remains registered until its defeat commits"),
        Mode->LivePopulationCreatures.Contains(TWeakObjectPtr<AKalmalaWildlifeSpawn>(Rat)));
    TestTrue(TEXT("Rat queue rejection leaves the live save object unchanged"),
        Mode->PopulationSaveGame == RatLiveSaveBeforeReject);
    TArray<uint8> RatAfterRejectedLiveBytes;
    TestTrue(TEXT("Rat live save still serializes after queue rejection"),
        UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, RatAfterRejectedLiveBytes));
    TestTrue(TEXT("Rat queue rejection leaves the serialized live save unchanged"),
        RatAfterRejectedLiveBytes == RatRejectedLiveBytes);
    TestTrue(TEXT("Rat queue rejection leaves durable bytes unchanged"),
        DurablePopulationBytes == RatRejectedDurableBytes);
    TestFalse(TEXT("Rat queue rejection does not add a defeated ID"),
        Mode->PopulationSaveGame->IsDefeated(RatSpawnId));
    TestEqual(TEXT("Rat queue rejection does not attempt a slot write"), PopulationWriteAttempts, 0);
    UKalmalaWorldPopulationSaveGame* RatRejectedReload = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Rat rejected durable snapshot reloads"), RatRejectedReload)) return false;
    TestFalse(TEXT("Rat rejected durable snapshot has no defeated ID"),
        RatRejectedReload->IsDefeated(RatSpawnId));
    TestEqual(TEXT("Rat rejected snapshot keeps the conflicting owner claim"),
        CountDefeatClaims(RatRejectedReload, EKalmalaWildlifeArchetype::Rat, RatSpawnId), 1);

    TestTrue(TEXT("Rat conflict can be removed before retry"),
        Mode->PopulationSaveGame->RemovePendingWildlifeReward(RatSpawnId));
    if (!TestTrue(TEXT("Rat retry baseline is durably prepared in memory"), Mode->WritePopulationSaveToSlot())) return false;
    Attacker->SetPlayerState(nullptr);
    Rat->Tick(0.5f);
    Attacker->SetPlayerState(PlayerState);
    TestTrue(TEXT("Rat server retry accepts its defeat once its claim can be queued"), Rat->IsDefeated());
    TestTrue(TEXT("Successful Rat retry accepts the defeat"), Rat->IsDefeated());
    TestFalse(TEXT("Successful Rat retry clears pending-defeat state"), Rat->bPendingDefeat);
    TestEqual(TEXT("Rat defeat broadcasts exactly once after the retry"), RatDefeatBroadcastCount, 1);
    TestFalse(TEXT("Rat unregisters only after its defeat commits"),
        Mode->LivePopulationCreatures.Contains(TWeakObjectPtr<AKalmalaWildlifeSpawn>(Rat)));
    TestTrue(TEXT("Successful Rat retry records its defeated ID"),
        Mode->PopulationSaveGame->IsDefeated(RatSpawnId));
    TestEqual(TEXT("Successful Rat retry queues exactly one claim"),
        CountDefeatClaims(Mode->PopulationSaveGame, EKalmalaWildlifeArchetype::Rat, RatSpawnId), 1);
    FKalmalaPendingWildlifeReward RatClaim;
    TestTrue(TEXT("Rat retry retains its owner-bound claim"),
        FindDefeatClaim(Mode->PopulationSaveGame, EKalmalaWildlifeArchetype::Rat, RatSpawnId, RatClaim));
    TestEqual(TEXT("Rat claim belongs to the authenticated attacker"), RatClaim.PlayerIdentity, OwnerIdentity);
    UKalmalaWorldPopulationSaveGame* RatAcceptedReload = Cast<UKalmalaWorldPopulationSaveGame>(
        UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
    if (!TestNotNull(TEXT("Rat accepted durable save reloads"), RatAcceptedReload)) return false;
    TestTrue(TEXT("Rat accepted retry persists its defeated ID"), RatAcceptedReload->IsDefeated(RatSpawnId));
    TestEqual(TEXT("Rat accepted retry persists exactly one claim"),
        CountDefeatClaims(RatAcceptedReload, EKalmalaWildlifeArchetype::Rat, RatSpawnId), 1);
    TestFalse(TEXT("Completed Rat defeat cannot be accepted twice"), Rat->DefeatServer());
    TestEqual(TEXT("Completed Rat defeat does not broadcast again"), RatDefeatBroadcastCount, 1);

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
    const auto RepeatCallback = [Mode](const EKalmalaWildlifeArchetype Archetype, const FString& SpawnId)
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

    int32 Slot = 2;
    for (const FSpeciesCase& Species : SpeciesCases)
    {
        AKalmalaWildlifeSpawn* Creature = SpawnCreature(Species.Archetype, Slot++);
        if (!TestNotNull(FString::Printf(TEXT("%s defeat actor created"), Species.Name), Creature)) return false;
        const FString SpawnId = Creature->GetPersistentSpawnId();
        if (!TestTrue(FString::Printf(TEXT("%s actor has a stable identity"), Species.Name), !SpawnId.IsEmpty())) return false;
        TestFalse(FString::Printf(TEXT("%s starts with an undefeated stable ID"), Species.Name),
            Mode->PopulationSaveGame->IsDefeated(SpawnId));

        TArray<uint8> LiveBeforeFailure;
        if (!TestTrue(FString::Printf(TEXT("%s live population baseline serializes"), Species.Name),
                UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, LiveBeforeFailure))) return false;
        const TArray<uint8> DurableBeforeFailure = DurablePopulationBytes;
        UKalmalaWorldPopulationSaveGame* SaveBeforeFailure = Mode->PopulationSaveGame;
        int32 WildlifeRewardRollCount = 0;
        if (Species.Archetype == EKalmalaWildlifeArchetype::Wolf)
        {
            Mode->WildlifeRewardRollForTests = [&WildlifeRewardRollCount]()
            {
                return ++WildlifeRewardRollCount == 1 ? 0.1f : 0.9f;
            };
        }
        bFailNextPopulationWrite = true;
        TestFalse(FString::Printf(TEXT("%s lethal hit is rejected when its slot write fails"), Species.Name),
            Creature->ApplyCombatDamageFromServer(100.0f, Attacker));
        TestFalse(FString::Printf(TEXT("%s remains unaccepted after its failed callback"), Species.Name),
            Creature->IsDefeated());
        TestTrue(FString::Printf(TEXT("%s waits inactive after its failed callback"), Species.Name),
            Creature->bPendingDefeat && Creature->GetHealth() == 0.0f && Creature->IsHidden());
        TestEqual(FString::Printf(TEXT("%s retains its original owner after callback failure"), Species.Name),
            Creature->PendingDefeatRewardContext.CreditedPlayerIdentity, OwnerIdentity);
        TestNull(FString::Printf(TEXT("%s does not retain an unaccepted attacker"), Species.Name),
            Creature->LastValidatedAttacker.Get());
        TestTrue(FString::Printf(TEXT("%s failed callback retains the live save object"), Species.Name),
            Mode->PopulationSaveGame == SaveBeforeFailure);
        TArray<uint8> LiveAfterFailure;
        TestTrue(FString::Printf(TEXT("%s live save remains serializable after failure"), Species.Name),
            UGameplayStatics::SaveGameToMemory(Mode->PopulationSaveGame, LiveAfterFailure));
        TestTrue(FString::Printf(TEXT("%s failed callback leaves serialized live state unchanged"), Species.Name),
            LiveAfterFailure == LiveBeforeFailure);
        TestTrue(FString::Printf(TEXT("%s failed callback leaves durable bytes unchanged"), Species.Name),
            DurablePopulationBytes == DurableBeforeFailure);
        TestFalse(FString::Printf(TEXT("%s failed callback leaves no live defeated ID"), Species.Name),
            Mode->PopulationSaveGame->IsDefeated(SpawnId));
        TestEqual(FString::Printf(TEXT("%s failed callback leaves no reward claim"), Species.Name),
            CountDefeatClaims(Mode->PopulationSaveGame, Species.Archetype, SpawnId), 0);
        UKalmalaWorldPopulationSaveGame* FailedReload = Cast<UKalmalaWorldPopulationSaveGame>(
            UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
        if (!TestNotNull(FString::Printf(TEXT("%s failed durable snapshot reloads"), Species.Name), FailedReload))
            return false;
        TestFalse(FString::Printf(TEXT("%s failed durable snapshot has no defeated ID"), Species.Name),
            FailedReload->IsDefeated(SpawnId));
        TestEqual(FString::Printf(TEXT("%s failed durable snapshot has no claim"), Species.Name),
            CountDefeatClaims(FailedReload, Species.Archetype, SpawnId), 0);

        Attacker->SetPlayerState(nullptr);
        Creature->Tick(0.5f);
        Attacker->SetPlayerState(PlayerState);
        TestTrue(FString::Printf(TEXT("%s automatic retry accepts its defeat"), Species.Name),
            Creature->IsDefeated());
        if (Species.Archetype == EKalmalaWildlifeArchetype::Wolf)
        {
            TestEqual(TEXT("Wolf reward outcome is captured once across a failed write and retry"),
                WildlifeRewardRollCount, 1);
            Mode->WildlifeRewardRollForTests = TFunction<float()>();
        }
        TestTrue(FString::Printf(TEXT("%s actor accepts defeat on retry"), Species.Name), Creature->IsDefeated());
        TestTrue(FString::Printf(TEXT("%s retry records its stable defeated ID"), Species.Name),
            Mode->PopulationSaveGame->IsDefeated(SpawnId));
        TestEqual(FString::Printf(TEXT("%s retry queues one owner-bound claim"), Species.Name),
            CountDefeatClaims(Mode->PopulationSaveGame, Species.Archetype, SpawnId), 1);
        FKalmalaPendingWildlifeReward ClaimBeforeRepeat;
        if (!TestTrue(FString::Printf(TEXT("%s retry creates its stable reward claim"), Species.Name),
                FindDefeatClaim(Mode->PopulationSaveGame, Species.Archetype, SpawnId, ClaimBeforeRepeat))) return false;
        TestEqual(FString::Printf(TEXT("%s claim belongs to the authenticated attacker"), Species.Name),
            ClaimBeforeRepeat.PlayerIdentity, OwnerIdentity);
        FString DecodedSpawnId;
        TestTrue(FString::Printf(TEXT("%s claim carries a valid stable spawn identity"), Species.Name),
            TryDecodeDefeatClaim(ClaimBeforeRepeat.PersistentSpawnId, Species.Archetype, DecodedSpawnId));
        TestEqual(FString::Printf(TEXT("%s claim decodes to its exact actor identity"), Species.Name),
            DecodedSpawnId, SpawnId);

        UKalmalaWorldPopulationSaveGame* AcceptedReload = Cast<UKalmalaWorldPopulationSaveGame>(
            UGameplayStatics::LoadGameFromMemory(DurablePopulationBytes));
        if (!TestNotNull(FString::Printf(TEXT("%s accepted durable save reloads"), Species.Name), AcceptedReload))
            return false;
        TestTrue(FString::Printf(TEXT("%s retry persists the defeated ID"), Species.Name),
            AcceptedReload->IsDefeated(SpawnId));
        TestEqual(FString::Printf(TEXT("%s retry persists exactly one reward claim"), Species.Name),
            CountDefeatClaims(AcceptedReload, Species.Archetype, SpawnId), 1);
        FKalmalaPendingWildlifeReward DurableClaim;
        TestTrue(FString::Printf(TEXT("%s retry persists its reward claim"), Species.Name),
            FindDefeatClaim(AcceptedReload, Species.Archetype, SpawnId, DurableClaim));
        TestEqual(FString::Printf(TEXT("%s durable claim retains its owner"), Species.Name),
            DurableClaim.PlayerIdentity, OwnerIdentity);
        TestEqual(FString::Printf(TEXT("%s durable claim retains its stable outcome"), Species.Name),
            DurableClaim.PersistentSpawnId, ClaimBeforeRepeat.PersistentSpawnId);

        TestTrue(FString::Printf(TEXT("%s duplicate callback remains idempotent"), Species.Name),
            RepeatCallback(Species.Archetype, SpawnId));
        TestEqual(FString::Printf(TEXT("%s duplicate callback leaves one claim"), Species.Name),
            CountDefeatClaims(Mode->PopulationSaveGame, Species.Archetype, SpawnId), 1);
        FKalmalaPendingWildlifeReward ClaimAfterRepeat;
        TestTrue(FString::Printf(TEXT("%s repeated callback retains its claim"), Species.Name),
            FindDefeatClaim(Mode->PopulationSaveGame, Species.Archetype, SpawnId, ClaimAfterRepeat));
        TestEqual(FString::Printf(TEXT("%s repeated callback does not change owner"), Species.Name),
            ClaimAfterRepeat.PlayerIdentity, ClaimBeforeRepeat.PlayerIdentity);
        TestEqual(FString::Printf(TEXT("%s repeated callback does not reroll its outcome"), Species.Name),
            ClaimAfterRepeat.PersistentSpawnId, ClaimBeforeRepeat.PersistentSpawnId);
    }

    AKalmalaWildlifeSpawn* QueueFullRat = SpawnCreature(EKalmalaWildlifeArchetype::Rat, Slot++);
    if (!TestNotNull(TEXT("Queue-full Rat actor created"), QueueFullRat)) return false;
    const FString QueueFullRatSpawnId = QueueFullRat->GetPersistentSpawnId();
    int32 QueueFillerIndex = 0;
    FString FreedQueueClaimId;
    while (Mode->PopulationSaveGame->GetPendingWildlifeRewards().Num()
        < UKalmalaWorldPopulationSaveGame::MaxPendingWildlifeRewards)
    {
        const FString FillerId = FString::Printf(TEXT("wildlife-defeat-filler-%d"), QueueFillerIndex++);
        if (!Mode->PopulationSaveGame->TryQueuePendingWildlifeReward(FillerId, OwnerIdentity))
        {
            TestTrue(TEXT("Queue-full fixture fills the pending reward ledger"), false);
            return false;
        }
        FreedQueueClaimId = FillerId;
    }
    TestEqual(TEXT("Queue-full fixture reaches the pending reward cap"),
        Mode->PopulationSaveGame->GetPendingWildlifeRewards().Num(),
        UKalmalaWorldPopulationSaveGame::MaxPendingWildlifeRewards);

    int32 QueueFullRatDefeatBroadcastCount = 0;
    QueueFullRat->OnDefeated.AddLambda(
        [&QueueFullRatDefeatBroadcastCount](const FString&) { ++QueueFullRatDefeatBroadcastCount; });
    const int32 WritesBeforeQueueFullHit = PopulationWriteAttempts;
    TestFalse(TEXT("Queue-full Rat lethal hit remains unaccepted until a claim slot opens"),
        QueueFullRat->ApplyCombatDamageFromServer(100.0f, Attacker));
    TestTrue(TEXT("Queue-full Rat is inactive with zero health"),
        QueueFullRat->bPendingDefeat && QueueFullRat->GetHealth() == 0.0f
            && QueueFullRat->IsHidden());
    TestFalse(TEXT("Queue-full Rat has no accepted defeat yet"), QueueFullRat->IsDefeated());
    TestEqual(TEXT("Queue-full rejection does not write a partial claim"),
        PopulationWriteAttempts, WritesBeforeQueueFullHit);
    QueueFullRat->Tick(0.5f);
    TestFalse(TEXT("Queue-full retry remains pending until capacity opens"), QueueFullRat->IsDefeated());
    TestEqual(TEXT("Queue-full retry does not broadcast defeat"), QueueFullRatDefeatBroadcastCount, 0);
    TestEqual(TEXT("Queue-full retry does not attempt a save write"),
        PopulationWriteAttempts, WritesBeforeQueueFullHit);
    QueueFullRat->Tick(0.5f);
    TestFalse(TEXT("Repeated queue-full retry stays pending"), QueueFullRat->IsDefeated());
    TestEqual(TEXT("Repeated queue-full retry does not broadcast defeat"), QueueFullRatDefeatBroadcastCount, 0);
    TestTrue(TEXT("Queue-full fixture frees one pending claim"),
        Mode->PopulationSaveGame->RemovePendingWildlifeReward(FreedQueueClaimId));
    Attacker->SetPlayerState(nullptr);
    QueueFullRat->Tick(0.5f);
    Attacker->SetPlayerState(PlayerState);
    TestTrue(TEXT("Queue-full server retry completes the pending defeat"), QueueFullRat->IsDefeated());
    TestEqual(TEXT("Queue-full retry broadcasts exactly once"), QueueFullRatDefeatBroadcastCount, 1);
    TestTrue(TEXT("Queue-full retry records the defeated ID"),
        Mode->PopulationSaveGame->IsDefeated(QueueFullRatSpawnId));
    TestEqual(TEXT("Queue-full retry creates exactly one claim"),
        CountDefeatClaims(Mode->PopulationSaveGame, EKalmalaWildlifeArchetype::Rat,
            QueueFullRatSpawnId), 1);
    FKalmalaPendingWildlifeReward QueueFullRatClaim;
    TestTrue(TEXT("Queue-full retry queues the stable reward claim"),
        FindDefeatClaim(Mode->PopulationSaveGame, EKalmalaWildlifeArchetype::Rat,
            QueueFullRatSpawnId, QueueFullRatClaim));
    TestEqual(TEXT("Queue-full retry preserves the original owner"),
        QueueFullRatClaim.PlayerIdentity, OwnerIdentity);

    return true;
}

#endif
