#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaWildlifeSpawn.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaWildlifeStatProfileTest,
    "Kalmala.Gameplay.Wildlife.Stats.AllReviewedArchetypes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKalmalaWildlifeStatProfileTest::RunTest(const FString& Parameters)
{
    struct FExpectedProfile
    {
        EKalmalaWildlifeArchetype Archetype;
        float Health;
        float Armor;
        float Slash;
        float Stab;
        float Crush;
        float Pierce;
    };

    const FExpectedProfile ExpectedProfiles[] =
    {
        { EKalmalaWildlifeArchetype::Boar, 20.0f, 5.0f, 0.0f, 0.0f, 10.0f, 0.0f },
        { EKalmalaWildlifeArchetype::Deer, 30.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::Mireling, 75.0f, 15.0f, 0.0f, 0.0f, 20.0f, 0.0f },
        { EKalmalaWildlifeArchetype::Rat, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 5.0f },
        { EKalmalaWildlifeArchetype::Badger, 100.0f, 10.0f, 20.0f, 0.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::MoonBear, 250.0f, 20.0f, 0.0f, 0.0f, 75.0f, 0.0f },
        { EKalmalaWildlifeArchetype::DeathAdder, 125.0f, 20.0f, 0.0f, 75.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::Viper, 100.0f, 10.0f, 0.0f, 50.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::GiantSnake, 250.0f, 30.0f, 0.0f, 100.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::MosquitoSwarm, 25.0f, 0.0f, 0.0f, 5.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::PeatHusk, 300.0f, 50.0f, 0.0f, 0.0f, 150.0f, 0.0f },
        { EKalmalaWildlifeArchetype::Wolf, 250.0f, 40.0f, 100.0f, 0.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::Lynx, 150.0f, 30.0f, 75.0f, 0.0f, 0.0f, 0.0f },
        { EKalmalaWildlifeArchetype::PolarBear, 750.0f, 60.0f, 0.0f, 0.0f, 200.0f, 0.0f },
        { EKalmalaWildlifeArchetype::HornedGoat, 200.0f, 50.0f, 0.0f, 0.0f, 50.0f, 0.0f },
        { EKalmalaWildlifeArchetype::ThunderSentinel, 1250.0f, 75.0f, 0.0f, 0.0f, 300.0f, 0.0f },
        { EKalmalaWildlifeArchetype::ReefMaw, 180.0f, 2.0f, 0.0f, 50.0f, 0.0f, 0.0f },
    };

    for (const FExpectedProfile& Expected : ExpectedProfiles)
    {
        const FString Name = StaticEnum<EKalmalaWildlifeArchetype>()->GetNameStringByValue(static_cast<int64>(Expected.Archetype));
        const FKalmalaDamageChannels ActualAttack =
            AKalmalaWildlifeSpawn::GetBaseAttackChannelsForArchetype(Expected.Archetype);
        const FString HealthLabel = FString::Printf(TEXT("%s health"), *Name);
        const FString ArmorLabel = FString::Printf(TEXT("%s armor"), *Name);
        TestEqual(HealthLabel, AKalmalaWildlifeSpawn::GetMaximumHealthForArchetype(Expected.Archetype), Expected.Health);
        TestEqual(ArmorLabel, AKalmalaWildlifeSpawn::GetArmorForArchetype(Expected.Archetype), Expected.Armor);

        const FString SlashLabel = FString::Printf(TEXT("%s Slash"), *Name);
        const FString StabLabel = FString::Printf(TEXT("%s Stab"), *Name);
        const FString CrushLabel = FString::Printf(TEXT("%s Crush"), *Name);
        const FString PierceLabel = FString::Printf(TEXT("%s Pierce"), *Name);
        TestEqual(SlashLabel, ActualAttack.Slash, Expected.Slash);
        TestEqual(StabLabel, ActualAttack.Stab, Expected.Stab);
        TestEqual(CrushLabel, ActualAttack.Crush, Expected.Crush);
        TestEqual(PierceLabel, ActualAttack.Pierce, Expected.Pierce);
        TestEqual(FString::Printf(TEXT("%s Fire"), *Name), ActualAttack.Fire, 0.0f);
        TestEqual(FString::Printf(TEXT("%s Water"), *Name), ActualAttack.Water, 0.0f);
        TestEqual(FString::Printf(TEXT("%s Holy"), *Name), ActualAttack.Holy, 0.0f);
        TestEqual(FString::Printf(TEXT("%s Poison"), *Name), ActualAttack.Poison, 0.0f);
        TestEqual(FString::Printf(TEXT("%s total attack"), *Name),
            AKalmalaWildlifeSpawn::GetBaseAttackDamageForArchetype(Expected.Archetype),
            Expected.Slash + Expected.Stab + Expected.Crush + Expected.Pierce);
    }
    return true;
}
#endif
