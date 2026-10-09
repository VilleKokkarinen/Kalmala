#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaItemCatalogue.h"
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
        TEXT("Workbench"), TEXT("Use"));
    TestTrue(TEXT("Prompt names the visible target"), Available.Contains(TEXT("Workbench")));
    TestTrue(TEXT("Prompt names the supported action"), Available.Contains(TEXT("Use")));
    TestFalse(TEXT("Prompt omits keyboard binding labels"), Available.Contains(TEXT("Keyboard:")));
    TestFalse(TEXT("Prompt omits controller binding labels"), Available.Contains(TEXT("Gamepad:")));
    const FKalmalaItemDefinition* Workbench = UKalmalaItemCatalogue::Get()->FindItem(TEXT("WorkbenchKit"));
    TestNotNull(TEXT("Prompt target resolves the Workbench catalogue identity"), Workbench);
    if (Workbench)
    {
        TestEqual(TEXT("Workbench prompt resolves the reviewed catalogue label"), Workbench->DisplayName, FString(TEXT("Workbench")));
        TestEqual(TEXT("Workbench prompt displays the current name and action"),
            UKalmalaInteractionPromptWidget::BuildPromptText(Workbench->DisplayName,
                UKalmalaInteractionPromptWidget::GetConstructionActionName(TEXT("WorkbenchKit"))),
            FString(TEXT("Workbench\nUse")));
    }

    const FString Unavailable = UKalmalaInteractionPromptWidget::BuildPromptText(
        TEXT("Densewood trunk"), TEXT("Chop"), TEXT("No suitable tool available"));
    TestTrue(TEXT("Unavailable target retains its supported action"), Unavailable.Contains(TEXT("Densewood trunk\nChop")));
    TestTrue(TEXT("Unavailable state is explicit"), Unavailable.Contains(TEXT("Unavailable: No suitable tool available")));
    TestFalse(TEXT("Unavailable state omits keyboard and controller bindings"),
        Unavailable.Contains(TEXT("Keyboard:")) || Unavailable.Contains(TEXT("Gamepad:")));
    TestTrue(TEXT("No crosshair candidate clears the prompt"),
        UKalmalaInteractionPromptWidget::BuildPromptText(FString(), FString()).IsEmpty());
    TestTrue(TEXT("Modal input clears the prompt"),
        UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Workbench"), TEXT("Use"), FString(), true).IsEmpty());

    const FString GrindingStoneAction = UKalmalaInteractionPromptWidget::GetConstructionActionName(TEXT("GrindingStoneKit"));
    const FString GrindingStonePrompt = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Grinding Stone"), GrindingStoneAction);
    TestTrue(TEXT("Grinding Stone prompt names its direct Repair All action"),
        GrindingStoneAction == TEXT("Repair all") && GrindingStonePrompt == TEXT("Grinding Stone\nRepair all"));
    TestFalse(TEXT("Grinding Stone prompt does not expose a key binding"),
        GrindingStonePrompt.Contains(TEXT("Keyboard:")) || GrindingStonePrompt.Contains(TEXT("Gamepad:")));
    TestTrue(TEXT("Other construction prompts retain their existing Use action"),
        UKalmalaInteractionPromptWidget::GetConstructionActionName(TEXT("WorkbenchKit")) == TEXT("Use"));

    const FString CampfirePrompt = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Campfire"), TEXT("Add fuel"));
    const FString CampfireNoFuelPrompt = UKalmalaInteractionPromptWidget::BuildPromptText(
        TEXT("Campfire"), TEXT("Add fuel"), TEXT("No raw fuel"));
    const FString CampfireFullPrompt = UKalmalaInteractionPromptWidget::BuildPromptText(
        TEXT("Campfire"), TEXT("Add fuel"), TEXT("Fuel full"));
    TestTrue(TEXT("Campfire Interact prompt names the direct Add fuel action"),
        CampfirePrompt == TEXT("Campfire\nAdd fuel"));
    TestTrue(TEXT("Campfire prompt shows the no-fuel rejection"),
        CampfireNoFuelPrompt.Contains(TEXT("Unavailable: No raw fuel")));
    TestTrue(TEXT("Campfire prompt shows the full-capacity rejection"),
        CampfireFullPrompt.Contains(TEXT("Unavailable: Fuel full")));
    TestFalse(TEXT("Campfire prompt does not expose a key binding"),
        CampfirePrompt.Contains(TEXT("Keyboard:")) || CampfirePrompt.Contains(TEXT("Gamepad:")));

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
    const FString RemappedPrompt = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Workbench"), TEXT("Use"));
    TestTrue(TEXT("Remapping remains available in Options without changing prompt action text"), RemappedPrompt == Available);
    const FString RemappedGrindingStonePrompt = UKalmalaInteractionPromptWidget::BuildPromptText(TEXT("Grinding Stone"),
        UKalmalaInteractionPromptWidget::GetConstructionActionName(TEXT("GrindingStoneKit")));
    TestTrue(TEXT("Interact remapping leaves Grinding Stone action-only prompt unchanged"),
        RemappedGrindingStonePrompt == GrindingStonePrompt);
    TestFalse(TEXT("Remapped key names do not leak into the prompt"),
        RemappedPrompt.Contains(RemappedKeyboard) || RemappedPrompt.Contains(RemappedController));

    if (bHadKeyboardOverride) GConfig->SetString(Section, KeyboardConfigKey, *PreviousKeyboard, GGameUserSettingsIni);
    else GConfig->RemoveKey(Section, KeyboardConfigKey, GGameUserSettingsIni);
    if (bHadControllerOverride) GConfig->SetString(Section, ControllerConfigKey, *PreviousController, GGameUserSettingsIni);
    else GConfig->RemoveKey(Section, ControllerConfigKey, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
    return true;
}
#endif
