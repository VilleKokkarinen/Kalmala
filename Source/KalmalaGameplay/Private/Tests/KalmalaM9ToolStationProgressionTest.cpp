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
            ProgressIntent->FindPropertyByName(TEXT("ItemId")));
    }
    const UFunction* StoneUpgradeIntent =
        UKalmalaCraftingComponent::StaticClass()->FindFunctionByName(TEXT("ServerUpgradeStoneStarterTool"));
    if (TestNotNull(TEXT("Stone starter upgrade intent exists"), StoneUpgradeIntent))
    {
        TestTrue(TEXT("Stone starter upgrade is an owning-client server RPC"),
            StoneUpgradeIntent->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
        TestEqual(TEXT("Stone starter upgrade accepts only one tool identity"), int32(StoneUpgradeIntent->NumParms), 1);
        TestNotNull(TEXT("Stone starter upgrade cannot provide costs, station, or target level"),
            StoneUpgradeIntent->FindPropertyByName(TEXT("ItemId")));
        TestNull(TEXT("Stone starter upgrade cannot submit a client-selected target level"),
            StoneUpgradeIntent->FindPropertyByName(TEXT("TargetToolLevel")));
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
        {TEXT("Lightwood"), 4}, {TEXT("Stone"), 3}
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
    struct FStoneUpgradeCase
    {
        FName ItemId;
        int32 WoodCost;
        int32 StoneCost;
        float SlashGain;
        float CrushGain;
        float PierceGain;
        float StabGain;
    };
    const TArray<FStoneUpgradeCase> StoneUpgradeCases = {
        {FName(TEXT("StoneAxe")), 3, 2, 5.0f, 0.0f, 0.0f, 0.0f},
        {FName(TEXT("StoneKnife")), 1, 1, 5.0f, 0.0f, 0.0f, 5.0f},
        {FName(TEXT("StoneMace")), 2, 2, 0.0f, 5.0f, 5.0f, 0.0f},
        {FName(TEXT("StonePick")), 3, 3, 0.0f, 5.0f, 5.0f, 0.0f},
        {FName(TEXT("StoneSpear")), 3, 2, 0.0f, 0.0f, 0.0f, 5.0f}
    };
    const auto CostQuantity = [](const TArray<FKalmalaToolMaterialCost>& Costs, const FName ItemId)
    {
        const FKalmalaToolMaterialCost* Cost = Costs.FindByPredicate(
            [ItemId](const FKalmalaToolMaterialCost& Candidate) { return Candidate.ItemId == ItemId; });
        return Cost ? Cost->Quantity : 0;
    };
    for (int32 CaseIndex = 0; CaseIndex < StoneUpgradeCases.Num(); ++CaseIndex)
    {
        const FStoneUpgradeCase& Case = StoneUpgradeCases[CaseIndex];
        const FString Label = Case.ItemId.ToString();
        TestTrue(*FString::Printf(TEXT("%s is an explicitly supported Stone starter upgrade"), *Label),
            FKalmalaToolProgressionContract::IsStoneStarterUpgradeItem(Case.ItemId));

        const FKalmalaToolProgressionEntry* AcquisitionEntry =
            FKalmalaToolProgressionContract::FindEntry(Case.ItemId, 1);
        TestNotNull(*FString::Printf(TEXT("%s level-one acquisition remains authored"), *Label), AcquisitionEntry);
        TArray<FKalmalaToolState> UncarriedTools;
        for (const FKalmalaToolState& Existing : StartingTools)
            if (Existing.ItemId != Case.ItemId) UncarriedTools.Add(Existing);
        TestFalse(*FString::Printf(TEXT("%s cannot request its upgrade without a carried predecessor"), *Label),
            FKalmalaToolProgressionContract::HasExactlyOneCarriedToolAtLevel(Case.ItemId, 1, UncarriedTools));
        if (AcquisitionEntry)
        {
            TArray<FKalmalaInventoryStack> AcquisitionMaterials;
            for (const FKalmalaToolMaterialCost& Cost : AcquisitionEntry->MaterialCosts)
            {
                FKalmalaInventoryStack Stack;
                Stack.ItemId = Cost.ItemId;
                Stack.Quantity = Cost.Quantity;
                AcquisitionMaterials.Add(Stack);
            }
            TArray<FKalmalaToolState> AcquiredTools;
            TArray<FKalmalaInventoryStack> AcquiredInventory;
            TestTrue(*FString::Printf(TEXT("%s remains craftable at its matching Workbench when uncarried"), *Label),
                FKalmalaToolProgressionContract::BuildServerUpgrade(true, nullptr, TEXT("WorkbenchKit"),
                    AcquisitionEntry->RequiredStationLevel, Case.ItemId, UncarriedTools, AcquisitionMaterials,
                    AcquiredTools, AcquiredInventory, Reason));
            const FKalmalaToolState* Acquired = AcquiredTools.FindByPredicate(
                [&Case](const FKalmalaToolState& State) { return State.ItemId == Case.ItemId; });
            TestNotNull(*FString::Printf(TEXT("%s acquisition adds the level-one tool"), *Label), Acquired);
            if (Acquired)
            {
                TestEqual(*FString::Printf(TEXT("%s acquisition remains level one"), *Label), Acquired->ToolLevel, 1);
                TestTrue(*FString::Printf(TEXT("%s acquisition creates a valid tool identity"), *Label),
                    Acquired->InstanceGuid.IsValid());
                TestEqual(*FString::Printf(TEXT("%s acquisition starts at full condition"), *Label),
                    Acquired->Durability, FKalmalaToolLifecycleContract::GetMaxDurability(Case.ItemId, 1));
            }
        }

        const FKalmalaToolProgressionEntry* UpgradeEntry =
            FKalmalaToolProgressionContract::FindEntry(Case.ItemId, 2);
        TestNotNull(*FString::Printf(TEXT("%s level-two upgrade entry exists"), *Label), UpgradeEntry);
        if (UpgradeEntry)
        {
            TestEqual(*FString::Printf(TEXT("%s upgrade uses Workbench"), *Label),
                UpgradeEntry->RequiredStation, EKalmalaToolStationKind::Workbench);
            TestEqual(*FString::Printf(TEXT("%s upgrade requires a level-two Workbench"), *Label),
                UpgradeEntry->RequiredStationLevel, 2);
            TestEqual(*FString::Printf(TEXT("%s upgrade uses the exact authored Wood cost"), *Label),
                CostQuantity(UpgradeEntry->MaterialCosts, TEXT("Wood")), Case.WoodCost);
            TestEqual(*FString::Printf(TEXT("%s upgrade uses the exact authored Stone cost"), *Label),
                CostQuantity(UpgradeEntry->MaterialCosts, TEXT("Stone")), Case.StoneCost);
            TestEqual(*FString::Printf(TEXT("%s upgrade has only its authored material costs"), *Label),
                UpgradeEntry->MaterialCosts.Num(), 2);
        }

        FKalmalaToolState CarriedTool;
        CarriedTool.ItemId = Case.ItemId;
        CarriedTool.ToolLevel = 1;
        CarriedTool.InstanceGuid = FGuid(100, CaseIndex + 1, 300, 400);
        const int32 WornCondition = FKalmalaToolLifecycleContract::GetMaxDurability(Case.ItemId, 1) - 7;
        CarriedTool.Durability = WornCondition;
        const TArray<FKalmalaToolState> CarriedTools = {CarriedTool};
        TestTrue(*FString::Printf(TEXT("%s has exactly one carried level-one upgrade predecessor"), *Label),
            FKalmalaToolProgressionContract::HasExactlyOneCarriedToolAtLevel(Case.ItemId, 1, CarriedTools));
        TArray<FKalmalaToolState> WrongLevelTools = {CarriedTool};
        WrongLevelTools[0].ToolLevel = 2;
        TestFalse(*FString::Printf(TEXT("%s rejects a wrong-level upgrade predecessor"), *Label),
            FKalmalaToolProgressionContract::HasExactlyOneCarriedToolAtLevel(Case.ItemId, 1, WrongLevelTools));
        TArray<FKalmalaToolState> DuplicateTools = {CarriedTool, CarriedTool};
        TestFalse(*FString::Printf(TEXT("%s rejects an ambiguous duplicate predecessor"), *Label),
            FKalmalaToolProgressionContract::HasExactlyOneCarriedToolAtLevel(Case.ItemId, 1, DuplicateTools));

        const TArray<FKalmalaInventoryStack> UpgradeMaterials = {
            {TEXT("Wood"), Case.WoodCost}, {TEXT("Stone"), Case.StoneCost}
        };
        TArray<FKalmalaToolState> UpgradedTools;
        TArray<FKalmalaInventoryStack> UpgradedInventory;
        TestTrue(*FString::Printf(TEXT("%s upgrades at the matching level-two Workbench"), *Label),
            FKalmalaToolProgressionContract::BuildServerUpgrade(true, nullptr, TEXT("WorkbenchKit"), 2,
                Case.ItemId, CarriedTools, UpgradeMaterials, UpgradedTools, UpgradedInventory, Reason));
        TestEqual(*FString::Printf(TEXT("%s upgrade retains the carried tool count"), *Label),
            UpgradedTools.Num(), 1);
        const FKalmalaToolState* Upgraded = UpgradedTools.FindByPredicate(
            [&Case](const FKalmalaToolState& State) { return State.ItemId == Case.ItemId; });
        TestNotNull(*FString::Printf(TEXT("%s upgrade retains the tool identity"), *Label), Upgraded);
        if (Upgraded)
        {
            TestEqual(*FString::Printf(TEXT("%s reaches level two"), *Label), Upgraded->ToolLevel, 2);
            TestTrue(*FString::Printf(TEXT("%s upgrade retains the exact instance GUID"), *Label),
                Upgraded->InstanceGuid == CarriedTool.InstanceGuid);
            TestEqual(*FString::Printf(TEXT("%s upgrade preserves current condition"), *Label),
                Upgraded->Durability, WornCondition);
        }
        TestEqual(*FString::Printf(TEXT("%s level two adds its catalogued condition"), *Label),
            FKalmalaToolLifecycleContract::GetMaxDurability(Case.ItemId, 2),
            FKalmalaToolLifecycleContract::GetMaxDurability(Case.ItemId, 1) + 50);
        const FKalmalaDamageChannels LevelOneDamage =
            FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(Case.ItemId, 1);
        const FKalmalaDamageChannels LevelTwoDamage =
            FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(Case.ItemId, 2);
        TestTrue(*FString::Printf(TEXT("%s level-two Slash gain matches its catalogue"), *Label),
            FMath::IsNearlyEqual(LevelTwoDamage.Slash - LevelOneDamage.Slash, Case.SlashGain));
        TestTrue(*FString::Printf(TEXT("%s level-two Crush gain matches its catalogue"), *Label),
            FMath::IsNearlyEqual(LevelTwoDamage.Crush - LevelOneDamage.Crush, Case.CrushGain));
        TestTrue(*FString::Printf(TEXT("%s level-two Pierce gain matches its catalogue"), *Label),
            FMath::IsNearlyEqual(LevelTwoDamage.Pierce - LevelOneDamage.Pierce, Case.PierceGain));
        TestTrue(*FString::Printf(TEXT("%s level-two Stab gain matches its catalogue"), *Label),
            FMath::IsNearlyEqual(LevelTwoDamage.Stab - LevelOneDamage.Stab, Case.StabGain));
        TestEqual(*FString::Printf(TEXT("%s upgrade consumes its exact Wood cost"), *Label),
            Quantity(UpgradedInventory, TEXT("Wood")), 0);
        TestEqual(*FString::Printf(TEXT("%s upgrade consumes its exact Stone cost"), *Label),
            Quantity(UpgradedInventory, TEXT("Stone")), 0);

        const auto ExpectStoneUpgradeRejectedWithoutMutation = [&](const TArray<FKalmalaToolState>& InputTools,
            const TArray<FKalmalaInventoryStack>& InputInventory, const FName StationKit,
            const int32 StationLevel, const TCHAR* RejectionCase)
        {
            TArray<FKalmalaToolState> CandidateTools = InputTools;
            TArray<FKalmalaInventoryStack> CandidateInventory = InputInventory;
            TestFalse(*FString::Printf(TEXT("%s rejects %s"), *Label, RejectionCase),
                FKalmalaToolProgressionContract::BuildServerUpgrade(true, nullptr, StationKit, StationLevel,
                    Case.ItemId, InputTools, InputInventory, CandidateTools, CandidateInventory, Reason));
            TestEqual(*FString::Printf(TEXT("%s rejection leaves tool count unchanged"), *Label),
                CandidateTools.Num(), InputTools.Num());
            TestEqual(*FString::Printf(TEXT("%s rejection leaves inventory count unchanged"), *Label),
                CandidateInventory.Num(), InputInventory.Num());
            for (int32 ToolIndex = 0; ToolIndex < InputTools.Num() && CandidateTools.IsValidIndex(ToolIndex); ++ToolIndex)
            {
                TestTrue(*FString::Printf(TEXT("%s rejection leaves carried tool %d unchanged"), *Label, ToolIndex),
                    CandidateTools[ToolIndex].ItemId == InputTools[ToolIndex].ItemId
                    && CandidateTools[ToolIndex].ToolLevel == InputTools[ToolIndex].ToolLevel
                    && CandidateTools[ToolIndex].InstanceGuid == InputTools[ToolIndex].InstanceGuid
                    && CandidateTools[ToolIndex].Durability == InputTools[ToolIndex].Durability);
            }
            for (int32 StackIndex = 0; StackIndex < InputInventory.Num()
                && CandidateInventory.IsValidIndex(StackIndex); ++StackIndex)
            {
                TestTrue(*FString::Printf(TEXT("%s rejection leaves material stack %d unchanged"), *Label, StackIndex),
                    CandidateInventory[StackIndex].ItemId == InputInventory[StackIndex].ItemId
                    && CandidateInventory[StackIndex].Quantity == InputInventory[StackIndex].Quantity);
            }
        };
        TArray<FKalmalaInventoryStack> MissingUpgradeMaterials = UpgradeMaterials;
        MissingUpgradeMaterials[1].Quantity = Case.StoneCost - 1;
        ExpectStoneUpgradeRejectedWithoutMutation(CarriedTools, MissingUpgradeMaterials,
            TEXT("WorkbenchKit"), 2, TEXT("missing materials"));
        ExpectStoneUpgradeRejectedWithoutMutation(CarriedTools, UpgradeMaterials,
            TEXT("ForgeKit"), 2, TEXT("wrong station"));
        ExpectStoneUpgradeRejectedWithoutMutation(CarriedTools, UpgradeMaterials,
            TEXT("WorkbenchKit"), 1, TEXT("wrong Workbench level"));
        ExpectStoneUpgradeRejectedWithoutMutation(UncarriedTools, UpgradeMaterials,
            TEXT("WorkbenchKit"), 2, TEXT("missing carried predecessor"));
        ExpectStoneUpgradeRejectedWithoutMutation(WrongLevelTools, UpgradeMaterials,
            TEXT("WorkbenchKit"), 2, TEXT("wrong-level carried predecessor"));
    }

    TestTrue(TEXT("Bronze Axe is crafted at the matching level-one Forge"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("ForgeKit"), 1, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
            BronzeTools, BronzeInventory, Reason));

    const FKalmalaToolState* Bronze = BronzeTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("BronzeAxe"); });
    TestNotNull(TEXT("Accepted Forge transaction adds Bronze Axe"), Bronze);
    if (Bronze)
    {
        TestEqual(TEXT("Bronze Axe starts at its authored level"), Bronze->ToolLevel, 1);
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("BronzeAxe"));
        TestTrue(TEXT("Crafted Bronze Axe begins in full condition"),
            Definition && Bronze->Durability == Definition->MaxDurability);
    }
    TestEqual(TEXT("Bronze cost consumes four Lightwood"), Quantity(BronzeInventory, TEXT("Lightwood")), 0);
    TestEqual(TEXT("Bronze cost consumes three Stone"), Quantity(BronzeInventory, TEXT("Stone")), 0);

    const TArray<FKalmalaInventoryStack> ThunderMaulMaterials = {
        {TEXT("ThunderCore"), 1}, {TEXT("Iron"), 20}, {TEXT("Densewood"), 10}
    };
    TArray<FKalmalaToolState> ThunderMaulTools;
    TArray<FKalmalaInventoryStack> ThunderMaulInventory;
    TestTrue(TEXT("Thunder Maul is crafted at the matching level-one Forge"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("ForgeKit"), 1, TEXT("ThunderMaul"), StartingTools, ThunderMaulMaterials,
            ThunderMaulTools, ThunderMaulInventory, Reason));
    FKalmalaToolState* CraftedThunderMaul = ThunderMaulTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("ThunderMaul"); });
    TestNotNull(TEXT("Accepted Forge transaction adds Thunder Maul"), CraftedThunderMaul);
    FGuid ThunderMaulGuid;
    const int32 ThunderMaulWornCondition = 87;
    if (CraftedThunderMaul)
    {
        ThunderMaulGuid = CraftedThunderMaul->InstanceGuid;
        TestTrue(TEXT("Crafted Thunder Maul has its own item identity"), ThunderMaulGuid.IsValid());
        TestEqual(TEXT("Crafted Thunder Maul starts at level one"), CraftedThunderMaul->ToolLevel, 1);
        TestEqual(TEXT("Crafted Thunder Maul starts in full condition"), CraftedThunderMaul->Durability, 220);
        CraftedThunderMaul->Durability = ThunderMaulWornCondition;
    }
    TestEqual(TEXT("Thunder Maul acquisition consumes its Thunder Core"),
        Quantity(ThunderMaulInventory, TEXT("ThunderCore")), 0);
    TestEqual(TEXT("Thunder Maul acquisition consumes twenty Iron"),
        Quantity(ThunderMaulInventory, TEXT("Iron")), 0);
    TestEqual(TEXT("Thunder Maul acquisition consumes ten Densewood"),
        Quantity(ThunderMaulInventory, TEXT("Densewood")), 0);

    const TArray<FKalmalaInventoryStack> ThunderMaulUpgradeMaterials = {
        {TEXT("ThunderCore"), 1}, {TEXT("Iron"), 10}, {TEXT("Densewood"), 5}
    };
    TArray<FKalmalaToolState> ThunderMaulUpgradedTools;
    TArray<FKalmalaInventoryStack> ThunderMaulUpgradedInventory;
    TestTrue(TEXT("Thunder Maul upgrades at the matching level-two Forge"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("ForgeKit"), 2, TEXT("ThunderMaul"), ThunderMaulTools,
            ThunderMaulUpgradeMaterials, ThunderMaulUpgradedTools, ThunderMaulUpgradedInventory, Reason));
    const FKalmalaToolState* UpgradedThunderMaul = ThunderMaulUpgradedTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("ThunderMaul"); });
    TestNotNull(TEXT("Accepted Forge upgrade keeps Thunder Maul carried"), UpgradedThunderMaul);
    if (UpgradedThunderMaul)
    {
        TestTrue(TEXT("Thunder Maul upgrade keeps the same item identity"),
            UpgradedThunderMaul->InstanceGuid == ThunderMaulGuid);
        TestEqual(TEXT("Thunder Maul reaches level two"), UpgradedThunderMaul->ToolLevel, 2);
        TestEqual(TEXT("Thunder Maul upgrade preserves current condition"),
            UpgradedThunderMaul->Durability, ThunderMaulWornCondition);
    }
    TestEqual(TEXT("Thunder Maul upgrade consumes its Thunder Core"),
        Quantity(ThunderMaulUpgradedInventory, TEXT("ThunderCore")), 0);
    TestEqual(TEXT("Thunder Maul upgrade consumes ten Iron"),
        Quantity(ThunderMaulUpgradedInventory, TEXT("Iron")), 0);
    TestEqual(TEXT("Thunder Maul upgrade consumes five Densewood"),
        Quantity(ThunderMaulUpgradedInventory, TEXT("Densewood")), 0);

    const TArray<FKalmalaInventoryStack> ReefHarpoonMaterials = {
        {TEXT("ReefTooth"), 3}, {TEXT("Iron"), 8}, {TEXT("Lightwood"), 4}
    };
    TArray<FKalmalaToolState> ReefHarpoonTools;
    TArray<FKalmalaInventoryStack> ReefHarpoonInventory;
    TestTrue(TEXT("Reef Harpoon is crafted at the matching level-one Forge"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("ForgeKit"), 1, TEXT("ReefHarpoon"), StartingTools, ReefHarpoonMaterials,
            ReefHarpoonTools, ReefHarpoonInventory, Reason));
    FKalmalaToolState* CraftedReefHarpoon = ReefHarpoonTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("ReefHarpoon"); });
    TestNotNull(TEXT("Accepted Forge transaction adds Reef Harpoon"), CraftedReefHarpoon);
    FGuid ReefHarpoonGuid;
    const int32 ReefHarpoonWornCondition = 63;
    if (CraftedReefHarpoon)
    {
        ReefHarpoonGuid = CraftedReefHarpoon->InstanceGuid;
        TestTrue(TEXT("Crafted Reef Harpoon has its own item identity"), ReefHarpoonGuid.IsValid());
        TestEqual(TEXT("Crafted Reef Harpoon starts at level one"), CraftedReefHarpoon->ToolLevel, 1);
        TestEqual(TEXT("Crafted Reef Harpoon starts in full condition"), CraftedReefHarpoon->Durability, 150);
        CraftedReefHarpoon->Durability = ReefHarpoonWornCondition;
    }
    TestEqual(TEXT("Reef Harpoon acquisition consumes three Reef Teeth"),
        Quantity(ReefHarpoonInventory, TEXT("ReefTooth")), 0);
    TestEqual(TEXT("Reef Harpoon acquisition consumes eight Iron"),
        Quantity(ReefHarpoonInventory, TEXT("Iron")), 0);
    TestEqual(TEXT("Reef Harpoon acquisition consumes four Lightwood"),
        Quantity(ReefHarpoonInventory, TEXT("Lightwood")), 0);

    const TArray<FKalmalaInventoryStack> ReefHarpoonUpgradeMaterials = {
        {TEXT("ReefTooth"), 1}, {TEXT("Iron"), 4}, {TEXT("Lightwood"), 2}
    };
    TArray<FKalmalaToolState> ReefHarpoonUpgradedTools;
    TArray<FKalmalaInventoryStack> ReefHarpoonUpgradedInventory;
    TestTrue(TEXT("Reef Harpoon upgrades at the matching level-two Forge"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("ForgeKit"), 2, TEXT("ReefHarpoon"), ReefHarpoonTools,
            ReefHarpoonUpgradeMaterials, ReefHarpoonUpgradedTools, ReefHarpoonUpgradedInventory, Reason));
    const FKalmalaToolState* UpgradedReefHarpoon = ReefHarpoonUpgradedTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("ReefHarpoon"); });
    TestNotNull(TEXT("Accepted Forge upgrade keeps Reef Harpoon carried"), UpgradedReefHarpoon);
    if (UpgradedReefHarpoon)
    {
        TestTrue(TEXT("Reef Harpoon upgrade keeps the same item identity"),
            UpgradedReefHarpoon->InstanceGuid == ReefHarpoonGuid);
        TestEqual(TEXT("Reef Harpoon reaches level two"), UpgradedReefHarpoon->ToolLevel, 2);
        TestEqual(TEXT("Reef Harpoon upgrade preserves current condition"),
            UpgradedReefHarpoon->Durability, ReefHarpoonWornCondition);
    }
    TestEqual(TEXT("Reef Harpoon upgrade consumes its Reef Tooth"),
        Quantity(ReefHarpoonUpgradedInventory, TEXT("ReefTooth")), 0);
    TestEqual(TEXT("Reef Harpoon upgrade consumes four Iron"),
        Quantity(ReefHarpoonUpgradedInventory, TEXT("Iron")), 0);
    TestEqual(TEXT("Reef Harpoon upgrade consumes two Lightwood"),
        Quantity(ReefHarpoonUpgradedInventory, TEXT("Lightwood")), 0);

    TArray<FKalmalaToolState> RejectedTools = StartingTools;
    TArray<FKalmalaInventoryStack> RejectedInventory = BronzeMaterials;
    const auto RejectWithoutMutation = [&](const bool bServerAuthority,
        const FKalmalaSkillProgressionLedger* SkillLedger, const FName StationKit,
        const int32 StationLevel, const FName ItemId, const TArray<FKalmalaToolState>& Tools,
        const TArray<FKalmalaInventoryStack>& Inventory, const TCHAR* Message)
    {
        const TArray<FKalmalaToolState> BeforeTools = RejectedTools;
        const TArray<FKalmalaInventoryStack> BeforeInventory = RejectedInventory;
        TestFalse(Message, FKalmalaToolProgressionContract::BuildServerUpgrade(
            bServerAuthority, SkillLedger, StationKit, StationLevel, ItemId, Tools, Inventory,
            RejectedTools, RejectedInventory, Reason));
        TestEqual(TEXT("Rejected station/tool request keeps candidate tool output untouched"), RejectedTools.Num(), BeforeTools.Num());
        TestEqual(TEXT("Rejected station/tool request keeps candidate pack output untouched"), RejectedInventory.Num(), BeforeInventory.Num());
    };
    RejectWithoutMutation(false, nullptr, TEXT("WorkbenchKit"), 1, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("A non-authoritative progression call is rejected"));
    RejectWithoutMutation(true, nullptr, NAME_None, 0, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("Missing station is rejected"));
    RejectWithoutMutation(true, nullptr, TEXT("WorkbenchKit"), 1, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("Wrong station family is rejected"));
    RejectWithoutMutation(true, nullptr, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), StartingTools, BronzeMaterials,
        TEXT("Forge level must equal the target tool level"));
    TArray<FKalmalaToolState> MissingMaterialTools;
    TArray<FKalmalaInventoryStack> MissingMaterialInventory;
    TestFalse(TEXT("Bronze Axe rejects insufficient materials"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("ForgeKit"), 1, TEXT("BronzeAxe"), StartingTools, {},
            MissingMaterialTools, MissingMaterialInventory, Reason));
    TestTrue(TEXT("Missing materials leave candidate tool state unchanged"), MissingMaterialTools.IsEmpty());
    TestTrue(TEXT("Missing materials leave candidate inventory unchanged"), MissingMaterialInventory.IsEmpty());

    const TArray<FKalmalaInventoryStack> UpgradeMaterials = {
        {TEXT("Lightwood"), 4}, {TEXT("Bronze"), 3}
    };
    TArray<FKalmalaToolState> UpgradedTools;
    TArray<FKalmalaInventoryStack> UpgradedInventory;
    FKalmalaSkillProgressionLedger EmptySkills;
    for (const FKalmalaSkillProgressionLedger* Skills : {&CraftingLevelOne, &CraftingLevelFour, &EmptySkills,
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
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("BronzeAxe"); }))
    {
        if (const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("BronzeAxe")))
            CarriedBronze->Durability = Definition->MaxDurability + 1;
    }
    RejectWithoutMutation(true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), InvalidConditionTools, UpgradeMaterials,
        TEXT("Same-item upgrade rejects an invalid carried condition without changing candidates"));

    TArray<FKalmalaToolState> WornBronzeTools = BronzeTools;
    const int32 WornCondition = 11;
    if (FKalmalaToolState* CarriedBronze = WornBronzeTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("BronzeAxe"); }))
        CarriedBronze->Durability = WornCondition;
    TestTrue(TEXT("Level-two Forge upgrades Bronze Axe level one to level two"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, &CraftingLevelFive, TEXT("ForgeKit"), 2, TEXT("BronzeAxe"), WornBronzeTools, UpgradeMaterials,
            UpgradedTools, UpgradedInventory, Reason));
    TestEqual(TEXT("Upgrade preserves the single carried item"), UpgradedTools.Num(), WornBronzeTools.Num());
    const FKalmalaToolState* UpgradedBronze = UpgradedTools.FindByPredicate(
        [](const FKalmalaToolState& State) { return State.ItemId == TEXT("BronzeAxe"); });
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
        UpgradedTools.ContainsByPredicate([](const FKalmalaToolState& State) { return State.ItemId == TEXT("IronAxe"); }));
    TestEqual(TEXT("Bronze Axe upgrade consumes all approved material inputs"), UpgradedInventory.Num(), 0);

    TArray<FKalmalaToolState> DuplicateTools;
    TArray<FKalmalaInventoryStack> DuplicateInventory;
    TestFalse(TEXT("Bronze Axe cannot be crafted twice"),
        FKalmalaToolProgressionContract::BuildServerUpgrade(
            true, nullptr, TEXT("ForgeKit"), 1, TEXT("BronzeAxe"), BronzeTools, BronzeMaterials,
            DuplicateTools, DuplicateInventory, Reason));

    return true;
}

#endif
