#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaStatusHotbarWidget.h"
#include "KalmalaSurvivalStatusWidget.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStatusHotbarTest, "Kalmala.UI.StatusHotbar.SnapshotAndLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaStatusHotbarTest::RunTest(const FString&)
{
    FKalmalaSurvivalStatusSnapshot S;
    TestEqual(TEXT("No pawn draws no chrome"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 0);
    S.bHasCharacter = true;
    TestEqual(TEXT("Empty is empty"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 0);
    S.Statuses.Add({UKalmalaPlayerStatusComponent::SteadyMealStatusId, 74});
    S.Statuses.Add({UKalmalaPlayerStatusComponent::WetStatusId, 31.2f});
    S.Statuses.Add({UKalmalaPlayerStatusComponent::WetStatusId, 80});
    S.Exposure.HeatIntensity = .6f; S.Exposure.ColdIntensity = .5f; S.Exposure.Warmth = 20;
    S.ActiveSupportEffect = EKalmalaSupportEffect::HearthShield; S.ActiveSupportEffectExpiry = 80; S.ServerTimeSeconds = 45;
    S.bHasWeatherState = true; S.Weather.ServerStartTimeSeconds = 15;
    S.Weather.PrecipitationIntensity = .9f; S.Weather.WindStrength = .9f; S.Weather.RefreshActivityLevel();
    const auto Entries = UKalmalaStatusHotbarWidget::BuildEntries(S);
    TestEqual(TEXT("Simultaneous effects deduplicate"), Entries.Num(), 6);
    if (Entries.Num() != 6) return false;
    TestEqual(TEXT("Stable order"), Entries[0].Id, UKalmalaPlayerStatusComponent::WetStatusId);
    TestEqual(TEXT("Replicated duration rounded up"), Entries[0].Duration, FString(TEXT("32 s")));
    TestEqual(TEXT("Untimed heat"), Entries[2].Duration, FString(TEXT("ongoing")));
    TestEqual(TEXT("Storm distinct"), Entries[5].Icon, EKalmalaIcon::Storm);
    TestEqual(TEXT("Server weather interval"), Entries[5].Duration, FString(TEXT("90 s")));
    S.Statuses.Reset(); S.ActiveSupportEffectExpiry = 45;
    TestEqual(TEXT("Expired statuses removed together"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 3);
    S.bHasWeatherState = false; S.Exposure.HeatIntensity = 0; S.Exposure.ColdIntensity = 0;
    TestEqual(TEXT("Last expiry returns to empty"), UKalmalaStatusHotbarWidget::BuildEntries(S).Num(), 0);
    for (const FVector2D View : {FVector2D(1024,768), FVector2D(1280,720), FVector2D(2560,1080)})
        for (const int32 Scale : {100,125,150})
        {
            const auto Size = UKalmalaStatusHotbarWidget::CalculateSize(6, Scale, View);
            TestTrue(TEXT("Below minimap and inside viewport"), Size.X + 48 <= View.X && Size.Y + 244 + 24 <= View.Y);
            TestEqual(TEXT("Empty has zero height"), UKalmalaStatusHotbarWidget::CalculateSize(0,Scale,View).Y, 0.0);
        }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStatusHotbarTransitionsTest, "Kalmala.UI.StatusHotbar.Transitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaStatusHotbarTransitionsTest::RunTest(const FString&)
{
    const auto MakeEntry = [](const FName Id, const float RefreshValue,
        const EKalmalaStatusRefreshPolicy RefreshPolicy, const FString& Name)
    {
        FKalmalaStatusHotbarEntry Entry;
        Entry.Id = Id;
        Entry.Name = Name;
        Entry.Duration = TEXT("30 s");
        Entry.Icon = EKalmalaIcon::Drop;
        Entry.RefreshValue = RefreshValue;
        Entry.RefreshPolicy = RefreshPolicy;
        return Entry;
    };

    const FKalmalaStatusHotbarEntry Wet = MakeEntry(TEXT("Wet"), 18.0f,
        EKalmalaStatusRefreshPolicy::RemainingIncreased, TEXT("Wet"));
    TestEqual(TEXT("A reconnect or first owner snapshot is a silent baseline"),
        UKalmalaStatusHotbarWidget::BuildTransitions({}, { Wet }, false).Num(), 0);

    const auto Started = UKalmalaStatusHotbarWidget::BuildTransitions({}, { Wet }, true);
    TestEqual(TEXT("A new authoritative status produces one start cue"), Started.Num(), 1);
    if (Started.Num() == 1)
    {
        TestEqual(TEXT("New status cue kind"), Started[0].Kind, EKalmalaStatusCueKind::Started);
        TestEqual(TEXT("New status keeps its identity"), Started[0].Entry.Id, Wet.Id);
    }

    const FKalmalaStatusHotbarEntry Countdown = MakeEntry(TEXT("Wet"), 17.2f,
        EKalmalaStatusRefreshPolicy::RemainingIncreased, TEXT("Wet"));
    TestEqual(TEXT("A decreasing replicated countdown does not replay a refresh cue"),
        UKalmalaStatusHotbarWidget::BuildTransitions({ Wet }, { Countdown }, true).Num(), 0);

    const FKalmalaStatusHotbarEntry RefreshedWet = MakeEntry(TEXT("Wet"), 29.5f,
        EKalmalaStatusRefreshPolicy::RemainingIncreased, TEXT("Wet"));
    const auto Refreshed = UKalmalaStatusHotbarWidget::BuildTransitions({ Wet }, { RefreshedWet }, true);
    TestEqual(TEXT("An authoritative remaining-time reset is a refresh"), Refreshed.Num(), 1);
    if (Refreshed.Num() == 1)
    {
        TestEqual(TEXT("Refresh cue kind"), Refreshed[0].Kind, EKalmalaStatusCueKind::Refreshed);
    }

    const FKalmalaStatusHotbarEntry Weather = MakeEntry(TEXT("Weather"), 120.0f,
        EKalmalaStatusRefreshPolicy::AuthorityStampChanged, TEXT("Calm weather"));
    const FKalmalaStatusHotbarEntry WeatherCountdown = MakeEntry(TEXT("Weather"), 120.0f,
        EKalmalaStatusRefreshPolicy::AuthorityStampChanged, TEXT("Calm weather"));
    TestEqual(TEXT("Weather countdown leaves its authoritative interval stamp unchanged"),
        UKalmalaStatusHotbarWidget::BuildTransitions({ Weather }, { WeatherCountdown }, true).Num(), 0);
    const FKalmalaStatusHotbarEntry NewWeather = MakeEntry(TEXT("Weather"), 240.0f,
        EKalmalaStatusRefreshPolicy::AuthorityStampChanged, TEXT("Calm weather"));
    const auto WeatherRefresh = UKalmalaStatusHotbarWidget::BuildTransitions({ Weather }, { NewWeather }, true);
    TestEqual(TEXT("A new server weather interval is a refresh"), WeatherRefresh.Num(), 1);
    if (WeatherRefresh.Num() == 1)
    {
        TestEqual(TEXT("Weather refresh cue kind"), WeatherRefresh[0].Kind, EKalmalaStatusCueKind::Refreshed);
    }

    const FKalmalaStatusHotbarEntry Support = MakeEntry(TEXT("Shield"), 80.0f,
        EKalmalaStatusRefreshPolicy::ExpiryIncreased, TEXT("Hearth shield"));
    const FKalmalaStatusHotbarEntry SupportCountdown = MakeEntry(TEXT("Shield"), 79.0f,
        EKalmalaStatusRefreshPolicy::ExpiryIncreased, TEXT("Hearth shield"));
    TestEqual(TEXT("Decreasing authoritative support expiry is not a refresh"),
        UKalmalaStatusHotbarWidget::BuildTransitions({ Support }, { SupportCountdown }, true).Num(), 0);
    const FKalmalaStatusHotbarEntry SupportExtended = MakeEntry(TEXT("Shield"), 90.0f,
        EKalmalaStatusRefreshPolicy::ExpiryIncreased, TEXT("Hearth shield"));
    const auto SupportRefresh = UKalmalaStatusHotbarWidget::BuildTransitions({ Support }, { SupportExtended }, true);
    TestEqual(TEXT("Extended authoritative support expiry is a refresh"), SupportRefresh.Num(), 1);
    if (SupportRefresh.Num() == 1)
    {
        TestEqual(TEXT("Support refresh cue kind"), SupportRefresh[0].Kind, EKalmalaStatusCueKind::Refreshed);
    }

    const auto Ended = UKalmalaStatusHotbarWidget::BuildTransitions({ Wet }, {}, true);
    TestEqual(TEXT("Removal produces one explicit end cue"), Ended.Num(), 1);
    if (Ended.Num() == 1)
    {
        TestEqual(TEXT("End cue kind"), Ended[0].Kind, EKalmalaStatusCueKind::Ended);
        TestEqual(TEXT("End cue retains the ended icon identity"), Ended[0].Entry.Id, Wet.Id);
    }

    TestFalse(TEXT("Refresh coalesces into a visible start cue"),
        UKalmalaStatusHotbarWidget::ShouldReplaceActiveCue(EKalmalaStatusCueKind::Started, EKalmalaStatusCueKind::Refreshed));
    TestTrue(TEXT("End supersedes an earlier cue"),
        UKalmalaStatusHotbarWidget::ShouldReplaceActiveCue(EKalmalaStatusCueKind::Started, EKalmalaStatusCueKind::Ended));
    TestTrue(TEXT("A new start supersedes an ended cue"),
        UKalmalaStatusHotbarWidget::ShouldReplaceActiveCue(EKalmalaStatusCueKind::Ended, EKalmalaStatusCueKind::Started));
    TestTrue(TEXT("Reduced motion keeps a static cue visible until its themed expiry"),
        UKalmalaStatusHotbarWidget::CalculateCueOpacity(0.8f, 1.25f, true, true) == 1.0f);
    const float AnimatedOpacity = UKalmalaStatusHotbarWidget::CalculateCueOpacity(0.4f, 1.25f, true, false);
    TestTrue(TEXT("Animated cue remains visible during its brief lifetime"), AnimatedOpacity > 0.0f && AnimatedOpacity < 1.0f);
    TestEqual(TEXT("Cue clears at the themed expiry"),
        UKalmalaStatusHotbarWidget::CalculateCueOpacity(1.25f, 1.25f, true, false), 0.0f);
    TestEqual(TEXT("Theme motion-off path is also static"),
        UKalmalaStatusHotbarWidget::CalculateCueOpacity(0.4f, 1.25f, false, false), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaCatalogueIconTest, "Kalmala.UI.CatalogueIcons.CompleteCoverage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FKalmalaCatalogueIconTest::RunTest(const FString&)
{
    TSet<int32> Shapes;
    const auto Check = [&](FName Id)
    {
        EKalmalaIcon Icon; int32 Variant;
        TestTrue(*Id.ToString(), UKalmalaIconWidget::FindCatalogueIcon(Id, Icon, Variant));
        TestTrue(TEXT("Known original shape"), Icon != EKalmalaIcon::Unknown);
        const int32 Key = static_cast<int32>(Icon)*8 + Variant;
        TestFalse(TEXT("Distinct item/tool assignment"), Shapes.Contains(Key)); Shapes.Add(Key);
    };
    const auto* Items = UKalmalaItemCatalogue::Get();
    TestEqual(TEXT("Audited inventory count"), Items->Items.Num(), 42);
    for (const auto& Item : Items->Items) Check(Item.ItemId);
    for (const auto& Tool : FKalmalaToolLifecycleContract::GetDefinitions()) Check(Tool.ToolId);
    for (const auto& Tool : FKalmalaToolLifecycleContract::GetTieredAxeDefinitions()) Check(Tool.ToolId);
    Check(FKalmalaToolLifecycleContract::GetConstructionHammerDefinition().ToolId);
    const auto* Recipes = UKalmalaRecipeCatalogue::Get();
    TestEqual(TEXT("Audited recipe/build count"), Recipes->Recipes.Num(), 19);
    for (const auto& Recipe : Recipes->Recipes)
    {
        EKalmalaIcon Icon; int32 Variant;
        TestTrue(*Recipe.RecipeId.ToString(), UKalmalaIconWidget::FindCatalogueIcon(Recipe.Output, Icon, Variant));
        TestTrue(TEXT("Uses canonical inventory identity"), Items->FindItem(Recipe.Output) != nullptr);
    }
    EKalmalaIcon Missing; int32 Variant;
    TestFalse(TEXT("Forged IDs have no assigned icon"), UKalmalaIconWidget::FindCatalogueIcon(TEXT("Forged"), Missing, Variant));
    return true;
}
#endif
