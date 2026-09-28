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
    struct FExpectedRecipe { FName Id; FName Output; int32 MaxBatch; bool bRequiresHearth; bool bRequiresLitHearth; TArray<FKalmalaInventoryStack> Costs; FName ToolOutput = NAME_None; FName Station = NAME_None; };
    const TArray<FExpectedRecipe> Expected = {
        {TEXT("Fuel"), TEXT("Fuel"), 5, false, false, {{TEXT("Wood"),2},{TEXT("Fibre"),1}}},
        {TEXT("Timber"), TEXT("ConstructionSupply"), 5, false, false, {{TEXT("Wood"),3},{TEXT("Fibre"),2}}},
        {TEXT("Campfire"), TEXT("CampfireKit"), 1, false, false, {{TEXT("Stone"),5},{TEXT("Wood"),3}}},
        {TEXT("Workbench"), TEXT("WorkbenchKit"), 1, false, false, {{TEXT("ConstructionSupply"),3},{TEXT("Stone"),2}}},
        {TEXT("Forge"), TEXT("ForgeKit"), 1, false, false, {{TEXT("ConstructionSupply"),5},{TEXT("Stone"),6}}},
        {TEXT("WorkbenchToolRack"), TEXT("WorkbenchToolRackKit"), 1, false, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),2}}},
        {TEXT("ForgeAnvil"), TEXT("ForgeAnvilKit"), 1, false, false, {{TEXT("ConstructionSupply"),3},{TEXT("Stone"),4}}},
        {TEXT("GrindingStone"), TEXT("GrindingStoneKit"), 1, false, false, {{TEXT("ConstructionSupply"),2},{TEXT("Stone"),4}}, NAME_None, TEXT("WorkbenchKit")},
        {TEXT("Storage"), TEXT("StorageKit"), 1, false, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),4}}},
        {TEXT("CookingRack"), TEXT("CookingRackKit"), 1, false, false, {{TEXT("ConstructionSupply"),3},{TEXT("Fibre"),2}}},
        {TEXT("Cauldron"), TEXT("CauldronKit"), 1, true, false, {{TEXT("ConstructionSupply"),3},{TEXT("Stone"),3}}},
        {TEXT("SmokeFrame"), TEXT("SmokeFrameKit"), 1, false, false, {{TEXT("ConstructionSupply"),3},{TEXT("Fibre"),3}}},
        {TEXT("Floor"), TEXT("FloorKit"), 5, true, false, {{TEXT("ConstructionSupply"),2}}},
        {TEXT("Wall"), TEXT("WallKit"), 5, true, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),2}}},
        {TEXT("Roof"), TEXT("RoofKit"), 5, true, false, {{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),4}}},
        {TEXT("RoastBoarMeat"), TEXT("RoastedFieldMeat"), 5, false, true, {{TEXT("BoarMeat"),1}}},
        {TEXT("RoastDeerMeat"), TEXT("RoastedFieldMeat"), 5, false, true, {{TEXT("DeerMeat"),1}}},
        {TEXT("SimmerBoarBroth"), TEXT("HearthBroth"), 3, false, true, {{TEXT("BoarMeat"),1},{TEXT("Fuel"),1}}},
        {TEXT("SimmerDeerBroth"), TEXT("HearthBroth"), 3, false, true, {{TEXT("DeerMeat"),1},{TEXT("Fuel"),1}}},
        {TEXT("SmokeBoarMeat"), TEXT("SmokedFieldMeat"), 3, false, true, {{TEXT("BoarMeat"),1},{TEXT("Fuel"),1}}},
        {TEXT("SmokeDeerMeat"), TEXT("SmokedFieldMeat"), 3, false, true, {{TEXT("DeerMeat"),1},{TEXT("Fuel"),1}}},
        {TEXT("RaisedStorage"), TEXT("RaisedStorageKit"), 1, false, false,
            {{TEXT("Densewood"),3},{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),2}}, NAME_None, TEXT("WorkbenchKit")},
        {TEXT("Smokehouse"), TEXT("SmokehouseKit"), 1, false, false,
            {{TEXT("Densewood"),3},{TEXT("ConstructionSupply"),2},{TEXT("Fibre"),2}}, NAME_None, TEXT("WorkbenchKit")},
    };
    TestEqual(TEXT("Recipe catalogue matches its exact verified set"), Recipes->Recipes.Num(), Expected.Num());
    for (const FExpectedRecipe& Definition : Expected)
    {
        const FKalmalaRecipe* Recipe = Recipes->Find(Definition.Id);
        if (!TestNotNull(TEXT("Expected recipe is configured"), Recipe)) continue;
        TestEqual(TEXT("Recipe has one bounded output"), Recipe->OutputCount, 1);
        TestEqual(TEXT("Recipe item output matches contract"), Recipe->Output, Definition.Output);
        TestEqual(TEXT("Recipe tool output matches contract"), Recipe->OutputTool, Definition.ToolOutput);
        TestEqual(TEXT("Recipe batch cap matches contract"), Recipe->MaxBatch, Definition.MaxBatch);
        TestEqual(TEXT("Recipe hearth requirement matches contract"), Recipe->bRequiresCampfire, Definition.bRequiresHearth);
        TestEqual(TEXT("Recipe lit-hearth requirement matches contract"), Recipe->bRequiresLitCampfire, Definition.bRequiresLitHearth);
        if (!Definition.Station.IsNone()) TestEqual(TEXT("Recipe requires its authored visible station"), Recipe->RequiredStationKit, Definition.Station);
        TestEqual(TEXT("Recipe ingredient count matches contract"), Recipe->Ingredients.Num(), Definition.Costs.Num());
        for (int32 Index = 0; Index < Definition.Costs.Num() && Recipe->Ingredients.IsValidIndex(Index); ++Index)
        {
            TestEqual(TEXT("Recipe ingredient ID matches contract"), Recipe->Ingredients[Index].ItemId, Definition.Costs[Index].ItemId);
            TestEqual(TEXT("Recipe ingredient quantity matches contract"), Recipe->Ingredients[Index].Quantity, Definition.Costs[Index].Quantity);
        }
    }
    for (const FName Id : { FName(TEXT("RoastBoarMeat")), FName(TEXT("RoastDeerMeat")) })
    {
        const auto* Recipe = Recipes->Find(Id);
        if (Recipe) TestEqual(TEXT("Roasts name the required cooking rack"), Recipe->RequiredStationKit, FName(TEXT("CookingRackKit")));
    }
    for (const FName Id : { FName(TEXT("SimmerBoarBroth")), FName(TEXT("SimmerDeerBroth")) })
    {
        const auto* Recipe = Recipes->Find(Id);
        if (Recipe) TestEqual(TEXT("Broth recipes name the required cauldron"), Recipe->RequiredStationKit, FName(TEXT("CauldronKit")));
    }
    for (const FName Id : { FName(TEXT("SmokeBoarMeat")), FName(TEXT("SmokeDeerMeat")) })
    {
        const auto* Recipe = Recipes->Find(Id);
        if (Recipe)
        {
            TestEqual(TEXT("Smoke recipes name the required frame"), Recipe->RequiredStationKit, FName(TEXT("SmokeFrameKit")));
            TestEqual(TEXT("Smoke recipes unlock at Cooking level two"), Recipe->RequiredSkill, EKalmalaSkill::Cooking);
            TestEqual(TEXT("Smoke recipes use the first skill unlock tier"), Recipe->RequiredSkillLevel, 2);
        }
    }
    for (const FName Id : { FName(TEXT("RoastBoarMeat")), FName(TEXT("RoastDeerMeat")),
        FName(TEXT("SimmerBoarBroth")), FName(TEXT("SimmerDeerBroth")),
        FName(TEXT("SmokeBoarMeat")), FName(TEXT("SmokeDeerMeat")) })
    {
        const auto* Recipe = Recipes->Find(Id);
        if (!TestNotNull(TEXT("Prepared-food recipe is configured"), Recipe)) continue;
        TestEqual(TEXT("Food preparation awards the server-owned Cooking skill"), Recipe->ExperienceSkill, EKalmalaSkill::Cooking);
        TestEqual(TEXT("Each accepted food recipe action awards a fixed ten experience"), Recipe->ExperienceAward, 10);
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
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().Output=TEXT("Fuel");
    Bad->Recipes.Last().OutputTool=TEXT("FieldHatchet");
    Bad->Recipes.Last().RequiredStationKit=TEXT("WorkbenchKit");
    Bad->Recipes.Last().MaxBatch=1;
    TestFalse(TEXT("Tool recipes cannot also grant a regular item"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes[0].Ingredients[0].Quantity=MAX_int32;
    TestFalse(TEXT("Overflow definition fails closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().RequiredStationKit=TEXT("Wood");
    TestFalse(TEXT("Non-station items cannot satisfy configured station requirements"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().RequiredStationKit=TEXT("FloorKit");
    TestFalse(TEXT("Non-interactable construction kits cannot satisfy configured station requirements"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().bRequiresLitCampfire=false; Bad->Recipes.Last().bRequiresCampfire=true;
    TestFalse(TEXT("A lit hearth or a construction station cannot be confused with a generic assembly station"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().ExperienceAward=FKalmalaSkillProgressionContract::MaxAwardPerAcceptedAction + 1;
    TestFalse(TEXT("An experience award above the accepted-action cap fails closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().ExperienceSkill=static_cast<EKalmalaSkill>(255);
    TestFalse(TEXT("An unknown skill award fails closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().ExperienceSkill=EKalmalaSkill::None;
    TestFalse(TEXT("Experience cannot be configured without an allowlisted skill"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().RequiredSkill=EKalmalaSkill::Cooking; Bad->Recipes.Last().RequiredSkillLevel=0;
    TestFalse(TEXT("Skill requirement without a minimum level fails closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().RequiredSkill=EKalmalaSkill::None; Bad->Recipes.Last().RequiredSkillLevel=2;
    TestFalse(TEXT("Minimum skill level without a skill fails closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().RequiredSkill=static_cast<EKalmalaSkill>(255); Bad->Recipes.Last().RequiredSkillLevel=2;
    TestFalse(TEXT("Unknown recipe skill requirement fails closed"),Bad->IsValidCatalogue());
    Bad->Recipes=Recipes->Recipes; Bad->Recipes.Last().RequiredSkill=EKalmalaSkill::Cooking; Bad->Recipes.Last().RequiredSkillLevel=FKalmalaSkillProgressionContract::MaxLevel + 1;
    TestFalse(TEXT("Over-bound recipe skill level fails closed"),Bad->IsValidCatalogue());
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
