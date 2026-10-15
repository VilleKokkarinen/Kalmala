#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaInventoryComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaPlayerStatusComponent.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInventoryHotbarTest,
    "Kalmala.Gameplay.Inventory.MainHandHotbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInventoryHotbarTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Hotbar authority world exists"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    // No BeginPlay/game mode: avoid profile loading, save writes and world generation.
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    auto* Controller = World->SpawnActor<APlayerController>();
    if (!TestNotNull(TEXT("Hotbar pawn exists"), Pawn)
        || !TestNotNull(TEXT("Hotbar controller exists"), Controller)) return false;
    Controller->Possess(Pawn);
    auto* Inventory = Pawn->GetInventoryComponent();
    auto* Status = Pawn->GetStatusComponent();
    auto* Support = Pawn->GetSupportMagicComponent();
    if (!TestNotNull(TEXT("Inventory exists"), Inventory)
        || !TestNotNull(TEXT("Status exists"), Status)
        || !TestNotNull(TEXT("Support exists"), Support)) return false;

    // Seed two physical copies through reflection only in this isolated test world.
    const FArrayProperty* ToolsProperty = FindFProperty<FArrayProperty>(Pawn->GetClass(), TEXT("CarriedTools"));
    if (!TestNotNull(TEXT("Carried tool fixture property exists"), ToolsProperty)) return false;
    const FStructProperty* ToolType = CastField<FStructProperty>(ToolsProperty->Inner);
    if (!TestTrue(TEXT("Fixture matches the carried tool record type"),
        ToolType && ToolType->Struct == FKalmalaToolState::StaticStruct())) return false;
    const auto InitialTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
    if (!TestTrue(TEXT("Initial tools include a hammer"), !InitialTools.IsEmpty())) return false;
    FKalmalaToolState First = InitialTools[0];
    FKalmalaToolState Second = First;
    Second.InstanceGuid = FGuid::NewGuid();
    *ToolsProperty->ContainerPtrToValuePtr<TArray<FKalmalaToolState>>(Pawn) = {First, Second};
    Inventory->SynchronizeGridFromServer();
    FString Reason;
    if (!TestTrue(TEXT("First copy enters Main Hand"),
        Inventory->SetMainHandFromServer(First.InstanceGuid, Reason))) return false;

    auto UseNumber = [&](const int32 Slot)
    {
        // Advance beyond both hotbar and crafting request cadences.
        World->Tick(LEVELTICK_TimeOnly, 0.25f);
        Inventory->ServerUseHotbarSlot(Slot);
    };
    auto CheckHand = [&](const FGuid Expected)
    {
        TestTrue(TEXT("Main Hand retains the exact physical copy"), Inventory->GetMainHandGuid() == Expected);
        TestFalse(TEXT("Main Hand has no grid alias"), Inventory->GetGridInstanceGuids().Contains(Expected));
        TestEqual(TEXT("Selection preserves both physical tool records"), Pawn->GetCarriedToolInventory().Num(), 2);
        for (const FKalmalaToolState& Original : {First, Second})
        {
            const FKalmalaToolState* Current = Pawn->GetCarriedToolInventory().FindByPredicate(
                [&](const FKalmalaToolState& Tool) { return Tool.InstanceGuid == Original.InstanceGuid; });
            if (TestNotNull(TEXT("Original tool GUID remains owned"), Current))
            {
                TestEqual(TEXT("Tool item identity is unchanged"), Current->ItemId, Original.ItemId);
                TestEqual(TEXT("Tool level is unchanged"), Current->ToolLevel, Original.ToolLevel);
                TestEqual(TEXT("Tool condition is unchanged"), Current->Durability, Original.Durability);
            }
        }
    };
    const auto BeforeInvalid = Inventory->GetGridInstanceGuids();
    for (const int32 Slot : {-1, UKalmalaInventoryComponent::Columns, MAX_int32, 9}) UseNumber(Slot);
    CheckHand(First.InstanceGuid);
    TestTrue(TEXT("Invalid and empty numbers preserve layout"), Inventory->GetGridInstanceGuids() == BeforeInvalid);

    const int32 SecondSlot = Inventory->GetGridInstanceGuids().IndexOfByKey(Second.InstanceGuid);
    if (!TestTrue(TEXT("Second copy has a numbered cell"), SecondSlot >= 0 && SecondSlot < Inventory->Columns)) return false;
    UseNumber(SecondSlot);
    CheckHand(Second.InstanceGuid);
    TestFalse(TEXT("Selected numbered cell becomes empty"), Inventory->GetSlotInstanceGuid(SecondSlot).IsValid());
    TestTrue(TEXT("Displaced copy returns to pack"), Inventory->GetGridInstanceGuids().Contains(First.InstanceGuid));

    const FName FoodId = UKalmalaPlayerStatusComponent::HearthBrothItemId;
    if (!TestTrue(TEXT("Food fixture granted"), Inventory->TryGrantFromServer(FoodId, 2))) return false;
    const int32 FoodSlot = Inventory->GetGridSlots().IndexOfByKey(FoodId);
    if (!TestTrue(TEXT("Food has a numbered cell"), FoodSlot >= 0 && FoodSlot < Inventory->Columns)) return false;
    UseNumber(FoodSlot);
    TestEqual(TEXT("Accepted food use consumes one unit"), Inventory->GetQuantity(FoodId), 1);
    TestTrue(TEXT("Accepted food use applies its gameplay effect"), Status->HasStatus(UKalmalaPlayerStatusComponent::SteadyMealStatusId));
    CheckHand(Second.InstanceGuid);
    UseNumber(FoodSlot);
    TestEqual(TEXT("Active meal rejects repeated food without consuming it"), Inventory->GetQuantity(FoodId), 1);
    CheckHand(Second.InstanceGuid);

    if (!TestTrue(TEXT("Support fixture learned"), Support->LearnEffectFromServer(EKalmalaSupportEffect::HearthShield))) return false;
    const FName ScrollId = UKalmalaSupportMagicComponent::ScrollItemId(EKalmalaSupportEffect::HearthShield);
    const int32 ScrollSlot = Inventory->GetGridSlots().IndexOfByKey(ScrollId);
    if (!TestTrue(TEXT("Scroll has a numbered cell"), ScrollSlot >= 0 && ScrollSlot < Inventory->Columns)) return false;
    UseNumber(ScrollSlot);
    TestEqual(TEXT("Support use reaches accepted gameplay path"), Support->GetFeedback(), EKalmalaSupportFeedback::Accepted);
    TestTrue(TEXT("Support shield is active"), Support->GetHearthShieldStrength() > 0.0f);
    CheckHand(Second.InstanceGuid);
    UseNumber(ScrollSlot);
    TestEqual(TEXT("Cooldown rejects repeated support use"), Support->GetFeedback(), EKalmalaSupportFeedback::Unavailable);
    TestEqual(TEXT("Support use preserves the reusable scroll"), Inventory->GetQuantity(ScrollId), 1);
    CheckHand(Second.InstanceGuid);

    // Replace test stacks with 39 distinct non-stackable items, filling the pack
    // alongside the first tool while the second remains outside it in Main Hand.
    TArray<FKalmalaInventoryStack> FullPack;
    for (int32 Index = 0; Index < Inventory->MaxSlots - 1; ++Index)
    {
        FKalmalaInventoryStack Stack;
        Stack.ItemId = TEXT("BoarHideBoots");
        Stack.Quantity = 1;
        Stack.InstanceGuid = FGuid::NewGuid();
        Stack.ItemGuids.Add(FGuid::NewGuid());
        FullPack.Add(Stack);
    }
    const auto BeforeFill = Inventory->GetStacks();
    if (!TestTrue(TEXT("Full-pack fixture commits through inventory validation"),
        Inventory->TryCommitStacksFromServer(BeforeFill, FullPack))) return false;
    const int32 OrdinarySlot = Inventory->GetGridSlots().IndexOfByKey(TEXT("BoarHideBoots"));
    if (!TestTrue(TEXT("Ordinary item has a numbered cell"), OrdinarySlot >= 0 && OrdinarySlot < Inventory->Columns)) return false;
    const auto BeforeFullSelection = Inventory->GetGridInstanceGuids();
    const FGuid OrdinaryGuid = Inventory->GetSlotInstanceGuid(OrdinarySlot);
    UseNumber(OrdinarySlot);
    CheckHand(Second.InstanceGuid);
    TestTrue(TEXT("Full-pack selection rejection preserves the entire layout"), Inventory->GetGridInstanceGuids() == BeforeFullSelection);
    TestEqual(TEXT("Full-pack selection rejection preserves every stack"), Inventory->GetStacks().Num(), 39);

    if (!TestTrue(TEXT("Free one pack cell"), Inventory->TryConsumeFromServer(TEXT("BoarHideBoots"), 1))) return false;
    const int32 RemainingSlot = Inventory->GetGridInstanceGuids().IndexOfByKey(OrdinaryGuid);
    // The consumed unit may have been the originally selected boots; select a surviving copy.
    const int32 RetrySlot = RemainingSlot != INDEX_NONE ? RemainingSlot
        : Inventory->GetGridSlots().IndexOfByKey(TEXT("BoarHideBoots"));
    const FGuid RetryGuid = Inventory->GetSlotInstanceGuid(RetrySlot);
    UseNumber(RetrySlot);
    TestFalse(TEXT("Ordinary selection returns the hand when space exists"), Inventory->GetMainHandGuid().IsValid());
    TestTrue(TEXT("Ordinary selection publishes the exact selected stack"), Inventory->GetActiveInstanceGuid() == RetryGuid);
    TestTrue(TEXT("Returned hand is back in the pack"), Inventory->GetGridInstanceGuids().Contains(Second.InstanceGuid));
    return true;
}
#endif
