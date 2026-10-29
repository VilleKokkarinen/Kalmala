#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWorldListWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWorldListAvailabilityTest, "Kalmala.UI.WorldList.Availability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWorldListAvailabilityTest::RunTest(const FString& Parameters)
{
    const FString Unsupported(UKalmalaWorldListWidget::EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability::Unavailable));
    const FString Unknown(UKalmalaWorldListWidget::EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability::Unknown));
    const FString Available(UKalmalaWorldListWidget::EmptyOrUnavailableMessage(EKalmalaWorldMetadataAvailability::Available));
    TestTrue(TEXT("Unsupported listing is explicitly reported"), Unsupported.Contains(TEXT("not supported")));
    TestTrue(TEXT("Unknown availability is not presented as empty"), Unknown.Contains(TEXT("could not be determined")));
    TestFalse(TEXT("Available metadata alone does not prove an empty world list"), Available.Contains(TEXT("No worlds")));
    TestTrue(TEXT("The runtime adapter reports unsupported enumeration"),
        FKalmalaWorldDescriptorAdapter::WorldEnumerationAvailability() == EKalmalaWorldMetadataAvailability::Unavailable);
    return true;
}
#endif
