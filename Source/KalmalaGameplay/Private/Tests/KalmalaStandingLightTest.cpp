#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStandingLightContractTest,
    "Kalmala.Gameplay.Construction.StandingLightContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaStandingLightContractTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("Standing Light supports a server-rechecked construction preview"),
        FKalmalaPlacementPreview::IsSupportedKit(TEXT("StandingLightKit")));
    TestTrue(TEXT("Standing Light has a 1.5 m height and compact footprint"),
        AKalmalaConstructionActor::GetCollisionExtent(TEXT("StandingLightKit")) == FVector(35.0f, 35.0f, 75.0f));
    TestNotNull(TEXT("Standing Light has presentation-only buildable metadata"),
        Items ? Items->FindBuildable(TEXT("StandingLightKit")) : nullptr);
    TestNull(TEXT("Standing Light is not a portable inventory item"),
        Items ? Items->FindItem(TEXT("StandingLight")) : nullptr);

    const FKalmalaRecipe* Recipe = Recipes ? Recipes->Find(TEXT("StandingLight")) : nullptr;
    TestNotNull(TEXT("Standing Light direct-material recipe loads"), Recipe);
    if (Recipe)
    {
        TestEqual(TEXT("Standing Light recipe maps to the placed object identity"),
            Recipe->BuildableOutput, FName(TEXT("StandingLightKit")));
        TestTrue(TEXT("Standing Light recipe has no portable output"), Recipe->Output.IsNone());
        TArray<FKalmalaInventoryStack> Cost;
        FString Reason;
        TestTrue(TEXT("Standing Light direct material cost resolves"),
            UKalmalaRecipeCatalogue::BuildDirectMaterialCost(TEXT("StandingLightKit"), Cost, Reason));
        TestTrue(TEXT("Standing Light costs exactly 3 Wood and 1 Amber"), Cost.Num() == 2
            && Cost.ContainsByPredicate([](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("Wood") && Stack.Quantity == 3; })
            && Cost.ContainsByPredicate([](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == TEXT("Amber") && Stack.Quantity == 1; }));
    }

    FKalmalaStandingLightState Fuel;
    TestTrue(TEXT("Standing Light starts unlit and unfueled"), Fuel.IsEmpty());
    TestTrue(TEXT("One Wood adds 600 seconds"), Fuel.TryAddWood());
    TestEqual(TEXT("First fuel charge lasts 600 seconds"), Fuel.RemainingFuelSeconds, 600.0f);
    TestTrue(TEXT("A second Wood fills the 1200 second cap"), Fuel.TryAddWood());
    TestEqual(TEXT("Fuel cannot exceed 1200 seconds"), Fuel.RemainingFuelSeconds, 1200.0f);
    TestFalse(TEXT("A full light rejects another Wood charge"), Fuel.TryAddWood());
    TestTrue(TEXT("Fuel advances only by active lit seconds"), Fuel.Advance(450.0f));
    TestEqual(TEXT("Active burn stores remaining seconds"), Fuel.RemainingFuelSeconds, 750.0f);
    TestFalse(TEXT("Fuel rejects a new charge when less than 600 seconds fit"), Fuel.TryAddWood());
    TestTrue(TEXT("A 600 second remaining balance accepts one complete Wood charge"), Fuel.Advance(150.0f) && Fuel.TryAddWood());
    TestEqual(TEXT("Fuel remains within its cap after a refill"), Fuel.RemainingFuelSeconds, 1200.0f);
    TestFalse(TEXT("Zero active time cannot consume fuel"), Fuel.Advance(0.0f));
    TestFalse(TEXT("Non-finite active time cannot consume fuel"), Fuel.Advance(NAN));

    FKalmalaConstructionSaveRecord LightRecord;
    LightRecord.ConstructionId = TEXT("standing-light-contract");
    LightRecord.KitId = TEXT("StandingLightKit");
    LightRecord.Transform = FTransform(FVector(300.0f, 200.0f, 75.0f));
    LightRecord.StandingLight = Fuel;
    TestTrue(TEXT("Current construction save accepts bounded Standing Light fuel"),
        UKalmalaConstructionSaveGameV2::IsValidRecord(LightRecord));
    FKalmalaConstructionSaveRecord WrongKit = LightRecord;
    WrongKit.KitId = TEXT("CampBedKit");
    TestFalse(TEXT("Other construction records reject Standing Light fuel"),
        UKalmalaConstructionSaveGameV2::IsValidRecord(WrongKit));
    TestFalse(TEXT("Missing world data cannot make a Standing Light placement valid"),
        FKalmalaPlacementPreview::Evaluate(nullptr, nullptr, TEXT("StandingLightKit")).bIsValid);
    return true;
}
#endif
