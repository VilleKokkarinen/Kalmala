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
    struct FExpectedRecipe { FName Id; FName Output; int32 MaxBatch; bool bRequiresHearth; TArray<FKalmalaInventoryStack> Costs; };
    const TArray<FExpectedRecipe> Expected = {
        {TEXT("Fuel"), TEXT("Fuel"), 5, false, {{TEXT("Wood"),2},{TEXT("Fibre"),1}}},
        {TEXT("Timber"), TEXT("ConstructionSupply"), 5, false, {{TEXT("Wood"),3},{TEXT("Fibre"),2}}},
        {TEXT("Campfire"), TEXT("CampfireKit"), 1, false, {{TEXT("Stone"),5},{TEXT("Wood"),3}}},
        {TEXT("Workbench"), TEXT("WorkbenchKit"), 1, false, {{TEXT("ConstructionSupply"),3},{TEXT("Stone"),2}}},
        {TEXT("Storage"), TEXT("StorageKit"), 1, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),4}}},
        {TEXT("Floor"), TEXT("FloorKit"), 5, true, {{TEXT("ConstructionSupply"),2}}},
        {TEXT("Wall"), TEXT("WallKit"), 5, true, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),2}}},
        {TEXT("Roof"), TEXT("RoofKit"), 5, true, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),4}}}
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
