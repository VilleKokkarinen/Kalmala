#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "KalmalaWorldGenerationConfig.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaEnvironmentalSpawnerPolicy.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaInteractionGrid.h"
#include "KalmalaGameMode.generated.h"

class UWorld;
struct FKalmalaOceanDiscoveryDescriptor;
struct FKalmalaM9ExplorationRewardDescriptor;
struct FKalmalaCharcoalKilnState;
struct FKalmalaSmelterState;
struct FKalmalaGardenPlotState;
struct FKalmalaStandingLightState;
struct FUniqueNetIdRepl;
enum class EKalmalaOceanSkiffSeat : uint8;

/** Session-only capacity held for prospective wildlife actors; no actor is created here. */
struct FKalmalaPopulationCreatureReservation
{
    FVector Location = FVector::ZeroVector;
    FIntPoint HomeSpatialKey = FIntPoint::ZeroValue;
    TOptional<EKalmalaWildlifeArchetype> Archetype;
    FString EnvironmentalSpawnerSourceId;
};

enum class EKalmalaPopulationSpawnRejectionReason : uint8
{
    None,
    SchedulerBudget,
    SeededAdmissionRoll,
    PlayerDistance,
    SpeciesHabitat,
    Time,
    Weather,
    Progression,
    Caps,
    PhysicalPlacement
};

/** Ordered gate results supplied by later server-side wiring; false defaults fail closed. */
struct FKalmalaPopulationSpawnGateResults
{
    bool bSchedulerBudgetAvailable = false;
    bool bSeededAdmissionRollPassed = false;
    bool bPlayerDistancePassed = false;
    bool bSpeciesHabitatPassed = false;
    bool bTimePassed = false;
    bool bWeatherPassed = false;
    bool bProgressionPassed = false;
    bool bCapsPassed = false;
    bool bPhysicalPlacementPassed = false;
};

/** One descriptor examined within the bounded scheduler budget, including gate failures. */
struct FKalmalaPopulationSpawnAttempt
{
    FKalmalaWorldPopulationSpawn Descriptor;
    FKalmalaPopulationSpawnGateResults GateResults;
    EKalmalaPopulationSpawnRejectionReason RejectionReason = EKalmalaPopulationSpawnRejectionReason::None;
    bool bIsEnvironmentalSpawnerAttempt = false;
    bool bEnvironmentalSpawnerConfigured = false;
    FString EnvironmentalSpawnerSourceId;
    int32 EnvironmentalSpawnerSlotIndex = INDEX_NONE;
    FVector EnvironmentalSpawnerOrigin = FVector::ZeroVector;
    int32 EnvironmentalSpawnerCreatedCountThisCheck = 0;
};

struct FKalmalaPopulationSchedulerKeyWork
{
    FIntPoint SpatialKey = FIntPoint::ZeroValue;
    int32 CandidateBudget = 0;
    uint64 CheckOrdinal = 0;
    TArray<FKalmalaWorldPopulationSpawn> Candidates;
    TArray<FKalmalaPopulationSpawnAttempt> SpawnAttempts;
};

/**
 * Server-authoritative rules for a Kalmala session.
 * Gameplay systems are added in later milestones.
 */
