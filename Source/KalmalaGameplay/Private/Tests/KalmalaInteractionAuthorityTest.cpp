#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaInteractionTestActor.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCampfire.h"
#include "KalmalaCampfireWeatherResponse.h"
#include "KalmalaExposureResponse.h"
#include "KalmalaHarvestNode.h"
#include "KalmalaHazardSpawn.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaAutomaticPickupTest, "Kalmala.Gameplay.Pickup.AutomaticRadius",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaAutomaticPickupTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Pickup fixture world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->InitializeActorsForPlay(FURL());
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>(FVector(0, 0, 88), FRotator::ZeroRotator);
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Node = World->SpawnActor<AKalmalaHarvestNode>();
    if (!Pawn || !Controller || !Node) return false;
    Controller->Possess(Pawn);
    Pawn->DispatchBeginPlay();
    Controller->DispatchBeginPlay();
    Node->InitializeDiscoveryServer(TEXT("pickup-radius-fixture"), FVector(100, 0, 0));
    FName Item;
    int32 Amount;
    TestTrue(TEXT("Loose material exposes pickup metadata"), Node->GetPickupInfo(Item, Amount));
    TestEqual(TEXT("Loose material pickup has one unit"), Amount, 1);
    Pawn->UpdateAutomaticPickupFromServer(0.25f);
    TestFalse(TEXT("Preference disabled leaves loose material intact"), Node->IsHarvested());
    Pawn->ServerSetAutomaticPickupEnabled(true);
    TestTrue(TEXT("Owner preference reaches the server on the initialized world"), Pawn->IsAutomaticPickupEnabled());
    Pawn->SetActorLocation(FVector(-100, 0, 88));
    TestFalse(TEXT("Automatic pickup uses the smaller fixed radius"), Node->TryAutomaticPickupFromServer(Pawn));
    Pawn->SetActorLocation(FVector(0, 0, 88));
    auto* Wall = World->SpawnActor<AActor>();
    auto* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(8, 25, 60));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(50, 0, 44));
    TestFalse(TEXT("Walls prevent automatic pickup through geometry"), Node->TryAutomaticPickupFromServer(Pawn));
    Wall->Destroy();
    auto* Inventory = Pawn->GetInventoryComponent();
    const auto Before = Inventory->GetStacks();
    TArray<FKalmalaInventoryStack> Full;
    const int32 PackedTools = Inventory->CountPackedTools(Pawn->GetCarriedToolInventory());
    for (int32 Slot = PackedTools; Slot < UKalmalaInventoryComponent::MaxSlots; ++Slot)
        Full.Add({TEXT("BoneFragment"), 1});
    TestTrue(TEXT("Full-pack fixture installs"), Inventory->TryCommitStacksFromServer(Before, Full));
    const auto Filled = Inventory->GetStacks();
    TestFalse(TEXT("A full pack leaves nearby items on the ground"), Node->TryAutomaticPickupFromServer(Pawn));
    TestFalse(TEXT("Failed automatic grant does not deplete"), Node->IsHarvested());
    TestTrue(TEXT("Capacity fixture restores"), Inventory->TryCommitStacksFromServer(Filled, Before));
    Pawn->SetRole(ROLE_SimulatedProxy);
    TestFalse(TEXT("Client cannot grant proximity rewards"), Node->TryAutomaticPickupFromServer(Pawn));
    Pawn->SetRole(ROLE_Authority);
    TestTrue(TEXT("Pickup preference is enabled on the authoritative pawn"), Pawn->IsAutomaticPickupEnabled());
    const int32 BeforeCount = Inventory->GetQuantity(Item);
    Pawn->UpdateAutomaticPickupFromServer(0.1f);
    TestFalse(TEXT("Pickup scan is throttled"), Node->IsHarvested());
    Pawn->UpdateAutomaticPickupFromServer(0.15f);
    TestTrue(TEXT("Nearby material is collected without aiming or an action request"), Node->IsHarvested());
    TestEqual(TEXT("Only the server-authored quantity is granted"), Inventory->GetQuantity(Item), BeforeCount + 1);
    Pawn->UpdateAutomaticPickupFromServer(0.25f);
    TestEqual(TEXT("Repeated proximity scan cannot duplicate the grant"), Inventory->GetQuantity(Item), BeforeCount + 1);
    TestFalse(TEXT("Depleted drop no longer supplies a pickup prompt"), Node->GetPickupInfo(Item, Amount));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaInteractionAuthorityTest,
    "Kalmala.Gameplay.Interaction.ServerOnlyRangeValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaInteractionAuthorityTest::RunTest(const FString& Parameters)
{
    const FVector TargetLocation = FVector::ZeroVector;
    int32 ReplicatedCounter = 0;

    const bool bInvalidRequest = AKalmalaInteractionTestActor::IsInteractionAllowed(true, FVector(1000.0f, 0.0f, 0.0f), TargetLocation, 250.0f);
    if (bInvalidRequest)
    {
        ++ReplicatedCounter;
    }
    TestFalse(TEXT("An out-of-range request is rejected"), bInvalidRequest);
    TestEqual(TEXT("A rejected request cannot change interaction state"), ReplicatedCounter, 0);

    const bool bClientRequest = AKalmalaInteractionTestActor::IsInteractionAllowed(false, FVector(100.0f, 0.0f, 0.0f), TargetLocation, 250.0f);
    TestFalse(TEXT("A non-authoritative request is rejected"), bClientRequest);

    const bool bValidRequest = AKalmalaInteractionTestActor::IsInteractionAllowed(true, FVector(100.0f, 0.0f, 0.0f), TargetLocation, 250.0f);
    if (bValidRequest)
    {
        ++ReplicatedCounter;
    }
    TestTrue(TEXT("An in-range server request is accepted"), bValidRequest);
    TestEqual(TEXT("Only the accepted server request changes interaction state"), ReplicatedCounter, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaHarvestNodeAuthorityTest,
    "Kalmala.Gameplay.HarvestNode.AuthorityAndDepletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaHarvestNodeAuthorityTest::RunTest(const FString& Parameters)
{
    const FVector NodeLocation = FVector::ZeroVector;
    TestFalse(TEXT("Clients cannot harvest a generated node"), AKalmalaHarvestNode::IsHarvestAllowed(false, false, FVector(100.0f, 0.0f, 0.0f), NodeLocation));
    TestFalse(TEXT("The server rejects a distant harvester"), AKalmalaHarvestNode::IsHarvestAllowed(true, false, FVector(1000.0f, 0.0f, 0.0f), NodeLocation));
    TestFalse(TEXT("The server rejects a depleted node"), AKalmalaHarvestNode::IsHarvestAllowed(true, true, FVector(100.0f, 0.0f, 0.0f), NodeLocation));
    TestTrue(TEXT("The server accepts an in-range harvester for an available node"), AKalmalaHarvestNode::IsHarvestAllowed(true, false, FVector(100.0f, 0.0f, 0.0f), NodeLocation));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaCampfireWeatherTest,
    "Kalmala.Gameplay.Campfire.ServerWeatherResponse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaCampfireWeatherTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("A client cannot light a campfire"), AKalmalaCampfire::IsLightingAllowed(false, 0.0f));
    TestFalse(TEXT("The server rejects soaked fuel"), AKalmalaCampfire::IsLightingAllowed(true, FKalmalaCampfireWeatherResponse::ExtinguishWetness));
    TestTrue(TEXT("The server can light dry fuel"), AKalmalaCampfire::IsLightingAllowed(true, 0.0f));

    const float DryWarmth = FKalmalaCampfireWeatherResponse::CalculateEffectiveWarmth(true, 0.0f, 0.0f, 0.0f);
    const float StormWarmth = FKalmalaCampfireWeatherResponse::CalculateEffectiveWarmth(true, 0.35f, 1.0f, 1.0f);
    const float WetFuel = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(0.0f, 1.0f, 1.0f, 10.0f, true);
    const float DryingFuel = FKalmalaCampfireWeatherResponse::AdvanceFuelWetness(0.5f, 0.0f, 0.0f, 10.0f, true);
    TestEqual(TEXT("A dry sheltered campfire produces its full warmth contribution"), DryWarmth, 1.0f);
    TestTrue(TEXT("Rain, wind, and damp fuel reduce effective campfire warmth"), StormWarmth < DryWarmth);
    TestTrue(TEXT("Rain and wind soak exposed campfire fuel"), WetFuel > 0.0f);
    TestTrue(TEXT("A lit campfire dries fuel when rain stops"), DryingFuel < 0.5f);
    TestFalse(TEXT("Soaked fuel extinguishes a lit campfire"), FKalmalaCampfireWeatherResponse::ShouldRemainLit(true, FKalmalaCampfireWeatherResponse::ExtinguishWetness));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaExposureConsequenceTest,
    "Kalmala.Gameplay.Exposure.RecoverableTravelPenalty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaExposureConsequenceTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Hot ambient input maps to bounded heat intensity"), FKalmalaExposureResponse::GetHeatIntensity(20.0f), 1.0f);
    TestEqual(TEXT("Cold ambient input maps to no heat intensity"), FKalmalaExposureResponse::GetHeatIntensity(-20.0f), 0.0f);
    TestEqual(TEXT("Cold ambient input maps to bounded cold intensity"), FKalmalaExposureResponse::GetColdIntensity(-20.0f), 1.0f);
    TestEqual(TEXT("Warm ambient input maps to no cold intensity"), FKalmalaExposureResponse::GetColdIntensity(20.0f), 0.0f);
    TestEqual(TEXT("Non-finite climate values fail closed"), FKalmalaExposureResponse::GetColdIntensity(NAN), 0.0f);

    const float ExposedWetness = FKalmalaExposureResponse::AdvanceWetness(0.0f, 1.0f, 0.8f, 1.0f, 0.0f, 0.0f, 120.0f);
    const float ExposedWarmth = FKalmalaExposureResponse::AdvanceWarmth(100.0f, -12.0f, ExposedWetness, 1.0f, 0.0f, 0.0f, 120.0f);
    const float ExposedSpeed = FKalmalaExposureResponse::GetTravelSpeedMultiplier(ExposedWarmth);
    const float ExposedStaminaRecovery = FKalmalaExposureResponse::GetStaminaRecoveryMultiplier(ExposedWarmth);
    const float RecoveredWetness = FKalmalaExposureResponse::AdvanceWetness(ExposedWetness, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 60.0f);
    const float RecoveredWarmth = FKalmalaExposureResponse::AdvanceWarmth(ExposedWarmth, -12.0f, RecoveredWetness, 0.0f, 1.0f, 1.0f, 60.0f);

    TestTrue(TEXT("Prolonged exposed rain produces wetness"), ExposedWetness > 0.0f);
    TestTrue(TEXT("Prolonged cold exposure reduces warmth"), ExposedWarmth < 50.0f);
    TestTrue(TEXT("Low warmth applies a visible reversible travel penalty"), ExposedSpeed < 1.0f);
    TestTrue(TEXT("Cold exposure reduces stamina recovery by a bounded amount"), ExposedStaminaRecovery < 1.0f && ExposedStaminaRecovery >= FKalmalaExposureResponse::ColdStaminaRecoveryMinimumMultiplier);
    TestTrue(TEXT("Shelter and a lit fire dry the player"), RecoveredWetness < ExposedWetness);
    TestTrue(TEXT("Shelter and a lit fire recover warmth"), RecoveredWarmth > ExposedWarmth);
    TestEqual(TEXT("Recovered warmth removes the travel penalty"), FKalmalaExposureResponse::GetTravelSpeedMultiplier(RecoveredWarmth), 1.0f);
    TestEqual(TEXT("Recovered warmth restores full stamina recovery"), FKalmalaExposureResponse::GetStaminaRecoveryMultiplier(RecoveredWarmth), 1.0f);
    TestEqual(TEXT("Zero warmth cannot reduce stamina recovery below its floor"), FKalmalaExposureResponse::GetStaminaRecoveryMultiplier(0.0f), FKalmalaExposureResponse::ColdStaminaRecoveryMinimumMultiplier);
    TestEqual(TEXT("Non-finite warmth fails open to ordinary stamina recovery"), FKalmalaExposureResponse::GetStaminaRecoveryMultiplier(NAN), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaColdStaminaRecoveryAuthorityTest,
    "Kalmala.Gameplay.Exposure.ColdStaminaRecoveryAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaColdStaminaRecoveryAuthorityTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Cold stamina fixture spawned"), Pawn)) { World->DestroyWorld(false); return false; }
    auto* Movement = CastChecked<UKalmalaCharacterMovementComponent>(Pawn->GetCharacterMovement());
    Movement->SetMovementMode(MOVE_Walking);
    Movement->SetSprintRequested(true);
    Movement->Velocity = FVector(500.0f, 0.0f, 0.0f);
    for (int32 Step = 0; Step < 16; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Server sprint fixture drains a known stamina amount"), Movement->GetStamina(), 60.0f);

    FKalmalaExposureState Cold;
    Cold.Warmth = 0.0f;
    Cold.ColdIntensity = 1.0f;
    Pawn->SetExposureStateFromServer(Cold);
    Movement->SetSprintRequested(false);
    for (int32 Step = 0; Step < 4; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Server recovery at zero warmth is reduced to the bounded floor"), Movement->GetStamina(), 72.0f);

    FKalmalaExposureState Recovered;
    Recovered.Warmth = 100.0f;
    Pawn->SetExposureStateFromServer(Recovered);
    for (int32 Step = 0; Step < 4; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Server recovery returns to normal after shelter or fire restores warmth"), Movement->GetStamina(), 87.0f);

    Pawn->SetRole(ROLE_SimulatedProxy);
    Movement->AdvanceStaminaFromServer(1.0f);
    TestEqual(TEXT("A client-side movement copy cannot author stamina recovery"), Movement->GetStamina(), 87.0f);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaExposureAuthorityTest,
    "Kalmala.Gameplay.Exposure.ServerAuthoritativeReplicationContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaExposureAuthorityTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("A client cannot update replicated weather"), AKalmalaWorldGenerationGameState::IsWeatherUpdateAllowed(false));
    TestTrue(TEXT("Only the server can update replicated weather"), AKalmalaWorldGenerationGameState::IsWeatherUpdateAllowed(true));
    TestFalse(TEXT("A client cannot update replicated exposure"), AKalmalaCharacter::IsExposureUpdateAllowed(false));
    TestTrue(TEXT("Only the server can update replicated exposure"), AKalmalaCharacter::IsExposureUpdateAllowed(true));
    TestFalse(TEXT("A client cannot light a replicated campfire"), AKalmalaCampfire::IsLightingAllowed(false, 0.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaWildlifeSpawnAuthorityTest,
    "Kalmala.Gameplay.WildlifeSpawn.ServerOnlyDefeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWildlifeSpawnAuthorityTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Clients cannot defeat a generated wildlife spawn"), AKalmalaWildlifeSpawn::IsDefeatAllowed(false, false));
    TestFalse(TEXT("The server cannot defeat an already defeated wildlife spawn"), AKalmalaWildlifeSpawn::IsDefeatAllowed(true, true));
    TestTrue(TEXT("The server can record the first validated wildlife defeat"), AKalmalaWildlifeSpawn::IsDefeatAllowed(true, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaHazardSpawnAuthorityTest,
    "Kalmala.Gameplay.HazardSpawn.ServerOnlyDefeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaHazardSpawnAuthorityTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Clients cannot defeat a generated hazard spawn"), AKalmalaHazardSpawn::IsDefeatAllowed(false, false));
    TestFalse(TEXT("The server cannot defeat an already defeated hazard spawn"), AKalmalaHazardSpawn::IsDefeatAllowed(true, true));
    TestTrue(TEXT("The server can record the first validated hazard defeat"), AKalmalaHazardSpawn::IsDefeatAllowed(true, false));
    return true;
}

#endif
