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
    const FKalmalaToolProgressionEntry* UpgradeEntry =
        FKalmalaToolProgressionContract::FindEntry(TEXT("BronzeAxe"), 2);
    TestNotNull(TEXT("Bronze Axe level-two progression entry exists"), UpgradeEntry);
    if (UpgradeEntry)
    {
        TestEqual(TEXT("Bronze Axe upgrades do not require a skill"), UpgradeEntry->RequiredSkill, EKalmalaSkill::None);
        TestEqual(TEXT("Upgrade skill level always remains zero"), UpgradeEntry->RequiredSkillLevel, 0);
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

    const TArray<FKalmalaInventoryStack> UpgradeMaterials = {
        {TEXT("Lightwood"), 3}, {TEXT("PeatAmber"), 2}, {TEXT("Stone"), 4}, {TEXT("Fibre"), 2}
    };
    TArray<FKalmalaToolState> UpgradedTools;
    TArray<FKalmalaInventoryStack> UpgradedInventory;
    for (const FKalmalaSkillProgressionLedger* Skills : {&CraftingLevelOne, &CraftingLevelFour,
        static_cast<FKalmalaSkillProgressionLedger*>(nullptr)})
    {
        TestTrue(TEXT("Upgrade succeeds independently of skill level or a skill ledger"),
            FKalmalaToolProgressionContract::BuildServerUpgrade(
                true, Skills, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), BronzeTools, UpgradeMaterials,
                UpgradedTools, UpgradedInventory, Reason));
    }
    RejectWithoutMutation(true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), BronzeTools, {},
        TEXT("Bronze Axe upgrade rejects insufficient materials without a skill gate"));
    TestFalse(TEXT("Level-one Forge cannot satisfy the level-two Bronze Axe target"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, &CraftingLevelFive, TEXT("ForgeKit"), 1, TEXT("BronzeAxe"), BronzeTools, UpgradeMaterials,
            UpgradedTools, UpgradedInventory, Reason));
    TestFalse(TEXT("Level-two Bronze Axe cannot be crafted without its carried level-one prerequisite"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), StartingTools, UpgradeMaterials,
            UpgradedTools, UpgradedInventory, Reason));
    TArray<FKalmalaToolState> InvalidConditionTools = BronzeTools;
    if (FKalmalaToolState* CarriedBronze = InvalidConditionTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ToolId == TEXT("BronzeAxe"); }))
    {
        if (const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("BronzeAxe")))
            CarriedBronze->Durability = Definition->MaxDurability + 1;
    }
    RejectWithoutMutation(true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), InvalidConditionTools, UpgradeMaterials,
        TEXT("Same-item upgrade rejects an invalid carried condition without changing candidates"));

    TArray<FKalmalaToolState> WornBronzeTools = BronzeTools;
    const int32 WornCondition = 11;
    if (FKalmalaToolState* CarriedBronze = WornBronzeTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ToolId == TEXT("BronzeAxe"); }))
        CarriedBronze->Durability = WornCondition;
    TestTrue(TEXT("Level-two Forge upgrades Bronze Axe level one to level two"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), WornBronzeTools, UpgradeMaterials,
            UpgradedTools, UpgradedInventory, Reason));
    TestEqual(TEXT("Upgrade preserves the single carried item"), UpgradedTools.Num(), WornBronzeTools.Num());
    const FKalmalaToolState* UpgradedBronze = UpgradedTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ToolId == TEXT("BronzeAxe"); });
    TestNotNull(TEXT("Accepted Forge transaction keeps the Bronze Axe identity"), UpgradedBronze);
    if (UpgradedBronze)
    {
        TestEqual(TEXT("Bronze Axe reaches its next authored level"), UpgradedBronze->ToolLevel, 2);
        TestEqual(TEXT("Same-item upgrade preserves tool condition"), UpgradedBronze->Durability, WornCondition);
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("BronzeAxe"));
        TestTrue(TEXT("Level-two Bronze Axe keeps its item-bound Bronze harvesting tier"), Definition
            && FKalmalaToolLifecycleContract::GetToolTier(Definition->Kind, UpgradedBronze->ToolLevel) == EKalmalaToolTier::Bronze);
    }
    TestFalse(TEXT("Upgrade does not create or exchange to an Iron Axe"),
        UpgradedTools.ContainsByPredicate([](const FKalmalaToolState& State) { return State.ToolId == TEXT("IronAxe"); }));
    TestEqual(TEXT("Bronze Axe upgrade consumes all approved material inputs"), UpgradedInventory.Num(), 0);

    TArray<FKalmalaToolState> DuplicateTools;
    TArray<FKalmalaInventoryStack> DuplicateInventory;
    TestFalse(TEXT("Bronze Axe cannot be crafted twice"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("WorkbenchKit"), 1, TEXT("BronzeAxe"), BronzeTools, BronzeMaterials,
            DuplicateTools, DuplicateInventory, Reason));

    return true;
}

#endif
