#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCatalogueIconLibrary.h"
#include "KalmalaIconWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaToolCatalogue.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaEquipmentIconTest,
    "Kalmala.UI.CatalogueIcons.EquipmentCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaEquipmentIconTest::RunTest(const FString& Parameters)
{
    const UKalmalaItemCatalogue* Items = UKalmalaItemCatalogue::Get();
    const UKalmalaToolCatalogue* Tools = UKalmalaToolCatalogue::Get();
    if (!TestNotNull(TEXT("Equipment item catalogue loads"), Items)
        || !TestNotNull(TEXT("Main Hand profile catalogue loads"), Tools)) return false;
    TestTrue(TEXT("Equipment items validate"), Items->IsValidCatalogue());
    TestTrue(TEXT("Main Hand profiles validate"), Tools->IsValidCatalogue(Items));

    TSet<FName> Identities;
    TSet<FString> TexturePaths;
    const auto CheckIcon = [&](const FName ItemId)
    {
        const FString Label = ItemId.ToString();
        TestFalse(Label + TEXT(" has a unique equipment identity"), Identities.Contains(ItemId));
        Identities.Add(ItemId);
        EKalmalaIcon Icon = EKalmalaIcon::Unknown;
        int32 Variant = 0;
        TestTrue(Label + TEXT(" has a known icon mapping"),
            UKalmalaIconWidget::FindCatalogueIcon(ItemId, Icon, Variant)
                && Icon != EKalmalaIcon::Unknown);

        // Stone tools retain their original imported texture names.
        const FString Asset = ItemId == TEXT("StoneAxe") ? TEXT("FieldHatchet")
            : ItemId == TEXT("StoneKnife") ? TEXT("ReedKnife") : Label;
        FSoftObjectPath Path;
        TestTrue(Label + TEXT(" resolves an equipment texture"),
            FKalmalaCatalogueIconLibrary::GetTextureObjectPath(ItemId, Path));
        TestEqual(Label + TEXT(" retains its imported asset identity"), Path.ToString(),
            FString::Printf(TEXT("/Game/Kalmala/UI/Icons/Items/%s.%s"), *Asset, *Asset));
        TestFalse(Label + TEXT(" has distinct equipment artwork"), TexturePaths.Contains(Path.ToString()));
        TexturePaths.Add(Path.ToString());
        TestNotNull(Label + TEXT(" imported texture loads"), FKalmalaCatalogueIconLibrary::LoadTexture(ItemId));
        auto* Widget = NewObject<UKalmalaIconWidget>();
        Widget->SetCatalogueIcon(ItemId);
        TestTrue(Label + TEXT(" displays through the shared inventory icon widget"), Widget->HasCatalogueTexture());
    };

    int32 WearableCount = 0;
    for (const FKalmalaItemDefinition& Item : Items->Items)
    {
        if (Item.GearSlot.IsEmpty()) continue;
        ++WearableCount;
        TestEqual(Item.ItemId.ToString() + TEXT(" occupies one physical inventory unit"), Item.MaxStack, 1);
        CheckIcon(Item.ItemId);
    }
    TestEqual(TEXT("All six initial wearable families have icon coverage"), WearableCount, 6);
    TestEqual(TEXT("All seventeen current Main Hand profiles have icon coverage"), Tools->Profiles.Num(), 17);
    for (const FKalmalaToolProfileData& Profile : Tools->Profiles) CheckIcon(Profile.ItemId);
    return true;
}
#endif
