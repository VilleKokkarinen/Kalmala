#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9ToolProgressionCatalogueTest,
    "Kalmala.Gameplay.M9.ToolProgressionCatalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9ToolProgressionCatalogueTest::RunTest(const FString& Parameters)
{
    const TArray<FKalmalaToolProgressionEntry>& Entries = FKalmalaToolProgressionContract::GetEntries();
    TestTrue(TEXT("Axe progression catalogue is bounded and internally valid"),
        FKalmalaToolProgressionContract::IsCatalogueValid());
    TestEqual(TEXT("Both M9 axe tiers have one authored entry"), Entries.Num(),
        FKalmalaToolProgressionContract::MaxAxeProgressionEntries);

    const FKalmalaToolProgressionEntry* Bronze = FKalmalaToolProgressionContract::FindEntry(TEXT("BronzeAxe"));
    TestNotNull(TEXT("Bronze Axe has a progression entry"), Bronze);
    if (Bronze != nullptr)
    {
        TestEqual(TEXT("Bronze Axe is authored as tool level one"), Bronze->TargetToolLevel, 1);
        TestEqual(TEXT("Bronze Axe uses the level-one Workbench"),
            Bronze->RequiredStation, EKalmalaToolStationKind::Workbench);
        TestEqual(TEXT("Bronze Axe station level matches its target"), Bronze->RequiredStationLevel, 1);
        TestTrue(TEXT("Bronze Axe is the first step in this axe path"),
            Bronze->PreviousToolId.IsNone() && Bronze->PreviousToolLevel == 0);
        TestEqual(TEXT("Bronze Axe acquisition has three bounded materials"), Bronze->MaterialCosts.Num(), 3);
        TestTrue(TEXT("Bronze Axe uses the early-game material set"),
            Bronze->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Wood") && Cost.Quantity == 4; })
            && Bronze->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Stone") && Cost.Quantity == 3; })
            && Bronze->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Fibre") && Cost.Quantity == 2; }));
    }

    const FKalmalaToolProgressionEntry* Iron = FKalmalaToolProgressionContract::FindEntry(TEXT("IronAxe"));
    TestNotNull(TEXT("Iron Axe has a progression entry"), Iron);
    if (Iron != nullptr)
    {
        TestEqual(TEXT("Iron Axe is authored as tool level two"), Iron->TargetToolLevel, 2);
        TestEqual(TEXT("Iron Axe uses the level-two Forge"), Iron->RequiredStation, EKalmalaToolStationKind::Forge);
        TestEqual(TEXT("Iron Axe station level matches its target"), Iron->RequiredStationLevel, 2);
        TestEqual(TEXT("Iron Axe upgrades the level-one Bronze Axe"), Iron->PreviousToolId, FName(TEXT("BronzeAxe")));
        TestEqual(TEXT("Iron Axe prerequisite level is one"), Iron->PreviousToolLevel, 1);
        TestEqual(TEXT("Iron Axe upgrade has four bounded materials"), Iron->MaterialCosts.Num(), 4);
        TestTrue(TEXT("Iron Axe uses Lightwood, Peat Amber, Stone, and Fibre"),
            Iron->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Lightwood") && Cost.Quantity == 3; })
            && Iron->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("PeatAmber") && Cost.Quantity == 2; })
            && Iron->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Stone") && Cost.Quantity == 4; })
            && Iron->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Fibre") && Cost.Quantity == 2; }));
        TestFalse(TEXT("Iron Axe does not require Densewood gated behind itself"),
            Iron->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Densewood"); }));
    }

    const TArray<FKalmalaToolState> StartingTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
    TestFalse(TEXT("Uncrafted Bronze Axe is not granted at character start"),
        StartingTools.ContainsByPredicate([](const FKalmalaToolState& State) { return State.ToolId == TEXT("BronzeAxe"); }));
    TestFalse(TEXT("Uncrafted Iron Axe is not granted at character start"),
        StartingTools.ContainsByPredicate([](const FKalmalaToolState& State) { return State.ToolId == TEXT("IronAxe"); }));

    return true;
}

#endif
