#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
#include "KalmalaCharacterMovementComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerStatusComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaAccessoryStatsTest, "Kalmala.Gameplay.Status.AccessoryStats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaAccessoryStatsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    AKalmalaCharacter* Ally = World->SpawnActor<AKalmalaCharacter>();
    if (!Pawn || !Ally) { World->DestroyWorld(false); return false; }
    auto* Inventory = Pawn->GetInventoryComponent();
    auto* Status = Pawn->GetStatusComponent();
    auto* Movement = CastChecked<UKalmalaCharacterMovementComponent>(Pawn->GetCharacterMovement());
    const FFloatProperty* Health = FindFProperty<FFloatProperty>(AKalmalaCharacter::StaticClass(), TEXT("Health"));
    if (!TestNotNull(TEXT("Health fixture property"), Health)) { World->DestroyWorld(false); return false; }
    FString Reason;
    const auto Equip = [&](const FName Id)
    {
        if (!Inventory->TryGrantFromServer(Id, 1)) return false;
        const auto* Stack = Inventory->GetStacks().FindByPredicate([Id](const auto& Entry) { return Entry.ItemId == Id; });
        return Stack && Inventory->EquipGearFromServer(Stack->ItemGuids[0], Reason);
    };
    Health->SetPropertyValue_InContainer(Pawn, 70.0f);
    Movement->TryConsumeStaminaFromServer(60.0f);
    TestTrue(TEXT("Garnet equips"), Equip(TEXT("GarnetNecklace")));
    TestTrue(TEXT("Moonstone equips"), Equip(TEXT("MoonstoneRing")));
    TestEqual(TEXT("Flat health bonus"), Pawn->GetMaximumHealth(), 120.0f);
    TestEqual(TEXT("Flat stamina bonus"), Movement->GetMaximumStamina(), 120.0f);
    TestEqual(TEXT("Equip never heals"), Pawn->GetHealth(), 70.0f);
    TestEqual(TEXT("Equip never refills"), Movement->GetStamina(), 40.0f);
    Health->SetPropertyValue_InContainer(Pawn, 110.0f);
    TestTrue(TEXT("Health above 100 can still need a draught"), Status->CanApplyDraughtFromServer(TEXT("BerryRecoveryDraught")));
    TestTrue(TEXT("Recovery reaches increased maximum"), Pawn->RestoreHealthFromDraughtFromServer(20.0f));
    TestEqual(TEXT("Recovery clamps to Garnet maximum"), Pawn->GetHealth(), 120.0f);
    TestFalse(TEXT("Full effective health rejects recovery"), Pawn->RestoreHealthFromDraughtFromServer(1.0f));
    TestTrue(TEXT("Vigor coexists"), Movement->SetBearsVigorFromServer(true));
    TestEqual(TEXT("Vigor sums with ring"), Movement->GetMaximumStamina(), 160.0f);
    TestTrue(TEXT("Replace with Dusk"), Equip(TEXT("DuskRing")));
    TestEqual(TEXT("Replacement recalculates stamina"), Movement->GetMaximumStamina(), 150.0f);
    TestEqual(TEXT("Health contributions sum"), Pawn->GetMaximumHealth(), 130.0f);
    Health->SetPropertyValue_InContainer(Pawn, 100.0f);
    TestTrue(TEXT("Hybrid ring receives normal physical hit"), Pawn->ApplyWildlifeDamageFromServer(Ally, 20.0f));
    TestEqual(TEXT("Dusk Armor contributes five once"), Pawn->GetHealth(), 85.0f);
    Pawn->RefreshEquippedStatsFromServer();
    TestEqual(TEXT("Repeated refresh cannot stack"), Pawn->GetMaximumHealth(), 130.0f);
    Health->SetPropertyValue_InContainer(Pawn, 129.0f);
    TestTrue(TEXT("Remove health necklace"), Inventory->UnequipGearFromServer(EKalmalaGearSlot::Necklace, Reason));
    TestEqual(TEXT("Removal clamps health"), Pawn->GetHealth(), 110.0f);
    TestTrue(TEXT("Remove ring"), Inventory->UnequipGearFromServer(EKalmalaGearSlot::Ring, Reason));
    TestEqual(TEXT("No stale health bonus"), Pawn->GetMaximumHealth(), 100.0f);
    TestEqual(TEXT("Ring removal retains Vigor"), Movement->GetMaximumStamina(), 140.0f);
    Movement->SetBearsVigorFromServer(false);
    TestTrue(TEXT("Aquamarine equips"), Equip(TEXT("AquamarineRing")));
    const float Before = Movement->GetStamina();
    Movement->AdvanceStaminaFromServer(0.25f);
    TestTrue(TEXT("Active recovery gains exactly 15 percent"), FMath::IsNearlyEqual(Movement->GetStamina() - Before, 15.0f * 0.25f * 1.15f));
    TestTrue(TEXT("Tourmaline equips"), Equip(TEXT("TourmalineNecklace")));
    Health->SetPropertyValue_InContainer(Pawn, 50.0f);
    TestTrue(TEXT("Incoming Mending accepted"), Pawn->ReceiveMendingFromServer(Ally, 20.0f));
    TestEqual(TEXT("Incoming heal multiplied once"), Pawn->GetHealth(), 72.0f);
    Status->ApplyDraughtFromServer(TEXT("BerryRecoveryDraught"));
    Status->AdvanceFromServer(2.5f);
    TestEqual(TEXT("Partial draught tick uses equipped bonus"), Pawn->GetHealth(), 77.5f);
    Inventory->UnequipGearFromServer(EKalmalaGearSlot::Necklace, Reason);
    Status->AdvanceFromServer(2.5f);
    TestEqual(TEXT("Removal affects subsequent tick immediately"), Pawn->GetHealth(), 82.5f);
    TestEqual(TEXT("Other owner maximum remains independent"), Ally->GetMaximumHealth(), 100.0f);
    TestEqual(TEXT("Other owner healing modifier remains neutral"), Ally->GetStatusComponent()->GetAccessoryModifiers().HealingReceived, 0.0f);
    TestFalse(TEXT("Unowned GUID rejected"), Inventory->EquipGearFromServer(FGuid::NewGuid(), Reason));
    TestEqual(TEXT("Rejected request leaves stats unchanged"), Pawn->GetHealth(), 82.5f);
    TestTrue(TEXT("Quartz equips"), Equip(TEXT("QuartzNecklace")));
    Health->SetPropertyValue_InContainer(Pawn, 100.0f);
    FKalmalaDamageChannels Physical; Physical.Slash = 20.0f;
    TestTrue(TEXT("Authoritative physical hit accepted"), Pawn->ApplyWildlifeDamageFromServer(Ally, Physical));
    TestEqual(TEXT("Quartz Armor subtracts ten once"), Pawn->GetHealth(), 90.0f);
    FKalmalaDamageChannels Elemental; Elemental.Poison = 10.0f;
    Pawn->ApplyWildlifeDamageFromServer(Ally, Elemental);
    TestEqual(TEXT("Quartz Armor does not resist elemental damage"), Pawn->GetHealth(), 80.0f);
    const auto* Quartz = Inventory->FindEquippedGear(EKalmalaGearSlot::Necklace);
    TestTrue(TEXT("Accessory never accumulates wearable damage"), Quartz && Quartz->ConditionSpent == 0);
    TestTrue(TEXT("Jade equips"), Equip(TEXT("JadeRing")));
    TestTrue(TEXT("Jade reduces global stamina costs"), FMath::IsNearlyEqual(Status->CalculateStaminaCost(20.0f), 18.0f));
    TestTrue(TEXT("Support inherits Jade global delta"), FMath::IsNearlyEqual(Status->CalculateStaminaCost(20.0f, true), 18.0f));
    Status->ApplyWetFromServer();
    Status->ApplyFoodFromServer(TEXT("CookedBoarMeat"));
    TestTrue(TEXT("Wet Meal Jade add once"), FMath::IsNearlyEqual(Status->CalculateStaminaCost(20.0f), 19.0f));
    Inventory->UnequipGearFromServer(EKalmalaGearSlot::Ring, Reason);
    TestTrue(TEXT("Jade removal immediately restores future cost"), FMath::IsNearlyEqual(Status->CalculateStaminaCost(20.0f), 21.0f));
    TestTrue(TEXT("Amethyst equips"), Equip(TEXT("AmethystNecklace")));
    TestTrue(TEXT("Amethyst excludes ordinary costs"), FMath::IsNearlyEqual(Status->CalculateStaminaCost(20.0f), 21.0f));
    TestTrue(TEXT("Amethyst adds only on support resolution"), FMath::IsNearlyEqual(Status->CalculateStaminaCost(20.0f, true), 18.0f));
    TestEqual(TEXT("A zero-cost support action remains free"), Status->CalculateStaminaCost(0.0f, true), 0.0f);
    TestTrue(TEXT("Jade and Amethyst coexist"), Equip(TEXT("JadeRing")));
    TestTrue(TEXT("Mixed accessory and status deltas sum once"), FMath::IsNearlyEqual(Status->CalculateStaminaCost(20.0f, true), 16.0f));
    TestTrue(TEXT("Aquamarine replaces Jade"), Equip(TEXT("AquamarineRing")));
    Movement->SetMovementMode(MOVE_Walking);
    Movement->Velocity = FVector(100,0,0);
    Movement->SetSprintRequested(true);
    const float BeforeSprint = Movement->GetStamina();
    Movement->AdvanceStaminaFromServer(.25f);
    TestTrue(TEXT("Aquamarine cannot regenerate while sprint consumes"), Movement->GetStamina() < BeforeSprint);
    Movement->SetSprintRequested(false);
    Movement->Velocity = FVector::ZeroVector;
    const float BeforeDraught = Movement->GetStamina();
    Movement->RestoreStaminaFromDraughtFromServer(20.0f);
    TestTrue(TEXT("Aquamarine excludes immediate draught restoration"), FMath::IsNearlyEqual(Movement->GetStamina(), BeforeDraught + 20.0f));
    for (int32 Dose = 0; Dose < 5; ++Dose) Movement->RestoreStaminaFromDraughtFromServer(20.0f);
    Movement->AdvanceStaminaFromServer(.25f);
    TestEqual(TEXT("Recovery cannot exceed effective maximum"), Movement->GetStamina(), Movement->GetMaximumStamina());
    Inventory->UnequipGearFromServer(EKalmalaGearSlot::Ring, Reason);
    Movement->TryConsumeStaminaFromServer(60.0f);
    const float BeforeUnmodified = Movement->GetStamina();
    Movement->AdvanceStaminaFromServer(.25f);
    TestTrue(TEXT("Removal restores future base recovery"), FMath::IsNearlyEqual(Movement->GetStamina() - BeforeUnmodified, 3.75f));
    TestTrue(TEXT("Tourmaline re-equips"), Equip(TEXT("TourmalineNecklace")));
    Health->SetPropertyValue_InContainer(Ally, 10.0f);
    TestTrue(TEXT("Owner can send ordinary Mending"), Ally->ReceiveMendingFromServer(Pawn, 20.0f));
    TestEqual(TEXT("Tourmaline never increases outgoing healing"), Ally->GetHealth(), 30.0f);
    Health->SetPropertyValue_InContainer(Pawn, 90.0f);
    Pawn->ReceiveMendingFromServer(Ally, 20.0f);
    TestEqual(TEXT("Incoming bonus clamps to maximum health"), Pawn->GetHealth(), Pawn->GetMaximumHealth());
    TestFalse(TEXT("Full-health Mending remains rejected"), Pawn->ReceiveMendingFromServer(Ally, 20.0f));
    Health->SetPropertyValue_InContainer(Pawn, 1.0f);
    TestFalse(TEXT("Healing bonus cannot resurrect"), Pawn->RestoreHealthFromDraughtFromServer(20.0f));
    World->DestroyWorld(false);
    return true;
}
#endif
