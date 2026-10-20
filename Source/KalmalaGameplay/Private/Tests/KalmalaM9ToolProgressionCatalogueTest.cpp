#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolCatalogue.h"
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
    TestEqual(TEXT("Station upgrades have a level-ten cap"), FKalmalaToolProgressionContract::MaxStationToolLevel, 10);
    TestEqual(TEXT("All item sources share the hard level-twenty cap"), FKalmalaToolLifecycleContract::MaxToolLevel, 20);
    TestEqual(TEXT("Authored stats support levels through the hard cap"), UKalmalaToolCatalogue::MaxLevels, 20);
    TestNull(TEXT("Station progression has no level-eleven target"),
        FKalmalaToolProgressionContract::FindEntry(TEXT("BronzeAxe"), 11));
    TestNull(TEXT("A level-ten item cannot be upgraded further by a station"),
        FKalmalaToolProgressionContract::FindNextEntry(TEXT("BronzeAxe"), 10));
    TestNull(TEXT("A non-station level-twenty item cannot enter station upgrades"),
        FKalmalaToolProgressionContract::FindNextEntry(TEXT("BronzeAxe"), 20));
    TestFalse(TEXT("Level twenty-one is rejected for any item"),
        FKalmalaToolProgressionContract::IsValidToolLevel(TEXT("BronzeAxe"), 21));
    TestFalse(TEXT("Zero remains an invalid carried-item level"),
        FKalmalaToolProgressionContract::IsValidToolLevel(TEXT("BronzeAxe"), 0));
    TestEqual(TEXT("Above-cap durability cannot authorize use, repair or saving"),
        FKalmalaToolLifecycleContract::GetMaxDurability(TEXT("BronzeAxe"), 21), 0);
    TestEqual(TEXT("Above-cap damage cannot authorize combat"),
        FKalmalaToolLifecycleContract::GetWeaponDamage(TEXT("BronzeAxe"), 21), 0.0f);
    TestTrue(TEXT("Legacy Iron Axe level two remains valid"),
        FKalmalaToolProgressionContract::IsValidToolLevel(TEXT("IronAxe"), 2));

    // Use a transient catalogue so boundary authoring does not mutate live defaults.
    const UKalmalaToolCatalogue* AuthoredCatalogue = UKalmalaToolCatalogue::Get();
    const UKalmalaItemCatalogue* ItemCatalogue = UKalmalaItemCatalogue::Get();
    const UKalmalaRecipeCatalogue* RecipeCatalogue = UKalmalaRecipeCatalogue::Get();
    const TArray<TPair<FName, FName>> GearStations = {
        TPair<FName, FName>(TEXT("BoarHideBoots"), TEXT("Workbench")),
        TPair<FName, FName>(TEXT("BoarHideCape"), TEXT("Workbench")),
        TPair<FName, FName>(TEXT("BoarHideHelmet"), TEXT("Workbench")),
        TPair<FName, FName>(TEXT("BoarHideLeggings"), TEXT("Workbench")),
        TPair<FName, FName>(TEXT("BoarHideTunic"), TEXT("Workbench")),
        TPair<FName, FName>(TEXT("WoodShield"), TEXT("Workbench"))
    };
    TestTrue(TEXT("Armor item, recipe and upgrade catalogues validate"),
        ItemCatalogue && ItemCatalogue->IsValidCatalogue()
        && RecipeCatalogue && RecipeCatalogue->IsValidCatalogue()
        && AuthoredCatalogue && AuthoredCatalogue->IsValidCatalogue(ItemCatalogue));
    const FKalmalaToolRecipeData* IronAxeAcquisition = AuthoredCatalogue
        ? AuthoredCatalogue->Recipes.FindByPredicate(
            [](const FKalmalaToolRecipeData& Recipe) { return Recipe.ItemId == TEXT("IronAxe"); }) : nullptr;
    TestNotNull(TEXT("Iron Axe has a level-one acquisition recipe"), IronAxeAcquisition);
    if (IronAxeAcquisition)
    {
        TestEqual(TEXT("Iron Axe is a level-one acquisition"), IronAxeAcquisition->Level, 1);
        TestEqual(TEXT("Iron Axe acquisition uses the Forge"), IronAxeAcquisition->RequiredStation, FString(TEXT("Forge")));
        TestTrue(TEXT("Iron Axe acquisition uses four Iron and three Lightwood"),
            IronAxeAcquisition->Ingredients.Num() == 2
            && IronAxeAcquisition->Ingredients.ContainsByPredicate([](const FKalmalaToolRecipeIngredientData& Cost)
                { return Cost.ItemId == TEXT("Iron") && Cost.Quantity == 4; })
            && IronAxeAcquisition->Ingredients.ContainsByPredicate([](const FKalmalaToolRecipeIngredientData& Cost)
                { return Cost.ItemId == TEXT("Lightwood") && Cost.Quantity == 3; }));
    }
    const FKalmalaToolRecipeData* IronAxeUpgrade = AuthoredCatalogue
        ? AuthoredCatalogue->UpgradeCosts.FindByPredicate(
            [](const FKalmalaToolRecipeData& Cost) { return Cost.ItemId == TEXT("IronAxe"); }) : nullptr;
    TestNotNull(TEXT("Iron Axe retains its authored level-one upgrade cost"), IronAxeUpgrade);
    if (IronAxeUpgrade)
    {
        TestTrue(TEXT("Iron Axe upgrade uses four Iron and three Lightwood"),
            IronAxeUpgrade->Ingredients.Num() == 2
            && IronAxeUpgrade->Ingredients.ContainsByPredicate([](const FKalmalaToolRecipeIngredientData& Cost)
                { return Cost.ItemId == TEXT("Iron") && Cost.Quantity == 4; })
            && IronAxeUpgrade->Ingredients.ContainsByPredicate([](const FKalmalaToolRecipeIngredientData& Cost)
                { return Cost.ItemId == TEXT("Lightwood") && Cost.Quantity == 3; }));
    }
    for (const TPair<FName, FName>& GearAndStation : GearStations)
    {
        const FKalmalaItemDefinition* GearItem = ItemCatalogue
            ? ItemCatalogue->FindItem(GearAndStation.Key) : nullptr;
        TestNotNull(TEXT("Wearable item exists in the ordinary item catalogue"), GearItem);
        if (GearItem) TestFalse(TEXT("Wearable item has a slot"), GearItem->GearSlot.IsEmpty());

        const FKalmalaRecipe* Acquisition = RecipeCatalogue
            ? RecipeCatalogue->Find(FName(*(GearAndStation.Key.ToString() + TEXT("Recipe")))) : nullptr;
        TestNotNull(TEXT("Wearable has a station acquisition recipe"), Acquisition);
        if (Acquisition)
        {
            TestEqual(TEXT("Recipe outputs its matching wearable"), Acquisition->Output, GearAndStation.Key);
            TestEqual(TEXT("Gear acquisition is a single-item craft"), Acquisition->OutputCount, 1);
            TestEqual(TEXT("Gear acquisition cannot be batch crafted"), Acquisition->MaxBatch, 1);
            const FName RuntimeStation = GearAndStation.Value == TEXT("Workbench")
                ? FName(TEXT("WorkbenchKit")) : FName(TEXT("ForgeKit"));
            TestTrue(TEXT("Gear acquisition names its required station"),
                Acquisition->RequiredStation.Contains(RuntimeStation));
            if (GearAndStation.Key == TEXT("BoarHideCape"))
            {
                TestTrue(TEXT("Boar Hide Cape acquisition requires one Bone Fragment"),
                    Acquisition->Ingredients.ContainsByPredicate([](const FKalmalaInventoryStack& Ingredient)
                        { return Ingredient.ItemId == TEXT("BoneFragment") && Ingredient.Quantity == 1; }));
            }
        }

        const FKalmalaToolRecipeData* Upgrade = AuthoredCatalogue
            ? AuthoredCatalogue->UpgradeCosts.FindByPredicate([&GearAndStation](const FKalmalaToolRecipeData& Cost)
                { return Cost.ItemId == GearAndStation.Key; }) : nullptr;
        TestNotNull(TEXT("Wearable has a level-one upgrade cost record"), Upgrade);
        if (Upgrade)
        {
            TestEqual(TEXT("Wearable upgrade cost uses the acquisition station"),
                Upgrade->RequiredStation, GearAndStation.Value.ToString());
            TestEqual(TEXT("Wearable upgrade cost begins at level one"), Upgrade->Level, 1);
            if (GearAndStation.Key == TEXT("BoarHideCape"))
            {
                TestTrue(TEXT("Boar Hide Cape upgrade requires one Bone Fragment"),
                    Upgrade->Ingredients.ContainsByPredicate([](const FKalmalaToolRecipeIngredientData& Ingredient)
                        { return Ingredient.ItemId == TEXT("BoneFragment") && Ingredient.Quantity == 1; }));
            }
        }
    }
    const FKalmalaToolRecipeData* BowUpgrade = AuthoredCatalogue
        ? AuthoredCatalogue->UpgradeCosts.FindByPredicate(
            [](const FKalmalaToolRecipeData& Cost) { return Cost.ItemId == TEXT("Bow"); }) : nullptr;
    TestNotNull(TEXT("Bow has a level-one upgrade cost record"), BowUpgrade);
    if (BowUpgrade)
    {
        TestEqual(TEXT("Bow upgrades at the Workbench"), BowUpgrade->RequiredStation, FString(TEXT("Workbench")));
        TestEqual(TEXT("Bow upgrade starts at level one"), BowUpgrade->Level, 1);
        TestTrue(TEXT("Bow upgrade reuses its acquisition materials"),
            BowUpgrade->Ingredients.ContainsByPredicate([](const FKalmalaToolRecipeIngredientData& Cost)
                { return Cost.ItemId == TEXT("Wood") && Cost.Quantity == 5; })
            && BowUpgrade->Ingredients.ContainsByPredicate([](const FKalmalaToolRecipeIngredientData& Cost)
                { return Cost.ItemId == TEXT("BowString") && Cost.Quantity == 1; }));
    }
    FKalmalaToolLevelStats BowLevelTwo;
    TestTrue(TEXT("Bow level-two stats resolve"),
        AuthoredCatalogue && AuthoredCatalogue->GetLevelStats(TEXT("Bow"), 2, BowLevelTwo));
    if (AuthoredCatalogue && AuthoredCatalogue->GetLevelStats(TEXT("Bow"), 2, BowLevelTwo))
    {
        const FKalmalaToolProfileData* BowProfile = AuthoredCatalogue->FindProfile(TEXT("Bow"));
        TestNotNull(TEXT("Bow profile exists for progression"), BowProfile);
        if (BowProfile)
        {
            TestEqual(TEXT("Bow gains fifty durability per level"), BowLevelTwo.MaxDurability,
                BowProfile->MaxDurability + 50);
            TestEqual(TEXT("Bow receives no damage reward before shooting is implemented"),
                BowLevelTwo.DamageBonuses.Total(), BowProfile->DamageBonuses.Total());
        }
    }
    UKalmalaToolCatalogue* BoundaryCatalogue = NewObject<UKalmalaToolCatalogue>();
    BoundaryCatalogue->Profiles = AuthoredCatalogue->Profiles;
    BoundaryCatalogue->Recipes = AuthoredCatalogue->Recipes;
    BoundaryCatalogue->UpgradeCosts = AuthoredCatalogue->UpgradeCosts;
    BoundaryCatalogue->LevelUpRewards = AuthoredCatalogue->LevelUpRewards;
    BoundaryCatalogue->AttackLists = AuthoredCatalogue->AttackLists;
    FKalmalaToolProfileData* BoundaryAxe = BoundaryCatalogue->Profiles.FindByPredicate(
        [](const FKalmalaToolProfileData& Profile) { return Profile.ItemId == TEXT("BronzeAxe"); });
    if (TestNotNull(TEXT("Boundary catalogue contains Bronze Axe"), BoundaryAxe))
    {
        FKalmalaToolLevelStats Stats;
        TestTrue(TEXT("Reward definition produces level twenty"), BoundaryCatalogue->GetLevelStats(TEXT("BronzeAxe"), 20, Stats));
        TestEqual(TEXT("Durability adds the reward nineteen times"), Stats.MaxDurability, 1050);
        TestEqual(TEXT("Slash adds the reward nineteen times"), Stats.DamageBonuses.Slash, 145.0f);
        TestFalse(TEXT("Reward calculation rejects level twenty-one"), BoundaryCatalogue->GetLevelStats(TEXT("BronzeAxe"), 21, Stats));
        TestFalse(TEXT("Reward calculation rejects level zero"), BoundaryCatalogue->GetLevelStats(TEXT("BronzeAxe"), 0, Stats));
        TestTrue(TEXT("Reward catalogue remains valid through the hard cap"), BoundaryCatalogue->IsValidCatalogue());
        for (const FName ItemId : {FName(TEXT("StoneAxe")), FName(TEXT("BronzeAxe")), FName(TEXT("IronAxe"))})
        {
            const FKalmalaToolProfileData* Profile = BoundaryCatalogue->FindProfile(ItemId);
            if (TestNotNull(TEXT("Shared reward axe exists"), Profile)
                && TestTrue(TEXT("Shared reward resolves axe level two"), BoundaryCatalogue->GetLevelStats(ItemId, 2, Stats)))
            {
                TestEqual(TEXT("Every axe gains fifty durability"), Stats.MaxDurability, Profile->MaxDurability + 50);
                TestEqual(TEXT("Every axe gains five slash"), Stats.DamageBonuses.Slash, Profile->DamageBonuses.Slash + 5.0f);
            }
        }
        const int32 GroupIndex = BoundaryCatalogue->LevelUpRewards.IndexOfByPredicate(
            [](const FKalmalaLevelUpRewardGroupData& Group) { return Group.ItemIds.Contains(TEXT("BronzeAxe")); });
        if (TestTrue(TEXT("Axes share an authored reward group"), GroupIndex != INDEX_NONE))
        {
            const FKalmalaLevelUpRewardGroupData Original = BoundaryCatalogue->LevelUpRewards[GroupIndex];
            BoundaryCatalogue->LevelUpRewards[GroupIndex].LevelUpReward.MaxDurability = -1;
            TestFalse(TEXT("Negative shared rewards are rejected"), BoundaryCatalogue->IsValidCatalogue());
            BoundaryCatalogue->LevelUpRewards[GroupIndex] = Original;
            BoundaryCatalogue->LevelUpRewards[GroupIndex].ItemIds.Add(TEXT("UnknownTool"));
            TestFalse(TEXT("Unknown reward IDs are rejected"), BoundaryCatalogue->IsValidCatalogue());
            BoundaryCatalogue->LevelUpRewards[GroupIndex] = Original;
            BoundaryCatalogue->LevelUpRewards[GroupIndex].ItemIds.Add(TEXT("BronzeAxe"));
            TestFalse(TEXT("Duplicate IDs within a reward are rejected"), BoundaryCatalogue->IsValidCatalogue());
            BoundaryCatalogue->LevelUpRewards[GroupIndex] = Original;
            BoundaryCatalogue->LevelUpRewards.Add(Original);
            TestFalse(TEXT("Overlapping reward groups are rejected"), BoundaryCatalogue->IsValidCatalogue());
        }

    }

    TArray<FKalmalaToolRecipeIngredientData> BaseCosts;
    FKalmalaToolRecipeIngredientData& Wood = BaseCosts.AddDefaulted_GetRef();
    Wood.ItemId = TEXT("Wood"); Wood.Quantity = 5;
    FKalmalaToolRecipeIngredientData& Iron = BaseCosts.AddDefaulted_GetRef();
    Iron.ItemId = TEXT("Iron"); Iron.Quantity = 2;
    TArray<FKalmalaToolRecipeIngredientData> ScaledCosts;
    for (int32 CurrentLevel = 1; CurrentLevel <= 3; ++CurrentLevel)
    {
        if (TestTrue(TEXT("Base upgrade costs scale at consecutive levels"),
            UKalmalaToolCatalogue::ScaleUpgradeCosts(BaseCosts, CurrentLevel, ScaledCosts)))
        {
            TestEqual(TEXT("Wood doubles for every subsequent upgrade"), ScaledCosts[0].Quantity, 5 << (CurrentLevel - 1));
            TestEqual(TEXT("Other ingredients double with Wood"), ScaledCosts[1].Quantity, 2 << (CurrentLevel - 1));
        }
    }
    TestFalse(TEXT("Invalid current level rejects upgrade costs"),
        UKalmalaToolCatalogue::ScaleUpgradeCosts(BaseCosts, 0, ScaledCosts));
    TestFalse(TEXT("Overflow rejects upgrade costs"),
        UKalmalaToolCatalogue::ScaleUpgradeCosts(BaseCosts, 31, ScaledCosts));
    TestTrue(TEXT("Rejected scaling returns no partial costs"), ScaledCosts.IsEmpty());

    const TArray<FKalmalaToolProgressionEntry>& Entries = FKalmalaToolProgressionContract::GetEntries();
    TestEqual(TEXT("Progression bound covers every profile through the station level cap"),
        FKalmalaToolProgressionContract::MaxToolProgressionEntries,
        UKalmalaToolCatalogue::MaxProfiles * FKalmalaToolProgressionContract::MaxStationToolLevel);
    TestTrue(TEXT("Crafted tool progression catalogue is bounded and internally valid"),
        FKalmalaToolProgressionContract::IsCatalogueValid());
    int32 ExpectedProgressionEntryCount = AuthoredCatalogue ? AuthoredCatalogue->Recipes.Num() : 0;
    if (AuthoredCatalogue)
    {
        for (const FKalmalaToolRecipeData& Upgrade : AuthoredCatalogue->UpgradeCosts)
        {
            if (AuthoredCatalogue->FindProfile(Upgrade.ItemId))
                ExpectedProgressionEntryCount += FKalmalaToolProgressionContract::MaxStationToolLevel - 1;
        }
    }
    TestEqual(TEXT("Authored station progression covers acquisitions and each item upgrade"),
        Entries.Num(), ExpectedProgressionEntryCount);
    const FKalmalaToolProgressionEntry* BowLevelTwoEntry =
        FKalmalaToolProgressionContract::FindEntry(TEXT("Bow"), 2);
    TestNotNull(TEXT("Bow has a generated level-two progression entry"), BowLevelTwoEntry);
    if (BowLevelTwoEntry)
    {
        TestEqual(TEXT("Bow level two uses the Workbench"),
            BowLevelTwoEntry->RequiredStation, EKalmalaToolStationKind::Workbench);
        TestEqual(TEXT("Bow level two requires a level-two Workbench"),
            BowLevelTwoEntry->RequiredStationLevel, 2);
        TestTrue(TEXT("Bow level two scales from the acquisition materials"),
            BowLevelTwoEntry->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Wood") && Cost.Quantity == 5; })
            && BowLevelTwoEntry->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("BowString") && Cost.Quantity == 1; }));
    }
    const FKalmalaToolProgressionEntry* ConstructionHammerLevelTwoEntry =
        FKalmalaToolProgressionContract::FindEntry(TEXT("ConstructionHammer"), 2);
    const FKalmalaRecipe* ConstructionHammerRecipe = UKalmalaRecipeCatalogue::Get()->Find(TEXT("BuildingHammer"));
    TestNotNull(TEXT("Construction Hammer has an inventory acquisition recipe"), ConstructionHammerRecipe);
    if (ConstructionHammerRecipe)
    {
        TestTrue(TEXT("Construction Hammer recipe is enabled and station-free"),
            ConstructionHammerRecipe->bEnabled && ConstructionHammerRecipe->Output == TEXT("ConstructionHammer")
            && ConstructionHammerRecipe->RequiredStation.IsEmpty());
    }
    TestNotNull(TEXT("Inventory-crafted Construction Hammer has a level-two progression entry"), ConstructionHammerLevelTwoEntry);
    if (ConstructionHammerLevelTwoEntry)
    {
        TestEqual(TEXT("Level-two entry upgrades the inventory-crafted level-one item"),
            ConstructionHammerLevelTwoEntry->PreviousItemId, FName(TEXT("ConstructionHammer")));
        TestEqual(TEXT("Level-two entry requires the crafted hammer at level one"),
            ConstructionHammerLevelTwoEntry->PreviousToolLevel, 1);
    }
    TestTrue(TEXT("Generated entries remain bounded"), Entries.Num() <= FKalmalaToolProgressionContract::MaxToolProgressionEntries);
    for (const TPair<FName, FName>& GearAndStation : GearStations)
        TestNull(TEXT("Wearable costs do not enter carried-tool progression"),
            FKalmalaToolProgressionContract::FindEntry(GearAndStation.Key));

    const FKalmalaToolProgressionEntry* Bronze = FKalmalaToolProgressionContract::FindEntry(TEXT("BronzeAxe"));
    TestNotNull(TEXT("Bronze Axe has a progression entry"), Bronze);
    if (Bronze != nullptr)
    {
        TestEqual(TEXT("Bronze Axe is authored as tool level one"), Bronze->TargetToolLevel, 1);
        TestEqual(TEXT("Bronze Axe uses the level-one Forge"),
            Bronze->RequiredStation, EKalmalaToolStationKind::Forge);
        TestEqual(TEXT("Bronze Axe station level matches its target"), Bronze->RequiredStationLevel, 1);
        TestTrue(TEXT("Bronze Axe is the first step in this axe path"),
            Bronze->PreviousItemId.IsNone() && Bronze->PreviousToolLevel == 0);
        TestEqual(TEXT("Bronze Axe acquisition has two bounded materials"), Bronze->MaterialCosts.Num(), 2);
        TestTrue(TEXT("Bronze Axe uses its authored Lightwood and Stone costs"),
            Bronze->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Lightwood") && Cost.Quantity == 4; })
            && Bronze->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Stone") && Cost.Quantity == 3; }));
    }

    const FKalmalaToolProgressionEntry* BronzeLevelTwo =
        FKalmalaToolProgressionContract::FindEntry(TEXT("BronzeAxe"), 2);
    TestNotNull(TEXT("Bronze Axe has a level-two upgrade entry"), BronzeLevelTwo);
    if (BronzeLevelTwo != nullptr)
    {
        TestEqual(TEXT("Bronze Axe upgrades to tool level two"), BronzeLevelTwo->TargetToolLevel, 2);
        TestEqual(TEXT("Bronze Axe level two uses the Forge"), BronzeLevelTwo->RequiredStation, EKalmalaToolStationKind::Forge);
        TestEqual(TEXT("Forge level matches the Bronze Axe target"), BronzeLevelTwo->RequiredStationLevel, 2);
        TestEqual(TEXT("Upgrade keeps the same item identity"), BronzeLevelTwo->PreviousItemId, FName(TEXT("BronzeAxe")));
        TestEqual(TEXT("Upgrade requires Bronze Axe level one"), BronzeLevelTwo->PreviousToolLevel, 1);
        TestEqual(TEXT("Bronze Axe upgrade has two bounded materials"), BronzeLevelTwo->MaterialCosts.Num(), 2);
        TestTrue(TEXT("Bronze Axe upgrade uses its authored Lightwood and Bronze costs"),
            BronzeLevelTwo->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Lightwood") && Cost.Quantity == 4; })
            && BronzeLevelTwo->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Bronze") && Cost.Quantity == 3; }));
        TestFalse(TEXT("Bronze Axe level two does not require Densewood gated behind itself"),
            BronzeLevelTwo->MaterialCosts.ContainsByPredicate([](const FKalmalaToolMaterialCost& Cost)
                { return Cost.ItemId == TEXT("Densewood"); }));
    }

    const TArray<FKalmalaToolState> StartingTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
    TestFalse(TEXT("Uncrafted Bronze Axe is not granted at character start"),
        StartingTools.ContainsByPredicate([](const FKalmalaToolState& State) { return State.ItemId == TEXT("BronzeAxe"); }));
    TestFalse(TEXT("Uncrafted Iron Axe is not granted at character start"),
        StartingTools.ContainsByPredicate([](const FKalmalaToolState& State) { return State.ItemId == TEXT("IronAxe"); }));

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

    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const FKalmalaItemDefinition* ForgeKit = Items->FindBuildable(TEXT("ForgeKit"));
    TestNotNull(TEXT("Forge kit is in the item catalogue"), ForgeKit);
    const FKalmalaRecipe* ForgeRecipe = UKalmalaRecipeCatalogue::Get()->Find(TEXT("Forge"));
    TestNotNull(TEXT("Forge has a paid build recipe"), ForgeRecipe);
    if (ForgeRecipe)
    {
        TestEqual(TEXT("Forge recipe directly places its buildable identity"), ForgeRecipe->BuildableOutput, FName(TEXT("ForgeKit")));
        TestTrue(TEXT("Forge construction does not produce an inventory item"), ForgeRecipe->Output.IsNone());
        const FKalmalaInventoryStack* WoodCost = ForgeRecipe->Ingredients.FindByPredicate(
            [](const FKalmalaInventoryStack& Cost) { return Cost.ItemId == TEXT("Wood"); });
        const FKalmalaInventoryStack* StoneCost = ForgeRecipe->Ingredients.FindByPredicate(
            [](const FKalmalaInventoryStack& Cost) { return Cost.ItemId == TEXT("Stone"); });
        TestNotNull(TEXT("Forge recipe includes raw wood"), WoodCost);
        TestNotNull(TEXT("Forge recipe includes fieldstone"), StoneCost);
        if (WoodCost) TestEqual(TEXT("Forge recipe uses the direct wood value of five former supplies"), WoodCost->Quantity, 15);
        if (StoneCost) TestEqual(TEXT("Forge kit costs six fieldstone"), StoneCost->Quantity, 6);
    }

    const UKalmalaRecipeCatalogue* Recipes = UKalmalaRecipeCatalogue::Get();
    TestTrue(TEXT("The complete recipe catalogue accepts both paid build-menu attachment recipes"),
        Recipes->IsValidCatalogue());
    const FKalmalaRecipe* RackRecipe = Recipes->Find(TEXT("WorkbenchToolRack"));
    const FKalmalaRecipe* AnvilRecipe = Recipes->Find(TEXT("ForgeAnvil"));
    TestNotNull(TEXT("Workbench tool rack has a paid build recipe"), RackRecipe);
    TestNotNull(TEXT("Forge anvil has a paid build recipe"), AnvilRecipe);
    if (RackRecipe)
    {
        TestEqual(TEXT("Rack recipe places a Workbench attachment"), RackRecipe->BuildableOutput, FName(TEXT("WorkbenchToolRackKit")));
        TestEqual(TEXT("Rack recipe selects the Workbench family from its placed identity"),
            FKalmalaToolProgressionContract::GetAttachmentStationKit(RackRecipe->BuildableOutput), FName(TEXT("WorkbenchKit")));
        TestTrue(TEXT("Rack placement does not produce an inventory item"), RackRecipe->Output.IsNone());
        TestTrue(TEXT("Rack recipe is built from the hammer Build menu"), RackRecipe->RequiredStation.IsEmpty());
        TestTrue(TEXT("Rack recipe has paid material costs"), !RackRecipe->Ingredients.IsEmpty());
    }
    if (AnvilRecipe)
    {
        TestEqual(TEXT("Anvil recipe places a Forge attachment"), AnvilRecipe->BuildableOutput, FName(TEXT("ForgeAnvilKit")));
        TestEqual(TEXT("Anvil recipe selects the Forge family from its placed identity"),
            FKalmalaToolProgressionContract::GetAttachmentStationKit(AnvilRecipe->BuildableOutput), FName(TEXT("ForgeKit")));
        TestTrue(TEXT("Anvil placement does not produce an inventory item"), AnvilRecipe->Output.IsNone());
        TestTrue(TEXT("Anvil recipe is built from the hammer Build menu"), AnvilRecipe->RequiredStation.IsEmpty());
        TestTrue(TEXT("Anvil recipe has paid material costs"), !AnvilRecipe->Ingredients.IsEmpty());
    }

    const UFunction* PlaceIntent = UKalmalaCraftingComponent::StaticClass()->FindFunctionByName(TEXT("ServerPlaceConstruction"));
    if (TestNotNull(TEXT("Construction placement intent exists"), PlaceIntent))
    {
        TestTrue(TEXT("Construction placement is an owning-client server RPC"),
            PlaceIntent->HasAllFunctionFlags(FUNC_Net | FUNC_NetServer));
        TestEqual(TEXT("Placement submits only a canonical buildable identity"), int32(PlaceIntent->NumParms), 1);
        TestNotNull(TEXT("Placement identifies the requested buildable"),
            PlaceIntent->FindPropertyByName(TEXT("BuildableId")));
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FKalmalaUpgradeOnlyProgressionTest,
    "Kalmala.Gameplay.Tools.UpgradeOnlyProgression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaUpgradeOnlyProgressionTest::RunTest(const FString& Parameters)
{
    // Opening the hammer Build menu reads this complete list, even with no station open.
    const TArray<FKalmalaToolProgressionEntry>& InitialEntries = FKalmalaToolProgressionContract::GetEntries();
    int32 HammerUpgrades = 0;
    int32 PreviousHammerLevel = 1;
    for (const FKalmalaToolProgressionEntry& Entry : InitialEntries)
    {
        if (Entry.ItemId != TEXT("ConstructionHammer")) continue;
        ++HammerUpgrades;
        TestEqual(TEXT("Upgrade-only hammer entries retain ascending target levels"),
            Entry.TargetToolLevel, PreviousHammerLevel + 1);
        TestEqual(TEXT("Hammer upgrade preserves its item identity"), Entry.PreviousItemId, Entry.ItemId);
        PreviousHammerLevel = Entry.TargetToolLevel;
    }
    TestEqual(TEXT("Spawn-granted hammer retains all nine station upgrades without an acquisition entry"),
        HammerUpgrades, FKalmalaToolProgressionContract::MaxStationToolLevel - 1);
    TestNull(TEXT("Sorting upgrade-only items does not invent a hammer acquisition recipe"),
        FKalmalaToolProgressionContract::FindEntry(TEXT("ConstructionHammer"), 1));

    return true;
}

#endif
