#include "KalmalaGameMode.h"

#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

#include "KalmalaCharacter.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaDiscoveryActor.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaMapAwarenessComponent.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaStorageSaveGame.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaInteractionGrid.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaHazardSpawn.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldPopulationMarker.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "KalmalaWeatherCycle.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaCampConditionSampler.h"
#include "KalmalaBiomeExpansionContract.h"
#include "KalmalaShelterSampler.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaTerrainHeightSampler.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Crc.h"

namespace KalmalaGameMode
{
    constexpr int32 PlayerTerrainPatchRadius = 1;
    constexpr int32 MaxActiveTerrainPatches = 25;
    constexpr int32 MaxActivePopulationSpatialKeys = 9;
    constexpr float TerrainPatchActivationIntervalSeconds = 1.0f;
    constexpr float ExposureUpdateIntervalSeconds = 1.0f;
    constexpr float InteractionGridUpdateIntervalSeconds = 1.0f;
    constexpr float TraversalTestSpeed = 1800.0f;
    constexpr float TraversalTestArrivalDistance = 180.0f;
    constexpr int32 TraversalTargetSearchExtent = 12000;
    constexpr int32 TraversalTargetSearchStep = 250;
    FString PopulationSaveSlot(const FKalmalaWorldGenerationConfig& Config)
    {
        return FString::Printf(TEXT("KalmalaPopulationDeltas_%llu"), Config.WorldSeed);
    }
    FString ConstructionSaveSlot(const FKalmalaWorldGenerationConfig& Config)
    {
        return FString::Printf(TEXT("KalmalaConstruction_%llu"), Config.WorldSeed);
    }
    FString PlayerDiscoverySaveSlot(const FKalmalaWorldGenerationConfig& Config, const FString& PlayerIdentity)
    {
        return FString::Printf(TEXT("KalmalaPlayerDiscoveries_%llu_%08x"), Config.WorldSeed, FCrc::StrCrc32(*PlayerIdentity));
    }
}

void AKalmalaGameMode::UpdateInteractionGrid()
{
    const AKalmalaWorldGenerationGameState* WorldState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (!HasAuthority() || WorldState == nullptr)
    {
        return;
    }

    TSet<FIntPoint> RequestedKeys;
    TArray<FIntPoint> OrderedKeys;
    auto AddNeighborhood = [&RequestedKeys, &OrderedKeys](const FVector& Location, const int32 Radius)
    {
        const FIntPoint Center = FKalmalaInteractionGrid::ToCellKey(Location);
        for (int32 Y = Center.Y - Radius; Y <= Center.Y + Radius; ++Y)
        {
            for (int32 X = Center.X - Radius; X <= Center.X + Radius; ++X)
            {
                if (OrderedKeys.Num() >= FKalmalaInteractionGrid::MaxActiveCells) return;
                const FIntPoint Key(X, Y);
                if (!RequestedKeys.Contains(Key))
                {
                    RequestedKeys.Add(Key);
                    OrderedKeys.Add(Key);
                }
            }
        }
    };

    TArray<AKalmalaCampfire*> LitHearths;
    for (TActorIterator<AKalmalaCampfire> Iterator(GetWorld()); Iterator; ++Iterator)
    {
        if (Iterator->GetEffectiveWarmth() > 0.0f)
        {
            LitHearths.Add(*Iterator);
        }
    }
    LitHearths.Sort([](const AKalmalaCampfire& A, const AKalmalaCampfire& B) { return A.GetFName().LexicalLess(B.GetFName()); });
    for (const AKalmalaCampfire* Hearth : LitHearths)
    {
        AddNeighborhood(Hearth->GetActorLocation(), FKalmalaInteractionGrid::HearthRadiusCells);
        if (OrderedKeys.Num() >= FKalmalaInteractionGrid::MaxActiveCells) break;
    }

    TArray<APawn*> Pawns;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* Controller = Iterator->Get();
        if (Controller && Controller->GetPawn() && (!Controller->PlayerState || !Controller->PlayerState->IsOnlyASpectator())) Pawns.Add(Controller->GetPawn());
    }
    Pawns.Sort([](const APawn& A, const APawn& B) { return A.GetFName().LexicalLess(B.GetFName()); });
    for (const APawn* Pawn : Pawns)
    {
        AddNeighborhood(Pawn->GetActorLocation(), FKalmalaInteractionGrid::PawnRadiusCells);
        if (OrderedKeys.Num() >= FKalmalaInteractionGrid::MaxActiveCells) break;
    }

    TMap<FIntPoint, FKalmalaInteractionCellState> PreviousCells = MoveTemp(ActiveInteractionCells);
    ActiveInteractionCells.Reset(); // Cells absent from this tick deactivate and discard their transient state.
    const float Rain = WorldState->GetWeatherState().PrecipitationIntensity;
    for (const FIntPoint& Key : OrderedKeys)
    {
        FKalmalaInteractionCellState State = PreviousCells.Contains(Key)
            ? PreviousCells.FindChecked(Key)
            : FKalmalaInteractionGrid::MakeBaseline(WorldGenerationConfig, Key);
        FKalmalaInteractionGrid::AdvanceSurfaceMoisture(State, Rain, KalmalaGameMode::InteractionGridUpdateIntervalSeconds);
        ActiveInteractionCells.Add(Key, State);
    }
}

void AKalmalaGameMode::UpdatePlayerExposure(const float DeltaSeconds)
{
    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr)
    {
        return;
    }

    const FKalmalaWeatherState& Weather = WorldGenerationState->GetWeatherState();
    for (TActorIterator<AKalmalaConstructionActor> Construction(GetWorld()); Construction; ++Construction)
    {
        Construction->AdvanceRainWearFromServer(DeltaSeconds, Weather.PrecipitationIntensity);
    }
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        AKalmalaCharacter* Character = Iterator->Get() != nullptr ? Cast<AKalmalaCharacter>(Iterator->Get()->GetPawn()) : nullptr;
        if (Character == nullptr)
        {
            continue;
        }

        const FVector Location = Character->GetActorLocation();
        FKalmalaEnvironmentalExposureSample Environment = FKalmalaEnvironmentalExposureSampler::Sample(WorldGenerationConfig, FVector2D(Location));
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, FVector2D(Location)));
        if (Biome == EKalmalaBiome::ShimmeringLakes || Biome == EKalmalaBiome::Elderwood || Biome == EKalmalaBiome::MossyMire || Biome == EKalmalaBiome::FreezingTundra || Biome == EKalmalaBiome::ThunderMountains)
        {
            Environment = FKalmalaBiomeExpansionContract::ApplyExposureModifiers(Environment, Biome);
        }
        const FKalmalaShelterSample Shelter = FKalmalaShelterSampler::Sample(GetWorld(), Character, Environment.NaturalCover, Weather.WindDirectionDegrees);
        if (UKalmalaPlayerStatusComponent* Statuses = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>())
        {
            Statuses->AdvanceFromServer(DeltaSeconds);
            const bool bInWater = FKalmalaOceanSampler::Sample(WorldGenerationConfig, FVector2D(Location)).IsWater()
                || FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, FVector2D(Location));
            float& UnroofedRainSeconds = UnroofedRainSecondsByCharacter.FindOrAdd(Character);
            if (bInWater)
            {
                Statuses->ApplyWetFromServer();
                UnroofedRainSeconds = 0.0f;
            }
            else if (Weather.PrecipitationIntensity >= 0.05f && !Shelter.bHasRoof)
            {
                UnroofedRainSeconds = FMath::Clamp(UnroofedRainSeconds + FMath::Max(0.0f, DeltaSeconds), 0.0f, UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds);
                if (UnroofedRainSeconds >= UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds) Statuses->ApplyWetFromServer();
            }
            else
            {
                UnroofedRainSeconds = 0.0f;
            }
        }
        float FireWarmth = 0.0f;
        for (TActorIterator<AKalmalaCampfire> CampfireIterator(GetWorld()); CampfireIterator; ++CampfireIterator)
        {
            FireWarmth = FMath::Max(FireWarmth, CampfireIterator->GetWarmthContributionAt(Location));
            // Heat wins over water/rain refresh in this server environmental step.
            if (auto* Statuses = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>())
            {
                Statuses->TryRemoveWetAtCampfireFromServer(*CampfireIterator);
            }
        }

        FKalmalaExposureState State = Character->GetExposureState();
        State.Wetness = FKalmalaExposureResponse::AdvanceWetness(State.Wetness, Weather.PrecipitationIntensity, Environment.GroundWetness, Environment.WindExposure * Weather.WindStrength, Shelter.Shelter, FireWarmth, DeltaSeconds);
        State.Warmth = FKalmalaExposureResponse::AdvanceWarmth(State.Warmth, Environment.AmbientTemperature, State.Wetness, Environment.WindExposure * Weather.WindStrength, Shelter.Shelter, FireWarmth, DeltaSeconds);
        State.TravelSpeedMultiplier = FKalmalaExposureResponse::GetTravelSpeedMultiplier(State.Warmth);
        // Saturated Mire ground stays traversable, but server-owned footing drag makes dry hummocks and raised shelter meaningful.
        if (Biome == EKalmalaBiome::MossyMire)
        {
            State.TravelSpeedMultiplier = FMath::Max(0.68f, State.TravelSpeedMultiplier * 0.88f);
        }
        Character->SetExposureStateFromServer(State);
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaConstructionMovementTest")))
        {
            // Do not treat the normal pre-fixture open-world tick as an
            // assertion. The movement fixture is the only subject here.
            if (Shelter.bHasRoof && Shelter.bHasWindbreak)
            {
                const bool bPassed = Shelter.Shelter >= 0.8f;
                UE_LOG(LogTemp, Display, TEXT("Construction exposure server: Passed=%d Player=%d Roof=%d Windbreak=%d Shelter=%.2f Wetness=%.2f Warmth=%.2f Travel=%.2f"),
                bPassed, Character->GetPlayerState() ? Character->GetPlayerState()->GetPlayerId() : -1,
                Shelter.bHasRoof, Shelter.bHasWindbreak, Shelter.Shelter, State.Wetness, State.Warmth, State.TravelSpeedMultiplier);
            }
        }
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampChoiceTest")))
        {
            UE_LOG(LogTemp, Display, TEXT("Camp choice server %s: Wetness=%.2f Warmth=%.2f Travel=%.2f."), *FString::FromInt(Character->GetPlayerState()->GetPlayerId()), State.Wetness, State.Warmth, State.TravelSpeedMultiplier);
        }
        if (bExposureReplicationTestEnabled)
        {
            UE_LOG(LogTemp, Display, TEXT("Exposure replication test server state for %s: Weather=%d/%.2f/%d/%.2f Shelter=%.2f FireWarmth=%.2f Wetness=%.2f Warmth=%.2f TravelMultiplier=%.2f."), *Character->GetName(), Weather.WeatherCycleIndex, Weather.PrecipitationIntensity, Weather.WindDirectionDegrees, Weather.WindStrength, Shelter.Shelter, FireWarmth, State.Wetness, State.Warmth, State.TravelSpeedMultiplier);
        }
    }
}

