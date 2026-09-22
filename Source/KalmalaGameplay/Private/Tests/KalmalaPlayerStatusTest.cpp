#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCampfire.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerWetStatusTest, "Kalmala.Gameplay.Status.Wet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaPlayerWetStatusTest::RunTest(const FString& Parameters)
{
    TArray<FKalmalaPlayerStatusEntry> Entries;
    UKalmalaPlayerStatusComponent::ApplyWet(Entries);
    TestEqual(TEXT("Wet creates one bounded status entry"), Entries.Num(), 1);
    TestEqual(TEXT("Wet starts at its configured maximum"), Entries[0].RemainingSeconds, UKalmalaPlayerStatusComponent::WetMaximumSeconds);

    UKalmalaPlayerStatusComponent::Advance(Entries, 10.0f);
    TestEqual(TEXT("Server time advances remaining duration"), Entries[0].RemainingSeconds, 110.0f);
    UKalmalaPlayerStatusComponent::ApplyWet(Entries);
    TestEqual(TEXT("Reapplication clamps instead of stacking duration"), Entries[0].RemainingSeconds, UKalmalaPlayerStatusComponent::WetMaximumSeconds);

    FKalmalaPlayerStatusEntry Malformed;
    Malformed.StatusId = UKalmalaPlayerStatusComponent::WetStatusId;
    Malformed.RemainingSeconds = NAN;
    Entries.Add(Malformed);
    UKalmalaPlayerStatusComponent::Advance(Entries, -1.0f);
    TestEqual(TEXT("Malformed and duplicate status data fails closed"), Entries.Num(), 1);
    UKalmalaPlayerStatusComponent::Advance(Entries, 1000.0f);
    TestEqual(TEXT("Expired Wet is removed"), Entries.Num(), 0);
    TestEqual(TEXT("Rain trigger remains explicit"), UKalmalaPlayerStatusComponent::UnroofedRainTriggerSeconds, 10.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlayerWetModifiersTest, "Kalmala.Gameplay.Status.WetModifiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaPlayerWetModifiersTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Status movement fixture spawned"), Pawn)) { World->DestroyWorld(false); return false; }
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    auto* Movement = CastChecked<UKalmalaCharacterMovementComponent>(Pawn->GetCharacterMovement());
    Movement->SetMovementMode(MOVE_Walking);
    const float DrySpeed = Movement->GetMaxSpeed();
    Status->ApplyWetFromServer();
    TestTrue(TEXT("Wet walking uses the tuned shared modifier"), FMath::IsNearlyEqual(Movement->GetMaxSpeed(), DrySpeed * UKalmalaPlayerStatusComponent::WetMovementMultiplier));
    Movement->SetSprintRequested(true);
    TestTrue(TEXT("Sprint retains the tuned Wet penalty"), FMath::IsNearlyEqual(Movement->GetMaxSpeed(), DrySpeed * 1.5f * UKalmalaPlayerStatusComponent::WetMovementMultiplier));
    Movement->SetMovementMode(MOVE_Custom, UKalmalaCharacterMovementComponent::GeneratedOceanSwimmingMode);
    TestTrue(TEXT("Swimming applies tuned Wet after its speed cap"), FMath::IsNearlyEqual(Movement->GetMaxSpeed(), FMath::Min(DrySpeed, 420.0f) * UKalmalaPlayerStatusComponent::WetMovementMultiplier));
    TestEqual(TEXT("Authoritative base stamina cost gains the tuned 15 percent"), Status->CalculateStaminaCost(20.0f), 23.0f);
    TestEqual(TEXT("Invalid cost cannot become a credit"), Status->CalculateStaminaCost(-20.0f), 0.0f);
    TArray<FKalmalaPlayerStatusEntry> Entries = Status->GetStatuses();
    const FKalmalaPlayerStatusEntry Duplicate = Entries[0];
    Entries.Add(Duplicate);
    TestEqual(TEXT("Duplicate Wet cannot stack the tuned movement penalty"), UKalmalaPlayerStatusComponent::EvaluateModifiers(Entries).Movement, UKalmalaPlayerStatusComponent::WetMovementMultiplier);
    Status->AdvanceFromServer(120.0f);
    Movement->SetMovementMode(MOVE_Walking);
    Movement->SetSprintRequested(false);
    TestEqual(TEXT("Expiry restores walking immediately"), Movement->GetMaxSpeed(), DrySpeed);
    TestEqual(TEXT("Expiry restores stamina cost"), Status->CalculateStaminaCost(20.0f), 20.0f);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWetStaminaTest, "Kalmala.Gameplay.Status.WetStamina",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWetStaminaTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Stamina fixture spawned"), Pawn)) { World->DestroyWorld(false); return false; }
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    auto* Movement = CastChecked<UKalmalaCharacterMovementComponent>(Pawn->GetCharacterMovement());
    Movement->SetMovementMode(MOVE_Walking);
    Movement->SetSprintRequested(true);
    Movement->Velocity = FVector(500, 0, 0);
    for (int32 Step = 0; Step < 4; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Dry sprint consumes ten per second"), Movement->GetStamina(), 90.0f);
    Status->ApplyWetFromServer();
    for (int32 Step = 0; Step < 4; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Wet actually consumes the tuned 15 percent more"), Movement->GetStamina(), 78.5f);
    Movement->AdvanceStaminaFromServer(NAN);
    Movement->AdvanceStaminaFromServer(-1.0f);
    TestEqual(TEXT("Malformed elapsed time cannot change stamina"), Movement->GetStamina(), 78.5f);
    for (int32 Step = 0; Step < 28; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Exhaustion clamps at zero"), Movement->GetStamina(), 0.0f);
    TestTrue(TEXT("Exhaustion disables sprint despite held intent"), Movement->IsSprintExhausted());
    TestTrue(TEXT("Exhaustion retains ordinary tuned Wet walking"), FMath::IsNearlyEqual(Movement->GetMaxSpeed(), Movement->MaxWalkSpeed * UKalmalaPlayerStatusComponent::WetMovementMultiplier));
    for (int32 Step = 0; Step < 5; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestTrue(TEXT("Recovery threshold prevents rapid sprint toggles"), Movement->IsSprintExhausted());
    Movement->AdvanceStaminaFromServer(0.25f);
    TestFalse(TEXT("Sufficient recovery restores sprint"), Movement->IsSprintExhausted());
    Movement->SetSprintRequested(false);
    for (int32 Step = 0; Step < 40; ++Step) Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Recovery clamps at maximum"), Movement->GetStamina(), 100.0f);
    Movement->SetSprintRequested(true);
    Movement->Velocity = FVector::ZeroVector;
    Movement->AdvanceStaminaFromServer(0.25f);
    TestEqual(TEXT("Stationary held sprint has no cost"), Movement->GetStamina(), 100.0f);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWetCampfireTest, "Kalmala.Gameplay.Status.WetCampfire",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWetCampfireTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Fire = World->SpawnActor<AKalmalaCampfire>();
    if (!Pawn || !Controller || !Fire) { AddError(TEXT("Campfire fixture spawn failed")); World->DestroyWorld(false); return false; }
    Controller->Possess(Pawn);
    Fire->SetActorLocation(Pawn->GetActorLocation() + FVector(120, 0, 0));
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    Status->ApplyWetFromServer();
    TestFalse(TEXT("Missing source cannot remove Wet"), Status->TryRemoveWetAtCampfireFromServer(nullptr));
    TestFalse(TEXT("Unlit nearby fire cannot remove Wet"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    Fire->InitializePaidFromServer(Pawn);
    Fire->Interact_Implementation(Pawn);
    TestTrue(TEXT("Fixture is lit"), Fire->IsLit());
    TestFalse(TEXT("Lit with no effective heat cannot remove Wet"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    Fire->AdvanceFromServer(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("Fixture produces heat"), Fire->GetEffectiveWarmth() > 0);
    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("Client cannot invoke removal"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    TestTrue(TEXT("Client attempt preserves Wet"), Status->HasStatus(Status->WetStatusId));
    Pawn->SetRole(ROLE_Authority);
    Fire->SetActorLocation(Pawn->GetActorLocation() + FVector(600, 0, 0));
    TestFalse(TEXT("Heat radius edge cannot remove Wet"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    Fire->SetActorLocation(Pawn->GetActorLocation() + FVector(120, 0, 0));
    TestTrue(TEXT("Authoritative nearby heat removes Wet"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    TestFalse(TEXT("Wet entry removed"), Status->HasStatus(Status->WetStatusId));
    TestEqual(TEXT("Movement modifier recovers"), Status->GetModifiers().Movement, 1.0f);
    TestEqual(TEXT("Stamina cost recovers"), Status->CalculateStaminaCost(10), 10.0f);
    TestFalse(TEXT("Repeated removal is a no-op"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    Status->ApplyWetFromServer();
    Fire->AdvanceFromServer(60, 0, 0);
    TestFalse(TEXT("Fuel-exhausted fire cannot remove reapplied Wet"), Status->TryRemoveWetAtCampfireFromServer(Fire));
    Status->AdvanceFromServer(120);
    TestFalse(TEXT("Expiry remains valid without heat"), Status->HasStatus(Status->WetStatusId));
    World->DestroyWorld(false);
    return true;
}

#endif
