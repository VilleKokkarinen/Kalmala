#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaConstructionSaveGame.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9GrindingStoneRepairTest,
    "Kalmala.Gameplay.M9.GrindingStoneRepairAll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaM9GrindingStoneRepairTest::RunTest(const FString& Parameters)
{
    const auto* Items = GetDefault<UKalmalaItemCatalogue>();
    const auto* Recipes = GetDefault<UKalmalaRecipeCatalogue>();
    const FKalmalaRecipe* Recipe = Recipes ? Recipes->Find(TEXT("GrindingStone")) : nullptr;
    TestTrue(TEXT("Grinding stone kit is a catalogue item"), Items && Items->FindItem(TEXT("GrindingStoneKit")));
    TestTrue(TEXT("Grinding stone kit has a paid Workbench recipe"), Recipe
        && Recipe->Output == TEXT("GrindingStoneKit") && Recipe->RequiredStationKit == TEXT("WorkbenchKit")
        && Recipe->MaxBatch == 1);
    TestTrue(TEXT("Grinding stone is a supported buildable and saveable kit"),
        FKalmalaPlacementPreview::IsSupportedKit(TEXT("GrindingStoneKit")));
    FKalmalaConstructionSaveRecord SaveRecord;
    SaveRecord.ConstructionId = TEXT("repair-stone-contract");
    SaveRecord.KitId = TEXT("GrindingStoneKit");
    SaveRecord.Transform = FTransform::Identity;
    TestTrue(TEXT("Grinding stone uses the existing construction save record"),
        UKalmalaConstructionSaveGame::IsValidRecord(SaveRecord));

    TArray<FKalmalaToolState> Before;
    const auto AddTool = [&Before](const FName ToolId, const int32 Durability, const int32 Level)
    {
        FKalmalaToolState State;
        State.ToolId = ToolId;
        State.Durability = Durability;
        State.ToolLevel = Level;
        Before.Add(State);
    };
    AddTool(TEXT("ReedKnife"), 0, 1);
    AddTool(TEXT("FieldHatchet"), 12, 1);
    AddTool(TEXT("StonePick"), 20, 1);
    AddTool(TEXT("BronzeAxe"), 0, 1);
    AddTool(TEXT("IronAxe"), 23, 2);

    TArray<FKalmalaToolState> Repaired;
    int32 RepairedCount = -1;
    TestTrue(TEXT("Server Repair All accepts the validated stone and carried list"),
        FKalmalaToolLifecycleContract::BuildServerRepairAll(true, true, Before, Repaired, RepairedCount));
    TestEqual(TEXT("All four damaged or broken tools are repaired"), RepairedCount, 4);
    TestEqual(TEXT("Repair keeps the list bounded and in its existing slots"), Repaired.Num(), Before.Num());
    for (int32 Index = 0; Index < Before.Num() && Index < Repaired.Num(); ++Index)
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Before[Index].ToolId);
        TestTrue(TEXT("Tool identity is unchanged"), Repaired[Index].ToolId == Before[Index].ToolId);
        TestEqual(TEXT("Tool level is unchanged"), Repaired[Index].ToolLevel, Before[Index].ToolLevel);
        TestEqual(TEXT("Damaged tools become full while full tools stay full"), Repaired[Index].Durability,
            Definition ? Definition->MaxDurability : -1);
    }

    const auto TestRejectedWithoutMutation = [this, &Before](const TCHAR* Label, const bool bServer, const bool bAtStone,
        const TArray<FKalmalaToolState>& Input)
    {
        TArray<FKalmalaToolState> Candidate;
        int32 Count = 0;
        TestFalse(Label, FKalmalaToolLifecycleContract::BuildServerRepairAll(
            bServer, bAtStone, Input, Candidate, Count));
        TestEqual(TEXT("Rejected repair count remains zero"), Count, 0);
        TestEqual(TEXT("Rejected candidate retains the original number of tools"), Candidate.Num(), Input.Num());
        for (int32 Index = 0; Index < Input.Num() && Index < Candidate.Num(); ++Index)
        {
            TestTrue(TEXT("Rejected candidate preserves every original tool state"),
                Candidate[Index].ToolId == Input[Index].ToolId
                && Candidate[Index].ToolLevel == Input[Index].ToolLevel
                && Candidate[Index].Durability == Input[Index].Durability);
        }
    };
    TestRejectedWithoutMutation(TEXT("Client-authored repair is rejected"), false, true, Before);
    TestRejectedWithoutMutation(TEXT("Missing validated Grinding Stone is rejected"), true, false, Before);

    TArray<FKalmalaToolState> Malformed = Before;
    Malformed.Add(Before[0]);
    TestRejectedWithoutMutation(TEXT("Oversized carried list is rejected atomically"), true, true, Malformed);
    Malformed = Before;
    Malformed[4].ToolId = TEXT("FieldHatchet");
    TestRejectedWithoutMutation(TEXT("Duplicate carried tool is rejected atomically"), true, true, Malformed);
    Malformed = Before;
    Malformed[3].ToolId = TEXT("ForgedTool");
    TestRejectedWithoutMutation(TEXT("Unknown carried tool is rejected atomically"), true, true, Malformed);
    Malformed = Before;
    Malformed[3].Durability = 25;
    TestRejectedWithoutMutation(TEXT("Out-of-range condition is rejected atomically"), true, true, Malformed);
    return true;
}

#endif
