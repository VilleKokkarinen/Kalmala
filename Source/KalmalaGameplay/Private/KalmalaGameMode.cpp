#include "KalmalaGameMode.h"

#include "KalmalaCharacter.h"
#include "KalmalaCampfire.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaHazardSpawn.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldPopulationMarker.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "KalmalaWeatherCycle.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaCampConditionSampler.h"
#include "KalmalaShelterSampler.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaTerrainHeightSampler.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace KalmalaGameMode
{
    constexpr int32 PlayerTerrainPatchRadius = 1;
    constexpr int32 MaxActiveTerrainPatches = 25;
    constexpr int32 MaxActivePopulationSpatialKeys = 9;
    constexpr float TerrainPatchActivationIntervalSeconds = 1.0f;
    constexpr float ExposureUpdateIntervalSeconds = 1.0f;
    constexpr float TraversalTestSpeed = 1800.0f;
    constexpr float TraversalTestArrivalDistance = 180.0f;
    constexpr int32 TraversalTargetSearchExtent = 12000;
    constexpr int32 TraversalTargetSearchStep = 250;
    const FString PopulationSaveSlot = TEXT("KalmalaPopulationDeltas");
}

void AKalmalaGameMode::UpdatePlayerExposure(const float DeltaSeconds)
{
    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr)
    {
        return;
    }

    const FKalmalaWeatherState& Weather = WorldGenerationState->GetWeatherState();
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        AKalmalaCharacter* Character = Iterator->Get() != nullptr ? Cast<AKalmalaCharacter>(Iterator->Get()->GetPawn()) : nullptr;
        if (Character == nullptr)
        {
            continue;
        }

        const FVector Location = Character->GetActorLocation();
        const FKalmalaEnvironmentalExposureSample Environment = FKalmalaEnvironmentalExposureSampler::Sample(WorldGenerationConfig, FVector2D(Location));
        const FKalmalaShelterSample Shelter = FKalmalaShelterSampler::Sample(GetWorld(), Character, Environment.NaturalCover, Weather.WindDirectionDegrees);
        float FireWarmth = 0.0f;
        for (TActorIterator<AKalmalaCampfire> CampfireIterator(GetWorld()); CampfireIterator; ++CampfireIterator)
        {
            FireWarmth = FMath::Max(FireWarmth, CampfireIterator->GetWarmthContributionAt(Location));
        }

        FKalmalaExposureState State = Character->GetExposureState();
        State.Wetness = FKalmalaExposureResponse::AdvanceWetness(State.Wetness, Weather.PrecipitationIntensity, Environment.GroundWetness, Environment.WindExposure * Weather.WindStrength, Shelter.Shelter, FireWarmth, DeltaSeconds);
        State.Warmth = FKalmalaExposureResponse::AdvanceWarmth(State.Warmth, Environment.AmbientTemperature, State.Wetness, Environment.WindExposure * Weather.WindStrength, Shelter.Shelter, FireWarmth, DeltaSeconds);
        State.TravelSpeedMultiplier = FKalmalaExposureResponse::GetTravelSpeedMultiplier(State.Warmth);
        Character->SetExposureStateFromServer(State);
    }
}

AKalmalaGameMode::AKalmalaGameMode()
{
    bUseSeamlessTravel = true;
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AKalmalaCharacter::StaticClass();
    GameStateClass = AKalmalaWorldGenerationGameState::StaticClass();
}

void AKalmalaGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    const AKalmalaWorldGenerationGameState* WorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldGenerationState == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create the generated player start because world identity is unavailable."));
        return;
    }

    WorldGenerationConfig = WorldGenerationState->GetWorldGenerationConfig();
    InitializeWeatherCycle();
    PopulationSaveGame = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromSlot(KalmalaGameMode::PopulationSaveSlot, 0));
    if (PopulationSaveGame == nullptr || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame = NewObject<UKalmalaWorldPopulationSaveGame>(this);
        PopulationSaveGame->InitializeForWorld(WorldGenerationConfig);
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
        UE_LOG(LogTemp, Display, TEXT("Server activated %d seed-derived terrain patches around the generated start."), ActiveTerrainPatchCoordinates.Num());
        ConfigureTraversalTest();
        bExposureInspectionEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaExposureInspection"));
        bCampConditionInspectionEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampConditionInspection"));
        if (!ReconnectVerificationMode.IsEmpty())
        {
            AKalmalaCharacter* VerificationPawn = GetWorld()->SpawnActor<AKalmalaCharacter>(
                AKalmalaCharacter::StaticClass(), GeneratedPlayerStart->GetActorLocation(), FRotator::ZeroRotator, SpawnParameters);
            RunReconnectVerification(VerificationPawn);
        }
    }
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

void AKalmalaGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || !WorldGenerationConfig.IsValid() || GetWorld() == nullptr)
    {
        return;
    }

    DriveTraversalTest();
    AdvanceWeatherCycleIfNeeded();

    if (GetWorld()->GetTimeSeconds() >= NextExposureUpdateTime)
    {
        NextExposureUpdateTime = GetWorld()->GetTimeSeconds() + KalmalaGameMode::ExposureUpdateIntervalSeconds;
        UpdatePlayerExposure(KalmalaGameMode::ExposureUpdateIntervalSeconds);
    }

    if (GetWorld()->GetTimeSeconds() < NextTerrainPatchActivationTime)
    {
        return;
    }

    NextTerrainPatchActivationTime = GetWorld()->GetTimeSeconds() + KalmalaGameMode::TerrainPatchActivationIntervalSeconds;
    for (FConstPlayerControllerIterator PlayerControllerIterator = GetWorld()->GetPlayerControllerIterator(); PlayerControllerIterator; ++PlayerControllerIterator)
    {
        const APlayerController* PlayerController = PlayerControllerIterator->Get();
        const APawn* PlayerPawn = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
        if (PlayerPawn != nullptr)
        {
            ActivateTerrainPatchNeighborhood(FVector2D(PlayerPawn->GetActorLocation()));
            ActivatePopulationKey(FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(PlayerPawn->GetActorLocation())));
        }
    }
}

void AKalmalaGameMode::ActivatePopulationKey(const FIntPoint& SpatialKey)
{
    if (!HasAuthority() || GetWorld() == nullptr || ActivePopulationSpatialKeys.Contains(SpatialKey) || ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
    {
        return;
    }

    int32 SpawnedMarkerCount = 0;
    for (const EKalmalaWorldPopulationKind Kind : { EKalmalaWorldPopulationKind::Wildlife, EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Hazard })
    {
        for (const FKalmalaWorldPopulationSpawn& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, Kind))
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

    ActivePopulationSpatialKeys.Add(SpatialKey);
    UE_LOG(LogTemp, Display, TEXT("Server activated %d deterministic population markers for spatial key (%d, %d); %d/%d active."), SpawnedMarkerCount, SpatialKey.X, SpatialKey.Y, ActivePopulationSpatialKeys.Num(), KalmalaGameMode::MaxActivePopulationSpatialKeys);
}

void AKalmalaGameMode::RecordHarvestedSpawn(const FString& PersistentSpawnId)
{
    if (HasAuthority() && PopulationSaveGame != nullptr && PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame->MarkHarvested(PersistentSpawnId);
        UGameplayStatics::SaveGameToSlot(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot, 0);
    }
}

void AKalmalaGameMode::RecordDefeatedSpawn(const FString& PersistentSpawnId)
{
    if (HasAuthority() && PopulationSaveGame != nullptr && PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame->MarkDefeated(PersistentSpawnId);
        UGameplayStatics::SaveGameToSlot(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot, 0);
    }
}

void AKalmalaGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if ((!bTraversalTestEnabled && ReconnectVerificationMode.IsEmpty() && !bExposureInspectionEnabled && !bCampConditionInspectionEnabled) || NewPlayer == nullptr)
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

    UE_LOG(LogTemp, Display, TEXT("Developer verification server player joined with pawn: %s."), *GetNameSafe(NewPlayer->GetPawn()));
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

    const FIntPoint SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(ServerPawn->GetActorLocation()));
    if (ReconnectVerificationMode.Equals(TEXT("WildlifeDefeat"), ESearchCase::IgnoreCase) || ReconnectVerificationMode.Equals(TEXT("WildlifeVerify"), ESearchCase::IgnoreCase))
    {
        const TArray<FKalmalaWorldPopulationSpawn> WildlifeSpawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife);
        if (WildlifeSpawns.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("Reconnect verification found no generated wildlife spawn for its server spatial key."));
            FPlatformMisc::RequestExit(false);
            return;
        }

        const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(WildlifeSpawns[0]);
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

void AKalmalaGameMode::ActivateTerrainPatch(const FIntPoint& PatchCoordinate)
{
    if (!HasAuthority() || ActiveTerrainPatchCoordinates.Contains(PatchCoordinate) || ActiveTerrainPatchCoordinates.Num() >= KalmalaGameMode::MaxActiveTerrainPatches || GetWorld() == nullptr)
    {
        return;
    }

    AKalmalaGeneratedTerrainPatch* TerrainPatch = GetWorld()->SpawnActor<AKalmalaGeneratedTerrainPatch>(AKalmalaGeneratedTerrainPatch::StaticClass());
    if (TerrainPatch == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not activate terrain patch (%d, %d)."), PatchCoordinate.X, PatchCoordinate.Y);
        return;
    }

    TerrainPatch->Initialize(WorldGenerationConfig, FKalmalaTerrainPatchLayout::GetPatchCenter(TerrainPatchOrigin, PatchCoordinate.X, PatchCoordinate.Y));
    ActiveTerrainPatchCoordinates.Add(PatchCoordinate);
    UE_LOG(LogTemp, Display, TEXT("Server activated terrain patch (%d, %d); %d/%d active."), PatchCoordinate.X, PatchCoordinate.Y, ActiveTerrainPatchCoordinates.Num(), KalmalaGameMode::MaxActiveTerrainPatches);
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

AActor* AKalmalaGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    return GeneratedPlayerStart != nullptr ? GeneratedPlayerStart : Super::ChoosePlayerStart_Implementation(Player);
}
