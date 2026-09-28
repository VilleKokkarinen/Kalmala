#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaConstructionSaveGame.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaM9ToolStationProgressionTest,
    "Kalmala.Gameplay.M9.ToolStationProgression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaM9ToolStationProgressionTest::RunTest(const FString& Parameters)
{
    const UFunction* ProgressIntent = UKalmalaCraftingComponent::StaticClass()->FindFunctionByName(TEXT("ServerProgressTool"));
    if (TestNotNull(TEXT("Tool progression intent exists"), ProgressIntent))
    {
        TestTrue(TEXT("Tool progression is an owning-client server RPC"),
            ProgressIntent->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Tool progression accepts only one tool identity"), int32(ProgressIntent->NumParms), 1);
        TestNotNull(TEXT("Tool progression cannot provide a station, level, costs, or result"),
            ProgressIntent->FindPropertyByName(TEXT("ToolId")));
    }

    UKalmalaConstructionSaveGame* ConstructionSave = NewObject<UKalmalaConstructionSaveGame>();
    FKalmalaConstructionSaveRecord ForgeRecord;
    ForgeRecord.ConstructionId = TEXT("m9-forge");
    ForgeRecord.KitId = TEXT("ForgeKit");
    TestTrue(TEXT("Forge fits the existing schema-one construction save record"),
        ConstructionSave->AddRecord(ForgeRecord));

    const TArray<FKalmalaToolState> StartingTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
    const FKalmalaToolProgressionEntry* IronEntry = FKalmalaToolProgressionContract::FindEntry(TEXT("IronAxe"));
    TestNotNull(TEXT("Iron Axe progression entry exists"), IronEntry);
    if (IronEntry)
    {
        TestEqual(TEXT("Iron Axe tier two requires Crafting"), IronEntry->RequiredSkill, EKalmalaSkill::Crafting);
        TestEqual(TEXT("Iron Axe requires the level-five second-tier unlock"), IronEntry->RequiredSkillLevel, 5);
    }
    FKalmalaSkillProgressionLedger CraftingLevelOne;
    CraftingLevelOne.Initialize();
    FKalmalaSkillProgressionLedger CraftingLevelFour = CraftingLevelOne;
    bool bReachedLevelFour = true;
    for (int32 Award = 0; Award < 12; ++Award)
        bReachedLevelFour &= CraftingLevelFour.AwardExperienceFromServer(EKalmalaSkill::Crafting, true, true, 25);
    TestTrue(TEXT("Server ledger can reach Crafting level four"), bReachedLevelFour);
    FKalmalaSkillProgressionLedger CraftingLevelFive = CraftingLevelOne;
    bool bReachedLevelFive = true;
    for (int32 Award = 0; Award < 16; ++Award)
        bReachedLevelFive &= CraftingLevelFive.AwardExperienceFromServer(EKalmalaSkill::Crafting, true, true, 25);
    TestTrue(TEXT("Server ledger reaches Crafting level five through accepted awards"), bReachedLevelFive);
    const FKalmalaSkillState* LevelFiveCrafting = CraftingLevelFive.Find(EKalmalaSkill::Crafting);
    TestNotNull(TEXT("Level-five Crafting state exists"), LevelFiveCrafting);
    if (LevelFiveCrafting)
    {
        TestEqual(TEXT("Four hundred Crafting experience gives level five"), LevelFiveCrafting->Level, 5);
        TestTrue(TEXT("Level five derives the existing second-tier unlock"),
            (LevelFiveCrafting->UnlockMask & static_cast<uint8>(EKalmalaSkillUnlock::SecondTier)) != 0);
    }
    const TArray<FKalmalaInventoryStack> BronzeMaterials = {
        {TEXT("Wood"), 4}, {TEXT("Stone"), 3}, {TEXT("Fibre"), 2}
    };
    TArray<FKalmalaToolState> BronzeTools;
    TArray<FKalmalaInventoryStack> BronzeInventory;
    FString Reason;
    const auto Quantity = [](const TArray<FKalmalaInventoryStack>& Inventory, const FName ItemId)
    {
        const FKalmalaInventoryStack* Stack = Inventory.FindByPredicate(
            [ItemId](const FKalmalaInventoryStack& Candidate) { return Candidate.ItemId == ItemId; });
        return Stack ? Stack->Quantity : 0;
    };
    TestTrue(TEXT("Bronze Axe is crafted at the matching level-one Workbench"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("WorkbenchKit"), 1, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
            BronzeTools, BronzeInventory, Reason));

    const FKalmalaToolState* Bronze = BronzeTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ToolId == TEXT("BronzeAxe"); });
    TestNotNull(TEXT("Accepted Workbench transaction adds Bronze Axe"), Bronze);
    if (Bronze)
    {
        TestEqual(TEXT("Bronze Axe starts at its authored level"), Bronze->ToolLevel, 1);
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("BronzeAxe"));
        TestTrue(TEXT("Crafted Bronze Axe begins in full condition"),
            Definition && Bronze->Durability == Definition->MaxDurability);
    }
    TestEqual(TEXT("Bronze cost consumes four Wood"), Quantity(BronzeInventory, TEXT("Wood")), 0);
    TestEqual(TEXT("Bronze cost consumes three Stone"), Quantity(BronzeInventory, TEXT("Stone")), 0);
    TestEqual(TEXT("Bronze cost consumes two Fibre"), Quantity(BronzeInventory, TEXT("Fibre")), 0);

    TArray<FKalmalaToolState> RejectedTools = StartingTools;
    TArray<FKalmalaInventoryStack> RejectedInventory = BronzeMaterials;
    const auto RejectWithoutMutation = [&](const bool bServerAuthority,
        const FKalmalaSkillProgressionLedger* SkillLedger, const FName StationKit,
        const int32 StationLevel, const FName ToolId, const TArray<FKalmalaToolState>& Tools,
        const TArray<FKalmalaInventoryStack>& Inventory, const TCHAR* Message)
    {
        const TArray<FKalmalaToolState> BeforeTools = RejectedTools;
        const TArray<FKalmalaInventoryStack> BeforeInventory = RejectedInventory;
        TestFalse(Message, FKalmalaToolProgressionContract::BuildServerUpgrade(
            bServerAuthority, SkillLedger, StationKit, StationLevel, ToolId, Tools, Inventory,
            RejectedTools, RejectedInventory, Reason));
        TestEqual(TEXT("Rejected station/tool request keeps candidate tool output untouched"), RejectedTools.Num(), BeforeTools.Num());
        TestEqual(TEXT("Rejected station/tool request keeps candidate pack output untouched"), RejectedInventory.Num(), BeforeInventory.Num());
    };
    RejectWithoutMutation(false, nullptr, TEXT("WorkbenchKit"), 1, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("A non-authoritative progression call is rejected"));
    RejectWithoutMutation(true, nullptr, NAME_None, 0, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("Missing station is rejected"));
    RejectWithoutMutation(true, nullptr, TEXT("ForgeKit"), 1, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("Wrong station family is rejected"));
    RejectWithoutMutation(true, nullptr, TEXT("WorkbenchKit"), 2, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("Workbench level must equal the target tool level"));
    TArray<FKalmalaToolState> MissingMaterialTools;
    TArray<FKalmalaInventoryStack> MissingMaterialInventory;
    TestFalse(TEXT("Bronze Axe rejects insufficient materials"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("WorkbenchKit"), 1, TEXT("BronzeAxe"), StartingTools, {},
            MissingMaterialTools, MissingMaterialInventory, Reason));
    TestTrue(TEXT("Missing materials leave candidate tool state unchanged"), MissingMaterialTools.IsEmpty());
    TestTrue(TEXT("Missing materials leave candidate inventory unchanged"), MissingMaterialInventory.IsEmpty());

    const TArray<FKalmalaInventoryStack> IronMaterials = {
        {TEXT("Lightwood"), 3}, {TEXT("PeatAmber"), 2}, {TEXT("Stone"), 4}, {TEXT("Fibre"), 2}
    };
    TArray<FKalmalaToolState> IronTools;
    TArray<FKalmalaInventoryStack> IronInventory;
    RejectWithoutMutation(true, &CraftingLevelOne, TEXT("ForgeKit"), 2, TEXT("IronAxe"), BronzeTools, IronMaterials,
        TEXT("Iron Axe rejects Crafting level one without changing candidate state"));
    RejectWithoutMutation(true, &CraftingLevelFour, TEXT("ForgeKit"), 2, TEXT("IronAxe"), BronzeTools, IronMaterials,
        TEXT("Iron Axe rejects Crafting level four without the second-tier unlock"));
    RejectWithoutMutation(true, nullptr, TEXT("ForgeKit"), 2, TEXT("IronAxe"), BronzeTools, IronMaterials,
        TEXT("Iron Axe rejects a missing server skill ledger"));
    RejectWithoutMutation(true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("IronAxe"), BronzeTools, {},
        TEXT("Iron Axe rejects insufficient materials after skill acceptance"));
    TestFalse(TEXT("Level-one Forge cannot satisfy the level-two Iron Axe target"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, &CraftingLevelFive, TEXT("ForgeKit"), 1, TEXT("IronAxe"), BronzeTools, IronMaterials,
            IronTools, IronInventory, Reason));
    TestFalse(TEXT("Iron Axe cannot be crafted without its carried Bronze Axe prerequisite"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("IronAxe"), StartingTools, IronMaterials,
            IronTools, IronInventory, Reason));
    TArray<FKalmalaToolState> InvalidConditionTools = BronzeTools;
    if (FKalmalaToolState* CarriedBronze = InvalidConditionTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ToolId == TEXT("BronzeAxe"); }))
    {
        if (const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("BronzeAxe")))
            CarriedBronze->Durability = Definition->MaxDurability + 1;
    }
    RejectWithoutMutation(true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("IronAxe"), InvalidConditionTools, IronMaterials,
        TEXT("Iron upgrade rejects an invalid carried condition without changing candidates"));

    TestTrue(TEXT("Level-two Forge upgrades the carried Bronze Axe to Iron Axe"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("IronAxe"), BronzeTools, IronMaterials,
            IronTools, IronInventory, Reason));
    TestFalse(TEXT("Upgrade exchanges away the previous Bronze Axe"),
        IronTools.ContainsByPredicate([](const FKalmalaToolState& State) { return State.ToolId == TEXT("BronzeAxe"); }));
    const FKalmalaToolState* Iron = IronTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ToolId == TEXT("IronAxe"); });
    TestNotNull(TEXT("Accepted Forge transaction adds Iron Axe"), Iron);
    if (Iron)
    {
        TestEqual(TEXT("Iron Axe has its authored level"), Iron->ToolLevel, 2);
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("IronAxe"));
        TestTrue(TEXT("Upgraded Iron Axe begins in full condition"),
            Definition && Iron->Durability == Definition->MaxDurability);
    }
    TestEqual(TEXT("Iron upgrade consumes all approved material inputs"), IronInventory.Num(), 0);

    TArray<FKalmalaToolState> DuplicateTools;
    TArray<FKalmalaInventoryStack> DuplicateInventory;
    TestFalse(TEXT("Bronze Axe cannot be crafted twice"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("WorkbenchKit"), 1, TEXT("BronzeAxe"), BronzeTools, BronzeMaterials,
            DuplicateTools, DuplicateInventory, Reason));

    return true;
}

#endif
