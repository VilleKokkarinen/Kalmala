#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaEquipmentSaveTest,
    "Kalmala.Gameplay.Inventory.EquipmentCurrentSaveFormat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaEquipmentSaveTest::RunTest(const FString& Parameters)
{
    const UKalmalaToolCatalogue* Tools = UKalmalaToolCatalogue::Get();
    if (!TestNotNull(TEXT("Equipment profiles load"), Tools)) return false;
    if (!TestEqual(TEXT("Fixture covers all current Main Hand profiles"), Tools->Profiles.Num(), 16)) return false;

    FKalmalaWorldGenerationConfig World;
    World.WorldSeed = 418;
    const FString Owner(TEXT("provider:equipment-save-owner"));
    auto* Save = NewObject<UKalmalaPlayerDiscoverySaveGameV2>();
    Save->InitializeForPlayer(World, Owner);
    TArray<FKalmalaPlayerToolSaveRecord> Expected;
    for (const FKalmalaToolProfileData& Profile : Tools->Profiles)
    {
        // Same-item copies must retain independent GUIDs, progression and wear.
        for (const int32 Level : {1, 20})
        {
            const int32 Maximum = FKalmalaToolLifecycleContract::GetMaxDurability(Profile.ItemId, Level);
            if (!TestTrue(Profile.ItemId.ToString() + TEXT(" has authored durability"), Maximum > 0)) return false;
            FKalmalaPlayerToolSaveRecord Record;
            Record.ItemId = Profile.ItemId;
            Record.ToolLevel = Level;
            Record.Condition = Level == 1 ? Maximum : 0;
            Record.InstanceGuid = FGuid::NewGuid();
            Expected.Add(Record);
        }
    }
    if (!TestTrue(TEXT("All physical equipment copies fit the existing format"),
        Save->TryReplaceToolRecords(Expected))) return false;

    const auto CheckRecords = [&](const UKalmalaPlayerDiscoverySaveGameV2* Candidate)
    {
        if (!TestEqual(TEXT("Every physical copy is retained"), Candidate->GetToolRecords().Num(), Expected.Num())) return;
        for (int32 Index = 0; Index < Expected.Num(); ++Index)
        {
            const auto& Actual = Candidate->GetToolRecords()[Index];
            const auto& Original = Expected[Index];
            const FString Label = FString::Printf(TEXT("%s copy level %d"), *Original.ItemId.ToString(), Original.ToolLevel);
            TestEqual(Label + TEXT(" keeps canonical ItemId"), Actual.ItemId, Original.ItemId);
            TestEqual(Label + TEXT(" keeps level"), Actual.ToolLevel, Original.ToolLevel);
            TestEqual(Label + TEXT(" keeps remaining condition"), Actual.Condition, Original.Condition);
            TestTrue(Label + TEXT(" keeps its exact GUID without backfill"), Actual.InstanceGuid == Original.InstanceGuid);
        }
    };
    const auto RoundTrip = [&](UKalmalaPlayerDiscoverySaveGameV2* Candidate)
    {
        TArray<uint8> Bytes;
        if (!TestTrue(TEXT("Equipment candidate serializes in memory"),
            UGameplayStatics::SaveGameToMemory(Candidate, Bytes))) return static_cast<UKalmalaPlayerDiscoverySaveGameV2*>(nullptr);
        return Cast<UKalmalaPlayerDiscoverySaveGameV2>(UGameplayStatics::LoadGameFromMemory(Bytes));
    };
    auto* Reloaded = RoundTrip(Save);
    if (!TestNotNull(TEXT("Current equipment save reloads with its type"), Reloaded)) return false;
    TestTrue(TEXT("Reloaded equipment matches the current world and owner"), Reloaded->MatchesPlayer(World, Owner));
    CheckRecords(Reloaded);
    TestFalse(TEXT("Equipment save cannot be adopted by a different player"),
        Reloaded->MatchesPlayer(World, TEXT("provider:other-owner")));
    auto OtherWorld = World;
    ++OtherWorld.WorldSeed;
    TestFalse(TEXT("Equipment save cannot be adopted by a different world"), Reloaded->MatchesPlayer(OtherWorld, Owner));
    CheckRecords(Reloaded);

    // Failed complete writes must preserve all physical copies, not only a count.
    auto Invalid = Expected;
    Invalid.Last().InstanceGuid = Invalid[0].InstanceGuid;
    TestFalse(TEXT("Duplicate GUID replacement is rejected atomically"), Reloaded->TryReplaceToolRecords(Invalid));
    CheckRecords(Reloaded);
    Invalid = Expected;
    while (Invalid.Num() <= UKalmalaPlayerDiscoverySaveGameV2::MaxInventorySlots)
    {
        auto Extra = Expected[0];
        Extra.InstanceGuid = FGuid::NewGuid();
        Invalid.Add(Extra);
    }
    TestFalse(TEXT("Forty-first physical tool remains outside the current save contract"),
        Reloaded->TryReplaceToolRecords(Invalid));
    CheckRecords(Reloaded);

    // Exercise serialized incompatible versions without a legacy conversion path.
    const FIntProperty* Schema = FindFProperty<FIntProperty>(Save->GetClass(), TEXT("SchemaVersion"));
    if (!TestNotNull(TEXT("Current save exposes its reflected schema gate"), Schema)) return false;
    for (const int32 Version : {0, 1, UKalmalaPlayerDiscoverySaveGameV2::SchemaVersionValue + 1})
    {
        auto* Incompatible = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Save, GetTransientPackage());
        Schema->SetPropertyValue_InContainer(Incompatible, Version);
        auto* Rejected = RoundTrip(Incompatible);
        if (!TestNotNull(TEXT("Incompatible candidate deserializes for validation"), Rejected)) return false;
        TestFalse(TEXT("Incompatible equipment save fails current-format acceptance"), Rejected->MatchesPlayer(World, Owner));
        TestFalse(TEXT("Rejected equipment save cannot accept replacement writes"), Rejected->TryReplaceToolRecords(Expected));
        TestEqual(TEXT("Validation does not rewrite an incompatible schema"), Schema->GetPropertyValue_InContainer(Rejected), Version);
        CheckRecords(Rejected);
    }

    const FArrayProperty* Records = FindFProperty<FArrayProperty>(Save->GetClass(), TEXT("ToolRecords"));
    if (!TestNotNull(TEXT("Current save exposes its physical record array"), Records)) return false;
    const FStructProperty* RecordType = CastField<FStructProperty>(Records->Inner);
    if (!TestTrue(TEXT("Fixture uses the current equipment save record type"),
        RecordType && RecordType->Struct == FKalmalaPlayerToolSaveRecord::StaticStruct())) return false;
    auto* MissingGuid = DuplicateObject<UKalmalaPlayerDiscoverySaveGameV2>(Save, GetTransientPackage());
    auto& Corrupted = *Records->ContainerPtrToValuePtr<TArray<FKalmalaPlayerToolSaveRecord>>(MissingGuid);
    Corrupted[0].InstanceGuid.Invalidate();
    auto* RejectedGuid = RoundTrip(MissingGuid);
    if (!TestNotNull(TEXT("Missing-GUID candidate reloads for validation"), RejectedGuid)) return false;
    TestFalse(TEXT("Current-format saves require pre-existing physical GUIDs"), RejectedGuid->MatchesPlayer(World, Owner));
    TestFalse(TEXT("Invalid loaded identity blocks replacement writes"), RejectedGuid->TryReplaceToolRecords(Expected));
    TestFalse(TEXT("Rejected loaded identity is never backfilled"), RejectedGuid->GetToolRecords()[0].InstanceGuid.IsValid());
    TestEqual(TEXT("Missing-GUID rejection preserves remaining record data"),
        RejectedGuid->GetToolRecords()[0].Condition, Expected[0].Condition);
    CheckRecords(Save);
    return true;
}
#endif
