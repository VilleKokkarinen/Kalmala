#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaDamageProfile.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaToolLifecycleContract.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaDamageProfileTest,
    "Kalmala.Gameplay.Combat.DamageProfiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaDamageProfileTest::RunTest(const FString& Parameters)
{
    FKalmalaDamageChannels Weapon;
    Weapon.Slash = 50.0f;
    Weapon.Stab = 10.0f;
    Weapon.Fire = 5.0f;
    FKalmalaDamageChannels Attack;
    Attack.Slash = 5.0f;
    Attack.Crush = 8.0f;
    Attack.Poison = 2.0f;
    const FKalmalaDamageChannels Combined = Weapon.Added(Attack);
    TestEqual(TEXT("Weapon and attack add the same channel"), Combined.Slash, 55.0f);
    TestEqual(TEXT("Attack can add another simultaneous melee channel"), Combined.Crush, 8.0f);
    TestEqual(TEXT("Elemental channels contribute to the hit"), Combined.Total(), 80.0f);
    TestEqual(TEXT("Strength scales every active channel"), Combined.Scaled(2.0f).Poison, 4.0f);
    TestTrue(TEXT("Inspection names every active type"), Combined.Describe().Contains(TEXT("Slash bonus"))
        && Combined.Describe().Contains(TEXT("Stab bonus")) && Combined.Describe().Contains(TEXT("Fire bonus")));

    FKalmalaResistanceProfile Defenses;
    Defenses.Reductions.Slash = 5.0f;
    Defenses.Reductions.Stab = 20.0f; // Excess resistance cannot cancel other types.
    Defenses.Reductions.Fire = 2.0f;
    Defenses.Reductions.Pierce = 999.0f; // Ranged resistance does not resist melee stab.
    Defenses.Armor = 4.0f;
    float Resolved = -1.0f;
    TestTrue(TEXT("Valid mixed hit resolves"), Defenses.Resolve(Combined, Resolved));
    TestEqual(TEXT("Flat per-type resistance, clamping, then armor once"), Resolved, 59.0f);
    Defenses.Armor = 100.0f;
    TestTrue(TEXT("Armor can fully block a hit"), Defenses.Resolve(Combined, Resolved));
    TestEqual(TEXT("Blocked hits cannot heal"), Resolved, 0.0f);
    Defenses.Armor = -1.0f;
    TestFalse(TEXT("Negative armor fails closed"), Defenses.Resolve(Combined, Resolved));
    TestEqual(TEXT("Invalid resolution returns no damage"), Resolved, 0.0f);
    FKalmalaDamageChannels Invalid = Combined;
    Invalid.Poison = -1.0f;
    TestFalse(TEXT("Negative damage is invalid"), Invalid.IsValid());
    Invalid.Poison = 100001.0f;
    TestFalse(TEXT("Unbounded damage is invalid"), Invalid.IsValid());

    Invalid.Fire = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("Nonfinite damage is invalid"), Invalid.IsValid());
    Defenses.Armor = std::numeric_limits<float>::infinity();
    TestFalse(TEXT("Nonfinite defenses fail closed"), Defenses.Resolve(Combined, Resolved));

    FKalmalaDamageChannels WeaknessHit;
    WeaknessHit.Slash = 10.0f;
    FKalmalaResistanceProfile Weakness;
    Weakness.Reductions.Slash = -5.0f;
    Weakness.Reductions.Fire = -100.0f;
    Weakness.Armor = 2.0f;
    TestTrue(TEXT("Negative resistance is a valid authored weakness"), Weakness.Resolve(WeaknessHit, Resolved));
    TestEqual(TEXT("Slash weakness adds flat damage before armor; absent fire adds none"), Resolved, 13.0f);
    TestTrue(TEXT("An empty hit remains empty despite weaknesses"), Weakness.Resolve(FKalmalaDamageChannels(), Resolved));
    TestEqual(TEXT("Weaknesses cannot create damage from inactive channels"), Resolved, 0.0f);
    Weakness.Reductions.Slash = -100001.0f;
    TestFalse(TEXT("Excessively negative resistance fails closed"), Weakness.Resolve(WeaknessHit, Resolved));
    Weakness.Reductions.Slash = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("Nonfinite weakness fails closed"), Weakness.Resolve(WeaknessHit, Resolved));

    const FKalmalaDamageChannels Axe = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(TEXT("BronzeAxe"), 1, TEXT("Basic"));
    TestEqual(TEXT("Bronze Axe level one has a slash bonus"), Axe.Slash, 50.0f);
    TestEqual(TEXT("Bronze Axe upgrade changes the slash bonus"),
        FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(TEXT("BronzeAxe"), 2).Slash, 55.0f);
    TestEqual(TEXT("Undefined attack styles cannot authorize damage"),
        FKalmalaToolLifecycleContract::GetAttackDamageBonuses(TEXT("BronzeAxe"), 1, TEXT("Unknown")).Total(), 0.0f);
    const FKalmalaDamageChannels Spear = FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(TEXT("StoneSpear"), 1);
    TestEqual(TEXT("Melee spears use stab"), Spear.Stab, 42.0f);
    TestEqual(TEXT("Melee spears do not use ranged pierce"), Spear.Pierce, 0.0f);
    for (const TPair<FName, float>& Mace : TArray<TPair<FName, float>>{
        {TEXT("StoneMace"), 30.0f}, {TEXT("BronzeMace"), 42.0f}, {TEXT("IronMace"), 58.0f}})
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Mace.Key);
        if (TestNotNull(TEXT("Mace identity resolves to a server weapon definition"), Definition))
        {
            const FKalmalaDamageChannels Bonuses = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(Mace.Key, 1, TEXT("Basic"));
            TestEqual(TEXT("Mace Basic attack uses the knife-tier amount as crush"), Bonuses.Crush, Mace.Value);
            TestEqual(TEXT("Maces have only crush by default"), Bonuses.Total(), Bonuses.Crush);
            TestEqual(TEXT("Maces cannot perform material gathering"), Definition->Action, EKalmalaToolAction::None);
            TestTrue(TEXT("Appended mace kind is allowlisted"), FKalmalaToolLifecycleContract::IsKnownTool(Definition->Kind));
        }
    }
    TestTrue(TEXT("The migrated mixed-channel catalogue remains valid"), UKalmalaToolCatalogue::Get()->IsValidCatalogue());
    return true;
}
#endif