AKalmalaGameMode::AKalmalaGameMode()
{
    bUseSeamlessTravel = true;
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AKalmalaCharacter::StaticClass();
    PlayerControllerClass = AKalmalaMapPlayerController::StaticClass();
    PlayerStateClass = AKalmalaMapPlayerState::StaticClass();
    GameStateClass = AKalmalaWorldGenerationGameState::StaticClass();
}

void AKalmalaGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    const double GenerationStartTime = FPlatformTime::Seconds();
    bWorldProfileEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaWorldProfile"));
    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create the generated player start because world identity is unavailable."));
        return;
    }

    WorldGenerationConfig = WorldGenerationState->GetWorldGenerationConfig();
    InitializeWeatherCycle();
    PopulationSaveGame = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromSlot(KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0));
    if (PopulationSaveGame == nullptr || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame = NewObject<UKalmalaWorldPopulationSaveGame>(this);
        PopulationSaveGame->InitializeForWorld(WorldGenerationConfig);
    }
    ConstructionSaveGame = Cast<UKalmalaConstructionSaveGame>(UGameplayStatics::LoadGameFromSlot(KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0));
    if (ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        ConstructionSaveGame = NewObject<UKalmalaConstructionSaveGame>(this);
        ConstructionSaveGame->InitializeForWorld(WorldGenerationConfig);
    }
    const FString StorageSlot = UKalmalaStorageSaveGame::MakeSlotName(WorldGenerationConfig);
    if (UGameplayStatics::DoesSaveGameExist(StorageSlot, 0))
    {
        StorageSaveGame = Cast<UKalmalaStorageSaveGame>(UGameplayStatics::LoadGameFromSlot(StorageSlot, 0));
        if (!StorageSaveGame || !StorageSaveGame->MatchesWorld(WorldGenerationConfig))
        {
            StorageSaveGame = nullptr;
            UE_LOG(LogTemp, Warning, TEXT("Storage unavailable: existing save is invalid or incompatible; preserved without overwrite."));
        }
    }
    else
    {
        StorageSaveGame = NewObject<UKalmalaStorageSaveGame>(this);
        StorageSaveGame->InitializeForWorld(WorldGenerationConfig);
    }
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GeneratedPlayerStart = GetWorld()->SpawnActor<APlayerStart>(
        APlayerStart::StaticClass(),
        FKalmalaWorldPlayerStartResolver::ResolveStartTransform(WorldGenerationState->GetWorldGenerationConfig()),
        SpawnParameters);

    if (GeneratedPlayerStart != nullptr)
    {
        GeneratedPlayerStart->Tags.Add(TEXT("GeneratedWorldPlayerStart"));
        UE_LOG(LogTemp, Display, TEXT("Server created seed-derived player start at %s."), *GeneratedPlayerStart->GetActorLocation().ToCompactString());

        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation();
        TerrainPatchOrigin = FVector2D(StartLocation.X, StartLocation.Y);
        ActivateTerrainPatchNeighborhood(TerrainPatchOrigin);
        RestorePersistedConstruction();
        UE_LOG(LogTemp, Display, TEXT("Server activated %d seed-derived terrain patches around the generated start."), ActiveTerrainPatchCoordinates.Num());
        for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            PlacePawnAtGeneratedStart(Iterator->Get());
        }
        ConfigureTraversalTest();
        bExposureInspectionEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaExposureInspection"));
        bExposureReplicationTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaExposureReplicationTest"));
        bCampConditionInspectionEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampConditionInspection"));
        bBiomeFeatureInspectionEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaBiomeFeatureInspection"));
        if (!ReconnectVerificationMode.IsEmpty())
        {
            AKalmalaCharacter* VerificationPawn = GetWorld()->SpawnActor<AKalmalaCharacter>(
                AKalmalaCharacter::StaticClass(), GeneratedPlayerStart->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);
            RunReconnectVerification(VerificationPawn);
        }
    }

    InitialGenerationMilliseconds = (FPlatformTime::Seconds() - GenerationStartTime) * 1000.0;
}

