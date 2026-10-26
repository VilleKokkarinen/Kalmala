#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaBiomeContentContract.h"
#include "KalmalaForestResourceContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaForestResourceContractTest,
    "Kalmala.Gameplay.Harvest.ForestResourceIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaForestResourceContractTest::RunTest(const FString& Parameters)
{
    struct FExpectedTreeKind
    {
        EKalmalaForestTreeKind Kind;
        FName SourceId;
        FName PresentationId;
    };
    const FExpectedTreeKind Expected[] = {
        {EKalmalaForestTreeKind::MeadowsBirch, TEXT("meadows-birch-trunk"), TEXT("birch-trunk-harvest")},
        {EKalmalaForestTreeKind::MeadowsAspen, TEXT("meadows-aspen-trunk"), TEXT("aspen-trunk-harvest")},
        {EKalmalaForestTreeKind::ElderwoodIronheart, TEXT("elderwood-ironheart-trunk"), TEXT("ironheart-trunk-harvest")},
        {EKalmalaForestTreeKind::FreezingTundraBirch, TEXT("meadows-birch-trunk"), TEXT("birch-trunk-harvest")}
    };
    for (const auto& Entry : Expected)
    {
        const FName SourceId = FKalmalaForestResourceContract::GetSourceId(Entry.Kind);
        TestEqual(TEXT("Tree kind selects its explicit existing timber source"), SourceId, Entry.SourceId);
        TestEqual(TEXT("Tree kind uses the expected existing presentation"),
            FKalmalaBiomeContentContract::GetGatheringPresentationId(SourceId), Entry.PresentationId);
    }
    TestEqual(TEXT("Tundra trees never inherit the Elderwood-only source"),
        FKalmalaForestResourceContract::GetSourceId(EKalmalaForestTreeKind::FreezingTundraBirch),
        FKalmalaForestResourceContract::GetSourceId(EKalmalaForestTreeKind::MeadowsBirch));
    TestTrue(TEXT("Unknown kinds cannot select a resource"),
        FKalmalaForestResourceContract::GetSourceId(static_cast<EKalmalaForestTreeKind>(255)).IsNone());
    return true;
}
#endif
