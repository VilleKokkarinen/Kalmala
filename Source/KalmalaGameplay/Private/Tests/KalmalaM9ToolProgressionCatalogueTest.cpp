#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaCraftingComponent.h"
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

    TestEqual(TEXT("Workbench is a buildable level-one station"),
        FKalmalaToolProgressionContract::GetBaseStationLevel(TEXT("WorkbenchKit")), 1);
    TestEqual(TEXT("Forge is a buildable level-one station"),
        FKalmalaToolProgressionContract::GetBaseStationLevel(TEXT("ForgeKit")), 1);
    TestEqual(TEXT("Workbench progression selects its station kit"),
        FKalmalaToolProgressionContract::GetStationKit(EKalmalaToolStationKind::Workbench), FName(TEXT("WorkbenchKit")));
    TestEqual(TEXT("Forge progression selects its station kit"),
        FKalmalaToolProgressionContract::GetStationKit(EKalmalaToolStationKind::Forge), FName(TEXT("ForgeKit")));
    TestTrue(TEXT("Forge supports paid placement and existing construction saves"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("ForgeKit")));

    const TArray<FKalmalaStationAttachmentDefinition>& Attachments =
        FKalmalaToolProgressionContract::GetAttachmentDefinitions();
    TestEqual(TEXT("Workbench and Forge each have one authored attachment"), Attachments.Num(), 2);
    TestEqual(TEXT("Workbench tool rack upgrades only a Workbench"),
        FKalmalaToolProgressionContract::GetAttachmentStationKit(TEXT("WorkbenchToolRackKit")), FName(TEXT("WorkbenchKit")));
    TestEqual(TEXT("Forge anvil upgrades only a Forge"),
        FKalmalaToolProgressionContract::GetAttachmentStationKit(TEXT("ForgeAnvilKit")), FName(TEXT("ForgeKit")));
    TestTrue(TEXT("Both station attachment kits support the validated placement preview"),
        FKalmalaPlacementPreview::IsSupportedKit(TEXT("WorkbenchToolRackKit"))
        && FKalmalaPlacementPreview::IsSupportedKit(TEXT("ForgeAnvilKit")));

    FKalmalaStationAttachmentCandidate RackCandidate;
    RackCandidate.KitId = TEXT("WorkbenchToolRackKit");
    RackCandidate.DistanceToStationCm = FKalmalaToolProgressionContract::MaxAttachmentDistanceCm;
    RackCandidate.bSameWorld = true;
    RackCandidate.bInitialized = true;
    TArray<FKalmalaStationAttachmentCandidate> RackCandidates;
    RackCandidates.Add(RackCandidate);
    TestEqual(TEXT("A same-world nearby tool rack raises its Workbench to level two"),
        FKalmalaToolProgressionContract::DeriveEffectiveStationLevel(TEXT("WorkbenchKit"), RackCandidates), 2);
    TestEqual(TEXT("A tool rack does not raise a Forge level"),
        FKalmalaToolProgressionContract::DeriveEffectiveStationLevel(TEXT("ForgeKit"), RackCandidates), 1);
    RackCandidate.DistanceToStationCm += 0.1f;
    RackCandidates[0] = RackCandidate;
    TestEqual(TEXT("A distant tool rack does not raise station level"),
        FKalmalaToolProgressionContract::DeriveEffectiveStationLevel(TEXT("WorkbenchKit"), RackCandidates), 1);
    RackCandidate.DistanceToStationCm = 100.0f;
    RackCandidate.bSameWorld = false;
    RackCandidates[0] = RackCandidate;
    TestEqual(TEXT("A foreign-world tool rack does not raise station level"),
        FKalmalaToolProgressionContract::DeriveEffectiveStationLevel(TEXT("WorkbenchKit"), RackCandidates), 1);

    FString AttachmentReason;
    TestTrue(TEXT("Server-usable matching station accepts a paid nearby attachment"),
        FKalmalaToolProgressionContract::CanPlaceAttachment(
            TEXT("ForgeAnvilKit"), TEXT("ForgeKit"), 100.0f, true, false, AttachmentReason));
    TestFalse(TEXT("An attachment rejects the wrong station family"),
        FKalmalaToolProgressionContract::CanPlaceAttachment(
            TEXT("ForgeAnvilKit"), TEXT("WorkbenchKit"), 100.0f, true, false, AttachmentReason));
    TestFalse(TEXT("An attachment rejects placement outside its station radius"),
        FKalmalaToolProgressionContract::CanPlaceAttachment(
            TEXT("ForgeAnvilKit"), TEXT("ForgeKit"), 126.0f, true, false, AttachmentReason));
    TestFalse(TEXT("An attachment rejects an unusable or out-of-range station"),
        FKalmalaToolProgressionContract::CanPlaceAttachment(
            TEXT("ForgeAnvilKit"), TEXT("ForgeKit"), 100.0f, false, false, AttachmentReason));
    TestFalse(TEXT("A station cannot receive a duplicate level attachment"),
        FKalmalaToolProgressionContract::CanPlaceAttachment(
            TEXT("ForgeAnvilKit"), TEXT("ForgeKit"), 100.0f, true, true, AttachmentReason));

    const UKalmalaItemCatalogue* Items = GetDefault<UKalmalaItemCatalogue>();
    const FKalmalaItemDefinition* ForgeKit = Items->FindItem(TEXT("ForgeKit"));
    TestNotNull(TEXT("Forge kit is in the item catalogue"), ForgeKit);
    const FKalmalaRecipe* ForgeRecipe = GetDefault<UKalmalaRecipeCatalogue>()->Find(TEXT("Forge"));
    TestNotNull(TEXT("Forge has a paid build recipe"), ForgeRecipe);
    if (ForgeRecipe)
    {
        TestEqual(TEXT("Forge recipe yields its buildable kit"), ForgeRecipe->Output, FName(TEXT("ForgeKit")));
        const FKalmalaInventoryStack* TimberCost = ForgeRecipe->Ingredients.FindByPredicate(
            [](const FKalmalaInventoryStack& Cost) { return Cost.ItemId == TEXT("ConstructionSupply"); });
        const FKalmalaInventoryStack* StoneCost = ForgeRecipe->Ingredients.FindByPredicate(
            [](const FKalmalaInventoryStack& Cost) { return Cost.ItemId == TEXT("Stone"); });
        TestNotNull(TEXT("Forge recipe includes lashed timber"), TimberCost);
        TestNotNull(TEXT("Forge recipe includes fieldstone"), StoneCost);
        if (TimberCost) TestEqual(TEXT("Forge kit costs five lashed timber"), TimberCost->Quantity, 5);
        if (StoneCost) TestEqual(TEXT("Forge kit costs six fieldstone"), StoneCost->Quantity, 6);
    }

    const UKalmalaRecipeCatalogue* Recipes = GetDefault<UKalmalaRecipeCatalogue>();
    TestTrue(TEXT("The complete recipe catalogue accepts both paid station attachment recipes"),
        Recipes->IsValidCatalogue());
    const FKalmalaRecipe* RackRecipe = Recipes->Find(TEXT("WorkbenchToolRack"));
    const FKalmalaRecipe* AnvilRecipe = Recipes->Find(TEXT("ForgeAnvil"));
    TestNotNull(TEXT("Workbench tool rack has a paid build recipe"), RackRecipe);
    TestNotNull(TEXT("Forge anvil has a paid build recipe"), AnvilRecipe);
    if (RackRecipe)
    {
        TestEqual(TEXT("Rack recipe yields a Workbench attachment kit"), RackRecipe->Output, FName(TEXT("WorkbenchToolRackKit")));
        TestEqual(TEXT("Rack recipe selects the Workbench family from its output kit"),
            FKalmalaToolProgressionContract::GetAttachmentStationKit(RackRecipe->Output), FName(TEXT("WorkbenchKit")));
        TestTrue(TEXT("Rack recipe has paid material costs"), !RackRecipe->Ingredients.IsEmpty());
    }
    if (AnvilRecipe)
    {
        TestEqual(TEXT("Anvil recipe yields a Forge attachment kit"), AnvilRecipe->Output, FName(TEXT("ForgeAnvilKit")));
        TestEqual(TEXT("Anvil recipe selects the Forge family from its output kit"),
            FKalmalaToolProgressionContract::GetAttachmentStationKit(AnvilRecipe->Output), FName(TEXT("ForgeKit")));
        TestTrue(TEXT("Anvil recipe has paid material costs"), !AnvilRecipe->Ingredients.IsEmpty());
    }

    const UFunction* PlaceIntent = UKalmalaCraftingComponent::StaticClass()->FindFunctionByName(TEXT("ServerPlaceConstruction"));
    if (TestNotNull(TEXT("Construction placement intent exists"), PlaceIntent))
    {
        TestTrue(TEXT("Construction placement is an owning-client server RPC"),
            PlaceIntent->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Placement submits only a kit identity"), int32(PlaceIntent->NumParms), 1);
        TestNotNull(TEXT("Placement cannot submit a station, transform, level, or cost"),
            PlaceIntent->FindPropertyByName(TEXT("KitId")));
    }

    return true;
}

#endif
