#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaCampfire.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaSupportMagicComponent.h"
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
    TestEqual(TEXT("Wet applies its shared multiplier to the tuned support base cost"), Status->CalculateStaminaCost(UKalmalaSupportMagicComponent::ActivationCost), 20.7f);
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
    TestTrue(TEXT("Dedicated light action starts the paid campfire"), Fire->TryLightFromServer(Pawn));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSteadyMealStatusTest, "Kalmala.Gameplay.Status.SteadyMeal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaSteadyMealStatusTest::RunTest(const FString& Parameters)
{
    TArray<FKalmalaPlayerStatusEntry> Entries;
    TestFalse(TEXT("Unknown items cannot create food effects"), UKalmalaPlayerStatusComponent::ApplyFood(Entries, TEXT("Wood")));
    const FName CurrentCookingOutputs[] = {
        FName(TEXT("CookedBoarMeat")), FName(TEXT("CookedDeerMeat")), FName(TEXT("MeatStew")),
        FName(TEXT("RootVegetableSoup")), FName(TEXT("RoastedRootVegetables")), FName(TEXT("DeerRootRoast")),
        FName(TEXT("RoastedBadger"))
    };
    for (const FName FoodId : CurrentCookingOutputs)
    {
        TestTrue(FString::Printf(TEXT("Current cooking output %s is recognized"), *FoodId.ToString()),
            UKalmalaPlayerStatusComponent::IsKnownFoodItem(FoodId));
        TestTrue(FString::Printf(TEXT("Current cooking output %s applies the meal effect"), *FoodId.ToString()),
            UKalmalaPlayerStatusComponent::ApplyFood(Entries, FoodId));
        UKalmalaPlayerStatusComponent::Advance(Entries, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    }
    TestFalse(TEXT("Raw meat remains an ingredient, not a meal"), UKalmalaPlayerStatusComponent::IsKnownFoodItem(TEXT("BoarMeat")));
    TestFalse(TEXT("Retired roasted-field-meat ID is not a meal"), UKalmalaPlayerStatusComponent::IsKnownFoodItem(TEXT("RoastedFieldMeat")));
    TestFalse(TEXT("Retired smoked-field-meat ID is not a meal"), UKalmalaPlayerStatusComponent::IsKnownFoodItem(TEXT("SmokedFieldMeat")));
    TestTrue(TEXT("Foraged Lingonberry is recognized as food"), UKalmalaPlayerStatusComponent::IsKnownFoodItem(TEXT("Lingonberry")));
    TestTrue(TEXT("Lingonberry applies the shared Steady Meal effect"), UKalmalaPlayerStatusComponent::ApplyFood(Entries, TEXT("Lingonberry")));
    UKalmalaPlayerStatusComponent::Advance(Entries, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestTrue(TEXT("Foraged Forest Chanterelle is recognized as food"), UKalmalaPlayerStatusComponent::IsKnownFoodItem(TEXT("ForestChanterelle")));
    TestTrue(TEXT("Forest Chanterelle applies the shared Steady Meal effect"), UKalmalaPlayerStatusComponent::ApplyFood(Entries, TEXT("ForestChanterelle")));
    UKalmalaPlayerStatusComponent::Advance(Entries, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestTrue(TEXT("Current catalogue hearth broth creates a meal effect"), UKalmalaPlayerStatusComponent::ApplyFood(Entries, TEXT("HearthBroth")));
    TestEqual(TEXT("Meal has one server-defined finite entry"), Entries.Num(), 1);
    TestEqual(TEXT("Meal uses the fixed server duration"), Entries[0].RemainingSeconds, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestEqual(TEXT("Meal reduces stamina use without changing movement speed"),
        UKalmalaPlayerStatusComponent::EvaluateModifiers(Entries).StaminaUse,
        UKalmalaPlayerStatusComponent::SteadyMealStaminaUseMultiplier);
    TestEqual(TEXT("Meal does not add a movement-speed bonus"), UKalmalaPlayerStatusComponent::EvaluateModifiers(Entries).Movement, 1.0f);
    TestFalse(TEXT("A second broth serving cannot stack or replace the active meal"), UKalmalaPlayerStatusComponent::ApplyFood(Entries, TEXT("HearthBroth")));
    TestEqual(TEXT("Rejected meal preserves its timer"), Entries[0].RemainingSeconds, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);

    UKalmalaPlayerStatusComponent::Advance(Entries, 30.0f);
    TestEqual(TEXT("Meal duration advances with server status time"), Entries[0].RemainingSeconds, 90.0f);
    UKalmalaPlayerStatusComponent::Advance(Entries, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestTrue(TEXT("Expired meal is removed"), Entries.IsEmpty());
    TestEqual(TEXT("Expired meal restores stamina costs"), UKalmalaPlayerStatusComponent::EvaluateModifiers(Entries).StaminaUse, 1.0f);
    TestTrue(TEXT("Hearth broth may supply the next meal after expiry"), UKalmalaPlayerStatusComponent::ApplyFood(Entries, TEXT("HearthBroth")));
    UKalmalaPlayerStatusComponent::Advance(Entries, UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World ? World->SpawnActor<AKalmalaCharacter>() : nullptr;
    APlayerController* Controller = World ? World->SpawnActor<APlayerController>() : nullptr;
    if (!TestNotNull(TEXT("Authority status fixture spawned"), Pawn) || !TestNotNull(TEXT("Meal inventory controller spawned"), Controller))
    {
        if (World) World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    auto* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    auto* Inventory = Pawn->GetInventoryComponent();
    if (!TestNotNull(TEXT("Food consumption component exists"), Crafting) || !TestNotNull(TEXT("Food inventory exists"), Inventory))
    {
        World->DestroyWorld(false);
        return false;
    }
    TestTrue(TEXT("Server owner may apply the current catalogue meal item"), Status && Status->ApplyFoodFromServer(TEXT("HearthBroth")));
    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("Owning client cannot author a meal effect"), Status && Status->ApplyFoodFromServer(TEXT("HearthBroth")));
    TestEqual(TEXT("Rejected client application retains the server timer"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    Pawn->SetRole(ROLE_Authority);
    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    TestTrue(TEXT("Food transaction fixture grants two current cooking outputs"), Inventory->TryGrantFromServer(TEXT("CookedBoarMeat"), 2));
    FString Reason;
    TestTrue(TEXT("Server food transaction consumes and applies a current cooking output"), Crafting->ConsumeFoodFromServer(TEXT("CookedBoarMeat"), Reason));
    TestEqual(TEXT("Accepted meal consumes exactly one item"), Inventory->GetQuantity(TEXT("CookedBoarMeat")), 1);
    TestEqual(TEXT("Accepted meal publishes its fixed effect duration"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId),
        UKalmalaPlayerStatusComponent::SteadyMealMaximumSeconds);
    const float MealTimeBeforeDuplicate = Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId);
    TestFalse(TEXT("An active meal rejects a second consumption transaction"), Crafting->ConsumeFoodFromServer(TEXT("CookedBoarMeat"), Reason));
    TestEqual(TEXT("Duplicate rejection preserves the remaining inventory item"), Inventory->GetQuantity(TEXT("CookedBoarMeat")), 1);
    TestEqual(TEXT("Duplicate rejection preserves the active timer"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId), MealTimeBeforeDuplicate);
    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("Client cannot consume food through the server transaction"), Crafting->ConsumeFoodFromServer(TEXT("CookedBoarMeat"), Reason));
    TestEqual(TEXT("Client request preserves food inventory"), Inventory->GetQuantity(TEXT("CookedBoarMeat")), 1);
    TestEqual(TEXT("Client request preserves the active meal timer"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::SteadyMealStatusId), MealTimeBeforeDuplicate);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBerryRecoveryDraughtStatusTest,
    "Kalmala.Gameplay.Status.BerryRecoveryDraught",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaBerryRecoveryDraughtStatusTest::RunTest(const FString& Parameters)
{
    TArray<FKalmalaPlayerStatusEntry> Entries;
    TestFalse(TEXT("Unknown item cannot start a recovery effect"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("Wood")));
    TestTrue(TEXT("Berry Recovery Draught is allowlisted"),
        UKalmalaPlayerStatusComponent::IsKnownDraughtItem(TEXT("BerryRecoveryDraught")));
    TestFalse(TEXT("Steady Meal foods are not draught items"),
        UKalmalaPlayerStatusComponent::IsKnownDraughtItem(TEXT("HearthBroth")));
    TestTrue(TEXT("Draught applies one healing effect and the shared cooldown"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("BerryRecoveryDraught")));
    TestEqual(TEXT("Draught publishes both bounded statuses"), Entries.Num(), 2);
    const FKalmalaPlayerStatusEntry* RecoveryEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::BerryRecoveryStatusId; });
    const FKalmalaPlayerStatusEntry* CooldownEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::DraughtCooldownStatusId; });
    TestNotNull(TEXT("Healing status is present"), RecoveryEntry);
    TestNotNull(TEXT("Shared cooldown status is present"), CooldownEntry);
    if (RecoveryEntry && CooldownEntry)
    {
        TestEqual(TEXT("Healing lasts ten seconds"), RecoveryEntry->RemainingSeconds,
            UKalmalaPlayerStatusComponent::BerryRecoveryMaximumSeconds);
        TestEqual(TEXT("Shared draught cooldown lasts sixty seconds"), CooldownEntry->RemainingSeconds,
            UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    }
    TestFalse(TEXT("A second draught cannot refresh the shared cooldown"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("BerryRecoveryDraught")));
    UKalmalaPlayerStatusComponent::Advance(Entries, 5.0f);
    RecoveryEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::BerryRecoveryStatusId; });
    CooldownEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::DraughtCooldownStatusId; });
    TestTrue(TEXT("Healing timer advances"), RecoveryEntry && RecoveryEntry->RemainingSeconds == 5.0f);
    TestTrue(TEXT("Cooldown advances from the same server time"), CooldownEntry && CooldownEntry->RemainingSeconds == 55.0f);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World ? World->SpawnActor<AKalmalaCharacter>() : nullptr;
    APlayerController* Controller = World ? World->SpawnActor<APlayerController>() : nullptr;
    if (!TestNotNull(TEXT("Draught character fixture spawned"), Pawn)
        || !TestNotNull(TEXT("Draught controller fixture spawned"), Controller))
    {
        if (World) World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    auto* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    auto* Inventory = Pawn->GetInventoryComponent();
    const FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(AKalmalaCharacter::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Draught status component exists"), Status)
        || !TestNotNull(TEXT("Draught crafting component exists"), Crafting)
        || !TestNotNull(TEXT("Draught inventory exists"), Inventory)
        || !TestNotNull(TEXT("Health fixture property exists"), HealthProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    HealthProperty->SetPropertyValue_InContainer(Pawn, 60.0f);
    TestTrue(TEXT("Server accepts a draught only when health is missing"),
        Status->CanApplyDraughtFromServer(TEXT("BerryRecoveryDraught")));
    TestTrue(TEXT("Inventory can receive the authored draught"),
        Inventory->TryGrantFromServer(TEXT("BerryRecoveryDraught"), 2));
    FString Reason;
    TestTrue(TEXT("Server transaction consumes one dose and applies recovery"),
        Crafting->ConsumeFoodFromServer(TEXT("BerryRecoveryDraught"), Reason));
    TestEqual(TEXT("Accepted dose consumes exactly one item"),
        Inventory->GetQuantity(TEXT("BerryRecoveryDraught")), 1);
    TestEqual(TEXT("Dose starts without an immediate health jump"), Pawn->GetHealth(), 60.0f);
    TestEqual(TEXT("Dose publishes shared cooldown"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId),
        UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    TestFalse(TEXT("Cooldown rejects a second dose before inventory payment"),
        Crafting->ConsumeFoodFromServer(TEXT("BerryRecoveryDraught"), Reason));
    TestEqual(TEXT("Cooldown rejection preserves the remaining dose"),
        Inventory->GetQuantity(TEXT("BerryRecoveryDraught")), 1);

    Status->AdvanceFromServer(5.0f);
    TestEqual(TEXT("Five server seconds restore ten health"), Pawn->GetHealth(), 70.0f);
    Status->AdvanceFromServer(10.0f);
    TestEqual(TEXT("Healing completes at twenty health restored"), Pawn->GetHealth(), 80.0f);
    TestFalse(TEXT("Ten seconds removes the healing entry"), Status->HasStatus(UKalmalaPlayerStatusComponent::BerryRecoveryStatusId));
    TestEqual(TEXT("Healing completion leaves the shared cooldown running"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId), 45.0f);
    Status->AdvanceFromServer(45.0f);
    TestFalse(TEXT("Sixty server seconds expire the draught cooldown"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId));

    HealthProperty->SetPropertyValue_InContainer(Pawn, 60.0f);
    TestTrue(TEXT("A second dose can be used after cooldown expiry"),
        Crafting->ConsumeFoodFromServer(TEXT("BerryRecoveryDraught"), Reason));
    HealthProperty->SetPropertyValue_InContainer(Pawn, 1.0f);
    Status->AdvanceFromServer(1.0f);
    TestFalse(TEXT("Death stops the remaining healing timer"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::BerryRecoveryStatusId));
    TestEqual(TEXT("A dead player receives no further healing"), Pawn->GetHealth(), 1.0f);

    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 100.0f);
    TestTrue(TEXT("Full-health rejection fixture has a dose"),
        Inventory->TryGrantFromServer(TEXT("BerryRecoveryDraught"), 1));
    TestFalse(TEXT("Full health rejects the draught without consuming it"),
        Crafting->ConsumeFoodFromServer(TEXT("BerryRecoveryDraught"), Reason));
    TestEqual(TEXT("Full-health rejection preserves the item"),
        Inventory->GetQuantity(TEXT("BerryRecoveryDraught")), 1);
    TestFalse(TEXT("Full-health rejection starts no cooldown"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId));

    Pawn->SetRole(ROLE_AutonomousProxy);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 60.0f);
    TestFalse(TEXT("Client cannot author draught status"),
        Status->ApplyDraughtFromServer(TEXT("BerryRecoveryDraught")));
    TestFalse(TEXT("Client cannot consume a dose"),
        Crafting->ConsumeFoodFromServer(TEXT("BerryRecoveryDraught"), Reason));
    TestEqual(TEXT("Client rejection preserves inventory"),
        Inventory->GetQuantity(TEXT("BerryRecoveryDraught")), 1);

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRatTailTonicStatusTest,
    "Kalmala.Gameplay.Status.RatTailTonic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaRatTailTonicStatusTest::RunTest(const FString& Parameters)
{
    TArray<FKalmalaPlayerStatusEntry> Entries;
    TestTrue(TEXT("Rat-tail Tonic is allowlisted as a draught"),
        UKalmalaPlayerStatusComponent::IsKnownDraughtItem(TEXT("RatTailTonic")));
    TestTrue(TEXT("Rat-tail Tonic starts a health effect and shared cooldown"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("RatTailTonic")));
    const FKalmalaPlayerStatusEntry* TonicEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::RatTailTonicStatusId; });
    const FKalmalaPlayerStatusEntry* CooldownEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::DraughtCooldownStatusId; });
    TestTrue(TEXT("Tonic publishes its ten-second healing status"),
        TonicEntry && TonicEntry->RemainingSeconds == UKalmalaPlayerStatusComponent::RatTailTonicMaximumSeconds);
    TestTrue(TEXT("Tonic uses the shared sixty-second cooldown"),
        CooldownEntry && CooldownEntry->RemainingSeconds == UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    TestFalse(TEXT("Tonic healing rejects another health draught"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("BerryRecoveryDraught")));
    UKalmalaPlayerStatusComponent::Advance(Entries, 5.0f);
    TonicEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::RatTailTonicStatusId; });
    CooldownEntry = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::DraughtCooldownStatusId; });
    TestTrue(TEXT("Tonic healing and shared cooldown advance together"),
        TonicEntry && TonicEntry->RemainingSeconds == 5.0f
        && CooldownEntry && CooldownEntry->RemainingSeconds == 55.0f);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World ? World->SpawnActor<AKalmalaCharacter>() : nullptr;
    APlayerController* Controller = World ? World->SpawnActor<APlayerController>() : nullptr;
    if (!TestNotNull(TEXT("Tonic character fixture spawned"), Pawn)
        || !TestNotNull(TEXT("Tonic controller fixture spawned"), Controller))
    {
        if (World) World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    auto* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    auto* Inventory = Pawn->GetInventoryComponent();
    const FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(AKalmalaCharacter::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Tonic status component exists"), Status)
        || !TestNotNull(TEXT("Tonic crafting component exists"), Crafting)
        || !TestNotNull(TEXT("Tonic inventory exists"), Inventory)
        || !TestNotNull(TEXT("Tonic health fixture property exists"), HealthProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    HealthProperty->SetPropertyValue_InContainer(Pawn, 60.0f);
    TestTrue(TEXT("Inventory receives the authored tonic doses"), Inventory->TryGrantFromServer(TEXT("RatTailTonic"), 5));
    FString Reason;
    TestTrue(TEXT("Server use consumes one tonic and starts healing"),
        Crafting->ConsumeFoodFromServer(TEXT("RatTailTonic"), Reason));
    TestEqual(TEXT("Accepted tonic use consumes one dose"), Inventory->GetQuantity(TEXT("RatTailTonic")), 4);
    TestEqual(TEXT("Tonic healing does not jump immediately"), Pawn->GetHealth(), 60.0f);
    TestEqual(TEXT("Tonic starts the owner-visible recovery timer"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::RatTailTonicStatusId), 10.0f);
    TestEqual(TEXT("Tonic starts the A6 shared cooldown"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId), 60.0f);
    TestTrue(TEXT("Inventory receives the A6 recovery draught for shared-cooldown coverage"),
        Inventory->TryGrantFromServer(TEXT("BerryRecoveryDraught"), 1));
    TestFalse(TEXT("Tonic cooldown rejects Berry Recovery Draught use"),
        Crafting->ConsumeFoodFromServer(TEXT("BerryRecoveryDraught"), Reason));
    TestEqual(TEXT("Shared-cooldown rejection preserves the Berry dose"),
        Inventory->GetQuantity(TEXT("BerryRecoveryDraught")), 1);
    Status->AdvanceFromServer(5.0f);
    TestEqual(TEXT("Five server seconds restore fifteen tonic health"), Pawn->GetHealth(), 75.0f);
    Status->AdvanceFromServer(5.0f);
    TestEqual(TEXT("Ten server seconds restore thirty tonic health"), Pawn->GetHealth(), 90.0f);
    TestFalse(TEXT("The ten-second tonic effect expires"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::RatTailTonicStatusId));
    TestEqual(TEXT("Healing completion leaves fifty shared cooldown seconds"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId), 50.0f);

    Status->AdvanceFromServer(50.0f);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 100.0f);
    TestFalse(TEXT("Full health rejects tonic use"), Crafting->ConsumeFoodFromServer(TEXT("RatTailTonic"), Reason));
    TestEqual(TEXT("Full-health rejection preserves the tonic"), Inventory->GetQuantity(TEXT("RatTailTonic")), 4);
    TestFalse(TEXT("Full-health rejection does not start the cooldown"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId));

    HealthProperty->SetPropertyValue_InContainer(Pawn, 80.0f);
    TestTrue(TEXT("A tonic is usable while health is missing"),
        Crafting->ConsumeFoodFromServer(TEXT("RatTailTonic"), Reason));
    Status->AdvanceFromServer(10.0f);
    TestEqual(TEXT("Tonic healing clamps at maximum health"), Pawn->GetHealth(), 100.0f);
    TestFalse(TEXT("Tonic healing stops when health becomes full"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::RatTailTonicStatusId));
    TestEqual(TEXT("Early healing completion does not refund its cooldown"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId), 50.0f);

    Status->AdvanceFromServer(50.0f);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 60.0f);
    TestTrue(TEXT("Tonic can be used after the cooldown expires"),
        Crafting->ConsumeFoodFromServer(TEXT("RatTailTonic"), Reason));
    HealthProperty->SetPropertyValue_InContainer(Pawn, 1.0f);
    Status->AdvanceFromServer(1.0f);
    TestEqual(TEXT("Death cancels tonic healing without restoring health"), Pawn->GetHealth(), 1.0f);
    TestFalse(TEXT("Death clears the active tonic healing status"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::RatTailTonicStatusId));
    TestEqual(TEXT("Death does not refund the shared cooldown"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId), 59.0f);

    Status->AdvanceFromServer(59.0f);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 98.0f);
    TestTrue(TEXT("A small missing amount accepts the final tonic dose"),
        Crafting->ConsumeFoodFromServer(TEXT("RatTailTonic"), Reason));
    Status->AdvanceFromServer(2.0f);
    TestEqual(TEXT("Tonic healing never exceeds maximum health"), Pawn->GetHealth(), 100.0f);
    TestFalse(TEXT("Healing status ends as soon as the player is full"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::RatTailTonicStatusId));

    Pawn->SetRole(ROLE_AutonomousProxy);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 60.0f);
    TestFalse(TEXT("A client cannot author tonic status"), Status->ApplyDraughtFromServer(TEXT("RatTailTonic")));
    TestFalse(TEXT("A client cannot consume a tonic"), Crafting->ConsumeFoodFromServer(TEXT("RatTailTonic"), Reason));
    TestEqual(TEXT("Client rejection preserves remaining tonic doses"), Inventory->GetQuantity(TEXT("RatTailTonic")), 1);

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRootStaminaDraughtStatusTest,
    "Kalmala.Gameplay.Status.RootStaminaDraught",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaRootStaminaDraughtStatusTest::RunTest(const FString& Parameters)
{
    TArray<FKalmalaPlayerStatusEntry> Entries;
    TestTrue(TEXT("Root Stamina Draught is allowlisted"),
        UKalmalaPlayerStatusComponent::IsKnownDraughtItem(TEXT("RootStaminaDraught")));
    TestTrue(TEXT("Root draught applies the shared cooldown"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("RootStaminaDraught")));
    TestEqual(TEXT("Immediate stamina recovery publishes only the shared cooldown"), Entries.Num(), 1);
    TestTrue(TEXT("Root draught does not start Berry healing"),
        Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == UKalmalaPlayerStatusComponent::DraughtCooldownStatusId; }));
    TestFalse(TEXT("Root draught has no healing timer"),
        Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == UKalmalaPlayerStatusComponent::BerryRecoveryStatusId; }));
    TestFalse(TEXT("Berry and Root draughts share one cooldown"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("BerryRecoveryDraught")));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World ? World->SpawnActor<AKalmalaCharacter>() : nullptr;
    APlayerController* Controller = World ? World->SpawnActor<APlayerController>() : nullptr;
    if (!TestNotNull(TEXT("Root draught character fixture spawned"), Pawn)
        || !TestNotNull(TEXT("Root draught controller fixture spawned"), Controller))
    {
        if (World) World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    auto* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    auto* Inventory = Pawn->GetInventoryComponent();
    auto* Movement = Cast<UKalmalaCharacterMovementComponent>(Pawn->GetCharacterMovement());
    const FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(AKalmalaCharacter::StaticClass(), TEXT("Health"));
    const FFloatProperty* StaminaProperty = FindFProperty<FFloatProperty>(UKalmalaCharacterMovementComponent::StaticClass(), TEXT("Stamina"));
    if (!TestNotNull(TEXT("Root draught status component exists"), Status)
        || !TestNotNull(TEXT("Root draught crafting component exists"), Crafting)
        || !TestNotNull(TEXT("Root draught inventory exists"), Inventory)
        || !TestNotNull(TEXT("Root draught movement component exists"), Movement)
        || !TestNotNull(TEXT("Root draught health fixture property exists"), HealthProperty)
        || !TestNotNull(TEXT("Root draught stamina fixture property exists"), StaminaProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    HealthProperty->SetPropertyValue_InContainer(Pawn, 100.0f);
    TestTrue(TEXT("Server accepts recovery when stamina is missing despite full health"),
        Movement->TryConsumeStaminaFromServer(60.0f)
        && Status->CanApplyDraughtFromServer(TEXT("RootStaminaDraught")));
    TestTrue(TEXT("Inventory can receive two Root Stamina Draught doses"),
        Inventory->TryGrantFromServer(TEXT("RootStaminaDraught"), 2));
    FString Reason;
    TestTrue(TEXT("Server transaction consumes one dose and restores twenty stamina"),
        Crafting->ConsumeFoodFromServer(TEXT("RootStaminaDraught"), Reason));
    TestEqual(TEXT("Accepted Root dose consumes exactly one item"),
        Inventory->GetQuantity(TEXT("RootStaminaDraught")), 1);
    TestEqual(TEXT("Root dose restores exactly twenty stamina"), Movement->GetStamina(), 60.0f);
    TestEqual(TEXT("Root dose starts the shared sixty-second cooldown"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId),
        UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    TestFalse(TEXT("Root dose does not create a healing-over-time status"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::BerryRecoveryStatusId));
    TestFalse(TEXT("Cooldown rejects the next Root dose"),
        Crafting->ConsumeFoodFromServer(TEXT("RootStaminaDraught"), Reason));
    TestEqual(TEXT("Cooldown rejection preserves the remaining dose"),
        Inventory->GetQuantity(TEXT("RootStaminaDraught")), 1);

    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    TestTrue(TEXT("Vigor maximum can be active before Root recovery"), Movement->SetBearsVigorFromServer(true));
    StaminaProperty->SetPropertyValue_InContainer(Movement, 125.0f);
    TestTrue(TEXT("A dose fits a current maximum above the base stamina cap"),
        Crafting->ConsumeFoodFromServer(TEXT("RootStaminaDraught"), Reason));
    TestEqual(TEXT("Recovery clamps at the current 140-stamina maximum"), Movement->GetStamina(), 140.0f);

    Status->AdvanceFromServer(UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    TestTrue(TEXT("Full-stamina rejection fixture has a dose"),
        Inventory->TryGrantFromServer(TEXT("RootStaminaDraught"), 1));
    TestFalse(TEXT("Full stamina rejects the Root dose without consuming it"),
        Crafting->ConsumeFoodFromServer(TEXT("RootStaminaDraught"), Reason));
    TestEqual(TEXT("Full-stamina rejection preserves the item"),
        Inventory->GetQuantity(TEXT("RootStaminaDraught")), 1);
    TestFalse(TEXT("Full-stamina rejection starts no cooldown"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId));

    StaminaProperty->SetPropertyValue_InContainer(Movement, 120.0f);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 1.0f);
    TestFalse(TEXT("A dead player cannot use Root Stamina Draught"),
        Crafting->ConsumeFoodFromServer(TEXT("RootStaminaDraught"), Reason));
    TestEqual(TEXT("Death rejection preserves the item"),
        Inventory->GetQuantity(TEXT("RootStaminaDraught")), 1);
    Pawn->SetRole(ROLE_AutonomousProxy);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 100.0f);
    TestFalse(TEXT("A client cannot author Root draught status"),
        Status->ApplyDraughtFromServer(TEXT("RootStaminaDraught")));
    TestFalse(TEXT("A client cannot consume a Root dose"),
        Crafting->ConsumeFoodFromServer(TEXT("RootStaminaDraught"), Reason));
    TestEqual(TEXT("Client rejection preserves the Root dose"),
        Inventory->GetQuantity(TEXT("RootStaminaDraught")), 1);
    TestEqual(TEXT("Client rejection leaves stamina unchanged"), Movement->GetStamina(), 120.0f);

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaViperWeakPoisonStatusTest,
    "Kalmala.Gameplay.Status.ViperWeakPoison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaViperWeakPoisonStatusTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World != nullptr ? World->SpawnActor<AKalmalaCharacter>() : nullptr;
    if (!TestNotNull(TEXT("Weak-poison character fixture exists"), Pawn))
    {
        if (World != nullptr) World->DestroyWorld(false);
        return false;
    }

    UKalmalaPlayerStatusComponent* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    UKalmalaResistanceComponent* Defenses = Pawn->FindComponentByClass<UKalmalaResistanceComponent>();
    const FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(AKalmalaCharacter::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Weak-poison status component exists"), Status)
        || !TestNotNull(TEXT("Poison resistance profile exists"), Defenses)
        || !TestNotNull(TEXT("Health fixture property exists"), HealthProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    HealthProperty->SetPropertyValue_InContainer(Pawn, 100.0f);
    Defenses->Profile.Armor = 100.0f;
    TestFalse(TEXT("Zero-duration poison is rejected"), Status->ApplyWeakPoisonFromServer(0.0f));
    TestFalse(TEXT("Poison above the shared ten-second bound is rejected"),
        Status->ApplyWeakPoisonFromServer(UKalmalaPlayerStatusComponent::WeakPoisonMaximumSeconds + 0.01f));
    TestFalse(TEXT("Non-finite poison duration is rejected"),
        Status->ApplyWeakPoisonFromServer(TNumericLimits<float>::QuietNaN()));
    TestTrue(TEXT("Authority applies the Viper's five-second weak poison"), Status->ApplyWeakPoisonFromServer(5.0f));
    TestEqual(TEXT("Weak poison starts at five seconds"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId), 5.0f);

    Status->AdvanceFromServer(0.5f);
    TestEqual(TEXT("Weak poison waits for its first one-second tick"), Pawn->GetHealth(), 100.0f);
    Status->AdvanceFromServer(0.5f);
    TestEqual(TEXT("The first poison tick deals one Poison damage through physical armor"), Pawn->GetHealth(), 99.0f);

    Status->AdvanceFromServer(0.5f);
    TestEqual(TEXT("The next poison tick still waits for a full interval"), Pawn->GetHealth(), 99.0f);
    TestTrue(TEXT("A second Viper bite refreshes poison"), Status->ApplyWeakPoisonFromServer(5.0f));
    TestEqual(TEXT("Refresh extends expiry without adding another status"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId), 5.0f);
    TestEqual(TEXT("Weak poison never stacks entries"), Status->GetStatuses().Num(), 1);
    Status->AdvanceFromServer(0.49f);
    TestEqual(TEXT("Refresh preserves the existing tick clock"), Pawn->GetHealth(), 99.0f);
    Status->AdvanceFromServer(0.01f);
    TestEqual(TEXT("The tick remains on the original cadence after refresh"), Pawn->GetHealth(), 98.0f);

    for (int32 Tick = 0; Tick < 4; ++Tick)
    {
        Status->AdvanceFromServer(1.0f);
    }
    TestEqual(TEXT("Weak poison applies one damage on each due second"), Pawn->GetHealth(), 94.0f);
    Status->AdvanceFromServer(0.5f);
    TestFalse(TEXT("Refreshed weak poison expires after its remaining five seconds"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId));

    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("A client cannot author Viper poison"), Status->ApplyWeakPoisonFromServer(5.0f));
    Status->AdvanceFromServer(1.0f);
    TestEqual(TEXT("Client status updates cannot add poison damage"), Pawn->GetHealth(), 94.0f);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaDeathAdderMajorPoisonStatusTest,
    "Kalmala.Gameplay.Status.DeathAdderMajorPoison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaDeathAdderMajorPoisonStatusTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World != nullptr ? World->SpawnActor<AKalmalaCharacter>() : nullptr;
    if (!TestNotNull(TEXT("Major-poison character fixture exists"), Pawn))
    {
        if (World != nullptr) World->DestroyWorld(false);
        return false;
    }

    UKalmalaPlayerStatusComponent* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    UKalmalaResistanceComponent* Defenses = Pawn->FindComponentByClass<UKalmalaResistanceComponent>();
    const FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(AKalmalaCharacter::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Major-poison status component exists"), Status)
        || !TestNotNull(TEXT("Poison defense profile exists"), Defenses)
        || !TestNotNull(TEXT("Health fixture property exists"), HealthProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    HealthProperty->SetPropertyValue_InContainer(Pawn, 100.0f);
    Defenses->Profile.Armor = 100.0f;
    TestFalse(TEXT("Zero-duration major poison is rejected"), Status->ApplyMajorPoisonFromServer(0.0f));
    TestFalse(TEXT("Major poison cannot exceed its ten-second bound"),
        Status->ApplyMajorPoisonFromServer(UKalmalaPlayerStatusComponent::MajorPoisonMaximumSeconds + 0.01f));
    TestFalse(TEXT("Non-finite major poison duration is rejected"),
        Status->ApplyMajorPoisonFromServer(TNumericLimits<float>::QuietNaN()));
    TestTrue(TEXT("An accepted Death Adder bite can apply major poison"), Status->ApplyMajorPoisonFromServer(10.0f));
    TestEqual(TEXT("Major poison starts with ten seconds remaining"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::MajorPoisonStatusId), 10.0f);

    Status->AdvanceFromServer(0.4f);
    TestTrue(TEXT("A second Death Adder refreshes the shared major effect"), Status->ApplyMajorPoisonFromServer(10.0f));
    TestEqual(TEXT("Major refresh does not add a second status row"), Status->GetStatuses().Num(), 1);
    TestEqual(TEXT("Major refresh retains its full bounded duration"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::MajorPoisonStatusId), 10.0f);
    Status->AdvanceFromServer(0.59f);
    TestEqual(TEXT("The shared poison clock does not tick early"), Pawn->GetHealth(), 100.0f);
    Status->AdvanceFromServer(0.01f);
    TestEqual(TEXT("First major tick deals five Poison through physical armor"), Pawn->GetHealth(), 95.0f);

    TestTrue(TEXT("Weak poison can remain independently under major poison"),
        Status->ApplyWeakPoisonFromServer(5.0f));
    TestEqual(TEXT("Weak and major poison retain separate status timers"), Status->GetStatuses().Num(), 2);
    TestEqual(TEXT("Weak applications do not extend the active major timer"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::MajorPoisonStatusId), 9.0f);
    for (int32 Tick = 0; Tick < 8; ++Tick) Status->AdvanceFromServer(1.0f);
    TestEqual(TEXT("Only one five-damage major tick is applied per due second"), Pawn->GetHealth(), 55.0f);
    TestEqual(TEXT("Major poison has one second left before its tenth tick"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::MajorPoisonStatusId), 1.0f);

    TestTrue(TEXT("A weak bite shortly before major expiry remains independently active"),
        Status->ApplyWeakPoisonFromServer(5.0f));
    TestEqual(TEXT("Weak bite cannot extend major poison"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::MajorPoisonStatusId), 1.0f);
    Status->AdvanceFromServer(1.0f);
    TestEqual(TEXT("Major poison applies its tenth tick at expiry"), Pawn->GetHealth(), 50.0f);
    TestFalse(TEXT("The major effect expires after ten ticks"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId));
    TestEqual(TEXT("An unexpired weak dose resumes with its own remaining time"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId), 4.0f);
    Status->AdvanceFromServer(1.0f);
    TestEqual(TEXT("Weak poison resumes at one damage per second"), Pawn->GetHealth(), 49.0f);

    Pawn->SetRole(ROLE_AutonomousProxy);
    TestFalse(TEXT("A client cannot author major poison"), Status->ApplyMajorPoisonFromServer(10.0f));
    Status->AdvanceFromServer(1.0f);
    TestEqual(TEXT("Client status updates cannot add poison damage"), Pawn->GetHealth(), 49.0f);
    Pawn->SetRole(ROLE_Authority);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 1.0f);
    Status->AdvanceFromServer(0.1f);
    TestFalse(TEXT("Death clears any remaining poison status"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId));

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaAntidoteStatusTest,
    "Kalmala.Gameplay.Status.Antidote",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaAntidoteStatusTest::RunTest(const FString& Parameters)
{
    TArray<FKalmalaPlayerStatusEntry> Entries = {
        {UKalmalaPlayerStatusComponent::WeakPoisonStatusId, 4.0f},
        {UKalmalaPlayerStatusComponent::MajorPoisonStatusId, 8.0f}
    };
    TestTrue(TEXT("Antidote is a registered draught action"),
        UKalmalaPlayerStatusComponent::IsKnownDraughtItem(TEXT("Antidote")));
    TestTrue(TEXT("Antidote clears weak poison and downgrades major poison"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("Antidote")));
    TestFalse(TEXT("Antidote removes the major poison profile"),
        Entries.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == UKalmalaPlayerStatusComponent::MajorPoisonStatusId; }));
    const FKalmalaPlayerStatusEntry* ConvertedPoison = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::WeakPoisonStatusId; });
    TestTrue(TEXT("Major poison converts to weak for at most five seconds"),
        ConvertedPoison && ConvertedPoison->RemainingSeconds == UKalmalaPlayerStatusComponent::AntidoteWeakPoisonMaximumSeconds);
    const FKalmalaPlayerStatusEntry* Resistance = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::PoisonResistanceStatusId; });
    const FKalmalaPlayerStatusEntry* Cooldown = Entries.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::DraughtCooldownStatusId; });
    TestTrue(TEXT("Antidote grants ten minutes of poison resistance"),
        Resistance && Resistance->RemainingSeconds == UKalmalaPlayerStatusComponent::AntidoteResistanceMaximumSeconds);
    TestTrue(TEXT("Antidote starts the shared sixty-second cooldown"),
        Cooldown && Cooldown->RemainingSeconds == UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds);
    TestFalse(TEXT("Resistance prevents another Antidote"),
        UKalmalaPlayerStatusComponent::ApplyDraught(Entries, TEXT("Antidote")));

    TArray<FKalmalaPlayerStatusEntry> ShortMajor = {
        {UKalmalaPlayerStatusComponent::MajorPoisonStatusId, 3.0f}
    };
    TestTrue(TEXT("Antidote accepts an already-active short major dose"),
        UKalmalaPlayerStatusComponent::ApplyDraught(ShortMajor, TEXT("Antidote")));
    ConvertedPoison = ShortMajor.FindByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
        { return Entry.StatusId == UKalmalaPlayerStatusComponent::WeakPoisonStatusId; });
    TestTrue(TEXT("Conversion preserves a shorter remaining major duration"),
        ConvertedPoison && ConvertedPoison->RemainingSeconds == 3.0f);
    UKalmalaPlayerStatusComponent::Advance(ShortMajor, UKalmalaPlayerStatusComponent::AntidoteResistanceMaximumSeconds);
    TestFalse(TEXT("Converted poison expires without restoring major strength"),
        ShortMajor.ContainsByPredicate([](const FKalmalaPlayerStatusEntry& Entry)
            { return Entry.StatusId == UKalmalaPlayerStatusComponent::WeakPoisonStatusId
                || Entry.StatusId == UKalmalaPlayerStatusComponent::MajorPoisonStatusId; }));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World ? World->SpawnActor<AKalmalaCharacter>() : nullptr;
    APlayerController* Controller = World ? World->SpawnActor<APlayerController>() : nullptr;
    if (!TestNotNull(TEXT("Antidote character fixture spawned"), Pawn)
        || !TestNotNull(TEXT("Antidote controller fixture spawned"), Controller))
    {
        if (World) World->DestroyWorld(false);
        return false;
    }
    Controller->Possess(Pawn);
    auto* Status = Pawn->FindComponentByClass<UKalmalaPlayerStatusComponent>();
    auto* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    auto* Inventory = Pawn->GetInventoryComponent();
    const FFloatProperty* HealthProperty = FindFProperty<FFloatProperty>(AKalmalaCharacter::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Antidote status component exists"), Status)
        || !TestNotNull(TEXT("Antidote crafting component exists"), Crafting)
        || !TestNotNull(TEXT("Antidote inventory exists"), Inventory)
        || !TestNotNull(TEXT("Antidote health fixture property exists"), HealthProperty))
    {
        World->DestroyWorld(false);
        return false;
    }

    HealthProperty->SetPropertyValue_InContainer(Pawn, 100.0f);
    TestTrue(TEXT("Living player can carry multiple Antidote doses"),
        Inventory->TryGrantFromServer(TEXT("Antidote"), 3));
    TestTrue(TEXT("Weak poison fixture starts on the server"), Status->ApplyWeakPoisonFromServer(4.0f));
    FString Reason;
    TestTrue(TEXT("Antidote can be used at full health and consumes exactly one dose"),
        Crafting->ConsumeFoodFromServer(TEXT("Antidote"), Reason));
    TestEqual(TEXT("Accepted Antidote consumes one item"), Inventory->GetQuantity(TEXT("Antidote")), 2);
    TestFalse(TEXT("Antidote clears active weak poison"), Status->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId));
    TestTrue(TEXT("Antidote publishes owner poison resistance"), Status->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId));
    TestEqual(TEXT("Resistance publishes the ten-minute duration"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId),
        UKalmalaPlayerStatusComponent::AntidoteResistanceMaximumSeconds);
    TestFalse(TEXT("Resistance blocks new weak poison"), Status->ApplyWeakPoisonFromServer(5.0f));
    TestTrue(TEXT("Resistance downgrades new major poison"), Status->ApplyMajorPoisonFromServer(10.0f));
    TestFalse(TEXT("Resistant hits do not publish major poison"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::MajorPoisonStatusId));
    TestEqual(TEXT("Resistant major poison is capped at five weak seconds"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId),
        UKalmalaPlayerStatusComponent::AntidoteWeakPoisonMaximumSeconds);
    TestFalse(TEXT("Shared cooldown rejects another Antidote"),
        Crafting->ConsumeFoodFromServer(TEXT("Antidote"), Reason));
    TestEqual(TEXT("Cooldown rejection preserves the next dose"), Inventory->GetQuantity(TEXT("Antidote")), 2);

    Status->AdvanceFromServer(60.0f);
    TestFalse(TEXT("Converted weak poison expires independently of resistance"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::WeakPoisonStatusId));
    TestFalse(TEXT("Shared cooldown expires after sixty server seconds"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId));
    TestTrue(TEXT("Antidote cannot refresh an active resistance"),
        !Crafting->ConsumeFoodFromServer(TEXT("Antidote"), Reason));
    TestEqual(TEXT("Active-resistance rejection preserves the dose"), Inventory->GetQuantity(TEXT("Antidote")), 2);
    Status->AdvanceFromServer(540.0f);
    TestFalse(TEXT("Ten-minute resistance expires on server time"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId));
    TestTrue(TEXT("Major poison is restored only for a new hit after resistance expires"),
        Status->ApplyMajorPoisonFromServer(10.0f));
    TestTrue(TEXT("Antidote can be used again after resistance expires"),
        Crafting->ConsumeFoodFromServer(TEXT("Antidote"), Reason));
    TestEqual(TEXT("Second accepted Antidote consumes exactly one dose"), Inventory->GetQuantity(TEXT("Antidote")), 1);
    TestEqual(TEXT("Second Antidote downgrades active major poison to five weak seconds"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::WeakPoisonStatusId),
        UKalmalaPlayerStatusComponent::AntidoteWeakPoisonMaximumSeconds);
    HealthProperty->SetPropertyValue_InContainer(Pawn, 1.0f);
    Status->AdvanceFromServer(1.0f);
    TestFalse(TEXT("Death ends poison resistance"),
        Status->HasStatus(UKalmalaPlayerStatusComponent::PoisonResistanceStatusId));
    TestEqual(TEXT("Death does not refund the dose"), Inventory->GetQuantity(TEXT("Antidote")), 1);
    TestEqual(TEXT("Death does not clear the shared recovery cooldown"),
        Status->GetRemainingSeconds(UKalmalaPlayerStatusComponent::DraughtCooldownStatusId),
        UKalmalaPlayerStatusComponent::DraughtCooldownMaximumSeconds - 1.0f);

    World->DestroyWorld(false);
    return true;
}

#endif
