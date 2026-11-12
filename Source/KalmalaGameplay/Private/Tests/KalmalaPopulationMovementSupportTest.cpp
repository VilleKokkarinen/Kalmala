#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaGameMode.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldBounds.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPopulationLayout.h"
#include "KalmalaWolfPackContract.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationMovementSupportTest,
    "Kalmala.Gameplay.Population.MovementSupport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationMovementSupportTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population movement-support fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population movement-support GameMode created"), Mode)) return false;
    Mode->WorldGenerationConfig.WorldSeed = 418;

    struct FSupportPoint
    {
        FVector Location;
        EKalmalaBiome Biome = EKalmalaBiome::Meadows;
        bool bIsWater = false;
        bool bIsTraversable = false;
    };

    TArray<FSupportPoint> SupportPoints;
    constexpr int32 SearchSpacingCentimetres = 25000;
    const int32 SearchExtentCentimetres = static_cast<int32>(FKalmalaWorldBounds::Radius);
    const FKalmalaWorldGenerationConfig LandConfig = Mode->WorldGenerationConfig;
    for (int32 Y = -SearchExtentCentimetres + SearchSpacingCentimetres;
        Y < SearchExtentCentimetres; Y += SearchSpacingCentimetres)
    {
        for (int32 X = -SearchExtentCentimetres + SearchSpacingCentimetres;
            X < SearchExtentCentimetres; X += SearchSpacingCentimetres)
        {
            const FVector2D Position(X, Y);
            if (!FKalmalaWorldBounds::Contains(LandConfig, Position)) continue;

            const float GroundHeight = FKalmalaTerrainHeightSampler::SampleHeight(LandConfig, Position);
            const FVector SurfaceNormal = FKalmalaTerrainHeightSampler::SampleSurfaceNormal(LandConfig, Position);
            if (!FMath::IsFinite(GroundHeight) || !FMath::IsFinite(SurfaceNormal.Z)) continue;

            const FKalmalaOceanSample Ocean = FKalmalaOceanSampler::Sample(LandConfig, Position);
            FSupportPoint& Point = SupportPoints.AddDefaulted_GetRef();
            Point.Location = FVector(Position.X, Position.Y, GroundHeight);
            Point.Biome = FKalmalaBiomeClassifier::Classify(
                FKalmalaWorldFieldSampler::Sample(LandConfig, Position));
            Point.bIsWater = Ocean.IsWater()
                || FKalmalaShimmeringLakeSampler::IsWater(LandConfig, Position);
            Point.bIsTraversable = SurfaceNormal.Z >= 0.82f;
        }
    }

    constexpr int32 DryWaterState = 0;
    constexpr int32 WetWaterState = 1;
    constexpr int32 AnyTraversabilityState = -1;
    constexpr int32 SteepTraversabilityState = 0;
    constexpr int32 TraversableState = 1;
    const auto FindPosition = [&SupportPoints](const EKalmalaBiome RequiredBiome,
        const int32 WaterState, const int32 TraversabilityState, FVector& OutLocation)
    {
        for (const FSupportPoint& Point : SupportPoints)
        {
            if (Point.Biome != RequiredBiome
                || (WaterState == DryWaterState && Point.bIsWater)
                || (WaterState == WetWaterState && !Point.bIsWater)
                || (TraversabilityState == SteepTraversabilityState && Point.bIsTraversable)
                || (TraversabilityState == TraversableState && !Point.bIsTraversable))
            {
                continue;
            }
            OutLocation = Point.Location;
            return true;
        }
        return false;
    };

    FVector MeadowPosition;
    FVector ElderwoodPosition;
    FVector TundraPosition;
    FVector SteepTundraPosition;
    FVector ThunderPosition;
    FVector SteepThunderPosition;
    FVector MirePosition;
    FVector LakesPosition;
    FVector OceanWaterPosition;
    const bool bFoundRuleFixtures =
        FindPosition(EKalmalaBiome::Meadows, DryWaterState, AnyTraversabilityState, MeadowPosition)
        && FindPosition(EKalmalaBiome::Elderwood, DryWaterState, AnyTraversabilityState, ElderwoodPosition)
        && FindPosition(EKalmalaBiome::FreezingTundra, DryWaterState, TraversableState, TundraPosition)
        && FindPosition(EKalmalaBiome::ThunderMountains, DryWaterState, TraversableState, ThunderPosition)
        && FindPosition(EKalmalaBiome::MossyMire, DryWaterState, AnyTraversabilityState, MirePosition)
        && FindPosition(EKalmalaBiome::ShimmeringLakes, DryWaterState, AnyTraversabilityState, LakesPosition)
        && FindPosition(EKalmalaBiome::Ocean, WetWaterState, AnyTraversabilityState, OceanWaterPosition);
    if (!TestTrue(TEXT("Deterministic dry, wet, biome, and traversability samples exist"), bFoundRuleFixtures))
    {
        return false;
    }
    const bool bFoundSteepTundra = FindPosition(
        EKalmalaBiome::FreezingTundra, DryWaterState, SteepTraversabilityState, SteepTundraPosition);
    const bool bFoundSteepThunder = FindPosition(
        EKalmalaBiome::ThunderMountains, DryWaterState, SteepTraversabilityState, SteepThunderPosition);

    const auto Supports = [&LandConfig](const EKalmalaWildlifeArchetype Archetype,
        const FVector& Position, const FVector& SpawnOrigin)
    {
        return AKalmalaWildlifeSpawn::IsPopulationSpawnPositionSupported(
            LandConfig, Archetype, Position, SpawnOrigin);
    };

    const EKalmalaWildlifeArchetype S20OnlyArchetypes[] = {
        EKalmalaWildlifeArchetype::Mireling,
        EKalmalaWildlifeArchetype::Boar,
        EKalmalaWildlifeArchetype::Deer,
        EKalmalaWildlifeArchetype::MosquitoSwarm
    };
    for (const EKalmalaWildlifeArchetype Archetype : S20OnlyArchetypes)
    {
        TestTrue(TEXT("Species relying on S20 accept the real dry Meadow sample"),
            Supports(Archetype, MeadowPosition, MeadowPosition));
    }
    TestTrue(TEXT("Rat movement support accepts dry ground"),
        Supports(EKalmalaWildlifeArchetype::Rat, MeadowPosition, MeadowPosition));
    TestFalse(TEXT("Rat movement support rejects Ocean water"),
        Supports(EKalmalaWildlifeArchetype::Rat, OceanWaterPosition, OceanWaterPosition));

    const EKalmalaWildlifeArchetype ElderwoodArchetypes[] = {
        EKalmalaWildlifeArchetype::Badger,
        EKalmalaWildlifeArchetype::MoonBear
    };
    for (const EKalmalaWildlifeArchetype Archetype : ElderwoodArchetypes)
    {
        TestTrue(TEXT("Elderwood species accept dry Elderwood"),
            Supports(Archetype, ElderwoodPosition, ElderwoodPosition));
        TestFalse(TEXT("Elderwood species reject a different dry biome"),
            Supports(Archetype, MeadowPosition, MeadowPosition));
    }

    const EKalmalaWildlifeArchetype TundraArchetypes[] = {
        EKalmalaWildlifeArchetype::Wolf,
        EKalmalaWildlifeArchetype::Lynx,
        EKalmalaWildlifeArchetype::PolarBear
    };
    for (const EKalmalaWildlifeArchetype Archetype : TundraArchetypes)
    {
        TestTrue(TEXT("Tundra species accept dry traversable Tundra"),
            Supports(Archetype, TundraPosition, TundraPosition));
        TestFalse(TEXT("Tundra species reject a different dry biome"),
            Supports(Archetype, MeadowPosition, MeadowPosition));
        if (bFoundSteepTundra)
        {
            TestFalse(TEXT("Tundra species reject terrain below the traversal threshold"),
                Supports(Archetype, SteepTundraPosition, SteepTundraPosition));
        }
    }

    TestFalse(TEXT("Mossy Mire habitat support rejects water"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, true));
    TestFalse(TEXT("Thunder Mountains habitat support rejects water"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
            EKalmalaBiome::ThunderMountains, true, true));
    TestFalse(TEXT("Thunder Mountains habitat support rejects steep terrain"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
            EKalmalaBiome::ThunderMountains, false, false));
    const EKalmalaWildlifeArchetype ThunderArchetypes[] = {
        EKalmalaWildlifeArchetype::HornedGoat,
        EKalmalaWildlifeArchetype::ThunderSentinel
    };
    for (const EKalmalaWildlifeArchetype Archetype : ThunderArchetypes)
    {
        TestTrue(TEXT("Thunder Mountains species accept traversable dry terrain"),
            Supports(Archetype, ThunderPosition, ThunderPosition));
        if (bFoundSteepThunder)
        {
            TestFalse(TEXT("Thunder Mountains species reject terrain below the traversal threshold"),
                Supports(Archetype, SteepThunderPosition, SteepThunderPosition));
        }
        TestFalse(TEXT("Thunder Mountains species reject a different dry biome"),
            Supports(Archetype, MeadowPosition, MeadowPosition));
    }

    TestTrue(TEXT("Peat Husk support accepts dry Mossy Mire"),
        Supports(EKalmalaWildlifeArchetype::PeatHusk, MirePosition, MirePosition));
    TestFalse(TEXT("Peat Husk support rejects a different dry biome"),
        Supports(EKalmalaWildlifeArchetype::PeatHusk, MeadowPosition, MeadowPosition));

    const FVector BeyondSnakeLeash = LakesPosition
        + FVector(AKalmalaWildlifeSpawn::SnakeSpawnLeashCentimetres + 1.0f, 0.0, 0.0);
    TestTrue(TEXT("Death Adder support accepts dry Mossy Mire at its spawn origin"),
        Supports(EKalmalaWildlifeArchetype::DeathAdder, MirePosition, MirePosition));
    TestFalse(TEXT("Death Adder support rejects the wrong biome"),
        Supports(EKalmalaWildlifeArchetype::DeathAdder, LakesPosition, LakesPosition));
    TestFalse(TEXT("Death Adder support enforces its spawn leash"),
        Supports(EKalmalaWildlifeArchetype::DeathAdder, MirePosition, BeyondSnakeLeash));
    const EKalmalaWildlifeArchetype LakeSnakes[] = {
        EKalmalaWildlifeArchetype::Viper,
        EKalmalaWildlifeArchetype::GiantSnake
    };
    for (const EKalmalaWildlifeArchetype Archetype : LakeSnakes)
    {
        TestTrue(TEXT("Lake snake support accepts dry Shimmering Lakes at its spawn origin"),
            Supports(Archetype, LakesPosition, LakesPosition));
        TestFalse(TEXT("Lake snake support rejects a different dry biome"),
            Supports(Archetype, MirePosition, MirePosition));
        TestFalse(TEXT("Lake snake support enforces its spawn leash"),
            Supports(Archetype, LakesPosition, BeyondSnakeLeash));
    }

    TestTrue(TEXT("Mossy Mire habitat support accepts dry land"),
        AKalmalaWildlifeSpawn::IsMossyMireSpawnHabitatAllowed(EKalmalaBiome::MossyMire, false));
    TestTrue(TEXT("Thunder Mountains habitat support accepts dry traversable terrain"),
        AKalmalaWildlifeSpawn::IsThunderMountainsSpawnHabitatAllowed(
            EKalmalaBiome::ThunderMountains, false, true));

    FKalmalaWorldPopulationSpawn BoarDescriptor;
    bool bFoundBoarDescriptor = false;
    for (const FSupportPoint& Point : SupportPoints)
    {
        if (Point.Biome != EKalmalaBiome::Meadows
            && Point.Biome != EKalmalaBiome::ShimmeringLakes
            && Point.Biome != EKalmalaBiome::Elderwood)
        {
            continue;
        }

        FKalmalaWorldPopulationSpawn Candidate;
        Candidate.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Candidate.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(Point.Location));
        Candidate.SpawnSeed = 0;
        Candidate.Location = Point.Location;
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        if (Mode->DoesPopulationDescriptorPassSpeciesHabitat(Candidate, &Archetype)
            && Archetype == EKalmalaWildlifeArchetype::Boar
            && Mode->DoesPopulationDescriptorPassMovementSupport(Candidate))
        {
            BoarDescriptor = Candidate;
            bFoundBoarDescriptor = true;
            break;
        }
    }
    if (!TestTrue(TEXT("A valid land descriptor passes the S22c movement-support gate"),
        bFoundBoarDescriptor)) return false;

    TArray<FKalmalaWildlifeBodyCollisionShape> BoarShapes;
    if (!TestTrue(TEXT("Boar exposes its placement body profile"),
        AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            EKalmalaWildlifeArchetype::Boar, BoarShapes))) return false;
    FVector BoarActorLocation;
    if (!TestTrue(TEXT("Boar has an actual proposed actor location"),
        Mode->TryGetPopulationActorPlacementLocation(
            BoarDescriptor, EKalmalaWildlifeArchetype::Boar, BoarShapes, BoarActorLocation))) return false;
    TestTrue(TEXT("The real movement helper supports the proposed Boar actor location"),
        Supports(EKalmalaWildlifeArchetype::Boar, BoarActorLocation, BoarActorLocation));
    TestTrue(TEXT("Reef Maw rejects a position outside world bounds"),
        !AKalmalaWildlifeSpawn::IsPopulationSpawnPositionSupported(LandConfig,
            EKalmalaWildlifeArchetype::ReefMaw,
            FVector(FKalmalaWorldBounds::Radius + 1.0, 0.0, 0.0), FVector::ZeroVector));

    FKalmalaWorldPopulationSpawn WolfPack;
    bool bFoundWolfPack = false;
    for (const FSupportPoint& Point : SupportPoints)
    {
        if (Point.Biome != EKalmalaBiome::FreezingTundra || Point.bIsWater || !Point.bIsTraversable)
        {
            continue;
        }

        FKalmalaWorldPopulationSpawn Candidate;
        Candidate.Kind = EKalmalaWorldPopulationKind::Wildlife;
        Candidate.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(FVector2D(Point.Location));
        Candidate.SpawnSeed = 5;
        Candidate.Location = Point.Location;
        EKalmalaWildlifeArchetype Archetype = EKalmalaWildlifeArchetype::Mireling;
        if (Mode->DoesPopulationDescriptorPassSpeciesHabitat(Candidate, &Archetype)
            && Archetype == EKalmalaWildlifeArchetype::Wolf
            && Mode->DoesPopulationDescriptorPassMovementSupport(Candidate))
        {
            WolfPack = Candidate;
            bFoundWolfPack = true;
            break;
        }
    }
    if (!TestTrue(TEXT("A generated Tundra Wolf pack passes S22c for all members"), bFoundWolfPack))
    {
        return false;
    }

    TArray<FKalmalaWildlifeBodyCollisionShape> WolfShapes;
    if (!TestTrue(TEXT("Wolf exposes its placement body profile"),
        AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            EKalmalaWildlifeArchetype::Wolf, WolfShapes))) return false;
    TArray<FVector> ExpectedWolfLocations;
    ExpectedWolfLocations.Reserve(FKalmalaWolfPackContract::MemberCount);
    for (int32 MemberIndex = 0; MemberIndex < FKalmalaWolfPackContract::MemberCount; ++MemberIndex)
    {
        FKalmalaWorldPopulationSpawn MemberDescriptor;
        if (!TestTrue(FString::Printf(TEXT("Wolf member %d descriptor is generated"), MemberIndex),
            FKalmalaWolfPackContract::TryBuildMemberSpawn(
                Mode->WorldGenerationConfig, WolfPack, MemberIndex, MemberDescriptor))) return false;
        FVector MemberLocation;
        if (!TestTrue(FString::Printf(TEXT("Wolf member %d has an actual actor location"), MemberIndex),
            Mode->TryGetPopulationActorPlacementLocation(
                MemberDescriptor, EKalmalaWildlifeArchetype::Wolf, WolfShapes, MemberLocation))) return false;
        TestTrue(FString::Printf(TEXT("Wolf member %d is supported by the shared movement helper"), MemberIndex),
            Supports(EKalmalaWildlifeArchetype::Wolf, MemberLocation, MemberLocation));
        ExpectedWolfLocations.Add(MemberLocation);
    }
    TArray<FVector> ObservedWolfLocations;
    bool bAllWolfMovementResultsAllowed = true;
    Mode->PopulationMovementSupportForTests = [&ObservedWolfLocations, &bAllWolfMovementResultsAllowed](
        const FVector& Location, const bool bMovementSupported)
    {
        ObservedWolfLocations.Add(Location);
        bAllWolfMovementResultsAllowed = bAllWolfMovementResultsAllowed && bMovementSupported;
        return bMovementSupported;
    };
    TestTrue(TEXT("The valid Wolf pack passes movement-support composition"),
        Mode->DoesPopulationDescriptorPassMovementSupport(WolfPack));
    TestEqual(TEXT("Wolf composition checks every generated member exactly once"),
        ObservedWolfLocations.Num(), FKalmalaWolfPackContract::MemberCount);
    for (int32 MemberIndex = 0; MemberIndex < ExpectedWolfLocations.Num(); ++MemberIndex)
    {
        TestTrue(FString::Printf(TEXT("Wolf composition checks actual member %d location"), MemberIndex),
            ObservedWolfLocations.IsValidIndex(MemberIndex)
                && ObservedWolfLocations[MemberIndex].Equals(ExpectedWolfLocations[MemberIndex]));
    }
    TestTrue(TEXT("The real movement helper accepts every generated Wolf member"),
        bAllWolfMovementResultsAllowed);
    Mode->PopulationMovementSupportForTests = nullptr;

    TArray<FVector> ObservedLocations;
    bool bObservedRealSupport = false;
    Mode->PopulationMovementSupportForTests = [&ObservedLocations, &bObservedRealSupport](
        const FVector& Location, const bool bMovementSupported)
    {
        ObservedLocations.Add(Location);
        bObservedRealSupport = bMovementSupported;
        return bMovementSupported;
    };
    TestTrue(TEXT("A valid Boar passes land movement-support composition"),
        Mode->DoesPopulationDescriptorPassMovementSupport(BoarDescriptor));
    TestEqual(TEXT("Land composition checks the proposed Boar actor location once"),
        ObservedLocations.Num(), 1);
    TestTrue(TEXT("Land composition passes the actual Boar actor location"),
        ObservedLocations.Num() == 1 && ObservedLocations[0].Equals(BoarActorLocation));
    TestTrue(TEXT("The shared helper accepted the Boar position before the test observation"),
        bObservedRealSupport);

    bool bRejectedBoarLocationWasSupported = false;
    Mode->PopulationMovementSupportForTests = [&BoarActorLocation, &bRejectedBoarLocationWasSupported](
        const FVector& Location, const bool bMovementSupported)
    {
        if (Location.Equals(BoarActorLocation))
        {
            bRejectedBoarLocationWasSupported = bMovementSupported;
            return false;
        }
        return bMovementSupported;
    };
    TestFalse(TEXT("S22c rejects a land descriptor when movement support denies its actual actor position"),
        Mode->DoesPopulationDescriptorPassMovementSupport(BoarDescriptor));
    TestTrue(TEXT("The real helper evaluated the Boar coordinate before the injected rejection"),
        bRejectedBoarLocationWasSupported);

    for (int32 FailedMemberIndex = 0; FailedMemberIndex < ExpectedWolfLocations.Num(); ++FailedMemberIndex)
    {
        ObservedLocations.Reset();
        bool bRejectedMemberWasSupported = false;
        const FVector RejectedLocation = ExpectedWolfLocations[FailedMemberIndex];
        Mode->PopulationMovementSupportForTests = [
            &ObservedLocations, &RejectedLocation, &bRejectedMemberWasSupported,
            FailedMemberIndex](const FVector& Location, const bool bMovementSupported)
        {
            const int32 CurrentMemberIndex = ObservedLocations.Num();
            ObservedLocations.Add(Location);
            if (CurrentMemberIndex == FailedMemberIndex)
            {
                bRejectedMemberWasSupported = bMovementSupported;
                return false;
            }
            return bMovementSupported;
        };

        TestFalse(FString::Printf(TEXT("Unsupported Wolf member %d rejects the entire descriptor"),
            FailedMemberIndex), Mode->DoesPopulationDescriptorPassMovementSupport(WolfPack));
        TestEqual(FString::Printf(TEXT("Wolf composition reaches member %d before rejecting it"),
            FailedMemberIndex), ObservedLocations.Num(), FailedMemberIndex + 1);
        TestTrue(FString::Printf(TEXT("Wolf member %d is checked at its generated actual location"),
            FailedMemberIndex), ObservedLocations.IsValidIndex(FailedMemberIndex)
                && ObservedLocations[FailedMemberIndex].Equals(RejectedLocation));
        TestTrue(FString::Printf(TEXT("The real helper supports Wolf member %d before its result is rejected"),
            FailedMemberIndex), bRejectedMemberWasSupported);
    }

    Mode->PopulationMovementSupportForTests = nullptr;
    Mode->WorldGenerationConfig.WorldSeed = 7;
    const FKalmalaWorldGenerationConfig AquaticConfig = Mode->WorldGenerationConfig;
    FKalmalaWorldPopulationSpawn ReefMawDescriptor;
    bool bFoundReefMaw = false;
    const float MinimumFullBodyDepth = AKalmalaWildlifeSpawn::ReefMawSpawnDepthCentimetres
        + AKalmalaWildlifeSpawn::ReefMawBodyCollisionRadiusCentimetres;
    for (int32 Y = -SearchExtentCentimetres + SearchSpacingCentimetres;
        Y < SearchExtentCentimetres && !bFoundReefMaw; Y += SearchSpacingCentimetres)
    {
        for (int32 X = -SearchExtentCentimetres + SearchSpacingCentimetres;
            X < SearchExtentCentimetres && !bFoundReefMaw; X += SearchSpacingCentimetres)
        {
            const FVector2D Position(X, Y);
            FKalmalaOceanSample Ocean;
            if (!AKalmalaWildlifeSpawn::IsReefMawPositionNavigable(AquaticConfig, Position, &Ocean)
                || Ocean.WaterDepth < MinimumFullBodyDepth)
            {
                continue;
            }

            FKalmalaWorldPopulationSpawn Candidate;
            Candidate.Kind = EKalmalaWorldPopulationKind::Wildlife;
            Candidate.SpatialKey = FKalmalaWorldPopulationLayout::GetSpatialKey(Position);
            Candidate.SpawnSeed = 9;
            Candidate.Location = FVector(Position.X, Position.Y,
                FKalmalaTerrainHeightSampler::SampleHeight(AquaticConfig, Position));
            if (Mode->DoesPopulationDescriptorPassMovementSupport(Candidate))
            {
                ReefMawDescriptor = Candidate;
                bFoundReefMaw = true;
            }
        }
    }
    if (!TestTrue(TEXT("A valid Reef Maw descriptor passes S21 and S22c aquatic support"), bFoundReefMaw))
    {
        return false;
    }

    TArray<FKalmalaWildlifeBodyCollisionShape> ReefMawShapes;
    if (!TestTrue(TEXT("Reef Maw exposes its placement body profile"),
        AKalmalaWildlifeSpawn::GetBodyCollisionShapesForArchetype(
            EKalmalaWildlifeArchetype::ReefMaw, ReefMawShapes))) return false;
    FVector ReefMawActorLocation;
    if (!TestTrue(TEXT("Reef Maw has an actual proposed actor location"),
        Mode->TryGetPopulationActorPlacementLocation(
            ReefMawDescriptor, EKalmalaWildlifeArchetype::ReefMaw,
            ReefMawShapes, ReefMawActorLocation))) return false;
    TestTrue(TEXT("The shared movement helper supports the Reef Maw's deep-water actor position"),
        AKalmalaWildlifeSpawn::IsPopulationSpawnPositionSupported(
            AquaticConfig, EKalmalaWildlifeArchetype::ReefMaw,
            ReefMawActorLocation, ReefMawActorLocation));

    ObservedLocations.Reset();
    Mode->PopulationMovementSupportForTests = [&ObservedLocations](
        const FVector& Location, const bool bMovementSupported)
    {
        ObservedLocations.Add(Location);
        return bMovementSupported;
    };
    TestTrue(TEXT("The valid Reef Maw passes aquatic movement-support composition"),
        Mode->DoesPopulationDescriptorPassMovementSupport(ReefMawDescriptor));
    TestEqual(TEXT("Aquatic composition checks the proposed Reef Maw actor position once"),
        ObservedLocations.Num(), 1);
    TestTrue(TEXT("Aquatic composition uses the proposed Reef Maw actor position"),
        ObservedLocations.Num() == 1 && ObservedLocations[0].Equals(ReefMawActorLocation));

    bool bRejectedReefMawLocationWasSupported = false;
    Mode->PopulationMovementSupportForTests = [&ReefMawActorLocation, &bRejectedReefMawLocationWasSupported](
        const FVector& Location, const bool bMovementSupported)
    {
        if (Location.Equals(ReefMawActorLocation))
        {
            bRejectedReefMawLocationWasSupported = bMovementSupported;
            return false;
        }
        return bMovementSupported;
    };
    TestFalse(TEXT("S22c rejects Reef Maw when movement support denies its actual actor position"),
        Mode->DoesPopulationDescriptorPassMovementSupport(ReefMawDescriptor));
    TestTrue(TEXT("The real helper evaluated the Reef Maw coordinate before the injected rejection"),
        bRejectedReefMawLocationWasSupported);
    Mode->PopulationMovementSupportForTests = nullptr;

    return true;
}

#endif
