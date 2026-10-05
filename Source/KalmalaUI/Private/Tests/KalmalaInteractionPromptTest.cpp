#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaSettingsWidget.h"
#include "InputCoreTypes.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaInteractionPromptTest, "Kalmala.UI.InteractionPrompt.Presentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaInteractionPromptTest::RunTest(const FString& Parameters)
{
    const FString Available = UKalmalaInteractionPromptWidget::BuildPromptText(
        TEXT("Workbench"), TEXT("Use"), TEXT("F"), TEXT("Gamepad Face Button Right"));
    TestTrue(TEXT("Prompt names the visible target"), Available.Contains(TEXT("Workbench")));
    TestTrue(TEXT("Prompt names the supported action"), Available.Contains(TEXT("Use")));
    TestTrue(TEXT("Prompt shows the keyboard binding"), Available.Contains(TEXT("Keyboard: F")));
    TestTrue(TEXT("Prompt shows the gamepad binding"), Available.Contains(TEXT("Gamepad: Face Button Right")));

    const FString Unavailable = UKalmalaInteractionPromptWidget::BuildPromptText(
        TEXT("Densewood trunk"), TEXT("Chop"), TEXT("E"), TEXT("Gamepad Face Button Bottom"), TEXT("No suitable tool available"));
    TestTrue(TEXT("Unavailable target retains its supported action"), Unavailable.Contains(TEXT("Densewood trunk\nChop")));
    TestTrue(TEXT("Unavailable state is explicit"), Unavailable.Contains(TEXT("Unavailable: No suitable tool available")));
    TestTrue(TEXT("Unavailable state keeps both bindings visible"),
        Unavailable.Contains(TEXT("Keyboard: E")) && Unavailable.Contains(TEXT("Gamepad: Face Button Bottom")));
    TestTrue(TEXT("No crosshair candidate clears the prompt"),
        UKalmalaInteractionPromptWidget::BuildPromptText(FString(), FString(), TEXT("E"), TEXT("A")).IsEmpty());
    TestTrue(TEXT("Modal input clears the prompt"),
        UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Workbench"), TEXT("Use"), TEXT("E"), TEXT("A"), FString(), true).IsEmpty());
    TestTrue(TEXT("Missing bindings make the action unavailable"),
        UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Workbench"), TEXT("Use"),
            TEXT("Not bound"), TEXT("Not bound")).Contains(TEXT("Unavailable: No binding")));

    if (!GConfig)
    {
        AddError(TEXT("Config cache is unavailable for the remapping case."));
        return false;
    }

    const TCHAR* Section = TEXT("/Script/KalmalaUI.KalmalaSettingsWidget");
    const TCHAR* KeyboardConfigKey = TEXT("LocalInput_Interact_Keyboard");
    const TCHAR* ControllerConfigKey = TEXT("LocalInput_Interact_Controller");
    FString PreviousKeyboard;
    FString PreviousController;
    const bool bHadKeyboardOverride = GConfig->GetString(Section, KeyboardConfigKey, PreviousKeyboard, GGameUserSettingsIni);
    const bool bHadControllerOverride = GConfig->GetString(Section, ControllerConfigKey, PreviousController, GGameUserSettingsIni);

    const bool bKeyboardChanged = UKalmalaSettingsWidget::SetLocalInputBinding(TEXT("Interact"), false, EKeys::F);
    const bool bControllerChanged = UKalmalaSettingsWidget::SetLocalInputBinding(
        TEXT("Interact"), true, EKeys::Gamepad_FaceButton_Right);
    const FString RemappedKeyboard = UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), false).ToString();
    const FString RemappedController = UKalmalaSettingsWidget::GetLocalInputBindingLabel(TEXT("Interact"), true).ToString();
    TestTrue(TEXT("Current keyboard remap is read live"), bKeyboardChanged && RemappedKeyboard == EKeys::F.GetDisplayName().ToString());
    TestTrue(TEXT("Current controller remap is read live"),
        bControllerChanged && RemappedController == EKeys::Gamepad_FaceButton_Right.GetDisplayName().ToString());
    const FString Remapped = UKalmalaInteractionPromptWidget::BuildPromptText(
        TEXT("Workbench"), TEXT("Use"), RemappedKeyboard, RemappedController);
    FString ShortRemappedController = RemappedController;
    if (ShortRemappedController.StartsWith(TEXT("Gamepad "))) ShortRemappedController.RightChopInline(8, EAllowShrinking::No);
    TestTrue(TEXT("Prompt formatting reflects the current remapped bindings"),
        Remapped.Contains(FString::Printf(TEXT("Keyboard: %s"), *RemappedKeyboard))
        && Remapped.Contains(FString::Printf(TEXT("Gamepad: %s"), *ShortRemappedController)));

    if (bHadKeyboardOverride) GConfig->SetString(Section, KeyboardConfigKey, *PreviousKeyboard, GGameUserSettingsIni);
    else GConfig->RemoveKey(Section, KeyboardConfigKey, GGameUserSettingsIni);
    if (bHadControllerOverride) GConfig->SetString(Section, ControllerConfigKey, *PreviousController, GGameUserSettingsIni);
    else GConfig->RemoveKey(Section, ControllerConfigKey, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    return true;
}
#endif