bool AKalmalaGameMode::CanPersistConstruction(const FName KitId, const FTransform& Transform) const
{
    if (!HasAuthority() || ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    FKalmalaConstructionSaveRecord Candidate;
    Candidate.ConstructionId = TEXT("pending");
    Candidate.KitId = KitId;
    Candidate.Transform = Transform;
    return UKalmalaConstructionSaveGame::IsValidRecord(Candidate)
        && ConstructionSaveGame->GetRecords().Num() < UKalmalaConstructionSaveGame::MaxRecords;
}

bool AKalmalaGameMode::PersistConstruction(AKalmalaConstructionActor* Construction)
{
    if (!HasAuthority() || !IsValid(Construction) || ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    FKalmalaConstructionSaveRecord Record;
    Record.ConstructionId = Construction->GetConstructionId();
    Record.KitId = Construction->GetConstructionKit();
    Record.Transform = Construction->GetActorTransform();
    if (!ConstructionSaveGame->AddRecord(Record)) return false;
    if (!UGameplayStatics::SaveGameToSlot(ConstructionSaveGame, KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        ConstructionSaveGame->RemoveRecord(Record.ConstructionId);
        UE_LOG(LogTemp, Error, TEXT("Construction save failed for %s."), *Record.ConstructionId);
        return false;
    }
    return true;
}

void AKalmalaGameMode::RestorePersistedConstruction()
{
    if (!HasAuthority() || ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return;
    for (const FKalmalaConstructionSaveRecord& Record : ConstructionSaveGame->GetRecords())
    {
        if (!UKalmalaConstructionSaveGame::IsValidRecord(Record))
        {
            UE_LOG(LogTemp, Warning, TEXT("Construction restore rejected malformed record."));
            continue;
        }
        auto* Construction = GetWorld()->SpawnActorDeferred<AKalmalaConstructionActor>(AKalmalaConstructionActor::StaticClass(), Record.Transform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Construction == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Construction restore allocation failed for %s."), *Record.ConstructionId);
            continue;
        }
        Construction->InitializeFromServer(Record.KitId, Record.ConstructionId);
        Construction->FinishSpawning(Record.Transform);
        UE_LOG(LogTemp, Display, TEXT("Construction restored: Id=%s Kit=%s."), *Record.ConstructionId, *Record.KitId.ToString());
    }
}

bool AKalmalaGameMode::ReadStorage(const AKalmalaConstructionActor* Construction, TArray<FKalmalaInventoryStack>& Out) const
{
    if (!HasAuthority() || !IsValid(Construction) || !Construction->HasAuthority() || Construction->GetWorld() != GetWorld()
        || Construction->GetConstructionKit() != TEXT("StorageKit") || !StorageSaveGame || !ConstructionSaveGame
        || !StorageSaveGame->MatchesWorld(WorldGenerationConfig) || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    // Only a paid, registered construction in this immutable world can address a chest record.
    const auto* Placed = ConstructionSaveGame->GetRecords().FindByPredicate([&](const auto& Record) {
        return Record.ConstructionId == Construction->GetConstructionId() && Record.KitId == TEXT("StorageKit")
            && Record.Transform.Equals(Construction->GetActorTransform());
    });
    if (!Placed) return false;
    const auto* Stored = StorageSaveGame->FindRecord(Placed->ConstructionId);
    Out = Stored ? Stored->Stacks : TArray<FKalmalaInventoryStack>();
    return true;
}

bool AKalmalaGameMode::PersistStorage(const AKalmalaConstructionActor* Construction, const TArray<FKalmalaInventoryStack>& Stacks)
{
    TArray<FKalmalaInventoryStack> Before;
    if (!ReadStorage(Construction, Before)) return false;
    auto* Candidate = DuplicateObject<UKalmalaStorageSaveGame>(StorageSaveGame, this);
    if (!Candidate || !Candidate->UpsertRecord(Construction->GetConstructionId(), Stacks)
        || !UGameplayStatics::SaveGameToSlot(Candidate, UKalmalaStorageSaveGame::MakeSlotName(WorldGenerationConfig), 0)) return false;
    StorageSaveGame = Candidate;
    return true;
}

void AKalmalaGameMode::LogExposureInspection(const AActor* Occupant) const
{
    if (Occupant == nullptr)
    {
        return;
    }

    const FVector Location = Occupant->GetActorLocation();
    const FVector2D Position(Location);
    const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position);
    const FKalmalaEnvironmentalExposureSample Exposure = FKalmalaEnvironmentalExposureSampler::Sample(WorldGenerationConfig, Position);
    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr)
    {
        return;
    }

    const FKalmalaWeatherState& Weather = WorldGenerationState->GetWeatherState();
    const FKalmalaShelterSample Shelter = FKalmalaShelterSampler::Sample(GetWorld(), Occupant, Exposure.NaturalCover, Weather.WindDirectionDegrees);
    UE_LOG(LogTemp, Display, TEXT("Exposure inspection (server): Pos=%s Temp=%.1f Humidity=%.2f Elevation=%.2f GroundWet=%.2f LowWet=%d Shoreline=%d ShoreWet=%.2f Ridge=%.2f Cover=%.2f Wind=%.2f Precipitation=%.2f WeatherWind=%.2f WindDirection=%d NaturalShelter=%.2f Roof=%d Windbreak=%d Shelter=%.2f Wetness=0.00 Warmth=100.00 Mitigation=None."), *Location.ToCompactString(), Exposure.AmbientTemperature, Fields.Humidity, Fields.Elevation, Exposure.GroundWetness, Exposure.bIsLowWetGround, Exposure.bIsShoreline, Exposure.ShorelineWetness, Exposure.RidgeExposure, Exposure.NaturalCover, Exposure.WindExposure, Weather.PrecipitationIntensity, Weather.WindStrength, Weather.WindDirectionDegrees, Shelter.NaturalCoverShelter, Shelter.bHasRoof, Shelter.bHasWindbreak, Shelter.Shelter);
}

void AKalmalaGameMode::LogCampConditionInspection(const AActor* Occupant) const
{
    if (Occupant == nullptr) return;
    const FVector Location = Occupant->GetActorLocation();
    const FKalmalaCampConditionSample Conditions = FKalmalaCampConditionSampler::Sample(WorldGenerationConfig, FVector2D(Location));
    UE_LOG(LogTemp, Display, TEXT("Camp condition inspection (server): Pos=%s Cover=%.2f GroundWet=%.2f WaterDistance=%.0f NearbyHarvestNodes=%d ResourceScore=%.2f. Local assessment only; no camp is authored or reserved."), *Location.ToCompactString(), Conditions.NaturalCover, Conditions.GroundWetness, Conditions.WaterDistance, Conditions.NearbyHarvestNodeCount, Conditions.NearbyResourceScore);
}

void AKalmalaGameMode::LogBiomeFeatureInspection(const AActor* Occupant) const
{
    if (Occupant == nullptr) return;
    const FVector Location = Occupant->GetActorLocation();
    const FKalmalaBiomeFeatureInspection Inspection = FKalmalaBiomeExpansionContract::Inspect(WorldGenerationConfig, FVector2D(Location));
    UE_LOG(LogTemp, Display, TEXT("Biome feature inspection (server): Pos=%s Biome=%d Seam=%d Terrain=%.2f Population=%.2f/%.2f/%.2f Exposure=%.2f/%.2f/%.2f Discovery=%s. Candidate is not spawned, revealed, or persisted."), *Location.ToCompactString(), static_cast<uint8>(Inspection.Biome), Inspection.bHasClassifierSeam, Inspection.Profile.TerrainFeatureStrength, Inspection.Profile.WildlifeBudgetMultiplier, Inspection.Profile.HarvestBudgetMultiplier, Inspection.Profile.HazardBudgetMultiplier, Inspection.Profile.GroundWetnessMultiplier, Inspection.Profile.WindExposureMultiplier, Inspection.Profile.NaturalCoverMultiplier, *Inspection.DiscoveryCandidate.StableId);
}

void AKalmalaGameMode::InitializeWeatherCycle()
{
    check(HasAuthority());
    AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    check(WorldGenerationState != nullptr);
    WorldGenerationState->SetWeatherStateFromServer(FKalmalaWeatherCycle::DeriveState(WorldGenerationConfig, 0, GetWorld()->GetTimeSeconds()));
}

void AKalmalaGameMode::AdvanceWeatherCycleIfNeeded()
{
    AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr)
    {
        return;
    }

    FKalmalaWeatherState Weather = WorldGenerationState->GetWeatherState();
    const float ServerTimeSeconds = GetWorld()->GetTimeSeconds();
    while (ServerTimeSeconds >= Weather.ServerStartTimeSeconds + Weather.DurationSeconds)
    {
        const float NextStartTimeSeconds = Weather.ServerStartTimeSeconds + Weather.DurationSeconds;
        Weather = FKalmalaWeatherCycle::DeriveState(WorldGenerationConfig, Weather.WeatherCycleIndex + 1, NextStartTimeSeconds);
        WorldGenerationState->SetWeatherStateFromServer(Weather);
    }
}

void AKalmalaGameMode::DriveRainVerticalSliceTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaRainVerticalSliceTest")) || RainVerticalSliceStage == 99) return;
    const float Now = GetWorld()->GetTimeSeconds();
    auto SetWeather = [this, Now](const int32 Cycle, const float Rain)
    {
        if (AKalmalaWorldGenerationGameState* State = GetGameState<AKalmalaWorldGenerationGameState>())
        {
            FKalmalaWeatherState Weather = State->GetWeatherState();
            Weather.WeatherCycleIndex = Cycle;
            Weather.ServerStartTimeSeconds = Now;
            Weather.DurationSeconds = 120.0f;
            Weather.PrecipitationIntensity = Rain;
            Weather.WindDirectionDegrees = 0;
            Weather.WindStrength = 0.0f;
            State->SetWeatherStateFromServer(Weather);
        }
    };
    auto HasWet = [](const AKalmalaCharacter* Character)
    {
        const UKalmalaPlayerStatusComponent* Statuses = Character ? Character->FindComponentByClass<UKalmalaPlayerStatusComponent>() : nullptr;
        return Statuses != nullptr && Statuses->HasStatus(UKalmalaPlayerStatusComponent::WetStatusId);
    };
    if (RainVerticalSliceStage == 0)
    {
        RainVerticalSlicePlayers.Reset();
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            if (AKalmalaCharacter* Character = It->Get() ? Cast<AKalmalaCharacter>(It->Get()->GetPawn()) : nullptr)
            {
                RainVerticalSlicePlayers.Add(Character);
                if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
                {
                    Movement->StopMovementImmediately();
                    Movement->DisableMovement();
                }
            }
        if (RainVerticalSlicePlayers.Num() != 2) return;
        // The generated start is already server-confirmed dry land. Water is
        // deliberately located from the same broad, deterministic fixture
        // domain used by the world coverage tests; the former 120 km local
        // search is not guaranteed to intersect a lake or coast for seed 418.
        FVector2D Dry = TerrainPatchOrigin;
        FVector2D Water = FVector2D::ZeroVector;
        bool bFoundWater = false;
        for (int32 Y = -800000; Y <= 800000 && !bFoundWater; Y += 8000)
        {
            for (int32 X = -800000; X <= 800000 && !bFoundWater; X += 8000)
            {
                const FVector2D Candidate(X, Y);
                const bool bWater = FKalmalaOceanSampler::Sample(WorldGenerationConfig, Candidate).IsWater()
                    || FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Candidate);
                if (bWater && !bFoundWater) { Water = Candidate; bFoundWater = true; }
            }
        }
        if (!bFoundWater)
        {
            UE_LOG(LogTemp, Error, TEXT("Rain vertical slice FAILED: seed has no bounded dry/water fixture."));
            RainVerticalSliceStage = 99;
            return;
        }
        ActivateTerrainPatchNeighborhood(Dry);
        ActivateTerrainPatchNeighborhood(Water);
        const FVector DryLocation(Dry.X, Dry.Y, FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Dry) + 20.0f);
        FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto SpawnPiece = [this, &Params](const FName Kit, const FString& Id, const FVector& Location, const FName Tag)
        {
            AKalmalaConstructionActor* Piece = GetWorld()->SpawnActor<AKalmalaConstructionActor>(AKalmalaConstructionActor::StaticClass(), Location, FRotator::ZeroRotator, Params);
            if (Piece) { Piece->InitializeFromServer(Kit, Id); Piece->Tags.AddUnique(Tag); }
            return Piece;
        };
        RainVerticalSliceExposedFloor = SpawnPiece(TEXT("FloorKit"), TEXT("RainSliceExposedFloor"), DryLocation + FVector(500, 0, 0), TEXT("KalmalaRainSliceExposedFloor"));
        RainVerticalSliceRoofedFloor = SpawnPiece(TEXT("FloorKit"), TEXT("RainSliceRoofedFloor"), DryLocation, TEXT("KalmalaRainSliceRoofedFloor"));
        RainVerticalSliceFloorRoof = SpawnPiece(TEXT("RoofKit"), TEXT("RainSliceFloorRoof"), DryLocation + FVector(0, 0, 280), TEXT("KalmalaRainSliceFloorRoof"));
        RainVerticalSliceFireRoof = SpawnPiece(TEXT("RoofKit"), TEXT("RainSliceFireRoof"), DryLocation + FVector(1000, 350, 280), TEXT("KalmalaRainSliceFireRoof"));
        RainVerticalSliceFire = GetWorld()->SpawnActor<AKalmalaCampfire>(AKalmalaCampfire::StaticClass(), DryLocation + FVector(0, 350, 0), FRotator::ZeroRotator, Params);
        AKalmalaCharacter* FirstPlayer = RainVerticalSlicePlayers[0].Get();
        if (!RainVerticalSliceExposedFloor.IsValid() || !RainVerticalSliceRoofedFloor.IsValid() || !RainVerticalSliceFloorRoof.IsValid() || !RainVerticalSliceFireRoof.IsValid() || !RainVerticalSliceFire.IsValid() || !FirstPlayer)
        {
            UE_LOG(LogTemp, Error, TEXT("Rain vertical slice FAILED: fixture spawn failed."));
            RainVerticalSliceStage = 99;
            return;
        }
        RainVerticalSliceFire->Tags.AddUnique(TEXT("KalmalaRainSliceFire"));
        RainVerticalSliceFire->InitializePaidFromServer(FirstPlayer);
        FirstPlayer->SetActorLocation(RainVerticalSliceFire->GetActorLocation() + FVector(80, 0, 90));
        RainVerticalSliceFire->Interact_Implementation(FirstPlayer);
        SetWeather(80, 0.0f);
        for (const TWeakObjectPtr<AKalmalaCharacter>& Player : RainVerticalSlicePlayers)
        {
            AKalmalaCharacter* Character = Player.Get();
            if (!Character) continue;
            Character->SetActorLocation(FVector(Water.X, Water.Y, FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Water) + Character->GetSimpleCollisionHalfHeight() + 10.0f));
            UnroofedRainSecondsByCharacter.Remove(Character);
        }
        RainVerticalSliceStage = 1; RainVerticalSliceStageTime = Now;
        return;
    }
    if (RainVerticalSliceStage == 1 && Now - RainVerticalSliceStageTime >= 2.0f)
    {
        bool bWaterWet = true;
        for (const TWeakObjectPtr<AKalmalaCharacter>& Player : RainVerticalSlicePlayers) bWaterWet &= HasWet(Player.Get());
        if (!bWaterWet) { UE_LOG(LogTemp, Error, TEXT("Rain vertical slice FAILED: water did not immediately apply Wet.")); RainVerticalSliceStage = 99; return; }
        for (int32 Index = 0; Index < RainVerticalSlicePlayers.Num(); ++Index)
            if (AKalmalaCharacter* Character = RainVerticalSlicePlayers[Index].Get()) { Character->SetActorLocation(RainVerticalSliceFire->GetActorLocation() + FVector(80, Index * 80, 90)); UnroofedRainSecondsByCharacter.Remove(Character); }
        RainVerticalSliceStage = 2; RainVerticalSliceStageTime = Now;
        return;
    }
    if (RainVerticalSliceStage == 2 && Now - RainVerticalSliceStageTime >= 2.0f)
    {
        bool bRecovered = RainVerticalSliceFire->IsLit();
        for (const TWeakObjectPtr<AKalmalaCharacter>& Player : RainVerticalSlicePlayers) bRecovered &= !HasWet(Player.Get());
        if (!bRecovered) { UE_LOG(LogTemp, Error, TEXT("Rain vertical slice FAILED: dry lit hearth did not remove water Wet.")); RainVerticalSliceStage = 99; return; }
        RainVerticalSliceFireRoof->SetActorLocation(RainVerticalSliceFire->GetActorLocation() + FVector(1000, 0, 280));
        SetWeather(81, 1.0f);
        RainVerticalSliceExposedFloor->AdvanceRainWearFromServer(1000.0f, 1.0f);
        RainVerticalSliceRoofedFloor->AdvanceRainWearFromServer(1000.0f, 1.0f);
        RainVerticalSliceFloorRoof->AdvanceRainWearFromServer(1000.0f, 1.0f);
        RainVerticalSliceStage = 3; RainVerticalSliceStageTime = Now;
        return;
    }
    if (RainVerticalSliceStage == 3 && Now - RainVerticalSliceStageTime >= UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds + 1.0f)
    {
        bool bRainWet = RainVerticalSliceFire->GetHearthState() == EKalmalaHearthState::Smouldering
            && FMath::IsNearlyEqual(RainVerticalSliceExposedFloor->GetHealth(), AKalmalaConstructionActor::RainHealthFloor)
            && FMath::IsNearlyEqual(RainVerticalSliceRoofedFloor->GetHealth(), AKalmalaConstructionActor::MaximumHealth)
            && FMath::IsNearlyEqual(RainVerticalSliceFloorRoof->GetHealth(), AKalmalaConstructionActor::MaximumHealth);
        for (const TWeakObjectPtr<AKalmalaCharacter>& Player : RainVerticalSlicePlayers) bRainWet &= HasWet(Player.Get());
        if (!bRainWet) { UE_LOG(LogTemp, Error, TEXT("Rain vertical slice FAILED: delayed rain, wear floor, or smoulder state missing.")); RainVerticalSliceStage = 99; return; }
        RainVerticalSliceFireRoof->SetActorLocation(RainVerticalSliceFire->GetActorLocation() + FVector(0, 0, 280));
        RainVerticalSliceStage = 4; RainVerticalSliceStageTime = Now;
        return;
    }
    if (RainVerticalSliceStage == 4 && Now - RainVerticalSliceStageTime >= 1.0f)
    {
        const bool bLit = RainVerticalSliceFire->IsLit();
        const bool bFireRoofed = RainVerticalSliceFire->HasRoof();
        bool bPlayersRecovered = true;
        for (const TWeakObjectPtr<AKalmalaCharacter>& Player : RainVerticalSlicePlayers) bPlayersRecovered &= !HasWet(Player.Get());
        const bool bPassed = bLit && bFireRoofed && bPlayersRecovered;
        if (bPassed)
        {
            SetWeather(82, 1.0f);
            UE_LOG(LogTemp, Display, TEXT("Rain vertical slice server: Passed=1 WaterWet=1 RainWet=1 ExposedHealth=%.1f RoofedHealth=%.1f RoofHealth=%.1f FireState=%d Roofed=%d WetRemoved=1"), RainVerticalSliceExposedFloor->GetHealth(), RainVerticalSliceRoofedFloor->GetHealth(), RainVerticalSliceFloorRoof->GetHealth(), static_cast<int32>(RainVerticalSliceFire->GetHearthState()), RainVerticalSliceFire->HasRoof());
            RainVerticalSliceStage = 99;
        }
        else if (Now - RainVerticalSliceStageTime >= 8.0f)
        {
            SetWeather(82, 1.0f);
            UE_LOG(LogTemp, Error, TEXT("Rain vertical slice server: Passed=0 Lit=%d FireRoofed=%d PlayersRecovered=%d FireState=%d"), bLit, bFireRoofed, bPlayersRecovered, static_cast<int32>(RainVerticalSliceFire->GetHearthState()));
            RainVerticalSliceStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DriveCombatPeerTest()
{
#if !UE_BUILD_SHIPPING
    const bool bCombatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaCombatPeerTest"));
    const bool bMirelingPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMirelingPeerTest"));
    const bool bBoarPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaBoarPeerTest"));
    const bool bDeerPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaDeerPeerTest"));
    if ((!bCombatPeerTest && !bMirelingPeerTest && !bBoarPeerTest && !bDeerPeerTest) || GetWorld() == nullptr)
    {
        return;
    }

    const TCHAR* VerificationName = bBoarPeerTest ? TEXT("Boar") : (bDeerPeerTest ? TEXT("Deer") : (bMirelingPeerTest ? TEXT("Mireling") : TEXT("Combat")));
    const EKalmalaWildlifeArchetype ExpectedArchetype = bBoarPeerTest ? EKalmalaWildlifeArchetype::Boar : (bDeerPeerTest ? EKalmalaWildlifeArchetype::Deer : EKalmalaWildlifeArchetype::Mireling);

    if (!bCombatPeerTestLogged)
    {
        bCombatPeerTestLogged = true;
        UE_LOG(LogTemp, Display, TEXT("%s verification server fixture enabled."), VerificationName);
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (CombatPeerTestStage == 0)
    {
        APlayerController* LocalController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* LocalServerPlayer = LocalController ? Cast<AKalmalaCharacter>(LocalController->GetPawn()) : nullptr;
        AKalmalaCharacter* RemotePlayer = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate->GetPlayerState() == nullptr || Candidate == LocalServerPlayer) continue;
            if (RemotePlayer == nullptr) RemotePlayer = Candidate;
        }
        if (LocalServerPlayer == nullptr || RemotePlayer == nullptr) return;

        CombatPeerTestAttacker = LocalServerPlayer;
        CombatPeerTestRemote = RemotePlayer;
        FIntPoint SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(LocalServerPlayer->GetActorLocation()));
        TArray<FKalmalaWorldPopulationSpawn> Descriptors = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife);
        if ((bBoarPeerTest || bDeerPeerTest) && !Descriptors.ContainsByPredicate([ExpectedArchetype](const FKalmalaWorldPopulationSpawn& Descriptor) { return AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Descriptor.SpawnSeed) == ExpectedArchetype; }))
        {
            // Stay within the normal bounded activation neighborhood while selecting
            // a deterministic boar descriptor rather than depending on actor order.
            const FIntPoint BaseKey = SpatialKey;
            bool bFoundArchetypeDescriptor = false;
            for (int32 OffsetY = -1; OffsetY <= 1 && !bFoundArchetypeDescriptor; ++OffsetY)
            {
                for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
                {
                    const FIntPoint CandidateKey = BaseKey + FIntPoint(OffsetX, OffsetY);
                    const TArray<FKalmalaWorldPopulationSpawn> CandidateDescriptors = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    if (CandidateDescriptors.ContainsByPredicate([ExpectedArchetype](const FKalmalaWorldPopulationSpawn& Descriptor) { return AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Descriptor.SpawnSeed) == ExpectedArchetype; }))
                    {
                        SpatialKey = CandidateKey;
                        Descriptors = CandidateDescriptors;
                        bFoundArchetypeDescriptor = true;
                        break;
                    }
                }
            }
        }
        const TArray<FKalmalaWorldPopulationSpawn> ReproducedDescriptors = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife);
        const int32 Budget = FKalmalaWorldPopulationLayout::GetSpawnBudget(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife);
        bool bSeedReproduced = Descriptors.Num() == ReproducedDescriptors.Num() && Descriptors.Num() <= Budget;
        for (int32 Index = 0; bSeedReproduced && Index < Descriptors.Num(); ++Index)
        {
            bSeedReproduced &= FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptors[Index]) == FKalmalaWorldPopulationLayout::GetPersistentSpawnId(ReproducedDescriptors[Index]);
        }
        const FKalmalaWorldPopulationSpawn* ExpectedDescriptor = Descriptors.FindByPredicate([ExpectedArchetype](const FKalmalaWorldPopulationSpawn& Descriptor)
        {
            return AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Descriptor.SpawnSeed) == ExpectedArchetype;
        });
        if (ExpectedDescriptor == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: server spatial key has no matching wildlife descriptor."), VerificationName);
            CombatPeerTestStage = 99;
            return;
        }
        const FString ExpectedSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(*ExpectedDescriptor);
        ActivatePopulationKey(SpatialKey);
        int32 ActiveInKey = 0;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if (Descriptors.ContainsByPredicate([&](const FKalmalaWorldPopulationSpawn& Descriptor) { return FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptor) == (*It)->GetPersistentSpawnId(); })) ++ActiveInKey;
            if (!CombatPeerTestTarget.IsValid() && !(*It)->IsDefeated() && (*It)->GetPersistentSpawnId() == ExpectedSpawnId) { CombatPeerTestTarget = *It; }
            if (bDeerPeerTest && !CombatPeerTestHerdMate.IsValid() && !(*It)->IsDefeated() && (*It)->GetArchetype() == EKalmalaWildlifeArchetype::Deer && (*It)->GetPersistentSpawnId() != ExpectedSpawnId) { CombatPeerTestHerdMate = *It; }
        }
        if (bDeerPeerTest && !CombatPeerTestHerdMate.IsValid())
        {
            // A herd mate must also come from a normal descriptor. Search only the same
            // bounded 3x3 activation neighborhood, then materialize its existing key.
            for (int32 OffsetY = -1; OffsetY <= 1 && !CombatPeerTestHerdMate.IsValid(); ++OffsetY)
            {
                for (int32 OffsetX = -1; OffsetX <= 1 && !CombatPeerTestHerdMate.IsValid(); ++OffsetX)
                {
                    const FIntPoint HerdKey = SpatialKey + FIntPoint(OffsetX, OffsetY);
                    const TArray<FKalmalaWorldPopulationSpawn> HerdDescriptors = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, HerdKey, EKalmalaWorldPopulationKind::Wildlife);
                    if (!HerdDescriptors.ContainsByPredicate([](const FKalmalaWorldPopulationSpawn& Descriptor) { return AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Descriptor.SpawnSeed) == EKalmalaWildlifeArchetype::Deer; })) continue;
                    ActivatePopulationKey(HerdKey);
                    for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
                    {
                        if (!(*It)->IsDefeated() && (*It)->GetArchetype() == EKalmalaWildlifeArchetype::Deer && (*It)->GetPersistentSpawnId() != ExpectedSpawnId)
                        {
                            CombatPeerTestHerdMate = *It;
                            break;
                        }
                    }
                }
            }
        }
        if (!bSeedReproduced || ActiveInKey > Descriptors.Num() || !CombatPeerTestTarget.IsValid() || (bDeerPeerTest && !CombatPeerTestHerdMate.IsValid()))
        {
            UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: seed reproduction=%d active=%d descriptors=%d target=%d."), VerificationName, bSeedReproduced, ActiveInKey, Descriptors.Num(), CombatPeerTestTarget.IsValid());
            CombatPeerTestStage = 99;
            return;
        }

        AKalmalaCharacter* Attacker = CombatPeerTestAttacker.Get();
        AKalmalaCharacter* Remote = CombatPeerTestRemote.Get();
        const FVector Forward = Attacker->GetActorForwardVector().GetSafeNormal2D();
        // Keep the boar at its generated rest origin: its normal territorial
        // policy must choose a nearby pawn, rather than a fixture bypassing it.
        Attacker->SetActorLocation(CombatPeerTestTarget->GetActorLocation() - Forward * 150.0f, false);
        Remote->SetActorLocation(CombatPeerTestTarget->GetActorLocation() + FVector(0.0f, 1000.0f, 0.0f), false);
        if (bDeerPeerTest)
        {
            // The companion remains a normal generated deer. This controlled positioning only
            // puts it within the production 800 cm server noise radius for the herd assertion.
            CombatPeerTestHerdMate->SetActorLocation(CombatPeerTestTarget->GetActorLocation() + FVector(0.0f, 600.0f, 0.0f), false);
            CombatPeerTestHerdMate->ForceNetUpdate();
        }
        Attacker->ForceNetUpdate();
        Remote->ForceNetUpdate();
        CombatPeerTestStage = 1;
        CombatPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Attacker = CombatPeerTestAttacker.Get();
    AKalmalaCharacter* Remote = CombatPeerTestRemote.Get();
    AKalmalaWildlifeSpawn* Target = CombatPeerTestTarget.Get();
    if (Attacker == nullptr || Remote == nullptr || Target == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Combat verification FAILED: fixture actor disappeared."));
        CombatPeerTestStage = 99;
        return;
    }
    UKalmalaCombatComponent* AttackerCombat = Attacker->GetCombatComponent();
    if (AttackerCombat == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Combat verification FAILED: server attacker combat component missing."));
        CombatPeerTestStage = 99;
        return;
    }

    // Give the bounded server-only encounter loop enough wall time to commit
    // one melee hit before asserting replicated pressure.
    if (CombatPeerTestStage == 1 && Now - CombatPeerTestStageTime >= 2.5f)
    {
        if (!FMath::IsNearlyEqual(Target->GetHealth(), 100.0f) || ((bMirelingPeerTest || bBoarPeerTest) && Attacker->GetHealth() >= 100.0f))
        {
            UE_LOG(LogTemp, Error, TEXT("Combat verification FAILED: remote target-free attack mutated the target or the server-owned encounter did not apply pressure. Health=%.1f PlayerHealth=%.1f"), Target->GetHealth(), Attacker->GetHealth());
            CombatPeerTestStage = 99;
            return;
        }
        CombatPeerTestStage = 2;
        CombatPeerTestStageTime = Now;
        return;
    }

    if (CombatPeerTestStage == 2 && AttackerCombat->GetActionPhase() == EKalmalaCombatActionPhase::Idle && CombatPeerTestSequence < 4)
    {
        Target->SetActorLocation(Attacker->GetActorLocation() + Attacker->GetActorForwardVector().GetSafeNormal2D() * 150.0f);
        Target->ForceNetUpdate();
        AttackerCombat->ServerRequestAttack(++CombatPeerTestSequence);
        CombatPeerTestStageTime = Now;
        return;
    }

    // The charge proof above already exercises real territorial movement. Keep
    // the target in the ordinary 220 cm combat trace for the separate player
    // attack contract, so a moving encounter cannot race its windup recheck.
    if (CombatPeerTestStage == 2 && AttackerCombat->GetActionPhase() == EKalmalaCombatActionPhase::Windup)
    {
        Target->SetActorLocation(Attacker->GetActorLocation() + Attacker->GetActorForwardVector().GetSafeNormal2D() * 150.0f, false);
        Target->ForceNetUpdate();
    }

    if (bDeerPeerTest && CombatPeerTestSequence > 0 && CombatPeerTestHerdMate.IsValid()
        && CombatPeerTestHerdMate->GetBehaviour() != EKalmalaWildlifeBehaviour::Idle)
    {
        bCombatPeerTestHerdAlertObserved = true;
    }

    if (CombatPeerTestStage == 2 && CombatPeerTestSequence == 4 && AttackerCombat->GetActionPhase() == EKalmalaCombatActionPhase::Idle && Now - CombatPeerTestStageTime >= 0.1f)
    {
        const bool bSavedDefeat = Target->IsDefeated() && !Target->GetPersistentSpawnId().IsEmpty() && PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(Target->GetPersistentSpawnId());
        const int32 Ash = Attacker->GetInventoryComponent() ? Attacker->GetInventoryComponent()->GetQuantity(TEXT("MirelingAsh")) : 0;
        const int32 RemoteAsh = Remote->GetInventoryComponent() ? Remote->GetInventoryComponent()->GetQuantity(TEXT("MirelingAsh")) : 0;
        const int32 Meat = Attacker->GetInventoryComponent() ? Attacker->GetInventoryComponent()->GetQuantity(TEXT("BoarMeat")) : 0;
        const int32 Hide = Attacker->GetInventoryComponent() ? Attacker->GetInventoryComponent()->GetQuantity(TEXT("BoarHide")) : 0;
        const int32 RemoteMeat = Remote->GetInventoryComponent() ? Remote->GetInventoryComponent()->GetQuantity(TEXT("BoarMeat")) : 0;
        const int32 RemoteHide = Remote->GetInventoryComponent() ? Remote->GetInventoryComponent()->GetQuantity(TEXT("BoarHide")) : 0;
        const int32 DeerMeat = Attacker->GetInventoryComponent() ? Attacker->GetInventoryComponent()->GetQuantity(TEXT("DeerMeat")) : 0;
        const int32 DeerHide = Attacker->GetInventoryComponent() ? Attacker->GetInventoryComponent()->GetQuantity(TEXT("DeerHide")) : 0;
        const int32 RemoteDeerMeat = Remote->GetInventoryComponent() ? Remote->GetInventoryComponent()->GetQuantity(TEXT("DeerMeat")) : 0;
        const int32 RemoteDeerHide = Remote->GetInventoryComponent() ? Remote->GetInventoryComponent()->GetQuantity(TEXT("DeerHide")) : 0;
        const bool bRewardValid = bMirelingPeerTest ? (Ash == 1 && RemoteAsh == 0) : (bBoarPeerTest ? (Meat == 1 && Hide == 1 && RemoteMeat == 0 && RemoteHide == 0) : (!bDeerPeerTest || (DeerMeat == 1 && DeerHide == 1 && RemoteDeerMeat == 0 && RemoteDeerHide == 0)));
        if (AttackerCombat->GetActionSerial() == 4 && bSavedDefeat && bRewardValid && (!bDeerPeerTest || bCombatPeerTestHerdAlertObserved))
        {
            UE_LOG(LogTemp, Display, TEXT("%s verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 HerdAlert=%d InvalidRejected=1 ActionSerial=%u Health=%.1f PlayerHealth=%.1f Defeated=1 Saved=1 Ash=%d RemoteAsh=%d Meat=%d Hide=%d RemoteMeat=%d RemoteHide=%d DeerMeat=%d DeerHide=%d RemoteDeerMeat=%d RemoteDeerHide=%d"), VerificationName, bCombatPeerTestHerdAlertObserved, AttackerCombat->GetActionSerial(), Target->GetHealth(), Attacker->GetHealth(), Ash, RemoteAsh, Meat, Hide, RemoteMeat, RemoteHide, DeerMeat, DeerHide, RemoteDeerMeat, RemoteDeerHide);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Combat verification server: Passed=0 ActionSerial=%u Health=%.1f Defeated=%d Saved=%d"), AttackerCombat->GetActionSerial(), Target->GetHealth(), Target->IsDefeated(), bSavedDefeat);
        }
        CombatPeerTestStage = 99;
    }