UCLASS()
class KALMALAGAMEPLAY_API AKalmalaGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    static constexpr int32 TraversalTestTargetSearchExtentCm = 1200000;
    static constexpr int32 TraversalTestTargetSearchStepCm = 25000;

    AKalmalaGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PreLogin(const FString& Options, const FString& Address,
        const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    bool CanPersistConstruction(FName KitId, const FTransform& Transform) const;
    bool PersistConstruction(class AKalmalaConstructionActor* Construction);
    bool CanReplaceWallWithDoor(const class AKalmalaConstructionActor* Wall) const;
    bool ReplaceWallWithDoor(class AKalmalaConstructionActor* Wall);
    bool PersistDoorOpenState(class AKalmalaConstructionActor* Door, bool bOpen);
    bool PersistCharcoalKilnState(class AKalmalaConstructionActor* Construction,
        const FKalmalaCharcoalKilnState& State);
    bool PersistSmelterState(class AKalmalaConstructionActor* Construction,
        const FKalmalaSmelterState& State);
    bool PersistGardenPlotState(class AKalmalaConstructionActor* Construction,
        const FKalmalaGardenPlotState& State);
    bool PersistStandingLightState(class AKalmalaConstructionActor* Construction,
        const FKalmalaStandingLightState& State);
    bool IsTerrainPatchActiveAt(const FVector& WorldLocation) const;
    bool ReadStorage(const class AKalmalaConstructionActor* Construction, TArray<FKalmalaInventoryStack>& Out) const;
    bool PersistStorage(const class AKalmalaConstructionActor* Construction, const TArray<FKalmalaInventoryStack>& Stacks);
    const struct FKalmalaTreasureState* PrepareWorldTreasure(const struct FKalmalaWorldDiscoveryDescriptor& Descriptor);
    bool ClaimWorldTreasure(class AKalmalaCharacter* Interactor, const struct FKalmalaWorldDiscoveryDescriptor& Descriptor);
    bool ClaimDiscovery(class AKalmalaCharacter* Interactor, const struct FKalmalaWorldDiscoveryDescriptor& Descriptor);
    bool ClaimOceanDiscovery(class AKalmalaCharacter* Interactor, const FKalmalaOceanDiscoveryDescriptor& Descriptor);
    bool ClaimM9ExplorationReward(class AKalmalaCharacter* Interactor, const FKalmalaM9ExplorationRewardDescriptor& Descriptor);
    bool ClaimMirelingBossScroll(class AKalmalaCharacter* Attacker, const FString& PersistentSpawnId);
    const struct FKalmalaGemstoneState* ReadGemstoneState(const FString& SpawnId) const;
    bool PersistGemstoneState(const struct FKalmalaGemstoneState& State, uint8 ExpectedHits, uint8 ExpectedOutcome);
    bool RecordRatDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordBadgerDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    FName SelectEnemyGemForSpawn(const FString& SpawnId, bool bMoonBear) const;
    bool RecordMirelingDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordMoonBearDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordViperDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordGiantSnakeDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordDeathAdderDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordPeatHuskDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordWolfDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordLynxDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordPolarBearDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordHornedGoatDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordThunderSentinelDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool RecordReefMawDefeatReward(const FString& PersistentSpawnId, class AKalmalaCharacter* CreditedAttacker);
    bool PersistPlayerStateFromServer(class AKalmalaCharacter* Character,
        const FString& AdditionalDiscoveryId = FString(), const FString& AdditionalLearnedEffectId = FString(),
        const FString& AdditionalM9ClaimId = FString());
    bool TrySaveWorldFromLocalHost(class APlayerController* RequestingController);
    bool TryReturnToHomeFromSolo(class APlayerController* RequestingController);
    bool TryReturnNetworkClientToHomeFromServer(class APlayerController* RequestingController,
        class AKalmalaCharacter* Character);
    bool TryReturnListenHostToHome(class APlayerController* RequestingController);
    bool CanLaunchPrimaryOceanSkiff() const { return !bOceanTravelSavedVesselRestoreBlocked; }
    static bool IsMirelingBossRewardId(const FString& PersistentSpawnId);
    static FString GetMirelingBossScrollDefinition(uint64 WorldSeed);
    static FString GetMirelingBossScrollId(uint64 WorldSeed);
    bool OnOceanSkiffStateChanged(class AKalmalaOceanSkiff* Skiff);
    bool OnOceanSkiffSeatAssigned(class AKalmalaOceanSkiff* Skiff,
        class AKalmalaCharacter* Interactor, EKalmalaOceanSkiffSeat Seat);
    bool ClearOceanTravelPassenger(class AKalmalaCharacter* Interactor, const FString& VesselId);

private:
    friend class AKalmalaCharacter;
    friend class FKalmalaUnroofedRainAccumulatorTest;
    friend class FKalmalaForestPatchLifecycleTest;
    friend class FKalmalaTundraForestPatchLifecycleTest;
    friend class FKalmalaForestRestartTest;
    friend class FKalmalaM9SourceAcceptanceTest;
    friend class FKalmalaStormcutAcceptedHitWearTest;
    friend class FKalmalaPopulationLivingPlayersTest;
    friend class FKalmalaPopulationSpatialKeyUnionTest;
    friend class FKalmalaWorldHomeReturnTransactionTest;
    friend class FKalmalaPopulationSchedulerCadenceTest;
    friend class FKalmalaPopulationSchedulerBudgetTest;
    friend class FKalmalaPopulationRingCandidateSelectorTest;
    friend class FKalmalaPopulationPlayerDistanceGateTest;
    friend class FKalmalaPopulationLiveCreatureRegistryTest;
    friend class FKalmalaPopulationAreaCapsTest;
    friend class FKalmalaPopulationWolfReservationTest;
    friend class FKalmalaPopulationSpawnGateEvaluatorTest;
    friend class FKalmalaPopulationLandSurfaceTest;
    friend class FKalmalaPopulationAquaticSurfaceTest;
    friend class FKalmalaPopulationMovementSupportTest;
    friend class FKalmalaPopulationBodyClearanceTest;
    friend class FKalmalaPopulationRoofClearanceTest;
    friend class FKalmalaPopulationStationCylinderTest;
    friend class FKalmalaPopulationSphericalSuppressionTest;
    friend class FKalmalaPopulationPhysicalPlacementGateTest;
    friend class FKalmalaPopulationCreationTest;
    friend class FKalmalaPopulationHazardMaterializationTest;
    friend class FKalmalaPopulationKeyTurnoverTest;
    friend class FKalmalaPopulationDepletionWriteRetryTest;
    friend class FKalmalaWildlifeDefeatAtomicityTest;
    friend class FKalmalaWildlifeGameModeRewardCallbackTest;
    friend class FKalmalaMirelingGemRewardDeliveryTest;
    friend class FKalmalaM9WorldDepletionPersistenceTest;
    friend class FKalmalaPlayerDiscoveryFailClosedLoadTest;
    friend class FKalmalaM9ExplorationRewardRollbackTest;
    friend class FKalmalaOceanSkiffRestoreCollisionTest;
    friend class FKalmalaEnvironmentalSpawnerAdmissionTest;
    friend class FKalmalaMirelingBossScrollRecoveryTest;
    friend class AKalmalaHarvestNode;
    friend class AKalmalaWildlifeSpawn;

#if WITH_DEV_AUTOMATION_TESTS
    friend class FKalmalaA10FreshStartBronzeAndIronTest;
    friend class FKalmalaAllItemSwingTest;
    friend class FKalmalaBiomeBaselineArgumentTest;
    friend class FKalmalaBiomeComparisonFixtureTest;
    friend class FKalmalaOceanTravelWorldSlotFailClosedTest;
    friend class FKalmalaOceanTravelSaveFailureRollbackTest;
    friend class FKalmalaOceanDiscoveryClaimTransactionTest;
    friend class FKalmalaWorldTreasureClaimTransactionTest;
    // Isolate the owner-scoped Ocean discovery ledger while exercising GameMode claims.
    TFunction<bool(const FString&, int32, class USaveGame*&)> OceanDiscoverySaveLoaderForTests;
    TFunction<bool(class UKalmalaM7PersistenceSaveGame*, const FString&, int32)> OceanDiscoverySaveWriterForTests;
    // Force inventory publication to fail after a durable pending claim is written.
    TFunction<bool()> OceanDiscoveryInventoryCommitGateForTests;
    TFunction<bool(const FVector&, bool)> PopulationLandSurfaceSampleForTests;
    // Isolate S22c's use of the real movement-support result at each actor location.
    TFunction<bool(const FVector&, bool)> PopulationMovementSupportForTests;
    // Narrow the suppression fields in composition fixtures so nearby Wolf origins are distinguishable.
    double PopulationSuppressionRadiusCmForTests = -1.0;
    // Mutates a completed sample only for descriptor-path automation fixtures.
    TFunction<void(const FVector2D&, bool, bool&, FKalmalaOceanSample&)> PopulationAquaticSurfaceSampleForTests;
    // Isolate sparse world-population save writes while exercising real callbacks.
    TFunction<void(class UKalmalaWorldPopulationSaveGame*, const FString&, int32)> PopulationSaveWriterForTests;
    // Report whether a population-slot write succeeded, including injected failures.
    TFunction<bool(class UKalmalaWorldPopulationSaveGame*, const FString&, int32)> PopulationSaveWriteAttemptForTests;
    bool bA9PopulationWriterInstalled = false;
    bool bA9FailNextTreasureSaveWrite = false;
    // Isolate the world-scoped M9 depletion slot while exercising real harvests.
    TFunction<class UKalmalaM7PersistenceSaveGame*(const FString&, int32)> M9ResourceSaveLoaderForTests;
    TFunction<bool(class UKalmalaM7PersistenceSaveGame*, const FString&, int32)> M9ResourceSaveWriteAttemptForTests;
    // Isolate exact player-slot existence and loading while exercising GameMode validation.
    TFunction<bool(const FString&, int32, class USaveGame*&)> PlayerDiscoverySaveLoaderForTests;
    TFunction<bool(class UKalmalaPlayerDiscoverySaveGameV2*, const FString&, int32)> PlayerDiscoverySaveWriterForTests;
    TFunction<bool(class USaveGame*, const FString&, int32)> WorldComponentSaveWriterForTests;
    TFunction<bool(class USaveGame*, const FString&, int32)> OceanTravelSaveWriterForTests;
    TFunction<ENetMode(const UWorld*)> HomeReturnNetModeForTests;
    TFunction<bool(const APlayerController*)> HomeReturnLocalControllerForTests;
    TFunction<bool(const APlayerController*)> HomeReturnConnectionForTests;
    TFunction<void(class AKalmalaCharacter*, bool)> HomeReturnClientResponseForTests;
    TFunction<void(APlayerController*)> HomeReturnTravelForTests;
    TFunction<bool(UWorld*)> ListenHostNetDriverTeardownForTests;
    // Inject a failed actor creation at a chosen member index in population transactions.
    TFunction<bool(int32)> PopulationActorSpawnAllowedForTests;
    // Force the player-slot write boundary to fail after a candidate save is prepared.
    TFunction<bool()> PlayerStateSaveGateForTests;
#endif

    void ActivateTerrainPatch(const FIntPoint& PatchCoordinate);
    void ActivateTerrainPatchNeighborhood(const FVector2D& WorldPosition);
    void RefreshTerrainPatchNeighborhoods();
    TArray<class AKalmalaCharacter*> GetLivingPlayerPawnsForPopulation() const;
    TArray<FIntPoint> GetPopulationSpatialKeyCandidates() const;
    bool TryMakeRoomForPopulationKey();
    void RetirePopulationKey(const FIntPoint& SpatialKey);
    bool WritePopulationSaveToSlot();
    bool WritePopulationSaveObjectToSlot(class UKalmalaWorldPopulationSaveGame* SaveGame);
    bool TryCreatePopulationSaveCandidate(class UKalmalaWorldPopulationSaveGame*& OutCandidate) const;
    bool TryCommitPopulationSaveCandidate(class UKalmalaWorldPopulationSaveGame* Candidate);
    void RetryPendingPopulationDepletionWrites(double NowSeconds);
    bool EnsureMirelingBossScrollEntitlement(class AKalmalaCharacter* Attacker,
        const FString& PersistentSpawnId, bool& bOutAlreadyClaimed);
    void UpdatePopulationKeySchedule(double NowSeconds);
    TArray<FIntPoint> GetDuePopulationSpatialKeys(double NowSeconds) const;
    bool MarkPopulationSpatialKeyServiced(const FIntPoint& SpatialKey, double NowSeconds);
    void UpdatePopulationSchedulerWake(double NowSeconds);
    void UpdatePopulationCreatureUnloads(double NowSeconds,
        const TArray<class AKalmalaCharacter*>& LivingPlayers);
    bool TryCreatePopulationSpawnTransaction(FKalmalaPopulationSpawnAttempt& Attempt,
        double NowSeconds);
    bool TryCreateConfiguredEnvironmentalSpawnerCandidates(
        FKalmalaEnvironmentalSpawnerCheck& Check,
        const TArray<FKalmalaWorldPopulationSpawn>& FixedCandidates,
        int32& RemainingGlobalCandidateBudget, double NowSeconds,
        int32& OutCandidatesExamined, TArray<FKalmalaPopulationSpawnAttempt>& OutAttempts);
    TArray<FKalmalaEnvironmentalSpawnerCreatureSample> GetEnvironmentalSpawnerLivingSamples(
        const FString& SourceId) const;
    bool DoesEnvironmentalSpawnerAttemptPassLocalCap(
        const FKalmalaPopulationSpawnAttempt& Attempt, int32 AdditionalCreatureCount) const;
    bool HasLivePopulationCreatureWithSpawnId(const FString& PersistentSpawnId);
    bool IsPopulationSpawnReactivationReady(const FString& PersistentSpawnId,
        double NowSeconds) const;
    void SelectPopulationRingCandidates(FKalmalaPopulationSchedulerKeyWork& KeyWork,
        const TArray<FKalmalaWorldPopulationSpawn>& Descriptors,
        const TArray<class AKalmalaCharacter*>& LivingPlayers);
    void RegisterLivePopulationCreature(class AKalmalaWildlifeSpawn* Creature);
    void UnregisterLivePopulationCreature(class AKalmalaWildlifeSpawn* Creature);
    int32 CountLivePopulationCreaturesWithinRadius(const FVector& Center, float RadiusCm);
    int32 GetPopulationSpeciesAreaCap(EKalmalaWildlifeArchetype Archetype) const;
    void GetPopulationCreatureCapCounts(const FVector& Center, EKalmalaWildlifeArchetype Archetype,
        const FIntPoint& HomeSpatialKey, int32& OutWorldTotal, int32& OutNearbySpecies,
        int32& OutHomeKeyTotal);
    bool CanAdmitPopulationCreatureWithinCaps(const FVector& CandidateLocation,
        const FIntPoint& HomeSpatialKey, EKalmalaWildlifeArchetype Archetype);
    static EKalmalaPopulationSpawnRejectionReason EvaluatePopulationSpawnGates(
        const FKalmalaPopulationSpawnGateResults& GateResults);
    void ApplyPopulationSpeciesHabitatGate(FKalmalaPopulationSchedulerKeyWork& KeyWork) const;
    void ApplyPopulationTimeGate(FKalmalaPopulationSchedulerKeyWork& KeyWork) const;
    void ApplyPopulationWeatherGate(FKalmalaPopulationSchedulerKeyWork& KeyWork) const;
    void ApplyPopulationProgressionGate(FKalmalaPopulationSchedulerKeyWork& KeyWork) const;
    void ApplyPopulationCapGate(FKalmalaPopulationSchedulerKeyWork& KeyWork);
    void ApplyPopulationPhysicalPlacementGate(FKalmalaPopulationSchedulerKeyWork& KeyWork) const;
    bool TryReserveWolfPackCapacity(const TArray<FVector>& MemberLocations,
        const FIntPoint& HomeSpatialKey, FGuid& OutReservationId);
    bool TryReservePopulationCreatureCapacity(const TArray<FVector>& Locations,
        const FIntPoint& HomeSpatialKey, EKalmalaWildlifeArchetype Archetype,
        const FString& EnvironmentalSpawnerSourceId, int32 EnvironmentalSpawnerSlotIndex,
        const FVector& EnvironmentalSpawnerOrigin, FGuid& OutReservationId);
    bool ReleasePopulationCreatureReservation(const FGuid& ReservationId);
    void ApplyPopulationPlayerDistanceGate(FKalmalaPopulationSchedulerKeyWork& KeyWork,
        const TArray<class AKalmalaCharacter*>& LivingPlayers) const;
    bool DoesPopulationDescriptorPassSpeciesHabitat(const FKalmalaWorldPopulationSpawn& Descriptor,
        EKalmalaWildlifeArchetype* OutArchetype = nullptr) const;
    bool DoesPopulationDescriptorPassLandSurface(const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationDescriptorPassAquaticSurface(const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationDescriptorPassBodyClearance(const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationDescriptorPassRoofClearance(const FKalmalaWorldPopulationSpawn& Descriptor,
        const AActor* ProposedActorToIgnore = nullptr) const;
    bool DoesPopulationDescriptorPassMovementSupport(const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationDescriptorPassPhysicalPlacement(
        const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationPointOverlapStationCylinder(const FVector& CandidateLocation) const;
    bool DoesPopulationPointOverlapCampSphere(const FVector& CandidateLocation) const;
    bool TryGetPopulationActorPlacementLocation(const FKalmalaWorldPopulationSpawn& Descriptor,
        EKalmalaWildlifeArchetype Archetype, const TArray<FKalmalaWildlifeBodyCollisionShape>& BodyShapes,
        FVector& OutActorLocation) const;
    static bool TryGetPopulationBodyVerticalSupportOffset(
        const TArray<FKalmalaWildlifeBodyCollisionShape>& BodyShapes, FVector SurfaceNormal,
        double& OutVerticalSupportOffset);
    static bool TryGetPopulationBodyTopPositions(
        const TArray<FKalmalaWildlifeBodyCollisionShape>& BodyShapes, FVector ActorLocation,
        FRotator ActorRotation, TArray<FVector>& OutBodyTopPositions);
    static bool TryGetPopulationBodyTop(const TArray<FKalmalaWildlifeBodyCollisionShape>& BodyShapes,
        FVector ActorLocation, FRotator ActorRotation, FVector& OutBodyTop);
    static bool DoesSolidConstructionRoofIntersectSegment(UWorld* World, FVector TraceStart,
        FVector TraceEnd, const AActor* ActorToIgnore = nullptr);
    static bool DoesPopulationCollisionShapeOverlap(UWorld* World,
        const FKalmalaWildlifeBodyCollisionShape& BodyShape, FVector ActorLocation,
        FRotator ActorRotation, const AActor* ActorToIgnore = nullptr);
    static bool IsPopulationBodyShapeWithinWorldBounds(const FKalmalaWildlifeBodyCollisionShape& BodyShape,
        FVector ActorLocation, FRotator ActorRotation);
    bool DoesPopulationLandPositionPassSurface(const FVector& Location) const;
    static bool IsPopulationLandSurfaceSampleAllowed(bool bWithinBounds, bool bHasGeneratedGround,
        bool bIsDry, const FVector& SurfaceNormal);
    static bool AreAllPopulationLandSurfacesAllowed(const TArray<bool>& SurfaceResults);
    static bool DoesReefMawBodyFitOceanSample(const FKalmalaOceanSample& OceanSample,
        double HorizontalDistanceFromCenterCentimetres);
    static bool IsPopulationAquaticArchetype(EKalmalaWildlifeArchetype Archetype);
    bool DoesPopulationDescriptorPassTimeGate(const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationDescriptorPassWeatherGate(const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationDescriptorPassProgressionGate(const FKalmalaWorldPopulationSpawn& Descriptor) const;
    bool DoesPopulationDescriptorPassCaps(const FKalmalaWorldPopulationSpawn& Descriptor);
    bool IsDescriptorInSpawnRing(const FVector& SpawnLocation,
        const TArray<class AKalmalaCharacter*>& LivingPlayers) const;
    bool IsDescriptorOutsidePlayerExclusion(const FVector& SpawnLocation,
        const TArray<class AKalmalaCharacter*>& LivingPlayers) const;
    static bool IsPopulationTimeGateAllowed(EKalmalaWildlifeArchetype Archetype);
    static bool IsPopulationWeatherGateAllowed(EKalmalaWildlifeArchetype Archetype);
    static bool IsPopulationProgressionGateAllowed(EKalmalaWildlifeArchetype Archetype);
#if WITH_DEV_AUTOMATION_TESTS
    static bool IsPopulationTimeGateAllowedForTests(EKalmalaWildlifeArchetype Archetype,
        bool bForceRejectForTests);
    static bool IsPopulationWeatherGateAllowedForTests(EKalmalaWildlifeArchetype Archetype,
        bool bForceRejectForTests);
    static bool IsPopulationProgressionGateAllowedForTests(EKalmalaWildlifeArchetype Archetype,
        bool bForceRejectForTests);
#endif
    void ActivatePopulationKey(const FIntPoint& SpatialKey,
        bool bAllowLegacyWildlifeForVerification = false);
    void RefreshOceanDiscoveries();
    bool TryPersistHarvestedSpawnFromServer(const FString& PersistentSpawnId);
    bool TryRollbackHarvestedSpawnFromServer(const FString& PersistentSpawnId);
    bool HasM9ResourceDepletionFromServer(const FString& StableResourceId) const;
    bool TryPersistM9ResourceDepletionFromServer(const FString& StableResourceId);
    bool TryRollbackM9ResourceDepletionFromServer(const FString& StableResourceId);
    bool WriteM9ResourceDepletionSaveObjectToSlot(class UKalmalaM7PersistenceSaveGame* SaveGame);
    void RecordHarvestedSpawn(const FString& PersistentSpawnId);
    void LoadPopulationSave();
    void LoadM9ResourceDepletionSave();
    void RecordDefeatedSpawn(const FString& PersistentSpawnId);
    void DeliverPendingWildlifeRewards();
    class UKalmalaPlayerDiscoverySaveGameV2* GetPlayerDiscoverySave(class AKalmalaCharacter* Interactor, FString& OutIdentity);
    bool IsCurrentM9ClaimId(const FString& ClaimId) const;
    bool IsCurrentDiscoveryDescriptor(const struct FKalmalaWorldDiscoveryDescriptor& Descriptor) const;
    void ConfigureTraversalTest();
    void DriveTraversalTest();
    void ReportWorldProfileIfReady();
    void RunReconnectVerification(APawn* ServerPawn);
    void RestorePersistedConstruction();
    void LogExposureInspection(const AActor* Occupant) const;
    void LogCampConditionInspection(const AActor* Occupant) const;
    void LogBiomeFeatureInspection(const AActor* Occupant) const;
    void PruneExpiredRainAccumulators();
    void UpdatePlayerExposure(float DeltaSeconds);
    void UpdateInteractionGrid();
    void DriveCampChoiceTest();
    void DriveRainVerticalSliceTest();
    void DriveCombatPeerTest();
    void DriveMosquitoPeerTest();
    void DrivePeatHuskPeerTest();
    void DriveWolfPeerTest();
    void DriveLynxPeerTest();
    void DrivePolarBearPeerTest();
    void DriveHornedGoatPeerTest();
    void DriveThunderSentinelPeerTest();
    void DriveReefMawPeerTest();
    void DriveA9PeerTest();
    int32 A9PeerStage = 0;
    int32 A9SearchIndex = 0;
    double A9NextTime = 0;
    FIntPoint A9NodeKey;
    uint64 A9NodeSeed = 0;
    FVector A9NodeLocation;
    TWeakObjectPtr<class AKalmalaHarvestNode> A9Node;
    TWeakObjectPtr<class AKalmalaDiscoveryActor> A9Treasure;
    TArray<FKalmalaInventoryStack> A9MinerBefore;
    TArray<FKalmalaInventoryStack> A9ContenderBefore;
    TArray<FKalmalaToolState> A9MinerToolsBeforeTreasure;
    TArray<FKalmalaInventoryStack> A9MinerTreasureExpected;
    TArray<FKalmalaInventoryStack> A9ContenderTreasureExpected;
    TArray<FKalmalaInventoryStack> A9TreasureFullPack;
    TArray<FName> A9TreasureItems;
    FString A9TreasureIdToFailSave;
    int32 A9TreasurePhase = 0;
    int32 A9MinerRequestBaseline = 0;
    int32 A9ContenderRequestBaseline = 0;
    int32 A9PickBefore = 0;
    bool bA9FullPackObserved = false;
    void DriveDiscoveryPeerTest();
    void DriveM9ExplorationRewardPeerTest();
    void RunM9Schema2CandidateWorldPeerTest();
    void RunM9Schema2CandidatePlayerPeerTest(APlayerController* PlayerController);
    void DriveOceanTravelFeedbackTest();
    void DriveOceanWeatherPeerTest();
    void DriveOceanJourneyPeerTest();
    float CampChoiceStartTime = -1.0f;
    int32 CampChoiceStage = 0;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> CampChoicePlayers;
    TArray<float> CampChoiceBaselineWarmth;
    TArray<float> CampChoiceBaselineWetness;
    int32 RainVerticalSliceStage = 0;
    float RainVerticalSliceStageTime = 0.0f;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> RainVerticalSlicePlayers;
    TWeakObjectPtr<class AKalmalaCampfire> RainVerticalSliceFire;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceExposedFloor;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceRoofedFloor;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceFloorRoof;
    TWeakObjectPtr<class AKalmalaConstructionActor> RainVerticalSliceFireRoof;
    int32 CombatPeerTestStage = 0;
    bool bCombatPeerTestLogged = false;
    float CombatPeerTestStageTime = 0.0f;
    uint32 CombatPeerTestSequence = 0;
    TWeakObjectPtr<class AKalmalaCharacter> CombatPeerTestAttacker;
    TWeakObjectPtr<class AKalmalaCharacter> CombatPeerTestRemote;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> CombatPeerTestTarget;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> CombatPeerTestHerdMate;
    bool bCombatPeerTestHerdAlertObserved = false;
    bool bCombatPeerTestViperPoisonObserved = false;
    bool bCombatPeerTestGiantSnakeBiteObserved = false;
    int32 MosquitoPeerTestStage = 0;
    float MosquitoPeerTestStageTime = 0.0f;
    float MosquitoPeerTestStartingHealth = 100.0f;
    FIntPoint MosquitoPeerTestSpatialKey = FIntPoint::ZeroValue;
    uint64 MosquitoPeerTestWorldSeed = 0;
    FString MosquitoPeerTestSpawnId;
    TWeakObjectPtr<class AKalmalaCharacter> MosquitoPeerTestHost;
    TWeakObjectPtr<class AKalmalaCharacter> MosquitoPeerTestListenHost;
    TWeakObjectPtr<class AKalmalaCharacter> MosquitoPeerTestObserver;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> MosquitoPeerTestTarget;
    TWeakObjectPtr<class AKalmalaCampfire> MosquitoPeerTestCampfire;
    FVector MosquitoPeerTestContactLocation = FVector::ZeroVector;
    int32 MosquitoPeerTestBiteBaseline = 0;
    int32 MosquitoPeerTestCampfireBiteBaseline = 0;
    float MosquitoPeerTestAcceptedRollTime = 0.0f;
    float MosquitoPeerTestTorchStartedTime = 0.0f;
    bool bMosquitoPeerTestHeldTorchVerified = false;
    bool bMosquitoPeerTestUnlitCampfireVerified = false;
    int32 PeatHuskPeerTestStage = 0;
    float PeatHuskPeerTestStageTime = 0.0f;
    float PeatHuskPeerTestStartingHealth = 0.0f;
    float PeatHuskPeerTestStartDistance = 0.0f;
    bool bPeatHuskPeerTestSawTelegraph = false;
    bool bPeatHuskPeerTestSawRecovery = false;
    FIntPoint PeatHuskPeerTestSpatialKey = FIntPoint::ZeroValue;
    FString PeatHuskPeerTestSpawnId;
    TWeakObjectPtr<class AKalmalaCharacter> PeatHuskPeerTestHost;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> PeatHuskPeerTestObservers;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> PeatHuskPeerTestTarget;
    int32 WolfPeerTestStage = 0;
    float WolfPeerTestStageTime = 0.0f;
    float WolfPeerTestStartingHealth = 0.0f;
    FIntPoint WolfPeerTestSpatialKey = FIntPoint::ZeroValue;
    uint64 WolfPeerTestWorldSeed = 0;
    FString WolfPeerTestPackSpawnId;
    TArray<FString> WolfPeerTestMemberSpawnIds;
    TArray<FVector> WolfPeerTestExpectedLocations;
    TArray<TWeakObjectPtr<class AKalmalaWildlifeSpawn>> WolfPeerTestMembers;
    TWeakObjectPtr<class AKalmalaCharacter> WolfPeerTestHost;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> WolfPeerTestObservers;
    bool bWolfPeerTestSawTelegraph = false;
    bool bWolfPeerTestSawLunge = false;
    bool bWolfPeerTestSawRecovery = false;
    int32 LynxPeerTestStage = 0;
    float LynxPeerTestStageTime = 0.0f;
    float LynxPeerTestStartingHealth = 0.0f;
    FIntPoint LynxPeerTestSpatialKey = FIntPoint::ZeroValue;
    uint64 LynxPeerTestWorldSeed = 0;
    FString LynxPeerTestSpawnId;
    FVector LynxPeerTestSpawnLocation = FVector::ZeroVector;
    FVector LynxPeerTestHostRingLocation = FVector::ZeroVector;
    FVector LynxPeerTestHostLocation = FVector::ZeroVector;
    TWeakObjectPtr<class AKalmalaCharacter> LynxPeerTestHost;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> LynxPeerTestObservers;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> LynxPeerTestTarget;
    bool bLynxPeerTestSawTelegraph = false;
    bool bLynxPeerTestSawLeap = false;
    bool bLynxPeerTestSawRecovery = false;
    int32 PolarBearPeerTestStage = 0;
    float PolarBearPeerTestStageTime = 0.0f;
    float PolarBearPeerTestStartingHealth = 0.0f;
    float PolarBearPeerTestStartDistance = 0.0f;
    FIntPoint PolarBearPeerTestSpatialKey = FIntPoint::ZeroValue;
    uint64 PolarBearPeerTestWorldSeed = 0;
    FString PolarBearPeerTestSpawnId;
    FVector PolarBearPeerTestSpawnLocation = FVector::ZeroVector;
    FVector PolarBearPeerTestHostRingLocation = FVector::ZeroVector;
    FVector PolarBearPeerTestHostLocation = FVector::ZeroVector;
    TWeakObjectPtr<class AKalmalaCharacter> PolarBearPeerTestHost;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> PolarBearPeerTestObservers;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> PolarBearPeerTestTarget;
    bool bPolarBearPeerTestSawTelegraph = false;
    bool bPolarBearPeerTestSawCharge = false;
    bool bPolarBearPeerTestSawRetreat = false;
    bool bPolarBearPeerTestSawRecovery = false;
    int32 HornedGoatPeerTestStage = 0;
    float HornedGoatPeerTestStageTime = 0.0f;
    float HornedGoatPeerTestStartingHealth = 0.0f;
    float HornedGoatPeerTestStaggerStartTime = 0.0f;
    float HornedGoatPeerTestObservedStaggerDuration = 0.0f;
    FIntPoint HornedGoatPeerTestSpatialKey = FIntPoint::ZeroValue;
    uint64 HornedGoatPeerTestWorldSeed = 0;
    FString HornedGoatPeerTestSpawnId;
    FVector HornedGoatPeerTestSpawnLocation = FVector::ZeroVector;
    FVector HornedGoatPeerTestHostRingLocation = FVector::ZeroVector;
    FVector HornedGoatPeerTestHostLocation = FVector::ZeroVector;
    FVector HornedGoatPeerTestHostStartingLocation = FVector::ZeroVector;
    TWeakObjectPtr<class AKalmalaCharacter> HornedGoatPeerTestHost;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> HornedGoatPeerTestObservers;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> HornedGoatPeerTestTarget;
    bool bHornedGoatPeerTestSawTelegraph = false;
    bool bHornedGoatPeerTestSawRecovery = false;
    bool bHornedGoatPeerTestSawStagger = false;
    bool bHornedGoatPeerTestSawStaggerEnd = false;
    int32 ThunderSentinelPeerTestStage = 0;
    float ThunderSentinelPeerTestStageTime = 0.0f;
    float ThunderSentinelPeerTestStartingHealth = 0.0f;
    float ThunderSentinelPeerTestStartDistanceCm = 0.0f;
    float ThunderSentinelPeerTestHostDistanceCm = 0.0f;
    float ThunderSentinelPeerTestObservedStepCm = 0.0f;
    FIntPoint ThunderSentinelPeerTestSpatialKey = FIntPoint::ZeroValue;
    uint64 ThunderSentinelPeerTestWorldSeed = 0;
    FString ThunderSentinelPeerTestSpawnId;
    FVector ThunderSentinelPeerTestSpawnLocation = FVector::ZeroVector;
    FVector ThunderSentinelPeerTestSentinelStartLocation = FVector::ZeroVector;
    FVector ThunderSentinelPeerTestHostRingLocation = FVector::ZeroVector;
    FVector ThunderSentinelPeerTestHostLocation = FVector::ZeroVector;
    TWeakObjectPtr<class AKalmalaCharacter> ThunderSentinelPeerTestHost;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> ThunderSentinelPeerTestObservers;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> ThunderSentinelPeerTestTarget;
    bool bThunderSentinelPeerTestSawTelegraph = false;
    bool bThunderSentinelPeerTestSawSlam = false;
    bool bThunderSentinelPeerTestSawRecovery = false;
    int32 ReefMawPeerTestStage = 0;
    float ReefMawPeerTestStageTime = 0.0f;
    float ReefMawPeerTestStartingHealth = 0.0f;
    float ReefMawPeerTestStartDistanceCm = 0.0f;
    float ReefMawPeerTestSpawnDepthCm = 0.0f;
    float ReefMawPeerTestHostDistanceCm = 0.0f;
    float ReefMawPeerTestBiteDistanceCm = 0.0f;
    FIntPoint ReefMawPeerTestSpatialKey = FIntPoint::ZeroValue;
    uint64 ReefMawPeerTestWorldSeed = 0;
    FString ReefMawPeerTestSpawnId;
    FVector ReefMawPeerTestSpawnLocation = FVector::ZeroVector;
    TArray<FVector> ReefMawPeerTestOceanLocations;
    TWeakObjectPtr<class AKalmalaCharacter> ReefMawPeerTestHost;
    TArray<TWeakObjectPtr<class AKalmalaCharacter>> ReefMawPeerTestObservers;
    TWeakObjectPtr<class AKalmalaWildlifeSpawn> ReefMawPeerTestTarget;
    bool bReefMawPeerTestSawTelegraph = false;
    bool bReefMawPeerTestSawBite = false;
    bool bReefMawPeerTestSawRecovery = false;
    int32 DiscoveryPeerTestStage = 0;
    float DiscoveryPeerTestStageTime = 0.0f;
    bool bDiscoveryPeerTestLogged = false;
    FKalmalaWorldDiscoveryDescriptor DiscoveryPeerTestDescriptor;
    TWeakObjectPtr<class AKalmalaCharacter> DiscoveryPeerTestEntitled;
    TWeakObjectPtr<class AKalmalaCharacter> DiscoveryPeerTestRemote;
    TWeakObjectPtr<class AKalmalaDiscoveryActor> DiscoveryPeerTestActor;
    int32 M9ExplorationRewardPeerTestStage = 0;
    float M9ExplorationRewardPeerTestStageTime = 0.0f;
    FKalmalaM9ExplorationRewardDescriptor M9ExplorationRewardPeerTestDescriptor;
    TWeakObjectPtr<class AKalmalaCharacter> M9ExplorationRewardPeerTestHost;
    TWeakObjectPtr<class AKalmalaCharacter> M9ExplorationRewardPeerTestRemote;
    TWeakObjectPtr<class AKalmalaDiscoveryActor> M9ExplorationRewardPeerTestActor;
    int32 M9ExplorationRewardPeerTestHostBaseline = 0;
    int32 M9ExplorationRewardPeerTestRemoteBaseline = 0;
    int32 OceanTravelFeedbackTestStage = 0;
    float OceanTravelFeedbackTestStageTime = 0.0f;
    TWeakObjectPtr<class AKalmalaCharacter> OceanTravelFeedbackTestHost;
    TWeakObjectPtr<class AKalmalaCharacter> OceanTravelFeedbackTestRemote;
    int32 OceanWeatherPeerTestStage = 0;
    float OceanWeatherPeerTestStageTime = 0.0f;
    int32 OceanJourneyPeerTestStage = 0;
    float OceanJourneyPeerTestStartTime = 0.0f;
    float OceanJourneyPeerTestStageTime = 0.0f;
    FVector2D OceanJourneyPeerTestLaunch = FVector2D::ZeroVector;
    FVector2D OceanJourneyPeerTestTarget = FVector2D::ZeroVector;
    float OceanJourneyPeerTestYaw = 0.0f;
    bool bOceanJourneyPeerTestSawCrosswind = false;
    bool bOceanJourneyPeerTestSawCalm = false;
    TWeakObjectPtr<class AKalmalaCharacter> OceanJourneyPeerTestHelm;
    TWeakObjectPtr<class AKalmalaCharacter> OceanJourneyPeerTestPassenger;
    TWeakObjectPtr<class AKalmalaOceanSkiff> OceanJourneyPeerTestSkiff;
    void InitializeWeatherCycle();
    void AdvanceWeatherCycleIfNeeded();
    void PlacePawnAtGeneratedStart(class APlayerController* PlayerController);
#if !UE_BUILD_SHIPPING
    static bool TryParseBiomeBaselineNumericArguments(const TCHAR* CommandLine, float& OutXcm, float& OutYcm,
        float& OutViewZcm, int32& OutTemporalFrameCount, float& OutTemporalIntervalSeconds, int32& OutGpuCaptureFrameCount);
    static bool HasCompleteBiomeBaselineComparisonFixture(const FString& RequestedSite,
        const TArray<FString>& TaggedActorNames);
#endif
    void TryCaptureBiomeBaselineView(class APlayerController* LocalController);
    void SpawnOceanTravelTestFixture();
    void LoadOceanTravelWorldSave();
    void CompleteListenHostReturn();
    ENetMode GetHomeReturnNetMode(const UWorld* World) const;
    bool IsHomeReturnLocalController(const APlayerController* Controller) const;
    bool HasHomeReturnConnection(const APlayerController* Controller) const;
    bool SaveHomeReturnComponent(class USaveGame* SaveGame, const FString& Slot) const;
    bool HandleInFlightListenHostJoin(APlayerController* NewPlayer);
    void SendHomeReturnClientResponse(class AKalmalaCharacter* Character, bool bSaved);
    void OpenHomeAfterReturn(APlayerController* Controller);
    bool TearDownListenHostDriverForReturn(UWorld* World);
    void RestorePersistedOceanSkiff();
    void RestoreOceanTravelForPlayer(class APlayerController* PlayerController);
    void LoadOceanDiscoveryLedger(class AKalmalaCharacter* Interactor);
    bool PersistOceanTravelState(class AKalmalaOceanSkiff* Skiff);
    bool PersistOceanTravelState(class AKalmalaOceanSkiff* Skiff,
        class AKalmalaCharacter* PassengerToPersist, EKalmalaOceanSkiffSeat Seat);
    bool PersistOceanTravelPassenger(class AKalmalaCharacter* Interactor,
        const FString& VesselId, EKalmalaOceanSkiffSeat Seat);
    bool WriteOceanTravelSave(class USaveGame* SaveGame, const FString& Slot);

    class APlayerStart* GeneratedPlayerStart = nullptr;
    FKalmalaWorldGenerationConfig WorldGenerationConfig;
#if !UE_BUILD_SHIPPING
    bool bWorldGenerationConfigInitialized = false;
    bool bBiomeBaselineCaptureRequested = false;
#endif
    UPROPERTY(Transient) TObjectPtr<class AKalmalaOceanTravelTestFixture> OceanTravelTestFixture;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaWorldPopulationSaveGame> PopulationSaveGame;
    TSet<FString> PendingPopulationDepletionWriteIds;
    double NextPopulationDepletionWriteRetryTime = 0.0;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaM7PersistenceSaveGame> M9ResourceDepletionSaveGame;
    bool bM9ResourceDepletionPersistenceWritable = false;
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<class UKalmalaPlayerDiscoverySaveGameV2>> PlayerDiscoverySaves;
    TSet<FString> RejectedPlayerDiscoverySaveIdentities;
    TSet<FString> DeliveredWildlifeRewardSpawnIds;
    TSet<FString> MirelingBossScrollUnavailableFeedbackKeys;
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<class UKalmalaM7PersistenceSaveGame>> OceanDiscoverySaves;
    TMap<FString, TWeakObjectPtr<class AKalmalaCharacter>> OceanDiscoveryRewardDeliveryPawnsThisSession;
    TMap<FString, TSet<FString>> SessionM9ExplorationClaims;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaOceanTravelPersistenceSaveGame> OceanTravelWorldSave;
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<class UKalmalaOceanTravelPersistenceSaveGame>> OceanTravelPlayerSaves;
    TWeakObjectPtr<class AKalmalaOceanSkiff> RestoredOceanSkiff;
    TArray<TWeakObjectPtr<APlayerController>> PendingOceanTravelRestores;
    TSet<TWeakObjectPtr<APlayerController>> PendingNetworkHomeReturns;
    TSet<TWeakObjectPtr<APlayerController>> PendingListenHostReturnClients;
    TWeakObjectPtr<APlayerController> ListenHostReturningController;
    bool bOceanTravelPersistenceWritable = false;
    bool bOceanTravelSavedVesselRestoreBlocked = false;
    bool bSoloReturnToHomeInProgress = false;
    bool bListenHostReturnInProgress = false;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaConstructionSaveGameV2> ConstructionSaveGame;
    UPROPERTY(Transient) TObjectPtr<class UKalmalaStorageSaveGame> StorageSaveGame;
    FVector2D TerrainPatchOrigin = FVector2D::ZeroVector;
    TSet<FIntPoint> ActiveTerrainPatchCoordinates;
    TMap<FIntPoint, TObjectPtr<class AKalmalaGeneratedTerrainPatch>> ActiveTerrainPatches;
    TMap<FIntPoint, TArray<TWeakObjectPtr<class AKalmalaHarvestNode>>> ActiveForestNodes;
    TMap<FIntPoint, TArray<TWeakObjectPtr<AActor>>> ActivePopulationKeyActors;
    TSet<FIntPoint> ActivePopulationSpatialKeys;
    TSet<FIntPoint> ActivePopulationSchedulerKeys;
    TMap<FIntPoint, double> PopulationKeyNextDueTimes;
    TMap<FIntPoint, int32> PopulationKeyCandidateCursors;
    TMap<FIntPoint, uint64> PopulationKeyServicedCheckOrdinals;
    TSet<TWeakObjectPtr<class AKalmalaWildlifeSpawn>> LivePopulationCreatures;
    TMap<FString, FString> EnvironmentalSpawnerSourceIdBySpawnId;
    TMap<FIntPoint, int32> PopulationKeyWildlifeActorBudgets;
    TMap<FString, double> PopulationCreatureReactivationDueTimes;
    TMap<FGuid, TArray<FKalmalaPopulationCreatureReservation>> PendingPopulationCreatureReservations;
    double NextPopulationSchedulerWakeTime = 0.0;
    // Latest wake's bounded descriptor work and ordered gate results.
    TArray<FKalmalaPopulationSchedulerKeyWork> PopulationSchedulerWakeWork;
    TSet<FIntPoint> ActiveShimmeringLakeDiscoveryKeys;
    TSet<FIntPoint> ActiveElderwoodDiscoveryKeys;
    TSet<FIntPoint> ActiveMossyMireDiscoveryKeys;
    TSet<FIntPoint> ActiveFreezingTundraDiscoveryKeys;
    TSet<FIntPoint> ActiveThunderMountainsDiscoveryKeys;
    TMap<FIntPoint, TArray<TWeakObjectPtr<class AKalmalaDiscoveryActor>>> ActiveOceanDiscoveryActors;
    float NextTerrainPatchActivationTime = 0.0f;
    float NextOceanDiscoveryRefreshTime = 0.0f;
    float NextExposureUpdateTime = 0.0f;
    float NextInteractionGridUpdateTime = 0.0f;
    float WildlifeRewardRetrySeconds = 0.0f;
    TMap<FIntPoint, FKalmalaInteractionCellState> ActiveInteractionCells;
    TMap<TWeakObjectPtr<class AKalmalaCharacter>, float> UnroofedRainSecondsByCharacter;
    float WorldProfileReportTime = -1.0f;
    double InitialGenerationMilliseconds = -1.0;
    bool bTraversalTestEnabled = false;
    bool bExposureInspectionEnabled = false;
    bool bExposureReplicationTestEnabled = false;
    bool bExposureReplicationCampfireSpawned = false;
    bool bCampConditionInspectionEnabled = false;
    bool bBiomeFeatureInspectionEnabled = false;
    bool bWorldProfileEnabled = false;
    bool bWorldProfileReported = false;
    bool bM9Schema2CandidatePeerTestEnabled = false;
    FString M9Schema2CandidatePeerTestPhase;
    int32 M9Schema2CandidatePeerLoginCount = 0;
    FString ReconnectVerificationMode;
    FVector2D TraversalTestTarget = FVector2D::ZeroVector;
    float TraversalTestNextProgressReportTime = 0.0f;
    TSet<TWeakObjectPtr<APawn>> TraversalTestCompletedPawns;
    TSet<TWeakObjectPtr<APawn>> TraversalTestStartedPawns;
};
