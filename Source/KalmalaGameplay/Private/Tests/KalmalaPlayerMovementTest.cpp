#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaEnvironmentalExposureSampler.h"
#include "KalmalaOceanSampler.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaShimmeringLakeSampler.h"
#include "KalmalaWorldGenerationConfig.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerMovementTest, "Kalmala.Gameplay.Movement.SprintSavedMoves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlayerMovementTest::RunTest(const FString& Parameters)
{
    FSavedMove_Kalmala Move;
    Move.Clear();
    Move.bSavedSprint = true;
    Move.bPressedJump = true;
    const uint8 Flags = Move.GetCompressedFlags();
    TestTrue(TEXT("Sprint intent is encoded in the custom move flag"), (Flags & FSavedMove_Character::FLAG_Custom_0) != 0);
    TestTrue(TEXT("Jump retains its built-in network flag alongside sprint"), (Flags & FSavedMove_Character::FLAG_JumpPressed) != 0);
    auto* Movement = NewObject<UKalmalaCharacterMovementComponent>();
    Movement->UpdateFromCompressedFlags(Flags);
    TestTrue(TEXT("Server movement decodes sprint intent"), Movement->IsSprintRequested());
    Movement->UpdateFromCompressedFlags(0);
    TestFalse(TEXT("Release is decoded from the next move"), Movement->IsSprintRequested());
    FSavedMovePtr Released(new FSavedMove_Kalmala());
    Released->Clear();
    TestFalse(TEXT("Sprint and release moves cannot be combined and lose the transition"), Move.CanCombineWith(Released, nullptr, 0.125f));
    Move.Clear();
    TestFalse(TEXT("Recycled saved moves cannot keep a stale sprint flag"), Move.bSavedSprint);
    TestEqual(TEXT("Generated ocean swim uses a dedicated custom movement mode"), UKalmalaCharacterMovementComponent::GeneratedOceanSwimmingMode, static_cast<uint8>(1));
    TestFalse(TEXT("Shallow shore does not enter swimming"), UKalmalaCharacterMovementComponent::ShouldEnterGeneratedOcean(99.9f));
    TestTrue(TEXT("Deep shared sea enters swimming"), UKalmalaCharacterMovementComponent::ShouldEnterGeneratedOcean(100.0f));
    TestFalse(TEXT("Shore hysteresis retains swimming at 75 cm"), UKalmalaCharacterMovementComponent::ShouldReturnToLand(75.0f));
    TestTrue(TEXT("Shore hysteresis returns to land below 75 cm"), UKalmalaCharacterMovementComponent::ShouldReturnToLand(74.9f));

    const FKalmalaWorldGenerationConfig RiverConfig{418ull};
    const FVector2D RiverPosition(679237.9f, -16414.3f);
    const FKalmalaRegionalSample RiverRegion = FKalmalaRegionalGeneration::Sample(RiverConfig, RiverPosition);
    const FKalmalaOceanSample RiverSeaDepth = FKalmalaOceanSampler::Sample(RiverConfig, RiverPosition);
    const bool bRiverRefreshesWetStatus = RiverSeaDepth.IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(RiverConfig, RiverPosition);
    TestEqual(TEXT("Fixed seed-418 river keeps its sea-level surface"), RiverRegion.WaterLevel, 0.0f);
    TestTrue(TEXT("Fixed river sample remains a carved wet regional channel"), RiverRegion.bHasWater
        && RiverRegion.Height < RiverRegion.WaterLevel && RiverRegion.BasinWeight == 0.0f
        && RiverRegion.RiverWeight > 0.0f);
    TestTrue(TEXT("The server water gate still refreshes Wet in the sea-level river"), bRiverRefreshesWetStatus);
    TestTrue(TEXT("The shallow river remains below the generated-ocean swim threshold"),
        RiverSeaDepth.IsWater() && RiverSeaDepth.WaterDepth < 100.0f
        && !UKalmalaCharacterMovementComponent::ShouldEnterGeneratedOcean(RiverSeaDepth.WaterDepth));

    const FKalmalaWorldGenerationConfig OceanConfig{419ull};
    const FVector2D OceanPosition(-243087.4f, -970004.9f);
    const FKalmalaRegionalSample OceanRegion = FKalmalaRegionalGeneration::Sample(OceanConfig, OceanPosition);
    const FKalmalaOceanSample DeepOcean = FKalmalaOceanSampler::Sample(OceanConfig, OceanPosition);
    const bool bOceanRefreshesWetStatus = DeepOcean.IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(OceanConfig, OceanPosition);
    TestEqual(TEXT("Fixed seed-419 ocean retains the sea-level plane"), OceanRegion.WaterLevel, 0.0f);
    TestTrue(TEXT("Fixed ocean sample remains wet without basin support"), OceanRegion.bHasWater
        && OceanRegion.Height < OceanRegion.WaterLevel && OceanRegion.BasinWeight == 0.0f);
    TestTrue(TEXT("Deep generated ocean still refreshes Wet on the server"), bOceanRefreshesWetStatus);
    TestTrue(TEXT("Deep generated ocean still enters swimming at the shared threshold"), DeepOcean.WaterDepth >= 100.0f
        && UKalmalaCharacterMovementComponent::ShouldEnterGeneratedOcean(DeepOcean.WaterDepth));

    const FVector2D FormerBasinPosition(100830.5f, -143038.6f);
    const FKalmalaRegionalSample FormerBasin = FKalmalaRegionalGeneration::Sample(RiverConfig, FormerBasinPosition);
    const FKalmalaOceanSample FormerBasinOcean = FKalmalaOceanSampler::Sample(RiverConfig, FormerBasinPosition);
    const FKalmalaEnvironmentalExposureSample FormerBasinExposure =
        FKalmalaEnvironmentalExposureSampler::Sample(RiverConfig, FormerBasinPosition);
    const bool bFormerBasinRefreshesWetStatus = FormerBasinOcean.IsWater()
        || FKalmalaShimmeringLakeSampler::IsWater(RiverConfig, FormerBasinPosition);
    TestFalse(TEXT("Former basin remains dry in regional output"), FormerBasin.bHasWater);
    TestEqual(TEXT("Former basin has no analytic support"), FormerBasin.BasinWeight, 0.0f);
    TestFalse(TEXT("Former basin cannot refresh the server Wet status"), bFormerBasinRefreshesWetStatus);
    TestFalse(TEXT("Former basin cannot add shoreline exposure wetness"), FormerBasinExposure.bIsShoreline);
    TestEqual(TEXT("Former basin shoreline wetness remains zero"), FormerBasinExposure.ShorelineWetness, 0.0f);
    return true;
}
#endif
