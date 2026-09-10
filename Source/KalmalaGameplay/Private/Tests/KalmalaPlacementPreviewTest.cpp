#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlacementPreview.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaPlacementPreviewTest, "Kalmala.Gameplay.Construction.LocalPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaPlacementPreviewTest::RunTest(const FString& Parameters)
{
    for (const FName Kit : {FName(TEXT("CampfireKit")), FName(TEXT("WorkbenchKit")), FName(TEXT("StorageKit")),
        FName(TEXT("FloorKit")), FName(TEXT("WallKit")), FName(TEXT("RoofKit"))})
        TestTrue(TEXT("Camp and construction kit supports a local preview"), FKalmalaPlacementPreview::IsSupportedKit(Kit));
    for (const FName NotAKit : {FName(TEXT("Wood")), FName(TEXT("Fuel")), FName(TEXT("ConstructionSupply")), FName()})
        TestFalse(TEXT("Materials and ingredients do not create a placement preview"), FKalmalaPlacementPreview::IsSupportedKit(NotAKit));
    const FKalmalaPlacementPreview NullPreview = FKalmalaPlacementPreview::Evaluate(nullptr, nullptr, TEXT("CampfireKit"));
    TestFalse(TEXT("Missing local presentation context cannot claim validity"), NullPreview.bIsValid);
    TestTrue(TEXT("Missing local context reports a textual reason"), NullPreview.Message.Contains(TEXT("waiting"), ESearchCase::IgnoreCase));
    const FKalmalaPlacementPreview Unsupported = FKalmalaPlacementPreview::Evaluate(nullptr, nullptr, TEXT("Wood"));
    TestFalse(TEXT("Unsupported material cannot claim validity"), Unsupported.bIsValid);
    TestTrue(TEXT("Unsupported material reports a textual reason"), Unsupported.Message.Contains(TEXT("select"), ESearchCase::IgnoreCase));
    return true;
}
#endif
