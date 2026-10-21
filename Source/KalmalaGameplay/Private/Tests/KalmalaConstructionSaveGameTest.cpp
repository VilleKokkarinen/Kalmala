#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaPlacementPreview.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionSaveGameTest, "Kalmala.Gameplay.Construction.SaveContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionSaveGameTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World; World.WorldSeed = 418; auto* Save = NewObject<UKalmalaConstructionSaveGame>(); Save->InitializeForWorld(World);
    FKalmalaConstructionSaveRecord Record; Record.ConstructionId = TEXT("camp-0001"); Record.KitId = TEXT("FloorKit"); Record.Transform = FTransform(FVector(100, 200, 300));
    TestTrue(TEXT("Construction save accepts a bounded valid record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-rack"); Record.KitId = TEXT("CookingRackKit");
    TestTrue(TEXT("Cooking rack uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-cauldron"); Record.KitId = TEXT("CauldronKit");
    TestTrue(TEXT("Cauldron uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-frying-pan"); Record.KitId = TEXT("FryingPanKit");
    TestTrue(TEXT("Placeable frying pan uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("camp-charcoal-kiln"); Record.KitId = TEXT("CharcoalKilnKit");
    TestTrue(TEXT("Charcoal Kiln uses the existing bounded construction record"), Save->AddRecord(Record));
    Record.ConstructionId = TEXT("session-attachment"); Record.KitId = TEXT("WorkbenchToolRackKit");
    TestFalse(TEXT("M9 station attachments remain outside the existing save schema"), UKalmalaConstructionSaveGame::IsValidRecord(Record));
    TestTrue(TEXT("Station attachment session policy is explicit"), FKalmalaPlacementPreview::IsSessionOnlyKit(TEXT("WorkbenchToolRackKit")));
    Record.ConstructionId = TEXT("camp-0001"); Record.KitId = TEXT("FloorKit");
    Record.Transform.SetLocation(FVector(NAN, 0, 0));
    TestFalse(TEXT("Construction save rejects non-finite transforms"), UKalmalaConstructionSaveGame::IsValidRecord(Record));
    Record.Transform = FTransform(FVector(100, 200, 300));
    TestFalse(TEXT("Construction save rejects duplicate stable IDs"), Save->AddRecord(Record));
    TestTrue(TEXT("Construction save can roll back a newly added record"), Save->RemoveRecord(TEXT("camp-0001")));
    TestTrue(TEXT("Construction save can re-add a rolled-back record"), Save->AddRecord(Record));
    for (int32 Index = Save->GetRecords().Num() + 1; Index <= UKalmalaConstructionSaveGame::MaxRecords; ++Index)
    {
        Record.ConstructionId = FString::Printf(TEXT("camp-%04d"), Index);
        TestTrue(TEXT("Construction save accepts records within its cap"), Save->AddRecord(Record));
    }
    Record.ConstructionId = TEXT("camp-over-cap");
    TestFalse(TEXT("Construction save rejects records beyond its cap"), Save->AddRecord(Record));
    TArray<uint8> Bytes; TestTrue(TEXT("Construction save serializes in memory"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Reloaded = Cast<UKalmalaConstructionSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Construction save reloads with its type"), Reloaded))
    {
        TestTrue(TEXT("Construction save retains matching world identity"), Reloaded->MatchesWorld(World));
        TestEqual(TEXT("Construction save retains every bounded stable record"), Reloaded->GetRecords().Num(), UKalmalaConstructionSaveGame::MaxRecords);
    }
    World.WorldSeed = 419;
    TestFalse(TEXT("Construction save rejects a different world identity"), Save->MatchesWorld(World));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaConstructionSaveGameV2Test,
    "Kalmala.Gameplay.Construction.Schema5CurrentFormat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaConstructionSaveGameV2Test::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;

    const auto MakeRecord = [](const TCHAR* Id, const FName KitId)
    {
        FKalmalaConstructionSaveRecord Record;
        Record.ConstructionId = Id;
        Record.KitId = KitId;
        Record.Transform = FTransform(FVector(100.0f, 200.0f, 300.0f));
        return Record;
    };
    const auto Serialize = [](USaveGame* Save, TArray<uint8>& OutBytes)
    {
        OutBytes.Reset();
        return UGameplayStatics::SaveGameToMemory(Save, OutBytes);
    };

    UKalmalaConstructionSaveGameV2* Save = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    Save->InitializeForWorld(World);
    const FKalmalaConstructionSaveRecord Floor = MakeRecord(TEXT("camp-floor-01"), TEXT("FloorKit"));
    const FKalmalaConstructionSaveRecord Rack = MakeRecord(TEXT("camp-rack-01"), TEXT("WorkbenchToolRackKit"));
    const FKalmalaConstructionSaveRecord Campfire = MakeRecord(TEXT("camp-fire-01"), TEXT("CampfireKit"));
    FKalmalaConstructionSaveRecord Kiln = MakeRecord(TEXT("camp-kiln-01"), TEXT("CharcoalKilnKit"));
    Kiln.CharcoalKiln.InputWood = 2;
    Kiln.CharcoalKiln.OutputCoal = 1;
    Kiln.CharcoalKiln.JobRemainingSeconds = 12.5f;
    FKalmalaConstructionSaveRecord Smelter = MakeRecord(TEXT("camp-smelter-01"), TEXT("SmelterKit"));
    TestTrue(TEXT("Smelter fixture queues Copper Ore"), Smelter.Smelter.TryAddOre(EKalmalaSmelterOre::Copper));
    TestTrue(TEXT("Smelter fixture queues Coal"), Smelter.Smelter.TryAddCoal());
    TestTrue(TEXT("Smelter fixture starts and saves an active job"), Smelter.Smelter.Advance(12.5f));
    FKalmalaConstructionSaveRecord Plot = MakeRecord(TEXT("camp-garden-01"), TEXT("GardenPlotKit"));
    TestTrue(TEXT("Carrot food mode plants into an empty Garden Plot"),
        Plot.GardenPlot.PlantCarrot(EKalmalaGardenCropMode::CarrotFood));
    TestEqual(TEXT("Carrot growth begins at 600 active seconds"), Plot.GardenPlot.RemainingActiveSeconds,
        FKalmalaGardenPlotState::CropDurationSeconds);
    TestTrue(TEXT("Carrot growth advances by active time"), Plot.GardenPlot.Advance(17.25f));
    TestTrue(TEXT("Carrot state retains its exact remaining active seconds"),
        FMath::IsNearlyEqual(Plot.GardenPlot.RemainingActiveSeconds, 582.75f));
    TestFalse(TEXT("Zero active time cannot advance a crop"), Plot.GardenPlot.Advance(0.0f));
    FKalmalaGardenPlotState SeedPlotState;
    TestTrue(TEXT("Carrot seed mode plants from one Carrot"),
        SeedPlotState.PlantCarrot(EKalmalaGardenCropMode::CarrotSeed));
    TestTrue(TEXT("Carrot seed mode matures after 600 active seconds"), SeedPlotState.Advance(600.0f));
    TestTrue(TEXT("Mature Carrot seed mode is harvestable once"), SeedPlotState.ClearMatureCrop());
    TestFalse(TEXT("Harvest cannot clear a crop a second time"), SeedPlotState.ClearMatureCrop());
    FKalmalaConstructionSaveRecord InvalidFloor = Floor;
    InvalidFloor.GardenPlot.PlantCarrot(EKalmalaGardenCropMode::CarrotFood);
    TestFalse(TEXT("Non-plot construction rejects crop state"),
        UKalmalaConstructionSaveGameV2::IsValidRecord(InvalidFloor));
    TestTrue(TEXT("Schema 5 accepts an established camp record"), Save->AddRecord(Floor));
    TestTrue(TEXT("Schema 5 accepts a bounded station attachment"), Save->AddStationAttachmentRecord(Rack));
    TestTrue(TEXT("Schema 5 retains the stable Campfire construction identity"), Save->AddRecord(Campfire));
    TestTrue(TEXT("Schema 5 saves kiln input, output, and remaining active seconds"), Save->AddRecord(Kiln));
    TestTrue(TEXT("Schema 5 saves Smelter input and active job state"), Save->AddRecord(Smelter));
    TestTrue(TEXT("Garden Plot saves in the current construction record"), Save->AddRecord(Plot));
    TestFalse(TEXT("Schema 5 rejects a duplicate stable construction ID across record kinds"),
        Save->AddStationAttachmentRecord(MakeRecord(TEXT("camp-floor-01"), TEXT("ForgeAnvilKit"))));
    TestFalse(TEXT("Schema 5 rejects unsupported construction identities"),
        Save->AddRecord(MakeRecord(TEXT("camp-unknown-01"), TEXT("UnknownConstructionKit"))));
    TestEqual(TEXT("Rejected schema-5 additions leave the live candidate unchanged"), Save->GetRecords().Num(), 6);

    const auto BuildPlotRemovalCandidate = [this, &World, &MakeRecord](const TCHAR* Id, const EKalmalaGardenCropMode Mode)
    {
        UKalmalaConstructionSaveGameV2* Candidate = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
        Candidate->InitializeForWorld(World);
        FKalmalaConstructionSaveRecord Record = MakeRecord(Id, TEXT("GardenPlotKit"));
        Record.GardenPlot.PlantCarrot(Mode);
        TestTrue(TEXT("Planted Garden Plot fixture is a valid saved record"), Candidate->AddRecord(Record));
        return Candidate;
    };
    UKalmalaConstructionSaveGameV2* PlantedPlot = BuildPlotRemovalCandidate(
        TEXT("plot-growing"), EKalmalaGardenCropMode::CarrotFood);
    TestFalse(TEXT("Growing crop prevents Garden Plot removal"), PlantedPlot->RemoveRecord(TEXT("plot-growing")));
    FKalmalaGardenPlotState MatureCrop = PlantedPlot->GetRecords()[0].GardenPlot;
    TestTrue(TEXT("Carrot reaches maturity after active seconds"), MatureCrop.Advance(600.0f));
    TestTrue(TEXT("Mature crop clears to an empty reusable plot"), MatureCrop.ClearMatureCrop());
    TestTrue(TEXT("Empty plot state can be persisted after harvest"), PlantedPlot->UpdateGardenPlotRecord(TEXT("plot-growing"), MatureCrop));
    TestTrue(TEXT("Harvested empty Garden Plot can be removed"), PlantedPlot->RemoveRecord(TEXT("plot-growing")));

    const auto BuildKilnRemovalCandidate = [this, &World, &MakeRecord](const TCHAR* Id, const FKalmalaCharcoalKilnState& State)
    {
        UKalmalaConstructionSaveGameV2* Candidate = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
        Candidate->InitializeForWorld(World);
        FKalmalaConstructionSaveRecord Record = MakeRecord(Id, TEXT("CharcoalKilnKit"));
        Record.CharcoalKiln = State;
        TestTrue(TEXT("Kiln removal fixture contains a valid record"), Candidate->AddRecord(Record));
        return Candidate;
    };
    FKalmalaCharcoalKilnState RemovalState;
    RemovalState.InputWood = 1;
    UKalmalaConstructionSaveGameV2* RemovalCandidate = BuildKilnRemovalCandidate(TEXT("kiln-input"), RemovalState);
    TestFalse(TEXT("Kiln with queued Wood cannot be removed"), RemovalCandidate->RemoveRecord(TEXT("kiln-input")));
    TestEqual(TEXT("Rejected Kiln removal preserves the saved record"), RemovalCandidate->GetRecords().Num(), 1);
    RemovalState = FKalmalaCharcoalKilnState();
    RemovalState.OutputCoal = 1;
    RemovalCandidate = BuildKilnRemovalCandidate(TEXT("kiln-output"), RemovalState);
    TestFalse(TEXT("Kiln with output Coal cannot be removed"), RemovalCandidate->RemoveRecord(TEXT("kiln-output")));
    RemovalState = FKalmalaCharcoalKilnState();
    RemovalState.JobRemainingSeconds = 12.5f;
    RemovalCandidate = BuildKilnRemovalCandidate(TEXT("kiln-active"), RemovalState);
    TestFalse(TEXT("Kiln with active job time cannot be removed"), RemovalCandidate->RemoveRecord(TEXT("kiln-active")));
    RemovalCandidate = BuildKilnRemovalCandidate(TEXT("kiln-empty"), FKalmalaCharcoalKilnState());
    TestTrue(TEXT("Empty Kiln can be removed"), RemovalCandidate->RemoveRecord(TEXT("kiln-empty")));
    TestEqual(TEXT("Successful empty Kiln removal deletes one record"), RemovalCandidate->GetRecords().Num(), 0);

    const auto BuildSmelterRemovalCandidate = [this, &World, &MakeRecord](const TCHAR* Id, const FKalmalaSmelterState& State)
    {
        UKalmalaConstructionSaveGameV2* Candidate = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
        Candidate->InitializeForWorld(World);
        FKalmalaConstructionSaveRecord Record = MakeRecord(Id, TEXT("SmelterKit"));
        Record.Smelter = State;
        TestTrue(TEXT("Smelter removal fixture contains a valid record"), Candidate->AddRecord(Record));
        return Candidate;
    };
    FKalmalaSmelterState SmelterRemovalState;
    SmelterRemovalState.TryAddOre(EKalmalaSmelterOre::Copper);
    RemovalCandidate = BuildSmelterRemovalCandidate(TEXT("smelter-input"), SmelterRemovalState);
    TestFalse(TEXT("Smelter with queued ore cannot be removed"), RemovalCandidate->RemoveRecord(TEXT("smelter-input")));
    SmelterRemovalState = FKalmalaSmelterState();
    SmelterRemovalState.OutputCopper = 1;
    RemovalCandidate = BuildSmelterRemovalCandidate(TEXT("smelter-output"), SmelterRemovalState);
    TestFalse(TEXT("Smelter with output cannot be removed"), RemovalCandidate->RemoveRecord(TEXT("smelter-output")));
    SmelterRemovalState = FKalmalaSmelterState();
    SmelterRemovalState.TryAddOre(EKalmalaSmelterOre::Iron);
    SmelterRemovalState.TryAddCoal();
    SmelterRemovalState.Advance(5.0f);
    RemovalCandidate = BuildSmelterRemovalCandidate(TEXT("smelter-active"), SmelterRemovalState);
    TestFalse(TEXT("Smelter with an active job cannot be removed"), RemovalCandidate->RemoveRecord(TEXT("smelter-active")));
    RemovalCandidate = BuildSmelterRemovalCandidate(TEXT("smelter-empty"), FKalmalaSmelterState());
    TestTrue(TEXT("Empty Smelter can be removed"), RemovalCandidate->RemoveRecord(TEXT("smelter-empty")));

    TArray<uint8> CurrentBytes;
    TestTrue(TEXT("Schema 5 construction records serialize in memory"), Serialize(Save, CurrentBytes));
    UKalmalaConstructionSaveGameV2* Reloaded =
        Cast<UKalmalaConstructionSaveGameV2>(UGameplayStatics::LoadGameFromMemory(CurrentBytes));
    if (TestNotNull(TEXT("Schema 5 construction records reload with their type"), Reloaded))
    {
        TestTrue(TEXT("Reloaded container carries exact world, revision, and world scope"), Reloaded->MatchesWorld(World));
        TestEqual(TEXT("Existing construction, Campfire, Kiln, Smelter, plot, and attachment records round-trip together"), Reloaded->GetRecords().Num(), 6);
        TestTrue(TEXT("Garden Plot crop mode and active time survive reload"), Reloaded->GetRecords().ContainsByPredicate(
            [](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("camp-garden-01") && Record.KitId == TEXT("GardenPlotKit")
                    && Record.GardenPlot.CropMode == EKalmalaGardenCropMode::CarrotFood
                    && FMath::IsNearlyEqual(Record.GardenPlot.RemainingActiveSeconds, 582.75f);
            }));
        TestTrue(TEXT("Kiln inventory and job timer survive reload"), Reloaded->GetRecords().ContainsByPredicate(
            [](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("camp-kiln-01") && Record.KitId == TEXT("CharcoalKilnKit")
                    && Record.CharcoalKiln.InputWood == 2 && Record.CharcoalKiln.OutputCoal == 1
                    && FMath::IsNearlyEqual(Record.CharcoalKiln.JobRemainingSeconds, 12.5f);
            }));
        TestTrue(TEXT("Reloaded container retains the Campfire construction ID"),
            Reloaded->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("camp-fire-01") && Record.KitId == TEXT("CampfireKit");
            }));
        TestTrue(TEXT("Reloaded container retains its station attachment"),
            Reloaded->GetRecords().ContainsByPredicate([](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.KitId == TEXT("WorkbenchToolRackKit");
            }));
        TestTrue(TEXT("Smelter ore type and active timer survive reload"), Reloaded->GetRecords().ContainsByPredicate(
            [](const FKalmalaConstructionSaveRecord& Record)
            {
                return Record.ConstructionId == TEXT("camp-smelter-01") && Record.KitId == TEXT("SmelterKit")
                    && Record.Smelter.InputOreQueue.IsEmpty() && Record.Smelter.InputCoal == 0
                    && Record.Smelter.ActiveJobOre == EKalmalaSmelterOre::Copper
                    && FMath::IsNearlyEqual(Record.Smelter.JobRemainingSeconds, 17.5f);
            }));
    }

    UKalmalaConstructionSaveGameV2* IdentityChecks = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    const FKalmalaM7SaveIdentity ExpectedIdentity = IdentityChecks->Identity;
    IdentityChecks->Identity.WorldSeed++;
    TestFalse(TEXT("Schema 5 rejects a mismatched identity seed"), IdentityChecks->MatchesWorld(World));
    IdentityChecks->Identity = ExpectedIdentity;
    IdentityChecks->Identity.GeneratorRevision--;
    TestFalse(TEXT("Schema 5 rejects a mismatched generator revision"), IdentityChecks->MatchesWorld(World));
    IdentityChecks->Identity = ExpectedIdentity;
    IdentityChecks->Identity.Scope = EKalmalaM7SaveScope::Player;
    IdentityChecks->Identity.OwnerIdentity = TEXT("provider:other");
    TestFalse(TEXT("Schema 5 rejects a mismatched persistence scope"), IdentityChecks->MatchesWorld(World));

    UKalmalaConstructionSaveGameV2* InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->SchemaVersion = 0;
    TestFalse(TEXT("Current loads reject unsupported schema zero"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent->SchemaVersion = UKalmalaConstructionSaveGameV2::SchemaVersionValue + 1;
    TestFalse(TEXT("Current loads reject future schema versions"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->Records[0].Transform.SetLocation(FVector(0.0f, NAN, 0.0f));
    TestFalse(TEXT("Current loads reject malformed transforms"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->Records[0].ConstructionId.Reset();
    TestFalse(TEXT("Current loads reject malformed construction identities"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    InvalidCurrent->Records[3].CharcoalKiln.OutputCoal = FKalmalaCharcoalKilnState::OutputCapacity;
    InvalidCurrent->Records[3].CharcoalKiln.JobRemainingSeconds = 1.0f;
    TestFalse(TEXT("Current loads reject a kiln job when its output is full"), InvalidCurrent->MatchesWorld(World));
    InvalidCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    const FKalmalaConstructionSaveRecord DuplicateCurrentRecord = InvalidCurrent->Records[0];
    InvalidCurrent->Records.Add(DuplicateCurrentRecord);
    TestFalse(TEXT("Current loads reject duplicate construction identities"), InvalidCurrent->MatchesWorld(World));

    UKalmalaConstructionSaveGameV2* OverCapCurrent = DuplicateObject<UKalmalaConstructionSaveGameV2>(Save, GetTransientPackage());
    for (int32 Index = 0; Index < UKalmalaConstructionSaveGameV2::MaxRecords; ++Index)
    {
        OverCapCurrent->Records.Add(MakeRecord(*FString::Printf(TEXT("current-%03d"), Index), TEXT("FloorKit")));
    }
    TestFalse(TEXT("Current loads reject combined construction records above the total cap"), OverCapCurrent->MatchesWorld(World));

    UKalmalaConstructionSaveGameV2* AttachmentCap = NewObject<UKalmalaConstructionSaveGameV2>(GetTransientPackage());
    AttachmentCap->InitializeForWorld(World);
    for (int32 Index = 0; Index < UKalmalaConstructionSaveGameV2::MaxStationAttachments; ++Index)
    {
        const FName KitId = Index % 2 == 0 ? FName(TEXT("WorkbenchToolRackKit")) : FName(TEXT("ForgeAnvilKit"));
        TestTrue(TEXT("Station attachment remains within its accepted cap"),
            AttachmentCap->AddStationAttachmentRecord(MakeRecord(*FString::Printf(TEXT("attachment-%02d"), Index), KitId)));
    }
    TestFalse(TEXT("Station attachment cap plus one is rejected"),
        AttachmentCap->AddStationAttachmentRecord(MakeRecord(TEXT("attachment-over-cap"), TEXT("ForgeAnvilKit"))));
    TestEqual(TEXT("Rejected attachment does not change the candidate"),
        AttachmentCap->GetRecords().Num(), UKalmalaConstructionSaveGameV2::MaxStationAttachments);

    FKalmalaCharcoalKilnState Process;
    for (int32 Index = 0; Index < FKalmalaCharcoalKilnState::InputCapacity; ++Index)
        TestTrue(TEXT("Kiln accepts Wood up to its input capacity"), Process.TryAddWood());
    TestFalse(TEXT("Kiln input rejects Wood above capacity"), Process.TryAddWood());
    TestTrue(TEXT("The first active second reserves one Wood and starts a job"), Process.Advance(1.0f));
    TestEqual(TEXT("Active job reserves exactly one Wood"), Process.InputWood, 9);
    TestTrue(TEXT("The job stores its remaining active-game time"), FMath::IsNearlyEqual(Process.JobRemainingSeconds, 29.0f));
    TestTrue(TEXT("The remaining 29 active seconds complete one job"), Process.Advance(29.0f));
    TestEqual(TEXT("A completed job produces exactly one Coal"), Process.OutputCoal, 1);
    FKalmalaCharcoalKilnState FullOutput;
    FullOutput.InputWood = 1;
    FullOutput.OutputCoal = FKalmalaCharcoalKilnState::OutputCapacity;
    TestFalse(TEXT("A full output pauses the queued input"), FullOutput.Advance(5.0f));
    TestEqual(TEXT("Output-full pause retains queued Wood"), FullOutput.InputWood, 1);

    FKalmalaSmelterState Smelt;
    for (int32 Index = 0; Index < FKalmalaSmelterState::InputOreCapacity; ++Index)
        TestTrue(TEXT("Smelter accepts supported ores up to the shared input capacity"),
            Smelt.TryAddOre(Index % 3 == 0 ? EKalmalaSmelterOre::Copper
                : Index % 3 == 1 ? EKalmalaSmelterOre::Tin : EKalmalaSmelterOre::Iron));
    TestFalse(TEXT("Smelter rejects ore above its input capacity"), Smelt.TryAddOre(EKalmalaSmelterOre::Copper));
    for (int32 Index = 0; Index < FKalmalaSmelterState::InputCoalCapacity; ++Index)
        TestTrue(TEXT("Smelter accepts Coal up to its input capacity"), Smelt.TryAddCoal());
    TestFalse(TEXT("Smelter rejects Coal above its input capacity"), Smelt.TryAddCoal());
    TestTrue(TEXT("Smelter reserves the first ore and one Coal when processing starts"), Smelt.Advance(1.0f));
    TestEqual(TEXT("Reserved Smelter ore remains counted against input capacity"), Smelt.GetInputOreCount(), 10);
    TestEqual(TEXT("Reserved Smelter Coal remains counted against input capacity"), Smelt.GetInputCoalCount(), 10);
    TestFalse(TEXT("Reserved inputs keep both Smelter capacities full"), Smelt.TryAddOre(EKalmalaSmelterOre::Copper)
        || Smelt.TryAddCoal());

    FKalmalaSmelterState OrderedSmelt;
    OrderedSmelt.TryAddOre(EKalmalaSmelterOre::Copper);
    OrderedSmelt.TryAddOre(EKalmalaSmelterOre::Tin);
    OrderedSmelt.TryAddCoal();
    OrderedSmelt.TryAddCoal();
    TestTrue(TEXT("Thirty active seconds finish the first queued ore and start the next"), OrderedSmelt.Advance(31.0f));
    TestEqual(TEXT("FIFO processing produces the first matching Copper bar"), OrderedSmelt.OutputCopper, 1);
    TestEqual(TEXT("Next job retains its matching Tin identity"), OrderedSmelt.ActiveJobOre, EKalmalaSmelterOre::Tin);
    TestTrue(TEXT("The next 29 active seconds finish the Tin job"), OrderedSmelt.Advance(29.0f));
    TestEqual(TEXT("Tin output remains separate from Copper"), OrderedSmelt.OutputTin, 1);
    FKalmalaSmelterState FullSmelterOutput;
    FullSmelterOutput.OutputIron = FKalmalaSmelterState::OutputCapacity;
    FullSmelterOutput.TryAddOre(EKalmalaSmelterOre::Iron);
    FullSmelterOutput.TryAddCoal();
    TestFalse(TEXT("A full Smelter output pauses its queued input"), FullSmelterOutput.Advance(5.0f));
    TestEqual(TEXT("Output-full pause retains queued ore"), FullSmelterOutput.GetInputOreCount(), 1);
    return true;
}
#endif