#endif
}

void AKalmalaGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || !WorldGenerationConfig.IsValid() || GetWorld() == nullptr)
    {
        return;
    }

    DriveTraversalTest();
    DriveCampChoiceTest();
    DriveRainVerticalSliceTest();
    DriveCombatPeerTest();
    ReportWorldProfileIfReady();
    AdvanceWeatherCycleIfNeeded();

    if (GetWorld()->GetTimeSeconds() >= NextExposureUpdateTime)
    {
        NextExposureUpdateTime = GetWorld()->GetTimeSeconds() + KalmalaGameMode::ExposureUpdateIntervalSeconds;
        UpdatePlayerExposure(KalmalaGameMode::ExposureUpdateIntervalSeconds);
    }

    if (GetWorld()->GetTimeSeconds() >= NextInteractionGridUpdateTime)
    {
        NextInteractionGridUpdateTime = GetWorld()->GetTimeSeconds() + KalmalaGameMode::InteractionGridUpdateIntervalSeconds;
        UpdateInteractionGrid();
    }

    if (GetWorld()->GetTimeSeconds() < NextTerrainPatchActivationTime)
    {
        return;
    }

    NextTerrainPatchActivationTime = GetWorld()->GetTimeSeconds() + KalmalaGameMode::TerrainPatchActivationIntervalSeconds;
    RefreshTerrainPatchNeighborhoods();
    for (FConstPlayerControllerIterator PlayerControllerIterator = GetWorld()->GetPlayerControllerIterator(); PlayerControllerIterator; ++PlayerControllerIterator)
    {
        const APlayerController* PlayerController = PlayerControllerIterator->Get();
        const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
        if (PlayerPawn != nullptr)
        {
            ActivatePopulationKey(FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(PlayerPawn->GetActorLocation())));
        }
    }
}

