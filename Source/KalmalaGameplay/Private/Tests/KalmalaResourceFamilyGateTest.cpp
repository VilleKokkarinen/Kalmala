#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaResourceFamilyGateTest, "Kalmala.Gameplay.Harvest.ResourceFamilyGates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaResourceFamilyGateTest::RunTest(const FString& Parameters)
{
    const FName Sources[] = { TEXT("meadows-birch-bark"), TEXT("elderwood-resinwood"), TEXT("meadows-aspen-trunk"),
        TEXT("meadows-birch-trunk"), TEXT("elderwood-ironheart-trunk"), TEXT("mountains-slate-vein"),
        TEXT("mire-bog-iron"), TEXT("mire-peat-amber-seam"), TEXT("tundra-frost-salt-deposit"),
        TEXT("elderwood-copper-vein"), TEXT("lakes-tin-shore-gravel") };
    const FName Items[] = { TEXT("StoneAxe"), TEXT("BronzeAxe"), TEXT("IronAxe"),
        TEXT("StonePick"), TEXT("BronzePick"), TEXT("IronPick") };
    for (const FName Item : Items)
    {
        const auto* Definition = FKalmalaToolLifecycleContract::FindDefinition(Item);
        if (!TestNotNull(TEXT("Equipment resolves"), Definition)) continue;
        for (const FName Source : Sources)
        {
            FKalmalaToolServerSelection Selection;
            if (!TestTrue(TEXT("Resource has a server selection"), FKalmalaToolLifecycleContract::BuildServerSelection(Source, Selection))) continue;
            const bool bFamilyMatches = Definition->Action == Selection.Action;
            TestEqual(TEXT("Family compatibility ignores tier"),
                FKalmalaToolLifecycleContract::IsResourceFamilyCompatible(Definition->Kind, Selection.Action), bFamilyMatches);
            for (const int32 Level : { 1, 10, 20 })
            {
                const bool bExpected = bFamilyMatches
                    && FKalmalaToolLifecycleContract::GetToolTier(Definition->Kind, Level) >= Selection.MinimumToolTier
                    && (Selection.Action != EKalmalaToolAction::Mining
                        || static_cast<uint8>(FKalmalaToolLifecycleContract::GetMaximumMineableResourceTier(Definition->Kind))
                            >= static_cast<uint8>(Selection.ResourceTier));
                TestEqual(FString::Printf(TEXT("%s level %d against %s"), *Item.ToString(), Level, *Source.ToString()),
                    FKalmalaToolLifecycleContract::IsToolSuitableForSelection(*Definition, Level, Selection), bExpected);
                if (!bFamilyMatches)
                {
                    FKalmalaToolDefinition Forged = *Definition;
                    Forged.Action = Selection.Action;
                    TestFalse(TEXT("Changing action cannot grant another family's resource access"),
                        FKalmalaToolLifecycleContract::IsToolSuitableForSelection(Forged, Level, Selection));
                }
            }
        }
    }
    for (const auto Kind : { EKalmalaToolKind::StoneKnife, EKalmalaToolKind::IronSpear,
        EKalmalaToolKind::IronMace, EKalmalaToolKind::ConstructionHammer, EKalmalaToolKind::None })
    {
        TestFalse(TEXT("Other families cannot chop"), FKalmalaToolLifecycleContract::IsResourceFamilyCompatible(Kind, EKalmalaToolAction::Woodcutting));
        TestFalse(TEXT("Other families cannot mine"), FKalmalaToolLifecycleContract::IsResourceFamilyCompatible(Kind, EKalmalaToolAction::Mining));
    }
    return true;
}
#endif
