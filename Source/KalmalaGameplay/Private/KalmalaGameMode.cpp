#include "KalmalaGameMode.h"
#include "Camera/CameraActor.h"
#include "UnrealClient.h"

#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaDiscoveryActor.h"
#include "KalmalaDiscoveryProgressComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaMapAwarenessComponent.h"
#include "KalmalaCampfire.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaStorageSaveGame.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaInteractionGrid.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaHazardSpawn.h"
#include "KalmalaOceanDiscoveryCatalogue.h"
#include "KalmalaM9ExplorationRewardCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaM7PersistenceContract.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWolfPackContract.h"
#include "KalmalaGeneratedTerrainPatch.h"
#include "KalmalaForestLayout.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWorldGenerationGameState.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldPopulationMarker.h"
#include "KalmalaWorldPopulationSaveGame.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaM9SourceLootContract.h"
#include "KalmalaWeatherCycle.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaCampConditionSampler.h"
#include "KalmalaBiomeExpansionContract.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaShelterSampler.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaIslandLocator.h"
#include "KalmalaOceanTravelTestFixture.h"
#include "KalmalaOceanTravelFeedbackComponent.h"
#include "KalmalaOceanTravelPersistenceContract.h"
#include "KalmalaOceanSkiff.h"
#include "KalmalaOceanDiscoveryDisembarkPeerTest.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Crc.h"
#include "OnlineSubsystemTypes.h"

namespace KalmalaGameMode
{
    constexpr int32 PlayerTerrainPatchRadius = 1;
    constexpr int32 MaxActiveTerrainPatches = 25;
    constexpr int32 MaxActivePopulationSpatialKeys = 9;
    constexpr int32 MaxSessionM9ExplorationClaimsPerPlayer = 64;
    constexpr float TerrainPatchActivationIntervalSeconds = 1.0f;
    constexpr float ExposureUpdateIntervalSeconds = 1.0f;
    constexpr float InteractionGridUpdateIntervalSeconds = 1.0f;
    constexpr float TraversalTestSpeed = 1800.0f;
    constexpr float TraversalTestArrivalDistance = 180.0f;
    constexpr int32 TraversalTargetSearchExtent = 12000;
    constexpr int32 TraversalTargetSearchStep = 250;
    bool TryGetWildlifeArchetypeForSpawn(const FKalmalaWorldGenerationConfig& Config,
        const FKalmalaWorldPopulationSpawn& Spawn, EKalmalaWildlifeArchetype& OutArchetype)
    {
        const FVector2D Position(Spawn.Location);
        const EKalmalaBiome SpawnBiome = FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(Config, Position));
        if (!AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Spawn.SpawnSeed, SpawnBiome, OutArchetype))
        {
            return false;
        }
        if (OutArchetype == EKalmalaWildlifeArchetype::Mireling
            || OutArchetype == EKalmalaWildlifeArchetype::DeathAdder
            || OutArchetype == EKalmalaWildlifeArchetype::MosquitoSwarm
            || OutArchetype == EKalmalaWildlifeArchetype::PeatHusk)
        {
            const bool bWater = FKalmalaOceanSampler::Sample(Config, Position).IsWater()
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
            return AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(SpawnBiome, bWater);
        }
        if (OutArchetype == EKalmalaWildlifeArchetype::Viper
            || OutArchetype == EKalmalaWildlifeArchetype::GiantSnake)
        {
            const bool bWater = FKalmalaOceanSampler::Sample(Config, Position).IsWater()
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
            const FKalmalaEnvironmentalExposureSample Exposure = FKalmalaEnvironmentalExposureSampler::Sample(Config, Position);
            return SpawnBiome == EKalmalaBiome::ShimmeringLakes && !bWater && Exposure.bIsShoreline;
        }
        if (OutArchetype == EKalmalaWildlifeArchetype::Rat)
        {
            return SpawnBiome == EKalmalaBiome::Meadows
                && !FKalmalaOceanSampler::Sample(Config, Position).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
        }
        if (OutArchetype == EKalmalaWildlifeArchetype::Wolf
            || OutArchetype == EKalmalaWildlifeArchetype::Lynx
            || OutArchetype == EKalmalaWildlifeArchetype::PolarBear)
        {
            return SpawnBiome == EKalmalaBiome::FreezingTundra
                && !FKalmalaOceanSampler::Sample(Config, Position).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position)
                && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.82f;
        }
        if (OutArchetype == EKalmalaWildlifeArchetype::HornedGoat
            || OutArchetype == EKalmalaWildlifeArchetype::ThunderSentinel)
        {
            const bool bWater = FKalmalaOceanSampler::Sample(Config, Position).IsWater()
                || FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
            const bool bTraversable = FKalmalaWorldBounds::Contains(Config, Position)
                && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(Config, Position).Z >= 0.82f;
            return AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
                SpawnBiome, bWater, bTraversable);
        }
        if (OutArchetype == EKalmalaWildlifeArchetype::ReefMaw)
        {
            const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(Config, Position);
            const bool bLakeWater = FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
            return Ocean.IsWater() && AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(
                SpawnBiome, bLakeWater, Ocean.WaterDepth);
        }
        if (OutArchetype == EKalmalaWildlifeArchetype::Badger
            || OutArchetype == EKalmalaWildlifeArchetype::MoonBear)
        {
            return SpawnBiome == EKalmalaBiome::Elderwood
                && !FKalmalaOceanSampler::Sample(Config, Position).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(Config, Position);
        }
        return true;
    }

    bool IsWildlifeSpawnOfArchetype(const FKalmalaWorldGenerationConfig& Config,
        const FKalmalaWorldPopulationSpawn& Spawn, const EKalmalaWildlifeArchetype ExpectedArchetype)
    {
        EKalmalaWildlifeArchetype Archetype;
        return TryGetWildlifeArchetypeForSpawn(Config, Spawn, Archetype) && Archetype == ExpectedArchetype;
    }

    TArray<FKalmalaWorldPopulationSpawn> FilterEligibleWildlifeSpawns(const FKalmalaWorldGenerationConfig& Config,
        const TArray<FKalmalaWorldPopulationSpawn>& Spawns)
    {
        TArray<FKalmalaWorldPopulationSpawn> EligibleSpawns;
        EligibleSpawns.Reserve(Spawns.Num());
        for (const FKalmalaWorldPopulationSpawn& Spawn : Spawns)
        {
            EKalmalaWildlifeArchetype Archetype;
            if (TryGetWildlifeArchetypeForSpawn(Config, Spawn, Archetype)) EligibleSpawns.Add(Spawn);
        }
        return EligibleSpawns;
    }

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
    FString OceanDiscoverySaveSlot(const FKalmalaM7SaveIdentity& Identity)
    {
        return FString::Printf(TEXT("KalmalaOceanClaims_%llu_%d_%08x"), Identity.WorldSeed, Identity.GeneratorRevision, FCrc::StrCrc32(*Identity.OwnerIdentity));
    }
    FString OceanTravelWorldSaveSlot(const FKalmalaM7SaveIdentity& Identity)
    {
        return FString::Printf(TEXT("KalmalaOceanTravelWorld_%llu_%d"), Identity.WorldSeed, Identity.GeneratorRevision);
    }
    FString OceanTravelPlayerSaveSlot(const FKalmalaM7SaveIdentity& Identity)
    {
        return FString::Printf(TEXT("KalmalaOceanTravelPlayer_%llu_%d_%08x"), Identity.WorldSeed,
            Identity.GeneratorRevision, FCrc::StrCrc32(*Identity.OwnerIdentity));
    }
    FString GetAuthenticatedPlayerIdentity(const APlayerState* PlayerState)
    {
        if (PlayerState == nullptr) return FString();
        const FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
        if (!UniqueId.IsValid()) return FString();
        const TSharedPtr<const FUniqueNetId> AuthenticatedId = UniqueId.GetUniqueNetId();
        if (!AuthenticatedId.IsValid()) return FString();
        const FString Identity = AuthenticatedId->GetType().ToString() + TEXT(":") + AuthenticatedId->ToString();
        return Identity.Len() <= FKalmalaM7SaveIdentity::MaxOwnerIdentityLength ? Identity : FString();
    }
}

namespace KalmalaOceanJourneyPeer
{
    constexpr double LaunchDepthCm = 180.0;
    constexpr double RouteLengthCm = 240000.0;
    constexpr double RouteSampleSpacingCm = 5000.0;
    constexpr double RadialScanSpacingCm = 10000.0;

    bool FindRoute(const FKalmalaWorldGenerationConfig& Config, const FVector2D Origin,
        const FVector2D PatchOrigin, FVector2D& OutLaunch, float& OutYaw)
    {
        constexpr int32 RayCount = 72;
        constexpr double MaximumSearchDistanceCm = FKalmalaWorldBounds::Radius - 200000.0;
        for (int32 RayIndex = 0; RayIndex < RayCount; ++RayIndex)
        {
            const float RayYaw = 360.0f * static_cast<float>(RayIndex) / RayCount;
            const FVector2D RayDirection(FMath::Cos(FMath::DegreesToRadians(RayYaw)),
                FMath::Sin(FMath::DegreesToRadians(RayYaw)));
            double PreviousDistance = 0.0;
            float PreviousDepth = FKalmalaOceanSampler::Sample(Config, Origin).WaterDepth;
            for (double Distance = RadialScanSpacingCm; Distance <= MaximumSearchDistanceCm;
                Distance += RadialScanSpacingCm)
            {
                const FVector2D Probe = Origin + RayDirection * Distance;
                const float ProbeDepth = FKalmalaOceanSampler::Sample(Config, Probe).WaterDepth;
                if (PreviousDepth < LaunchDepthCm && ProbeDepth >= LaunchDepthCm)
                {
                    double Low = PreviousDistance;
                    double High = Distance;
                    for (int32 Iteration = 0; Iteration < 20; ++Iteration)
                    {
                        const double Mid = (Low + High) * 0.5;
                        if (FKalmalaOceanSampler::Sample(Config, Origin + RayDirection * Mid).WaterDepth
                            < LaunchDepthCm)
                        {
                            Low = Mid;
                        }
                        else
                        {
                            High = Mid;
                        }
                    }

                    const FVector2D Launch = Origin + RayDirection * High;
                    const FKalmalaOceanSample LaunchSample = FKalmalaOceanSampler::Sample(Config, Launch);
                    if (FMath::Abs(LaunchSample.WaterDepth - LaunchDepthCm) <= 12.0f)
                    {
                        for (int32 HeadingStep = 0; HeadingStep < 24; ++HeadingStep)
                        {
                            const float Yaw = HeadingStep * 15.0f;
                            if (!AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(Config, Launch, Yaw))
                            {
                                continue;
                            }

                            const FVector2D Direction(FMath::Cos(FMath::DegreesToRadians(Yaw)),
                                FMath::Sin(FMath::DegreesToRadians(Yaw)));
                            bool bRouteOpen = true;
                            for (double AlongRoute = RouteSampleSpacingCm; AlongRoute <= RouteLengthCm;
                                AlongRoute += RouteSampleSpacingCm)
                            {
                                const FVector2D Candidate = Launch + Direction * AlongRoute;
                                if (!AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(Config, Candidate, Yaw))
                                {
                                    bRouteOpen = false;
                                    break;
                                }
                            }

                            const FIntPoint StartPatch = FKalmalaTerrainPatchLayout::GetPatchCoordinate(PatchOrigin, Launch);
                            const FIntPoint EndPatch = FKalmalaTerrainPatchLayout::GetPatchCoordinate(
                                PatchOrigin, Launch + Direction * RouteLengthCm);
                            if (bRouteOpen && StartPatch != EndPatch)
                            {
                                OutLaunch = Launch;
                                OutYaw = Yaw;
                                return true;
                            }
                        }
                    }
                }
                PreviousDistance = Distance;
                PreviousDepth = ProbeDepth;
            }
        }
        return false;
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
        AddNeighborhood(Hearth->GetActorLocation(), FKalmalaInteractionGrid::CampFireRadiusCells);
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
        Character->AdvanceHeldTorchFromServer(DeltaSeconds,
            Weather.PrecipitationIntensity >= 0.05f && !Shelter.bHasRoof);
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
                const bool bHighlyActiveWeather = Weather.ActivityLevel == EKalmalaWeatherActivityLevel::HighlyActive;
                const float WetTriggerSeconds = FKalmalaExposureResponse::GetUnroofedRainTriggerSeconds(
                    UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds, bHighlyActiveWeather);
                UnroofedRainSeconds = FMath::Clamp(UnroofedRainSeconds + FMath::Max(0.0f, DeltaSeconds), 0.0f, UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds);
                if (UnroofedRainSeconds >= WetTriggerSeconds) Statuses->ApplyWetFromServer();
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
#if !UE_BUILD_SHIPPING
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
        {
            // Keep the server-owned Wet entry observable alongside the lit test hearth.
            if (UKalmalaPlayerStatusComponent* Statuses = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>())
            {
                Statuses->ApplyWetFromServer();
            }
        }
#endif

        FKalmalaExposureState State = Character->GetExposureState();
        State.Wetness = FKalmalaExposureResponse::AdvanceWetness(State.Wetness, Weather.PrecipitationIntensity, Environment.GroundWetness, Environment.WindExposure * Weather.WindStrength, Shelter.Shelter, FireWarmth, DeltaSeconds);
        State.Warmth = FKalmalaExposureResponse::AdvanceWarmth(State.Warmth, Environment.AmbientTemperature, State.Wetness, Environment.WindExposure * Weather.WindStrength, Shelter.Shelter, FireWarmth, DeltaSeconds);
        State.HeatIntensity = FKalmalaExposureResponse::GetHeatIntensity(Environment.AmbientTemperature);
        State.ColdIntensity = FKalmalaExposureResponse::GetColdIntensity(Environment.AmbientTemperature);
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
            UE_LOG(LogTemp, Display, TEXT("Camp choice server %s: Wetness=%.2f Warmth=%.2f Heat=%.2f Cold=%.2f Travel=%.2f."), *FString::FromInt(Character->GetPlayerState()->GetPlayerId()), State.Wetness, State.Warmth, State.HeatIntensity, State.ColdIntensity, State.TravelSpeedMultiplier);
        }
        if (bExposureReplicationTestEnabled)
        {
            UE_LOG(LogTemp, Display, TEXT("Exposure replication test server state for %s: Weather=%d/%.2f/%d/%.2f Shelter=%.2f FireWarmth=%.2f Wetness=%.2f Warmth=%.2f Heat=%.2f Cold=%.2f TravelMultiplier=%.2f."), *Character->GetName(), Weather.WeatherCycleIndex, Weather.PrecipitationIntensity, Weather.WindDirectionDegrees, Weather.WindStrength, Shelter.Shelter, FireWarmth, State.Wetness, State.Warmth, State.HeatIntensity, State.ColdIntensity, State.TravelSpeedMultiplier);
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
#if !UE_BUILD_SHIPPING
    bM9Schema2CandidatePeerTestEnabled = FParse::Param(FCommandLine::Get(), TEXT("KalmalaM9Schema2PeerTest"));
    if (bM9Schema2CandidatePeerTestEnabled)
    {
        FParse::Value(FCommandLine::Get(), TEXT("KalmalaM9Schema2Phase="), M9Schema2CandidatePeerTestPhase);
    }
#endif
    LoadOceanTravelWorldSave();
    InitializeWeatherCycle();
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest")))
    {
        FKalmalaWeatherState VerificationWeather;
        VerificationWeather.WeatherCycleIndex = 0;
        VerificationWeather.ServerStartTimeSeconds = GetWorld()->GetTimeSeconds();
        VerificationWeather.DurationSeconds = 180.0f;
        VerificationWeather.PrecipitationIntensity = 0.04f;
        VerificationWeather.WindDirectionDegrees = 45;
        VerificationWeather.WindStrength = 0.80f;
        if (AKalmalaWorldGenerationGameState* MutableWorldGenerationState = GetGameState<AKalmalaWorldGenerationGameState>())
        {
            MutableWorldGenerationState->SetWeatherStateFromServer(VerificationWeather);
        }
    }
#endif
    LoadPopulationSave();
    const FString ConstructionSlot = KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig);
    const bool bConstructionSaveExists = UGameplayStatics::DoesSaveGameExist(ConstructionSlot, 0);
    USaveGame* ExistingConstructionSave = bConstructionSaveExists
        ? UGameplayStatics::LoadGameFromSlot(ConstructionSlot, 0) : nullptr;
    UKalmalaConstructionSaveGameV2* LoadedConstructionSave = nullptr;
    if ((!bConstructionSaveExists || ExistingConstructionSave != nullptr)
        && UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
            ExistingConstructionSave, WorldGenerationConfig, this, LoadedConstructionSave))
    {
        ConstructionSaveGame = LoadedConstructionSave;
        UE_LOG(LogTemp, Display, TEXT("Construction save ready: Schema=%d Records=%d"),
            ConstructionSaveGame->GetSchemaVersion(), ConstructionSaveGame->GetRecords().Num());
    }
    else
    {
        ConstructionSaveGame = nullptr;
        UE_LOG(LogTemp, Warning, TEXT("Construction unavailable: existing save is invalid or incompatible; preserved without overwrite."));
    }
#if !UE_BUILD_SHIPPING
    if (bM9Schema2CandidatePeerTestEnabled)
    {
        RunM9Schema2CandidateWorldPeerTest();
    }
#endif
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
        SpawnOceanTravelTestFixture();
        RestorePersistedConstruction();
        RestorePersistedOceanSkiff();
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

void AKalmalaGameMode::LoadPopulationSave()
{
    const FString Slot = KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig);
    PopulationSaveGame = nullptr;
    if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
    {
        auto* Loaded = Cast<UKalmalaWorldPopulationSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
        if (!Loaded || !Loaded->MatchesWorld(WorldGenerationConfig))
        {
            UE_LOG(LogTemp, Warning, TEXT("Population save rejected: existing data is incompatible or invalid; preserved without overwrite."));
            return;
        }
        PopulationSaveGame = Loaded;
        return;
    }
    PopulationSaveGame = NewObject<UKalmalaWorldPopulationSaveGame>(this);
    PopulationSaveGame->InitializeForWorld(WorldGenerationConfig);
}

void AKalmalaGameMode::SpawnOceanTravelTestFixture()
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || GetWorld() == nullptr || !FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanTravelTest")) || GeneratedPlayerStart == nullptr)
    {
        return;
    }

    FVector2D IslandTarget = FVector2D::ZeroVector;
    if (!FKalmalaIslandLocator::FindNearest(WorldGenerationConfig, TerrainPatchOrigin, IslandTarget))
    {
        UE_LOG(LogTemp, Error, TEXT("Ocean travel test fixture could not resolve its seeded island endpoint."));
        return;
    }

    const FVector2D TravelDirection = (IslandTarget - TerrainPatchOrigin).GetSafeNormal();
    const FVector2D EntryPoint = TerrainPatchOrigin + TravelDirection * 1000.0f;
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    OceanTravelTestFixture = GetWorld()->SpawnActor<AKalmalaOceanTravelTestFixture>(
        AKalmalaOceanTravelTestFixture::StaticClass(), FVector(EntryPoint, 0.0f), FRotator::ZeroRotator, SpawnParameters);
    if (OceanTravelTestFixture == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Ocean travel test fixture could not be spawned."));
        return;
    }

    OceanTravelTestFixture->Initialize(EntryPoint, IslandTarget, 900.0f, 220.0f);
    UE_LOG(LogTemp, Display, TEXT("Ocean travel test spawned an authoritative deep-water fixture from %s to seeded island %s at depth %.0f."),
        *EntryPoint.ToString(), *IslandTarget.ToString(), 220.0f);
#endif
}

bool AKalmalaGameMode::CanPersistConstruction(const FName KitId, const FTransform& Transform) const
{
    if (!HasAuthority() || ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    FKalmalaConstructionSaveRecord Candidate;
    Candidate.ConstructionId = TEXT("pending");
    Candidate.KitId = KitId;
    Candidate.Transform = Transform;
    if (!UKalmalaConstructionSaveGameV2::IsValidRecord(Candidate)
        || ConstructionSaveGame->GetRecords().Num() >= UKalmalaConstructionSaveGameV2::MaxRecords) return false;
    if (FKalmalaToolProgressionContract::IsStationAttachmentKit(KitId))
    {
        int32 AttachmentCount = 0;
        for (const FKalmalaConstructionSaveRecord& ExistingRecord : ConstructionSaveGame->GetRecords())
        {
            AttachmentCount += FKalmalaToolProgressionContract::IsStationAttachmentKit(ExistingRecord.KitId) ? 1 : 0;
        }
        if (AttachmentCount >= UKalmalaConstructionSaveGameV2::MaxStationAttachments) return false;
    }
    return true;
}

bool AKalmalaGameMode::PersistConstruction(AKalmalaConstructionActor* Construction)
{
    if (!HasAuthority() || !IsValid(Construction) || ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    FKalmalaConstructionSaveRecord Record;
    Record.ConstructionId = Construction->GetConstructionId();
    Record.KitId = Construction->GetConstructionKit();
    Record.Transform = Construction->GetActorTransform();
    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    const bool bCandidateReady = UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
        ConstructionSaveGame, WorldGenerationConfig, this, Candidate);
    const bool bAdded = bCandidateReady && (FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId)
        ? Candidate->AddStationAttachmentRecord(Record) : Candidate->AddRecord(Record));
    if (!bAdded || !Candidate->MatchesWorld(WorldGenerationConfig)) return false;
    if (!UGameplayStatics::SaveGameToSlot(Candidate, KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Construction save failed for %s."), *Record.ConstructionId);
        return false;
    }
    ConstructionSaveGame = Candidate;
    return true;
}

bool AKalmalaGameMode::CanReplaceWallWithDoor(const AKalmalaConstructionActor* Wall) const
{
    if (!HasAuthority() || !IsValid(Wall) || !Wall->HasAuthority() || Wall->GetWorld() != GetWorld()
        || Wall->GetConstructionKit() != TEXT("WallKit") || Wall->GetConstructionId().IsEmpty()
        || ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    const FString ConstructionId = Wall->GetConstructionId();
    const FKalmalaConstructionSaveRecord* Record = ConstructionSaveGame->GetRecords().FindByPredicate(
        [&ConstructionId](const FKalmalaConstructionSaveRecord& Candidate)
        {
            return Candidate.ConstructionId == ConstructionId && Candidate.KitId == TEXT("WallKit");
        });
    return Record != nullptr && Record->Transform.Equals(Wall->GetActorTransform());
}

bool AKalmalaGameMode::ReplaceWallWithDoor(AKalmalaConstructionActor* Wall)
{
    if (!CanReplaceWallWithDoor(Wall)) return false;
    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    if (!UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
        ConstructionSaveGame, WorldGenerationConfig, this, Candidate) || !Candidate
        || !Candidate->ReplaceWallRecordWithDoor(Wall->GetConstructionId(), Wall->GetActorTransform())
        || !Candidate->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(Candidate,
            KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Timber Door replacement save failed for %s."),
            *GetNameSafe(Wall));
        return false;
    }
    ConstructionSaveGame = Candidate;
    return Wall->ReplaceWallWithDoorFromServer();
}

bool AKalmalaGameMode::PersistDoorOpenState(AKalmalaConstructionActor* Door, const bool bOpen)
{
    if (!HasAuthority() || !IsValid(Door) || !Door->HasAuthority() || Door->GetWorld() != GetWorld()
        || Door->GetConstructionKit() != TEXT("TimberDoorKit") || Door->GetConstructionId().IsEmpty()
        || Door->IsDoorOpen() == bOpen || ConstructionSaveGame == nullptr
        || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    if (!UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
        ConstructionSaveGame, WorldGenerationConfig, this, Candidate) || !Candidate
        || !Candidate->UpdateDoorOpenRecord(Door->GetConstructionId(), Door->GetActorTransform(), bOpen)
        || !Candidate->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(Candidate,
            KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Timber Door state save failed for %s."), *GetNameSafe(Door));
        return false;
    }
    ConstructionSaveGame = Candidate;
    return true;
}

bool AKalmalaGameMode::PersistCharcoalKilnState(AKalmalaConstructionActor* Construction,
    const FKalmalaCharcoalKilnState& State)
{
    if (!HasAuthority() || !IsValid(Construction) || !Construction->HasAuthority()
        || Construction->GetWorld() != GetWorld()
        || Construction->GetConstructionKit() != TEXT("CharcoalKilnKit")
        || !State.IsValid() || ConstructionSaveGame == nullptr
        || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    const FString ConstructionId = Construction->GetConstructionId();
    const auto* ExistingRecord = ConstructionSaveGame->GetRecords().FindByPredicate(
        [&ConstructionId](const FKalmalaConstructionSaveRecord& Record)
        {
            return Record.ConstructionId == ConstructionId && Record.KitId == TEXT("CharcoalKilnKit");
        });
    if (!ExistingRecord || !ExistingRecord->Transform.Equals(Construction->GetActorTransform())) return false;

    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    if (!UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
        ConstructionSaveGame, WorldGenerationConfig, this, Candidate) || !Candidate) return false;
    if (!Candidate->UpdateCharcoalKilnRecord(ConstructionId, State)
        || !Candidate->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(Candidate,
            KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Charcoal Kiln state save failed for %s."), *ConstructionId);
        return false;
    }
    ConstructionSaveGame = Candidate;
    return true;
}

bool AKalmalaGameMode::PersistSmelterState(AKalmalaConstructionActor* Construction,
    const FKalmalaSmelterState& State)
{
    if (!HasAuthority() || !IsValid(Construction) || !Construction->HasAuthority()
        || Construction->GetWorld() != GetWorld()
        || Construction->GetConstructionKit() != TEXT("SmelterKit")
        || !State.IsValid() || ConstructionSaveGame == nullptr
        || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    const FString ConstructionId = Construction->GetConstructionId();
    const auto* ExistingRecord = ConstructionSaveGame->GetRecords().FindByPredicate(
        [&ConstructionId](const FKalmalaConstructionSaveRecord& Record)
        {
            return Record.ConstructionId == ConstructionId && Record.KitId == TEXT("SmelterKit");
        });
    if (!ExistingRecord || !ExistingRecord->Transform.Equals(Construction->GetActorTransform())) return false;

    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    if (!UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
        ConstructionSaveGame, WorldGenerationConfig, this, Candidate) || !Candidate) return false;
    if (!Candidate->UpdateSmelterRecord(ConstructionId, State)
        || !Candidate->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(Candidate,
            KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Smelter state save failed for %s."), *ConstructionId);
        return false;
    }
    ConstructionSaveGame = Candidate;
    return true;
}

bool AKalmalaGameMode::PersistGardenPlotState(AKalmalaConstructionActor* Construction,
    const FKalmalaGardenPlotState& State)
{
    if (!HasAuthority() || !IsValid(Construction) || !Construction->HasAuthority()
        || Construction->GetWorld() != GetWorld()
        || Construction->GetConstructionKit() != TEXT("GardenPlotKit")
        || !State.IsValid() || ConstructionSaveGame == nullptr
        || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    const FString ConstructionId = Construction->GetConstructionId();
    const auto* ExistingRecord = ConstructionSaveGame->GetRecords().FindByPredicate(
        [&ConstructionId](const FKalmalaConstructionSaveRecord& Record)
        {
            return Record.ConstructionId == ConstructionId && Record.KitId == TEXT("GardenPlotKit");
        });
    if (!ExistingRecord || !ExistingRecord->Transform.Equals(Construction->GetActorTransform())) return false;

    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    if (!UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
        ConstructionSaveGame, WorldGenerationConfig, this, Candidate) || !Candidate) return false;
    if (!Candidate->UpdateGardenPlotRecord(ConstructionId, State)
        || !Candidate->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(Candidate,
            KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Garden Plot state save failed for %s."), *ConstructionId);
        return false;
    }
    ConstructionSaveGame = Candidate;
    return true;
}

bool AKalmalaGameMode::PersistStandingLightState(AKalmalaConstructionActor* Construction,
    const FKalmalaStandingLightState& State)
{
    if (!HasAuthority() || !IsValid(Construction) || !Construction->HasAuthority()
        || Construction->GetWorld() != GetWorld()
        || Construction->GetConstructionKit() != TEXT("StandingLightKit")
        || !State.IsValid() || ConstructionSaveGame == nullptr
        || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    const FString ConstructionId = Construction->GetConstructionId();
    const auto* ExistingRecord = ConstructionSaveGame->GetRecords().FindByPredicate(
        [&ConstructionId](const FKalmalaConstructionSaveRecord& Record)
        {
            return Record.ConstructionId == ConstructionId && Record.KitId == TEXT("StandingLightKit");
        });
    if (!ExistingRecord || !ExistingRecord->Transform.Equals(Construction->GetActorTransform())) return false;

    UKalmalaConstructionSaveGameV2* Candidate = nullptr;
    if (!UKalmalaConstructionSaveGameV2::TryBuildWriteCandidate(
        ConstructionSaveGame, WorldGenerationConfig, this, Candidate) || !Candidate) return false;
    if (!Candidate->UpdateStandingLightRecord(ConstructionId, State)
        || !Candidate->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(Candidate,
            KalmalaGameMode::ConstructionSaveSlot(WorldGenerationConfig), 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Standing Light state save failed for %s."), *ConstructionId);
        return false;
    }
    ConstructionSaveGame = Candidate;
    return true;
}

bool AKalmalaGameMode::IsTerrainPatchActiveAt(const FVector& WorldLocation) const
{
    if (!HasAuthority() || WorldLocation.ContainsNaN()) return false;
    const FIntPoint PatchCoordinate = FKalmalaTerrainPatchLayout::GetPatchCoordinate(
        TerrainPatchOrigin, FVector2D(WorldLocation));
    return ActiveTerrainPatchCoordinates.Contains(PatchCoordinate);
}

void AKalmalaGameMode::RestorePersistedConstruction()
{
    if (!HasAuthority() || ConstructionSaveGame == nullptr || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return;
    for (const FKalmalaConstructionSaveRecord& Record : ConstructionSaveGame->GetRecords())
    {
        if (!UKalmalaConstructionSaveGameV2::IsValidRecord(Record))
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
        Construction->InitializeFromServer(Record.KitId, Record.ConstructionId,
            Record.CharcoalKiln, Record.Smelter, Record.GardenPlot, Record.bDoorOpen, Record.StandingLight);
        Construction->FinishSpawning(Record.Transform);
        UE_LOG(LogTemp, Display, TEXT("Construction restored: Id=%s Kit=%s."), *Record.ConstructionId, *Record.KitId.ToString());
    }
}

bool AKalmalaGameMode::ReadStorage(const AKalmalaConstructionActor* Construction, TArray<FKalmalaInventoryStack>& Out) const
{
    if (!HasAuthority() || !IsValid(Construction) || !Construction->HasAuthority() || Construction->GetWorld() != GetWorld()) return false;
    if (!AKalmalaConstructionActor::IsStorageKit(Construction->GetConstructionKit()) || !StorageSaveGame || !ConstructionSaveGame
        || !StorageSaveGame->MatchesWorld(WorldGenerationConfig) || !ConstructionSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    // Only a paid, registered construction in this immutable world can address a chest record.
    const auto* Placed = ConstructionSaveGame->GetRecords().FindByPredicate([&](const auto& Record) {
        return Record.ConstructionId == Construction->GetConstructionId()
            && Record.KitId == Construction->GetConstructionKit()
            && Record.Transform.Equals(Construction->GetActorTransform());
    });
    if (!Placed) return false;
    const auto* Stored = StorageSaveGame->FindRecord(Placed->ConstructionId);
    if (Stored && Stored->KitId != Placed->KitId) return false;
    Out = Stored ? Stored->Stacks : TArray<FKalmalaInventoryStack>();
    return true;
}

bool AKalmalaGameMode::PersistStorage(const AKalmalaConstructionActor* Construction, const TArray<FKalmalaInventoryStack>& Stacks)
{
    TArray<FKalmalaInventoryStack> Before;
    if (!ReadStorage(Construction, Before)) return false;
    auto* Candidate = DuplicateObject<UKalmalaStorageSaveGame>(StorageSaveGame, this);
    if (!Candidate || !Candidate->UpsertRecord(Construction->GetConstructionId(), Construction->GetConstructionKit(), Stacks)
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
#if !UE_BUILD_SHIPPING
    if (Weather.WeatherCycleIndex == 7002
        && FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest")))
    {
        return;
    }
#endif
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
            Weather.WindStrength = Rain >= 0.65f ? 1.0f : 0.0f;
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
    const AKalmalaWorldGenerationGameState* RainWeatherState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (RainVerticalSliceStage == 3 && (!RainWeatherState || RainWeatherState->GetWeatherState().ActivityLevel != EKalmalaWeatherActivityLevel::HighlyActive))
    {
        UE_LOG(LogTemp, Error, TEXT("Rain vertical slice FAILED: stage did not establish Highly Active weather."));
        RainVerticalSliceStage = 99;
        return;
    }
    const float HighlyActiveRainTriggerSeconds = FKalmalaExposureResponse::GetUnroofedRainTriggerSeconds(
        UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds, true);
    if (RainVerticalSliceStage == 3 && Now - RainVerticalSliceStageTime >= HighlyActiveRainTriggerSeconds + 1.0f)
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
    const bool bBadgerPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaBadgerPeerTest"));
    const bool bMoonBearPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMoonBearPeerTest"));
    const bool bRatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaRatPeerTest"));
    const bool bViperPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaViperPeerTest"));
    const bool bGiantSnakePeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaGiantSnakePeerTest"));
    const bool bElderwoodRewardPeerTest = bBadgerPeerTest || bMoonBearPeerTest;
    const bool bPendingRewardPeerTest = bElderwoodRewardPeerTest || bRatPeerTest || bViperPeerTest || bGiantSnakePeerTest;
    const bool bTwoItemRewardPeerTest = bRatPeerTest || bViperPeerTest || bGiantSnakePeerTest;
    const bool bShoreSnakePeerTest = bViperPeerTest || bGiantSnakePeerTest;
    const uint32 ExpectedAttackCount = bRatPeerTest ? 1u : 4u;
    if ((!bCombatPeerTest && !bMirelingPeerTest && !bBoarPeerTest && !bDeerPeerTest && !bPendingRewardPeerTest)
        || (bBadgerPeerTest && bMoonBearPeerTest)
        || (bTwoItemRewardPeerTest && (bCombatPeerTest || bMirelingPeerTest || bBoarPeerTest || bDeerPeerTest || bElderwoodRewardPeerTest
            || (bRatPeerTest && bViperPeerTest) || (bRatPeerTest && bGiantSnakePeerTest) || (bViperPeerTest && bGiantSnakePeerTest)))
        || GetWorld() == nullptr)
    {
        return;
    }

    const TCHAR* VerificationName = bRatPeerTest ? TEXT("Rat")
        : bViperPeerTest ? TEXT("Viper")
        : bGiantSnakePeerTest ? TEXT("Giant Snake")
        : bBadgerPeerTest ? TEXT("Badger")
        : bMoonBearPeerTest ? TEXT("Moon Bear")
        : bBoarPeerTest ? TEXT("Boar")
        : bDeerPeerTest ? TEXT("Deer")
        : bMirelingPeerTest ? TEXT("Mireling") : TEXT("Combat");
    const EKalmalaWildlifeArchetype ExpectedArchetype = bRatPeerTest ? EKalmalaWildlifeArchetype::Rat
        : bViperPeerTest ? EKalmalaWildlifeArchetype::Viper
        : bGiantSnakePeerTest ? EKalmalaWildlifeArchetype::GiantSnake
        : bBadgerPeerTest ? EKalmalaWildlifeArchetype::Badger
        : bMoonBearPeerTest ? EKalmalaWildlifeArchetype::MoonBear
        : bBoarPeerTest ? EKalmalaWildlifeArchetype::Boar
        : bDeerPeerTest ? EKalmalaWildlifeArchetype::Deer : EKalmalaWildlifeArchetype::Mireling;

    if (!bCombatPeerTestLogged)
    {
        bCombatPeerTestLogged = true;
        UE_LOG(LogTemp, Display, TEXT("%s verification server fixture enabled."), VerificationName);
    }
    if (CombatPeerTestStage == 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (CombatPeerTestStage == 0)
    {
        APlayerController* LocalController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* LocalServerPlayer = LocalController ? Cast<AKalmalaCharacter>(LocalController->GetPawn()) : nullptr;
        AKalmalaCharacter* RemotePlayer = nullptr;
        int32 RemotePlayerCount = 0;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate->GetPlayerState() == nullptr || Candidate == LocalServerPlayer) continue;
            ++RemotePlayerCount;
            if (RemotePlayer == nullptr) RemotePlayer = Candidate;
        }
        if (LocalServerPlayer == nullptr || RemotePlayer == nullptr || (bTwoItemRewardPeerTest && RemotePlayerCount < 2)) return;

        const auto SelectTestWeapon = [](AKalmalaCharacter* Character, const FName ItemId, FString& OutReason)
        {
            UKalmalaInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
            const FKalmalaToolState* TestWeapon = Character
                ? Character->GetCarriedToolInventory().FindByPredicate(
                    [ItemId](const FKalmalaToolState& Tool) { return Tool.ItemId == ItemId; })
                : nullptr;
            if (Inventory == nullptr || TestWeapon == nullptr)
            {
                OutReason = FString::Printf(TEXT("%s is missing from the initial loadout"), *ItemId.ToString());
                return false;
            }
            return Inventory->SetMainHandFromServer(TestWeapon->InstanceGuid, OutReason);
        };
        FString EquipReason;
        const FName ServerWeaponId = bDeerPeerTest ? FName(TEXT("StoneKnife")) : FName(TEXT("StoneMace"));
        if (!SelectTestWeapon(LocalServerPlayer, ServerWeaponId, EquipReason)
            || !SelectTestWeapon(RemotePlayer, FName(TEXT("StoneMace")), EquipReason))
        {
            UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: test combat weapon could not be selected for both peers (%s)."), VerificationName, *EquipReason);
            CombatPeerTestStage = 99;
            return;
        }
        if (bRatPeerTest || bViperPeerTest || bGiantSnakePeerTest)
        {
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                AKalmalaCharacter* Candidate = *It;
                if (!IsValid(Candidate) || Candidate == LocalServerPlayer || Candidate == RemotePlayer
                    || Candidate->GetPlayerState() == nullptr) continue;
                if (!SelectTestWeapon(Candidate, FName(TEXT("StoneMace")), EquipReason))
                {
                    UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: an observer peer could not equip the test mace (%s)."), VerificationName, *EquipReason);
                    CombatPeerTestStage = 99;
                    return;
                }
            }
        }

        CombatPeerTestAttacker = LocalServerPlayer;
        CombatPeerTestRemote = RemotePlayer;
        FIntPoint SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(LocalServerPlayer->GetActorLocation()));
        const FIntPoint StartingSpatialKey = SpatialKey;
        TArray<FKalmalaWorldPopulationSpawn> Descriptors = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife);
        const int32 MaxSearchRadius = (bMirelingPeerTest || bPendingRewardPeerTest) ? 64 : 1;
        if ((bMirelingPeerTest || bBoarPeerTest || bDeerPeerTest || bPendingRewardPeerTest)
            && !Descriptors.ContainsByPredicate([this, ExpectedArchetype](const FKalmalaWorldPopulationSpawn& Descriptor)
            { return KalmalaGameMode::IsWildlifeSpawnOfArchetype(WorldGenerationConfig, Descriptor, ExpectedArchetype); }))
        {
            // Boar and Deer use the player's adjacent-key set. Mireling, Rat, Viper,
            // Giant Snake and the Elderwood species search a larger bounded ring for a biome-valid descriptor
            // before the fixture activates exactly one population key.
            const FIntPoint BaseKey = SpatialKey;
            bool bFoundArchetypeDescriptor = false;
            for (int32 Radius = 1; Radius <= MaxSearchRadius && !bFoundArchetypeDescriptor; ++Radius)
            {
                for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundArchetypeDescriptor; ++OffsetY)
                {
                    for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                    {
                        if (Radius > 1 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                        const FIntPoint CandidateKey = BaseKey + FIntPoint(OffsetX, OffsetY);
                        const TArray<FKalmalaWorldPopulationSpawn> CandidateDescriptors = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                        if (CandidateDescriptors.ContainsByPredicate([this, ExpectedArchetype](const FKalmalaWorldPopulationSpawn& Descriptor)
                            { return KalmalaGameMode::IsWildlifeSpawnOfArchetype(WorldGenerationConfig, Descriptor, ExpectedArchetype); }))
                        {
                            SpatialKey = CandidateKey;
                            Descriptors = CandidateDescriptors;
                            bFoundArchetypeDescriptor = true;
                            break;
                        }
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
        const FKalmalaWorldPopulationSpawn* ExpectedDescriptor = Descriptors.FindByPredicate([this, ExpectedArchetype](const FKalmalaWorldPopulationSpawn& Descriptor)
        {
            return KalmalaGameMode::IsWildlifeSpawnOfArchetype(WorldGenerationConfig, Descriptor, ExpectedArchetype);
        });
        if (ExpectedDescriptor == nullptr)
        {
            UE_LOG(LogTemp, Error,
                TEXT("%s verification FAILED: no matching wildlife descriptor from start key (%d,%d) within radius %d for world seed %llu."),
                VerificationName, StartingSpatialKey.X, StartingSpatialKey.Y, MaxSearchRadius,
                static_cast<unsigned long long>(WorldGenerationConfig.WorldSeed));
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
                    if (!HerdDescriptors.ContainsByPredicate([this](const FKalmalaWorldPopulationSpawn& Descriptor)
                        { return KalmalaGameMode::IsWildlifeSpawnOfArchetype(WorldGenerationConfig, Descriptor, EKalmalaWildlifeArchetype::Deer); })) continue;
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

        if (bPendingRewardPeerTest)
        {
            UKalmalaInventoryComponent* Inventory = LocalServerPlayer->GetInventoryComponent();
            const UKalmalaItemCatalogue* Catalogue = UKalmalaItemCatalogue::Get();
            if (Inventory == nullptr || Catalogue == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: server inventory or item catalogue is unavailable."), VerificationName);
                CombatPeerTestStage = 99;
                return;
            }
            if (bGiantSnakePeerTest)
            {
                const FName TestArmorId(TEXT("PolarBearCoat"));
                FString ProtectiveEquipReason;
                const bool bArmorGranted = Inventory->TryGrantFromServer(TestArmorId, 1);
                const FKalmalaInventoryStack* ArmorStack = Inventory->GetStacks().FindByPredicate(
                    [TestArmorId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TestArmorId; });
                const FGuid ArmorGuid = ArmorStack != nullptr && ArmorStack->ItemGuids.Num() == 1
                    ? ArmorStack->ItemGuids[0] : FGuid();
                FKalmalaGearLevelStats ArmorStats;
                if (!bArmorGranted || !ArmorGuid.IsValid()
                    || !Inventory->EquipGearFromServer(ArmorGuid, ProtectiveEquipReason)
                    || !Catalogue->GetGearLevelStats(TestArmorId, 1, ArmorStats))
                {
                    UE_LOG(LogTemp, Error, TEXT("Giant Snake verification FAILED: could not equip the catalogue-backed protective coat (%s)."), *ProtectiveEquipReason);
                    CombatPeerTestStage = 99;
                    return;
                }
                UE_LOG(LogTemp, Display, TEXT("Giant Snake verification equipped catalogue-backed %s with %d Armor so the host can continue combat after the real bite."), *TestArmorId.ToString(), ArmorStats.Armor);
            }
            const int32 FullPackCapacity = UKalmalaInventoryComponent::MaxSlots
                - Inventory->CountPackedTools(LocalServerPlayer->GetCarriedToolInventory());
            for (const FKalmalaItemDefinition& Definition : Catalogue->Items)
            {
                if (Inventory->GetStacks().Num() >= FullPackCapacity) break;
                if (Definition.ItemId.IsNone() || Definition.ItemId == TEXT("BoneFragment")
                    || Definition.ItemId == TEXT("BadgerFur") || Definition.ItemId == TEXT("BadgerMeat")
                    || Definition.ItemId == TEXT("RatTail")
                    || Definition.ItemId == TEXT("ViperFang")
                    || Definition.ItemId == TEXT("GiantScale")
                    || Definition.ItemId.ToString().EndsWith(TEXT("Kit"))
                    || Catalogue->FindBuildable(Definition.ItemId) != nullptr
                    || Inventory->GetQuantity(Definition.ItemId) > 0)
                {
                    continue;
                }
                Inventory->TryGrantFromServer(Definition.ItemId, 1);
            }
            if (FullPackCapacity <= 0 || Inventory->GetStacks().Num() != FullPackCapacity)
            {
                UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: could not fill the owner's pack (%d/%d cells)."),
                    VerificationName, Inventory->GetStacks().Num(), FullPackCapacity);
                CombatPeerTestStage = 99;
                return;
            }
        }

        AKalmalaCharacter* Attacker = CombatPeerTestAttacker.Get();
        AKalmalaCharacter* Remote = CombatPeerTestRemote.Get();
        const FVector Forward = Attacker->GetActorForwardVector().GetSafeNormal2D();
        // Keep the boar at its generated rest origin: its normal territorial
        // policy must choose a nearby pawn, rather than a fixture bypassing it.
        // Keep the Rat test host near the ground-level wildlife root so the
        // ordinary forward/low melee traces can acquire the small target.
        const float InitialEncounterDistance = bRatPeerTest ? 1.0f : (bMirelingPeerTest ? 100.0f : 150.0f);
        const auto PlacePeerOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
        {
            if (Character == nullptr) return;
            const FVector2D Position(DesiredLocation);
            ActivateTerrainPatchNeighborhood(Position);
            FVector GroundedLocation = DesiredLocation;
            GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Position)
                + Character->GetSimpleCollisionHalfHeight() + 1.0f;
            Character->SetActorLocation(GroundedLocation, false);
            Character->ForceNetUpdate();
        };
        const auto IsShimmeringLakesDryPosition = [this](const FVector2D& Position)
        {
            return FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position))
                    == EKalmalaBiome::ShimmeringLakes
                && !FKalmalaOceanSampler::Sample(WorldGenerationConfig, Position).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position);
        };
        FVector AttackerLocation = CombatPeerTestTarget->GetActorLocation() - Forward * InitialEncounterDistance;
        if (bShoreSnakePeerTest)
        {
            const FVector SnakeOrigin = CombatPeerTestTarget->GetActorLocation();
            bool bFoundDryPath = false;
            const float Radius = bGiantSnakePeerTest ? 480.0f : 150.0f;
            for (int32 Direction = 0; Direction < 36 && !bFoundDryPath; ++Direction)
            {
                const float Angle = FMath::DegreesToRadians(Direction * 10.0f);
                const FVector2D Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
                const int32 Samples = FMath::Max(1, FMath::CeilToInt(Radius / 80.0f));
                bool bPathIsDry = true;
                for (int32 SampleIndex = 1; SampleIndex <= Samples; ++SampleIndex)
                {
                    const FVector2D SamplePosition = FVector2D(SnakeOrigin) + Offset * (static_cast<float>(SampleIndex) / Samples);
                    if (!IsShimmeringLakesDryPosition(SamplePosition))
                    {
                        bPathIsDry = false;
                        break;
                    }
                }
                if (!bPathIsDry) continue;
                AttackerLocation = SnakeOrigin + FVector(Offset.X, Offset.Y, 0.0f);
                bFoundDryPath = true;
            }
            if (!bFoundDryPath)
            {
                UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: no dry Shimmering Lakes approach within the snake lunge reach."), VerificationName);
                CombatPeerTestStage = 99;
                return;
            }
        }
        if (bRatPeerTest || bShoreSnakePeerTest)
        {
            PlacePeerOnTerrain(Attacker, AttackerLocation);
        }
        else
        {
            Attacker->SetActorLocation(AttackerLocation, false);
        }
        if (bShoreSnakePeerTest)
        {
            Attacker->SetActorRotation((CombatPeerTestTarget->GetActorLocation() - Attacker->GetActorLocation()).Rotation());
            Attacker->GetCharacterMovement()->StopMovementImmediately();
            Attacker->GetCharacterMovement()->DisableMovement();
        }
        if (bMirelingPeerTest)
        {
            // Keep the listen-server pawn on the generated fixture surface while the encounter
            // proves production 3D melee range; this avoids gravity moving it away between ticks.
            Attacker->GetCharacterMovement()->StopMovementImmediately();
            Attacker->GetCharacterMovement()->DisableMovement();
            CombatPeerTestTarget->SetActorTickEnabled(true);
            CombatPeerTestTarget->SetActorTickInterval(0.0f);
        }
        const FVector RemoteLocation = CombatPeerTestTarget->GetActorLocation() + FVector(0.0f, 1000.0f, 0.0f);
        if (bRatPeerTest || bShoreSnakePeerTest)
        {
            PlacePeerOnTerrain(Remote, RemoteLocation);
        }
        else
        {
            Remote->SetActorLocation(RemoteLocation, false);
        }
        if (bRatPeerTest || bShoreSnakePeerTest)
        {
            int32 ObserverOffset = 2000;
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                AKalmalaCharacter* Candidate = *It;
                if (!IsValid(Candidate) || Candidate == Attacker || Candidate == Remote
                    || Candidate->GetPlayerState() == nullptr) continue;
                const FVector ObserverLocation = CombatPeerTestTarget->GetActorLocation()
                    + FVector(0.0f, ObserverOffset, 0.0f);
                PlacePeerOnTerrain(Candidate, ObserverLocation);
                ObserverOffset += 1000;
            }
        }
        if (bViperPeerTest)
        {
            if (UKalmalaPlayerStatusComponent* Statuses = Attacker->GetStatusComponent())
            {
                const float PriorPoisonSeconds = FMath::Max(
                    Statuses->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId),
                    Statuses->GetRemainingSeconds(UKalmalaPlayerStatusComponent::MajorPoisonStatusId));
                if (PriorPoisonSeconds > 0.0f)
                    Statuses->AdvanceFromServer(PriorPoisonSeconds + KINDA_SMALL_NUMBER);
            }
            for (int32 HealAttempt = 0; HealAttempt < 4 && Attacker->GetHealth() < 100.0f; ++HealAttempt)
            {
                if (!Attacker->RestoreHealthFromDraughtFromServer(30.0f)) break;
            }
            if (!FMath::IsNearlyEqual(Attacker->GetHealth(), 100.0f))
            {
                UE_LOG(LogTemp, Error, TEXT("Viper verification FAILED: host could not be reset to full health before the poison bite. Health=%.1f."), Attacker->GetHealth());
                CombatPeerTestStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("Viper verification prepared a full-health host with prior poison expired."));
        }
        if (bGiantSnakePeerTest)
        {
            for (int32 HealAttempt = 0; HealAttempt < 4 && Attacker->GetHealth() < 100.0f; ++HealAttempt)
            {
                if (!Attacker->RestoreHealthFromDraughtFromServer(30.0f)) break;
            }
            if (!FMath::IsNearlyEqual(Attacker->GetHealth(), 100.0f))
            {
                UE_LOG(LogTemp, Error, TEXT("Giant Snake verification FAILED: host could not be reset to full health before the Stab bite. Health=%.1f."), Attacker->GetHealth());
                CombatPeerTestStage = 99;
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("Giant Snake verification prepared a full-health host for the shoreline lunge."));
        }
        if (bDeerPeerTest)
        {
            // The companion remains a normal generated deer. This controlled positioning only
            // puts it within the production 800 cm server noise radius for the herd assertion.
            CombatPeerTestHerdMate->SetActorLocation(CombatPeerTestTarget->GetActorLocation() + FVector(0.0f, 600.0f, 0.0f), false);
            CombatPeerTestHerdMate->ForceNetUpdate();
        }

#if !UE_BUILD_SHIPPING
        const TCHAR* ScreenshotArgument = bMirelingPeerTest ? TEXT("KalmalaMirelingScreenshot=") : (bDeerPeerTest ? TEXT("KalmalaDeerScreenshot=") : nullptr);
        FString ScreenshotPath;
        if (ScreenshotArgument != nullptr && FParse::Value(FCommandLine::Get(), ScreenshotArgument, ScreenshotPath))
        {
            if (LocalController != nullptr)
            {
                const FVector PresentationViewPoint = CombatPeerTestTarget->GetActorLocation() + FVector(0.0f, 0.0f, bMirelingPeerTest ? 85.0f : 75.0f);
                const FVector ViewSide = FVector::CrossProduct(Attacker->GetActorForwardVector().GetSafeNormal2D(), FVector::UpVector).GetSafeNormal();
                const FVector CameraLocation = PresentationViewPoint + ViewSide * 350.0f + FVector(0.0f, 0.0f, 90.0f);
                FActorSpawnParameters CameraSpawnParameters;
                CameraSpawnParameters.ObjectFlags |= RF_Transient;
                CameraSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                if (ACameraActor* ScreenshotCamera = GetWorld()->SpawnActor<ACameraActor>(
                    ACameraActor::StaticClass(), CameraLocation, (PresentationViewPoint - CameraLocation).Rotation(), CameraSpawnParameters))
                {
                    LocalController->SetViewTarget(ScreenshotCamera);
                }
            }
            FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
            UE_LOG(LogTemp, Display, TEXT("%s presentation capture requested after bounded target positioning: %s"), VerificationName, *ScreenshotPath);
        }
#endif
        Attacker->ForceNetUpdate();
        Remote->ForceNetUpdate();
        UE_LOG(LogTemp, Display, TEXT("%s fixture arranged: Target=%s TargetLocation=%s Attacker=%s AttackerLocation=%s Remote=%s RemoteLocation=%s HerdMate=%s HerdMateBehavior=%d"), VerificationName, *CombatPeerTestTarget->GetName(), *CombatPeerTestTarget->GetActorLocation().ToCompactString(), *Attacker->GetName(), *Attacker->GetActorLocation().ToCompactString(), *Remote->GetName(), *Remote->GetActorLocation().ToCompactString(), bDeerPeerTest && CombatPeerTestHerdMate.IsValid() ? *CombatPeerTestHerdMate->GetName() : TEXT("None"), bDeerPeerTest && CombatPeerTestHerdMate.IsValid() ? static_cast<int32>(CombatPeerTestHerdMate->GetBehaviour()) : -1);
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
    if (bViperPeerTest && CombatPeerTestStage >= 1 && CombatPeerTestStage <= 2)
    {
        const UKalmalaPlayerStatusComponent* AttackerStatuses = Attacker->GetStatusComponent();
        bCombatPeerTestViperPoisonObserved |= AttackerStatuses != nullptr
            && AttackerStatuses->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId);
        if (bCombatPeerTestViperPoisonObserved && Target->IsActorTickEnabled())
        {
            // Stop repeat bites after the real first poison application so the
            // four player swings test replication without requiring fixture heals.
            Target->SetActorTickEnabled(false);
            UE_LOG(LogTemp, Display, TEXT("Viper verification fixture paused the target after its first weak-poison bite."));
        }
    }
    if (bGiantSnakePeerTest && CombatPeerTestStage >= 1 && CombatPeerTestStage <= 2
        && Attacker->GetHealth() < 100.0f && !bCombatPeerTestGiantSnakeBiteObserved)
    {
        bCombatPeerTestGiantSnakeBiteObserved = true;
        if (Target->IsActorTickEnabled())
        {
            Target->SetActorTickEnabled(false);
            UE_LOG(LogTemp, Display, TEXT("Giant Snake verification fixture paused the target after its first accepted server Stab bite."));
        }
    }
    const auto PlacePeerTestTargetInFront = [this, Attacker, Target, bRatPeerTest, bShoreSnakePeerTest](const float Distance)
    {
        FVector TargetLocation = Attacker->GetActorLocation()
            + Attacker->GetActorForwardVector().GetSafeNormal2D() * Distance;
        if (bRatPeerTest || bShoreSnakePeerTest)
        {
            const FVector2D TargetPosition(TargetLocation);
            TargetLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, TargetPosition);
        }
        if (bShoreSnakePeerTest)
        {
            const FVector2D TargetPosition(TargetLocation);
            const bool bDryShore = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, TargetPosition))
                    == EKalmalaBiome::ShimmeringLakes
                && !FKalmalaOceanSampler::Sample(WorldGenerationConfig, TargetPosition).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, TargetPosition);
            if (!bDryShore) return false;
        }
        Target->SetActorLocation(TargetLocation, false);
        Target->ForceNetUpdate();
        return true;
    };

    // Give the bounded server-only encounter loop enough simulated time to
    // commit one melee hit before asserting replicated pressure. Headless
    // editor startup can leave the first few ticks below real-time speed, so
    // retain a bounded grace window instead of treating that startup delay as
    // a gameplay failure.
    if (CombatPeerTestStage == 1 && Now - CombatPeerTestStageTime >= 2.5f)
    {
        const float ExpectedTargetHealth = AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(Target->GetArchetype());
        const bool bTargetMutated = !FMath::IsNearlyEqual(Target->GetHealth(), ExpectedTargetHealth);
        const bool bEncounterAppliedPressure = Attacker->GetHealth() < 100.0f;
        const UKalmalaPlayerStatusComponent* AttackerStatuses = Attacker->GetStatusComponent();
        bCombatPeerTestViperPoisonObserved |= bViperPeerTest && AttackerStatuses != nullptr
            && AttackerStatuses->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId);
        const bool bEncounterPressureMissing = ((bMirelingPeerTest || bBoarPeerTest || bRatPeerTest) && !bEncounterAppliedPressure)
            || (bViperPeerTest && !bCombatPeerTestViperPoisonObserved)
            || (bGiantSnakePeerTest && !bCombatPeerTestGiantSnakeBiteObserved);
        if (bTargetMutated || bEncounterPressureMissing)
        {
            if (Now - CombatPeerTestStageTime < 8.0f)
            {
                return;
            }
            UE_LOG(LogTemp, Error, TEXT("Combat verification FAILED: remote target-free attack mutated the target or the server-owned encounter did not apply pressure. Mutated=%d EncounterPressure=%d ViperPoison=%d Archetype=%d Behaviour=%d Distance=%.1f Health=%.1f ExpectedHealth=%.1f PlayerHealth=%.1f AttackerLocation=%s TargetLocation=%s"), bTargetMutated, bEncounterAppliedPressure, bCombatPeerTestViperPoisonObserved, static_cast<int32>(Target->GetArchetype()), static_cast<int32>(Target->GetBehaviour()), FVector::Distance(Target->GetActorLocation(), Attacker->GetActorLocation()), Target->GetHealth(), ExpectedTargetHealth, Attacker->GetHealth(), *Attacker->GetActorLocation().ToCompactString(), *Target->GetActorLocation().ToCompactString());
            CombatPeerTestStage = 99;
            return;
        }
        CombatPeerTestStage = 2;
        CombatPeerTestStageTime = Now;
        return;
    }

    if (CombatPeerTestStage == 2 && AttackerCombat->GetActionPhase() == EKalmalaCombatActionPhase::Idle && CombatPeerTestSequence < ExpectedAttackCount)
    {
        if (!PlacePeerTestTargetInFront(bRatPeerTest ? 50.0f : 150.0f))
        {
            UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: could not keep the target on dry Shimmering Lakes shore."), VerificationName);
            CombatPeerTestStage = 99;
            return;
        }
        AttackerCombat->ServerRequestAttack(++CombatPeerTestSequence);
        CombatPeerTestStageTime = Now;
        return;
    }

    // The charge proof above already exercises real territorial movement. Keep
    // the target in the ordinary 220 cm combat trace for the separate player
    // attack contract, so a moving encounter cannot race its windup recheck.
    if (CombatPeerTestStage == 2 && AttackerCombat->GetActionPhase() == EKalmalaCombatActionPhase::Windup)
    {
        if (!PlacePeerTestTargetInFront(bRatPeerTest ? 50.0f : 150.0f))
        {
            UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: windup target left the dry Shimmering Lakes shore."), VerificationName);
            CombatPeerTestStage = 99;
            return;
        }
    }

    if (bDeerPeerTest && CombatPeerTestSequence > 0 && CombatPeerTestHerdMate.IsValid()
        && CombatPeerTestHerdMate->GetBehaviour() != EKalmalaWildlifeBehaviour::Idle)
    {
        bCombatPeerTestHerdAlertObserved = true;
    }

    if (bPendingRewardPeerTest && CombatPeerTestStage == 3)
    {
        UKalmalaInventoryComponent* AttackerInventory = Attacker->GetInventoryComponent();
        UKalmalaInventoryComponent* RemoteInventory = Remote->GetInventoryComponent();
        const FString ClaimId = bRatPeerTest ? Target->GetPersistentSpawnId()
            : (bViperPeerTest ? UKalmalaWorldPopulationSaveGame::MakeViperPendingRewardClaimId(Target->GetPersistentSpawnId())
            : (bGiantSnakePeerTest ? UKalmalaWorldPopulationSaveGame::MakeGiantSnakePendingRewardClaimId(Target->GetPersistentSpawnId())
            : (bBadgerPeerTest
                ? UKalmalaWorldPopulationSaveGame::MakeBadgerPendingRewardClaimId(Target->GetPersistentSpawnId())
                : UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(Target->GetPersistentSpawnId()))));
        const FString OwnerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Attacker->GetPlayerState());
        const TArray<FKalmalaPendingWildlifeReward> PendingRewards = PopulationSaveGame
            ? PopulationSaveGame->GetPendingWildlifeRewards() : TArray<FKalmalaPendingWildlifeReward>();
        const bool bClaimStillPending = PendingRewards.ContainsByPredicate([&ClaimId](const FKalmalaPendingWildlifeReward& Reward)
        {
            return Reward.PersistentSpawnId == ClaimId;
        });
        const int32 BadgerFur = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("BadgerFur")) : 0;
        const int32 BadgerMeat = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("BadgerMeat")) : 0;
        const int32 AttackerBone = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("BoneFragment")) : 0;
        const int32 RatTail = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("RatTail")) : 0;
        const int32 ViperFang = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("ViperFang")) : 0;
        const int32 GiantScale = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("GiantScale")) : 0;
        const int32 RemoteFur = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("BadgerFur")) : 0;
        const int32 RemoteMeat = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("BadgerMeat")) : 0;
        const int32 RemoteBone = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("BoneFragment")) : 0;
        const int32 RemoteRatTail = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("RatTail")) : 0;
        const int32 RemoteViperFang = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("ViperFang")) : 0;
        const int32 RemoteGiantScale = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("GiantScale")) : 0;
        int32 OtherPeerCount = 0;
        int32 OtherPeerRatTail = 0;
        int32 OtherPeerViperFang = 0;
        int32 OtherPeerGiantScale = 0;
        int32 OtherPeerBoneFragment = 0;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (!IsValid(Candidate) || Candidate == Attacker || Candidate->GetPlayerState() == nullptr) continue;
            ++OtherPeerCount;
            const UKalmalaInventoryComponent* CandidateInventory = Candidate->GetInventoryComponent();
            if (CandidateInventory != nullptr)
            {
                OtherPeerRatTail += CandidateInventory->GetQuantity(TEXT("RatTail"));
                OtherPeerViperFang += CandidateInventory->GetQuantity(TEXT("ViperFang"));
                OtherPeerGiantScale += CandidateInventory->GetQuantity(TEXT("GiantScale"));
                OtherPeerBoneFragment += CandidateInventory->GetQuantity(TEXT("BoneFragment"));
            }
            else
            {
                OtherPeerRatTail = MAX_int32;
                OtherPeerViperFang = MAX_int32;
                OtherPeerGiantScale = MAX_int32;
                OtherPeerBoneFragment = MAX_int32;
            }
        }
        const int32 ExpectedBone = bGiantSnakePeerTest ? 2 : (bMoonBearPeerTest ? 3 : 1);
        const bool bExpectedReward = AttackerInventory != nullptr && RemoteInventory != nullptr
            && (bRatPeerTest
                ? (RatTail == 1 && AttackerBone == 1 && BadgerFur == 0 && BadgerMeat == 0
                    && RemoteRatTail == 0 && OtherPeerCount >= 2 && OtherPeerRatTail == 0
                    && OtherPeerBoneFragment == 0
                    && RemoteBone == 0 && RemoteFur == 0 && RemoteMeat == 0)
                : (bViperPeerTest
                    ? (ViperFang == 1 && AttackerBone == 1 && RatTail == 0
                        && RemoteViperFang == 0 && OtherPeerCount >= 2 && OtherPeerViperFang == 0
                        && OtherPeerBoneFragment == 0 && RemoteBone == 0)
                : (bGiantSnakePeerTest
                    ? (GiantScale == 2 && AttackerBone == 2 && RemoteGiantScale == 0
                        && OtherPeerCount >= 2 && OtherPeerGiantScale == 0 && OtherPeerBoneFragment == 0
                        && RemoteBone == 0)
                : (AttackerBone == ExpectedBone
                    && (!bBadgerPeerTest || (BadgerFur == 1 && BadgerMeat == 1))
                    && (bBadgerPeerTest || (BadgerFur == 0 && BadgerMeat == 0))
                    && RemoteBone == 0 && RemoteFur == 0 && RemoteMeat == 0))));
        if (!bClaimStillPending && bExpectedReward && Target->IsDefeated()
            && PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(Target->GetPersistentSpawnId()))
        {
            if (bRatPeerTest)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("Rat verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=%u Health=%.1f PlayerHealth=%.1f Defeated=1 Saved=1 FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 BoneFragment=%d RatTail=%d RemoteBoneFragment=%d RemoteRatTail=%d OtherPeerBoneFragments=%d OtherPeerRatTails=%d OtherPeers=%d Owner=%s SpawnId=%s"),
                    AttackerCombat->GetActionSerial(), Target->GetHealth(), Attacker->GetHealth(), AttackerBone,
                    RatTail, RemoteBone, RemoteRatTail, OtherPeerBoneFragment, OtherPeerRatTail, OtherPeerCount,
                    *OwnerIdentity, *Target->GetPersistentSpawnId());
            }
            else if (bViperPeerTest)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("Viper verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=%u WeakPoison=%d Defeated=1 Saved=1 FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 BoneFragment=%d ViperFang=%d RemoteBoneFragment=%d RemoteViperFang=%d OtherPeerBoneFragments=%d OtherPeerViperFangs=%d OtherPeers=%d Owner=%s SpawnId=%s"),
                    AttackerCombat->GetActionSerial(), bCombatPeerTestViperPoisonObserved, AttackerBone, ViperFang, RemoteBone, RemoteViperFang,
                    OtherPeerBoneFragment, OtherPeerViperFang, OtherPeerCount, *OwnerIdentity, *Target->GetPersistentSpawnId());
            }
            else if (bGiantSnakePeerTest)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("Giant Snake verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=%u Bite=%d Defeated=1 Saved=1 FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 BoneFragment=%d GiantScale=%d RemoteBoneFragment=%d RemoteGiantScale=%d OtherPeerBoneFragments=%d OtherPeerGiantScales=%d OtherPeers=%d Owner=%s SpawnId=%s"),
                    AttackerCombat->GetActionSerial(), bCombatPeerTestGiantSnakeBiteObserved, AttackerBone,
                    GiantScale, RemoteBone, RemoteGiantScale, OtherPeerBoneFragment, OtherPeerGiantScale,
                    OtherPeerCount, *OwnerIdentity, *Target->GetPersistentSpawnId());
            }
            else
            {
                UE_LOG(LogTemp, Display,
                    TEXT("%s verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 InvalidRejected=1 ActionSerial=4 Defeated=1 Saved=1 FullPackEscrow=1 RetryDelivered=1 OwnerOnly=1 BoneFragment=%d BadgerFur=%d BadgerMeat=%d RemoteBoneFragment=%d RemoteBadgerFur=%d RemoteBadgerMeat=%d Owner=%s SpawnId=%s"),
                    VerificationName, AttackerBone, BadgerFur, BadgerMeat, RemoteBone, RemoteFur, RemoteMeat,
                    *OwnerIdentity, *Target->GetPersistentSpawnId());
            }
            CombatPeerTestStage = 99;
            return;
        }
        if (Now - CombatPeerTestStageTime >= 20.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("%s verification FAILED: reward retry did not complete. Pending=%d ExpectedReward=%d Saved=%d Slots=%d Owner=%s"),
                VerificationName, bClaimStillPending, bExpectedReward,
                PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(Target->GetPersistentSpawnId()),
                AttackerInventory ? AttackerInventory->GetStacks().Num() : -1, *OwnerIdentity);
            CombatPeerTestStage = 99;
        }
        return;
    }

    if (CombatPeerTestStage == 2 && CombatPeerTestSequence == ExpectedAttackCount && AttackerCombat->GetActionPhase() == EKalmalaCombatActionPhase::Idle && Now - CombatPeerTestStageTime >= 0.1f)
    {
        if ((bViperPeerTest || bGiantSnakePeerTest) && AttackerCombat->GetActionSerial() != ExpectedAttackCount)
        {
            UE_LOG(LogTemp, Error,
                TEXT("%s verification FAILED: expected four accepted host attacks before reward setup. ActionSerial=%u RequestSequence=%u Health=%.1f."),
                VerificationName, AttackerCombat->GetActionSerial(), CombatPeerTestSequence, Attacker->GetHealth());
            CombatPeerTestStage = 99;
            return;
        }
        if (bPendingRewardPeerTest)
        {
            const FName BoneFragmentId(TEXT("BoneFragment"));
            const FName BadgerFurId(TEXT("BadgerFur"));
            const FName BadgerMeatId(TEXT("BadgerMeat"));
            const FName RatTailId(TEXT("RatTail"));
            const FName ViperFangId(TEXT("ViperFang"));
            const FName GiantScaleId(TEXT("GiantScale"));
            const FString ClaimId = bRatPeerTest ? Target->GetPersistentSpawnId()
                : (bViperPeerTest ? UKalmalaWorldPopulationSaveGame::MakeViperPendingRewardClaimId(Target->GetPersistentSpawnId())
                : (bGiantSnakePeerTest ? UKalmalaWorldPopulationSaveGame::MakeGiantSnakePendingRewardClaimId(Target->GetPersistentSpawnId())
                : (bBadgerPeerTest
                    ? UKalmalaWorldPopulationSaveGame::MakeBadgerPendingRewardClaimId(Target->GetPersistentSpawnId())
                    : UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(Target->GetPersistentSpawnId()))));
            const FString OwnerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Attacker->GetPlayerState());
            if (bRatPeerTest && !Target->IsDefeated())
            {
                UE_LOG(LogTemp, Error,
                    TEXT("Rat verification FAILED: the single accepted server mace hit did not defeat the generated 10-health rat. Health=%.1f Distance=%.1f AttackerLocation=%s TargetLocation=%s."),
                    Target->GetHealth(), FVector::Dist(Attacker->GetActorLocation(), Target->GetActorLocation()),
                    *Attacker->GetActorLocation().ToCompactString(), *Target->GetActorLocation().ToCompactString());
                CombatPeerTestStage = 99;
                return;
            }
            if (!Target->IsDefeated())
            {
                const FVector Forward = Attacker->GetActorForwardVector().GetSafeNormal2D();
                Target->SetActorLocation(Attacker->GetActorLocation() + Forward * 150.0f, false);
                Target->ForceNetUpdate();
                FKalmalaDamageChannels FixtureDamage;
                FixtureDamage.Crush = 100.0f;
                UE_LOG(LogTemp, Display,
                    TEXT("%s reward fixture: completing post-combat defeat with server-only bounded damage after four accepted host attacks, target reset to 150 cm."),
                    VerificationName);
                int32 FixtureHitsAccepted = 0;
                for (int32 Hit = 0; Hit < 4 && !Target->IsDefeated(); ++Hit)
                {
                    if (Target->ApplyCombatDamageFromServer(FixtureDamage, Attacker)) ++FixtureHitsAccepted;
                }
                if (!Target->IsDefeated()) Target->DefeatServer();
                UE_LOG(LogTemp, Display,
                    TEXT("%s reward fixture result: AcceptedFixtureHits=%d Distance=%.1f Health=%.1f Defeated=%d AttackerHealth=%.1f."),
                    VerificationName, FixtureHitsAccepted, FVector::Distance(Attacker->GetActorLocation(), Target->GetActorLocation()),
                    Target->GetHealth(), Target->IsDefeated(), Attacker->GetHealth());
            }
            const TArray<FKalmalaPendingWildlifeReward> PendingRewards = PopulationSaveGame
                ? PopulationSaveGame->GetPendingWildlifeRewards() : TArray<FKalmalaPendingWildlifeReward>();
            const bool bOwnerClaimQueued = PendingRewards.ContainsByPredicate([&ClaimId, &OwnerIdentity](const FKalmalaPendingWildlifeReward& Reward)
            {
                return Reward.PersistentSpawnId == ClaimId && Reward.PlayerIdentity == OwnerIdentity;
            });
            UKalmalaInventoryComponent* AttackerInventory = Attacker->GetInventoryComponent();
            UKalmalaInventoryComponent* RemoteInventory = Remote->GetInventoryComponent();
            int32 OtherPeerCount = 0;
            int32 OtherPeerRatTail = 0;
            int32 OtherPeerViperFang = 0;
            int32 OtherPeerGiantScale = 0;
            int32 OtherPeerBoneFragment = 0;
            for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
            {
                AKalmalaCharacter* Candidate = *It;
                if (!IsValid(Candidate) || Candidate == Attacker || Candidate->GetPlayerState() == nullptr) continue;
                ++OtherPeerCount;
                const UKalmalaInventoryComponent* CandidateInventory = Candidate->GetInventoryComponent();
                if (CandidateInventory != nullptr)
                {
                    OtherPeerRatTail += CandidateInventory->GetQuantity(RatTailId);
                    OtherPeerViperFang += CandidateInventory->GetQuantity(ViperFangId);
                    OtherPeerGiantScale += CandidateInventory->GetQuantity(GiantScaleId);
                    OtherPeerBoneFragment += CandidateInventory->GetQuantity(BoneFragmentId);
                }
                else
                {
                    OtherPeerRatTail = MAX_int32;
                    OtherPeerViperFang = MAX_int32;
                    OtherPeerGiantScale = MAX_int32;
                    OtherPeerBoneFragment = MAX_int32;
                }
            }
            const bool bEscrowedWithoutPartialGrant = Target->IsDefeated() && bOwnerClaimQueued
                && AttackerInventory != nullptr
                && AttackerInventory->GetStacks().Num() == UKalmalaInventoryComponent::MaxSlots
                    - AttackerInventory->CountPackedTools(Attacker->GetCarriedToolInventory())
                && AttackerInventory->GetQuantity(BoneFragmentId) == 0
                && AttackerInventory->GetQuantity(BadgerFurId) == 0
                && AttackerInventory->GetQuantity(BadgerMeatId) == 0
                && AttackerInventory->GetQuantity(RatTailId) == 0
                && AttackerInventory->GetQuantity(ViperFangId) == 0
                && AttackerInventory->GetQuantity(GiantScaleId) == 0
                && RemoteInventory != nullptr && RemoteInventory->GetQuantity(BoneFragmentId) == 0
                && RemoteInventory->GetQuantity(BadgerFurId) == 0 && RemoteInventory->GetQuantity(BadgerMeatId) == 0
                && RemoteInventory->GetQuantity(RatTailId) == 0
                && RemoteInventory->GetQuantity(ViperFangId) == 0
                && RemoteInventory->GetQuantity(GiantScaleId) == 0
                && (!bTwoItemRewardPeerTest || (OtherPeerCount >= 2
                    && OtherPeerRatTail == 0 && OtherPeerViperFang == 0
                    && OtherPeerGiantScale == 0
                    && OtherPeerBoneFragment == 0));
            if (!bEscrowedWithoutPartialGrant)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("%s verification FAILED: full-pack defeat did not retain one owner-bound claim without a partial reward. Defeated=%d OwnerClaim=%d Slots=%d"),
                    VerificationName, Target->IsDefeated(), bOwnerClaimQueued,
                    AttackerInventory ? AttackerInventory->GetStacks().Num() : -1);
                CombatPeerTestStage = 99;
                return;
            }

            int32 FreedCells = 0;
            const int32 RequiredCells = bTwoItemRewardPeerTest ? 2 : (bBadgerPeerTest ? 3 : 1);
            const int32 FullPackCapacity = UKalmalaInventoryComponent::MaxSlots
                - AttackerInventory->CountPackedTools(Attacker->GetCarriedToolInventory());
            const TArray<FKalmalaInventoryStack> FullPack = AttackerInventory->GetStacks();
            for (const FKalmalaInventoryStack& Stack : FullPack)
            {
                if (FreedCells >= RequiredCells) break;
                if (Stack.Quantity != 1 || Stack.ItemId == BoneFragmentId || Stack.ItemId == BadgerFurId
                    || Stack.ItemId == BadgerMeatId || Stack.ItemId == RatTailId || Stack.ItemId == ViperFangId
                    || Stack.ItemId == GiantScaleId) continue;
                if (AttackerInventory->TryConsumeFromServer(Stack.ItemId, 1)
                    && AttackerInventory->GetStacks().Num() < FullPackCapacity)
                {
                    ++FreedCells;
                }
            }
            if (FreedCells != RequiredCells)
            {
                UE_LOG(LogTemp, Error, TEXT("%s verification FAILED: freed %d of %d inventory cells for reward retry."),
                    VerificationName, FreedCells, RequiredCells);
                CombatPeerTestStage = 99;
                return;
            }
            CombatPeerTestStage = 3;
            CombatPeerTestStageTime = Now;
            return;
        }

        const bool bSavedDefeat = Target->IsDefeated() && !Target->GetPersistentSpawnId().IsEmpty() && PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(Target->GetPersistentSpawnId());
        UKalmalaInventoryComponent* AttackerInventory = Attacker->GetInventoryComponent();
        UKalmalaInventoryComponent* RemoteInventory = Remote->GetInventoryComponent();
        const bool bMirelingDropAbsent = AttackerInventory != nullptr && RemoteInventory != nullptr
            && AttackerInventory->GetStacks().IsEmpty() && RemoteInventory->GetStacks().IsEmpty();
        const int32 Meat = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("BoarMeat")) : 0;
        const int32 Hide = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("BoarHide")) : 0;
        const int32 BoneFragment = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("BoneFragment")) : 0;
        const int32 RemoteMeat = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("BoarMeat")) : 0;
        const int32 RemoteHide = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("BoarHide")) : 0;
        const int32 RemoteBoneFragment = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("BoneFragment")) : 0;
        const int32 DeerMeat = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("DeerMeat")) : 0;
        const int32 DeerHide = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("DeerHide")) : 0;
        const int32 RemoteDeerMeat = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("DeerMeat")) : 0;
        const int32 RemoteDeerHide = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("DeerHide")) : 0;
        const int32 DeerSinew = AttackerInventory ? AttackerInventory->GetQuantity(TEXT("Sinew")) : 0;
        const int32 RemoteDeerSinew = RemoteInventory ? RemoteInventory->GetQuantity(TEXT("Sinew")) : 0;
        const bool bRewardValid = bMirelingPeerTest ? bMirelingDropAbsent : (bBoarPeerTest ? (Meat == 1 && Hide == 1 && BoneFragment == 1 && RemoteMeat == 0 && RemoteHide == 0 && RemoteBoneFragment == 0) : (!bDeerPeerTest || (DeerMeat == 1 && DeerHide == 1 && BoneFragment == 1 && DeerSinew >= 0 && DeerSinew <= 1 && RemoteDeerMeat == 0 && RemoteDeerHide == 0 && RemoteBoneFragment == 0 && RemoteDeerSinew == 0)));
        if (AttackerCombat->GetActionSerial() == ExpectedAttackCount && bSavedDefeat && bRewardValid && (!bDeerPeerTest || bCombatPeerTestHerdAlertObserved))
        {
            UE_LOG(LogTemp, Display, TEXT("%s verification server: Passed=1 SeedReproduced=1 BoundedActivation=1 HerdAlert=%d InvalidRejected=1 ActionSerial=%u Health=%.1f PlayerHealth=%.1f Defeated=1 Saved=1 Meat=%d Hide=%d BoneFragment=%d RemoteMeat=%d RemoteHide=%d RemoteBoneFragment=%d DeerMeat=%d DeerHide=%d DeerSinew=%d RemoteDeerMeat=%d RemoteDeerHide=%d RemoteDeerSinew=%d SpawnId=%s"), VerificationName, bCombatPeerTestHerdAlertObserved, AttackerCombat->GetActionSerial(), Target->GetHealth(), Attacker->GetHealth(), Meat, Hide, BoneFragment, RemoteMeat, RemoteHide, RemoteBoneFragment, DeerMeat, DeerHide, DeerSinew, RemoteDeerMeat, RemoteDeerHide, RemoteDeerSinew, *Target->GetPersistentSpawnId());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Combat verification server: Passed=0 ActionSerial=%u Health=%.1f Defeated=%d Saved=%d HerdAlert=%d HerdMateBehavior=%d RewardValid=%d DeerMeat=%d DeerHide=%d DeerSinew=%d BoneFragment=%d RemoteDeerMeat=%d RemoteDeerHide=%d RemoteDeerSinew=%d RemoteBoneFragment=%d"), AttackerCombat->GetActionSerial(), Target->GetHealth(), Target->IsDefeated(), bSavedDefeat, bCombatPeerTestHerdAlertObserved, bDeerPeerTest && CombatPeerTestHerdMate.IsValid() ? static_cast<int32>(CombatPeerTestHerdMate->GetBehaviour()) : -1, bRewardValid, DeerMeat, DeerHide, DeerSinew, BoneFragment, RemoteDeerMeat, RemoteDeerHide, RemoteDeerSinew, RemoteBoneFragment);
        }
        CombatPeerTestStage = 99;
    }
#endif
}

void AKalmalaGameMode::DriveMosquitoPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaMosquitoPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || MosquitoPeerTestStage >= 99)
    {
        return;
    }

    const auto Fail = [this](const TCHAR* Reason)
    {
        UE_LOG(LogTemp, Error, TEXT("Mosquito verification FAILED: %s"), Reason);
        MosquitoPeerTestStage = 99;
    };
    const float Now = GetWorld()->GetTimeSeconds();
    const auto PlaceOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
    {
        const FVector2D Position(DesiredLocation);
        ActivateTerrainPatchNeighborhood(Position);
        FVector GroundedLocation = DesiredLocation;
        GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Position)
            + Character->GetSimpleCollisionHalfHeight() + 1.0f;
        Character->SetActorLocation(GroundedLocation, false);
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            Movement->StopMovementImmediately();
        Character->ForceNetUpdate();
    };
    const auto PlaceSwarmAtContact = [this, &PlaceOnTerrain](AKalmalaCharacter* Character,
        AKalmalaWildlifeSpawn* Target)
    {
        if (!IsValid(Character) || !IsValid(Target)) return false;
        PlaceOnTerrain(Character, MosquitoPeerTestContactLocation);
        Target->SetActorLocation(Character->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f), false);
        Target->ForceNetUpdate();
        return true;
    };

    if (MosquitoPeerTestStage == 0)
    {
        APlayerController* ListenHostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* ListenHost = ListenHostController
            ? Cast<AKalmalaCharacter>(ListenHostController->GetPawn()) : nullptr;
        TArray<AKalmalaCharacter*> Peers;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == nullptr || Candidate == ListenHost || Candidate->GetPlayerState() == nullptr) continue;
            Peers.Add(Candidate);
        }
        if (ListenHost == nullptr || ListenHost->GetPlayerState() == nullptr || Peers.Num() < 2) return;

        // One remote pawn owns the bitten character so its private status
        // replication is exercised; the other remote peer observes public health.
        MosquitoPeerTestHost = Peers[0];
        MosquitoPeerTestListenHost = ListenHost;
        MosquitoPeerTestObserver = Peers[1];

        if (AKalmalaWorldGenerationGameState* WorldState = GetWorld()->GetGameState<AKalmalaWorldGenerationGameState>())
        {
            FKalmalaWeatherState Weather = WorldState->GetWeatherState();
            Weather.PrecipitationIntensity = 0.0f;
            WorldState->SetWeatherStateFromServer(Weather);
        }

        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(
            FVector2D(ListenHost->GetActorLocation()));
        bool bFoundSpawn = false;
        for (int32 Radius = 0; Radius <= 64 && !bFoundSpawn; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundSpawn; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    const FVector2D KeyCentre = (FVector2D(CandidateKey) + FVector2D(0.5f, 0.5f))
                        * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
                    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                        FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, KeyCentre));
                    if (KeyBiome == EKalmalaBiome::ShimmeringLakes || KeyBiome == EKalmalaBiome::Elderwood
                        || KeyBiome == EKalmalaBiome::MossyMire || KeyBiome == EKalmalaBiome::FreezingTundra
                        || KeyBiome == EKalmalaBiome::ThunderMountains)
                    {
                        Descriptors.SetNum(FMath::Min(Descriptors.Num(),
                            FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                                Descriptors.Num(), EKalmalaWorldPopulationKind::Wildlife, KeyBiome)));
                    }
                    const FKalmalaWorldPopulationSpawn* Candidate = Descriptors.FindByPredicate(
                        [this](const FKalmalaWorldPopulationSpawn& Descriptor)
                        {
                            return KalmalaGameMode::IsWildlifeSpawnOfArchetype(WorldGenerationConfig,
                                Descriptor, EKalmalaWildlifeArchetype::MosquitoSwarm)
                                && (PopulationSaveGame == nullptr || !PopulationSaveGame->IsDefeated(
                                    FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptor)));
                        });
                    if (Candidate == nullptr) continue;

                    MosquitoPeerTestSpatialKey = CandidateKey;
                    MosquitoPeerTestSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(*Candidate);
                    MosquitoPeerTestWorldSeed = WorldGenerationConfig.WorldSeed;
                    bFoundSpawn = true;
                    break;
                }
            }
        }
        if (!bFoundSpawn || MosquitoPeerTestSpawnId.IsEmpty())
        {
            Fail(TEXT("no normal generated Mossy Mire swarm descriptor was found within the bounded search"));
            return;
        }

        ActivatePopulationKey(MosquitoPeerTestSpatialKey);
        MosquitoPeerTestStageTime = Now;
        MosquitoPeerTestStage = 1;
        UE_LOG(LogTemp, Display,
            TEXT("Mosquito verification selected generated swarm Seed=%llu SpatialKey=(%d,%d) SpawnId=%s."),
            static_cast<unsigned long long>(MosquitoPeerTestWorldSeed), MosquitoPeerTestSpatialKey.X,
            MosquitoPeerTestSpatialKey.Y, *MosquitoPeerTestSpawnId);
        return;
    }

    if (MosquitoPeerTestStage == 1)
    {
        AKalmalaCharacter* Host = MosquitoPeerTestHost.Get();
        AKalmalaCharacter* ListenHost = MosquitoPeerTestListenHost.Get();
        AKalmalaCharacter* Observer = MosquitoPeerTestObserver.Get();
        if (!IsValid(Host) || !IsValid(ListenHost) || !IsValid(Observer))
        {
            Fail(TEXT("a required host or peer pawn disappeared during setup"));
            return;
        }
        AKalmalaWildlifeSpawn* Target = nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if ((*It)->GetPersistentSpawnId() == MosquitoPeerTestSpawnId)
            {
                Target = *It;
                break;
            }
        }
        if (Target == nullptr)
        {
            if (Now - MosquitoPeerTestStageTime > 15.0f)
                Fail(TEXT("the selected generated swarm did not activate"));
            return;
        }
        if (Target->IsDefeated() || Target->GetArchetype() != EKalmalaWildlifeArchetype::MosquitoSwarm)
        {
            Fail(TEXT("the selected generated actor is defeated or has the wrong archetype"));
            return;
        }

        const FVector SpawnLocation = Target->GetActorLocation();
        PlaceOnTerrain(Host, SpawnLocation);
        PlaceOnTerrain(Observer, SpawnLocation + FVector(0.0f, 3000.0f, 0.0f));
        PlaceOnTerrain(ListenHost, SpawnLocation + FVector(0.0f, 4000.0f, 0.0f));
        MosquitoPeerTestContactLocation = Host->GetActorLocation();
        Target->SetActorTickEnabled(true);
        Target->ForceNetUpdate();
        MosquitoPeerTestTarget = Target;
        MosquitoPeerTestStartingHealth = Host->GetHealth();
        MosquitoPeerTestStageTime = Now;
        MosquitoPeerTestStage = 2;
        UE_LOG(LogTemp, Display,
            TEXT("Mosquito verification placed the owner client by the swarm and the listen host and observer outside its target leash."));
        return;
    }

    if (MosquitoPeerTestStage == 2)
    {
        AKalmalaCharacter* Host = MosquitoPeerTestHost.Get();
        AKalmalaWildlifeSpawn* Target = MosquitoPeerTestTarget.Get();
        UKalmalaPlayerStatusComponent* Statuses = IsValid(Host) ? Host->GetStatusComponent() : nullptr;
        if (!IsValid(Host) || !IsValid(Target) || Statuses == nullptr)
        {
            Fail(TEXT("host, generated swarm, or status component disappeared during the encounter"));
            return;
        }

        const int32 Bites = Target->GetMosquitoAcceptedBiteCountForVerification();
        const float Damage = MosquitoPeerTestStartingHealth - Host->GetHealth();
        const float WeakPoisonSeconds = Statuses->GetRemainingSeconds(
            UKalmalaPlayerStatusComponent::WeakPoisonStatusId);
        if (Bites >= 2 && Damage >= 10.0f && WeakPoisonSeconds >= 8.0f)
        {
            MosquitoPeerTestBiteBaseline = Bites;
            MosquitoPeerTestStageTime = Now;
            MosquitoPeerTestStage = 3;
            Host->ClientRequestMosquitoPeerTestDodgeRoll();
            UE_LOG(LogTemp, Display, TEXT("Mosquito verification requested an owner-client dodge roll after %d bites."), Bites);
            return;
        }
        if (Now - MosquitoPeerTestStageTime > 30.0f)
        {
            Fail(TEXT("the normal swarm did not land two damaging bites and apply its ten-second weak poison"));
        }
    }

    if (MosquitoPeerTestStage >= 3 && MosquitoPeerTestStage <= 9)
    {
        AKalmalaCharacter* Host = MosquitoPeerTestHost.Get();
        AKalmalaWildlifeSpawn* Target = MosquitoPeerTestTarget.Get();
        if (!IsValid(Host) || !IsValid(Target))
        {
            Fail(TEXT("the host pawn or generated swarm disappeared during counterplay checks"));
            return;
        }
        const int32 Bites = Target->GetMosquitoAcceptedBiteCountForVerification();
        const auto FailOnUnexpectedBite = [this, Bites](const TCHAR* Reason)
        {
            if (Bites != MosquitoPeerTestBiteBaseline)
            {
                UE_LOG(LogTemp, Error, TEXT("Mosquito verification FAILED: %s (expected bites=%d actual=%d)."),
                    Reason, MosquitoPeerTestBiteBaseline, Bites);
                MosquitoPeerTestStage = 99;
                return true;
            }
            return false;
        };

        if (MosquitoPeerTestStage == 3)
        {
            if (Host->MosquitoPeerTestAcceptedDodgeRequests == 1)
            {
                MosquitoPeerTestAcceptedRollTime = Now;
                Host->ClientRequestMosquitoPeerTestDodgeRoll();
                MosquitoPeerTestStageTime = Now;
                MosquitoPeerTestStage = 4;
            }
            else if (Host->MosquitoPeerTestRejectedDodgeRequests > 0)
                Fail(TEXT("the grounded owner-client dodge request was unexpectedly rejected"));
            else if (Now - MosquitoPeerTestStageTime > 10.0f)
                Fail(TEXT("the owner-client dodge request did not reach the server"));
            return;
        }

        if (MosquitoPeerTestStage == 4)
        {
            if (FailOnUnexpectedBite(TEXT("the swarm bit after the accepted dodge roll broke contact"))) return;
            if (Host->MosquitoPeerTestRejectedDodgeRequests == 1)
            {
                MosquitoPeerTestStageTime = Now;
                MosquitoPeerTestStage = 5;
            }
            else if (Now - MosquitoPeerTestStageTime > 10.0f)
                Fail(TEXT("the immediate cooldown dodge request was not rejected"));
            return;
        }

        if (MosquitoPeerTestStage == 5)
        {
            if (FailOnUnexpectedBite(TEXT("the accepted roll did not keep the swarm off contact"))) return;
            if (Host->MosquitoPeerTestAcceptedDodgeRequests != 1
                || Host->MosquitoPeerTestRejectedDodgeRequests != 1)
            {
                Fail(TEXT("dodge request totals did not show one accepted roll and one rejected cooldown request"));
                return;
            }
            if (Now - MosquitoPeerTestAcceptedRollTime < 1.25f) return;
            if (!Host->IsDodgeRollRepellingMosquitoesFromServer())
            {
                Fail(TEXT("the accepted dodge roll did not retain its two-second mosquito escape window"));
                return;
            }

            Host->EnsureToolInstanceGuidsFromServer();
            UKalmalaInventoryComponent* Inventory = Host->GetInventoryComponent();
            UKalmalaCraftingComponent* Crafting = Host->FindComponentByClass<UKalmalaCraftingComponent>();
            if (Inventory == nullptr || Crafting == nullptr)
            {
                Fail(TEXT("the owner has no inventory or crafting component for held-flame counterplay"));
                return;
            }
            if (!Inventory->TryGrantFromServer(TEXT("Wood"), 2)
                || !Inventory->TryGrantFromServer(TEXT("Amber"), 1))
            {
                Fail(TEXT("the test owner could not receive the Basic Torch recipe materials"));
                return;
            }
            FString CraftReason;
            if (!Crafting->CraftFromServer(TEXT("BasicTorch"), 1, CraftReason))
            {
                Fail(TEXT("the server could not craft the test Basic Torch from its recipe materials"));
                return;
            }
            FKalmalaToolState* Torch = Host->CarriedTools.FindByPredicate(
                [](const FKalmalaToolState& Tool) { return Tool.ItemId == TEXT("BasicTorch"); });
            if (Torch == nullptr)
            {
                Fail(TEXT("the server-crafted Basic Torch is missing from the owner loadout"));
                return;
            }
            Inventory->SynchronizeGridFromServer();
            FString Reason;
            if (!Inventory->SetMainHandFromServer(Torch->InstanceGuid, Reason))
            {
                Fail(TEXT("the server could not select the test Basic Torch"));
                return;
            }
            Host->AdvanceHeldTorchFromServer(0.01f, false);
            if (!Host->IsHeldTorchFlameEligibleForServer())
            {
                Fail(TEXT("the test torch is not a valid server flame"));
                return;
            }
            if (!PlaceSwarmAtContact(Host, Target))
            {
                Fail(TEXT("the owner and generated swarm could not be returned to the counterplay contact fixture"));
                return;
            }
            MosquitoPeerTestTorchStartedTime = Now;
            MosquitoPeerTestStageTime = Now;
            MosquitoPeerTestStage = 6;
            return;
        }

        if (MosquitoPeerTestStage == 6)
        {
            if (FailOnUnexpectedBite(TEXT("the held, fuelled torch failed to repel bites after the accepted roll expired"))) return;
            if (!Host->IsHeldTorchFlameEligibleForServer())
            {
                Fail(TEXT("the held torch lost its authoritative flame during the protection interval"));
                return;
            }
            if (Now - MosquitoPeerTestTorchStartedTime < 1.25f) return;
            if (Now - MosquitoPeerTestAcceptedRollTime < 2.05f) return;
            if (Host->IsDodgeRollRepellingMosquitoesFromServer())
            {
                Fail(TEXT("the rejected cooldown request extended the accepted roll's two-second escape window"));
                return;
            }

            UKalmalaInventoryComponent* Inventory = Host->GetInventoryComponent();
            FString Reason;
            if (Inventory == nullptr || !Inventory->SetMainHandFromServer(FGuid(), Reason))
            {
                Fail(TEXT("the server could not clear the Basic Torch for its unlit state check"));
                return;
            }
            Host->AdvanceHeldTorchFromServer(0.01f, false);
            if (Host->IsHeldTorchFlameEligibleForServer())
            {
                Fail(TEXT("an unequipped torch still qualified as an authoritative flame"));
                return;
            }
            bMosquitoPeerTestHeldTorchVerified = true;
            MosquitoPeerTestBiteBaseline = Bites;
            if (!PlaceSwarmAtContact(Host, Target))
            {
                Fail(TEXT("the swarm could not be returned after the torch was cleared"));
                return;
            }
            MosquitoPeerTestStageTime = Now;
            MosquitoPeerTestStage = 7;
            return;
        }

        if (MosquitoPeerTestStage == 7)
        {
            if (Host->IsHeldTorchFlameEligibleForServer())
            {
                Fail(TEXT("the cleared Main Hand still reports a lit torch"));
                return;
            }
            if (Bites <= MosquitoPeerTestBiteBaseline)
            {
                if (Now - MosquitoPeerTestStageTime > 10.0f)
                    Fail(TEXT("the swarm did not resume bites after the held torch was unequipped"));
                return;
            }

            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaCampfire* Fire = GetWorld()->SpawnActor<AKalmalaCampfire>(
                AKalmalaCampfire::StaticClass(), Host->GetActorLocation() + FVector(200.0f, 0.0f, 0.0f),
                FRotator::ZeroRotator, Params);
            if (Fire == nullptr || Fire->IsLit() || Fire->IsFlameEligibleForMosquitoesFromServer())
            {
                Fail(TEXT("the unlit placed-campfire fixture did not begin without flame protection"));
                return;
            }
            MosquitoPeerTestCampfire = Fire;
            MosquitoPeerTestCampfireBiteBaseline = Bites;
            if (!PlaceSwarmAtContact(Host, Target))
            {
                Fail(TEXT("the swarm could not be returned to the unlit placed-fire fixture"));
                return;
            }
            MosquitoPeerTestStageTime = Now;
            MosquitoPeerTestStage = 8;
            return;
        }

        if (MosquitoPeerTestStage == 8)
        {
            AKalmalaCampfire* Fire = MosquitoPeerTestCampfire.Get();
            if (!IsValid(Fire) || Fire->IsLit() || Fire->IsFlameEligibleForMosquitoesFromServer())
            {
                Fail(TEXT("the unlit placed campfire unexpectedly qualified as a flame"));
                return;
            }
            if (Bites <= MosquitoPeerTestCampfireBiteBaseline)
            {
                if (Now - MosquitoPeerTestStageTime > 10.0f)
                    Fail(TEXT("the swarm did not bite while the placed campfire remained unlit"));
                return;
            }
            bMosquitoPeerTestUnlitCampfireVerified = true;
            Fire->InitializePaidFromServer(Host);
            if (!Fire->TryLightFromServer(Host) || !Fire->IsFlameEligibleForMosquitoesFromServer())
            {
                Fail(TEXT("the server could not light the paid placed campfire for counterplay"));
                return;
            }
            MosquitoPeerTestBiteBaseline = Bites;
            if (!PlaceSwarmAtContact(Host, Target))
            {
                Fail(TEXT("the swarm could not be returned to the lit placed-fire fixture"));
                return;
            }
            MosquitoPeerTestStageTime = Now;
            MosquitoPeerTestStage = 9;
            return;
        }

        if (MosquitoPeerTestStage == 9)
        {
            AKalmalaCampfire* Fire = MosquitoPeerTestCampfire.Get();
            UKalmalaPlayerStatusComponent* Statuses = Host->GetStatusComponent();
            if (!IsValid(Fire) || !Fire->IsFlameEligibleForMosquitoesFromServer())
            {
                Fail(TEXT("the placed campfire lost its authoritative lit-flame state"));
                return;
            }
            if (Bites != MosquitoPeerTestBiteBaseline)
            {
                Fail(TEXT("the lit placed campfire failed to stop the swarm from biting"));
                return;
            }
            if (Now - MosquitoPeerTestStageTime < 1.25f) return;
            const float Damage = MosquitoPeerTestStartingHealth - Host->GetHealth();
            const float WeakPoisonSeconds = Statuses ? Statuses->GetRemainingSeconds(
                UKalmalaPlayerStatusComponent::WeakPoisonStatusId) : 0.0f;
            if (!bMosquitoPeerTestHeldTorchVerified || !bMosquitoPeerTestUnlitCampfireVerified
                || Host->MosquitoPeerTestAcceptedDodgeRequests != 1
                || Host->MosquitoPeerTestRejectedDodgeRequests != 1
                || Host->IsDodgeRollRepellingMosquitoesFromServer()
                || Bites < 4 || Damage < 20.0f || WeakPoisonSeconds < 8.0f)
            {
                Fail(TEXT("counterplay evidence did not retain the accepted/rejected roll, fire transitions, repeated bites, and weak poison"));
                return;
            }
            Target->SetActorTickEnabled(false);
            Target->ForceNetUpdate();
            UE_LOG(LogTemp, Display,
                TEXT("Mosquito verification server: Passed=1 Seed=%llu Bites=%d Damage=%.1f WeakPoisonSeconds=%.2f OtherPeers=2 DodgeAccepted=%d DodgeRejected=%d RollExpired=1 HeldTorchRepelled=1 HeldTorchOffNoProtection=1 CampfireUnlitNoProtection=1 CampfireLitRepelled=1 SpawnId=%s"),
                static_cast<unsigned long long>(MosquitoPeerTestWorldSeed), Bites, Damage, WeakPoisonSeconds,
                Host->MosquitoPeerTestAcceptedDodgeRequests, Host->MosquitoPeerTestRejectedDodgeRequests,
                *MosquitoPeerTestSpawnId);
            MosquitoPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DrivePeatHuskPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaPeatHuskPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || PeatHuskPeerTestStage >= 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (PeatHuskPeerTestStage == 0)
    {
        APlayerController* HostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Host = HostController != nullptr
            ? Cast<AKalmalaCharacter>(HostController->GetPawn()) : nullptr;
        if (Host == nullptr || GeneratedPlayerStart == nullptr)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Peat Husk verification FAILED: generated start or listen host was unavailable."));
                PeatHuskPeerTestStage = 99;
            }
            return;
        }

        PeatHuskPeerTestObservers.Reset();
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == Host || Candidate->GetPlayerState() == nullptr) continue;
            PeatHuskPeerTestObservers.Add(Candidate);
        }
        if (PeatHuskPeerTestObservers.Num() < 2)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Peat Husk verification FAILED: two observing client peers did not join."));
                PeatHuskPeerTestStage = 99;
            }
            return;
        }

        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation();
        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(StartLocation));
        bool bFoundCandidate = false;
        for (int32 Radius = 0; Radius <= 64 && !bFoundCandidate; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundCandidate; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    const TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    for (const FKalmalaWorldPopulationSpawn& Descriptor : Descriptors)
                    {
                        if (!KalmalaGameMode::IsWildlifeSpawnOfArchetype(
                                WorldGenerationConfig, Descriptor, EKalmalaWildlifeArchetype::PeatHusk))
                        {
                            continue;
                        }
                        const float StartDistance = FVector::Dist2D(Descriptor.Location, StartLocation);
                        if (!AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                                true, true, StartDistance))
                        {
                            continue;
                        }
                        const FString PersistentSpawnId =
                            FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Descriptor);
                        if (PersistentSpawnId.IsEmpty()
                            || (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(PersistentSpawnId)))
                        {
                            continue;
                        }

                        PeatHuskPeerTestSpatialKey = CandidateKey;
                        PeatHuskPeerTestSpawnId = PersistentSpawnId;
                        PeatHuskPeerTestStartDistance = StartDistance;
                        bFoundCandidate = true;
                        break;
                    }
                }
            }
        }
        if (!bFoundCandidate)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Peat Husk verification FAILED: no undefeated generated Peat Husk descriptor beyond the 150 m start exclusion was found within 64 spatial keys."));
            PeatHuskPeerTestStage = 99;
            return;
        }

        if (!ActivePopulationSpatialKeys.Contains(PeatHuskPeerTestSpatialKey)
            && ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Peat Husk verification FAILED: the bounded active population-key budget is full before fixture activation."));
            PeatHuskPeerTestStage = 99;
            return;
        }
        ActivatePopulationKey(PeatHuskPeerTestSpatialKey);
        PeatHuskPeerTestHost = Host;
        PeatHuskPeerTestStage = 1;
        PeatHuskPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Host = PeatHuskPeerTestHost.Get();
    if (!IsValid(Host))
    {
        UE_LOG(LogTemp, Error, TEXT("Peat Husk verification FAILED: the listen host disappeared."));
        PeatHuskPeerTestStage = 99;
        return;
    }

    if (PeatHuskPeerTestStage == 1)
    {
        AKalmalaWildlifeSpawn* Target = nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if ((*It)->GetPersistentSpawnId() == PeatHuskPeerTestSpawnId)
            {
                Target = *It;
                break;
            }
        }
        if (Target == nullptr)
        {
            if (Now - PeatHuskPeerTestStageTime > 15.0f)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("Peat Husk verification FAILED: the normal generated descriptor did not materialize after bounded activation. SpawnId=%s."),
                    *PeatHuskPeerTestSpawnId);
                PeatHuskPeerTestStage = 99;
            }
            return;
        }
        if (Target->GetArchetype() != EKalmalaWildlifeArchetype::PeatHusk
            || !FMath::IsNearlyEqual(Target->GetHealth(), 300.0f, 0.1f)
            || PeatHuskPeerTestStartDistance < AKalmalaWildlifeSpawn::HardEnemyStartExclusionCentimetres)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Peat Husk verification FAILED: generated spawn profile or start exclusion was incorrect. Archetype=%d Health=%.1f StartDistance=%.1f."),
                static_cast<int32>(Target->GetArchetype()), Target->GetHealth(), PeatHuskPeerTestStartDistance);
            PeatHuskPeerTestStage = 99;
            return;
        }

        const auto PlaceOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
        {
            if (!IsValid(Character)) return;
            ActivateTerrainPatchNeighborhood(FVector2D(DesiredLocation));
            FVector GroundedLocation = DesiredLocation;
            GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(
                WorldGenerationConfig, FVector2D(DesiredLocation))
                + Character->GetSimpleCollisionHalfHeight() + 1.0f;
            Character->SetActorLocation(GroundedLocation, false);
            Character->ForceNetUpdate();
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->StopMovementImmediately();
                Movement->DisableMovement();
            }
        };

        PeatHuskPeerTestStartingHealth = Host->GetHealth();
        if (!FMath::IsFinite(PeatHuskPeerTestStartingHealth) || PeatHuskPeerTestStartingHealth <= 1.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Peat Husk verification FAILED: listen host cannot receive the test sweep at health %.1f."),
                PeatHuskPeerTestStartingHealth);
            PeatHuskPeerTestStage = 99;
            return;
        }

        const FVector TargetLocation = Target->GetActorLocation();
        const auto IsDryMossyMirePoint = [this](const FVector2D& Position)
        {
            const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
                FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position));
            const bool bWater = FKalmalaOceanSampler::Sample(WorldGenerationConfig, Position).IsWater()
                || FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position);
            return AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(Biome, bWater);
        };
        FVector HostDesiredLocation = TargetLocation;
        bool bFoundDryApproach = false;
        for (int32 Direction = 0; Direction < 16 && !bFoundDryApproach; ++Direction)
        {
            const float Angle = FMath::DegreesToRadians(Direction * 22.5f);
            const FVector Offset(FMath::Cos(Angle) * 120.0f, FMath::Sin(Angle) * 120.0f, 0.0f);
            bool bPathIsDry = true;
            for (int32 SampleIndex = 1; SampleIndex <= 3; ++SampleIndex)
            {
                const FVector2D SamplePosition = FVector2D(TargetLocation + Offset * (SampleIndex / 3.0f));
                if (!IsDryMossyMirePoint(SamplePosition))
                {
                    bPathIsDry = false;
                    break;
                }
            }
            if (bPathIsDry)
            {
                HostDesiredLocation = TargetLocation + Offset;
                bFoundDryApproach = true;
            }
        }
        if (!bFoundDryApproach)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Peat Husk verification FAILED: generated spawn had no dry 120 cm approach for the live sweep."));
            PeatHuskPeerTestStage = 99;
            return;
        }
        PlaceOnTerrain(Host, HostDesiredLocation);
        int32 ObserverIndex = 0;
        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : PeatHuskPeerTestObservers)
        {
            AKalmalaCharacter* Observer = ObserverWeak.Get();
            if (!IsValid(Observer)) continue;
            ++ObserverIndex;
            PlaceOnTerrain(Observer,
                TargetLocation + FVector(0.0f, 1000.0f * ObserverIndex, 0.0f));
        }
        if (ObserverIndex < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Peat Husk verification FAILED: an observing client disappeared before the attack."));
            PeatHuskPeerTestStage = 99;
            return;
        }

        Target->SetActorTickEnabled(true);
        Target->ForceNetUpdate();
        PeatHuskPeerTestTarget = Target;
        PeatHuskPeerTestStage = 2;
        PeatHuskPeerTestStageTime = Now;
        UE_LOG(LogTemp, Display,
            TEXT("Peat Husk verification fixture armed: Seed=%llu SpawnId=%s StartDistanceCm=%.1f HostHealth=%.1f Observers=2."),
            static_cast<unsigned long long>(WorldGenerationConfig.WorldSeed), *PeatHuskPeerTestSpawnId,
            PeatHuskPeerTestStartDistance, PeatHuskPeerTestStartingHealth);
        return;
    }

    if (PeatHuskPeerTestStage == 2)
    {
        AKalmalaWildlifeSpawn* Target = PeatHuskPeerTestTarget.Get();
        if (!IsValid(Target))
        {
            UE_LOG(LogTemp, Error, TEXT("Peat Husk verification FAILED: generated target disappeared before its sweep."));
            PeatHuskPeerTestStage = 99;
            return;
        }

        const EKalmalaWildlifeAttackCue Cue = Target->GetPeatHuskAttackCueForVerification();
        bPeatHuskPeerTestSawTelegraph |= Cue == EKalmalaWildlifeAttackCue::Telegraph;
        bPeatHuskPeerTestSawRecovery |= Cue == EKalmalaWildlifeAttackCue::Recovery;
        const int32 SweepHits = Target->GetPeatHuskSweepHitCountForVerification();
        const float HostDamage = PeatHuskPeerTestStartingHealth - Host->GetHealth();
        if (bPeatHuskPeerTestSawTelegraph && bPeatHuskPeerTestSawRecovery
            && SweepHits > 0 && HostDamage > 0.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Peat Husk verification server: Passed=1 Seed=%llu StartDistanceCm=%.1f Telegraph=1 Recovery=1 SweepHits=%d HostDamage=%.1f Observers=2 SpawnId=%s"),
                static_cast<unsigned long long>(WorldGenerationConfig.WorldSeed), PeatHuskPeerTestStartDistance,
                SweepHits, HostDamage, *PeatHuskPeerTestSpawnId);
            PeatHuskPeerTestStage = 99;
            return;
        }

        if (Now - PeatHuskPeerTestStageTime > 18.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Peat Husk verification FAILED: generated sweep did not complete. Telegraph=%d Recovery=%d SweepHits=%d HostDamage=%.1f Cue=%d."),
                bPeatHuskPeerTestSawTelegraph, bPeatHuskPeerTestSawRecovery, SweepHits, HostDamage,
                static_cast<int32>(Cue));
            PeatHuskPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DriveWolfPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaWolfPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || WolfPeerTestStage >= 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (WolfPeerTestStage == 0)
    {
        APlayerController* HostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Host = HostController != nullptr
            ? Cast<AKalmalaCharacter>(HostController->GetPawn()) : nullptr;
        if (Host == nullptr || GeneratedPlayerStart == nullptr)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: generated start or listen host was unavailable."));
                WolfPeerTestStage = 99;
            }
            return;
        }

        WolfPeerTestObservers.Reset();
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == Host || Candidate->GetPlayerState() == nullptr) continue;
            WolfPeerTestObservers.Add(Candidate);
        }
        if (WolfPeerTestObservers.Num() < 2)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: two observing client peers did not join."));
                WolfPeerTestStage = 99;
            }
            return;
        }

        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation();
        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(StartLocation));
        bool bFoundCandidate = false;
        FVector HostDesiredLocation = FVector::ZeroVector;
        TArray<FKalmalaWorldPopulationSpawn> SelectedMemberSpawns;
        TArray<FString> SelectedMemberIds;

        for (int32 Radius = 0; Radius <= 64 && !bFoundCandidate; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundCandidate; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    const TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    for (const FKalmalaWorldPopulationSpawn& PackSpawn : Descriptors)
                    {
                        if (!KalmalaGameMode::IsWildlifeSpawnOfArchetype(
                                WorldGenerationConfig, PackSpawn, EKalmalaWildlifeArchetype::Wolf))
                        {
                            continue;
                        }

                        TArray<FKalmalaWorldPopulationSpawn> MemberSpawns;
                        TArray<FString> MemberIds;
                        bool bCompleteUndefeatedPack = true;
                        for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
                        {
                            FKalmalaWorldPopulationSpawn MemberSpawn;
                            const FString MemberId = FKalmalaWolfPackContract::GetMemberPersistentSpawnId(
                                PackSpawn, MemberIndex);
                            if (MemberId.IsEmpty()
                                || !FKalmalaWolfPackContract::TryBuildMemberSpawn(
                                    WorldGenerationConfig, PackSpawn, MemberIndex, MemberSpawn)
                                || (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(MemberId)))
                            {
                                bCompleteUndefeatedPack = false;
                                break;
                            }
                            MemberSpawns.Add(MoveTemp(MemberSpawn));
                            MemberIds.Add(MemberId);
                        }
                        if (!bCompleteUndefeatedPack
                            || MemberSpawns.Num() != FKalmalaWolfPackContract::MemberCount)
                        {
                            continue;
                        }

                        bool bFoundApproach = false;
                        for (int32 ApproachIndex = 0; ApproachIndex < 16 && !bFoundApproach; ++ApproachIndex)
                        {
                            const float Angle = static_cast<float>(ApproachIndex) * (2.0f * PI / 16.0f);
                            const FVector2D ApproachPosition(
                                MemberSpawns[0].Location.X + FMath::Cos(Angle) * 120.0f,
                                MemberSpawns[0].Location.Y + FMath::Sin(Angle) * 120.0f);
                            if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, ApproachPosition)
                                || FKalmalaBiomeClassifier::Classify(
                                    FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, ApproachPosition))
                                    != EKalmalaBiome::FreezingTundra
                                || FKalmalaTerrainHeightSampler::SampleSurfaceNormal(
                                    WorldGenerationConfig, ApproachPosition).Z < 0.82f)
                            {
                                continue;
                            }

                            FVector CandidateHostLocation(ApproachPosition.X, ApproachPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ApproachPosition));
                            if (FVector::Dist(CandidateHostLocation, MemberSpawns[0].Location)
                                > AKalmalaWildlifeSpawn::WolfBiteReachCentimetres - 10.0f)
                            {
                                continue;
                            }

                            HostDesiredLocation = CandidateHostLocation;
                            bFoundApproach = true;
                        }
                        if (!bFoundApproach) continue;

                        WolfPeerTestSpatialKey = CandidateKey;
                        WolfPeerTestPackSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(PackSpawn);
                        SelectedMemberSpawns = MoveTemp(MemberSpawns);
                        SelectedMemberIds = MoveTemp(MemberIds);
                        bFoundCandidate = !WolfPeerTestPackSpawnId.IsEmpty();
                        if (bFoundCandidate) break;
                    }
                }
            }
        }

        if (!bFoundCandidate)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Wolf verification FAILED: no undefeated generated three-member Tundra pack with a reachable host approach was found within 64 spatial keys."));
            WolfPeerTestStage = 99;
            return;
        }
        if (!ActivePopulationSpatialKeys.Contains(WolfPeerTestSpatialKey)
            && ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
        {
            UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: the bounded active population-key budget is full before fixture activation."));
            WolfPeerTestStage = 99;
            return;
        }

        const auto PlaceOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
        {
            if (!IsValid(Character)) return;
            ActivateTerrainPatchNeighborhood(FVector2D(DesiredLocation));
            FVector GroundedLocation = DesiredLocation;
            GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(
                WorldGenerationConfig, FVector2D(DesiredLocation));
            Character->SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);
            Character->ForceNetUpdate();
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->StopMovementImmediately();
                Movement->DisableMovement();
            }
        };

        WolfPeerTestStartingHealth = Host->GetHealth();
        if (!FMath::IsFinite(WolfPeerTestStartingHealth) || WolfPeerTestStartingHealth <= 1.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: listen host cannot receive the test bite at health %.1f."),
                WolfPeerTestStartingHealth);
            WolfPeerTestStage = 99;
            return;
        }

        PlaceOnTerrain(Host, HostDesiredLocation);
        int32 ObserverIndex = 0;
        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : WolfPeerTestObservers)
        {
            AKalmalaCharacter* Observer = ObserverWeak.Get();
            if (!IsValid(Observer)) continue;

            const float Distance = 1000.0f + 500.0f * static_cast<float>(ObserverIndex);
            bool bPlacedObserver = false;
            for (int32 DirectionIndex = 0; DirectionIndex < 16 && !bPlacedObserver; ++DirectionIndex)
            {
                const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                const FVector2D ObserverPosition(
                    SelectedMemberSpawns[0].Location.X + FMath::Cos(Angle) * Distance,
                    SelectedMemberSpawns[0].Location.Y + FMath::Sin(Angle) * Distance);
                if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, ObserverPosition)) continue;

                const FVector DesiredObserverLocation(ObserverPosition.X, ObserverPosition.Y,
                    FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ObserverPosition));
                PlaceOnTerrain(Observer, DesiredObserverLocation);
                bPlacedObserver = true;
            }
            if (!bPlacedObserver)
            {
                UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: an observer could not be placed outside the wolf leash."));
                WolfPeerTestStage = 99;
                return;
            }
            ++ObserverIndex;
        }
        if (ObserverIndex < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: an observing client disappeared before pack activation."));
            WolfPeerTestStage = 99;
            return;
        }

        WolfPeerTestWorldSeed = WorldGenerationConfig.WorldSeed;
        WolfPeerTestMemberSpawnIds = MoveTemp(SelectedMemberIds);
        WolfPeerTestExpectedLocations.Reset();
        for (const FKalmalaWorldPopulationSpawn& MemberSpawn : SelectedMemberSpawns)
            WolfPeerTestExpectedLocations.Add(MemberSpawn.Location);
        WolfPeerTestMembers.Reset();
        WolfPeerTestHost = Host;
        bWolfPeerTestSawTelegraph = false;
        bWolfPeerTestSawLunge = false;
        bWolfPeerTestSawRecovery = false;
        ActivatePopulationKey(WolfPeerTestSpatialKey);
        WolfPeerTestStage = 1;
        WolfPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Host = WolfPeerTestHost.Get();
    if (!IsValid(Host))
    {
        UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: listen host disappeared."));
        WolfPeerTestStage = 99;
        return;
    }

    if (WolfPeerTestStage == 1)
    {
        WolfPeerTestMembers.Reset();
        for (int32 MemberIndex = 0; MemberIndex < WolfPeerTestMemberSpawnIds.Num(); ++MemberIndex)
        {
            AKalmalaWildlifeSpawn* Member = nullptr;
            for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
            {
                if ((*It)->GetPersistentSpawnId() == WolfPeerTestMemberSpawnIds[MemberIndex])
                {
                    Member = *It;
                    break;
                }
            }
            if (Member == nullptr)
            {
                WolfPeerTestMembers.Reset();
                if (Now - WolfPeerTestStageTime > 20.0f)
                {
                    UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: generated pack member %d did not materialize."),
                        MemberIndex);
                    WolfPeerTestStage = 99;
                }
                return;
            }
            WolfPeerTestMembers.Add(Member);
        }

        if (WolfPeerTestMembers.Num() != FKalmalaWolfPackContract::MemberCount)
        {
            UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: generated pack did not contain exactly three members."));
            WolfPeerTestStage = 99;
            return;
        }
        for (int32 MemberIndex = 0; MemberIndex < WolfPeerTestMembers.Num(); ++MemberIndex)
        {
            AKalmalaWildlifeSpawn* Member = WolfPeerTestMembers[MemberIndex].Get();
            if (!IsValid(Member) || Member->GetArchetype() != EKalmalaWildlifeArchetype::Wolf
                || !FMath::IsNearlyEqual(Member->GetHealth(), 250.0f, 0.1f)
                || FVector::Dist2D(Member->GetActorLocation(), WolfPeerTestExpectedLocations[MemberIndex]) > 60.0f)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("Wolf verification FAILED: generated member identity, 250 health, or stable formation was invalid at member %d."),
                    MemberIndex);
                WolfPeerTestStage = 99;
                return;
            }
        }
        const float ExpectedSpacing = FKalmalaWolfPackContract::FormationRadiusCentimetres * FMath::Sqrt(3.0f);
        for (int32 First = 0; First < WolfPeerTestMembers.Num(); ++First)
        {
            for (int32 Second = First + 1; Second < WolfPeerTestMembers.Num(); ++Second)
            {
                const float Spacing = FVector::Dist2D(WolfPeerTestMembers[First]->GetActorLocation(),
                    WolfPeerTestMembers[Second]->GetActorLocation());
                if (!FMath::IsNearlyEqual(Spacing, ExpectedSpacing, 100.0f))
                {
                    UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: pack formation spacing %.1f was unexpected."),
                        Spacing);
                    WolfPeerTestStage = 99;
                    return;
                }
            }
        }

        UE_LOG(LogTemp, Display,
            TEXT("Wolf verification fixture armed: Seed=%llu PackMembers=3 MemberSpawnId=%s Observers=2."),
            static_cast<unsigned long long>(WolfPeerTestWorldSeed), *WolfPeerTestMemberSpawnIds[0]);
        WolfPeerTestStage = 2;
        WolfPeerTestStageTime = Now;
        return;
    }

    if (WolfPeerTestStage == 2)
    {
        AKalmalaWildlifeSpawn* AttackWolf = WolfPeerTestMembers.Num() > 0
            ? WolfPeerTestMembers[0].Get() : nullptr;
        if (!IsValid(AttackWolf))
        {
            UE_LOG(LogTemp, Error, TEXT("Wolf verification FAILED: the attacking pack member disappeared."));
            WolfPeerTestStage = 99;
            return;
        }

        const EKalmalaWildlifeAttackCue Cue = AttackWolf->GetWolfAttackCueForVerification();
        bWolfPeerTestSawTelegraph |= Cue == EKalmalaWildlifeAttackCue::Telegraph;
        bWolfPeerTestSawLunge |= Cue == EKalmalaWildlifeAttackCue::Lunge;
        bWolfPeerTestSawRecovery |= Cue == EKalmalaWildlifeAttackCue::Recovery;
        const int32 BiteHits = AttackWolf->GetWolfBiteHitCountForVerification();
        const float HostDamage = WolfPeerTestStartingHealth - Host->GetHealth();
        if (bWolfPeerTestSawTelegraph && bWolfPeerTestSawLunge && bWolfPeerTestSawRecovery
            && BiteHits > 0 && HostDamage > 0.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Wolf verification server: Passed=1 Seed=%llu PackMembers=3 Telegraph=1 Lunge=1 Recovery=1 BiteHits=%d HostDamage=%.1f PackSpawnId=%s MemberSpawnId=%s"),
                static_cast<unsigned long long>(WolfPeerTestWorldSeed), BiteHits, HostDamage,
                *WolfPeerTestPackSpawnId, *WolfPeerTestMemberSpawnIds[0]);
            WolfPeerTestStage = 99;
            return;
        }
        if (Now - WolfPeerTestStageTime > 18.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Wolf verification FAILED: server lunge bite did not complete. Telegraph=%d Lunge=%d Recovery=%d BiteHits=%d HostDamage=%.1f Cue=%d."),
                bWolfPeerTestSawTelegraph, bWolfPeerTestSawLunge, bWolfPeerTestSawRecovery, BiteHits, HostDamage,
                static_cast<int32>(Cue));
            WolfPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DriveLynxPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaLynxPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || LynxPeerTestStage >= 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const auto PlaceOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
    {
        if (!IsValid(Character)) return;
        ActivateTerrainPatchNeighborhood(FVector2D(DesiredLocation));
        FVector GroundedLocation = DesiredLocation;
        GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(
            WorldGenerationConfig, FVector2D(DesiredLocation))
            + Character->GetSimpleCollisionHalfHeight() + 1.0f;
        Character->SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Character->ForceNetUpdate();
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
            Movement->DisableMovement();
        }
    };
    if (LynxPeerTestStage == 0)
    {
        APlayerController* HostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Host = HostController != nullptr
            ? Cast<AKalmalaCharacter>(HostController->GetPawn()) : nullptr;
        if (Host == nullptr || GeneratedPlayerStart == nullptr)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: generated start or listen host was unavailable."));
                LynxPeerTestStage = 99;
            }
            return;
        }

        LynxPeerTestObservers.Reset();
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == Host || Candidate->GetPlayerState() == nullptr) continue;
            LynxPeerTestObservers.Add(Candidate);
        }
        if (LynxPeerTestObservers.Num() < 2)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: two observing client peers did not join."));
                LynxPeerTestStage = 99;
            }
            return;
        }

        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation();
        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(StartLocation));
        const auto IsDryTundraSurface = [this](const FVector2D& Position)
        {
            return FKalmalaWorldBounds::Contains(WorldGenerationConfig, Position)
                && FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position))
                    == EKalmalaBiome::FreezingTundra
                && !FKalmalaOceanSampler::Sample(WorldGenerationConfig, Position).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position)
                && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(WorldGenerationConfig, Position).Z >= 0.82f;
        };
        bool bFoundCandidate = false;
        FVector HostDesiredLocation = FVector::ZeroVector;
        FVector HostRingDesiredLocation = FVector::ZeroVector;

        for (int32 Radius = 0; Radius <= 64 && !bFoundCandidate; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundCandidate; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    const FVector2D KeyCentre = (FVector2D(CandidateKey) + FVector2D(0.5f, 0.5f))
                        * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
                    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                        FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, KeyCentre));
                    if (KeyBiome == EKalmalaBiome::ShimmeringLakes || KeyBiome == EKalmalaBiome::Elderwood
                        || KeyBiome == EKalmalaBiome::MossyMire || KeyBiome == EKalmalaBiome::FreezingTundra
                        || KeyBiome == EKalmalaBiome::ThunderMountains)
                    {
                        Descriptors.SetNum(FMath::Min(Descriptors.Num(),
                            FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                                Descriptors.Num(), EKalmalaWorldPopulationKind::Wildlife, KeyBiome)));
                    }
                    const int32 WildlifeActorBudget = KeyBiome == EKalmalaBiome::FreezingTundra
                        ? FKalmalaWolfPackContract::GetActorBudgetWithPackRoom(
                            FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                                Descriptors.Num(), EKalmalaWorldPopulationKind::Wildlife, KeyBiome))
                        : Descriptors.Num();
                    int32 SpawnedWildlifeActorCount = 0;
                    for (const FKalmalaWorldPopulationSpawn& Spawn : Descriptors)
                    {
                        EKalmalaWildlifeArchetype Archetype;
                        if (!KalmalaGameMode::TryGetWildlifeArchetypeForSpawn(
                                WorldGenerationConfig, Spawn, Archetype))
                        {
                            continue;
                        }
                        const bool bHardEnemy = Archetype == EKalmalaWildlifeArchetype::GiantSnake
                            || Archetype == EKalmalaWildlifeArchetype::MoonBear
                            || Archetype == EKalmalaWildlifeArchetype::PeatHusk
                            || Archetype == EKalmalaWildlifeArchetype::PolarBear
                            || Archetype == EKalmalaWildlifeArchetype::ThunderSentinel
                            || Archetype == EKalmalaWildlifeArchetype::ReefMaw;
                        const float SpawnStartDistance = GeneratedPlayerStart != nullptr
                            ? FVector::Dist2D(Spawn.Location, GeneratedPlayerStart->GetActorLocation()) : 0.0f;
                        if (!AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                                bHardEnemy, GeneratedPlayerStart != nullptr, SpawnStartDistance))
                        {
                            continue;
                        }
                        const FString SpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                        if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(SpawnId))
                            continue;

                        if (Archetype == EKalmalaWildlifeArchetype::Wolf)
                        {
                            if (SpawnedWildlifeActorCount + FKalmalaWolfPackContract::MemberCount > WildlifeActorBudget)
                                continue;
                            int32 PackMemberCount = 0;
                            bool bValidFormation = true;
                            for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
                            {
                                FKalmalaWorldPopulationSpawn MemberSpawn;
                                if (!FKalmalaWolfPackContract::TryBuildMemberSpawn(
                                        WorldGenerationConfig, Spawn, MemberIndex, MemberSpawn))
                                {
                                    bValidFormation = false;
                                    break;
                                }
                                const FString MemberSpawnId = FKalmalaWolfPackContract::GetMemberPersistentSpawnId(
                                    Spawn, MemberIndex);
                                if (PopulationSaveGame == nullptr || !PopulationSaveGame->IsDefeated(MemberSpawnId))
                                    ++PackMemberCount;
                            }
                            if (bValidFormation) SpawnedWildlifeActorCount += PackMemberCount;
                            continue;
                        }
                        if (SpawnedWildlifeActorCount >= WildlifeActorBudget) continue;
                        if (Archetype != EKalmalaWildlifeArchetype::Lynx)
                        {
                            ++SpawnedWildlifeActorCount;
                            continue;
                        }
                        const FVector2D SpawnPosition(Spawn.Location);
                        if (!IsDryTundraSurface(SpawnPosition)) continue;

                        for (int32 ApproachIndex = 0; ApproachIndex < 16; ++ApproachIndex)
                        {
                            const float Angle = static_cast<float>(ApproachIndex) * (2.0f * PI / 16.0f);
                            const FVector2D ApproachDirection(FMath::Cos(Angle), FMath::Sin(Angle));
                            const FVector2D ApproachPosition(
                                SpawnPosition.X + ApproachDirection.X * 120.0f,
                                SpawnPosition.Y + ApproachDirection.Y * 120.0f);
                            const FVector2D RingPosition = SpawnPosition + ApproachDirection * 5000.0f;
                            if (!IsDryTundraSurface(ApproachPosition)) continue;
                            if (!IsDryTundraSurface(RingPosition)) continue;
                            const int32 PathSamples = FMath::Max(1,
                                FMath::CeilToInt(FVector2D::Distance(SpawnPosition, ApproachPosition) / 80.0f));
                            bool bPathTraversable = true;
                            for (int32 PathIndex = 0; PathIndex <= PathSamples; ++PathIndex)
                            {
                                const FVector2D PathPosition = FMath::Lerp(SpawnPosition, ApproachPosition,
                                    static_cast<float>(PathIndex) / PathSamples);
                                if (!IsDryTundraSurface(PathPosition))
                                {
                                    bPathTraversable = false;
                                    break;
                                }
                            }
                            if (!bPathTraversable) continue;

                            HostDesiredLocation = FVector(ApproachPosition.X, ApproachPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ApproachPosition));
                            if (FVector::Dist(HostDesiredLocation, Spawn.Location)
                                > AKalmalaWildlifeSpawn::LynxLeapImpactReachCentimetres - 10.0f)
                            {
                                continue;
                            }

                            HostRingDesiredLocation = FVector(RingPosition.X, RingPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, RingPosition));
                            LynxPeerTestSpatialKey = CandidateKey;
                            LynxPeerTestSpawnId = SpawnId;
                            LynxPeerTestSpawnLocation = Spawn.Location;
                            LynxPeerTestHostRingLocation = HostRingDesiredLocation;
                            LynxPeerTestHostLocation = HostDesiredLocation;
                            bFoundCandidate = true;
                            break;
                        }
                        if (bFoundCandidate) break;
                    }
                }
            }
        }

        if (!bFoundCandidate)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Lynx verification FAILED: no undefeated materializable Tundra Lynx with a reachable dry approach was found within 64 spatial keys."));
            LynxPeerTestStage = 99;
            return;
        }
        if (!ActivePopulationSpatialKeys.Contains(LynxPeerTestSpatialKey)
            && ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
        {
            UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: the bounded active population-key budget is full before fixture activation."));
            LynxPeerTestStage = 99;
            return;
        }

        LynxPeerTestStartingHealth = Host->GetHealth();
        if (!FMath::IsFinite(LynxPeerTestStartingHealth) || LynxPeerTestStartingHealth <= 75.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: listen host cannot receive the test leap at health %.1f."),
                LynxPeerTestStartingHealth);
            LynxPeerTestStage = 99;
            return;
        }

        PlaceOnTerrain(Host, LynxPeerTestHostRingLocation);
        LynxPeerTestWorldSeed = WorldGenerationConfig.WorldSeed;
        LynxPeerTestHost = Host;
        LynxPeerTestTarget.Reset();
        bLynxPeerTestSawTelegraph = false;
        bLynxPeerTestSawLeap = false;
        bLynxPeerTestSawRecovery = false;
        ActivatePopulationKey(LynxPeerTestSpatialKey);
        LynxPeerTestStage = 1;
        LynxPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Host = LynxPeerTestHost.Get();
    if (!IsValid(Host))
    {
        UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: listen host disappeared."));
        LynxPeerTestStage = 99;
        return;
    }

    if (LynxPeerTestStage == 1)
    {
        AKalmalaWildlifeSpawn* Target = nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if ((*It)->GetPersistentSpawnId() == LynxPeerTestSpawnId)
            {
                Target = *It;
                break;
            }
        }
        if (Target == nullptr)
        {
            if (Now - LynxPeerTestStageTime > 20.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: generated Lynx descriptor did not materialize."));
                LynxPeerTestStage = 99;
            }
            return;
        }
        if (Target->GetArchetype() != EKalmalaWildlifeArchetype::Lynx
            || !FMath::IsNearlyEqual(Target->GetHealth(), 150.0f, 0.1f))
        {
            UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: generated target archetype or 150-health profile was invalid."));
            LynxPeerTestStage = 99;
            return;
        }

        PlaceOnTerrain(Host, LynxPeerTestHostLocation);

        int32 ObserverIndex = 0;
        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : LynxPeerTestObservers)
        {
            AKalmalaCharacter* Observer = ObserverWeak.Get();
            if (!IsValid(Observer)) continue;

            const float Distance = 1000.0f + 500.0f * static_cast<float>(ObserverIndex);
            bool bPlacedObserver = false;
            for (int32 DirectionIndex = 0; DirectionIndex < 16 && !bPlacedObserver; ++DirectionIndex)
            {
                const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                const FVector2D ObserverPosition(
                    LynxPeerTestSpawnLocation.X + FMath::Cos(Angle) * Distance,
                    LynxPeerTestSpawnLocation.Y + FMath::Sin(Angle) * Distance);
                if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, ObserverPosition)) continue;
                const FVector DesiredObserverLocation(ObserverPosition.X, ObserverPosition.Y,
                    FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ObserverPosition));
                PlaceOnTerrain(Observer, DesiredObserverLocation);
                bPlacedObserver = true;
            }
            if (!bPlacedObserver)
            {
                UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: an observer could not be placed within replication range."));
                LynxPeerTestStage = 99;
                return;
            }
            ++ObserverIndex;
        }
        if (ObserverIndex < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: an observing client disappeared before the attack."));
            LynxPeerTestStage = 99;
            return;
        }

        LynxPeerTestTarget = Target;
        Target->ForceNetUpdate();
        UE_LOG(LogTemp, Display,
            TEXT("Lynx verification fixture armed: Seed=%llu SpawnId=%s HostDistanceCm=%.1f Observers=2."),
            static_cast<unsigned long long>(LynxPeerTestWorldSeed), *LynxPeerTestSpawnId,
            FVector::Dist(Host->GetActorLocation(), Target->GetActorLocation()));
        LynxPeerTestStage = 2;
        LynxPeerTestStageTime = Now;
        return;
    }

    if (LynxPeerTestStage == 2)
    {
        AKalmalaWildlifeSpawn* Target = LynxPeerTestTarget.Get();
        if (!IsValid(Target))
        {
            UE_LOG(LogTemp, Error, TEXT("Lynx verification FAILED: generated Lynx disappeared before its leap."));
            LynxPeerTestStage = 99;
            return;
        }

        const EKalmalaWildlifeAttackCue Cue = Target->GetLynxAttackCueForVerification();
        bLynxPeerTestSawTelegraph |= Cue == EKalmalaWildlifeAttackCue::Telegraph;
        bLynxPeerTestSawLeap |= Cue == EKalmalaWildlifeAttackCue::Lunge;
        bLynxPeerTestSawRecovery |= Cue == EKalmalaWildlifeAttackCue::Recovery;
        const float HostDamage = LynxPeerTestStartingHealth - Host->GetHealth();
        if (bLynxPeerTestSawTelegraph && bLynxPeerTestSawLeap && bLynxPeerTestSawRecovery && HostDamage > 0.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Lynx verification server: Passed=1 Seed=%llu Telegraph=1 Leap=1 Recovery=1 HostDamage=%.1f SpawnId=%s"),
                static_cast<unsigned long long>(LynxPeerTestWorldSeed), HostDamage, *LynxPeerTestSpawnId);
            LynxPeerTestStage = 99;
            return;
        }
        if (Now - LynxPeerTestStageTime > 18.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Lynx verification FAILED: generated leap did not complete. Telegraph=%d Leap=%d Recovery=%d HostDamage=%.1f Cue=%d."),
                bLynxPeerTestSawTelegraph, bLynxPeerTestSawLeap, bLynxPeerTestSawRecovery, HostDamage,
                static_cast<int32>(Cue));
            LynxPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DrivePolarBearPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaPolarBearPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || PolarBearPeerTestStage >= 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const auto PlaceOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
    {
        if (!IsValid(Character)) return;
        ActivateTerrainPatchNeighborhood(FVector2D(DesiredLocation));
        FVector GroundedLocation = DesiredLocation;
        GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(
            WorldGenerationConfig, FVector2D(DesiredLocation))
            + Character->GetSimpleCollisionHalfHeight() + 1.0f;
        Character->SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Character->ForceNetUpdate();
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
            Movement->DisableMovement();
        }
    };
    if (PolarBearPeerTestStage == 0)
    {
        APlayerController* HostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Host = HostController != nullptr
            ? Cast<AKalmalaCharacter>(HostController->GetPawn()) : nullptr;
        if (Host == nullptr || GeneratedPlayerStart == nullptr)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: generated start or listen host was unavailable."));
                PolarBearPeerTestStage = 99;
            }
            return;
        }

        PolarBearPeerTestObservers.Reset();
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == Host || Candidate->GetPlayerState() == nullptr) continue;
            PolarBearPeerTestObservers.Add(Candidate);
        }
        if (PolarBearPeerTestObservers.Num() < 2)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: two observing client peers did not join."));
                PolarBearPeerTestStage = 99;
            }
            return;
        }

        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation();
        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(StartLocation));
        const auto IsDryTundraSurface = [this](const FVector2D& Position)
        {
            return FKalmalaWorldBounds::Contains(WorldGenerationConfig, Position)
                && FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position))
                    == EKalmalaBiome::FreezingTundra
                && !FKalmalaOceanSampler::Sample(WorldGenerationConfig, Position).IsWater()
                && !FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position)
                && FKalmalaTerrainHeightSampler::SampleSurfaceNormal(WorldGenerationConfig, Position).Z >= 0.82f;
        };
        bool bFoundCandidate = false;
        FVector HostDesiredLocation = FVector::ZeroVector;
        FVector HostRingDesiredLocation = FVector::ZeroVector;

        for (int32 Radius = 0; Radius <= 64 && !bFoundCandidate; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundCandidate; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    const FVector2D KeyCentre = (FVector2D(CandidateKey) + FVector2D(0.5f, 0.5f))
                        * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
                    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                        FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, KeyCentre));
                    if (KeyBiome == EKalmalaBiome::ShimmeringLakes || KeyBiome == EKalmalaBiome::Elderwood
                        || KeyBiome == EKalmalaBiome::MossyMire || KeyBiome == EKalmalaBiome::FreezingTundra
                        || KeyBiome == EKalmalaBiome::ThunderMountains)
                    {
                        Descriptors.SetNum(FMath::Min(Descriptors.Num(),
                            FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                                Descriptors.Num(), EKalmalaWorldPopulationKind::Wildlife, KeyBiome)));
                    }
                    const int32 WildlifeActorBudget = KeyBiome == EKalmalaBiome::FreezingTundra
                        ? FKalmalaWolfPackContract::GetActorBudgetWithPackRoom(
                            FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                                Descriptors.Num(), EKalmalaWorldPopulationKind::Wildlife, KeyBiome))
                        : Descriptors.Num();
                    int32 SpawnedWildlifeActorCount = 0;
                    for (const FKalmalaWorldPopulationSpawn& Spawn : Descriptors)
                    {
                        EKalmalaWildlifeArchetype Archetype;
                        if (!KalmalaGameMode::TryGetWildlifeArchetypeForSpawn(
                                WorldGenerationConfig, Spawn, Archetype))
                        {
                            continue;
                        }
                        const bool bHardEnemy = Archetype == EKalmalaWildlifeArchetype::GiantSnake
                            || Archetype == EKalmalaWildlifeArchetype::MoonBear
                            || Archetype == EKalmalaWildlifeArchetype::PeatHusk
                            || Archetype == EKalmalaWildlifeArchetype::PolarBear
                            || Archetype == EKalmalaWildlifeArchetype::ThunderSentinel
                            || Archetype == EKalmalaWildlifeArchetype::ReefMaw;
                        const float SpawnStartDistance = GeneratedPlayerStart != nullptr
                            ? FVector::Dist2D(Spawn.Location, GeneratedPlayerStart->GetActorLocation()) : 0.0f;
                        if (!AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                                bHardEnemy, GeneratedPlayerStart != nullptr, SpawnStartDistance))
                        {
                            continue;
                        }
                        const FString SpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                        if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(SpawnId))
                            continue;

                        if (Archetype == EKalmalaWildlifeArchetype::Wolf)
                        {
                            if (SpawnedWildlifeActorCount + FKalmalaWolfPackContract::MemberCount > WildlifeActorBudget)
                                continue;
                            int32 PackMemberCount = 0;
                            bool bValidFormation = true;
                            for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
                            {
                                FKalmalaWorldPopulationSpawn MemberSpawn;
                                if (!FKalmalaWolfPackContract::TryBuildMemberSpawn(
                                        WorldGenerationConfig, Spawn, MemberIndex, MemberSpawn))
                                {
                                    bValidFormation = false;
                                    break;
                                }
                                const FString MemberSpawnId = FKalmalaWolfPackContract::GetMemberPersistentSpawnId(
                                    Spawn, MemberIndex);
                                if (PopulationSaveGame == nullptr || !PopulationSaveGame->IsDefeated(MemberSpawnId))
                                    ++PackMemberCount;
                            }
                            if (bValidFormation) SpawnedWildlifeActorCount += PackMemberCount;
                            continue;
                        }
                        if (SpawnedWildlifeActorCount >= WildlifeActorBudget) continue;
                        if (Archetype != EKalmalaWildlifeArchetype::PolarBear)
                        {
                            ++SpawnedWildlifeActorCount;
                            continue;
                        }
                        const FVector2D SpawnPosition(Spawn.Location);
                        if (!IsDryTundraSurface(SpawnPosition)) continue;

                        for (int32 ApproachIndex = 0; ApproachIndex < 16; ++ApproachIndex)
                        {
                            const float Angle = static_cast<float>(ApproachIndex) * (2.0f * PI / 16.0f);
                            const FVector2D ApproachDirection(FMath::Cos(Angle), FMath::Sin(Angle));
                            const FVector2D ApproachPosition(
                                SpawnPosition.X + ApproachDirection.X * 120.0f,
                                SpawnPosition.Y + ApproachDirection.Y * 120.0f);
                            const FVector2D RingPosition = SpawnPosition + ApproachDirection * 5000.0f;
                            if (!IsDryTundraSurface(ApproachPosition)) continue;
                            if (!IsDryTundraSurface(RingPosition)) continue;
                            const int32 PathSamples = FMath::Max(1,
                                FMath::CeilToInt(FVector2D::Distance(SpawnPosition, ApproachPosition) / 80.0f));
                            bool bPathTraversable = true;
                            for (int32 PathIndex = 0; PathIndex <= PathSamples; ++PathIndex)
                            {
                                const FVector2D PathPosition = FMath::Lerp(SpawnPosition, ApproachPosition,
                                    static_cast<float>(PathIndex) / PathSamples);
                                if (!IsDryTundraSurface(PathPosition))
                                {
                                    bPathTraversable = false;
                                    break;
                                }
                            }
                            if (!bPathTraversable) continue;

                            HostDesiredLocation = FVector(ApproachPosition.X, ApproachPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ApproachPosition));
                            if (FVector::Dist(HostDesiredLocation, Spawn.Location)
                                > AKalmalaWildlifeSpawn::PolarBearChargeImpactReachCentimetres - 10.0f)
                            {
                                continue;
                            }

                            HostRingDesiredLocation = FVector(RingPosition.X, RingPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, RingPosition));
                            PolarBearPeerTestSpatialKey = CandidateKey;
                            PolarBearPeerTestSpawnId = SpawnId;
                            PolarBearPeerTestSpawnLocation = Spawn.Location;
                            PolarBearPeerTestStartDistance = SpawnStartDistance;
                            PolarBearPeerTestHostRingLocation = HostRingDesiredLocation;
                            PolarBearPeerTestHostLocation = HostDesiredLocation;
                            bFoundCandidate = true;
                            break;
                        }
                        if (bFoundCandidate) break;
                    }
                }
            }
        }

        if (!bFoundCandidate)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Polar Bear verification FAILED: no undefeated materializable Tundra Polar Bear with a reachable dry approach was found within 64 spatial keys."));
            PolarBearPeerTestStage = 99;
            return;
        }
        if (!ActivePopulationSpatialKeys.Contains(PolarBearPeerTestSpatialKey)
            && ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
        {
            UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: the bounded active population-key budget is full before fixture activation."));
            PolarBearPeerTestStage = 99;
            return;
        }

        PolarBearPeerTestStartingHealth = 300.0f;
        // Keep the Development-only host alive through the 200-Crush charge and recovery.
        Host->Health = PolarBearPeerTestStartingHealth;
        Host->ForceNetUpdate();
        if (!FMath::IsFinite(PolarBearPeerTestStartingHealth) || PolarBearPeerTestStartingHealth <= 200.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: listen host cannot receive the test charge at health %.1f."),
                PolarBearPeerTestStartingHealth);
            PolarBearPeerTestStage = 99;
            return;
        }

        PlaceOnTerrain(Host, PolarBearPeerTestHostRingLocation);
        PolarBearPeerTestWorldSeed = WorldGenerationConfig.WorldSeed;
        PolarBearPeerTestHost = Host;
        PolarBearPeerTestTarget.Reset();
        bPolarBearPeerTestSawTelegraph = false;
        bPolarBearPeerTestSawCharge = false;
        bPolarBearPeerTestSawRetreat = false;
        bPolarBearPeerTestSawRecovery = false;
        ActivatePopulationKey(PolarBearPeerTestSpatialKey);
        PolarBearPeerTestStage = 1;
        PolarBearPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Host = PolarBearPeerTestHost.Get();
    if (!IsValid(Host))
    {
        UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: listen host disappeared."));
        PolarBearPeerTestStage = 99;
        return;
    }

    if (PolarBearPeerTestStage == 1)
    {
        AKalmalaWildlifeSpawn* Target = nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if ((*It)->GetPersistentSpawnId() == PolarBearPeerTestSpawnId)
            {
                Target = *It;
                break;
            }
        }
        if (Target == nullptr)
        {
            if (Now - PolarBearPeerTestStageTime > 20.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: generated Polar Bear descriptor did not materialize."));
                PolarBearPeerTestStage = 99;
            }
            return;
        }
        if (Target->GetArchetype() != EKalmalaWildlifeArchetype::PolarBear
            || !AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                true, true, PolarBearPeerTestStartDistance)
            || !FMath::IsNearlyEqual(Target->GetHealth(), 750.0f, 0.1f))
        {
            UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: generated archetype, start exclusion, or 750-health profile was invalid."));
            PolarBearPeerTestStage = 99;
            return;
        }

        PlaceOnTerrain(Host, PolarBearPeerTestHostLocation);

        int32 ObserverIndex = 0;
        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : PolarBearPeerTestObservers)
        {
            AKalmalaCharacter* Observer = ObserverWeak.Get();
            if (!IsValid(Observer)) continue;

            const float Distance = 1000.0f + 500.0f * static_cast<float>(ObserverIndex);
            bool bPlacedObserver = false;
            for (int32 DirectionIndex = 0; DirectionIndex < 16 && !bPlacedObserver; ++DirectionIndex)
            {
                const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                const FVector2D ObserverPosition(
                    PolarBearPeerTestSpawnLocation.X + FMath::Cos(Angle) * Distance,
                    PolarBearPeerTestSpawnLocation.Y + FMath::Sin(Angle) * Distance);
                if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, ObserverPosition)) continue;
                const FVector DesiredObserverLocation(ObserverPosition.X, ObserverPosition.Y,
                    FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ObserverPosition));
                PlaceOnTerrain(Observer, DesiredObserverLocation);
                bPlacedObserver = true;
            }
            if (!bPlacedObserver)
            {
                UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: an observer could not be placed within replication range."));
                PolarBearPeerTestStage = 99;
                return;
            }
            ++ObserverIndex;
        }
        if (ObserverIndex < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: an observing client disappeared before the attack."));
            PolarBearPeerTestStage = 99;
            return;
        }

        PolarBearPeerTestTarget = Target;
        Target->ForceNetUpdate();
        UE_LOG(LogTemp, Display,
            TEXT("Polar Bear verification fixture armed: Seed=%llu SpawnId=%s StartDistanceCm=%.1f HostDistanceCm=%.1f Observers=2."),
            static_cast<unsigned long long>(PolarBearPeerTestWorldSeed), *PolarBearPeerTestSpawnId,
            PolarBearPeerTestStartDistance, FVector::Dist(Host->GetActorLocation(), Target->GetActorLocation()));
        PolarBearPeerTestStage = 2;
        PolarBearPeerTestStageTime = Now;
        return;
    }

    if (PolarBearPeerTestStage == 2)
    {
        AKalmalaWildlifeSpawn* Target = PolarBearPeerTestTarget.Get();
        if (!IsValid(Target))
        {
            UE_LOG(LogTemp, Error, TEXT("Polar Bear verification FAILED: generated Polar Bear disappeared before its charge."));
            PolarBearPeerTestStage = 99;
            return;
        }

        const EKalmalaWildlifeAttackCue Cue = Target->GetPolarBearAttackCueForVerification();
        bPolarBearPeerTestSawTelegraph |= Cue == EKalmalaWildlifeAttackCue::Telegraph;
        bPolarBearPeerTestSawCharge |= Cue == EKalmalaWildlifeAttackCue::Lunge;
        bPolarBearPeerTestSawRetreat |= Cue == EKalmalaWildlifeAttackCue::Retreat;
        bPolarBearPeerTestSawRecovery |= Cue == EKalmalaWildlifeAttackCue::Recovery;
        const float HostDamage = PolarBearPeerTestStartingHealth - Host->GetHealth();
        if (bPolarBearPeerTestSawTelegraph && bPolarBearPeerTestSawCharge && bPolarBearPeerTestSawRetreat
            && bPolarBearPeerTestSawRecovery && HostDamage > 0.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Polar Bear verification server: Passed=1 Seed=%llu Telegraph=1 Charge=1 Retreat=1 Recovery=1 HostDamage=%.1f SpawnId=%s"),
                static_cast<unsigned long long>(PolarBearPeerTestWorldSeed), HostDamage, *PolarBearPeerTestSpawnId);
            PolarBearPeerTestStage = 99;
            return;
        }
        if (Now - PolarBearPeerTestStageTime > 18.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Polar Bear verification FAILED: generated charge did not complete. Telegraph=%d Charge=%d Retreat=%d Recovery=%d HostDamage=%.1f Cue=%d."),
                bPolarBearPeerTestSawTelegraph, bPolarBearPeerTestSawCharge, bPolarBearPeerTestSawRetreat,
                bPolarBearPeerTestSawRecovery, HostDamage,
                static_cast<int32>(Cue));
            PolarBearPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DriveHornedGoatPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaHornedGoatPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || HornedGoatPeerTestStage >= 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const auto PlaceOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
    {
        if (!IsValid(Character)) return;
        ActivateTerrainPatchNeighborhood(FVector2D(DesiredLocation));
        FVector GroundedLocation = DesiredLocation;
        GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(
            WorldGenerationConfig, FVector2D(DesiredLocation))
            + Character->GetSimpleCollisionHalfHeight() + 1.0f;
        Character->SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Character->ForceNetUpdate();
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
            if (Movement->MovementMode == MOVE_None) Movement->SetMovementMode(MOVE_Walking);
        }
    };
    const auto IsDryMountainSurface = [this](const FVector2D& Position)
    {
        if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, Position)) return false;
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position));
        const bool bWater = FKalmalaOceanSampler::Sample(WorldGenerationConfig, Position).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position);
        const bool bTraversable = FKalmalaTerrainHeightSampler::SampleSurfaceNormal(
            WorldGenerationConfig, Position).Z >= 0.82f;
        return AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(Biome, bWater, bTraversable);
    };
    const auto IsSafeMountainPath = [&IsDryMountainSurface, this](const FVector2D& Start, const FVector2D& End)
    {
        if (!IsDryMountainSurface(Start) || !IsDryMountainSurface(End)) return false;
        const float StartHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Start);
        const float Distance = FVector2D::Distance(Start, End);
        const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
        for (int32 Index = 0; Index <= Samples; ++Index)
        {
            const FVector2D Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
            if (!IsDryMountainSurface(Position)
                || FMath::Abs(FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Position)
                    - StartHeight) > 80.0f)
            {
                return false;
            }
        }
        return true;
    };

    if (HornedGoatPeerTestStage == 0)
    {
        APlayerController* HostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Host = HostController != nullptr
            ? Cast<AKalmalaCharacter>(HostController->GetPawn()) : nullptr;
        if (Host == nullptr || GeneratedPlayerStart == nullptr)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: generated start or listen host was unavailable."));
                HornedGoatPeerTestStage = 99;
            }
            return;
        }

        HornedGoatPeerTestObservers.Reset();
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == Host || Candidate->GetPlayerState() == nullptr) continue;
            HornedGoatPeerTestObservers.Add(Candidate);
        }
        if (HornedGoatPeerTestObservers.Num() < 2)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: two observing client peers did not join."));
                HornedGoatPeerTestStage = 99;
            }
            return;
        }

        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(
            FVector2D(GeneratedPlayerStart->GetActorLocation()));
        bool bFoundCandidate = false;
        for (int32 Radius = 0; Radius <= 64 && !bFoundCandidate; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundCandidate; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    const FVector2D KeyCentre = (FVector2D(CandidateKey) + FVector2D(0.5f, 0.5f))
                        * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
                    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                        FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, KeyCentre));
                    if (KeyBiome != EKalmalaBiome::ThunderMountains) continue;

                    TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    Descriptors.SetNum(FMath::Min(Descriptors.Num(),
                        FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                            Descriptors.Num(), EKalmalaWorldPopulationKind::Wildlife, KeyBiome)));
                    const int32 WildlifeActorBudget = Descriptors.Num();
                    int32 SpawnedWildlifeActorCount = 0;

                    for (const FKalmalaWorldPopulationSpawn& Spawn : Descriptors)
                    {
                        EKalmalaWildlifeArchetype Archetype;
                        if (!KalmalaGameMode::TryGetWildlifeArchetypeForSpawn(
                                WorldGenerationConfig, Spawn, Archetype))
                        {
                            continue;
                        }
                        const bool bHardEnemy = Archetype == EKalmalaWildlifeArchetype::GiantSnake
                            || Archetype == EKalmalaWildlifeArchetype::MoonBear
                            || Archetype == EKalmalaWildlifeArchetype::PeatHusk
                            || Archetype == EKalmalaWildlifeArchetype::PolarBear
                            || Archetype == EKalmalaWildlifeArchetype::ThunderSentinel
                            || Archetype == EKalmalaWildlifeArchetype::ReefMaw;
                        const float StartDistance = FVector::Dist2D(
                            Spawn.Location, GeneratedPlayerStart->GetActorLocation());
                        if (!AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                                bHardEnemy, true, StartDistance))
                        {
                            continue;
                        }
                        const FString SpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                        if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(SpawnId)) continue;
                        if (SpawnedWildlifeActorCount >= WildlifeActorBudget) continue;

                        ++SpawnedWildlifeActorCount;
                        if (Archetype != EKalmalaWildlifeArchetype::HornedGoat) continue;
                        if (!IsDryMountainSurface(FVector2D(Spawn.Location))) continue;

                        const FVector2D SpawnPosition(Spawn.Location);
                        for (int32 DirectionIndex = 0; DirectionIndex < 16; ++DirectionIndex)
                        {
                            const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                            const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
                            const FVector2D ContactPosition = SpawnPosition + Direction * 120.0f;
                            const FVector2D PushEndPosition = SpawnPosition + Direction * 400.0f;
                            const FVector2D RingPosition = SpawnPosition + Direction * 700.0f;
                            if (!IsSafeMountainPath(SpawnPosition, PushEndPosition)
                                || !IsSafeMountainPath(SpawnPosition, RingPosition))
                            {
                                continue;
                            }

                            HornedGoatPeerTestSpatialKey = CandidateKey;
                            HornedGoatPeerTestSpawnId = SpawnId;
                            HornedGoatPeerTestSpawnLocation = Spawn.Location;
                            HornedGoatPeerTestHostLocation = FVector(ContactPosition.X, ContactPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ContactPosition));
                            HornedGoatPeerTestHostRingLocation = FVector(RingPosition.X, RingPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, RingPosition));
                            bFoundCandidate = true;
                            break;
                        }
                        if (bFoundCandidate) break;
                    }
                }
            }
        }

        if (!bFoundCandidate)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Horned Goat verification FAILED: no undefeated generated Thunder Mountains Goat with a safe attack and push path was found within 64 spatial keys."));
            HornedGoatPeerTestStage = 99;
            return;
        }
        if (!ActivePopulationSpatialKeys.Contains(HornedGoatPeerTestSpatialKey)
            && ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
        {
            UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: the bounded active population-key budget is full before fixture activation."));
            HornedGoatPeerTestStage = 99;
            return;
        }

        Host->Health = 100.0f;
        Host->ForceNetUpdate();
        HornedGoatPeerTestWorldSeed = WorldGenerationConfig.WorldSeed;
        HornedGoatPeerTestHost = Host;
        HornedGoatPeerTestTarget.Reset();
        bHornedGoatPeerTestSawTelegraph = false;
        bHornedGoatPeerTestSawRecovery = false;
        bHornedGoatPeerTestSawStagger = false;
        bHornedGoatPeerTestSawStaggerEnd = false;
        HornedGoatPeerTestObservedStaggerDuration = 0.0f;
        PlaceOnTerrain(Host, HornedGoatPeerTestHostRingLocation);
        ActivatePopulationKey(HornedGoatPeerTestSpatialKey);
        HornedGoatPeerTestStage = 1;
        HornedGoatPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Host = HornedGoatPeerTestHost.Get();
    if (!IsValid(Host))
    {
        UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: listen host disappeared."));
        HornedGoatPeerTestStage = 99;
        return;
    }

    if (HornedGoatPeerTestStage == 1)
    {
        AKalmalaWildlifeSpawn* Target = nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if ((*It)->GetPersistentSpawnId() == HornedGoatPeerTestSpawnId)
            {
                Target = *It;
                break;
            }
        }
        if (Target == nullptr)
        {
            if (Now - HornedGoatPeerTestStageTime > 20.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: generated descriptor did not materialize."));
                HornedGoatPeerTestStage = 99;
            }
            return;
        }
        if (Target->GetArchetype() != EKalmalaWildlifeArchetype::HornedGoat
            || !FMath::IsNearlyEqual(Target->GetHealth(), 200.0f, 0.1f)
            || !FMath::IsNearlyEqual(AKalmalaWildlifeSpawn::GetArmorForArchetype(
                Target->GetArchetype()), 50.0f, 0.1f))
        {
            UE_LOG(LogTemp, Error,
                TEXT("Horned Goat verification FAILED: generated archetype, 200-health profile, or 50-armor profile was invalid."));
            HornedGoatPeerTestStage = 99;
            return;
        }

        PlaceOnTerrain(Host, HornedGoatPeerTestHostLocation);
        HornedGoatPeerTestHostStartingLocation = Host->GetActorLocation();
        HornedGoatPeerTestStartingHealth = Host->GetHealth();
        int32 ObserverIndex = 0;
        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : HornedGoatPeerTestObservers)
        {
            AKalmalaCharacter* Observer = ObserverWeak.Get();
            if (!IsValid(Observer)) continue;
            const float Distance = 1000.0f + 500.0f * static_cast<float>(ObserverIndex);
            bool bPlacedObserver = false;
            for (int32 DirectionIndex = 0; DirectionIndex < 16 && !bPlacedObserver; ++DirectionIndex)
            {
                const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                const FVector2D ObserverPosition(
                    HornedGoatPeerTestSpawnLocation.X + FMath::Cos(Angle) * Distance,
                    HornedGoatPeerTestSpawnLocation.Y + FMath::Sin(Angle) * Distance);
                if (!IsDryMountainSurface(ObserverPosition)) continue;
                const FVector DesiredObserverLocation(ObserverPosition.X, ObserverPosition.Y,
                    FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ObserverPosition));
                PlaceOnTerrain(Observer, DesiredObserverLocation);
                bPlacedObserver = true;
            }
            if (!bPlacedObserver)
            {
                UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: an observer could not be placed within replication range."));
                HornedGoatPeerTestStage = 99;
                return;
            }
            ++ObserverIndex;
        }
        if (ObserverIndex < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: an observing client disappeared before the attack."));
            HornedGoatPeerTestStage = 99;
            return;
        }

        HornedGoatPeerTestTarget = Target;
        Target->ForceNetUpdate();
        UE_LOG(LogTemp, Display,
            TEXT("Horned Goat verification fixture armed: Seed=%llu SpawnId=%s HostDistanceCm=%.1f Observers=2."),
            static_cast<unsigned long long>(HornedGoatPeerTestWorldSeed), *HornedGoatPeerTestSpawnId,
            FVector::Dist(Host->GetActorLocation(), Target->GetActorLocation()));
        HornedGoatPeerTestStage = 2;
        HornedGoatPeerTestStageTime = Now;
        return;
    }

    if (HornedGoatPeerTestStage == 2)
    {
        AKalmalaWildlifeSpawn* Target = HornedGoatPeerTestTarget.Get();
        if (!IsValid(Target))
        {
            UE_LOG(LogTemp, Error, TEXT("Horned Goat verification FAILED: generated Goat disappeared before its charge."));
            HornedGoatPeerTestStage = 99;
            return;
        }

        const EKalmalaWildlifeAttackCue Cue = Target->GetHornedGoatAttackCueForVerification();
        bHornedGoatPeerTestSawTelegraph |= Cue == EKalmalaWildlifeAttackCue::Telegraph;
        bHornedGoatPeerTestSawRecovery |= Cue == EKalmalaWildlifeAttackCue::Recovery;
        if (Host->IsWildlifeStaggered())
        {
            if (!bHornedGoatPeerTestSawStagger)
            {
                HornedGoatPeerTestStaggerStartTime = Now;
                bHornedGoatPeerTestSawStagger = true;
            }
        }
        else if (bHornedGoatPeerTestSawStagger && !bHornedGoatPeerTestSawStaggerEnd)
        {
            HornedGoatPeerTestObservedStaggerDuration = Now - HornedGoatPeerTestStaggerStartTime;
            bHornedGoatPeerTestSawStaggerEnd = true;
        }

        const float HostDamage = HornedGoatPeerTestStartingHealth - Host->GetHealth();
        const float HostPush = FVector::Dist2D(HornedGoatPeerTestHostStartingLocation, Host->GetActorLocation());
        const bool bStaggerDurationValid = HornedGoatPeerTestObservedStaggerDuration >= 0.65f
            && HornedGoatPeerTestObservedStaggerDuration <= 0.95f;
        if (bHornedGoatPeerTestSawTelegraph && bHornedGoatPeerTestSawRecovery
            && bHornedGoatPeerTestSawStagger && bHornedGoatPeerTestSawStaggerEnd
            && HostDamage > 0.0f && HostPush >= 50.0f && bStaggerDurationValid)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Horned Goat verification server: Passed=1 Seed=%llu Telegraph=1 Recovery=1 HostDamage=%.1f HostPushCm=%.1f StaggerSeconds=%.2f SpawnId=%s"),
                static_cast<unsigned long long>(HornedGoatPeerTestWorldSeed), HostDamage, HostPush,
                HornedGoatPeerTestObservedStaggerDuration, *HornedGoatPeerTestSpawnId);
            HornedGoatPeerTestStage = 99;
            return;
        }
        if (Now - HornedGoatPeerTestStageTime > 18.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Horned Goat verification FAILED: cue or authoritative horn effects incomplete. Telegraph=%d Recovery=%d Stagger=%d StaggerEnd=%d StaggerSeconds=%.2f HostDamage=%.1f HostPushCm=%.1f Cue=%d."),
                bHornedGoatPeerTestSawTelegraph, bHornedGoatPeerTestSawRecovery,
                bHornedGoatPeerTestSawStagger, bHornedGoatPeerTestSawStaggerEnd,
                HornedGoatPeerTestObservedStaggerDuration, HostDamage, HostPush, static_cast<int32>(Cue));
            HornedGoatPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DriveThunderSentinelPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaThunderSentinelPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || ThunderSentinelPeerTestStage >= 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const auto PlaceOnTerrain = [this](AKalmalaCharacter* Character, const FVector& DesiredLocation)
    {
        if (!IsValid(Character)) return;
        ActivateTerrainPatchNeighborhood(FVector2D(DesiredLocation));
        FVector GroundedLocation = DesiredLocation;
        GroundedLocation.Z = FKalmalaTerrainHeightSampler::SampleHeight(
            WorldGenerationConfig, FVector2D(DesiredLocation))
            + Character->GetSimpleCollisionHalfHeight() + 1.0f;
        Character->SetActorLocation(GroundedLocation, false, nullptr, ETeleportType::TeleportPhysics);
        Character->ForceNetUpdate();
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
            if (Movement->MovementMode == MOVE_None) Movement->SetMovementMode(MOVE_Walking);
        }
    };
    const auto IsDryMountainSurface = [this](const FVector2D& Position)
    {
        if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, Position)) return false;
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position));
        const bool bWater = FKalmalaOceanSampler::Sample(WorldGenerationConfig, Position).IsWater()
            || FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position);
        const bool bTraversable = FKalmalaTerrainHeightSampler::SampleSurfaceNormal(
            WorldGenerationConfig, Position).Z >= 0.82f;
        return AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(Biome, bWater, bTraversable);
    };
    const auto IsSafeMountainPath = [&IsDryMountainSurface, this](const FVector2D& Start, const FVector2D& End)
    {
        if (!IsDryMountainSurface(Start) || !IsDryMountainSurface(End)) return false;
        const float StartHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Start);
        const float Distance = FVector2D::Distance(Start, End);
        const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
        for (int32 Index = 0; Index <= Samples; ++Index)
        {
            const FVector2D Position = FMath::Lerp(Start, End, static_cast<float>(Index) / Samples);
            if (!IsDryMountainSurface(Position)
                || FMath::Abs(FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, Position)
                    - StartHeight) > 80.0f)
            {
                return false;
            }
        }
        return true;
    };

    if (ThunderSentinelPeerTestStage == 0)
    {
        APlayerController* HostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Host = HostController != nullptr
            ? Cast<AKalmalaCharacter>(HostController->GetPawn()) : nullptr;
        if (Host == nullptr || GeneratedPlayerStart == nullptr)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: generated start or listen host was unavailable."));
                ThunderSentinelPeerTestStage = 99;
            }
            return;
        }

        ThunderSentinelPeerTestObservers.Reset();
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == Host || Candidate->GetPlayerState() == nullptr) continue;
            ThunderSentinelPeerTestObservers.Add(Candidate);
        }
        if (ThunderSentinelPeerTestObservers.Num() < 2)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: two observing client peers did not join."));
                ThunderSentinelPeerTestStage = 99;
            }
            return;
        }

        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(
            FVector2D(GeneratedPlayerStart->GetActorLocation()));
        bool bFoundCandidate = false;
        for (int32 Radius = 0; Radius <= 64 && !bFoundCandidate; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundCandidate; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    const FVector2D KeyCentre = (FVector2D(CandidateKey) + FVector2D(0.5f, 0.5f))
                        * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
                    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                        FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, KeyCentre));
                    if (KeyBiome != EKalmalaBiome::ThunderMountains) continue;

                    TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    Descriptors.SetNum(FMath::Min(Descriptors.Num(),
                        FKalmalaBiomeExpansionContract::ApplyPopulationBudget(
                            Descriptors.Num(), EKalmalaWorldPopulationKind::Wildlife, KeyBiome)));
                    const int32 WildlifeActorBudget = Descriptors.Num();
                    int32 SpawnedWildlifeActorCount = 0;

                    for (const FKalmalaWorldPopulationSpawn& Spawn : Descriptors)
                    {
                        EKalmalaWildlifeArchetype Archetype;
                        if (!KalmalaGameMode::TryGetWildlifeArchetypeForSpawn(
                                WorldGenerationConfig, Spawn, Archetype))
                        {
                            continue;
                        }
                        const bool bHardEnemy = Archetype == EKalmalaWildlifeArchetype::GiantSnake
                            || Archetype == EKalmalaWildlifeArchetype::MoonBear
                            || Archetype == EKalmalaWildlifeArchetype::PeatHusk
                            || Archetype == EKalmalaWildlifeArchetype::PolarBear
                            || Archetype == EKalmalaWildlifeArchetype::ThunderSentinel
                            || Archetype == EKalmalaWildlifeArchetype::ReefMaw;
                        const float StartDistance = FVector::Dist2D(
                            Spawn.Location, GeneratedPlayerStart->GetActorLocation());
                        if (!AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                                bHardEnemy, true, StartDistance))
                        {
                            continue;
                        }
                        const FString SpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                        if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(SpawnId)) continue;
                        if (SpawnedWildlifeActorCount >= WildlifeActorBudget) continue;

                        ++SpawnedWildlifeActorCount;
                        if (Archetype != EKalmalaWildlifeArchetype::ThunderSentinel) continue;
                        if (!IsDryMountainSurface(FVector2D(Spawn.Location))) continue;

                        const FVector2D SpawnPosition(Spawn.Location);
                        for (int32 DirectionIndex = 0; DirectionIndex < 16; ++DirectionIndex)
                        {
                            const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                            const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
                            const FVector2D HostPosition = SpawnPosition + Direction * 400.0f;
                            const FVector2D RingPosition = SpawnPosition + Direction * 700.0f;
                            if (!IsSafeMountainPath(SpawnPosition, HostPosition)
                                || !IsSafeMountainPath(SpawnPosition, RingPosition))
                            {
                                continue;
                            }

                            ThunderSentinelPeerTestSpatialKey = CandidateKey;
                            ThunderSentinelPeerTestSpawnId = SpawnId;
                            ThunderSentinelPeerTestSpawnLocation = Spawn.Location;
                            ThunderSentinelPeerTestStartDistanceCm = StartDistance;
                            ThunderSentinelPeerTestHostLocation = FVector(HostPosition.X, HostPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, HostPosition));
                            ThunderSentinelPeerTestHostRingLocation = FVector(RingPosition.X, RingPosition.Y,
                                FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, RingPosition));
                            bFoundCandidate = true;
                            break;
                        }
                        if (bFoundCandidate) break;
                    }
                }
            }
        }

        if (!bFoundCandidate)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Thunder Sentinel verification FAILED: no undefeated generated Sentinel with a safe slam path and 150 m start exclusion was found within 64 spatial keys."));
            ThunderSentinelPeerTestStage = 99;
            return;
        }
        if (!ActivePopulationSpatialKeys.Contains(ThunderSentinelPeerTestSpatialKey)
            && ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
        {
            UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: the bounded active population-key budget is full before fixture activation."));
            ThunderSentinelPeerTestStage = 99;
            return;
        }

        ThunderSentinelPeerTestWorldSeed = WorldGenerationConfig.WorldSeed;
        ThunderSentinelPeerTestHost = Host;
        ThunderSentinelPeerTestTarget.Reset();
        ThunderSentinelPeerTestStartingHealth = 400.0f;
        Host->Health = ThunderSentinelPeerTestStartingHealth;
        Host->ForceNetUpdate();
        bThunderSentinelPeerTestSawTelegraph = false;
        bThunderSentinelPeerTestSawSlam = false;
        bThunderSentinelPeerTestSawRecovery = false;
        ThunderSentinelPeerTestObservedStepCm = 0.0f;
        PlaceOnTerrain(Host, ThunderSentinelPeerTestHostRingLocation);
        ActivatePopulationKey(ThunderSentinelPeerTestSpatialKey);
        ThunderSentinelPeerTestStage = 1;
        ThunderSentinelPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Host = ThunderSentinelPeerTestHost.Get();
    if (!IsValid(Host))
    {
        UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: listen host disappeared."));
        ThunderSentinelPeerTestStage = 99;
        return;
    }

    if (ThunderSentinelPeerTestStage == 1)
    {
        AKalmalaWildlifeSpawn* Target = nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if ((*It)->GetPersistentSpawnId() == ThunderSentinelPeerTestSpawnId)
            {
                Target = *It;
                break;
            }
        }
        if (Target == nullptr)
        {
            if (Now - ThunderSentinelPeerTestStageTime > 20.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: generated descriptor did not materialize."));
                ThunderSentinelPeerTestStage = 99;
            }
            return;
        }
        if (Target->GetArchetype() != EKalmalaWildlifeArchetype::ThunderSentinel
            || !FMath::IsNearlyEqual(Target->GetHealth(), 1250.0f, 0.1f)
            || !FMath::IsNearlyEqual(AKalmalaWildlifeSpawn::GetArmorForArchetype(
                Target->GetArchetype()), 75.0f, 0.1f)
            || ThunderSentinelPeerTestStartDistanceCm < 15000.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Thunder Sentinel verification FAILED: generated archetype, 1250-health profile, 75-armor profile, or 150 m start exclusion was invalid."));
            ThunderSentinelPeerTestStage = 99;
            return;
        }

        ThunderSentinelPeerTestSentinelStartLocation = Target->GetActorLocation();
        PlaceOnTerrain(Host, ThunderSentinelPeerTestHostLocation);
        ThunderSentinelPeerTestHostDistanceCm = FVector::Dist2D(Host->GetActorLocation(), Target->GetActorLocation());
        if (ThunderSentinelPeerTestHostDistanceCm <= AKalmalaWildlifeSpawn::ThunderSentinelSlamReachCentimetres
            || ThunderSentinelPeerTestHostDistanceCm > AKalmalaWildlifeSpawn::ThunderSentinelAttackTriggerCentimetres)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Thunder Sentinel verification FAILED: host did not start outside slam reach and inside the attack trigger. HostDistanceCm=%.1f."),
                ThunderSentinelPeerTestHostDistanceCm);
            ThunderSentinelPeerTestStage = 99;
            return;
        }

        ThunderSentinelPeerTestStartingHealth = Host->GetHealth();
        int32 ObserverIndex = 0;
        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : ThunderSentinelPeerTestObservers)
        {
            AKalmalaCharacter* Observer = ObserverWeak.Get();
            if (!IsValid(Observer)) continue;
            const float Distance = 1000.0f + 500.0f * static_cast<float>(ObserverIndex);
            bool bPlacedObserver = false;
            for (int32 DirectionIndex = 0; DirectionIndex < 16 && !bPlacedObserver; ++DirectionIndex)
            {
                const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                const FVector2D ObserverPosition(
                    ThunderSentinelPeerTestSpawnLocation.X + FMath::Cos(Angle) * Distance,
                    ThunderSentinelPeerTestSpawnLocation.Y + FMath::Sin(Angle) * Distance);
                if (!IsDryMountainSurface(ObserverPosition)) continue;
                const FVector DesiredObserverLocation(ObserverPosition.X, ObserverPosition.Y,
                    FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, ObserverPosition));
                PlaceOnTerrain(Observer, DesiredObserverLocation);
                bPlacedObserver = true;
            }
            if (!bPlacedObserver)
            {
                UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: an observer could not be placed within replication range."));
                ThunderSentinelPeerTestStage = 99;
                return;
            }
            ++ObserverIndex;
        }
        if (ObserverIndex < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: an observing client disappeared before the slam."));
            ThunderSentinelPeerTestStage = 99;
            return;
        }

        ThunderSentinelPeerTestTarget = Target;
        Target->ForceNetUpdate();
        UE_LOG(LogTemp, Display,
            TEXT("Thunder Sentinel verification fixture armed: Seed=%llu SpawnId=%s StartDistanceCm=%.1f HostDistanceCm=%.1f Observers=2."),
            static_cast<unsigned long long>(ThunderSentinelPeerTestWorldSeed), *ThunderSentinelPeerTestSpawnId,
            ThunderSentinelPeerTestStartDistanceCm, ThunderSentinelPeerTestHostDistanceCm);
        ThunderSentinelPeerTestStage = 2;
        ThunderSentinelPeerTestStageTime = Now;
        return;
    }

    if (ThunderSentinelPeerTestStage == 2)
    {
        AKalmalaWildlifeSpawn* Target = ThunderSentinelPeerTestTarget.Get();
        if (!IsValid(Target))
        {
            UE_LOG(LogTemp, Error, TEXT("Thunder Sentinel verification FAILED: generated Sentinel disappeared before its slam."));
            ThunderSentinelPeerTestStage = 99;
            return;
        }

        const EKalmalaWildlifeAttackCue Cue = Target->GetThunderSentinelAttackCueForVerification();
        bThunderSentinelPeerTestSawTelegraph |= Cue == EKalmalaWildlifeAttackCue::Telegraph;
        bThunderSentinelPeerTestSawSlam |= Cue == EKalmalaWildlifeAttackCue::Lunge;
        bThunderSentinelPeerTestSawRecovery |= Cue == EKalmalaWildlifeAttackCue::Recovery;
        if (Cue == EKalmalaWildlifeAttackCue::Lunge && ThunderSentinelPeerTestObservedStepCm <= 0.0f)
        {
            ThunderSentinelPeerTestObservedStepCm = FVector::Dist2D(
                ThunderSentinelPeerTestSentinelStartLocation, Target->GetActorLocation());
        }

        const float HostDamage = ThunderSentinelPeerTestStartingHealth - Host->GetHealth();
        const bool bDamageValid = FMath::IsNearlyEqual(HostDamage, 300.0f, 1.0f);
        const bool bStepValid = ThunderSentinelPeerTestObservedStepCm >= 80.0f
            && ThunderSentinelPeerTestObservedStepCm <= 120.5f;
        if (bThunderSentinelPeerTestSawTelegraph && bThunderSentinelPeerTestSawSlam
            && bThunderSentinelPeerTestSawRecovery && bDamageValid && bStepValid
            && Host->GetHealth() > 1.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Thunder Sentinel verification server: Passed=1 Seed=%llu Telegraph=1 Slam=1 Recovery=1 HostDamage=%.1f StepCm=%.1f SpawnId=%s"),
                static_cast<unsigned long long>(ThunderSentinelPeerTestWorldSeed), HostDamage,
                ThunderSentinelPeerTestObservedStepCm, *ThunderSentinelPeerTestSpawnId);
            ThunderSentinelPeerTestStage = 99;
            return;
        }
        if (Now - ThunderSentinelPeerTestStageTime > 18.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Thunder Sentinel verification FAILED: cue, 300-Crush slam, or short step incomplete. Telegraph=%d Slam=%d Recovery=%d HostDamage=%.1f StepCm=%.1f Cue=%d."),
                bThunderSentinelPeerTestSawTelegraph, bThunderSentinelPeerTestSawSlam,
                bThunderSentinelPeerTestSawRecovery, HostDamage, ThunderSentinelPeerTestObservedStepCm,
                static_cast<int32>(Cue));
            ThunderSentinelPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DriveReefMawPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaReefMawPeerTest"))
        || !HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid()
        || ReefMawPeerTestStage >= 99)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    const auto IsNavigableOceanPosition = [this](const FVector2D& Position)
    {
        if (!FKalmalaWorldBounds::Contains(WorldGenerationConfig, Position)) return false;
        const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(WorldGenerationConfig, Position);
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, Position));
        return Ocean.IsWater() && AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(
            Biome, FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, Position), Ocean.WaterDepth);
    };
    const auto IsNavigableOceanPath = [&IsNavigableOceanPosition](const FVector2D& Start, const FVector2D& End)
    {
        if (!IsNavigableOceanPosition(Start) || !IsNavigableOceanPosition(End)) return false;
        const float Distance = FVector2D::Distance(Start, End);
        const int32 Samples = FMath::Max(1, FMath::CeilToInt(Distance / 80.0f));
        for (int32 Index = 0; Index <= Samples; ++Index)
        {
            if (!IsNavigableOceanPosition(FMath::Lerp(Start, End, static_cast<float>(Index) / Samples))) return false;
        }
        return true;
    };
    const auto PlaceSwimmingCharacter = [this](AKalmalaCharacter* Character, const FVector2D& Position)
    {
        if (!IsValid(Character)) return false;
        ActivateTerrainPatchNeighborhood(Position);
        Character->SetActorLocation(FVector(Position.X, Position.Y,
            Character->GetSimpleCollisionHalfHeight() + 8.0f), false, nullptr, ETeleportType::TeleportPhysics);
        Character->ForceNetUpdate();
        UKalmalaCharacterMovementComponent* Movement =
            Cast<UKalmalaCharacterMovementComponent>(Character->GetCharacterMovement());
        if (Movement == nullptr) return false;
        Movement->StopMovementImmediately();
        Movement->SetMovementMode(MOVE_Custom, UKalmalaCharacterMovementComponent::GeneratedOceanSwimmingMode);
        return Movement->IsSwimmingInGeneratedOcean();
    };

    if (ReefMawPeerTestStage == 0)
    {
        APlayerController* HostController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Host = HostController != nullptr
            ? Cast<AKalmalaCharacter>(HostController->GetPawn()) : nullptr;
        if (Host == nullptr || GeneratedPlayerStart == nullptr)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: generated start or listen host was unavailable."));
                ReefMawPeerTestStage = 99;
            }
            return;
        }

        ReefMawPeerTestObservers.Reset();
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            AKalmalaCharacter* Candidate = *It;
            if (Candidate == Host || Candidate->GetPlayerState() == nullptr) continue;
            ReefMawPeerTestObservers.Add(Candidate);
        }
        if (ReefMawPeerTestObservers.Num() < 2)
        {
            if (Now > 90.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: two observing client peers did not join."));
                ReefMawPeerTestStage = 99;
            }
            return;
        }

        const FIntPoint StartKey = FKalmalaWorldPopulationLayout::GetSpatialKey(
            FVector2D(GeneratedPlayerStart->GetActorLocation()));
        bool bFoundCandidate = false;
        for (int32 Radius = 0; Radius <= 64 && !bFoundCandidate; ++Radius)
        {
            for (int32 OffsetY = -Radius; OffsetY <= Radius && !bFoundCandidate; ++OffsetY)
            {
                for (int32 OffsetX = -Radius; OffsetX <= Radius; ++OffsetX)
                {
                    if (Radius > 0 && FMath::Abs(OffsetX) != Radius && FMath::Abs(OffsetY) != Radius) continue;
                    const FIntPoint CandidateKey = StartKey + FIntPoint(OffsetX, OffsetY);
                    const FVector2D KeyCentre = (FVector2D(CandidateKey) + FVector2D(0.5f, 0.5f))
                        * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
                    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                        FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, KeyCentre));
                    if (KeyBiome != EKalmalaBiome::Ocean) continue;

                    TArray<FKalmalaWorldPopulationSpawn> Descriptors =
                        FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
                            WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife);
                    // Normal Ocean activation uses the full descriptor list; the land-biome
                    // expansion budget intentionally has a zero Ocean multiplier.
                    const int32 WildlifeActorBudget = Descriptors.Num();
                    int32 SpawnedWildlifeActorCount = 0;
                    for (const FKalmalaWorldPopulationSpawn& Spawn : Descriptors)
                    {
                        EKalmalaWildlifeArchetype Archetype;
                        if (!KalmalaGameMode::TryGetWildlifeArchetypeForSpawn(
                                WorldGenerationConfig, Spawn, Archetype)) continue;
                        const bool bHardEnemy = Archetype == EKalmalaWildlifeArchetype::GiantSnake
                            || Archetype == EKalmalaWildlifeArchetype::MoonBear
                            || Archetype == EKalmalaWildlifeArchetype::PeatHusk
                            || Archetype == EKalmalaWildlifeArchetype::PolarBear
                            || Archetype == EKalmalaWildlifeArchetype::ThunderSentinel
                            || Archetype == EKalmalaWildlifeArchetype::ReefMaw;
                        const float StartDistance = FVector::Dist2D(
                            Spawn.Location, GeneratedPlayerStart->GetActorLocation());
                        if (!AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                                bHardEnemy, true, StartDistance)) continue;
                        const FString SpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                        if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(SpawnId)) continue;
                        if (SpawnedWildlifeActorCount >= WildlifeActorBudget) continue;
                        ++SpawnedWildlifeActorCount;
                        if (Archetype != EKalmalaWildlifeArchetype::ReefMaw) continue;

                        const FVector2D SpawnPosition(Spawn.Location);
                        const FKalmalaOceanSample SpawnOcean = FKalmalaOceanSampler::Sample(
                            WorldGenerationConfig, SpawnPosition);
                        for (int32 DirectionIndex = 0; DirectionIndex < 16; ++DirectionIndex)
                        {
                            const float Angle = static_cast<float>(DirectionIndex) * (2.0f * PI / 16.0f);
                            const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
                            const FVector2D HostPosition = SpawnPosition + Direction * 400.0f;
                            TArray<FVector> OceanLocations;
                            bool bSafeOceanPath = IsNavigableOceanPath(SpawnPosition, HostPosition);
                            OceanLocations.Add(FVector(HostPosition.X, HostPosition.Y, 0.0f));
                            for (int32 ObserverIndex = 0; ObserverIndex < 2 && bSafeOceanPath; ++ObserverIndex)
                            {
                                const float ObserverDistance = 1000.0f + 500.0f * static_cast<float>(ObserverIndex);
                                const FVector2D ObserverPosition = SpawnPosition + Direction * ObserverDistance;
                                bSafeOceanPath = IsNavigableOceanPath(SpawnPosition, ObserverPosition);
                                OceanLocations.Add(FVector(ObserverPosition.X, ObserverPosition.Y, 0.0f));
                            }
                            if (!bSafeOceanPath) continue;

                            ReefMawPeerTestSpatialKey = CandidateKey;
                            ReefMawPeerTestSpawnId = SpawnId;
                            ReefMawPeerTestSpawnLocation = Spawn.Location;
                            ReefMawPeerTestStartDistanceCm = StartDistance;
                            ReefMawPeerTestSpawnDepthCm = SpawnOcean.WaterDepth;
                            ReefMawPeerTestOceanLocations = MoveTemp(OceanLocations);
                            bFoundCandidate = true;
                            break;
                        }
                        if (bFoundCandidate) break;
                    }
                }
            }
        }

        if (!bFoundCandidate)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Reef Maw verification FAILED: no undefeated generated Maw with three safe deep-water peer positions and the 150 m start exclusion was found within 64 spatial keys."));
            ReefMawPeerTestStage = 99;
            return;
        }
        if (!ActivePopulationSpatialKeys.Contains(ReefMawPeerTestSpatialKey)
            && ActivePopulationSpatialKeys.Num() >= KalmalaGameMode::MaxActivePopulationSpatialKeys)
        {
            UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: the bounded active population-key budget is full before fixture activation."));
            ReefMawPeerTestStage = 99;
            return;
        }

        ReefMawPeerTestWorldSeed = WorldGenerationConfig.WorldSeed;
        ReefMawPeerTestHost = Host;
        ReefMawPeerTestTarget.Reset();
        ActivatePopulationKey(ReefMawPeerTestSpatialKey);
        ReefMawPeerTestStage = 1;
        ReefMawPeerTestStageTime = Now;
        return;
    }

    AKalmalaCharacter* Host = ReefMawPeerTestHost.Get();
    if (!IsValid(Host))
    {
        UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: listen host disappeared."));
        ReefMawPeerTestStage = 99;
        return;
    }

    if (ReefMawPeerTestStage == 1)
    {
        AKalmalaWildlifeSpawn* Target = nullptr;
        for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
        {
            if ((*It)->GetPersistentSpawnId() == ReefMawPeerTestSpawnId)
            {
                Target = *It;
                break;
            }
        }
        if (Target == nullptr)
        {
            if (Now - ReefMawPeerTestStageTime > 20.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: generated descriptor did not materialize."));
                ReefMawPeerTestStage = 99;
            }
            return;
        }

        const FVector2D SpawnPosition(ReefMawPeerTestSpawnLocation);
        const FKalmalaOceanSample SpawnOcean = FKalmalaOceanSampler::Sample(WorldGenerationConfig, SpawnPosition);
        if (Target->GetArchetype() != EKalmalaWildlifeArchetype::ReefMaw
            || !FMath::IsNearlyEqual(Target->GetHealth(), 180.0f, 0.1f)
            || !FMath::IsNearlyEqual(AKalmalaWildlifeSpawn::GetArmorForArchetype(
                Target->GetArchetype()), 2.0f, 0.1f)
            || ReefMawPeerTestStartDistanceCm < 15000.0f
            || !AKalmalaWildlifeSpawn::IsReefMawSpawnHabitatAllowed(
                EKalmalaBiome::Ocean, FKalmalaShimmeringLakeSampler::IsWater(WorldGenerationConfig, SpawnPosition),
                SpawnOcean.WaterDepth)
            || ReefMawPeerTestOceanLocations.Num() != 3)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Reef Maw verification FAILED: generated archetype, 180/2 profile, deep Ocean habitat, 150 m start exclusion, or peer positions were invalid."));
            ReefMawPeerTestStage = 99;
            return;
        }

        int32 ObserverIndex = 0;
        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : ReefMawPeerTestObservers)
        {
            AKalmalaCharacter* Observer = ObserverWeak.Get();
            if (!IsValid(Observer) || ObserverIndex >= 2
                || !PlaceSwimmingCharacter(Observer,
                    FVector2D(ReefMawPeerTestOceanLocations[ObserverIndex + 1])))
            {
                UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: an observer could not enter deep generated Ocean."));
                ReefMawPeerTestStage = 99;
                return;
            }
            ++ObserverIndex;
        }
        if (ObserverIndex != 2 || !PlaceSwimmingCharacter(
                Host, FVector2D(ReefMawPeerTestOceanLocations[0])))
        {
            UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: the host could not enter deep generated Ocean."));
            ReefMawPeerTestStage = 99;
            return;
        }

        Host->Health = 150.0f;
        Host->ForceNetUpdate();
        ReefMawPeerTestStartingHealth = Host->GetHealth();
        ReefMawPeerTestHostDistanceCm = FVector::Dist(Target->GetActorLocation(), Host->GetActorLocation());
        if (ReefMawPeerTestHostDistanceCm <= AKalmalaWildlifeSpawn::ReefMawBiteReachCentimetres
            || ReefMawPeerTestHostDistanceCm > AKalmalaWildlifeSpawn::ReefMawAttackTriggerCentimetres)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Reef Maw verification FAILED: host did not start outside bite reach and inside the attack trigger. HostDistanceCm=%.1f."),
                ReefMawPeerTestHostDistanceCm);
            ReefMawPeerTestStage = 99;
            return;
        }

        for (const TWeakObjectPtr<AKalmalaCharacter>& ObserverWeak : ReefMawPeerTestObservers)
        {
            const AKalmalaCharacter* Observer = ObserverWeak.Get();
            const UKalmalaCharacterMovementComponent* Movement = IsValid(Observer)
                ? Cast<UKalmalaCharacterMovementComponent>(Observer->GetCharacterMovement()) : nullptr;
            if (!IsValid(Observer) || Observer->GetAttachParentActor() != nullptr
                || Movement == nullptr || !Movement->IsSwimmingInGeneratedOcean())
            {
                UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: a peer observer was not swimming freely in generated Ocean."));
                ReefMawPeerTestStage = 99;
                return;
            }
        }
        const UKalmalaCharacterMovementComponent* HostMovement =
            Cast<UKalmalaCharacterMovementComponent>(Host->GetCharacterMovement());
        if (Host->GetAttachParentActor() != nullptr || HostMovement == nullptr
            || !HostMovement->IsSwimmingInGeneratedOcean())
        {
            UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: the host was not swimming freely in generated Ocean."));
            ReefMawPeerTestStage = 99;
            return;
        }

        ReefMawPeerTestTarget = Target;
        bReefMawPeerTestSawTelegraph = false;
        bReefMawPeerTestSawBite = false;
        bReefMawPeerTestSawRecovery = false;
        ReefMawPeerTestBiteDistanceCm = 0.0f;
        Target->ForceNetUpdate();
        UE_LOG(LogTemp, Display,
            TEXT("Reef Maw verification fixture armed: Seed=%llu SpawnId=%s StartDistanceCm=%.1f SpawnDepthCm=%.1f HostDistanceCm=%.1f Observers=2."),
            static_cast<unsigned long long>(ReefMawPeerTestWorldSeed), *ReefMawPeerTestSpawnId,
            ReefMawPeerTestStartDistanceCm, ReefMawPeerTestSpawnDepthCm, ReefMawPeerTestHostDistanceCm);
        ReefMawPeerTestStage = 2;
        ReefMawPeerTestStageTime = Now;
        return;
    }

    if (ReefMawPeerTestStage == 2)
    {
        AKalmalaWildlifeSpawn* Target = ReefMawPeerTestTarget.Get();
        if (!IsValid(Target))
        {
            UE_LOG(LogTemp, Error, TEXT("Reef Maw verification FAILED: generated Maw disappeared before its bite."));
            ReefMawPeerTestStage = 99;
            return;
        }

        const EKalmalaWildlifeAttackCue Cue = Target->GetReefMawAttackCueForVerification();
        bReefMawPeerTestSawTelegraph |= Cue == EKalmalaWildlifeAttackCue::Telegraph;
        bReefMawPeerTestSawBite |= Cue == EKalmalaWildlifeAttackCue::Lunge;
        bReefMawPeerTestSawRecovery |= Cue == EKalmalaWildlifeAttackCue::Recovery;
        if (Cue == EKalmalaWildlifeAttackCue::Lunge && ReefMawPeerTestBiteDistanceCm <= 0.0f)
        {
            ReefMawPeerTestBiteDistanceCm = FVector::Dist(Target->GetActorLocation(), Host->GetActorLocation());
        }

        const float HostDamage = ReefMawPeerTestStartingHealth - Host->GetHealth();
        const bool bDamageValid = FMath::IsNearlyEqual(HostDamage, 50.0f, 1.0f);
        const bool bBiteRangeValid = ReefMawPeerTestBiteDistanceCm > 0.0f
            && ReefMawPeerTestBiteDistanceCm <= AKalmalaWildlifeSpawn::ReefMawBiteReachCentimetres;
        if (bReefMawPeerTestSawTelegraph && bReefMawPeerTestSawBite
            && bReefMawPeerTestSawRecovery && bDamageValid && bBiteRangeValid
            && Host->GetHealth() > 1.0f)
        {
            UE_LOG(LogTemp, Display,
                TEXT("Reef Maw verification server: Passed=1 Seed=%llu Telegraph=1 Bite=1 Recovery=1 HostDamage=%.1f BiteDistanceCm=%.1f SpawnId=%s"),
                static_cast<unsigned long long>(ReefMawPeerTestWorldSeed), HostDamage,
                ReefMawPeerTestBiteDistanceCm, *ReefMawPeerTestSpawnId);
            ReefMawPeerTestStage = 99;
            return;
        }
        if (Now - ReefMawPeerTestStageTime > 20.0f)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Reef Maw verification FAILED: cue, 50-Stab bite, or swimming approach incomplete. Telegraph=%d Bite=%d Recovery=%d HostDamage=%.1f BiteDistanceCm=%.1f Cue=%d."),
                bReefMawPeerTestSawTelegraph, bReefMawPeerTestSawBite,
                bReefMawPeerTestSawRecovery, HostDamage, ReefMawPeerTestBiteDistanceCm,
                static_cast<int32>(Cue));
            ReefMawPeerTestStage = 99;
        }
    }
#endif
}

void AKalmalaGameMode::DriveDiscoveryPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaDiscoveryPeerTest")) || GetWorld() == nullptr) return;
    if (!bDiscoveryPeerTestLogged)
    {
        bDiscoveryPeerTestLogged = true;
        UE_LOG(LogTemp, Display, TEXT("Discovery verification server fixture enabled."));
    }
    const float Now = GetWorld()->GetTimeSeconds();
    if (DiscoveryPeerTestStage == 0)
    {
        APlayerController* LocalController = GetWorld()->GetFirstPlayerController();
        AKalmalaCharacter* Entitled = LocalController ? Cast<AKalmalaCharacter>(LocalController->GetPawn()) : nullptr;
        AKalmalaCharacter* Remote = nullptr;
        for (TActorIterator<AKalmalaCharacter> It(GetWorld()); It; ++It)
        {
            if (*It != Entitled && (*It)->GetPlayerState() != nullptr) { Remote = *It; break; }
        }
        if (Entitled == nullptr || Remote == nullptr) return;
        const FIntPoint Centre = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(Entitled->GetActorLocation()));
        bool bFound = false;
        for (int32 Y = -3; Y <= 3 && !bFound; ++Y) for (int32 X = -3; X <= 3 && !bFound; ++X)
        {
            const FIntPoint Key = Centre + FIntPoint(X, Y);
            for (const EKalmalaWorldDiscoveryKind Kind : { EKalmalaWorldDiscoveryKind::PointOfInterest, EKalmalaWorldDiscoveryKind::Scroll })
            {
                const TArray<FKalmalaWorldDiscoveryDescriptor> First = FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(WorldGenerationConfig, Key, Kind);
                const TArray<FKalmalaWorldDiscoveryDescriptor> Repeat = FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(WorldGenerationConfig, Key, Kind);
                if (First.IsEmpty()) continue;
                FKalmalaWorldGenerationConfig DifferentWorld = WorldGenerationConfig; ++DifferentWorld.WorldSeed;
                const TArray<FKalmalaWorldDiscoveryDescriptor> Different = FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(DifferentWorld, Key, Kind);
                const FString Id = FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(First[0]);
                const bool bSameSeed = Repeat.ContainsByPredicate([&Id](const FKalmalaWorldDiscoveryDescriptor& Candidate) { return FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Candidate) == Id; });
                const bool bDifferentSeed = !Different.ContainsByPredicate([&Id](const FKalmalaWorldDiscoveryDescriptor& Candidate) { return FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Candidate) == Id; });
                if (!bSameSeed || !bDifferentSeed) continue;
                DiscoveryPeerTestDescriptor = First[0];
                ActivatePopulationKey(Key);
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            UE_LOG(LogTemp, Error, TEXT("Discovery verification FAILED: no same-seed/different-seed descriptor in bounded search."));
            DiscoveryPeerTestStage = 99;
            return;
        }
        DiscoveryPeerTestEntitled = Entitled;
        DiscoveryPeerTestRemote = Remote;
        DiscoveryPeerTestStage = 1;
        return;
    }
    AKalmalaCharacter* Entitled = DiscoveryPeerTestEntitled.Get();
    AKalmalaCharacter* Remote = DiscoveryPeerTestRemote.Get();
    if (Entitled == nullptr || Remote == nullptr) { UE_LOG(LogTemp, Error, TEXT("Discovery verification FAILED: player disappeared.")); DiscoveryPeerTestStage = 99; return; }
    if (DiscoveryPeerTestStage == 1)
    {
        const FString Id = FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(DiscoveryPeerTestDescriptor);
        for (TActorIterator<AKalmalaDiscoveryActor> It(GetWorld()); It; ++It)
        {
            if (FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId((*It)->GetDescriptor()) == Id) { DiscoveryPeerTestActor = *It; break; }
        }
        AKalmalaDiscoveryActor* Actor = DiscoveryPeerTestActor.Get();
        if (Actor == nullptr) return;
        // Keep the reach fixture stationary while streamed terrain settles.
        // Falling between probes must not replace the intended duplicate-claim test.
        for (AKalmalaCharacter* Pawn : {Entitled, Remote})
        {
            Pawn->GetCharacterMovement()->StopMovementImmediately();
            Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        }
        Entitled->SetActorLocation(Actor->GetActorLocation() - FVector(125.0f, 0.0f, 0.0f), false);
        Remote->SetActorLocation(Actor->GetActorLocation() + FVector(1000.0f, 0.0f, 0.0f), false);
        Entitled->ForceNetUpdate(); Remote->ForceNetUpdate();
        DiscoveryPeerTestStage = 2; DiscoveryPeerTestStageTime = Now;
        return;
    }
    AKalmalaDiscoveryActor* Actor = DiscoveryPeerTestActor.Get();
    if (Actor == nullptr) { UE_LOG(LogTemp, Error, TEXT("Discovery verification FAILED: descriptor actor disappeared.")); DiscoveryPeerTestStage = 99; return; }
    UKalmalaDiscoveryProgressComponent* EntitledFeedback = Entitled->GetDiscoveryProgressComponent();
    UKalmalaDiscoveryProgressComponent* RemoteFeedback = Remote->GetDiscoveryProgressComponent();
    if (EntitledFeedback == nullptr || RemoteFeedback == nullptr) { UE_LOG(LogTemp, Error, TEXT("Discovery verification FAILED: progress component missing.")); DiscoveryPeerTestStage = 99; return; }
    if (DiscoveryPeerTestStage == 2 && Now - DiscoveryPeerTestStageTime >= 0.25f)
    {
        const bool bDistantRejected = !Actor->CanInteract_Implementation(Remote) && !ClaimDiscovery(Remote, DiscoveryPeerTestDescriptor)
            && RemoteFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::None && RemoteFeedback->GetFeedbackSerial() == 0;
        Actor->Interact_Implementation(Entitled);
        if (!bDistantRejected || EntitledFeedback->GetFeedbackSerial() != 1 || RemoteFeedback->GetFeedbackSerial() != 0)
        { UE_LOG(LogTemp, Error, TEXT("Discovery verification FAILED: distant rejection or owner claim failed.")); DiscoveryPeerTestStage = 99; return; }
        DiscoveryPeerTestStage = 3; DiscoveryPeerTestStageTime = Now; return;
    }
    if (DiscoveryPeerTestStage == 3 && Now - DiscoveryPeerTestStageTime >= 0.25f)
    {
        UE_LOG(LogTemp, Display, TEXT("Discovery duplicate probe: CanInteract=%d Distance=%.1f Pawn=%s Actor=%s"),
            Actor->CanInteract_Implementation(Entitled), FVector::Distance(Entitled->GetActorLocation(), Actor->GetActorLocation()),
            *Entitled->GetActorLocation().ToCompactString(), *Actor->GetActorLocation().ToCompactString());
        Actor->Interact_Implementation(Entitled);
        const bool bPassed = EntitledFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::AlreadyFound && EntitledFeedback->GetFeedbackSerial() == 2
            && RemoteFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::None && RemoteFeedback->GetFeedbackSerial() == 0;
        if (bPassed)
        {
            UE_LOG(LogTemp, Display, TEXT("Discovery verification server: Passed=1 SeedReproduced=1 DifferentSeed=1 DistantRejected=1 DuplicateRejected=1 OwnerFeedbackSerial=%u RemoteFeedbackSerial=%u"), EntitledFeedback->GetFeedbackSerial(), RemoteFeedback->GetFeedbackSerial());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Discovery verification server: Passed=0 OwnerFeedbackSerial=%u RemoteFeedbackSerial=%u"), EntitledFeedback->GetFeedbackSerial(), RemoteFeedback->GetFeedbackSerial());
        }
        DiscoveryPeerTestStage = 99;
    }
#endif
}

void AKalmalaGameMode::DriveM9ExplorationRewardPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaM9ExplorationRewardPeerTest"))
        || GetWorld() == nullptr || M9ExplorationRewardPeerTestStage >= 99)
    {
        return;
    }

    if (M9ExplorationRewardPeerTestStage == 0)
    {
        AKalmalaCharacter* Host = nullptr;
        AKalmalaCharacter* Remote = nullptr;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* Controller = It->Get();
            AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
            if (Character == nullptr) continue;
            if (Controller->IsLocalController()) Host = Character;
            else Remote = Character;
        }
        if (Host == nullptr || Remote == nullptr || Host->GetPlayerState() == nullptr || Remote->GetPlayerState() == nullptr) return;

        const FUniqueNetIdStringRef HostId = FUniqueNetIdString::Create(
            TEXT("m9-land-host"), FName(TEXT("KalmalaM9ExplorationPeerTest")));
        const FUniqueNetIdStringRef RemoteId = FUniqueNetIdString::Create(
            TEXT("m9-land-remote"), FName(TEXT("KalmalaM9ExplorationPeerTest")));
        Host->GetPlayerState()->SetUniqueId(FUniqueNetIdRepl(*HostId));
        Remote->GetPlayerState()->SetUniqueId(FUniqueNetIdRepl(*RemoteId));

        bool bFoundDescriptor = false;
        for (int32 Y = -160; Y <= 160 && !bFoundDescriptor; ++Y)
        {
            for (int32 X = -160; X <= 160 && !bFoundDescriptor; ++X)
            {
                const FIntPoint SpatialKey(X, Y);
                const FVector2D KeyCenter = (FVector2D(SpatialKey) + FVector2D(0.5f, 0.5f))
                    * FKalmalaWorldPopulationLayout::SpatialKeySizeCm;
                const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(
                    FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, KeyCenter));
                if (KeyBiome != EKalmalaBiome::ShimmeringLakes && KeyBiome != EKalmalaBiome::ThunderMountains) continue;

                const TArray<FKalmalaM9ExplorationRewardDescriptor> Candidates =
                    FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(WorldGenerationConfig, SpatialKey);
                if (Candidates.IsEmpty()) continue;
                M9ExplorationRewardPeerTestDescriptor = Candidates[0];
                bFoundDescriptor = true;
            }
        }
        if (!bFoundDescriptor)
        {
            UE_LOG(LogTemp, Error, TEXT("M9 exploration reward verification FAILED: no canonical descriptor in bounded world scan."));
            M9ExplorationRewardPeerTestStage = 99;
            return;
        }

        const FKalmalaM9ExplorationRewardDefinition* Definition =
            FKalmalaM9ExplorationRewardCatalogue::FindDefinition(M9ExplorationRewardPeerTestDescriptor.CandidateId);
        UKalmalaInventoryComponent* HostInventory = Host->GetInventoryComponent();
        UKalmalaInventoryComponent* RemoteInventory = Remote->GetInventoryComponent();
        if (Definition == nullptr || HostInventory == nullptr || RemoteInventory == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("M9 exploration reward verification FAILED: catalogue or owner inventory missing."));
            M9ExplorationRewardPeerTestStage = 99;
            return;
        }

        ActivePopulationSpatialKeys.Add(M9ExplorationRewardPeerTestDescriptor.SpatialKey);
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaDiscoveryActor* Actor = GetWorld()->SpawnActor<AKalmalaDiscoveryActor>(
            AKalmalaDiscoveryActor::StaticClass(), M9ExplorationRewardPeerTestDescriptor.Location,
            FRotator::ZeroRotator, SpawnParameters);
        if (Actor == nullptr || !Actor->InitializeM9ExplorationRewardServer(M9ExplorationRewardPeerTestDescriptor))
        {
            if (Actor != nullptr) Actor->Destroy();
            UE_LOG(LogTemp, Error, TEXT("M9 exploration reward verification FAILED: server actor initialization rejected its descriptor."));
            M9ExplorationRewardPeerTestStage = 99;
            return;
        }

        M9ExplorationRewardPeerTestHost = Host;
        M9ExplorationRewardPeerTestRemote = Remote;
        M9ExplorationRewardPeerTestActor = Actor;
        M9ExplorationRewardPeerTestHostBaseline = HostInventory->GetQuantity(Definition->RewardItemId);
        M9ExplorationRewardPeerTestRemoteBaseline = RemoteInventory->GetQuantity(Definition->RewardItemId);
        Host->SetActorLocation(Actor->GetActorLocation() - FVector(125.0f, 0.0f, 0.0f), false);
        Remote->SetActorLocation(Actor->GetActorLocation() + FVector(1000.0f, 0.0f, 0.0f), false);
        Host->ForceNetUpdate();
        Remote->ForceNetUpdate();
        M9ExplorationRewardPeerTestStage = 1;
        return;
    }

    AKalmalaCharacter* Host = M9ExplorationRewardPeerTestHost.Get();
    AKalmalaCharacter* Remote = M9ExplorationRewardPeerTestRemote.Get();
    AKalmalaDiscoveryActor* Actor = M9ExplorationRewardPeerTestActor.Get();
    const FKalmalaM9ExplorationRewardDefinition* Definition =
        FKalmalaM9ExplorationRewardCatalogue::FindDefinition(M9ExplorationRewardPeerTestDescriptor.CandidateId);
    if (Host == nullptr || Remote == nullptr || Actor == nullptr || Definition == nullptr
        || Host->GetInventoryComponent() == nullptr || Remote->GetInventoryComponent() == nullptr
        || Host->GetDiscoveryProgressComponent() == nullptr || Remote->GetDiscoveryProgressComponent() == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("M9 exploration reward verification FAILED: peer state disappeared."));
        M9ExplorationRewardPeerTestStage = 99;
        return;
    }

    if (M9ExplorationRewardPeerTestStage == 1)
    {
        UKalmalaInventoryComponent* HostInventory = Host->GetInventoryComponent();
        UKalmalaInventoryComponent* RemoteInventory = Remote->GetInventoryComponent();
        UKalmalaDiscoveryProgressComponent* HostFeedback = Host->GetDiscoveryProgressComponent();
        UKalmalaDiscoveryProgressComponent* RemoteFeedback = Remote->GetDiscoveryProgressComponent();
        const bool bDistantRejected = !Actor->CanInteract_Implementation(Remote)
            && !ClaimM9ExplorationReward(Remote, M9ExplorationRewardPeerTestDescriptor)
            && RemoteInventory->GetQuantity(Definition->RewardItemId) == M9ExplorationRewardPeerTestRemoteBaseline
            && RemoteFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::None
            && RemoteFeedback->GetFeedbackSerial() == 0;

        FKalmalaM9ExplorationRewardDescriptor Forged = M9ExplorationRewardPeerTestDescriptor;
        Forged.Location.X += 500.0f;
        const bool bForgedRejected = !ClaimM9ExplorationReward(Host, Forged)
            && HostInventory->GetQuantity(Definition->RewardItemId) == M9ExplorationRewardPeerTestHostBaseline
            && HostFeedback->GetFeedbackSerial() == 0;

        Actor->Interact_Implementation(Host);
        const FString ExpectedFeedback = FString::Printf(TEXT("%s (+%d %s)"), *Definition->ObservationText,
            Definition->RewardQuantity, *Definition->RewardItemId.ToString());
        const bool bFirstClaimAccepted = HostInventory->GetQuantity(Definition->RewardItemId)
                == M9ExplorationRewardPeerTestHostBaseline + Definition->RewardQuantity
            && HostFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
            && HostFeedback->GetFeedbackSerial() == 1
            && HostFeedback->GetFeedbackLabel() == ExpectedFeedback;
        const bool bRemoteUnchanged = RemoteInventory->GetQuantity(Definition->RewardItemId)
                == M9ExplorationRewardPeerTestRemoteBaseline
            && RemoteFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::None
            && RemoteFeedback->GetFeedbackSerial() == 0;
        if (!bDistantRejected || !bForgedRejected || !bFirstClaimAccepted || !bRemoteUnchanged)
        {
            UE_LOG(LogTemp, Error,
                TEXT("M9 exploration reward verification FAILED: Distant=%d Forged=%d First=%d RemoteUnchanged=%d HostQty=%d RemoteQty=%d HostFeedback=%d RemoteFeedback=%d"),
                bDistantRejected, bForgedRejected, bFirstClaimAccepted, bRemoteUnchanged,
                HostInventory->GetQuantity(Definition->RewardItemId), RemoteInventory->GetQuantity(Definition->RewardItemId),
                HostFeedback->GetFeedbackSerial(), RemoteFeedback->GetFeedbackSerial());
            M9ExplorationRewardPeerTestStage = 99;
            return;
        }
        M9ExplorationRewardPeerTestStage = 2;
        return;
    }

    if (M9ExplorationRewardPeerTestStage == 2)
    {
        UKalmalaInventoryComponent* HostInventory = Host->GetInventoryComponent();
        UKalmalaInventoryComponent* RemoteInventory = Remote->GetInventoryComponent();
        UKalmalaDiscoveryProgressComponent* HostFeedback = Host->GetDiscoveryProgressComponent();
        UKalmalaDiscoveryProgressComponent* RemoteFeedback = Remote->GetDiscoveryProgressComponent();

        Host->SetActorLocation(Actor->GetActorLocation() - FVector(125.0f, 0.0f, 0.0f), false);
        Actor->Interact_Implementation(Host);
        const bool bActorReplayRejected = HostInventory->GetQuantity(Definition->RewardItemId)
                == M9ExplorationRewardPeerTestHostBaseline + Definition->RewardQuantity
            && HostFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::AlreadyFound
            && HostFeedback->GetFeedbackSerial() == 2;
        const bool bDirectReplayRejected = !ClaimM9ExplorationReward(Host, M9ExplorationRewardPeerTestDescriptor)
            && HostInventory->GetQuantity(Definition->RewardItemId)
                == M9ExplorationRewardPeerTestHostBaseline + Definition->RewardQuantity;
        const bool bOtherOwnerUnaffected = RemoteInventory->GetQuantity(Definition->RewardItemId)
                == M9ExplorationRewardPeerTestRemoteBaseline
            && RemoteFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::None
            && RemoteFeedback->GetFeedbackSerial() == 0;

        Remote->SetActorLocation(Actor->GetActorLocation() - FVector(125.0f, 0.0f, 0.0f), false);
        Remote->ForceNetUpdate();
        Actor->Interact_Implementation(Remote);
        const bool bIndependentPlayerClaim = RemoteInventory->GetQuantity(Definition->RewardItemId)
                == M9ExplorationRewardPeerTestRemoteBaseline + Definition->RewardQuantity
            && RemoteFeedback->GetFeedback() == EKalmalaDiscoveryFeedback::LandmarkFound
            && RemoteFeedback->GetFeedbackSerial() == 1;

        const bool bPassed = bActorReplayRejected && bDirectReplayRejected && bOtherOwnerUnaffected
            && bIndependentPlayerClaim
            && HostInventory->GetQuantity(Definition->RewardItemId)
                == M9ExplorationRewardPeerTestHostBaseline + Definition->RewardQuantity;
        if (bPassed)
        {
            UE_LOG(LogTemp, Display,
                TEXT("M9 exploration reward verification server: Passed=1 Deterministic=1 DistantRejected=1 ForgedRejected=1 ActorReplayRejected=1 DirectReplayRejected=1 IndependentPlayer=1 OwnerOnlyState=1 Candidate=%s HostQty=%d RemoteQty=%d HostFeedbackSerial=%u RemoteFeedbackSerial=%u"),
                *Definition->CandidateId.ToString(), HostInventory->GetQuantity(Definition->RewardItemId),
                RemoteInventory->GetQuantity(Definition->RewardItemId), HostFeedback->GetFeedbackSerial(), RemoteFeedback->GetFeedbackSerial());
        }
        else
        {
            UE_LOG(LogTemp, Error,
                TEXT("M9 exploration reward verification server: Passed=0 ActorReplay=%d DirectReplay=%d OtherOwner=%d Independent=%d HostQty=%d RemoteQty=%d HostFeedbackSerial=%u RemoteFeedbackSerial=%u"),
                bActorReplayRejected, bDirectReplayRejected, bOtherOwnerUnaffected, bIndependentPlayerClaim,
                HostInventory->GetQuantity(Definition->RewardItemId), RemoteInventory->GetQuantity(Definition->RewardItemId),
                HostFeedback->GetFeedbackSerial(), RemoteFeedback->GetFeedbackSerial());
        }
        M9ExplorationRewardPeerTestStage = 99;
    }
#endif
}

TArray<AKalmalaCharacter*> AKalmalaGameMode::GetLivingPlayerPawnsForPopulation() const
{
    TArray<AKalmalaCharacter*> LivingPlayerPawns;
    UWorld* World = GetWorld();
    if (!HasAuthority() || !IsValid(World))
    {
        return LivingPlayerPawns;
    }

    struct FPopulationPlayerEntry
    {
        int32 PlayerId = INDEX_NONE;
        FString ControllerPath;
        AKalmalaCharacter* Character = nullptr;
    };
    TArray<FPopulationPlayerEntry> Candidates;
    for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* Controller = Iterator->Get();
        if (!IsValid(Controller) || Controller->GetWorld() != World)
        {
            continue;
        }

        const APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>();
        if (!IsValid(PlayerState) || PlayerState->GetWorld() != World
            || PlayerState->IsInactive() || PlayerState->IsSpectator() || PlayerState->IsOnlyASpectator())
        {
            continue;
        }

        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Controller->GetPawn());
        if (!IsValid(Character) || Character->GetWorld() != World || Character->GetController() != Controller
            || !FMath::IsFinite(Character->GetHealth()) || Character->GetHealth() <= 1.0f)
        {
            continue;
        }

        FPopulationPlayerEntry& Entry = Candidates.AddDefaulted_GetRef();
        Entry.PlayerId = PlayerState->GetPlayerId();
        Entry.ControllerPath = Controller->GetPathName();
        Entry.Character = Character;
    }

    Candidates.Sort([](const FPopulationPlayerEntry& A, const FPopulationPlayerEntry& B)
    {
        if (A.PlayerId != B.PlayerId)
        {
            return A.PlayerId < B.PlayerId;
        }
        return A.ControllerPath < B.ControllerPath;
    });

    TSet<const AKalmalaCharacter*> SeenPawns;
    for (const FPopulationPlayerEntry& Candidate : Candidates)
    {
        if (SeenPawns.Contains(Candidate.Character))
        {
            continue;
        }
        SeenPawns.Add(Candidate.Character);
        LivingPlayerPawns.Add(Candidate.Character);
    }
    return LivingPlayerPawns;
}

void AKalmalaGameMode::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || !WorldGenerationConfig.IsValid() || GetWorld() == nullptr)
    {
        return;
    }

    WildlifeRewardRetrySeconds -= FMath::Clamp(DeltaSeconds, 0.0f, 0.10f);
    if (WildlifeRewardRetrySeconds <= 0.0f)
    {
        WildlifeRewardRetrySeconds = 0.50f;
        DeliverPendingWildlifeRewards();
    }

    for (int32 Index = PendingOceanTravelRestores.Num() - 1; Index >= 0; --Index)
    {
        APlayerController* PendingController = PendingOceanTravelRestores[Index].Get();
        if (!IsValid(PendingController))
        {
            PendingOceanTravelRestores.RemoveAtSwap(Index);
            continue;
        }
        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PendingController->GetPawn());
        if (Character == nullptr || Character->GetPlayerState() == nullptr) continue;

        PendingOceanTravelRestores.RemoveAtSwap(Index);
        FString IgnoredIdentity;
        GetPlayerDiscoverySave(Character, IgnoredIdentity);
        LoadOceanDiscoveryLedger(Character);
        RestoreOceanTravelForPlayer(PendingController);
    }

#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanDiscoveryDisembarkPeerTest")))
    {
        bool bFixtureExists = false;
        for (TActorIterator<AKalmalaOceanDiscoveryDisembarkPeerTest> It(GetWorld()); It; ++It)
        {
            bFixtureExists = true;
            break;
        }
        if (!bFixtureExists)
        {
            FActorSpawnParameters Parameters;
            Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            GetWorld()->SpawnActor<AKalmalaOceanDiscoveryDisembarkPeerTest>(
                AKalmalaOceanDiscoveryDisembarkPeerTest::StaticClass(), FVector::ZeroVector,
                FRotator::ZeroRotator, Parameters);
        }
    }
#endif

    DriveTraversalTest();
    DriveCampChoiceTest();
    DriveRainVerticalSliceTest();
    DriveCombatPeerTest();
    DriveMosquitoPeerTest();
    DrivePeatHuskPeerTest();
    DriveWolfPeerTest();
    DriveLynxPeerTest();
    DrivePolarBearPeerTest();
    DriveHornedGoatPeerTest();
    DriveThunderSentinelPeerTest();
    DriveReefMawPeerTest();
    DriveA9PeerTest();
    DriveDiscoveryPeerTest();
    DriveM9ExplorationRewardPeerTest();
    DriveOceanTravelFeedbackTest();
    DriveOceanWeatherPeerTest();
    DriveOceanJourneyPeerTest();
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

    if (GetWorld()->GetTimeSeconds() >= NextOceanDiscoveryRefreshTime)
    {
        NextOceanDiscoveryRefreshTime = GetWorld()->GetTimeSeconds() + 1.0f;
        RefreshOceanDiscoveries();
    }

    if (GetWorld()->GetTimeSeconds() < NextTerrainPatchActivationTime)
    {
        return;
    }

    NextTerrainPatchActivationTime = GetWorld()->GetTimeSeconds() + KalmalaGameMode::TerrainPatchActivationIntervalSeconds;
    RefreshTerrainPatchNeighborhoods();
    for (const AKalmalaCharacter* PlayerPawn : GetLivingPlayerPawnsForPopulation())
    {
        if (IsValid(PlayerPawn))
        {
            ActivatePopulationKey(FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(PlayerPawn->GetActorLocation())));
        }
    }
}

void AKalmalaGameMode::DriveOceanTravelFeedbackTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanSkiffFeedbackTest"))
        || GetWorld() == nullptr || OceanTravelFeedbackTestStage >= 2)
    {
        return;
    }

    if (OceanTravelFeedbackTestStage == 0)
    {
        AKalmalaCharacter* HostCharacter = nullptr;
        AKalmalaCharacter* RemoteCharacter = nullptr;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* Controller = It->Get();
            AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
            if (Character == nullptr) continue;
            if (Controller->IsLocalController()) HostCharacter = Character;
            else RemoteCharacter = Character;
        }
        if (HostCharacter == nullptr || RemoteCharacter == nullptr) return;

        UKalmalaOceanTravelFeedbackComponent* HostFeedback = HostCharacter->GetOceanTravelFeedbackComponent();
        UKalmalaOceanTravelFeedbackComponent* RemoteFeedback = RemoteCharacter->GetOceanTravelFeedbackComponent();
        if (HostFeedback == nullptr || RemoteFeedback == nullptr
            || !HostFeedback->SetFeedbackFromServer(EKalmalaOceanTravelFeedback::ShallowLaunch)
            || !RemoteFeedback->SetFeedbackFromServer(EKalmalaOceanTravelFeedback::SeatsFull))
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean skiff feedback verification FAILED: owner feedback component could not accept server state."));
            OceanTravelFeedbackTestStage = 2;
            return;
        }

        OceanTravelFeedbackTestHost = HostCharacter;
        OceanTravelFeedbackTestRemote = RemoteCharacter;
        OceanTravelFeedbackTestStageTime = GetWorld()->GetTimeSeconds();
        OceanTravelFeedbackTestStage = 1;
        return;
    }

    if (GetWorld()->GetTimeSeconds() - OceanTravelFeedbackTestStageTime < 1.0f) return;
    const AKalmalaCharacter* HostCharacter = OceanTravelFeedbackTestHost.Get();
    const AKalmalaCharacter* RemoteCharacter = OceanTravelFeedbackTestRemote.Get();
    const UKalmalaOceanTravelFeedbackComponent* HostFeedback = HostCharacter != nullptr
        ? HostCharacter->GetOceanTravelFeedbackComponent() : nullptr;
    const UKalmalaOceanTravelFeedbackComponent* RemoteFeedback = RemoteCharacter != nullptr
        ? RemoteCharacter->GetOceanTravelFeedbackComponent() : nullptr;
    const bool bPassed = HostFeedback != nullptr && RemoteFeedback != nullptr
        && HostFeedback->GetFeedback() == EKalmalaOceanTravelFeedback::ShallowLaunch
        && HostFeedback->GetFeedbackSerial() == 1
        && RemoteFeedback->GetFeedback() == EKalmalaOceanTravelFeedback::SeatsFull
        && RemoteFeedback->GetFeedbackSerial() == 1;
    if (bPassed)
    {
        UE_LOG(LogTemp, Display,
            TEXT("Ocean skiff feedback verification server: Passed=1 Host=ShallowLaunch Remote=SeatsFull HostSerial=%u RemoteSerial=%u"),
            HostFeedback->GetFeedbackSerial(), RemoteFeedback->GetFeedbackSerial());
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("Ocean skiff feedback verification server: Passed=0 HostSerial=%u RemoteSerial=%u"),
            HostFeedback != nullptr ? HostFeedback->GetFeedbackSerial() : 0,
            RemoteFeedback != nullptr ? RemoteFeedback->GetFeedbackSerial() : 0);
    }
    OceanTravelFeedbackTestStage = 2;
#endif
}

void AKalmalaGameMode::DriveOceanWeatherPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanWeatherPeerTest"))
        || GetWorld() == nullptr || OceanWeatherPeerTestStage >= 3)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (OceanWeatherPeerTestStage == 0)
    {
        int32 PlayerCount = 0;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            PlayerCount += It->Get() != nullptr ? 1 : 0;
        }
        if (PlayerCount < 2) return;

        const auto SetVerificationWeather = [this, Now](const int32 CycleIndex,
            const int32 WindDirectionDegrees, const float WindStrength)
        {
            AKalmalaWorldGenerationGameState* State = GetGameState<AKalmalaWorldGenerationGameState>();
            if (State == nullptr) return false;
            FKalmalaWeatherState Weather = State->GetWeatherState();
            Weather.WeatherCycleIndex = CycleIndex;
            Weather.ServerStartTimeSeconds = Now;
            Weather.DurationSeconds = 120.0f;
            Weather.PrecipitationIntensity = 0.0f;
            Weather.FogIntensity = 0.0f;
            Weather.WindDirectionDegrees = WindDirectionDegrees;
            Weather.WindStrength = WindStrength;
            Weather.RefreshActivityLevel();
            State->SetWeatherStateFromServer(Weather);
            return State->GetWeatherState().WeatherCycleIndex == CycleIndex;
        };

        if (!SetVerificationWeather(7001, 90, 1.0f))
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean weather peer verification FAILED: server could not select crosswind state."));
            OceanWeatherPeerTestStage = 3;
            return;
        }
        OceanWeatherPeerTestStageTime = Now;
        OceanWeatherPeerTestStage = 1;
        return;
    }

    if (OceanWeatherPeerTestStage == 1 && Now - OceanWeatherPeerTestStageTime >= 3.0f)
    {
        AKalmalaWorldGenerationGameState* State = GetGameState<AKalmalaWorldGenerationGameState>();
        if (State == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean weather peer verification FAILED: world state disappeared."));
            OceanWeatherPeerTestStage = 3;
            return;
        }
        FKalmalaWeatherState Weather = State->GetWeatherState();
        Weather.WeatherCycleIndex = 7002;
        Weather.ServerStartTimeSeconds = Now;
        Weather.DurationSeconds = 120.0f;
        Weather.PrecipitationIntensity = 0.0f;
        Weather.FogIntensity = 0.0f;
        Weather.WindDirectionDegrees = 0;
        Weather.WindStrength = 0.0f;
        Weather.RefreshActivityLevel();
        State->SetWeatherStateFromServer(Weather);
        if (State->GetWeatherState().WeatherCycleIndex != 7002)
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean weather peer verification FAILED: server could not select calm recovery state."));
            OceanWeatherPeerTestStage = 3;
            return;
        }
        OceanWeatherPeerTestStageTime = Now;
        OceanWeatherPeerTestStage = 2;
        return;
    }

    if (OceanWeatherPeerTestStage == 2 && Now - OceanWeatherPeerTestStageTime >= 3.0f)
    {
        UE_LOG(LogTemp, Display, TEXT("Ocean weather peer verification server states complete."));
        OceanWeatherPeerTestStage = 3;
    }
#endif
}

void AKalmalaGameMode::DriveOceanJourneyPeerTest()
{
#if !UE_BUILD_SHIPPING
    if (!FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest"))
        || GetWorld() == nullptr || OceanJourneyPeerTestStage >= 2)
    {
        return;
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (OceanJourneyPeerTestStartTime <= 0.0f)
    {
        OceanJourneyPeerTestStartTime = Now;
    }

    if (OceanJourneyPeerTestStage == 0)
    {
        AKalmalaCharacter* Host = nullptr;
        AKalmalaCharacter* Remote = nullptr;
        int32 PlayerCount = 0;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* Controller = It->Get();
            AKalmalaCharacter* Character = Controller != nullptr
                ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
            if (Controller == nullptr || Character == nullptr) continue;
            ++PlayerCount;
            if (Controller->IsLocalController()) Host = Character;
            else Remote = Character;
        }

        if (PlayerCount < 2)
        {
            if (Now - OceanJourneyPeerTestStartTime > 120.0f)
            {
                UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: two player pawns did not join."));
                OceanJourneyPeerTestStage = 99;
            }
            return;
        }
        if (PlayerCount != 2 || Host == nullptr || Remote == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: expected one listen host and one remote player; found %d."), PlayerCount);
            OceanJourneyPeerTestStage = 99;
            return;
        }

        if (!KalmalaOceanJourneyPeer::FindRoute(WorldGenerationConfig,
            FVector2D(GeneratedPlayerStart != nullptr ? GeneratedPlayerStart->GetActorLocation() : FVector::ZeroVector),
            TerrainPatchOrigin, OceanJourneyPeerTestLaunch, OceanJourneyPeerTestYaw))
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: seed %llu has no coast launch and 2.4 km deep-water patch-crossing corridor."),
                WorldGenerationConfig.WorldSeed);
            OceanJourneyPeerTestStage = 99;
            return;
        }

        for (TActorIterator<AKalmalaOceanSkiff> It(GetWorld()); It; ++It)
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: isolated journey profile already contains skiff %s."),
                *It->GetPersistentVesselId());
            OceanJourneyPeerTestStage = 99;
            return;
        }

        const FIntPoint LaunchPatchCoordinate = FKalmalaTerrainPatchLayout::GetPatchCoordinate(
            TerrainPatchOrigin, OceanJourneyPeerTestLaunch);
        ActivateTerrainPatchNeighborhood(OceanJourneyPeerTestLaunch);
        TObjectPtr<AKalmalaGeneratedTerrainPatch>* PatchEntry = ActiveTerrainPatches.Find(LaunchPatchCoordinate);
        AKalmalaGeneratedTerrainPatch* LaunchPatch = PatchEntry != nullptr ? PatchEntry->Get() : nullptr;
        TArray<UPrimitiveComponent*> Components;
        if (LaunchPatch != nullptr) LaunchPatch->GetComponents<UPrimitiveComponent>(Components);
        UPrimitiveComponent* TerrainCollision = nullptr;
        for (UPrimitiveComponent* Component : Components)
        {
            if (Component != nullptr && Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision
                && Component->GetCollisionObjectType() == ECC_WorldStatic)
            {
                TerrainCollision = Component;
                break;
            }
        }
        if (LaunchPatch == nullptr || !LaunchPatch->HasGenerationData() || TerrainCollision == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: generated launch patch collision was unavailable."));
            OceanJourneyPeerTestStage = 99;
            return;
        }

        const FKalmalaOceanSample LaunchSample = FKalmalaOceanSampler::Sample(
            WorldGenerationConfig, OceanJourneyPeerTestLaunch);
        const FVector HitLocation(OceanJourneyPeerTestLaunch.X, OceanJourneyPeerTestLaunch.Y,
            LaunchSample.TerrainHeight);
        const FVector PawnLocation = HitLocation + FVector(0.0f, 0.0f, 100.0f);
        const FRotator LaunchRotation(0.0f, OceanJourneyPeerTestYaw, 0.0f);
        Host->SetActorLocationAndRotation(PawnLocation, LaunchRotation, false, nullptr, ETeleportType::TeleportPhysics);
        Remote->SetActorLocationAndRotation(PawnLocation, LaunchRotation, false, nullptr, ETeleportType::TeleportPhysics);

        FHitResult LaunchHit(LaunchPatch, TerrainCollision, HitLocation, FVector::UpVector);
        LaunchHit.bBlockingHit = true;
        LaunchHit.ImpactPoint = HitLocation;
        LaunchHit.TraceStart = PawnLocation;
        LaunchHit.TraceEnd = HitLocation;
        LaunchHit.Distance = 100.0f;
        AKalmalaOceanSkiff* Skiff = AKalmalaOceanSkiff::TryLaunchFromServer(Remote, LaunchHit);
        if (Skiff == nullptr || !Skiff->TryInteractFromServer(Remote) || !Skiff->TryInteractFromServer(Host)
            || Skiff->GetHelmOccupant() != Remote || Skiff->GetPassengerOccupant() != Host)
        {
            UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: generated server launch or two-seat embark was rejected."));
            OceanJourneyPeerTestStage = 99;
            return;
        }

        OceanJourneyPeerTestSkiff = Skiff;
        OceanJourneyPeerTestHelm = Remote;
        OceanJourneyPeerTestPassenger = Host;
        OceanJourneyPeerTestTarget = OceanJourneyPeerTestLaunch
            + FVector2D(FRotator(0.0f, OceanJourneyPeerTestYaw, 0.0f).Vector()) * 240000.0f;
        OceanJourneyPeerTestStageTime = Now;
        OceanJourneyPeerTestStage = 1;
        UE_LOG(LogTemp, Display,
            TEXT("Ocean journey peer launched and embarked: Seed=%llu Vessel=%s Seats=Helm,Passenger Start=%s Target=%s Yaw=%.1f Depth=%.1f."),
            WorldGenerationConfig.WorldSeed, *Skiff->GetPersistentVesselId(), *OceanJourneyPeerTestLaunch.ToString(),
            *OceanJourneyPeerTestTarget.ToString(), OceanJourneyPeerTestYaw, LaunchSample.WaterDepth);
        return;
    }

    AKalmalaOceanSkiff* Skiff = OceanJourneyPeerTestSkiff.Get();
    AKalmalaCharacter* Helm = OceanJourneyPeerTestHelm.Get();
    AKalmalaCharacter* Passenger = OceanJourneyPeerTestPassenger.Get();
    const AKalmalaWorldGenerationGameState* State = GetGameState<AKalmalaWorldGenerationGameState>();
    if (!IsValid(Skiff) || !IsValid(Helm) || !IsValid(Passenger) || State == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: skiff, seat, or world state disappeared."));
        OceanJourneyPeerTestStage = 99;
        return;
    }

    const FKalmalaWeatherState& Weather = State->GetWeatherState();
    bOceanJourneyPeerTestSawCrosswind |= Weather.WeatherCycleIndex == 7001
        && Skiff->GetMode() == EKalmalaOceanSkiffMode::Underway;
    bOceanJourneyPeerTestSawCalm |= Weather.WeatherCycleIndex == 7002;

    const FVector2D StartPosition = OceanJourneyPeerTestLaunch;
    const float Travelled = FVector2D::Distance(StartPosition, FVector2D(Skiff->GetActorLocation()));
    if (Skiff->GetMode() == EKalmalaOceanSkiffMode::Blocked)
    {
        UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: travel blocked with reason %d at %s."),
            static_cast<int32>(Skiff->GetBlockReason()), *Skiff->GetActorLocation().ToCompactString());
        OceanJourneyPeerTestStage = 99;
        return;
    }
    if (Now - OceanJourneyPeerTestStageTime > 480.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: 2.4 km voyage exceeded 480 seconds; reached %.0f cm."), Travelled);
        OceanJourneyPeerTestStage = 99;
        return;
    }

    if (Skiff->GetMode() != EKalmalaOceanSkiffMode::Moored || Travelled < 239000.0f)
    {
        return;
    }

    const FVector2D CurrentPosition(Skiff->GetActorLocation());
    const FIntPoint StartPatch = FKalmalaTerrainPatchLayout::GetPatchCoordinate(TerrainPatchOrigin, OceanJourneyPeerTestLaunch);
    const FIntPoint CurrentPatch = FKalmalaTerrainPatchLayout::GetPatchCoordinate(TerrainPatchOrigin, CurrentPosition);
    const bool bSeatsMatch = Skiff->GetHelmOccupant() == Helm && Skiff->GetPassengerOccupant() == Passenger
        && Helm->GetAttachParentActor() == Skiff && Passenger->GetAttachParentActor() == Skiff;
    const bool bPassed = bSeatsMatch && CurrentPatch != StartPatch && !ActiveTerrainPatches.IsEmpty()
        && ActiveTerrainPatches.Num() <= KalmalaGameMode::MaxActiveTerrainPatches
        && bOceanJourneyPeerTestSawCrosswind && bOceanJourneyPeerTestSawCalm
        && FKalmalaWorldBounds::Contains(WorldGenerationConfig, CurrentPosition);
    UE_LOG(LogTemp, Display,
        TEXT("Ocean journey peer verification %s: Seed=%llu Seats=Helm,Passenger Distance=%.0f PatchStart=(%d,%d) PatchEnd=(%d,%d) ActivePatches=%d Crosswind=%d Calm=%d OriginShift=inactive."),
        bPassed ? TEXT("passed") : TEXT("FAILED"), WorldGenerationConfig.WorldSeed, Travelled,
        StartPatch.X, StartPatch.Y, CurrentPatch.X, CurrentPatch.Y, ActiveTerrainPatches.Num(),
        bOceanJourneyPeerTestSawCrosswind ? 1 : 0, bOceanJourneyPeerTestSawCalm ? 1 : 0);
    if (!bPassed)
    {
        UE_LOG(LogTemp, Error, TEXT("Ocean journey peer verification FAILED: seat, weather, bounds, or streaming contract did not hold."));
    }
    OceanJourneyPeerTestStage = 2;
#endif
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
    const EKalmalaBiome KeyBiome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, (FVector2D(SpatialKey) + FVector2D(0.5f, 0.5f)) * FKalmalaWorldPopulationLayout::SpatialKeySizeCm));
    for (const EKalmalaWorldPopulationKind Kind : { EKalmalaWorldPopulationKind::Wildlife, EKalmalaWorldPopulationKind::HarvestNode, EKalmalaWorldPopulationKind::Hazard })
    {
        TArray<FKalmalaWorldPopulationSpawn> Spawns = FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, Kind);
        // Preserve the ordinary budget, then retain the separately approved salt slot.
        TArray<FKalmalaWorldPopulationSpawn> SaltSlots;
        if (Kind == EKalmalaWorldPopulationKind::HarvestNode)
        {
            SaltSlots = Spawns.FilterByPredicate([](const auto& Spawn) { return Spawn.ContentId == TEXT("tundra-frost-salt-deposit") || Spawn.ContentId == TEXT("gem-opal-rock"); });
            Spawns.RemoveAll([](const auto& Spawn) { return Spawn.ContentId == TEXT("tundra-frost-salt-deposit") || Spawn.ContentId == TEXT("gem-opal-rock"); });
        }
        if (KeyBiome == EKalmalaBiome::ShimmeringLakes || KeyBiome == EKalmalaBiome::Elderwood || KeyBiome == EKalmalaBiome::MossyMire || KeyBiome == EKalmalaBiome::FreezingTundra || KeyBiome == EKalmalaBiome::ThunderMountains)
        {
            Spawns.SetNum(FMath::Min(Spawns.Num(), FKalmalaBiomeExpansionContract::ApplyPopulationBudget(Spawns.Num(), Kind, KeyBiome)));
        }
        Spawns.Append(SaltSlots);
        const int32 WildlifeActorBudget = Kind != EKalmalaWorldPopulationKind::Wildlife ? 0
            : (KeyBiome == EKalmalaBiome::FreezingTundra
                ? FKalmalaWolfPackContract::GetActorBudgetWithPackRoom(
                    FKalmalaBiomeExpansionContract::ApplyPopulationBudget(Spawns.Num(), Kind, KeyBiome))
                : Spawns.Num());
        int32 SpawnedWildlifeActorCount = 0;
        for (const FKalmalaWorldPopulationSpawn& Spawn : Spawns)
        {
            if (Kind == EKalmalaWorldPopulationKind::HarvestNode)
            {
                const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                const FString M9ResourceId = FKalmalaM9SourceLootContract::BuildResourceDepletionId(Spawn.ContentId, PersistentSpawnId);
                if (FKalmalaM9SourceLootContract::IsM9Source(Spawn.ContentId)
                    ? (M9ResourceId.IsEmpty() || SessionM9ResourceDepletionIds.Contains(M9ResourceId))
                    : (PopulationSaveGame != nullptr && PopulationSaveGame->IsHarvested(PersistentSpawnId)))
                {
                    continue;
                }

                AKalmalaHarvestNode* HarvestNode = GetWorld()->SpawnActor<AKalmalaHarvestNode>(AKalmalaHarvestNode::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
                if (HarvestNode != nullptr)
                {
                    HarvestNode->InitializeServer(Spawn);
                    HarvestNode->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
                    HarvestNode->OnM9ResourceDepleted.AddUObject(this, &AKalmalaGameMode::RecordM9ResourceDepleted);
                    ++SpawnedMarkerCount;
                }
            }
            else if (Kind == EKalmalaWorldPopulationKind::Wildlife)
            {
                EKalmalaWildlifeArchetype Archetype;
                if (!KalmalaGameMode::TryGetWildlifeArchetypeForSpawn(WorldGenerationConfig, Spawn, Archetype))
                {
                    continue;
                }
                const bool bHardEnemy = Archetype == EKalmalaWildlifeArchetype::GiantSnake
                    || Archetype == EKalmalaWildlifeArchetype::MoonBear
                    || Archetype == EKalmalaWildlifeArchetype::PeatHusk
                    || Archetype == EKalmalaWildlifeArchetype::PolarBear
                    || Archetype == EKalmalaWildlifeArchetype::ThunderSentinel
                    || Archetype == EKalmalaWildlifeArchetype::ReefMaw;
                const float StartDistance = GeneratedPlayerStart != nullptr
                    ? FVector::Dist2D(Spawn.Location, GeneratedPlayerStart->GetActorLocation()) : 0.0f;
                if (!AKalmalaWildlifeSpawn::IsHardEnemySpawnAllowedByStart(
                        bHardEnemy, GeneratedPlayerStart != nullptr, StartDistance))
                {
                    continue;
                }

                const FString PersistentSpawnId = FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn);
                if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(PersistentSpawnId))
                {
                    continue;
                }

                if (Archetype == EKalmalaWildlifeArchetype::Wolf)
                {
                    if (SpawnedWildlifeActorCount + FKalmalaWolfPackContract::MemberCount > WildlifeActorBudget)
                        continue;

                    TArray<FKalmalaWorldPopulationSpawn> MemberSpawns;
                    MemberSpawns.Reserve(FKalmalaWolfPackContract::MemberCount);
                    bool bValidFormation = true;
                    for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
                    {
                        FKalmalaWorldPopulationSpawn MemberSpawn;
                        if (!FKalmalaWolfPackContract::TryBuildMemberSpawn(
                                WorldGenerationConfig, Spawn, MemberIndex, MemberSpawn))
                        {
                            bValidFormation = false;
                            break;
                        }
                        MemberSpawns.Add(MoveTemp(MemberSpawn));
                    }
                    if (!bValidFormation || MemberSpawns.Num() != FKalmalaWolfPackContract::MemberCount) continue;

                    TArray<AKalmalaWildlifeSpawn*> PackMembers;
                    PackMembers.Reserve(FKalmalaWolfPackContract::MemberCount);
                    for (int32 MemberIndex = 0; MemberIndex < MemberSpawns.Num(); ++MemberIndex)
                    {
                        const FString MemberSpawnId = FKalmalaWolfPackContract::GetMemberPersistentSpawnId(Spawn, MemberIndex);
                        if (PopulationSaveGame != nullptr && PopulationSaveGame->IsDefeated(MemberSpawnId)) continue;

                        FActorSpawnParameters SpawnParameters;
                        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                        AKalmalaWildlifeSpawn* Member = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(
                            AKalmalaWildlifeSpawn::StaticClass(), MemberSpawns[MemberIndex].Location,
                            FRotator::ZeroRotator, SpawnParameters);
                        if (Member == nullptr)
                        {
                            bValidFormation = false;
                            break;
                        }
                        Member->InitializeServer(MemberSpawns[MemberIndex], EKalmalaWildlifeArchetype::Wolf, MemberIndex);
                        if (Member->GetPersistentSpawnId() != MemberSpawnId)
                        {
                            Member->Destroy();
                            bValidFormation = false;
                            break;
                        }
                        Member->OnDefeated.AddUObject(this, &AKalmalaGameMode::RecordDefeatedSpawn);
                        PackMembers.Add(Member);
                    }

                    if (!bValidFormation)
                    {
                        for (AKalmalaWildlifeSpawn* Member : PackMembers)
                            if (IsValid(Member)) Member->Destroy();
                        continue;
                    }

                    SpawnedWildlifeActorCount += PackMembers.Num();
                    SpawnedMarkerCount += PackMembers.Num();
                    continue;
                }

                if (SpawnedWildlifeActorCount >= WildlifeActorBudget) continue;

                AKalmalaWildlifeSpawn* WildlifeSpawn = GetWorld()->SpawnActor<AKalmalaWildlifeSpawn>(AKalmalaWildlifeSpawn::StaticClass(), Spawn.Location, FRotator::ZeroRotator);
                if (WildlifeSpawn != nullptr)
                {
                    WildlifeSpawn->InitializeServer(Spawn, Archetype);
                    WildlifeSpawn->OnDefeated.AddUObject(this, &AKalmalaGameMode::RecordDefeatedSpawn);
                    ++SpawnedWildlifeActorCount;
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

    for (const FKalmalaM9ExplorationRewardDescriptor& Descriptor :
        FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(WorldGenerationConfig, SpatialKey))
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaDiscoveryActor* Discovery = GetWorld()->SpawnActor<AKalmalaDiscoveryActor>(
            AKalmalaDiscoveryActor::StaticClass(), Descriptor.Location, FRotator::ZeroRotator, Parameters);
        if (Discovery != nullptr && Discovery->InitializeM9ExplorationRewardServer(Descriptor))
        {
            ++SpawnedMarkerCount;
        }
        else if (Discovery != nullptr)
        {
            Discovery->Destroy();
        }
    }

    ActivePopulationSpatialKeys.Add(SpatialKey);
    UE_LOG(LogTemp, Display, TEXT("Server activated %d deterministic population markers for spatial key (%d, %d); %d/%d active."), SpawnedMarkerCount, SpatialKey.X, SpatialKey.Y, ActivePopulationSpatialKeys.Num(), KalmalaGameMode::MaxActivePopulationSpatialKeys);
}

void AKalmalaGameMode::RecordHarvestedSpawn(const FString& PersistentSpawnId)
{
    if (HasAuthority() && !PersistentSpawnId.IsEmpty() && PopulationSaveGame != nullptr
        && PopulationSaveGame->MatchesWorld(WorldGenerationConfig))
    {
        PopulationSaveGame->MarkHarvested(PersistentSpawnId);
#if WITH_DEV_AUTOMATION_TESTS
        if (PopulationSaveWriterForTests)
        {
            PopulationSaveWriterForTests(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
            return;
        }
#endif
        const bool bSaved = UGameplayStatics::SaveGameToSlot(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
#if !UE_BUILD_SHIPPING
        if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaForestPickup"))
            && PersistentSpawnId.StartsWith(TEXT("forest:v1:")))
        {
            UE_LOG(LogTemp, Display, TEXT("Forest sparse write: Authority=1 Seed=%llu Id=%s Success=%d"),
                WorldGenerationConfig.WorldSeed, *PersistentSpawnId, bSaved);
        }
#endif
        (void)bSaved;
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

bool AKalmalaGameMode::RecordRatDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingWildlifeReward(PersistentSpawnId, Identity))
            return false;
    }

    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordBadgerDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingBadgerReward(PersistentSpawnId, Identity))
            return false;
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordMoonBearDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingMoonBearReward(PersistentSpawnId, Identity))
            return false;
        if (!Identity.IsEmpty() && FKalmalaGemstoneContract::MoonstoneSourceAccepted
            && !PopulationSaveGame->SetPendingGemReward(
                UKalmalaWorldPopulationSaveGame::MakeMoonBearPendingRewardClaimId(PersistentSpawnId),
                SelectEnemyGemForSpawn(PersistentSpawnId, true))) return false;
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordViperDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingViperReward(PersistentSpawnId, Identity))
            return false;
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordGiantSnakeDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingGiantSnakeReward(PersistentSpawnId, Identity))
            return false;
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordDeathAdderDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingDeathAdderReward(PersistentSpawnId, Identity))
            return false;
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordPeatHuskDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (PopulationSaveGame->IsDefeated(PersistentSpawnId))
    {
        return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
            KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty())
        {
            bool bFoundExistingClaim = false;
            bool bHasPeatHeart = false;
            for (const FKalmalaPendingWildlifeReward& Existing : PopulationSaveGame->GetPendingWildlifeRewards())
            {
                FString ExistingSpawnId;
                bool bExistingHasPeatHeart = false;
                if (UKalmalaWorldPopulationSaveGame::TryGetPeatHuskSpawnIdFromPendingRewardClaimId(
                        Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasPeatHeart)
                    && ExistingSpawnId == PersistentSpawnId)
                {
                    bFoundExistingClaim = true;
                    bHasPeatHeart = bExistingHasPeatHeart;
                    break;
                }
            }
            if (!bFoundExistingClaim) bHasPeatHeart = FMath::FRand() < 0.25f;
            if (!PopulationSaveGame->TryQueuePendingPeatHuskReward(PersistentSpawnId, Identity, bHasPeatHeart))
                return false;
        }
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordWolfDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (PopulationSaveGame->IsDefeated(PersistentSpawnId))
    {
        return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
            KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty())
        {
            bool bFoundExistingClaim = false;
            bool bHasWolfFang = false;
            for (const FKalmalaPendingWildlifeReward& Existing : PopulationSaveGame->GetPendingWildlifeRewards())
            {
                FString ExistingSpawnId;
                bool bExistingHasWolfFang = false;
                if (UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
                        Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasWolfFang)
                    && ExistingSpawnId == PersistentSpawnId)
                {
                    bFoundExistingClaim = true;
                    bHasWolfFang = bExistingHasWolfFang;
                    break;
                }
            }
            if (!bFoundExistingClaim) bHasWolfFang = FMath::FRand() < 0.25f;
            if (!PopulationSaveGame->TryQueuePendingWolfReward(PersistentSpawnId, Identity, bHasWolfFang))
                return false;
        }
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordLynxDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (PopulationSaveGame->IsDefeated(PersistentSpawnId))
    {
        return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
            KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty())
        {
            bool bFoundExistingClaim = false;
            bool bHasLynxPelt = false;
            for (const FKalmalaPendingWildlifeReward& Existing : PopulationSaveGame->GetPendingWildlifeRewards())
            {
                FString ExistingSpawnId;
                bool bExistingHasLynxPelt = false;
                if (UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
                        Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasLynxPelt)
                    && ExistingSpawnId == PersistentSpawnId)
                {
                    bFoundExistingClaim = true;
                    bHasLynxPelt = bExistingHasLynxPelt;
                    break;
                }
            }
            if (!bFoundExistingClaim) bHasLynxPelt = FMath::FRand() < 0.5f;
            if (!PopulationSaveGame->TryQueuePendingLynxReward(PersistentSpawnId, Identity, bHasLynxPelt))
                return false;
        }
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordPolarBearDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (PopulationSaveGame->IsDefeated(PersistentSpawnId))
    {
        return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
            KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingPolarBearReward(PersistentSpawnId, Identity))
            return false;
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordHornedGoatDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (PopulationSaveGame->IsDefeated(PersistentSpawnId))
    {
        return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
            KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty() && !PopulationSaveGame->TryQueuePendingHornedGoatReward(PersistentSpawnId, Identity))
            return false;
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordThunderSentinelDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (PopulationSaveGame->IsDefeated(PersistentSpawnId))
    {
        return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
            KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty())
        {
            bool bFoundExistingClaim = false;
            bool bHasThunderCore = false;
            for (const FKalmalaPendingWildlifeReward& Existing : PopulationSaveGame->GetPendingWildlifeRewards())
            {
                FString ExistingSpawnId;
                bool bExistingHasThunderCore = false;
                if (UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
                        Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasThunderCore)
                    && ExistingSpawnId == PersistentSpawnId)
                {
                    bFoundExistingClaim = true;
                    bHasThunderCore = bExistingHasThunderCore;
                    break;
                }
            }
            if (!bFoundExistingClaim) bHasThunderCore = FMath::FRand() < 0.25f;
            if (!PopulationSaveGame->TryQueuePendingThunderSentinelReward(
                    PersistentSpawnId, Identity, bHasThunderCore)) return false;
        }
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

bool AKalmalaGameMode::RecordReefMawDefeatReward(const FString& PersistentSpawnId,
    AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || PersistentSpawnId.IsEmpty() || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;

    if (PopulationSaveGame->IsDefeated(PersistentSpawnId))
    {
        return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
            KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
    }

    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority()
        && CreditedAttacker->GetWorld() == GetWorld() && CreditedAttacker->GetPlayerState() != nullptr)
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        if (!Identity.IsEmpty())
        {
            bool bFoundExistingClaim = false;
            bool bHasReefTooth = false;
            for (const FKalmalaPendingWildlifeReward& Existing : PopulationSaveGame->GetPendingWildlifeRewards())
            {
                FString ExistingSpawnId;
                bool bExistingHasReefTooth = false;
                if (UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
                        Existing.PersistentSpawnId, ExistingSpawnId, bExistingHasReefTooth)
                    && ExistingSpawnId == PersistentSpawnId)
                {
                    bFoundExistingClaim = true;
                    bHasReefTooth = bExistingHasReefTooth;
                    break;
                }
            }
            if (!bFoundExistingClaim) bHasReefTooth = FMath::FRand() < 0.5f;
            if (!PopulationSaveGame->TryQueuePendingReefMawReward(
                    PersistentSpawnId, Identity, bHasReefTooth)) return false;
        }
    }

    PopulationSaveGame->MarkDefeated(PersistentSpawnId);
    return UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
        KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
}

void AKalmalaGameMode::DeliverPendingWildlifeRewards()
{
    if (!HasAuthority() || GetWorld() == nullptr || PopulationSaveGame == nullptr
        || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return;

    const TArray<FKalmalaPendingWildlifeReward> PendingRewards = PopulationSaveGame->GetPendingWildlifeRewards();
    for (const FKalmalaPendingWildlifeReward& Reward : PendingRewards)
    {
        if (Reward.PersistentSpawnId.IsEmpty() || Reward.PlayerIdentity.IsEmpty()
            || DeliveredWildlifeRewardSpawnIds.Contains(Reward.PersistentSpawnId)) continue;

        FString BadgerSpawnId;
        const bool bBadgerReward = UKalmalaWorldPopulationSaveGame::TryGetBadgerSpawnIdFromPendingRewardClaimId(
            Reward.PersistentSpawnId, BadgerSpawnId);
        FString MoonBearSpawnId;
        const bool bMoonBearReward = UKalmalaWorldPopulationSaveGame::TryGetMoonBearSpawnIdFromPendingRewardClaimId(
            Reward.PersistentSpawnId, MoonBearSpawnId);
        FString ViperSpawnId;
        const bool bViperReward = UKalmalaWorldPopulationSaveGame::TryGetViperSpawnIdFromPendingRewardClaimId(
            Reward.PersistentSpawnId, ViperSpawnId);
        FString GiantSnakeSpawnId;
        const bool bGiantSnakeReward =
            UKalmalaWorldPopulationSaveGame::TryGetGiantSnakeSpawnIdFromPendingRewardClaimId(
                Reward.PersistentSpawnId, GiantSnakeSpawnId);
        FString DeathAdderSpawnId;
        const bool bDeathAdderReward =
            UKalmalaWorldPopulationSaveGame::TryGetDeathAdderSpawnIdFromPendingRewardClaimId(
                Reward.PersistentSpawnId, DeathAdderSpawnId);
        FString PeatHuskSpawnId;
        bool bPeatHuskHasHeart = false;
        const bool bPeatHuskReward =
            UKalmalaWorldPopulationSaveGame::TryGetPeatHuskSpawnIdFromPendingRewardClaimId(
                Reward.PersistentSpawnId, PeatHuskSpawnId, bPeatHuskHasHeart);
        FString WolfSpawnId;
        bool bWolfHasFang = false;
        const bool bWolfReward = UKalmalaWorldPopulationSaveGame::TryGetWolfSpawnIdFromPendingRewardClaimId(
            Reward.PersistentSpawnId, WolfSpawnId, bWolfHasFang);
        FString LynxSpawnId;
        bool bLynxHasPelt = false;
        const bool bLynxReward = UKalmalaWorldPopulationSaveGame::TryGetLynxSpawnIdFromPendingRewardClaimId(
            Reward.PersistentSpawnId, LynxSpawnId, bLynxHasPelt);
        FString PolarBearSpawnId;
        const bool bPolarBearReward =
            UKalmalaWorldPopulationSaveGame::TryGetPolarBearSpawnIdFromPendingRewardClaimId(
                Reward.PersistentSpawnId, PolarBearSpawnId);
        FString HornedGoatSpawnId;
        const bool bHornedGoatReward =
            UKalmalaWorldPopulationSaveGame::TryGetHornedGoatSpawnIdFromPendingRewardClaimId(
                Reward.PersistentSpawnId, HornedGoatSpawnId);
        FString ThunderSentinelSpawnId;
        bool bThunderSentinelHasCore = false;
        const bool bThunderSentinelReward =
            UKalmalaWorldPopulationSaveGame::TryGetThunderSentinelSpawnIdFromPendingRewardClaimId(
                Reward.PersistentSpawnId, ThunderSentinelSpawnId, bThunderSentinelHasCore);
        FString ReefMawSpawnId;
        bool bReefMawHasTooth = false;
        const bool bReefMawReward = UKalmalaWorldPopulationSaveGame::TryGetReefMawSpawnIdFromPendingRewardClaimId(
            Reward.PersistentSpawnId, ReefMawSpawnId, bReefMawHasTooth);
        const bool bMirelingReward = Reward.PersistentSpawnId.StartsWith(TEXT("mireling-gem|"));
        FString DefeatedSpawnId = bMirelingReward ? Reward.PersistentSpawnId.RightChop(12) : Reward.PersistentSpawnId;
        if (bBadgerReward) DefeatedSpawnId = BadgerSpawnId;
        else if (bMoonBearReward) DefeatedSpawnId = MoonBearSpawnId;
        else if (bViperReward) DefeatedSpawnId = ViperSpawnId;
        else if (bGiantSnakeReward) DefeatedSpawnId = GiantSnakeSpawnId;
        else if (bDeathAdderReward) DefeatedSpawnId = DeathAdderSpawnId;
        else if (bPeatHuskReward) DefeatedSpawnId = PeatHuskSpawnId;
        else if (bWolfReward) DefeatedSpawnId = WolfSpawnId;
        else if (bLynxReward) DefeatedSpawnId = LynxSpawnId;
        else if (bPolarBearReward) DefeatedSpawnId = PolarBearSpawnId;
        else if (bHornedGoatReward) DefeatedSpawnId = HornedGoatSpawnId;
        else if (bThunderSentinelReward) DefeatedSpawnId = ThunderSentinelSpawnId;
        else if (bReefMawReward) DefeatedSpawnId = ReefMawSpawnId;
        if (!PopulationSaveGame->IsDefeated(DefeatedSpawnId)) continue;

        for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            APlayerController* Controller = Iterator->Get();
            AKalmalaCharacter* Character = Controller != nullptr ? Cast<AKalmalaCharacter>(Controller->GetPawn()) : nullptr;
            if (!IsValid(Character) || !Character->HasAuthority() || Character->GetPlayerState() == nullptr
                || KalmalaGameMode::GetAuthenticatedPlayerIdentity(Character->GetPlayerState()) != Reward.PlayerIdentity)
                continue;

            UKalmalaInventoryComponent* Inventory = Character->GetInventoryComponent();
            if (Inventory == nullptr) break;

            const TArray<FKalmalaInventoryStack> Before = Inventory->GetStacks();
            TArray<FKalmalaInventoryStack> Candidate;
            FString Reason;
            if (bMirelingReward) Candidate = Before; // Mirelings have no ordinary material reward.
            else if (bPeatHuskReward)
            {
                TArray<FKalmalaInventoryStack> WithAmber;
                if (!UKalmalaInventoryComponent::BuildGrant(Before, TEXT("Amber"), 1, WithAmber, Reason)) break;
                if (bPeatHuskHasHeart)
                {
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithAmber, TEXT("PeatHeart"), 1, Candidate, Reason)) break;
                }
                else
                {
                    Candidate = MoveTemp(WithAmber);
                }
            }
            else if (bThunderSentinelReward)
            {
                TArray<FKalmalaInventoryStack> WithStone;
                if (!UKalmalaInventoryComponent::BuildGrant(Before, TEXT("Stone"),
                        AKalmalaWildlifeSpawn::GetStoneRewardQuantityForArchetype(
                            EKalmalaWildlifeArchetype::ThunderSentinel), WithStone, Reason)) break;
                if (bThunderSentinelHasCore)
                {
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithStone, TEXT("ThunderCore"), 1, Candidate, Reason)) break;
                }
                else
                {
                    Candidate = MoveTemp(WithStone);
                }
            }
            else if (bReefMawReward)
            {
                TArray<FKalmalaInventoryStack> WithBone;
                if (!UKalmalaInventoryComponent::BuildGrant(Before, TEXT("BoneFragment"),
                        AKalmalaWildlifeSpawn::GetBoneFragmentRewardQuantity(
                            EKalmalaWildlifeArchetype::ReefMaw), WithBone, Reason)) break;
                if (bReefMawHasTooth)
                {
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("ReefTooth"), 1, Candidate, Reason)) break;
                }
                else
                {
                    Candidate = MoveTemp(WithBone);
                }
            }
            else
            {
                TArray<FKalmalaInventoryStack> WithBone;
                TArray<FKalmalaInventoryStack> WithBadgerFur;
                const int32 BoneFragmentQuantity = (bMoonBearReward || bPolarBearReward || bReefMawReward) ? 3
                    : ((bGiantSnakeReward || bWolfReward || bLynxReward || bHornedGoatReward) ? 2 : 1);
                if (!UKalmalaInventoryComponent::BuildGrant(
                        Before, TEXT("BoneFragment"), BoneFragmentQuantity, WithBone, Reason)) break;
                if (bBadgerReward)
                {
                    TArray<FKalmalaInventoryStack> WithBadgerMeat;
                    if (!UKalmalaInventoryComponent::BuildGrant(WithBone, TEXT("BadgerFur"), 1, WithBadgerFur, Reason)
                        || !UKalmalaInventoryComponent::BuildGrant(
                            WithBadgerFur, TEXT("BadgerMeat"), 1, WithBadgerMeat, Reason)) break;
                    Candidate = MoveTemp(WithBadgerMeat);
                }
                else if (bViperReward)
                {
                    TArray<FKalmalaInventoryStack> WithViperFang;
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("ViperFang"), 1, WithViperFang, Reason)) break;
                    Candidate = MoveTemp(WithViperFang);
                }
                else if (bGiantSnakeReward)
                {
                    TArray<FKalmalaInventoryStack> WithGiantScale;
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("GiantScale"), 2, WithGiantScale, Reason)) break;
                    Candidate = MoveTemp(WithGiantScale);
                }
                else if (bDeathAdderReward)
                {
                    TArray<FKalmalaInventoryStack> WithVenomSac;
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("VenomSac"), 1, WithVenomSac, Reason)) break;
                    Candidate = MoveTemp(WithVenomSac);
                }
                else if (bWolfReward)
                {
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("WolfPelt"), 1, Candidate, Reason)) break;
                    TArray<FKalmalaInventoryStack> WithWolfMeat;
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            Candidate, TEXT("WolfMeat"), 1, WithWolfMeat, Reason)) break;
                    Candidate = MoveTemp(WithWolfMeat);
                    if (bWolfHasFang)
                    {
                        TArray<FKalmalaInventoryStack> WithWolfFang;
                        if (!UKalmalaInventoryComponent::BuildGrant(
                                Candidate, TEXT("WolfFang"), 1, WithWolfFang, Reason)) break;
                        Candidate = MoveTemp(WithWolfFang);
                    }
                }
                else if (bLynxReward && bLynxHasPelt)
                {
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("LynxPelt"), 1, Candidate, Reason)) break;
                }
                else if (bPolarBearReward)
                {
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("PolarBearPelt"), 2, Candidate, Reason)) break;
                }
                else if (bHornedGoatReward)
                {
                    if (!UKalmalaInventoryComponent::BuildGrant(
                            WithBone, TEXT("GoatHorn"), 1, Candidate, Reason)) break;
                }
                else if (bMoonBearReward
                    || bLynxReward || bMirelingReward)
                {
                    Candidate = MoveTemp(WithBone);
                }
                else if (!UKalmalaInventoryComponent::BuildGrant(WithBone, TEXT("RatTail"), 1, Candidate, Reason))
                {
                    break;
                }
            }
            if (!Reward.GemItemId.IsNone())
            {
                TArray<FKalmalaInventoryStack> WithGem;
                if (!UKalmalaInventoryComponent::BuildGrant(Candidate, Reward.GemItemId, 1, WithGem, Reason)) break;
                Candidate = MoveTemp(WithGem);
            }
            if (!Inventory->CanFitContents(Candidate, Inventory->CountPackedTools(Character->GetCarriedToolInventory()))) break;
            // A9 successful gem claims must be removed durably before inventory publication.
            // The game thread cannot service a competing request during this synchronous transaction.
            if (!Reward.GemItemId.IsNone())
            {
                PopulationSaveGame->RemovePendingWildlifeReward(Reward.PersistentSpawnId);
                const auto RestoreClaim = [&]()
                {
                    PopulationSaveGame->TryQueuePendingWildlifeReward(Reward.PersistentSpawnId, Reward.PlayerIdentity);
                    PopulationSaveGame->SetPendingGemReward(Reward.PersistentSpawnId, Reward.GemItemId);
                };
                if (!UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
                    KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0)) { RestoreClaim(); break; }
                if (!Inventory->TryCommitStacksFromServer(Before, Candidate))
                {
                    RestoreClaim();
                    UGameplayStatics::SaveGameToSlot(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
                    break;
                }
                DeliveredWildlifeRewardSpawnIds.Add(Reward.PersistentSpawnId);
                break;
            }
            if (!Inventory->TryCommitStacksFromServer(Before, Candidate)) break;

            // The reward claim ID is durable; keep a session guard if cleanup fails after the
            // inventory has accepted the complete reward.
            DeliveredWildlifeRewardSpawnIds.Add(Reward.PersistentSpawnId);
            PopulationSaveGame->RemovePendingWildlifeReward(Reward.PersistentSpawnId);
            const bool bSaved = UGameplayStatics::SaveGameToSlot(PopulationSaveGame,
                KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0);
            if (!bSaved)
            {
                UE_LOG(LogTemp, Warning, TEXT("Wildlife reward %s was granted but its pending-reward cleanup did not reach disk."),
                    *DefeatedSpawnId);
            }
            break;
        }
    }
}

void AKalmalaGameMode::RecordM9ResourceDepleted(const FString& StableResourceId)
{
    if (HasAuthority() && FKalmalaM7SparseDelta::IsValidStableId(StableResourceId)
        && StableResourceId.StartsWith(TEXT("resource:m9:v1:")))
    {
        SessionM9ResourceDepletionIds.Add(StableResourceId);
    }
}

void AKalmalaGameMode::RefreshOceanDiscoveries()
{
    if (!HasAuthority() || GetWorld() == nullptr || !WorldGenerationConfig.IsValid())
    {
        return;
    }

    constexpr int32 NeighborhoodRadius = 1;
    constexpr int32 MaxActiveKeys = 18; // Nine nearby cells for each prototype player.
    TSet<FIntPoint> DesiredKeys;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        const APlayerController* Controller = Iterator->Get();
        const APawn* Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
        if (Pawn == nullptr)
        {
            continue;
        }

        const FIntPoint CenterKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(Pawn->GetActorLocation()));
        for (int32 Y = -NeighborhoodRadius; Y <= NeighborhoodRadius && DesiredKeys.Num() < MaxActiveKeys; ++Y)
        {
            for (int32 X = -NeighborhoodRadius; X <= NeighborhoodRadius && DesiredKeys.Num() < MaxActiveKeys; ++X)
            {
                DesiredKeys.Add(CenterKey + FIntPoint(X, Y));
            }
        }

        if (DesiredKeys.Num() >= MaxActiveKeys)
        {
            break;
        }
    }

    for (auto It = ActiveOceanDiscoveryActors.CreateIterator(); It; ++It)
    {
        if (DesiredKeys.Contains(It.Key()))
        {
            continue;
        }

        for (const TWeakObjectPtr<AKalmalaDiscoveryActor>& WeakActor : It.Value())
        {
            if (AKalmalaDiscoveryActor* Actor = WeakActor.Get())
            {
                Actor->Destroy();
            }
        }
        It.RemoveCurrent();
    }

    for (const FIntPoint& SpatialKey : DesiredKeys)
    {
        if (ActiveOceanDiscoveryActors.Contains(SpatialKey))
        {
            continue;
        }

        TArray<TWeakObjectPtr<AKalmalaDiscoveryActor>> SpawnedActors;
        for (const FKalmalaOceanDiscoveryDescriptor& Descriptor : FKalmalaOceanDiscoveryCatalogue::BuildDescriptors(WorldGenerationConfig, SpatialKey))
        {
            FActorSpawnParameters Parameters;
            Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaDiscoveryActor* Actor = GetWorld()->SpawnActor<AKalmalaDiscoveryActor>(
                AKalmalaDiscoveryActor::StaticClass(), Descriptor.Location, FRotator::ZeroRotator, Parameters);
            if (Actor != nullptr && Actor->InitializeOceanServer(Descriptor))
            {
                SpawnedActors.Add(Actor);
            }
            else if (Actor != nullptr)
            {
                Actor->Destroy();
            }
        }

        ActiveOceanDiscoveryActors.Add(SpatialKey, MoveTemp(SpawnedActors));
    }
}

bool AKalmalaGameMode::IsCurrentM9ClaimId(const FString& ClaimId) const
{
    if (!UKalmalaPlayerDiscoverySaveGameV2::IsValidM9ClaimId(ClaimId)) return false;

    const FString Prefix = FString::Printf(TEXT("land-discovery:m9:%d:"),
        FKalmalaM9ExplorationRewardCatalogue::StableIdentityVersion);
    FString CandidateText;
    FString CoordinateText;
    if (!ClaimId.StartsWith(Prefix, ESearchCase::CaseSensitive)
        || !ClaimId.RightChop(Prefix.Len()).Split(TEXT(":"), &CandidateText, &CoordinateText)) return false;

    FString XText;
    FString YText;
    int32 KeyX = 0;
    int32 KeyY = 0;
    if (CandidateText.IsEmpty() || FName(*CandidateText).ToString() != CandidateText
        || !CoordinateText.Split(TEXT(","), &XText, &YText) || YText.Contains(TEXT(","))
        || !LexTryParseString(KeyX, *XText) || !LexTryParseString(KeyY, *YText)
        || FString::FromInt(KeyX) != XText || FString::FromInt(KeyY) != YText) return false;

    const FName CandidateId(*CandidateText);
    const FIntPoint SpatialKey(KeyX, KeyY);
    if (FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(CandidateId, SpatialKey) != ClaimId) return false;

    const TArray<FKalmalaM9ExplorationRewardDescriptor> Expected =
        FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(WorldGenerationConfig, SpatialKey);
    return Expected.ContainsByPredicate([this, &ClaimId](const FKalmalaM9ExplorationRewardDescriptor& Descriptor)
    {
        return FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
                Descriptor.CandidateId, Descriptor.SpatialKey) == ClaimId
            && FKalmalaM9ExplorationRewardCatalogue::IsCurrentDescriptor(WorldGenerationConfig, Descriptor);
    });
}

UKalmalaPlayerDiscoverySaveGameV2* AKalmalaGameMode::GetPlayerDiscoverySave(
    AKalmalaCharacter* Interactor, FString& OutIdentity)
{
    OutIdentity.Reset();
    if (!HasAuthority() || Interactor == nullptr || Interactor->GetWorld() != GetWorld()
        || Interactor->GetPlayerState() == nullptr) return nullptr;

    const AKalmalaWorldGenerationGameState* WorldState = GetGameState<AKalmalaWorldGenerationGameState>();
    if (WorldState == nullptr || !WorldState->GetWorldGenerationConfig().IsValid()) return nullptr;
    // PostLogin can run before GameMode::BeginPlay has copied the final server seed.
    // Bind player slots to the authoritative GameState identity before loading or caching them.
    WorldGenerationConfig = WorldState->GetWorldGenerationConfig();

    OutIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Interactor->GetPlayerState());
    if (OutIdentity.IsEmpty() || OutIdentity.Len() > FKalmalaM7SaveIdentity::MaxOwnerIdentityLength
        || RejectedPlayerDiscoverySaveIdentities.Contains(OutIdentity)) return nullptr;
    if (TObjectPtr<UKalmalaPlayerDiscoverySaveGameV2>* Existing = PlayerDiscoverySaves.Find(OutIdentity))
    {
        if (*Existing != nullptr && (*Existing)->MatchesPlayer(WorldGenerationConfig, OutIdentity)) return *Existing;
        RejectedPlayerDiscoverySaveIdentities.Add(OutIdentity);
        UE_LOG(LogTemp, Warning,
            TEXT("Player discovery restore and writes blocked for authenticated player %s; cached save identity no longer matches the authoritative world."),
            *OutIdentity);
        return nullptr;
    }

    const FString Slot = KalmalaGameMode::PlayerDiscoverySaveSlot(WorldGenerationConfig, OutIdentity);
    UKalmalaPlayerDiscoverySaveGameV2* Save = nullptr;
    if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
    {
        USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, 0);
        if (UKalmalaPlayerDiscoverySaveGameV2* Current = Cast<UKalmalaPlayerDiscoverySaveGameV2>(Loaded))
        {
            bool bValidClaims = Current->MatchesPlayer(WorldGenerationConfig, OutIdentity);
            if (bValidClaims)
            {
                for (const FString& ClaimId : Current->GetM9ClaimIds())
                {
                    bValidClaims = IsCurrentM9ClaimId(ClaimId);
                    if (!bValidClaims) break;
                }
            }
            if (bValidClaims) Save = Current;
        }

        if (Save == nullptr)
        {
            RejectedPlayerDiscoverySaveIdentities.Add(OutIdentity);
            UE_LOG(LogTemp, Warning,
                TEXT("Player discovery restore and writes blocked for authenticated player %s; existing save is invalid or incompatible and was preserved."),
                *OutIdentity);
            return nullptr;
        }
    }
    else
    {
        Save = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(this);
        if (Save != nullptr) Save->InitializeForPlayer(WorldGenerationConfig, OutIdentity);
    }

    if (Save == nullptr || !Save->MatchesPlayer(WorldGenerationConfig, OutIdentity)) return nullptr;
    PlayerDiscoverySaves.Add(OutIdentity, Save);

    if (!Save->GetToolRecords().IsEmpty())
    {
        Interactor->CarriedTools.Reset();
        for (const FKalmalaPlayerToolSaveRecord& Record : Save->GetToolRecords())
        {
            FKalmalaToolState ToolState;
            ToolState.ItemId = Record.ItemId;
            ToolState.ToolLevel = Record.ToolLevel;
            ToolState.Durability = Record.Condition;
            ToolState.InstanceGuid = Record.InstanceGuid;
            ToolState.RemainingBurnSeconds = Record.RemainingBurnSeconds;
            Interactor->CarriedTools.Add(ToolState);
        }
        Interactor->ForceNetUpdate();
    }

    if (UKalmalaSupportMagicComponent* Support = Interactor->GetSupportMagicComponent())
    {
        for (const EKalmalaSupportEffect Effect : { EKalmalaSupportEffect::Mending,
            EKalmalaSupportEffect::HearthShield, EKalmalaSupportEffect::BearsVigor, EKalmalaSupportEffect::DeerCall })
        {
            if (Save->HasLearnedEffect(UKalmalaSupportMagicComponent::CanonicalId(Effect)))
            {
                Support->LearnEffectFromServer(Effect);
            }
        }
    }
    return Save;
}

bool AKalmalaGameMode::PersistPlayerStateFromServer(
    AKalmalaCharacter* Character,
    const FString& AdditionalDiscoveryId,
    const FString& AdditionalLearnedEffectId,
    const FString& AdditionalM9ClaimId)
{
    if (!HasAuthority() || Character == nullptr || !Character->HasAuthority()
        || Character->GetWorld() != GetWorld() || Character->GetPlayerState() == nullptr) return false;

    const TArray<FKalmalaToolState> ToolStates = Character->GetCarriedToolInventory();
    FString Identity;
    UKalmalaPlayerDiscoverySaveGameV2* Current = GetPlayerDiscoverySave(Character, Identity);
    if (Current == nullptr) return false;

    TArray<uint8> CurrentBytes;
    if (!UGameplayStatics::SaveGameToMemory(Current, CurrentBytes)) return false;
    UKalmalaPlayerDiscoverySaveGameV2* Candidate =
        Cast<UKalmalaPlayerDiscoverySaveGameV2>(UGameplayStatics::LoadGameFromMemory(CurrentBytes));
    if (Candidate == nullptr || !Candidate->MatchesPlayer(WorldGenerationConfig, Identity)) return false;

    TArray<FKalmalaPlayerToolSaveRecord> ToolRecords;
    ToolRecords.Reserve(ToolStates.Num());
    for (const FKalmalaToolState& ToolState : ToolStates)
    {
        FKalmalaPlayerToolSaveRecord& Record = ToolRecords.AddDefaulted_GetRef();
        Record.ItemId = ToolState.ItemId;
        Record.ToolLevel = ToolState.ToolLevel;
        Record.Condition = ToolState.Durability;
        Record.InstanceGuid = ToolState.InstanceGuid;
        Record.RemainingBurnSeconds = ToolState.RemainingBurnSeconds;
    }
    if (!Candidate->TryReplaceToolRecords(ToolRecords)) return false;

    for (const FString& ClaimId : Candidate->GetM9ClaimIds())
    {
        if (!IsCurrentM9ClaimId(ClaimId)) return false;
    }

    const TSet<FString>* SessionClaims = SessionM9ExplorationClaims.Find(Identity);
    if (SessionClaims != nullptr)
    {
        if (SessionClaims->Num() > KalmalaGameMode::MaxSessionM9ExplorationClaimsPerPlayer) return false;
        for (const FString& ClaimId : *SessionClaims)
        {
            if (!IsCurrentM9ClaimId(ClaimId)) return false;
            if (!Candidate->HasDiscovery(ClaimId) && !Candidate->AddM9Claim(ClaimId)) return false;
        }
    }

    if (!AdditionalDiscoveryId.IsEmpty()
        && (Candidate->HasDiscovery(AdditionalDiscoveryId) || !Candidate->AddDiscovery(AdditionalDiscoveryId))) return false;
    if (!AdditionalLearnedEffectId.IsEmpty()
        && !Candidate->EnsureLearnedEffect(AdditionalLearnedEffectId)) return false;
    if (!AdditionalM9ClaimId.IsEmpty()
        && (!IsCurrentM9ClaimId(AdditionalM9ClaimId) || Candidate->HasDiscovery(AdditionalM9ClaimId)
            || !Candidate->AddM9Claim(AdditionalM9ClaimId))) return false;

    if (!Candidate->MatchesPlayer(WorldGenerationConfig, Identity)) return false;
    const FString Slot = KalmalaGameMode::PlayerDiscoverySaveSlot(WorldGenerationConfig, Identity);
    if (!UGameplayStatics::SaveGameToSlot(Candidate, Slot, 0)) return false;

    PlayerDiscoverySaves.Add(Identity, Candidate);
    return true;
}

void AKalmalaGameMode::LoadOceanTravelWorldSave()
{
    bOceanTravelPersistenceWritable = HasAuthority() && WorldGenerationConfig.IsValid();
    OceanTravelWorldSave = nullptr;
    if (!bOceanTravelPersistenceWritable) return;

    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForWorld(WorldGenerationConfig.WorldSeed);
    const FString Slot = KalmalaGameMode::OceanTravelWorldSaveSlot(Identity);
    if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return;

    UKalmalaOceanTravelPersistenceSaveGame* Loaded =
        Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (Loaded == nullptr || !Loaded->Matches(Identity) || !Loaded->HasVesselState()
        || !Loaded->GetVesselState().IsValid())
    {
        bOceanTravelPersistenceWritable = false;
        UE_LOG(LogTemp, Warning, TEXT("Ocean travel restore blocked: existing world save is invalid or incompatible; preserved without overwrite."));
        return;
    }
    OceanTravelWorldSave = Loaded;
}

void AKalmalaGameMode::RestorePersistedOceanSkiff()
{
    if (!bOceanTravelPersistenceWritable || OceanTravelWorldSave == nullptr) return;
    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForWorld(WorldGenerationConfig.WorldSeed);
    if (!OceanTravelWorldSave->Matches(Identity) || !OceanTravelWorldSave->HasVesselState()) return;

    const FKalmalaOceanTravelVesselState& Vessel = OceanTravelWorldSave->GetVesselState();
    const FVector2D Center(Vessel.SafeLocation);
    if (!Vessel.IsValid() || FMath::Abs(Vessel.SafeLocation.Z) > 250.0
        || !AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(WorldGenerationConfig, Center, Vessel.YawDegrees))
    {
        bOceanTravelPersistenceWritable = false;
        UE_LOG(LogTemp, Warning, TEXT("Ocean travel restore blocked: saved vessel no longer has a valid generated-ocean hull footprint; save preserved."));
        return;
    }

    for (TActorIterator<AKalmalaOceanSkiff> It(GetWorld()); It; ++It)
    {
        if (It->GetPersistentVesselId() == Vessel.VesselId)
        {
            RestoredOceanSkiff = *It;
            return;
        }
    }

    FActorSpawnParameters Parameters;
    Parameters.Owner = nullptr;
    Parameters.Instigator = nullptr;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    const FRotator Rotation(0.0f, Vessel.YawDegrees, 0.0f);
    AKalmalaOceanSkiff* Skiff = GetWorld()->SpawnActor<AKalmalaOceanSkiff>(
        Vessel.SafeLocation, Rotation, Parameters);
    if (Skiff == nullptr || !Skiff->InitializePersistentIdentityFromServer(Vessel.VesselId))
    {
        if (Skiff != nullptr) Skiff->Destroy();
        UE_LOG(LogTemp, Warning, TEXT("Ocean travel restore blocked: saved moored vessel could not be spawned without overlap; save preserved."));
        return;
    }

    RestoredOceanSkiff = Skiff;
    UE_LOG(LogTemp, Display, TEXT("Restored moored ocean skiff %s at %s after generated-ocean validation."),
        *Vessel.VesselId, *Vessel.SafeLocation.ToCompactString());
}

void AKalmalaGameMode::LoadOceanDiscoveryLedger(AKalmalaCharacter* Interactor)
{
    if (!HasAuthority() || Interactor == nullptr || Interactor->GetWorld() != GetWorld()) return;
    const FString PlayerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Interactor->GetPlayerState());
    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForPlayer(
        WorldGenerationConfig.WorldSeed, PlayerIdentity);
    if (!Identity.IsValid() || OceanDiscoverySaves.Contains(PlayerIdentity)) return;

    const FString Slot = KalmalaGameMode::OceanDiscoverySaveSlot(Identity);
    if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return;
    UKalmalaM7PersistenceSaveGame* Loaded =
        Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (Loaded == nullptr || !Loaded->Matches(Identity))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ocean discovery ledger for authenticated player %s is invalid or incompatible; preserved without overwrite."),
            *PlayerIdentity);
        return;
    }
    OceanDiscoverySaves.Add(PlayerIdentity, Loaded);
}

bool AKalmalaGameMode::PersistOceanTravelPassenger(AKalmalaCharacter* Interactor,
    const FString& VesselId, const EKalmalaOceanSkiffSeat Seat)
{
    if (!bOceanTravelPersistenceWritable || !IsValid(Interactor) || !Interactor->HasAuthority()
        || !FKalmalaOceanTravelVesselState::IsValidVesselId(VesselId)
        || (Seat != EKalmalaOceanSkiffSeat::Helm && Seat != EKalmalaOceanSkiffSeat::Passenger))
    {
        return false;
    }

    const FString PlayerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Interactor->GetPlayerState());
    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForPlayer(
        WorldGenerationConfig.WorldSeed, PlayerIdentity);
    if (!Identity.IsValid()) return false;
    const FString Slot = KalmalaGameMode::OceanTravelPlayerSaveSlot(Identity);

    UKalmalaOceanTravelPersistenceSaveGame* Existing = nullptr;
    if (TObjectPtr<UKalmalaOceanTravelPersistenceSaveGame>* Cached = OceanTravelPlayerSaves.Find(PlayerIdentity))
    {
        Existing = *Cached;
        if (Existing == nullptr || !Existing->Matches(Identity)) return false;
    }
    else if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
    {
        Existing = Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
        if (Existing == nullptr || !Existing->Matches(Identity))
        {
            UE_LOG(LogTemp, Warning, TEXT("Ocean travel passenger save for %s is invalid or incompatible; preserved without overwrite."),
                *PlayerIdentity);
            return false;
        }
    }

    UKalmalaOceanTravelPersistenceSaveGame* Candidate = nullptr;
    if (Existing == nullptr)
    {
        Candidate = NewObject<UKalmalaOceanTravelPersistenceSaveGame>(this);
        Candidate->Initialize(Identity);
    }
    else
    {
        TArray<uint8> Bytes;
        if (!UGameplayStatics::SaveGameToMemory(Existing, Bytes)) return false;
        Candidate = Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    }

    FKalmalaOceanTravelPassengerState Passenger;
    Passenger.VesselId = VesselId;
    Passenger.Seat = Seat == EKalmalaOceanSkiffSeat::Helm
        ? EKalmalaOceanTravelSavedSeat::Helm : EKalmalaOceanTravelSavedSeat::Passenger;
    if (Candidate == nullptr || !Candidate->GetIdentity().Matches(Identity)
        || !Candidate->SetPassengerState(Passenger)
        || !Candidate->Matches(Identity)
        || !UGameplayStatics::SaveGameToSlot(Candidate, Slot, 0))
    {
        return false;
    }

    OceanTravelPlayerSaves.Add(PlayerIdentity, Candidate);
    return true;
}

bool AKalmalaGameMode::PersistOceanTravelState(AKalmalaOceanSkiff* Skiff)
{
    if (!bOceanTravelPersistenceWritable || !HasAuthority() || !IsValid(Skiff)
        || !Skiff->HasAuthority() || Skiff->GetMode() != EKalmalaOceanSkiffMode::Moored
        || !WorldGenerationConfig.IsValid())
    {
        return false;
    }

    FKalmalaOceanTravelVesselState Vessel;
    Vessel.VesselId = Skiff->GetPersistentVesselId();
    Vessel.SafeLocation = Skiff->GetActorLocation();
    Vessel.YawDegrees = Skiff->GetActorRotation().Yaw;
    if (!Vessel.IsValid() || FMath::Abs(Vessel.SafeLocation.Z) > 250.0
        || !AKalmalaOceanSkiff::HasNavigableOceanFootprintForConfig(
            WorldGenerationConfig, FVector2D(Vessel.SafeLocation), Vessel.YawDegrees))
    {
        return false;
    }

    const FKalmalaM7SaveIdentity WorldIdentity = FKalmalaM7SaveIdentity::ForWorld(WorldGenerationConfig.WorldSeed);
    const FString WorldSlot = KalmalaGameMode::OceanTravelWorldSaveSlot(WorldIdentity);
    UKalmalaOceanTravelPersistenceSaveGame* CandidateWorld = nullptr;
    if (OceanTravelWorldSave == nullptr)
    {
        CandidateWorld = NewObject<UKalmalaOceanTravelPersistenceSaveGame>(this);
        CandidateWorld->Initialize(WorldIdentity);
    }
    else
    {
        TArray<uint8> Bytes;
        if (!OceanTravelWorldSave->Matches(WorldIdentity)
            || !UGameplayStatics::SaveGameToMemory(OceanTravelWorldSave, Bytes)) return false;
        CandidateWorld = Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    }
    if (CandidateWorld == nullptr || !CandidateWorld->GetIdentity().Matches(WorldIdentity)
        || !CandidateWorld->SetVesselState(Vessel)
        || !CandidateWorld->Matches(WorldIdentity)
        || !UGameplayStatics::SaveGameToSlot(CandidateWorld, WorldSlot, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not persist moored ocean skiff %s; existing world state remains authoritative."), *Vessel.VesselId);
        return false;
    }

    OceanTravelWorldSave = CandidateWorld;
    RestoredOceanSkiff = Skiff;
    bool bPassengersPersisted = true;
    if (AKalmalaCharacter* Helm = Skiff->GetHelmOccupant())
    {
        bPassengersPersisted &= PersistOceanTravelPassenger(Helm, Vessel.VesselId, EKalmalaOceanSkiffSeat::Helm);
    }
    if (AKalmalaCharacter* Passenger = Skiff->GetPassengerOccupant())
    {
        bPassengersPersisted &= PersistOceanTravelPassenger(Passenger, Vessel.VesselId, EKalmalaOceanSkiffSeat::Passenger);
    }
    return bPassengersPersisted;
}

void AKalmalaGameMode::OnOceanSkiffStateChanged(AKalmalaOceanSkiff* Skiff)
{
    if (!PersistOceanTravelState(Skiff))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ocean skiff state change was not persisted; session state remains server-authoritative."));
    }
}

bool AKalmalaGameMode::ClearOceanTravelPassenger(AKalmalaCharacter* Interactor, const FString& VesselId)
{
    if (!HasAuthority() || !IsValid(Interactor) || !Interactor->HasAuthority()
        || !FKalmalaOceanTravelVesselState::IsValidVesselId(VesselId)) return false;
    const FString PlayerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Interactor->GetPlayerState());
    const FKalmalaM7SaveIdentity Identity = FKalmalaM7SaveIdentity::ForPlayer(
        WorldGenerationConfig.WorldSeed, PlayerIdentity);
    if (!Identity.IsValid()) return true;
    const FString Slot = KalmalaGameMode::OceanTravelPlayerSaveSlot(Identity);
    if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
    {
        OceanTravelPlayerSaves.Remove(PlayerIdentity);
        return true;
    }
    if (!bOceanTravelPersistenceWritable)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not clear ocean travel seat association for %s while the world save is unavailable."), *PlayerIdentity);
        return false;
    }
    UKalmalaOceanTravelPersistenceSaveGame* Existing =
        Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (Existing == nullptr || !Existing->Matches(Identity) || !Existing->HasPassengerState()
        || Existing->GetPassengerState().VesselId != VesselId)
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not clear ocean travel seat association for %s because the record is incompatible or references another vessel."), *PlayerIdentity);
        return false;
    }
    if (!UGameplayStatics::DeleteGameInSlot(Slot, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not clear ocean travel seat association for %s; disembarkation rejected."), *PlayerIdentity);
        return false;
    }
    OceanTravelPlayerSaves.Remove(PlayerIdentity);
    return true;
}

void AKalmalaGameMode::RestoreOceanTravelForPlayer(APlayerController* PlayerController)
{
    if (!HasAuthority() || !IsValid(PlayerController) || !bOceanTravelPersistenceWritable) return;
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PlayerController->GetPawn());
    if (Character == nullptr || Character->GetPlayerState() == nullptr) return;
    const FString PlayerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Character->GetPlayerState());
    const FKalmalaM7SaveIdentity PlayerIdentityScope = FKalmalaM7SaveIdentity::ForPlayer(
        WorldGenerationConfig.WorldSeed, PlayerIdentity);
    if (!PlayerIdentityScope.IsValid()) return;

    const FString Slot = KalmalaGameMode::OceanTravelPlayerSaveSlot(PlayerIdentityScope);
    UKalmalaOceanTravelPersistenceSaveGame* PlayerSave = nullptr;
    if (TObjectPtr<UKalmalaOceanTravelPersistenceSaveGame>* Cached = OceanTravelPlayerSaves.Find(PlayerIdentity))
    {
        PlayerSave = *Cached;
    }
    else if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
    {
        PlayerSave = Cast<UKalmalaOceanTravelPersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    }
    if (PlayerSave == nullptr || !PlayerSave->Matches(PlayerIdentityScope) || !PlayerSave->HasPassengerState()
        || OceanTravelWorldSave == nullptr || !PlayerSave->CanPairWithWorldSave(*OceanTravelWorldSave))
    {
        return;
    }

    const FKalmalaOceanTravelPassengerState& SavedPassenger = PlayerSave->GetPassengerState();
    AKalmalaOceanSkiff* Skiff = RestoredOceanSkiff.Get();
    if (Skiff == nullptr)
    {
        for (TActorIterator<AKalmalaOceanSkiff> It(GetWorld()); It; ++It)
        {
            if (It->GetPersistentVesselId() == SavedPassenger.VesselId)
            {
                Skiff = *It;
                break;
            }
        }
    }
    if (Skiff == nullptr || Skiff->GetPersistentVesselId() != SavedPassenger.VesselId) return;

    const EKalmalaOceanSkiffSeat Seat = SavedPassenger.Seat == EKalmalaOceanTravelSavedSeat::Helm
        ? EKalmalaOceanSkiffSeat::Helm : EKalmalaOceanSkiffSeat::Passenger;
    const bool bAlreadyOccupiesSavedSeat = Seat == EKalmalaOceanSkiffSeat::Helm
        ? Skiff->GetHelmOccupant() == Character : Skiff->GetPassengerOccupant() == Character;
    if (bAlreadyOccupiesSavedSeat && Character->GetAttachParentActor() == Skiff)
    {
        OceanTravelPlayerSaves.Add(PlayerIdentity, PlayerSave);
        return;
    }
    if (!Skiff->RestoreSeatFromServer(Character, Seat))
    {
        UE_LOG(LogTemp, Warning, TEXT("Ocean travel seat restore rejected for player %s because the saved seat is occupied or the player is already attached."),
            *PlayerIdentity);
        return;
    }
    OceanTravelPlayerSaves.Add(PlayerIdentity, PlayerSave);
    UE_LOG(LogTemp, Display, TEXT("Restored authenticated player %s to ocean skiff %s seat %s."), *PlayerIdentity,
        *SavedPassenger.VesselId, Seat == EKalmalaOceanSkiffSeat::Helm ? TEXT("Helm") : TEXT("Passenger"));
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
    UKalmalaPlayerDiscoverySaveGameV2* Save = GetPlayerDiscoverySave(Interactor, Identity);
    if (Save == nullptr || Feedback == nullptr) return false;
    const FString Id = FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Descriptor);
    if (Save->HasDiscovery(Id))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::AlreadyFound, TEXT("Already discovered"));
        return false;
    }
    const EKalmalaSupportEffect Effect = Descriptor.Kind == EKalmalaWorldDiscoveryKind::Scroll ? UKalmalaSupportMagicComponent::FromScrollDefinition(Descriptor.DefinitionId) : EKalmalaSupportEffect::None;
    const FString EffectId = Effect != EKalmalaSupportEffect::None
        ? UKalmalaSupportMagicComponent::CanonicalId(Effect) : FString();
    if (!PersistPlayerStateFromServer(Interactor, Id, EffectId))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Discovery unavailable"));
        return false;
    }
    if (Effect != EKalmalaSupportEffect::None) Interactor->GetSupportMagicComponent()->LearnEffectFromServer(Effect);
    Feedback->PublishFeedbackFromServer(Descriptor.Kind == EKalmalaWorldDiscoveryKind::Scroll ? EKalmalaDiscoveryFeedback::ScrollFound : EKalmalaDiscoveryFeedback::LandmarkFound,
        Descriptor.Kind == EKalmalaWorldDiscoveryKind::Scroll ? FString::Printf(TEXT("Scroll found: %s"), *Descriptor.DefinitionId) : TEXT("Landmark discovered"));
    return true;
}

bool AKalmalaGameMode::ClaimOceanDiscovery(AKalmalaCharacter* Interactor, const FKalmalaOceanDiscoveryDescriptor& Descriptor)
{
    if (!HasAuthority() || Interactor == nullptr || !Interactor->HasAuthority() || Interactor->GetWorld() != GetWorld()
        || !FKalmalaOceanDiscoveryCatalogue::IsCurrentDescriptor(WorldGenerationConfig, Descriptor)
        || FVector::DistSquared(Interactor->GetActorLocation(), Descriptor.Location) > FMath::Square(250.0f))
    {
        return false;
    }

    const FKalmalaOceanDiscoveryDefinition* Definition = FKalmalaOceanDiscoveryCatalogue::FindDefinition(Descriptor.DiscoveryId);
    UKalmalaInventoryComponent* Inventory = Interactor->GetInventoryComponent();
    UKalmalaDiscoveryProgressComponent* Feedback = Interactor->GetDiscoveryProgressComponent();
    const APlayerState* PlayerState = Interactor->GetPlayerState();
    FString PlayerIdentity;
    if (PlayerState != nullptr)
    {
        const FUniqueNetIdRepl UniqueId = PlayerState->GetUniqueId();
        if (UniqueId.IsValid())
        {
            const TSharedPtr<const FUniqueNetId> AuthenticatedId = UniqueId.GetUniqueNetId();
            if (AuthenticatedId.IsValid())
            {
                PlayerIdentity = AuthenticatedId->GetType().ToString() + TEXT(":") + AuthenticatedId->ToString();
            }
        }
    }
    const FKalmalaM7SaveIdentity SaveIdentity = FKalmalaM7SaveIdentity::ForPlayer(WorldGenerationConfig.WorldSeed, PlayerIdentity);
    const FString SaveSlot = KalmalaGameMode::OceanDiscoverySaveSlot(SaveIdentity);
    if (Definition == nullptr || Inventory == nullptr || Feedback == nullptr || !SaveIdentity.IsValid()
        || !FKalmalaOceanDiscoveryCatalogue::IsValidDefinition(*Definition, UKalmalaItemCatalogue::Get()))
    {
        if (Feedback != nullptr)
        {
            Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery unavailable"));
        }
        return false;
    }

    UKalmalaM7PersistenceSaveGame* Save = nullptr;
    if (TObjectPtr<UKalmalaM7PersistenceSaveGame>* Existing = OceanDiscoverySaves.Find(PlayerIdentity))
    {
        Save = *Existing;
        if (Save == nullptr || !Save->Matches(SaveIdentity))
        {
            Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery save unavailable"));
            return false;
        }
    }
    else if (UGameplayStatics::DoesSaveGameExist(SaveSlot, 0))
    {
        Save = Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot, 0));
        if (Save == nullptr || !Save->Matches(SaveIdentity))
        {
            Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery save uses an unsupported version"));
            return false;
        }
    }
    else
    {
        Save = NewObject<UKalmalaM7PersistenceSaveGame>(this);
        Save->Initialize(SaveIdentity);
    }

    if (Save == nullptr || !Save->Matches(SaveIdentity))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery save unavailable"));
        return false;
    }

    const FString DiscoveryIdentity = FKalmalaOceanDiscoveryCatalogue::MakeStableIdentity(Descriptor.DiscoveryId, Descriptor.SpatialKey);
    if (Save->HasSparseDelta(EKalmalaM7SparseDeltaKind::DiscoveryClaimed, DiscoveryIdentity))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::AlreadyFound, TEXT("Sea discovery already claimed"));
        return false;
    }

    const TArray<FKalmalaInventoryStack> ExistingStacks = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> CandidateStacks;
    FString GrantFailure;
    if (!UKalmalaInventoryComponent::BuildGrant(ExistingStacks, Definition->RewardItemId, Definition->RewardQuantity, CandidateStacks, GrantFailure))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Pack full; sea discovery remains"));
        return false;
    }

    TArray<uint8> SaveBytes;
    if (!UGameplayStatics::SaveGameToMemory(Save, SaveBytes))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery unavailable"));
        return false;
    }
    UKalmalaM7PersistenceSaveGame* CandidateSave = Cast<UKalmalaM7PersistenceSaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
    FKalmalaM7SparseDelta ClaimDelta;
    ClaimDelta.Kind = EKalmalaM7SparseDeltaKind::DiscoveryClaimed;
    ClaimDelta.StableId = DiscoveryIdentity;
    if (CandidateSave == nullptr || !CandidateSave->Matches(SaveIdentity) || !CandidateSave->AddSparseDelta(ClaimDelta))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery ledger is full or invalid"));
        return false;
    }

    if (!UGameplayStatics::SaveGameToSlot(CandidateSave, SaveSlot, 0))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery could not be saved"));
        return false;
    }

    if (!Inventory->TryCommitStacksFromServer(ExistingStacks, CandidateStacks))
    {
        const bool bLedgerRestored = UGameplayStatics::SaveGameToSlot(Save, SaveSlot, 0);
        if (!bLedgerRestored)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to restore the prior sea-discovery ledger for player %s after reward rejection."), *PlayerIdentity);
        }
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Sea discovery reward could not be granted"));
        return false;
    }

    OceanDiscoverySaves.Add(PlayerIdentity, CandidateSave);
    Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::LandmarkFound,
        FString::Printf(TEXT("Sea discovery: %s (+%d %s)"), *Definition->DisplayName, Definition->RewardQuantity, *Definition->RewardItemId.ToString()));
    return true;
}

bool AKalmalaGameMode::ClaimM9ExplorationReward(
    AKalmalaCharacter* Interactor, const FKalmalaM9ExplorationRewardDescriptor& Descriptor)
{
    if (!HasAuthority() || Interactor == nullptr || !Interactor->HasAuthority() || Interactor->GetWorld() != GetWorld()
        || !ActivePopulationSpatialKeys.Contains(Descriptor.SpatialKey)
        || !FKalmalaM9ExplorationRewardCatalogue::IsCurrentDescriptor(WorldGenerationConfig, Descriptor)
        || FVector::DistSquared(Interactor->GetActorLocation(), Descriptor.Location) > FMath::Square(250.0f))
    {
        return false;
    }

    UKalmalaInventoryComponent* Inventory = Interactor->GetInventoryComponent();
    UKalmalaDiscoveryProgressComponent* Feedback = Interactor->GetDiscoveryProgressComponent();
    const FKalmalaM9ExplorationRewardDefinition* Definition =
        FKalmalaM9ExplorationRewardCatalogue::FindDefinition(Descriptor.CandidateId);
    const FString PlayerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(Interactor->GetPlayerState());
    const FString StableIdentity = FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
        Descriptor.CandidateId, Descriptor.SpatialKey);
    if (Inventory == nullptr || Feedback == nullptr || PlayerIdentity.IsEmpty() || StableIdentity.IsEmpty()
        || Definition == nullptr
        || !FKalmalaM9ExplorationRewardCatalogue::IsValidDefinition(*Definition, UKalmalaItemCatalogue::Get()))
    {
        if (Feedback != nullptr)
        {
            Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Land discovery unavailable"));
        }
        return false;
    }

    FString SaveIdentity;
    UKalmalaPlayerDiscoverySaveGameV2* PlayerSave = GetPlayerDiscoverySave(Interactor, SaveIdentity);
    if (PlayerSave == nullptr || SaveIdentity != PlayerIdentity)
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Land discovery save unavailable"));
        return false;
    }

    TSet<FString>* ExistingClaims = SessionM9ExplorationClaims.Find(PlayerIdentity);
    if (PlayerSave->HasDiscovery(StableIdentity)
        || (ExistingClaims != nullptr && ExistingClaims->Contains(StableIdentity)))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::AlreadyFound, TEXT("Land discovery already claimed"));
        return false;
    }
    if ((ExistingClaims != nullptr && ExistingClaims->Num() >= KalmalaGameMode::MaxSessionM9ExplorationClaimsPerPlayer)
        || PlayerSave->GetM9ClaimIds().Num() >= UKalmalaPlayerDiscoverySaveGameV2::MaxM9Claims)
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Land discovery record is full"));
        return false;
    }

    const TArray<FKalmalaInventoryStack> ExistingStacks = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> CandidateStacks;
    FString GrantFailure;
    if (!UKalmalaInventoryComponent::BuildGrant(
        ExistingStacks, Definition->RewardItemId, Definition->RewardQuantity, CandidateStacks, GrantFailure))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Pack full; discovery remains"));
        return false;
    }

    if (!Inventory->TryCommitStacksFromServer(ExistingStacks, CandidateStacks))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Discovery reward could not be granted"));
        return false;
    }

    if (!PersistPlayerStateFromServer(Interactor, FString(), FString(), StableIdentity))
    {
        if (!Inventory->TryCommitStacksFromServer(CandidateStacks, ExistingStacks))
        {
            UE_LOG(LogTemp, Error,
                TEXT("Failed to roll back inventory for player %s after M9 discovery persistence rejected the claim."),
                *PlayerIdentity);
        }
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Land discovery could not be saved"));
        return false;
    }

    SessionM9ExplorationClaims.FindOrAdd(PlayerIdentity).Add(StableIdentity);

    Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::LandmarkFound,
        FString::Printf(TEXT("%s (+%d %s)"), *Definition->ObservationText,
            Definition->RewardQuantity, *Definition->RewardItemId.ToString()));
    return true;
}

bool AKalmalaGameMode::ClaimMirelingBossScroll(AKalmalaCharacter* Attacker, const FString& PersistentSpawnId)
{
    if (!HasAuthority() || Attacker == nullptr || !Attacker->HasAuthority() || Attacker->GetWorld() != GetWorld()
        || !IsMirelingBossRewardId(PersistentSpawnId))
    {
        return false;
    }

    bool bDefeatedMireling = false;
    for (TActorIterator<AKalmalaWildlifeSpawn> It(GetWorld()); It; ++It)
    {
        if (*It != nullptr && (*It)->GetPersistentSpawnId() == PersistentSpawnId
            && (*It)->IsDefeated() && (*It)->GetArchetype() == EKalmalaWildlifeArchetype::Mireling)
        {
            bDefeatedMireling = true;
            break;
        }
    }
    if (!bDefeatedMireling) return false;

    UKalmalaPlayerDiscoverySaveGameV2* Save = nullptr;
    FString Identity;
    Save = GetPlayerDiscoverySave(Attacker, Identity);
    UKalmalaDiscoveryProgressComponent* Feedback = Attacker->GetDiscoveryProgressComponent();
    UKalmalaSupportMagicComponent* Support = Attacker->GetSupportMagicComponent();
    if (Save == nullptr || Feedback == nullptr || Support == nullptr) return false;

    const FString Definition = GetMirelingBossScrollDefinition(WorldGenerationConfig.WorldSeed);
    const EKalmalaSupportEffect Effect = UKalmalaSupportMagicComponent::FromScrollDefinition(Definition);
    const FString DiscoveryId = GetMirelingBossScrollId(WorldGenerationConfig.WorldSeed);
    if (!UKalmalaSupportMagicComponent::IsKnownEffect(Effect) || Save->HasDiscovery(DiscoveryId))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::AlreadyFound, TEXT("Mireling scroll already claimed"));
        return false;
    }
    if (!PersistPlayerStateFromServer(Attacker, DiscoveryId,
        UKalmalaSupportMagicComponent::CanonicalId(Effect)))
    {
        Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::Unavailable, TEXT("Mireling scroll unavailable"));
        return false;
    }

    Support->LearnEffectFromServer(Effect);
    Feedback->PublishFeedbackFromServer(EKalmalaDiscoveryFeedback::ScrollFound,
        FString::Printf(TEXT("Mireling boss scroll: %s"), *Definition));
    return true;
}

bool AKalmalaGameMode::IsMirelingBossRewardId(const FString& PersistentSpawnId)
{
    return !PersistentSpawnId.IsEmpty() && PersistentSpawnId.Len() <= 128
        && FCrc::StrCrc32(*PersistentSpawnId) % 5u == 0u;
}

FString AKalmalaGameMode::GetMirelingBossScrollDefinition(const uint64 WorldSeed)
{
    static const TCHAR* Definitions[] = { TEXT("mending"), TEXT("hearth-shield"), TEXT("bears-vigor"), TEXT("deer-call") };
    return Definitions[static_cast<uint32>(WorldSeed % UE_ARRAY_COUNT(Definitions))];
}

FString AKalmalaGameMode::GetMirelingBossScrollId(const uint64 WorldSeed)
{
    return FString::Printf(TEXT("Scroll:1:%s:mireling-boss"), *GetMirelingBossScrollDefinition(WorldSeed));
}

void AKalmalaGameMode::Logout(AController* Exiting)
{
    if (HasAuthority() && Exiting != nullptr)
    {
        if (AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(Exiting->GetPawn()))
            PersistPlayerStateFromServer(Character);
    }
    Super::Logout(Exiting);
}

void AKalmalaGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

#if !UE_BUILD_SHIPPING
    if (bM9Schema2CandidatePeerTestEnabled && NewPlayer != nullptr && NewPlayer->GetPlayerState<APlayerState>() != nullptr)
    {
        FString TestRole;
        if (NewPlayer->IsLocalController())
        {
            TestRole = TEXT("host");
        }
        else
        {
            TestRole = M9Schema2CandidatePeerLoginCount++ == 0 ? TEXT("owner") : TEXT("observer");
        }
        const FUniqueNetIdStringRef TestNetId = FUniqueNetIdString::Create(
            FString::Printf(TEXT("m9-schema2-%s"), *TestRole), FName(TEXT("KalmalaM9Schema2PeerTest")));
        NewPlayer->GetPlayerState<APlayerState>()->SetUniqueId(FUniqueNetIdRepl(*TestNetId));
        NewPlayer->GetPlayerState<APlayerState>()->SetPlayerName(FString::Printf(TEXT("M9Schema2-%s"), *TestRole));
        UE_LOG(LogTemp, Display, TEXT("M9 schema-2 candidate assigned test identity: Role=%s Phase=%s"),
            *TestRole, *M9Schema2CandidatePeerTestPhase);
    }

    if (FParse::Param(FCommandLine::Get(), TEXT("KalmalaInventoryTest"))
        && NewPlayer != nullptr && NewPlayer->GetPlayerState<APlayerState>() != nullptr)
    {
        const FString TestRole = NewPlayer->IsLocalController() ? TEXT("host") : TEXT("client");
        const FUniqueNetIdStringRef TestNetId = FUniqueNetIdString::Create(
            FString::Printf(TEXT("inventory-test-%s"), *TestRole), FName(TEXT("KalmalaInventoryReconnectTest")));
        NewPlayer->GetPlayerState<APlayerState>()->SetUniqueId(FUniqueNetIdRepl(*TestNetId));
        UE_LOG(LogTemp, Display, TEXT("Inventory reconnect fixture assigned authenticated test identity: Role=%s"), *TestRole);
    }

    FString ReconnectPhase;
    const bool bOceanReconnectPeerTest = FParse::Param(FCommandLine::Get(),
        TEXT("KalmalaOceanDiscoveryDisembarkPeerTest"))
        && (FParse::Value(FCommandLine::Get(), TEXT("KalmalaOceanReconnectPhase="), ReconnectPhase)
            || FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanIntegratedReconnectPeerTest")));
    if (NewPlayer != nullptr && bOceanReconnectPeerTest)
    {
        APlayerState* PlayerState = NewPlayer->GetPlayerState<APlayerState>();
        FString StableTestId;
        FString StableTestRole;
        if (NewPlayer->IsLocalController())
        {
            StableTestRole = TEXT("Host");
            StableTestId = TEXT("m8-reconnect-host");
        }
        else
        {
            int32 ExistingRemotePlayers = 0;
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                APlayerController* ExistingController = It->Get();
                if (ExistingController != nullptr && ExistingController != NewPlayer
                    && !ExistingController->IsLocalController()
                    && ExistingController->GetPlayerState<APlayerState>() != nullptr)
                {
                    ++ExistingRemotePlayers;
                }
            }
            StableTestRole = ExistingRemotePlayers == 0 ? TEXT("ReturningOwner") : TEXT("LateJoiner");
            StableTestId = ExistingRemotePlayers == 0
                ? TEXT("m8-reconnect-owner") : TEXT("m8-reconnect-late");
        }
        if (PlayerState != nullptr && !StableTestId.IsEmpty())
        {
            const FUniqueNetIdStringRef TestNetId = FUniqueNetIdString::Create(
                StableTestId, FName(TEXT("KalmalaReconnectTest")));
            PlayerState->SetUniqueId(FUniqueNetIdRepl(*TestNetId));
            UE_LOG(LogTemp, Display,
                TEXT("M8 reconnect fixture assigned stable test-provider identity: Role=%s Phase=%s"),
                *StableTestRole, *ReconnectPhase);
        }
    }
#endif

    PlacePawnAtGeneratedStart(NewPlayer);

    if (NewPlayer != nullptr)
    {
        AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(NewPlayer->GetPawn());
        if (Character != nullptr && Character->GetPlayerState() != nullptr)
        {
            FString IgnoredIdentity;
            GetPlayerDiscoverySave(Character, IgnoredIdentity);
            LoadOceanDiscoveryLedger(Character);
            RestoreOceanTravelForPlayer(NewPlayer);
        }
        PendingOceanTravelRestores.AddUnique(NewPlayer);
    }

#if !UE_BUILD_SHIPPING
    const bool bAmbientAudioTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaAmbientAudioTest"));
#else
    const bool bAmbientAudioTest = false;
#endif
    if ((!bTraversalTestEnabled && ReconnectVerificationMode.IsEmpty() && !bExposureInspectionEnabled && !bExposureReplicationTestEnabled && !bCampConditionInspectionEnabled && !bBiomeFeatureInspectionEnabled && !bWorldProfileEnabled && !FParse::Param(FCommandLine::Get(), TEXT("KalmalaCampChoiceTest")) && !FParse::Param(FCommandLine::Get(), TEXT("KalmalaDiscoveryPeerTest")) && !FParse::Param(FCommandLine::Get(), TEXT("KalmalaM9ExplorationRewardPeerTest")) && !FParse::Param(FCommandLine::Get(), TEXT("KalmalaOceanJourneyPeerTest")) && !bM9Schema2CandidatePeerTestEnabled && !bAmbientAudioTest) || NewPlayer == nullptr)
    {
        return;
    }

    // GameModeBase does not guarantee a pawn for a headless, command-line client.
    // The harness creates its normal default pawn only when the test switch is enabled.
    if (NewPlayer->GetPawn() == nullptr)
    {
        RestartPlayer(NewPlayer);
    }

#if !UE_BUILD_SHIPPING
    if (bM9Schema2CandidatePeerTestEnabled)
    {
        RunM9Schema2CandidatePlayerPeerTest(NewPlayer);
    }
#endif

    if (bAmbientAudioTest && NewPlayer->GetPawn() != nullptr)
    {
        if (AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(NewPlayer->GetPawn()))
        {
            const FVector Forward = FRotator(0.0f, Character->GetActorRotation().Yaw, 0.0f).Vector();
            const FVector HearthLocation = Character->GetActorLocation() + Forward * 170.0f - FVector(0.0f, 0.0f, 38.0f);
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AKalmalaCampfire* Campfire = GetWorld()->SpawnActor<AKalmalaCampfire>(
                AKalmalaCampfire::StaticClass(), HearthLocation, FRotator::ZeroRotator, SpawnParameters);
            if (Campfire != nullptr)
            {
                Campfire->InitializePaidFromServer(Character);
                Campfire->Interact_Implementation(Character);
                Campfire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
                UE_LOG(LogTemp, Display, TEXT("Ambient audio verification server spawned lit hearth: Lit=%d"),
                    Campfire->IsLit() ? 1 : 0);
            }
            if (UKalmalaPlayerStatusComponent* Statuses = Character->FindComponentByClass<UKalmalaPlayerStatusComponent>())
            {
                Statuses->ApplyWetFromServer();
                UE_LOG(LogTemp, Display, TEXT("Ambient audio verification server applied Wet status: Wet=%d"),
                    Statuses->HasStatus(UKalmalaPlayerStatusComponent::WetStatusId) ? 1 : 0);
            }
            if (UKalmalaSupportMagicComponent* Support = Character->GetSupportMagicComponent())
            {
                const bool bLearned = Support->LearnEffectFromServer(EKalmalaSupportEffect::HearthShield);
                if (bLearned)
                {
                    Support->ActivateSupportEffectFromHotbarServer(EKalmalaSupportEffect::HearthShield);
                }
                UE_LOG(LogTemp, Display,
                    TEXT("Ambient audio verification server support result: Learned=%d Accepted=%d Serial=%u"),
                    bLearned ? 1 : 0, Support->GetFeedback() == EKalmalaSupportFeedback::Accepted ? 1 : 0,
                    Support->GetFeedbackSerial());
            }

            const TWeakObjectPtr<AKalmalaCharacter> WeakCharacter(Character);
            FTimerHandle SupportExpiryAudioTimer;
            GetWorldTimerManager().SetTimer(SupportExpiryAudioTimer,
                FTimerDelegate::CreateWeakLambda(this, [WeakCharacter]()
                {
                    AKalmalaCharacter* TestCharacter = WeakCharacter.Get();
                    if (TestCharacter == nullptr) return;
                    UKalmalaSupportMagicComponent* Support = TestCharacter->GetSupportMagicComponent();
                    if (Support == nullptr) return;
                    const bool bLearned = Support->LearnEffectFromServer(EKalmalaSupportEffect::BearsVigor);
                    if (bLearned)
                    {
                        Support->ActivateSupportEffectFromHotbarServer(EKalmalaSupportEffect::BearsVigor);
                    }
                    UE_LOG(LogTemp, Display,
                        TEXT("Ambient audio verification server support result: Learned=%d Accepted=%d Serial=%u Effect=BearsVigor"),
                        bLearned ? 1 : 0, Support->GetFeedback() == EKalmalaSupportFeedback::Accepted ? 1 : 0,
                        Support->GetFeedbackSerial());
                }), UKalmalaSupportMagicComponent::CooldownSeconds + 0.5f, false);
            FTimerHandle AcceptedInteractionTimer;
            GetWorldTimerManager().SetTimer(AcceptedInteractionTimer,
                FTimerDelegate::CreateWeakLambda(this, [WeakCharacter]()
                {
                    AKalmalaCharacter* TestCharacter = WeakCharacter.Get();
                    if (TestCharacter == nullptr) return;
                    UKalmalaInventoryComponent* Inventory = TestCharacter->FindComponentByClass<UKalmalaInventoryComponent>();
                    UKalmalaCraftingComponent* Crafting = TestCharacter->FindComponentByClass<UKalmalaCraftingComponent>();
                    if (Inventory == nullptr || Crafting == nullptr) return;
                    const bool bIngredientsGranted = Inventory->TryGrantFromServer(TEXT("Wood"), 9);
                    const uint32 PreviousSerial = Crafting->GetResultSerial();
                    if (bIngredientsGranted)
                    {
                        Crafting->ServerCraft_Implementation(TEXT("CookingRack"), 1);
                    }
                    const bool bAccepted = bIngredientsGranted && Crafting->GetResultSerial() != PreviousSerial
                        && Crafting->WasLastResultAccepted() && Inventory->GetQuantity(TEXT("CookingRackKit")) == 1;
                    UE_LOG(LogTemp, Display,
                        TEXT("Ambient audio verification server crafted rack: Accepted=%d ResultSerial=%u"),
                        bAccepted ? 1 : 0, Crafting->GetResultSerial());
                }), 2.0f, false);

            FTimerHandle GatheringInteractionTimer;
            GetWorldTimerManager().SetTimer(GatheringInteractionTimer,
                FTimerDelegate::CreateWeakLambda(this, [WeakCharacter]()
                {
                    AKalmalaCharacter* TestCharacter = WeakCharacter.Get();
                    if (TestCharacter == nullptr) return;
                    UKalmalaInventoryComponent* Inventory = TestCharacter->FindComponentByClass<UKalmalaInventoryComponent>();
                    if (Inventory == nullptr) return;
                    FKalmalaWorldPopulationSpawn Spawn;
                    Spawn.Kind = EKalmalaWorldPopulationKind::HarvestNode;
                    Spawn.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(TestCharacter->GetActorLocation()));
                    Spawn.SpawnSeed = TestCharacter->GetUniqueID();
                    Spawn.Location = TestCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
                    FActorSpawnParameters SpawnParameters;
                    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                    AKalmalaHarvestNode* Node = TestCharacter->GetWorld()->SpawnActor<AKalmalaHarvestNode>(
                        AKalmalaHarvestNode::StaticClass(), Spawn.Location, FRotator::ZeroRotator, SpawnParameters);
                    if (Node == nullptr) return;
                    Node->InitializeServer(Spawn);
                    const FName ItemId = Node->GetHarvestItemId();
                    const int32 Before = Inventory->GetQuantity(ItemId);
                    Node->Interact_Implementation(TestCharacter);
                    const bool bAccepted = Inventory->GetQuantity(ItemId) == Before + 1;
                    Node->Interact_Implementation(TestCharacter);
                    const bool bDuplicateRejected = Inventory->GetQuantity(ItemId) == Before + 1;
                    UE_LOG(LogTemp, Display,
                        TEXT("Ambient audio verification server gathered: Accepted=%d DuplicateRejected=%d"),
                        bAccepted ? 1 : 0, bDuplicateRejected ? 1 : 0);
                    Node->Destroy();
                }), 3.0f, false);

            FTimerHandle RejectedInteractionTimer;
            GetWorldTimerManager().SetTimer(RejectedInteractionTimer,
                FTimerDelegate::CreateWeakLambda(this, [WeakCharacter]()
                {
                    AKalmalaCharacter* TestCharacter = WeakCharacter.Get();
                    if (TestCharacter == nullptr) return;
                    UKalmalaCraftingComponent* Crafting = TestCharacter->FindComponentByClass<UKalmalaCraftingComponent>();
                    if (Crafting == nullptr) return;
                    const uint32 PreviousSerial = Crafting->GetResultSerial();
                    Crafting->ServerCraft_Implementation(TEXT("UnknownAudioTestRecipe"), 1);
                    UE_LOG(LogTemp, Display,
                        TEXT("Ambient audio verification server rejected craft: Rejected=%d ResultSerial=%u"),
                        Crafting->GetResultSerial() != PreviousSerial && !Crafting->WasLastResultAccepted() ? 1 : 0,
                        Crafting->GetResultSerial());
                }), 4.0f, false);
        }
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
            if (auto* Pack = NewPlayer->GetPawn()->FindComponentByClass<UKalmalaInventoryComponent>()) Pack->TryGrantFromServer(TEXT("Wood"), 1);
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

void AKalmalaGameMode::RunM9Schema2CandidateWorldPeerTest()
{
#if !UE_BUILD_SHIPPING
    static const FString TestSlot(TEXT("KalmalaM9Schema2PeerWorld"));
    if (!HasAuthority() || !WorldGenerationConfig.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("M9 schema-2 candidate world: Phase=%s Passed=0 Reason=InvalidWorld"),
            *M9Schema2CandidatePeerTestPhase);
        return;
    }

    if (M9Schema2CandidatePeerTestPhase.Equals(TEXT("Seed"), ESearchCase::IgnoreCase))
    {
        UKalmalaConstructionSaveGameV2* Candidate = NewObject<UKalmalaConstructionSaveGameV2>(this);
        Candidate->InitializeForWorld(WorldGenerationConfig);
        const auto AddRecord = [Candidate](const TCHAR* Id, const FName KitId, const FVector& Location)
        {
            FKalmalaConstructionSaveRecord Record;
            Record.ConstructionId = Id;
            Record.KitId = KitId;
            Record.Transform = FTransform(Location);
            if (FKalmalaToolProgressionContract::IsStationAttachmentKit(KitId))
            {
                return Candidate->AddStationAttachmentRecord(Record);
            }
            return Candidate->AddRecord(Record);
        };
        const bool bAdded = AddRecord(TEXT("m9-peer-floor-01"), TEXT("FloorKit"), FVector(100.0f, 200.0f, 300.0f))
            && AddRecord(TEXT("m9-peer-rack-01"), TEXT("WorkbenchToolRackKit"), FVector(400.0f, 500.0f, 300.0f));
        const bool bSaved = bAdded && UGameplayStatics::SaveGameToSlot(Candidate, TestSlot, 0);
        UE_LOG(LogTemp, Display,
            TEXT("M9 schema-2 candidate world: Phase=Seed Passed=%d Records=%d Attachment=%d NormalSchema=%d"),
            bSaved ? 1 : 0, Candidate->GetRecords().Num(),
            Candidate->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId);
            }) ? 1 : 0,
            ConstructionSaveGame != nullptr ? ConstructionSaveGame->GetSchemaVersion() : 0);
        return;
    }

    if (M9Schema2CandidatePeerTestPhase.Equals(TEXT("Resume"), ESearchCase::IgnoreCase))
    {
        UKalmalaConstructionSaveGameV2* Candidate = Cast<UKalmalaConstructionSaveGameV2>(
            UGameplayStatics::LoadGameFromSlot(TestSlot, 0));
        const bool bMatches = Candidate != nullptr && Candidate->MatchesWorld(WorldGenerationConfig)
            && Candidate->GetRecords().Num() == 2
            && Candidate->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("m9-peer-floor-01") && Record.KitId == TEXT("FloorKit");
            })
            && Candidate->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("m9-peer-rack-01") && Record.KitId == TEXT("WorkbenchToolRackKit");
            });
        UE_LOG(LogTemp, Display,
            TEXT("M9 schema-2 candidate world: Phase=Resume Passed=%d Records=%d Attachment=%d NormalSchema=%d"),
            bMatches ? 1 : 0, Candidate != nullptr ? Candidate->GetRecords().Num() : 0,
            Candidate != nullptr && Candidate->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return FKalmalaToolProgressionContract::IsStationAttachmentKit(Record.KitId);
            }) ? 1 : 0,
            ConstructionSaveGame != nullptr ? ConstructionSaveGame->GetSchemaVersion() : 0);
        return;
    }

    UE_LOG(LogTemp, Error, TEXT("M9 schema-2 candidate world: Phase=%s Passed=0 Reason=InvalidPhase"),
        *M9Schema2CandidatePeerTestPhase);
#endif
}

void AKalmalaGameMode::RunM9Schema2CandidatePlayerPeerTest(APlayerController* PlayerController)
{
#if !UE_BUILD_SHIPPING
    if (!HasAuthority() || PlayerController == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("M9 schema-2 candidate player: Phase=%s Passed=0 Reason=InvalidController"),
            *M9Schema2CandidatePeerTestPhase);
        return;
    }

    if (PlayerController->IsLocalController())
    {
        UE_LOG(LogTemp, Display,
            TEXT("M9 schema-2 candidate player: Phase=%s Role=Host Passed=1 CandidateLoaded=0"),
            *M9Schema2CandidatePeerTestPhase);
        return;
    }

    APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>();
    AKalmalaCharacter* Character = Cast<AKalmalaCharacter>(PlayerController->GetPawn());
    const FString PlayerIdentity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(PlayerState);
    const FString TestRole = PlayerState != nullptr && PlayerIdentity.EndsWith(TEXT("m9-schema2-owner"))
        ? TEXT("Owner") : TEXT("Observer");
    FString NormalIdentity;
    UKalmalaPlayerDiscoverySaveGameV2* NormalPlayerSave = Character != nullptr
        ? GetPlayerDiscoverySave(Character, NormalIdentity) : nullptr;
    if (Character == nullptr || PlayerIdentity.IsEmpty() || NormalPlayerSave == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("M9 schema-2 candidate player: Phase=%s Role=%s Passed=0 Reason=MissingCharacterOrNormalSchema2"),
            *M9Schema2CandidatePeerTestPhase, *TestRole);
        return;
    }

    if (TestRole == TEXT("Observer"))
    {
        const bool bObserverHasTools = !Character->CarriedTools.IsEmpty();
        const bool bPassed = bObserverHasTools && NormalPlayerSave->GetToolRecords().IsEmpty();
        UE_LOG(LogTemp, Display,
            TEXT("M9 schema-2 candidate player: Phase=%s Role=Observer Passed=%d CandidateLoaded=0 ServerTools=%d NormalSchema=%d NormalTools=%d NormalFacts=0"),
            *M9Schema2CandidatePeerTestPhase, bPassed ? 1 : 0, Character->CarriedTools.Num(),
            UKalmalaPlayerDiscoverySaveGameV2::SchemaVersionValue, NormalPlayerSave->GetToolRecords().Num());
        return;
    }

    const FString TestSlot = FString::Printf(TEXT("KalmalaM9Schema2PeerPlayer_%08x"), FCrc::StrCrc32(*PlayerIdentity));
    UKalmalaPlayerDiscoverySaveGameV2* Candidate = nullptr;
    const FString DiscoveryId(TEXT("Poi:1:lakes-island-cache:-2,4:1"));
    const FString ClaimId = FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
        TEXT("lakes-rillworn-marker"), FIntPoint(-29, 14));
    bool bReplayRejected = false;
    if (M9Schema2CandidatePeerTestPhase.Equals(TEXT("Seed"), ESearchCase::IgnoreCase))
    {
        FString NormalClaimId;
        for (int32 Y = -12; Y <= 12 && NormalClaimId.IsEmpty(); ++Y)
        {
            for (int32 X = -12; X <= 12 && NormalClaimId.IsEmpty(); ++X)
            {
                const FIntPoint SpatialKey(X, Y);
                const TArray<FKalmalaM9ExplorationRewardDescriptor> Descriptors =
                    FKalmalaM9ExplorationRewardCatalogue::BuildDescriptors(WorldGenerationConfig, SpatialKey);
                if (!Descriptors.IsEmpty())
                {
                    NormalClaimId = FKalmalaM9ExplorationRewardCatalogue::MakeStableIdentity(
                        Descriptors[0].CandidateId, SpatialKey);
                }
            }
        }
        if (NormalClaimId.IsEmpty()
            || !PersistPlayerStateFromServer(Character, FString(), FString(), NormalClaimId))
        {
            UE_LOG(LogTemp, Error,
                TEXT("M9 schema-2 candidate player: Phase=Seed Role=Owner Passed=0 Reason=NormalSlotWriteOrClaimRevalidation"));
            return;
        }
        NormalPlayerSave = GetPlayerDiscoverySave(Character, NormalIdentity);
    }

    const bool bNormalFactsMatch = NormalPlayerSave != nullptr
        && !NormalPlayerSave->GetM9ClaimIds().IsEmpty()
        && NormalPlayerSave->GetToolRecords().Num() == Character->CarriedTools.Num();

    if (M9Schema2CandidatePeerTestPhase.Equals(TEXT("Seed"), ESearchCase::IgnoreCase))
    {
        Candidate = NewObject<UKalmalaPlayerDiscoverySaveGameV2>(this);
        Candidate->InitializeForPlayer(WorldGenerationConfig, PlayerIdentity);
        const auto AddTool = [Candidate](const FName ItemId, const int32 ToolLevel, const int32 Condition)
        {
            FKalmalaPlayerToolSaveRecord Record;
            Record.ItemId = ItemId;
            Record.ToolLevel = ToolLevel;
            Record.Condition = Condition;
            return Candidate->AddToolRecord(Record);
        };
        const bool bCandidateBuilt = Candidate->AddDiscovery(DiscoveryId)
            && Candidate->AddM9Claim(ClaimId)
            && Candidate->AddLearnedEffect(TEXT("Effect:mending"))
            && AddTool(TEXT("StoneKnife"), 1, 9)
            && AddTool(TEXT("BronzeAxe"), 1, 0)
            && AddTool(TEXT("IronAxe"), 2, 20);
        if (!bCandidateBuilt || !UGameplayStatics::SaveGameToSlot(Candidate, TestSlot, 0))
        {
            UE_LOG(LogTemp, Error, TEXT("M9 schema-2 candidate player: Phase=Seed Role=Owner Passed=0 Reason=CandidateWrite"));
            return;
        }
    }
    else if (M9Schema2CandidatePeerTestPhase.Equals(TEXT("Resume"), ESearchCase::IgnoreCase))
    {
        Candidate = Cast<UKalmalaPlayerDiscoverySaveGameV2>(UGameplayStatics::LoadGameFromSlot(TestSlot, 0));
        if (Candidate == nullptr || !Candidate->MatchesPlayer(WorldGenerationConfig, PlayerIdentity)
            || !Candidate->HasDiscovery(DiscoveryId) || !Candidate->HasLearnedEffect(TEXT("Effect:mending"))
            || !Candidate->GetM9ClaimIds().Contains(ClaimId) || Candidate->GetToolRecords().Num() != 3)
        {
            UE_LOG(LogTemp, Error, TEXT("M9 schema-2 candidate player: Phase=Resume Role=Owner Passed=0 Reason=CandidateMismatch"));
            return;
        }
        const int32 ClaimsBeforeReplay = Candidate->GetM9ClaimIds().Num();
        bReplayRejected = !Candidate->AddM9Claim(ClaimId)
            && Candidate->GetM9ClaimIds().Num() == ClaimsBeforeReplay;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("M9 schema-2 candidate player: Phase=%s Role=Owner Passed=0 Reason=InvalidPhase"),
            *M9Schema2CandidatePeerTestPhase);
        return;
    }

    Character->CarriedTools.Reset();
    for (const FKalmalaPlayerToolSaveRecord& Record : Candidate->GetToolRecords())
    {
        FKalmalaToolState ToolState;
        ToolState.ItemId = Record.ItemId;
        ToolState.ToolLevel = Record.ToolLevel;
        ToolState.Durability = Record.Condition;
        ToolState.InstanceGuid = Record.InstanceGuid;
        Character->CarriedTools.Add(ToolState);
    }
    Character->ForceNetUpdate();
    const bool bServerAgreement = Character->CarriedTools.Num() == Candidate->GetToolRecords().Num()
        && Character->GetToolDurability(TEXT("StoneKnife")) == 9
        && Character->GetToolDurability(TEXT("BronzeAxe")) == 0
        && Character->GetToolDurability(TEXT("IronAxe")) == 20
        && Character->GetCarriedToolLevel(TEXT("IronAxe")) == 2;
    const bool bReplayCheckPassed = M9Schema2CandidatePeerTestPhase.Equals(TEXT("Seed"), ESearchCase::IgnoreCase)
        || bReplayRejected;
    const bool bPassed = Candidate->MatchesPlayer(WorldGenerationConfig, PlayerIdentity)
        && Candidate->HasDiscovery(DiscoveryId) && Candidate->HasLearnedEffect(TEXT("Effect:mending"))
        && Candidate->GetM9ClaimIds().Contains(ClaimId) && bReplayCheckPassed && bServerAgreement
        && bNormalFactsMatch && NormalPlayerSave->MatchesPlayer(WorldGenerationConfig, NormalIdentity);
    UE_LOG(LogTemp, Display,
        TEXT("M9 schema-2 candidate player: Phase=%s Role=Owner Passed=%d CandidateReady=1 Discovery=1 Claim=1 ReplayRejected=%d ServerTools=%d NormalSchema=%d NormalTools=%d NormalFacts=%d"),
        *M9Schema2CandidatePeerTestPhase, bPassed ? 1 : 0, bReplayRejected ? 1 : 0,
        Character->CarriedTools.Num(), UKalmalaPlayerDiscoverySaveGameV2::SchemaVersionValue,
        NormalPlayerSave->GetToolRecords().Num(), bNormalFactsMatch ? 1 : 0);
#endif
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
    FString RequestedWildlifeSpawnId;
    int32 RequestedWildlifeKeyX = 0;
    int32 RequestedWildlifeKeyY = 0;
    const bool bHasRequestedWildlifeSpawnId = FParse::Value(
        FCommandLine::Get(), TEXT("KalmalaReconnectSpawnId="), RequestedWildlifeSpawnId);
    const bool bHasRequestedWildlifeKeyX = FParse::Value(
        FCommandLine::Get(), TEXT("KalmalaReconnectKeyX="), RequestedWildlifeKeyX);
    const bool bHasRequestedWildlifeKeyY = FParse::Value(
        FCommandLine::Get(), TEXT("KalmalaReconnectKeyY="), RequestedWildlifeKeyY);
    const bool bHasRequestedWildlifeKey = bHasRequestedWildlifeKeyX && bHasRequestedWildlifeKeyY;
    if (bHasRequestedWildlifeSpawnId != bHasRequestedWildlifeKey)
    {
        UE_LOG(LogTemp, Error, TEXT("Reconnect verification received an incomplete exact wildlife identity."));
        FPlatformMisc::RequestExit(false);
        return;
    }
    if (bHasRequestedWildlifeSpawnId)
    {
        SpatialKey = FIntPoint(RequestedWildlifeKeyX, RequestedWildlifeKeyY);
    }
    const bool bWildlifeBoarVerify = ReconnectVerificationMode.Equals(TEXT("WildlifeBoarVerify"), ESearchCase::IgnoreCase);
    const bool bWildlifeDeerVerify = ReconnectVerificationMode.Equals(TEXT("WildlifeDeerVerify"), ESearchCase::IgnoreCase);
    const EKalmalaWildlifeArchetype VerifyArchetype = bWildlifeDeerVerify ? EKalmalaWildlifeArchetype::Deer : EKalmalaWildlifeArchetype::Boar;
    if (ReconnectVerificationMode.Equals(TEXT("WildlifeDefeat"), ESearchCase::IgnoreCase) || ReconnectVerificationMode.Equals(TEXT("WildlifeVerify"), ESearchCase::IgnoreCase) || bWildlifeBoarVerify || bWildlifeDeerVerify)
    {
        TArray<FKalmalaWorldPopulationSpawn> WildlifeSpawns = KalmalaGameMode::FilterEligibleWildlifeSpawns(
            WorldGenerationConfig, FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, SpatialKey, EKalmalaWorldPopulationKind::Wildlife));
        const auto HasRequestedArchetype = [this, VerifyArchetype](const FKalmalaWorldPopulationSpawn& Spawn)
        {
            return KalmalaGameMode::IsWildlifeSpawnOfArchetype(WorldGenerationConfig, Spawn, VerifyArchetype);
        };
        if (RequestedWildlifeSpawnId.IsEmpty()
            && (((bWildlifeBoarVerify || bWildlifeDeerVerify) && !WildlifeSpawns.ContainsByPredicate(HasRequestedArchetype)) || WildlifeSpawns.IsEmpty()))
        {
            const FIntPoint BaseKey = SpatialKey;
            bool bFoundArchetype = false;
            for (int32 OffsetY = -1; OffsetY <= 1 && !bFoundArchetype; ++OffsetY)
            {
                for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
                {
                    const FIntPoint CandidateKey = BaseKey + FIntPoint(OffsetX, OffsetY);
                    TArray<FKalmalaWorldPopulationSpawn> CandidateSpawns = KalmalaGameMode::FilterEligibleWildlifeSpawns(
                        WorldGenerationConfig, FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(WorldGenerationConfig, CandidateKey, EKalmalaWorldPopulationKind::Wildlife));
                    const bool bHasCandidate = (bWildlifeBoarVerify || bWildlifeDeerVerify)
                        ? CandidateSpawns.ContainsByPredicate(HasRequestedArchetype)
                        : !CandidateSpawns.IsEmpty();
                    if (bHasCandidate)
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

        const FKalmalaWorldPopulationSpawn* ExpectedSpawn = nullptr;
        if (!RequestedWildlifeSpawnId.IsEmpty())
        {
            ExpectedSpawn = WildlifeSpawns.FindByPredicate([&RequestedWildlifeSpawnId](const FKalmalaWorldPopulationSpawn& Spawn)
            {
                return FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn) == RequestedWildlifeSpawnId;
            });
        }
        else
        {
            ExpectedSpawn = (bWildlifeBoarVerify || bWildlifeDeerVerify)
                ? WildlifeSpawns.FindByPredicate(HasRequestedArchetype) : &WildlifeSpawns[0];
        }
        if (ExpectedSpawn == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Reconnect verification found no requested server-derived wildlife spawn for its spatial key."));
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
                AKalmalaCharacter* Interactor = Cast<AKalmalaCharacter>(ServerPawn);
                bool bHarvestAccepted = false;
                if (Interactor != nullptr && Node->GetGatheringSourceId().IsNone())
                {
                    Node->Interact_Implementation(Interactor);
                    bHarvestAccepted = Node->IsHarvested();
                }
                else if (Interactor != nullptr)
                {
                    FKalmalaToolServerSelection Selection;
                    if (FKalmalaToolLifecycleContract::BuildServerSelection(Node->GetGatheringSourceId(), Selection))
                    {
                        const FKalmalaToolDefinition* Tool = FKalmalaToolLifecycleContract::FindMinimumQualifiedTool(Selection);
                        if (Tool != nullptr)
                        {
                            bHarvestAccepted = Node->InteractWithToolIntentFromServer(Interactor, 0.0f,
                                FKalmalaToolLifecycleContract::DefaultMaximumRange, Tool->ItemId,
                                static_cast<uint8>(Selection.Action));
                        }
                    }
                }

                if (bHarvestAccepted && PopulationSaveGame->IsHarvested(PersistentSpawnId))
                {
                    UE_LOG(LogTemp, Display, TEXT("Reconnect verification harvested generated node %s through its accepted server transaction before listen-server restart."), *PersistentSpawnId);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Reconnect verification could not commit generated harvest %s; accepted=%d sparse-state=%d."),
                        *PersistentSpawnId, bHarvestAccepted, PopulationSaveGame->IsHarvested(PersistentSpawnId));
                }
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

    AActor* SpawnStart = GeneratedPlayerStart;
#if !UE_BUILD_SHIPPING
    const bool bRatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaRatPeerTest"));
    const bool bGiantSnakePeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaGiantSnakePeerTest"));
    const bool bMosquitoPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMosquitoPeerTest"));
    if ((bRatPeerTest || bGiantSnakePeerTest || bMosquitoPeerTest)
        && PlayerController->StartSpot.IsValid())
    {
        SpawnStart = PlayerController->StartSpot.Get();
    }
#endif

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

    const FVector2D StartPosition(SpawnStart->GetActorLocation());
    const float TerrainHeight = FKalmalaTerrainHeightSampler::SampleHeight(WorldGenerationConfig, StartPosition);
    const float RequiredCentreHeight = TerrainHeight + Pawn->GetSimpleCollisionHalfHeight() + 30.0f;
    FVector SpawnLocation = SpawnStart->GetActorLocation();
    SpawnLocation.Z = FMath::Max(SpawnLocation.Z, RequiredCentreHeight);
    Pawn->SetActorLocationAndRotation(SpawnLocation, SpawnStart->GetActorRotation(), false, nullptr, ETeleportType::TeleportPhysics);
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
    TArray<int32> SuppressedForestCandidates;
    TArray<TWeakObjectPtr<AKalmalaHarvestNode>> ForestNodes;
    const bool bMatchingPopulationSave = PopulationSaveGame != nullptr && PopulationSaveGame->MatchesWorld(WorldGenerationConfig);
    for (const FKalmalaForestTreeDescriptor& Tree : FKalmalaForestLayout::Generate(WorldGenerationConfig, PatchCenter))
    {
        if (bMatchingPopulationSave && PopulationSaveGame->IsHarvested(Tree.PersistentSpawnId))
        {
            SuppressedForestCandidates.Add(Tree.CandidateIndex);
            continue;
        }
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = TerrainPatch;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaHarvestNode* Node = GetWorld()->SpawnActor<AKalmalaHarvestNode>(
            AKalmalaHarvestNode::StaticClass(), Tree.Location, FRotator(0.0f, Tree.Yaw, 0.0f), SpawnParameters);
        if (Node == nullptr) continue; // Retain the decorative tree on allocation failure.
        if (!Node->InitializeGeneratedForestServer(Tree))
        {
            Node->Destroy();
            continue;
        }
        Node->OnHarvested.AddUObject(this, &AKalmalaGameMode::RecordHarvestedSpawn);
        ForestNodes.Add(Node);
        SuppressedForestCandidates.Add(Tree.CandidateIndex);
    }
    if (!TerrainPatch->SetSuppressedForestCandidatesServer(SuppressedForestCandidates))
    {
        // Do not publish overlapping actor/cosmetic trees if validation fails.
        for (const TWeakObjectPtr<AKalmalaHarvestNode>& Node : ForestNodes)
        {
            if (Node.IsValid()) Node->Destroy();
        }
        TerrainPatch->Destroy();
        return;
    }
    ActiveForestNodes.Add(PatchCoordinate, MoveTemp(ForestNodes));
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
            if (TArray<TWeakObjectPtr<AKalmalaHarvestNode>>* Nodes = ActiveForestNodes.Find(Iterator.Key()))
            {
                // Eviction is not harvesting: never emit rewards or depletion.
                for (const TWeakObjectPtr<AKalmalaHarvestNode>& Node : *Nodes)
                {
                    if (Node.IsValid()) Node->Destroy();
                }
                ActiveForestNodes.Remove(Iterator.Key());
            }
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
#if !UE_BUILD_SHIPPING
    const bool bRatPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaRatPeerTest"));
    const bool bGiantSnakePeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaGiantSnakePeerTest"));
    const bool bMosquitoPeerTest = FParse::Param(FCommandLine::Get(), TEXT("KalmalaMosquitoPeerTest"));
    if ((bRatPeerTest || bGiantSnakePeerTest || bMosquitoPeerTest)
        && Player != nullptr && !Player->IsLocalController() && GeneratedPlayerStart != nullptr && GetWorld() != nullptr)
    {
        const FName PeerTestStartTag(bRatPeerTest
            ? TEXT("KalmalaRatPeerTestStart")
            : bGiantSnakePeerTest ? TEXT("KalmalaGiantSnakePeerTestStart") : TEXT("KalmalaMosquitoPeerTestStart"));
        if (AActor* ExistingStart = Player->StartSpot.Get(); ExistingStart != nullptr && ExistingStart->ActorHasTag(PeerTestStartTag))
        {
            return ExistingStart;
        }

        int32 ExistingPeerTestStarts = 0;
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            AController* ExistingController = It->Get();
            if (ExistingController != nullptr && ExistingController != Player && !ExistingController->IsLocalController())
            {
                const AActor* ExistingStart = ExistingController->StartSpot.Get();
                if (ExistingStart != nullptr && ExistingStart->ActorHasTag(PeerTestStartTag)) ++ExistingPeerTestStarts;
            }
        }

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        const FVector StartLocation = GeneratedPlayerStart->GetActorLocation() + FVector(0.0f, 1000.0f * (ExistingPeerTestStarts + 1), 0.0f);
        APlayerStart* PeerTestStart = GetWorld()->SpawnActor<APlayerStart>(
            APlayerStart::StaticClass(), StartLocation, GeneratedPlayerStart->GetActorRotation(), SpawnParameters);
        if (PeerTestStart != nullptr)
        {
            PeerTestStart->Tags.Add(PeerTestStartTag);
            return PeerTestStart;
        }
    }
#endif
    return GeneratedPlayerStart != nullptr ? GeneratedPlayerStart : Super::ChoosePlayerStart_Implementation(Player);
}

const FKalmalaGemstoneState* AKalmalaGameMode::ReadGemstoneState(const FString& SpawnId) const
{
    return HasAuthority() && PopulationSaveGame && PopulationSaveGame->MatchesWorld(WorldGenerationConfig)
        ? PopulationSaveGame->FindGemstone(SpawnId) : nullptr;
}

bool AKalmalaGameMode::PersistGemstoneState(const FKalmalaGemstoneState& State,
    const uint8 ExpectedHits, const uint8 ExpectedOutcome)
{
    if (!HasAuthority() || !PopulationSaveGame || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)) return false;
    const FKalmalaGemstoneState* Existing = PopulationSaveGame->FindGemstone(State.SpawnId);
    if (Existing ? (Existing->Hits != ExpectedHits || Existing->Outcome != ExpectedOutcome || Existing->SourceId != State.SourceId)
        : (ExpectedHits != 0 || ExpectedOutcome != 0)) return false;
    if (!((ExpectedOutcome == 0 && State.Hits == ExpectedHits + 1)
        || (ExpectedOutcome == 2 && State.Hits == ExpectedHits && State.Outcome == 3))) return false;
    const bool bHadExisting = Existing != nullptr;
    const FKalmalaGemstoneState Before = Existing ? *Existing : FKalmalaGemstoneState();
    if (!PopulationSaveGame->SetGemstone(State)) return false;
    if (UGameplayStatics::SaveGameToSlot(PopulationSaveGame, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0)) return true;
    if (bHadExisting) PopulationSaveGame->SetGemstone(Before);
    else PopulationSaveGame->RemoveGemstone(State.SpawnId);
    return false;
}

FName AKalmalaGameMode::SelectEnemyGemForSpawn(const FString& SpawnId, const bool bMoonBear) const
{
    if (!HasAuthority()) return NAME_None;
    TArray<FString> Parts;
    SpawnId.ParseIntoArray(Parts, TEXT("/"), false);
    int32 Kind = -1, X = 0, Y = 0;
    uint64 Seed = 0;
    if (Parts.Num() != 4 || !LexTryParseString(Kind, *Parts[0]) || !LexTryParseString(X, *Parts[1])
        || !LexTryParseString(Y, *Parts[2]) || !LexTryParseString(Seed, *Parts[3])
        || Kind != int32(EKalmalaWorldPopulationKind::Wildlife)) return NAME_None;
    for (const auto& Spawn : FKalmalaWorldPopulationLayout::BuildSpawnDescriptors(
        WorldGenerationConfig, FIntPoint(X, Y), EKalmalaWorldPopulationKind::Wildlife))
    {
        if (FKalmalaWorldPopulationLayout::GetPersistentSpawnId(Spawn) != SpawnId) continue;
        const EKalmalaBiome Biome = FKalmalaBiomeClassifier::Classify(
            FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, FVector2D(Spawn.Location)));
        EKalmalaWildlifeArchetype Archetype;
        if (!AKalmalaWildlifeSpawn::TryGetSpawnArchetypeForBiome(Spawn.SpawnSeed, Biome, Archetype)
            || Archetype != (bMoonBear ? EKalmalaWildlifeArchetype::MoonBear : EKalmalaWildlifeArchetype::Mireling)) return NAME_None;
        return FKalmalaGemstoneContract::SelectEnemyGem(bMoonBear, Biome, Seed ^ WorldGenerationConfig.WorldSeed);
    }
    return NAME_None;
}

const FKalmalaTreasureState* AKalmalaGameMode::PrepareWorldTreasure(const FKalmalaWorldDiscoveryDescriptor& Descriptor)
{
    if (!HasAuthority() || !PopulationSaveGame || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)
        || Descriptor.Kind != EKalmalaWorldDiscoveryKind::PointOfInterest) return nullptr;
    const auto Biome = FKalmalaBiomeClassifier::Classify(FKalmalaWorldFieldSampler::Sample(WorldGenerationConfig, FVector2D(Descriptor.Location)));
    if (Biome != EKalmalaBiome::Elderwood && Biome != EKalmalaBiome::ShimmeringLakes && Biome != EKalmalaBiome::ThunderMountains) return nullptr;
    const FString Id = TEXT("treasure:") + FKalmalaWorldPopulationLayout::GetPersistentDiscoveryId(Descriptor);
    bool bCanonical = false;
    for (const auto& Candidate : FKalmalaWorldPopulationLayout::BuildDiscoveryDescriptors(
        WorldGenerationConfig, Descriptor.SpatialKey, EKalmalaWorldDiscoveryKind::PointOfInterest))
        bCanonical |= Candidate.DefinitionId == Descriptor.DefinitionId && Candidate.DescriptorSeed == Descriptor.DescriptorSeed
            && Candidate.Ordinal == Descriptor.Ordinal && Candidate.Location.Equals(Descriptor.Location, 0.01f);
    if (!bCanonical) return nullptr;
    if (const auto* Existing = PopulationSaveGame->FindTreasure(Id)) return Existing;
    FKalmalaTreasureState State;
    State.Id = Id;
    State.Items = FKalmalaGemstoneContract::SelectTreasure(Biome, Descriptor.DescriptorSeed ^ WorldGenerationConfig.WorldSeed);
    auto* CandidateSave = DuplicateObject<UKalmalaWorldPopulationSaveGame>(PopulationSaveGame, this);
    if (!CandidateSave || !CandidateSave->SetTreasure(State) || !CandidateSave->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(CandidateSave, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0)) return nullptr;
    PopulationSaveGame = CandidateSave;
    return PopulationSaveGame->FindTreasure(Id);
}

bool AKalmalaGameMode::ClaimWorldTreasure(AKalmalaCharacter* Interactor, const FKalmalaWorldDiscoveryDescriptor& Descriptor)
{
    if (!HasAuthority() || !IsValid(Interactor) || !Interactor->HasAuthority() || Interactor->GetWorld() != GetWorld()
        || Interactor->GetHealth() <= 1.0f || FVector::DistSquared(Interactor->GetActorLocation(), Descriptor.Location) > FMath::Square(250.0f)) return false;
    const auto* Existing = PrepareWorldTreasure(Descriptor);
    auto* Inventory = Interactor->GetInventoryComponent();
    if (!Existing || Existing->bClaimed || !Inventory) return false;
    FKalmalaTreasureState Claimed = *Existing;
    const auto Before = Inventory->GetStacks();
    auto After = Before;
    FString Reason;
    for (const FName Item : Claimed.Items)
    {
        TArray<FKalmalaInventoryStack> Next;
        if (!UKalmalaInventoryComponent::BuildGrant(After, Item, 1, Next, Reason))
        {
            if (auto* Crafting = Interactor->FindComponentByClass<UKalmalaCraftingComponent>())
                Crafting->PublishInteractionResultFromServer(TEXT("Inventory is full; treasure remains"), false);
            return false;
        }
        After = MoveTemp(Next);
    }
    if (!Inventory->CanFitContents(After, Inventory->CountPackedTools(Interactor->GetCarriedToolInventory()))
        || !Inventory->TryCommitStacksFromServer(Before, After)) return false;
    Claimed.bClaimed = true;
    auto* CandidateSave = DuplicateObject<UKalmalaWorldPopulationSaveGame>(PopulationSaveGame, this);
    if (!CandidateSave || !CandidateSave->SetTreasure(Claimed) || !CandidateSave->MatchesWorld(WorldGenerationConfig)
        || !UGameplayStatics::SaveGameToSlot(CandidateSave, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0))
    {
        if (!Inventory->TryCommitStacksFromServer(After, Before)) UE_LOG(LogTemp, Error, TEXT("Treasure inventory rollback failed."));
        return false;
    }
    PopulationSaveGame = CandidateSave;
    if (auto* Crafting = Interactor->FindComponentByClass<UKalmalaCraftingComponent>())
        Crafting->PublishInteractionResultFromServer(Claimed.Items.IsEmpty() ? TEXT("Treasure chest is empty") : TEXT("Treasure collected"), true);
    return true;
}

bool AKalmalaGameMode::RecordMirelingDefeatReward(const FString& PersistentSpawnId, AKalmalaCharacter* CreditedAttacker)
{
    if (!HasAuthority() || !PopulationSaveGame || !PopulationSaveGame->MatchesWorld(WorldGenerationConfig)
        || PersistentSpawnId.IsEmpty() || !FKalmalaGemstoneContract::DuskDiamondSourceAccepted) return false;
    auto* CandidateSave = DuplicateObject<UKalmalaWorldPopulationSaveGame>(PopulationSaveGame, this);
    if (!CandidateSave) return false;
    if (IsValid(CreditedAttacker) && CreditedAttacker->HasAuthority() && CreditedAttacker->GetWorld() == GetWorld()
        && CreditedAttacker->GetPlayerState())
    {
        const FString Identity = KalmalaGameMode::GetAuthenticatedPlayerIdentity(CreditedAttacker->GetPlayerState());
        const FString ClaimId = TEXT("mireling-gem|") + PersistentSpawnId;
        if (!Identity.IsEmpty() && (!CandidateSave->TryQueuePendingWildlifeReward(ClaimId, Identity)
            || !CandidateSave->SetPendingGemReward(ClaimId, SelectEnemyGemForSpawn(PersistentSpawnId, false)))) return false;
    }
    CandidateSave->MarkDefeated(PersistentSpawnId);
    if (!CandidateSave->MatchesWorld(WorldGenerationConfig) || !UGameplayStatics::SaveGameToSlot(
        CandidateSave, KalmalaGameMode::PopulationSaveSlot(WorldGenerationConfig), 0)) return false;
    PopulationSaveGame = CandidateSave;
    return true;
}