void AKalmalaGameMode::ReportWorldProfileIfReady()
{
    if (!bWorldProfileEnabled || bWorldProfileReported || WorldProfileReportTime < 0.0f || GetWorld()->GetTimeSeconds() < WorldProfileReportTime)
    {
        return;
    }

    int32 ActorCount = 0;
    int32 ReplicatedActorCount = 0;
    int32 TerrainPatchCount = 0;
    for (TActorIterator<AActor> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
    {
        ++ActorCount;
        ReplicatedActorCount += ActorIterator->GetIsReplicated() ? 1 : 0;
        TerrainPatchCount += Cast<AKalmalaGeneratedTerrainPatch>(*ActorIterator) != nullptr ? 1 : 0;
    }

    TArray<uint8> SerializedSave;
    const bool bSaveSerialized = PopulationSaveGame != nullptr && UGameplayStatics::SaveGameToMemory(PopulationSaveGame, SerializedSave);
    const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
    int32 PlayerCount = 0;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        PlayerCount += Iterator->Get() != nullptr ? 1 : 0;
    }

    UE_LOG(LogTemp, Display, TEXT("World profile: InitialGenerationMs=%.2f UsedPhysicalMB=%.2f AvailablePhysicalMB=%.2f Actors=%d ReplicatedActors=%d TerrainPatches=%d PopulationKeys=%d SaveBytes=%d SaveSerialized=%d LateJoinPlayers=%d."),
        InitialGenerationMilliseconds,
        static_cast<double>(MemoryStats.UsedPhysical) / (1024.0 * 1024.0),
        static_cast<double>(MemoryStats.AvailablePhysical) / (1024.0 * 1024.0),
        ActorCount, ReplicatedActorCount, TerrainPatchCount, ActivePopulationSpatialKeys.Num(), SerializedSave.Num(), bSaveSerialized ? 1 : 0, PlayerCount);
    bWorldProfileReported = true;
}

