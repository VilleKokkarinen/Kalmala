#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaItemCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCraftingTransactionsTest,"Kalmala.Gameplay.Crafting.Transactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaCraftingTransactionsTest::RunTest(const FString& Parameters)
{
    const auto* Recipes=GetDefault<UKalmalaRecipeCatalogue>();
    TestTrue(TEXT("Configured recipes validate"),Recipes->IsValidCatalogue());
    for(const FName Id:{FName(TEXT("Campfire")),FName(TEXT("Workbench")),FName(TEXT("Storage")),FName(TEXT("Floor")),FName(TEXT("Wall")),FName(TEXT("Roof"))})
        TestNotNull(TEXT("Required recipe exists"),Recipes->Find(Id));
    struct FExpectedRecipe { FName Id; FName Output; int32 MaxBatch; bool bRequiresHearth; bool bRequiresLitHearth; TArray<FKalmalaInventoryStack> Costs; };
    const TArray<FExpectedRecipe> Expected = {
        {TEXT("Fuel"), TEXT("Fuel"), 5, false, false, {{TEXT("Wood"),2},{TEXT("Fibre"),1}}},
        {TEXT("Timber"), TEXT("ConstructionSupply"), 5, false, false, {{TEXT("Wood"),3},{TEXT("Fibre"),2}}},
        {TEXT("Campfire"), TEXT("CampfireKit"), 1, false, false, {{TEXT("Stone"),5},{TEXT("Wood"),3}}},
        {TEXT("Workbench"), TEXT("WorkbenchKit"), 1, false, false, {{TEXT("ConstructionSupply"),3},{TEXT("Stone"),2}}},
        {TEXT("Forge"), TEXT("ForgeKit"), 1, false, false, {{TEXT("ConstructionSupply"),5},{TEXT("Stone"),6}}},
        {TEXT("WorkbenchToolRack"), TEXT("WorkbenchToolRackKit"), 1, false, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),2}}},
        {TEXT("ForgeAnvil"), TEXT("ForgeAnvilKit"), 1, false, false, {{TEXT("ConstructionSupply"),3},{TEXT("Stone"),4}}},
        {TEXT("Storage"), TEXT("StorageKit"), 1, false, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),4}}},
        {TEXT("Floor"), TEXT("FloorKit"), 5, true, false, {{TEXT("ConstructionSupply"),2}}},
        {TEXT("Wall"), TEXT("WallKit"), 5, true, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),2}}},
        {TEXT("Roof"), TEXT("RoofKit"), 5, true, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),4}}},
        {TEXT("RoastBoarMeat"), TEXT("RoastedFieldMeat"), 5, false, true, {{TEXT("BoarMeat"),1}}},
        {TEXT("RoastDeerMeat"), TEXT("RoastedFieldMeat"), 5, false, true, {{TEXT("DeerMeat"),1}}}
    };
    TestEqual(TEXT("Recipe catalogue stays deliberately small"), Recipes->Recipes.Num(), Expected.Num());
    for (const FExpectedRecipe& Definition : Expected)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Definition.Id);
        if (!TestNotNull(TEXT("Expected recipe is configured"), Recipe)) continue;
        TestEqual(TEXT("Recipe has one bounded output"), Recipe->OutputCount, 1);
        TestEqual(TEXT("Recipe output matches contract"), Recipe->Output, Definition.Output);
        TestEqual(TEXT("Recipe batch cap matches contract"), Recipe->MaxBatch, Definition.MaxBatch);
        TestEqual(TEXT("Recipe hearth requirement matches contract"), Recipe->bRequiresCampfire, Definition.bRequiresHearth);
        TestEqual(TEXT("Recipe lit-hearth requirement matches contract"), Recipe->bRequiresLitCampfire, Definition.bRequiresLitHearth);
        TestEqual(TEXT("Recipe ingredient count matches contract"), Recipe->Ingredients.Num(), Definition.Costs.Num());
        for (int32 Index = 0; Index < Definition.Costs.Num() && Recipe->Ingredients.IsValidIndex(Index); ++Index)
        {
            TestEqual(TEXT("Recipe ingredient ID matches contract"), Recipe->Ingredients[Index].ItemId, Definition.Costs[Index].ItemId);
            TestEqual(TEXT("Recipe ingredient quantity matches contract"), Recipe->Ingredients[Index].Quantity, Definition.Costs[Index].Quantity);
        }
    }
    const auto* Fuel=Recipes->Find(TEXT("Fuel")); if(!Fuel) return false;
    TArray<FKalmalaInventoryStack> Costs; int32 Count=0;
    for(int32 Batch:{MIN_int32,-1,0,MAX_int32}) TestFalse(TEXT("Malformed batch"),UKalmalaRecipeCatalogue::Scale(*Fuel,Batch,Costs,Count));
    TestTrue(TEXT("Bounded batch"),UKalmalaRecipeCatalogue::Scale(*Fuel,1,Costs,Count));
    TArray<FKalmalaInventoryStack> Before={{TEXT("Wood"),4},{TEXT("Fibre"),2}}, After;
    FString Reason;
    TestTrue(TEXT("First craft"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,Fuel->Output,Count,After,Reason));
    Before=After;
    TestTrue(TEXT("Second craft consumes remaining ingredients"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,Fuel->Output,Count,After,Reason));
    TestEqual(TEXT("Only output remains"),After.Num(),1);
    TestEqual(TEXT("Exactly two bundles"),After[0].Quantity,2);
    Before=After;
    TestFalse(TEXT("Third attempt cannot duplicate output"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,Fuel->Output,Count,After,Reason));
    TestEqual(TEXT("Failure leaves caller output unchanged"),After[0].Quantity,2);
    Before={{TEXT("Wood"),4},{TEXT("Fibre"),2},{TEXT("Fuel"),20}};
    TestFalse(TEXT("Full output rejects entire craft"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,Fuel->Output,Count,After,Reason));
    TestEqual(TEXT("Inputs untouched on failure"),Before[0].Quantity,4);
    Before={{TEXT("Wood"),4}};
    TestFalse(TEXT("Missing second ingredient is atomic"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,Fuel->Output,Count,After,Reason));
    TestEqual(TEXT("First ingredient was not consumed"),Before[0].Quantity,4);
    for (const FName RetiredRecipe : {FName(TEXT("ReplaceFieldHatchet")), FName(TEXT("ReplaceStonePick")), FName(TEXT("ReplaceReedKnife"))})
        TestNull(TEXT("M7 material-paid tool replacement is retired"), Recipes->Find(RetiredRecipe));

    const auto* Wood = GetDefault<UKalmalaItemCatalogue>()->FindItem(TEXT("Wood"));
    if (Wood != nullptr)
    {
        Before = {{TEXT("Wood"), Wood->MaxStack - 1}};
        TestTrue(TEXT("Catalogue grant builds a scratch inventory candidate"),
            UKalmalaInventoryComponent::BuildGrant(Before, TEXT("Wood"), 1, After, Reason));
        TestEqual(TEXT("Grant candidate reaches but does not exceed the stack limit"), After[0].Quantity, Wood->MaxStack);
        const auto CandidateBeforeReject = After;
        TestFalse(TEXT("Full stack rejects a grant candidate"),
            UKalmalaInventoryComponent::BuildGrant(After, TEXT("Wood"), 1, After, Reason));
        TestEqual(TEXT("Rejected grant leaves the caller candidate unchanged"), After[0].Quantity, CandidateBeforeReject[0].Quantity);
        TestFalse(TEXT("Unknown item cannot build a grant"),
            UKalmalaInventoryComponent::BuildGrant(Before, TEXT("Forged"), 1, After, Reason));
    }
    const auto CostCopy=Costs[0]; Costs.Add(CostCopy);
    TestFalse(TEXT("Duplicate costs reject"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,Fuel->Output,Count,After,Reason));
    Costs={{TEXT("Wood"),MAX_int32}};
    TestFalse(TEXT("Overflow cost rejects"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,Fuel->Output,Count,After,Reason));
    auto* Bad=NewObject<UKalmalaRecipeCatalogue>(); Bad->Recipes=Recipes->Recipes;
    const auto Duplicate=Bad->Recipes[0]; Bad->Recipes.Add(Duplicate);
    TestFalse(TEXT("Duplicate recipe IDs fail closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes[0].Output=TEXT("Forged");
    TestFalse(TEXT("Unknown output fails closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes[0].Ingredients[0].Quantity=MAX_int32;
    TestFalse(TEXT("Overflow definition fails closed"),Bad->IsValidCatalogue());
    auto* Items=GetMutableDefault<UKalmalaItemCatalogue>(); const auto Saved=Items->Items;
    Before={{TEXT("Wood"),2}};
    for(int32 I=0;I<15;++I)
    {
        const FName Id(*FString::Printf(TEXT("TestSlot%d"),I));
        FKalmalaItemDefinition Def; Def.ItemId=Id; Def.DisplayName=Id.ToString(); Def.MaxStack=2; Items->Items.Add(Def);
        Before.Add({Id,1});
    }
    Costs={{TEXT("Wood"),1}};
    TestFalse(TEXT("Full slot count rejects new output"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,TEXT("Fuel"),1,After,Reason));
    Costs[0].Quantity=2;
    TestTrue(TEXT("Consumed stack frees a slot atomically"),UKalmalaInventoryComponent::BuildExchange(Before,Costs,TEXT("Fuel"),1,After,Reason));
    Items->Items=Saved;
    return true;
}
#endif
