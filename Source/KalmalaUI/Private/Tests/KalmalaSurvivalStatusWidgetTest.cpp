#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaSurvivalStatusWidget.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaSurvivalStatusWidgetTest,
    "Kalmala.UI.SurvivalStatus.LocalPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaSurvivalStatusWidgetTest::RunTest(const FString& Parameters)
{
    FKalmalaSurvivalStatusSnapshot Snapshot;
    TestEqual(TEXT("Unpossessed local view waits for its pawn"),
        UKalmalaSurvivalStatusWidget::BuildStatusText(Snapshot), FString(TEXT("Waiting for player character.")));

    Snapshot.bHasCharacter = true;
    Snapshot.ServerTimeSeconds = 45.0f;

    FKalmalaPlayerStatusEntry& Wet = Snapshot.Statuses.AddDefaulted_GetRef();
    Wet.StatusId = UKalmalaPlayerStatusComponent::WetStatusId;
    Wet.RemainingSeconds = 31.2f;

    FKalmalaPlayerStatusEntry& Meal = Snapshot.Statuses.AddDefaulted_GetRef();
    Meal.StatusId = UKalmalaPlayerStatusComponent::SteadyMealStatusId;
    Meal.RemainingSeconds = 74.0f;

    FKalmalaPlayerStatusEntry& Expired = Snapshot.Statuses.AddDefaulted_GetRef();
    Expired.StatusId = UKalmalaPlayerStatusComponent::WetStatusId;
    Expired.RemainingSeconds = 0.0f;

    Snapshot.bHasWeatherState = true;
    Snapshot.Weather.ServerStartTimeSeconds = 15.0f;
    Snapshot.Weather.DurationSeconds = 120.0f;
    Snapshot.Weather.PrecipitationIntensity = 0.8f;
    Snapshot.Weather.FogIntensity = 0.4f;
    Snapshot.Weather.WindStrength = 0.7f;
    Snapshot.Weather.RefreshActivityLevel();

    Snapshot.Exposure.ColdIntensity = 0.5f;
    Snapshot.Exposure.Warmth = 20.0f;
    Snapshot.Exposure.HeatIntensity = 0.0f;

    Snapshot.ActiveSupportEffect = EKalmalaSupportEffect::HearthShield;
    Snapshot.ActiveSupportEffectExpiry = 55.0f;
    Snapshot.HearthShieldStrength = 25.0f;

    const FString StatusText = UKalmalaSurvivalStatusWidget::BuildStatusText(Snapshot);
    TestTrue(TEXT("Wet row includes an explicit shape, timer and modifiers"),
        StatusText.Contains(TEXT("○ STATUS · WET · 32 s · movement −8% / stamina cost +15%")));
    TestTrue(TEXT("Wet row identifies possible sources and campfire recovery"),
        StatusText.Contains(TEXT("Source: exposed rain or water · Recovery: warm up near a lit campfire.")));
    TestTrue(TEXT("Food row shows its timer, finite benefit and non-stacking rule"),
        StatusText.Contains(TEXT("◆ FOOD · STEADY MEAL · 74 s · stamina cost −10%"))
        && StatusText.Contains(TEXT("prepared food · Recovery: wait for expiry; meals do not stack or replace.")));
    TestTrue(TEXT("Weather row uses replicated intensity values and interval time"),
        StatusText.Contains(TEXT("▲ HAZARD · WEATHER · rain 80% / fog 40% / wind 70% / storm 56% · 90 s")));
    TestTrue(TEXT("Cold row shows signal, warmth, gameplay recovery and guidance"),
        StatusText.Contains(TEXT("◇ HAZARD · COLD · signal 50% / warmth 20% / stamina recovery 90% · ongoing"))
        && StatusText.Contains(TEXT("Recovery: shelter or a lit hearth restores warmth.")));
    TestTrue(TEXT("Shield row shows its marker, server timer and remaining absorption"),
        StatusText.Contains(TEXT("□ SUPPORT · HEARTH SHIELD · 10 s · absorption 25"))
        && StatusText.Contains(TEXT("Source: accepted support magic · Recovery: effect expires on server time.")));
    TestFalse(TEXT("Expired status is not presented"), StatusText.Contains(TEXT("WET · 0 s")));
    TArray<FString> StatusRows;
    StatusText.ParseIntoArrayLines(StatusRows);
    int32 WetRowCount = 0;
    for (const FString& Row : StatusRows)
    {
        if (Row.StartsWith(TEXT("○ STATUS · WET"))) ++WetRowCount;
    }
    TestEqual(TEXT("Duplicate Wet input still resolves to one active row"), WetRowCount, 1);

    FKalmalaSurvivalStatusSnapshot EmptySnapshot;
    EmptySnapshot.bHasCharacter = true;
    TestEqual(TEXT("No active state has a readable local empty label"),
        UKalmalaSurvivalStatusWidget::BuildStatusText(EmptySnapshot), FString(TEXT("○ STATUS · NO ACTIVE EFFECTS")));

    UKalmalaSurvivalStatusWidget* Widget = NewObject<UKalmalaSurvivalStatusWidget>();
    Widget->ConfigureViewportPlacement();
    const FGameViewportWidgetSlot Slot = UGameViewportSubsystem::Get()->GetWidgetSlot(Widget);
    TestTrue(TEXT("Status column and its padding stay left of the centered 1280x720 arrival card"),
        24.0f + UKalmalaSurvivalStatusWidget::StatusPanelContentWidth + 24.0f <= 480.0f);
    TestTrue(TEXT("Status stays anchored to the local viewport's lower-left"), Slot.Anchors == FAnchors(0.0f, 1.0f));
    TestEqual(TEXT("Status panel is inset from the left edge"), Slot.Offsets.Left, 24.0f);
    TestEqual(TEXT("Status panel is inset from the bottom edge"), Slot.Offsets.Top, -24.0f);
    TestEqual(TEXT("Status panel does not take focus"), Widget->IsFocusable(), false);
    UGameViewportSubsystem::Get()->RemoveWidget(Widget);
    return true;
}

#endif