void AKalmalaGameMode::ActivatePopulationKey(const FIntPoint& SpatialKey)
{
    if (!HasAuthority() || GetWorld() == nullptr || ActivePopulationSpatialKeys.Contains(SpatialKey) || ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
    {
        return;
    }

    int32 SpawnedMarkerCount = 0;
    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, (FVector2D(SpatialKey) + FVector2D(0.5f, 0.5f)) * FKalmalaWorldPopulationLayout::SpatialKeySize));
    for (const EKalmalaWorldPopulationKind Kind : { EKalmalaWorldPopulationKind::Wildlife, EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Hazard })
    {
        TArray<FKalmalaWorldPopulationSpawn> Spawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, Kind);
        if (KeyBiome == EKalmalaBiome::ShimmeringLakes || KeyBiome == EKalmalaBiome::Elderwood || KeyBiome == EKalmalaBiome::MossyMire || KeyBiome == EKalmalaBiome::FreezingTundra || KeyBiome == EKalmalaBiome::ThunderMountains)
        {
            Spawns.SetNum(FMath::Min(Spawns.Num(), FKalmalaBiomeExpansionContract::ApplyPopulationBudget(Spawns.Num(), Kind, KeyBiome)));
        }
        for (const FKalmalaWorldPopulationSpawn& Spawn : Spawns)
        {
            if (Kind == EKalmalaWorldPopulationKind::HarvestNode)
            {
                const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (PopulationSaveGame != nullptr && PopulationSaveGame->IsHarvested(PersistentSpawnId))
                {
                    continue;
                }

                AKalmalaHarvestNode* HarvestNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
                if (HarvestNode != nullptr)
                {
                    HarvestNode->InitializeServer(Spawn);
                    HarvestNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                    ++SpawnedMarkerCount;
                }
            }
            else if (Kind == EKalmalaWorldPopulationKind::Wildlife)
            {
                const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(PersistentSpawnId))
                {
                    continue;
                }

                AKalmalaWildlifeSpawn* WildlifeSpawn = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(AKalmalaWildlifeSpawn::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
                if (WildlifeSpawn != nullptr)
                {
                    WildlifeSpawn->InitializeServer(Spawn);
                    WildlifeSpawn->OnDefeated.AddUObject(this, &AKalmalaGameMode::RecordDefeatedSpawn);
                    ++SpawnedMarkerCount;
                }
            }
            else if (Kind == EKalmalaWorldPopulationKind::Hazard)
            {
                const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(PersistentSpawnId))
                {
                    continue;
                }

                AKalmalaHazardSpawn* HazardSpawn = GetWorld()->SpawnActor<AKalmalaHazardSpawn>(AKalmalaHazardSpawn::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
                if (HazardSpawn != nullptr)
                {
                    HazardSpawn->InitializeServer(Spawn);
                    HazardSpawn->OnDefeated.AddUObject(this, &AKalmalaGameMode::RecordDefeatedSpawn);
                    ++SpawnedMarkerCount;
                }
            }
        }
    }

    // Discovery descriptors are server-derived. Actors are only materialized in
    // an already active key; no client receives descriptor candidates or can
    // choose a definition, position, or reward.
    for (const EKalmalaWorldDiscoveryKind Kind : { EKalmalaWorldDiscoveryKind::PointOfInterest, EKalmalaWorldDiscoveryKind::Scroll })
    {
        for (const FKalmalaWorldDiscoveryDescriptor& Descriptor : FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(WorldGenerationConfig, SpatialKey, Kind))
        {
            FActorSpawnParameters Parameters;
            Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            if (AKalmalaDiscoveryActor* Discovery = GetWorld()->SpawnActor<AKalmalaDiscoveryActor>(AKalmalaDiscoveryActor::StaticClass(), Descriptor.Location, FRotator::ZeroRotator, Parameters))
            {
                Discovery->InitializeServer(Descriptor);
                ++SpawnedMarkerCount;
            }
        }
    }

    if (KeyBiome == EKalmalaBiome::ShimmeringLakes && !ActiveShimmeringLakeDiscoveryKeys.Contains(SpatialKey))
    {
        FKalmalaBiomeDiscoveryCandidate Discovery;
        if (FKalmalaBiomeExpansionContract::TryBuildShimmeringLakeDiscovery(WorldGenerationConfig, SpatialKey, Discovery)
            && FKalmalaWorldBounds::Contains(WorldGenerationConfig, FVector2D(Discovery.Location))
            && (PopulationSaveGame == nullptr || !PopulationSaveGame->IsHarvested(Discovery.StableId)))
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AKalmalaHarvestNode* DiscoveryNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Discovery.Location, FRotator::ZeroRotator, SpawnParameters);
            if (DiscoveryNode != nullptr)
            {
                DiscoveryNode->InitializeDiscoveryServer(Discovery.StableId, Discovery.Location);
                DiscoveryNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                ++SpawnedMarkerCount;
                UE_LOG(LogTemp, Display, TEXT("Server materialized Shimmering Lakes lake-edge discovery %s."), *Discovery.StableId);
            }
        }
        ActiveShimmeringLakeDiscoveryKeys.Add(SpatialKey);
    }

    if (KeyBiome == EKalmalaBiome::Elderwood && !ActiveElderwoodDiscoveryKeys.Contains(SpatialKey))
    {
        FKalmalaBiomeDiscoveryCandidate Discovery;
        if (FKalmalaBiomeExpansionContract::TryBuildElderwoodDiscovery(WorldGenerationConfig, SpatialKey, Discovery)
            && FKalmalaWorldBounds::Contains(WorldGenerationConfig, FVector2D(Discovery.Location))
            && (PopulationSaveGame == nullptr || !PopulationSaveGame->IsHarvested(Discovery.StableId)))
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AKalmalaHarvestNode* DiscoveryNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Discovery.Location, FRotator::ZeroRotator, SpawnParameters);
            if (DiscoveryNode != nullptr)
            {
                DiscoveryNode->InitializeDiscoveryServer(Discovery.StableId, Discovery.Location);
                DiscoveryNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                ++SpawnedMarkerCount;
                UE_LOG(LogTemp, Display, TEXT("Server materialized Elderwood clearing discovery %s."), *Discovery.StableId);
            }
        }
        ActiveElderwoodDiscoveryKeys.Add(SpatialKey);
    }

    if (KeyBiome == EKalmalaBiome::MossyMire && !ActiveMossyMireDiscoveryKeys.Contains(SpatialKey))
    {
        FKalmalaBiomeDiscoveryCandidate Discovery;
        if (FKalmalaBiomeExpansionContract::TryBuildMossyMireDiscovery(WorldGenerationConfig, SpatialKey, Discovery)
            && FKalmalaWorldBounds::Contains(WorldGenerationConfig, FVector2D(Discovery.Location))
            && (PopulationSaveGame == nullptr || !PopulationSaveGame->IsHarvested(Discovery.StableId)))
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AKalmalaHarvestNode* DiscoveryNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Discovery.Location, FRotator::ZeroRotator, SpawnParameters);
            if (DiscoveryNode != nullptr)
            {
                DiscoveryNode->InitializeDiscoveryServer(Discovery.StableId, Discovery.Location);
                DiscoveryNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                ++SpawnedMarkerCount;
                UE_LOG(LogTemp, Display, TEXT("Server materialized Mossy Mire dry-hummock discovery %s."), *Discovery.StableId);
            }
        }
        ActiveMossyMireDiscoveryKeys.Add(SpatialKey);
    }

    if (KeyBiome == EKalmalaBiome::FreezingTundra && !ActiveFreezingTundraDiscoveryKeys.Contains(SpatialKey))
    {
        FKalmalaBiomeDiscoveryCandidate Discovery;
        if (FKalmalaBiomeExpansionContract::TryBuildFreezingTundraDiscovery(WorldGenerationConfig, SpatialKey, Discovery)
            && FKalmalaWorldBounds::Contains(WorldGenerationConfig, FVector2D(Discovery.Location))
            && (PopulationSaveGame == nullptr || !PopulationSaveGame->IsHarvested(Discovery.StableId)))
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AKalmalaHarvestNode* DiscoveryNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Discovery.Location, FRotator::ZeroRotator, SpawnParameters);
            if (DiscoveryNode != nullptr)
            {
                DiscoveryNode->InitializeDiscoveryServer(Discovery.StableId, Discovery.Location);
                DiscoveryNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                ++SpawnedMarkerCount;
                UE_LOG(LogTemp, Display, TEXT("Server materialized Freezing Tundra exposed-high-ground discovery %s."), *Discovery.StableId);
            }
        }
        ActiveFreezingTundraDiscoveryKeys.Add(SpatialKey);
    }

    if (KeyBiome == EKalmalaBiome::ThunderMountains && !ActiveThunderMountainsDiscoveryKeys.Contains(SpatialKey))
    {
        FKalmalaBiomeDiscoveryCandidate Discovery;
        if (FKalmalaBiomeExpansionContract::TryBuildThunderMountainsDiscovery(WorldGenerationConfig, SpatialKey, Discovery)
            && FKalmalaWorldBounds::Contains(WorldGenerationConfig, FVector2D(Discovery.Location))
            && (PopulationSaveGame == nullptr || !PopulationSaveGame->IsHarvested(Discovery.StableId)))
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            AKalmalaHarvestNode* DiscoveryNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Discovery.Location, FRotator::ZeroRotator, SpawnParameters);
            if (DiscoveryNode != nullptr)
            {
                DiscoveryNode->InitializeDiscoveryServer(Discovery.StableId, Discovery.Location);
                DiscoveryNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                ++SpawnedMarkerCount;
                UE_LOG(LogTemp, Display, TEXT("Server materialized Thunder Mountains storm-carved overlook discovery %s."), *Discovery.StableId);
            }
        }
        ActiveThunderMountainsDiscoveryKeys.Add(SpatialKey);
    }

    ActivePopulationSpatialKeys.Add(SpatialKey);
    UE_LOG(LogTemp, Display, TEXT("Server activated %d deterministic population markers for spatial key (%d, %d); %d/%d active."), SpawnedMarkerCount, SpatialKey.X, SpatialKey.Y, ActivePopulationSpatialKeys.Num(), KalmalaGameMode::MaxActivePopulationSpatialKeys);
}

void AKalmalaGameMode::RecordHarvestedSpawn(const FString& PersistentSpawnId)
{
    if (HasAuthority() && PopulationSaveGame != nullptr && PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame->MarkHarvested(PersistentSpawnId);
        UGameplayStatics::SaveGameToSlot(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }
}

void AKalmalaGameMode::RecordDefeatedSpawn(const FString& PersistentSpawnId)
{
    if (HasAuthority() && PopulationSaveGame != nullptr && PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame->MarkDefeated(PersistentSpawnId);
        UGameplayStatics::SaveGameToSlot(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }
}

UKalmalaPlayerDiscoverySaveGame* AKalmalaGameMode::GetPlayerDiscoverySave(AKalmalaCharacter* Interactor, FString& OutIdentity)
{
    OutIdentity.Reset();
    if (!HasAuthority() || Interactor == nullptr || Interactor->GetWorld() != GetWorld() || Interactor->GetPlayerState() == nullptr) return nullptr;
    const FUniqueNetIdRepl UniqueId = Interactor->GetPlayerState()->GetUniqueId();
    if (!UniqueId.IsValid()) return nullptr;
    const TSharedPtr<const FUniqueNetId> AuthenticatedId = UniqueId.GetUniqueNetId();
    if (!AuthenticatedId.IsValid()) return nullptr;
    OutIdentity = AuthenticatedId->GetType().ToString() + TEXT(":") + AuthenticatedId->ToString();
    if (OutIdentity.IsEmpty() || OutIdentity.Len() > 128) return nullptr;
    if (TObjectPtr<UKalmalaPlayerDiscoverySaveGame>* Existing = PlayerDiscoverySaves.Find(OutIdentity)) return *Existing;
    const FString Slot = KalmalaGameMode::PlayerDiscoverySaveSlot(WorldGenerationConfig, OutIdentity);
    UKalmalaPlayerDiscoverySaveGame* Save = Cast<UKalmalaPlayerDiscoverySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (Save == nullptr || !Save->Matches(WorldGenerationConfig, OutIdentity))
    {
        Save = NewObject<UKalmalaPlayerDiscoverySaveGame>(this);
        Save->InitializeForPlayer(WorldGenerationConfig, OutIdentity);
    }
    PlayerDiscoverySaves.Add(OutIdentity, Save);
    return Save;
}

bool AKalmalaGameMode::IsCurrentDiscoveryDescriptor(const FKalmalaWorldDiscoveryDescriptor& Descriptor) const
{
    if (Descriptor.DefinitionId.IsEmpty() || Descriptor.Ordinal < 0 || Descriptor.Ordinal >= 8 || !FKalmalaWorldBounds::Contains(WorldGenerationConfig, FVector2D(Descriptor.Location))) return false;
    const TArray<FKalmalaWorldDiscoveryDescriptor> Expected = FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(WorldGenerationConfig, Descriptor.SpatialKey, Descriptor.Kind);
    return Expected.ContainsByPredicate([&Descriptor](const FKalmalaWorldDiscoveryDescriptor& Candidate)
    { return FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Candidate) == FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Descriptor) && Candidate.Location.Equals(Descriptor.Location, 1.0f); });
}

