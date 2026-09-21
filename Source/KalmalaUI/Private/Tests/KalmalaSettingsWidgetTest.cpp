#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaSettingsWidget.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSettingsWidgetTest, "Kalmala.UI.Settings.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaSettingsWidgetTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Render distance quality keeps its lowest supported level"), UKalmalaSettingsWidget::ClampViewDistanceQuality(-1), 0);
    TestEqual(TEXT("Render distance quality preserves supported levels"), UKalmalaSettingsWidget::ClampViewDistanceQuality(2), 2);
    TestEqual(TEXT("Render distance quality clamps to Unreal scalability's highest supported level"), UKalmalaSettingsWidget::ClampViewDistanceQuality(4), 3);

    TestEqual(TEXT("Master volume clamps below silence"), UKalmalaSettingsWidget::ClampMasterVolume(-0.25f), 0.0f);
    TestEqual(TEXT("Master volume preserves an in-range level"), UKalmalaSettingsWidget::ClampMasterVolume(0.5f), 0.5f);
    TestEqual(TEXT("Master volume clamps above full volume"), UKalmalaSettingsWidget::ClampMasterVolume(1.25f), 1.0f);
    TestEqual(TEXT("Non-finite master volume falls back to full volume"), UKalmalaSettingsWidget::ClampMasterVolume(std::numeric_limits<float>::quiet_NaN()), 1.0f);

    const float OriginalRuntimeVolume = FApp::GetVolumeMultiplier();
    const float OriginalStoredVolume = UKalmalaSettingsWidget::GetStoredMasterVolume();
    UKalmalaSettingsWidget::SetMasterVolume(0.5f);
    TestEqual(TEXT("Master volume persists to the local settings file"), UKalmalaSettingsWidget::GetStoredMasterVolume(), 0.5f);
    TestEqual(TEXT("Master volume immediately applies to local audio"), FApp::GetVolumeMultiplier(), 0.5f);
    UKalmalaSettingsWidget::ToggleAudioMute();
    TestTrue(TEXT("Mute stores zero as the current master volume"), UKalmalaSettingsWidget::IsAudioMuted());
    TestEqual(TEXT("Mute immediately silences local audio"), FApp::GetVolumeMultiplier(), 0.0f);
    UKalmalaSettingsWidget::ToggleAudioMute();
    TestFalse(TEXT("Restore returns audio from mute"), UKalmalaSettingsWidget::IsAudioMuted());
    TestEqual(TEXT("Restore returns the previous master volume"), UKalmalaSettingsWidget::GetStoredMasterVolume(), 0.5f);
    UKalmalaSettingsWidget::SetMasterVolume(OriginalStoredVolume);
    FApp::SetVolumeMultiplier(OriginalRuntimeVolume);
    return true;
}

#endif
