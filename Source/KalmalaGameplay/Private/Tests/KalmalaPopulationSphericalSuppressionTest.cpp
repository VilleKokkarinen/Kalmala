#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaCampfire.h"
#include "KalmalaCharacter.h"
#include "KalmalaConstructionActor.h"
#include "KalmalaGameMode.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPopulationSphericalSuppressionTest,
    "Kalmala.Gameplay.Population.SphericalCampSuppression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPopulationSphericalSuppressionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Population spherical-suppression fixture world created"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    if (!TestNotNull(TEXT("Population spherical-suppression GameMode created"), Mode)) return false;
    AKalmalaWorldGenerationGameState* WorldState = World->SpawnActor<AKalmalaWorldGenerationGameState>();
    if (!TestNotNull(TEXT("Dry world-generation state created"), WorldState)) return false;
    World->SetGameState(WorldState);
    FKalmalaWeatherState DryWeather;
    DryWeather.RefreshActivityLevel();
    WorldState->SetWeatherStateFromServer(DryWeather);

    const auto SpawnConstruction = [World](const FName Kit, const FString& Id,
        const FVector Location, const FKalmalaStandingLightState& LightState)
        -> AKalmalaConstructionActor*
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AKalmalaConstructionActor* Construction = World->SpawnActor<AKalmalaConstructionActor>(
            Location, FRotator::ZeroRotator, SpawnParameters);
        if (IsValid(Construction) && !Kit.IsNone())
        {
            Construction->InitializeFromServer(Kit, Id, FKalmalaCharcoalKilnState(),
                FKalmalaSmelterState(), FKalmalaGardenPlotState(), false, LightState);
        }
        return Construction;
    };

    const double Radius = FKalmalaMonsterSpawnTuning::CampSuppressionRadiusCm;
    AKalmalaConstructionActor* Bed = SpawnConstruction(
        TEXT("CampBedKit"), TEXT("camp-sphere-bed"), FVector::ZeroVector, FKalmalaStandingLightState());
    if (!TestNotNull(TEXT("Intact Camp Bed created"), Bed)) return false;
    TestTrue(TEXT("Camp Bed includes the exact 18 m horizontal boundary"),
        Mode->DoesPopulationPointOverlapCampSphere(FVector(Radius, 0.0, 0.0)));
    TestTrue(TEXT("Camp Bed uses a full 3D sphere at the exact vertical boundary"),
        Mode->DoesPopulationPointOverlapCampSphere(FVector(0.0, 0.0, Radius)));
    TestFalse(TEXT("Camp Bed excludes points beyond the sphere"),
        Mode->DoesPopulationPointOverlapCampSphere(FVector(Radius + 1.0, 0.0, 0.0)));
    Bed->Destroy();
    TestFalse(TEXT("Destroyed Camp Bed no longer suppresses"),
        Mode->DoesPopulationPointOverlapCampSphere(FVector::ZeroVector));

    AKalmalaConstructionActor* EmptyLight = SpawnConstruction(
        TEXT("StandingLightKit"), TEXT("camp-sphere-empty-light"), FVector(6000.0, 0.0, 0.0),
        FKalmalaStandingLightState());
    if (!TestNotNull(TEXT("Unfuelled Standing Light created"), EmptyLight)) return false;
    TestFalse(TEXT("An empty Standing Light does not suppress"),
        Mode->DoesPopulationPointOverlapCampSphere(EmptyLight->GetActorLocation()));

    FKalmalaStandingLightState LightFuel;
    TestTrue(TEXT("Standing Light fixture accepts its first fuel charge"), LightFuel.TryAddWood());
    AKalmalaConstructionActor* LitLight = SpawnConstruction(
        TEXT("StandingLightKit"), TEXT("camp-sphere-lit-light"), FVector(10000.0, 0.0, 0.0), LightFuel);
    if (!TestNotNull(TEXT("Fuelled Standing Light created"), LitLight)) return false;
    TestTrue(TEXT("A fuelled, server-lit Standing Light suppresses at its origin"),
        Mode->DoesPopulationPointOverlapCampSphere(LitLight->GetActorLocation()));
    FKalmalaWeatherState RainWeather;
    RainWeather.PrecipitationIntensity = 0.1f;
    RainWeather.RefreshActivityLevel();
    WorldState->SetWeatherStateFromServer(RainWeather);
    TestFalse(TEXT("Rain-exposed Standing Light is not a live flame source"),
        Mode->DoesPopulationPointOverlapCampSphere(LitLight->GetActorLocation()));
    WorldState->SetWeatherStateFromServer(DryWeather);

    const FVector FireLocation(18000.0, 0.0, 0.0);
    AKalmalaCharacter* Character = World->SpawnActor<AKalmalaCharacter>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Campfire owner character created"), Character)
        || !TestNotNull(TEXT("Campfire owner controller created"), Controller)) return false;
    Controller->Possess(Character);
    Character->SetActorLocation(FireLocation);

    FActorSpawnParameters FireSpawnParameters;
    FireSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AKalmalaCampfire* Fire = World->SpawnActor<AKalmalaCampfire>(
        FireLocation, FRotator::ZeroRotator, FireSpawnParameters);
    if (!TestNotNull(TEXT("Placed Campfire created"), Fire)) return false;
    Fire->InitializePaidFromServer(Character);
    TestFalse(TEXT("Fuelled but unlit Campfire does not suppress"),
        Mode->DoesPopulationPointOverlapCampSphere(FireLocation));
    TestTrue(TEXT("The existing server interaction lights the Campfire"), Fire->TryLightFromServer(Character));
    TestTrue(TEXT("Lit, fuelled Campfire suppresses at the inclusive 18 m edge"),
        Mode->DoesPopulationPointOverlapCampSphere(FireLocation + FVector(0.0, 0.0, Radius)));
    Fire->AdvanceFromServer(0.0f, AKalmalaCampfire::RainThreshold, 0.0f);
    TestFalse(TEXT("Smouldering Campfire does not suppress"),
        Mode->DoesPopulationPointOverlapCampSphere(FireLocation));
    Fire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("Dry weather restores a lit fuelled Campfire source"),
        Mode->DoesPopulationPointOverlapCampSphere(FireLocation));
    Fire->AdvanceFromServer(Fire->GetFuelSeconds(), 0.0f, 0.0f);
    TestFalse(TEXT("Fuel exhaustion stops Campfire suppression"),
        Mode->DoesPopulationPointOverlapCampSphere(FireLocation));
    Fire->Destroy();
    TestFalse(TEXT("Destroyed Campfire no longer suppresses"),
        Mode->DoesPopulationPointOverlapCampSphere(FireLocation));

    AKalmalaConstructionActor* UnlistedStation = SpawnConstruction(
        TEXT("ForgeKit"), TEXT("camp-sphere-unlisted-station"), FVector(30000.0, 0.0, 0.0),
        FKalmalaStandingLightState());
    if (!TestNotNull(TEXT("Unlisted station fixture created"), UnlistedStation)) return false;
    TestFalse(TEXT("Workbench and Forge stations do not emit S24 spherical fields"),
        Mode->DoesPopulationPointOverlapCampSphere(UnlistedStation->GetActorLocation()));
    AKalmalaConstructionActor* OtherPiece = SpawnConstruction(
        TEXT("WallKit"), TEXT("camp-sphere-wall"), FVector(35000.0, 0.0, 0.0),
        FKalmalaStandingLightState());
    if (!TestNotNull(TEXT("Other construction fixture created"), OtherPiece)) return false;
    TestFalse(TEXT("Other construction pieces do not suppress"),
        Mode->DoesPopulationPointOverlapCampSphere(OtherPiece->GetActorLocation()));
    AActor* Cosmetic = World->SpawnActor<AActor>(FVector(40000.0, 0.0, 0.0), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Cosmetic actor fixture created"), Cosmetic)) return false;
    TestFalse(TEXT("Cosmetics do not suppress"),
        Mode->DoesPopulationPointOverlapCampSphere(Cosmetic->GetActorLocation()));

    const FVector Invalid(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0);
    TestTrue(TEXT("Invalid candidate input fails closed"),
        Mode->DoesPopulationPointOverlapCampSphere(Invalid));
    return true;
}

#endif