bool AKalmalaGameMode::ClaimDiscovery(AKalmalaCharacter* Interactor, const FKalmalaWorldDiscoveryDescriptor& Descriptor)
{
    if (!HasAuthority() || Interactor == nullptr || !Interactor->HasAuthority() || !IsCurrentDiscoveryDescriptor(Descriptor)
        || FVector::DistSquared(Interactor->GetActorLocation(), Descriptor.Location) > FMath::Square(250.0f)) return false;
    UKalmalaDiscoveryProgressComponent* Feedback = Interactor->GetDiscoveryProgressComponent();
    FString Identity;
    UKalmalaPlayerDiscoverySaveGame* Save = GetPlayerDiscoverySave(Interactor, Identity);
    if (Save == nullptr || Feedback == nullptr) return false;
    const FString Id = FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Descriptor);
    if (Save->HasDiscovery(Id))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::AlreadyFound, TEXT("Already discovered"));
        return false;
    }
    if (!Save->AddDiscovery(Id))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Discovery unavailable"));
        return false;
    }
    if (!UGameplayStatics::SaveGameToSlot(Save, KalmalaGameMode::PlayerDiscoverySaveSlot(WorldGenerationConfig, Identity), 0))
    {
        Save->RemoveDiscovery(Id);
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Discovery unavailable"));
        return false;
    }
    Feedback->PublishFeedbackFromServer(Descriptor.Kind == EKalmalaWorldDiscoveryKind::Scroll ? EKalmalaDiscoveryFeedback::ScrollFound : EKalmalaDiscoveryFeedback::LandmarkFound,
        Descriptor.Kind == EKalmalaWorldDiscoveryKind::Scroll ? FString::Printf(TEXT("Scroll found: %s"), *Descriptor.DefinitionId) : TEXT("Landmark discovered"));
    return true;
}

void AKalmalaGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    PlacePawnAtGeneratedStart(NewPlayer);

    if ((!bTraversalTestEnabled && ReconnectVerificationMode.IsEmpty() && !bExposureInspectionEnabled && !bExposureReplicationTestEnabled && !bCampConditionInspectionEnabled && !bBiomeFeatureInspectionEnabled && !bWorldProfileEnabled && !FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampChoiceTest"))) || NewPlayer == nullptr)
    {
        return;
    }

    // GameModeBase does not guarantee a pawn for a headless, command-line client.
    // The harness creates its normal default pawn only when the test switch is enabled.
    if (NewPlayer->GetPawn() == nullptr)
    {
        RestartPlayer(NewPlayer);
    }

    if (bExposureInspectionEnabled)
    {
        LogExposureInspection(NewPlayer->GetPawn());
    }

    if (bCampConditionInspectionEnabled)
    {
        LogCampConditionInspection(NewPlayer->GetPawn());
    }

    if (bBiomeFeatureInspectionEnabled)
    {
        LogBiomeFeatureInspection(NewPlayer->GetPawn());
    }

    if (bExposureReplicationTestEnabled && !bExposureReplicationCampfireSpawned && NewPlayer->GetPawn() != nullptr)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaCampfire* Campfire = GetWorld()->SpawnActor<AKalmalaCampfire>(AKalmalaCampfire::StaticClass(), NewPlayer->GetPawn()->GetActorLocation() + FVector(120.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
        if (Campfire != nullptr)
        {
            if (auto* Pack = NewPlayer->GetPawn()->FindComponentByClass<UKalmalaInventoryComponent>()) Pack->TryGrantFromServer(TEXT("Fuel"), 1);
            Campfire->TryRefuelFromServer(Cast<AKalmalaCharacter>(NewPlayer->GetPawn()));
            Campfire->Interact_Implementation(Cast<AKalmalaCharacter>(NewPlayer->GetPawn()));
            bExposureReplicationCampfireSpawned = Campfire->IsLit();
            UE_LOG(LogTemp, Display, TEXT("Exposure replication test server spawned lit campfire: Lit=%d FuelWetness=%.2f EffectiveWarmth=%.2f."), Campfire->IsLit(), Campfire->GetFuelWetness(), Campfire->GetEffectiveWarmth());
        }
    }

    if (bExposureReplicationTestEnabled)
    {
        if (AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(NewPlayer->GetPawn()))
        {
            FKalmalaExposureState RecoveryState;
            RecoveryState.Wetness = 45.0f;
            RecoveryState.Warmth = 65.0f;
            RecoveryState.TravelSpeedMultiplier = FKalmalaExposureResponse::GetTravelSpeedMultiplier(RecoveryState.Warmth);
            Character->SetExposureStateFromServer(RecoveryState);
            UE_LOG(LogTemp, Display, TEXT("Exposure replication test server initialized recovery state for %s: Wetness=%.2f Warmth=%.2f."), *Character->GetName(), RecoveryState.Wetness, RecoveryState.Warmth);
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Developer verification server player joined with pawn: %s."), *GetNameSafe(NewPlayer->GetPawn()));
    if (bWorldProfileEnabled && !bWorldProfileReported)
    {
        int32 PlayerCount = 0;
        for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            PlayerCount += Iterator->Get() != nullptr ? 1 : 0;
        }
        if (PlayerCount >= 2)
        {
            WorldProfileReportTime = GetWorld()->GetTimeSeconds() + 3.0f;
        }
    }
    RunReconnectVerification(NewPlayer->GetPawn());
}

void AKalmalaGameMode::ConfigureTraversalTest()
{
    bTraversalTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaTraversalTest"));
    FParse::Value(FCommandLine::Get(), TEXT("KalmalaReconnectVerification="), ReconnectVerificationMode);
    if (!bTraversalTestEnabled)
    {
        return;
    }

    float ClosestDistanceSquared = TNumericLimits<float>::Max();
    for (int32 Y = -KalmalaGameMode::TraversalTargetSearchExtent; Y <= KalmalaGameMode::TraversalTargetSearchExtent; Y += KalmalaGameMode::TraversalTargetSearchStep)
    {
        for (int32 X = -KalmalaGameMode::TraversalTargetSearchExtent; X <= KalmalaGameMode::TraversalTargetSearchExtent; X += KalmalaGameMode::TraversalTargetSearchStep)
        {
            const FVector2D Candidate = TerrainPatchOrigin + FVector2D(X, Y);
            if (!FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Candidate))
            {
                continue;
            }

            const float DistanceSquared = FVector2D::DistSquared(TerrainPatchOrigin, Candidate);
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                TraversalTestTarget = Candidate;
            }
        }
    }

    if (ClosestDistanceSquared == TNumericLimits<float>::Max())
    {
        UE_LOG(LogTemp, Error, TEXT("Traversal-test switch was requested but no Shimmering Lakes target was found within %d units."), KalmalaGameMode::TraversalTargetSearchExtent);
        bTraversalTestEnabled = false;
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("Traversal-test server target is %s, %.0f units from the generated start."), *TraversalTestTarget.ToString(), FMath::Sqrt(ClosestDistanceSquared));
}

void AKalmalaGameMode::RunReconnectVerification(APawn* ServerPawn)
{
    if (ReconnectVerificationMode.IsEmpty() || ServerPawn == nullptr || PopulationSaveGame == nullptr)
    {
        return;
    }

    FIntPoint SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(ServerPawn->GetActorLocation()));
    const bool bWildlifeBoarVerify = ReconnectVerificationMode.Equals(TEXT("WildlifeBoarVerify"), ESearchCase::IgnoreCase);
    const bool bWildlifeDeerVerify = ReconnectVerificationMode.Equals(TEXT("WildlifeDeerVerify"), ESearchCase::IgnoreCase);
    const EKalmalaWildlifeArchetype VerifyArchetype = bWildlifeDeerVerify ? EKalmalaWildlifeArchetype::Deer : EKalmalaWildlifeArchetype::Boar;
    if (ReconnectVerificationMode.Equals(TEXT("WildlifeDefeat"), ESearchCase::IgnoreCase) || ReconnectVerificationMode.Equals(TEXT("WildlifeVerify"), ESearchCase::IgnoreCase) || bWildlifeBoarVerify || bWildlifeDeerVerify)
    {
        TArray<FKalmalaWorldPopulationSpawn> WildlifeSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife);
        if ((bWildlifeBoarVerify || bWildlifeDeerVerify) && !WildlifeSpawns.ContainsByPredicate([VerifyArchetype](const FKalmalaWorldPopulationSpawn& Spawn) { return AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Spawn.SpawnSeed) == VerifyArchetype; }))
        {
            const FIntPoint BaseKey = SpatialKey;
            bool bFoundArchetype = false;
            for (int32 OffsetY = -1; OffsetY <= 1 && !bFoundArchetype; ++OffsetY)
            {
                for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
                {
                    const FIntPoint CandidateKey = BaseKey + FIntPoint(OffsetX, OffsetY);
                    const TArray<FKalmalaWorldPopulationSpawn> CandidateSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    if (CandidateSpawns.ContainsByPredicate([VerifyArchetype](const FKalmalaWorldPopulationSpawn& Spawn) { return AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Spawn.SpawnSeed) == VerifyArchetype; }))
                    {
                        SpatialKey = CandidateKey;
                        WildlifeSpawns = CandidateSpawns;
                        bFoundArchetype = true;
                        break;
                    }
                }
            }
        }
        if (WildlifeSpawns.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("Reconnect verification found no generated wildlife spawn for its server spatial key."));
            FPlatformMisc::RequestExit(false);
            return;
        }

        const FKalmalaWorldPopulationSpawn* ExpectedSpawn = (bWildlifeBoarVerify || bWildlifeDeerVerify) ? WildlifeSpawns.FindByPredicate([VerifyArchetype](const FKalmalaWorldPopulationSpawn& Spawn) { return AKalmalaWildlifeSpawn::GetArchetypeForSpawnSeed(Spawn.SpawnSeed) == VerifyArchetype; }) : &WildlifeSpawns[0];
        if (ExpectedSpawn == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Reconnect verification found no server-derived boar spawn for its bounded spatial neighborhood."));
            FPlatformMisc::RequestExit(false);
            return;
        }
        const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(*ExpectedSpawn);
        ActivatePopulationKey(SpatialKey);
        if (ReconnectVerificationMode.Equals(TEXT("WildlifeDefeat"), ESearchCase::IgnoreCase))
        {
            for (TActorIterator<AKalmalaWildlifeSpawn> WildlifeIterator(GetWorld()); WildlifeIterator; ++WildlifeIterator)
            {
                AKalmalaWildlifeSpawn* WildlifeSpawn = *WildlifeIterator;
                if (WildlifeSpawn != nullptr && WildlifeSpawn->GetPersistentSpawnId() == PersistentSpawnId && WildlifeSpawn->DefeatServer())
                {
                    UE_LOG(LogTemp, Display, TEXT("Reconnect verification defeated generated wildlife spawn %s before listen-server restart."), *PersistentSpawnId);
                    FPlatformMisc::RequestExit(false);
                    return;
                }
            }
            UE_LOG(LogTemp, Error, TEXT("Reconnect verification could not activate its generated wildlife spawn before restart."));
        }
        else
        {
            bool bWildlifeRecreated = false;
            for (TActorIterator<AKalmalaWildlifeSpawn> WildlifeIterator(GetWorld()); WildlifeIterator; ++WildlifeIterator)
            {
                if ((*WildlifeIterator)->GetPersistentSpawnId() == PersistentSpawnId)
                {
                    bWildlifeRecreated = true;
                    break;
                }
            }
            if (PopulationSaveGame->IsDefeated(PersistentSpawnId) && !bWildlifeRecreated)
            {
                UE_LOG(LogTemp, Display, TEXT("Reconnect verification passed: defeated generated wildlife spawn %s remained absent after listen-server restart."), *PersistentSpawnId);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Reconnect verification failed: wildlife defeated state=%d, spawn recreated=%d."), PopulationSaveGame->IsDefeated(PersistentSpawnId), bWildlifeRecreated);
            }
        }

        FPlatformMisc::RequestExit(false);
        return;
    }

    const TArray<FKalmalaWorldPopulationSpawn> HarvestSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::HarvestNode);
    if (HarvestSpawns.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Reconnect verification found no generated harvest node for its server spatial key."));
        FPlatformMisc::RequestExit(false);
        return;
    }

    const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(HarvestSpawns[0]);
    ActivatePopulationKey(SpatialKey);
    if (ReconnectVerificationMode.Equals(TEXT("Harvest"), ESearchCase::IgnoreCase))
    {
        for (TActorIterator<AKalmalaHarvestNode> NodeIterator(GetWorld()); NodeIterator; ++NodeIterator)
        {
            AKalmalaHarvestNode* Node = *NodeIterator;
            if (Node != nullptr && Node->GetPersistentSpawnId() == PersistentSpawnId)
            {
                ServerPawn->SetActorLocation(Node->GetActorLocation());
                Node->Interact_Implementation(Cast<AKalmalaCharacter>(ServerPawn));
                UE_LOG(LogTemp, Display, TEXT("Reconnect verification harvested generated node %s before listen-server restart."), *PersistentSpawnId);
                FPlatformMisc::RequestExit(false);
                return;
            }
        }
        UE_LOG(LogTemp, Error, TEXT("Reconnect verification could not activate its generated harvest node before restart."));
    }
    else if (ReconnectVerificationMode.Equals(TEXT("Verify"), ESearchCase::IgnoreCase))
    {
        bool bNodeRecreated = false;
        for (TActorIterator<AKalmalaHarvestNode> NodeIterator(GetWorld()); NodeIterator; ++NodeIterator)
        {
            if ((*NodeIterator)->GetPersistentSpawnId() == PersistentSpawnId)
            {
                bNodeRecreated = true;
                break;
            }
        }
        if (PopulationSaveGame->IsHarvested(PersistentSpawnId) && !bNodeRecreated)
        {
            UE_LOG(LogTemp, Display, TEXT("Reconnect verification passed: harvested generated node %s remained absent after listen-server restart."), *PersistentSpawnId);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Reconnect verification failed: harvested state=%d, node recreated=%d."), PopulationSaveGame->IsHarvested(PersistentSpawnId), bNodeRecreated);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Reconnect verification mode must be Harvest, Verify, WildlifeDefeat, or WildlifeVerify."));
    }

    FPlatformMisc::RequestExit(false);
}

void AKalmalaGameMode::DriveTraversalTest()
{
    if (!bTraversalTestEnabled)
    {
        return;
    }

    for (FConstPlayerControllerIterator PlayerControllerIterator = GetWorld()->GetPlayerControllerIterator(); PlayerControllerIterator; ++PlayerControllerIterator)
    {
        const APlayerController* PlayerController = PlayerControllerIterator->Get();
        if (PlayerController == nullptr || !PlayerController->IsLocalController())
        {
            continue;
        }

        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PlayerController->GetPawn());
        if (Character == nullptr || TraversalTestCompletedPawns.Contains(Character))
        {
            continue;
        }

        if (!TraversalTestStartedPawns.Contains(Character))
        {
            TraversalTestStartedPawns.Add(Character);
            Character->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
            UE_LOG(LogTemp, Display, TEXT("Traversal-test server is driving pawn %s from %s toward the Shimmering Lakes target."), *Character->GetName(), *Character->GetActorLocation().ToCompactString());
        }

        const FVector CurrentLocation = Character->GetActorLocation();
        const FVector2D RemainingOffset = TraversalTestTarget - FVector2D(CurrentLocation);
        if (RemainingOffset.SizeSquared() <= FMath::Square(KalmalaGameMode::TraversalTestArrivalDistance))
        {
            TraversalTestCompletedPawns.Add(Character);
            UE_LOG(LogTemp, Display, TEXT("Traversal-test server pawn %s reached the Shimmering Lakes target after travelling %.0f units."), *Character->GetName(), FMath::Sqrt(FVector2D::DistSquared(TerrainPatchOrigin, FVector2D(CurrentLocation))));
            continue;
        }

        Character->GetCharacterMovement()->MaxWalkSpeed = KalmalaGameMode::TraversalTestSpeed;
        Character->AddMovementInput(FVector(RemainingOffset.GetSafeNormal(), 0.0f), 1.0f, true);
    }
}

void AKalmalaGameMode::PlacePawnAtGeneratedStart(APlayerController* PlayerController)
{
    if (!HasAuthority() || PlayerController == nullptr || GeneratedPlayerStart == nullptr)
    {
        return;
    }

    if (PlayerController->GetPawn() == nullptr)
    {
        RestartPlayer(PlayerController);
    }

    APawn* Pawn = PlayerController->GetPawn();
    if (Pawn == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not spawn a pawn at the generated player start for %s."), *PlayerController->GetName());
        return;
    }

    const FVector2D StartPosition(GeneratedPlayerStart->GetActorLocation());
    const float TerrainHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, StartPosition);
    const float RequiredCentreHeight = TerrainHeight + Pawn->GetSimpleCollisionHalfHeight() + 30.0f;
    FVector SpawnLocation = GeneratedPlayerStart->GetActorLocation();
    SpawnLocation.Z = FMath::Max(SpawnLocation.Z, RequiredCentreHeight);
    Pawn->SetActorLocationAndRotation(SpawnLocation, GeneratedPlayerStart->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
    Pawn->ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("Server placed %s at generated terrain height %.2f with capsule bottom %.2f."), *Pawn->GetName(), TerrainHeight, SpawnLocation.Z - Pawn->GetSimpleCollisionHalfHeight());
}

void AKalmalaGameMode::ActivateTerrainPatch(const FIntPoint& PatchCoordinate)
{
    if (!HasAuthority() || ActiveTerrainPatchCoordinates.Contains(PatchCoordinate) || ActiveTerrainPatchCoordinates.Num() >= KalmalaGameMode::MaxActiveTerrainPatches || GetWorld() == nullptr)
    {
        return;
    }

    const FVector2D PatchCenter = FKalmalaTerrainPatchLayout::GetPatchCenter(TerrainPatchOrigin, PatchCoordinate.X, PatchCoordinate.Y);
    if (!FKalmalaWorldBounds::IntersectsPatch(WorldGenerationConfig, PatchCenter, FKalmalaTerrainPatchLayout::PatchSize * 0.5)) return;
    AKalmalaGeneratedTerrainPatch* TerrainPatch = GetWorld()->SpawnActor<AKalmalaGeneratedTerrainPatch>(
        AKalmalaGeneratedTerrainPatch::StaticClass(), FVector(PatchCenter.X, PatchCenter.Y, 0.0f), FRotator::ZeroRotator);
    if (TerrainPatch == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not activate terrain patch (%d, %d)."), PatchCoordinate.X, PatchCoordinate.Y);
        return;
    }

    TerrainPatch->Initialize(WorldGenerationConfig, PatchCenter);
    ActiveTerrainPatchCoordinates.Add(PatchCoordinate);
    ActiveTerrainPatches.Add(PatchCoordinate, TerrainPatch);
    UE_LOG(LogTemp, Display, TEXT("Server activated terrain patch (%d, %d) at %s; %d/%d active."), PatchCoordinate.X, PatchCoordinate.Y, *TerrainPatch->GetActorLocation().ToCompactString(), ActiveTerrainPatchCoordinates.Num(), KalmalaGameMode::MaxActiveTerrainPatches);
}

void AKalmalaGameMode::ActivateTerrainPatchNeighborhood(const FVector2D& WorldPosition)
{
    const FIntPoint CenterPatchCoordinate = FKalmalaTerrainPatchLayout::GetPatchCoordinate(TerrainPatchOrigin, WorldPosition);
    for (int32 PatchY = CenterPatchCoordinate.Y - KalmalaGameMode::PlayerTerrainPatchRadius; PatchY <= CenterPatchCoordinate.Y + KalmalaGameMode::PlayerTerrainPatchRadius; ++PatchY)
    {
        for (int32 PatchX = CenterPatchCoordinate.X - KalmalaGameMode::PlayerTerrainPatchRadius; PatchX <= CenterPatchCoordinate.X + KalmalaGameMode::PlayerTerrainPatchRadius; ++PatchX)
        {
            ActivateTerrainPatch(FIntPoint(PatchX, PatchY));
        }
    }
}

void AKalmalaGameMode::RefreshTerrainPatchNeighborhoods()
{
    if (!HasAuthority() || GetWorld() == nullptr) return;

    // Keep only the bounded union of server-observed player neighborhoods. This
    // recycles old generated collision/render patches as players cross open sea;
    // it neither expands streaming density nor allows a client to select patches.
    TSet<FIntPoint> RequiredPatches;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        const APawn* Pawn = Iterator->Get() != nullptr ? Iterator->Get()->GetPawn() : nullptr;
        if (Pawn == nullptr) continue;
        const FIntPoint Centre = FKalmalaTerrainPatchLayout::GetPatchCoordinate(TerrainPatchOrigin, FVector2D(Pawn->GetActorLocation()));
        for (int32 Y = Centre.Y - KalmalaGameMode::PlayerTerrainPatchRadius; Y <= Centre.Y + KalmalaGameMode::PlayerTerrainPatchRadius; ++Y)
        {
            for (int32 X = Centre.X - KalmalaGameMode::PlayerTerrainPatchRadius; X <= Centre.X + KalmalaGameMode::PlayerTerrainPatchRadius; ++X)
            {
                RequiredPatches.Add(FIntPoint(X, Y));
            }
        }
    }

    for (auto Iterator = ActiveTerrainPatches.CreateIterator(); Iterator; ++Iterator)
    {
        if (!RequiredPatches.Contains(Iterator.Key()))
        {
            if (AKalmalaGeneratedTerrainPatch* Patch = Iterator.Value()) Patch->Destroy();
            ActiveTerrainPatchCoordinates.Remove(Iterator.Key());
            Iterator.RemoveCurrent();
        }
    }
    for (const FIntPoint& PatchCoordinate : RequiredPatches)
    {
        ActivateTerrainPatch(PatchCoordinate);
    }
}

AActor* AKalmalaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    return GeneratedPlayerStart != nullptr ? GeneratedPlayerStart : Super::ChoosePlayerStart_Implementation(Player);
}
