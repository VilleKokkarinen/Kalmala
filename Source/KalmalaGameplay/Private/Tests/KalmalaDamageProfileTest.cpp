#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
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
    TestEqual(TEXT("Physical Armor applies once before independent elemental damage"), Resolved, 59.0f);
    Defenses.Armor = 100.0f;
    TestTrue(TEXT("Armor can fully block a hit"), Defenses.Resolve(Combined, Resolved));
    TestEqual(TEXT("General Armor cannot absorb the remaining Fire and Poison"), Resolved, 5.0f);
    Defenses.Armor = -1.0f;
    TestFalse(TEXT("Negative armor fails closed"), Defenses.Resolve(Combined, Resolved));
    TestEqual(TEXT("Invalid resolution returns no damage"), Resolved, 0.0f);
    const FKalmalaResistanceProfile PlayerDefenses;
    FKalmalaDamageChannels PhysicalHit;
    PhysicalHit.Slash = 3.0f;
    PhysicalHit.Crush = 4.0f;
    PhysicalHit.Pierce = 5.0f;
    PhysicalHit.Stab = 6.0f;
    TestTrue(TEXT("Integer player armor resolves combined melee and ranged channels"),
        PlayerDefenses.Resolve(PhysicalHit, Resolved, 10));
    TestEqual(TEXT("Armor is subtracted once per hit, not once per channel"), Resolved, 8.0f);
    PhysicalHit.Fire = 7.0f;
    PhysicalHit.Water = 2.0f;
    PhysicalHit.Holy = 3.0f;
    PhysicalHit.Poison = 4.0f;
    TestTrue(TEXT("Excess physical armor resolves mixed damage"), PlayerDefenses.Resolve(PhysicalHit, Resolved, 100));
    TestEqual(TEXT("Armor cannot absorb fire, water, holy or poison"), Resolved, 16.0f);
    PhysicalHit.Slash = PhysicalHit.Crush = PhysicalHit.Pierce = PhysicalHit.Stab = 0.0f;
    TestTrue(TEXT("Element-only hits resolve with player armor"), PlayerDefenses.Resolve(PhysicalHit, Resolved, 100));
    TestEqual(TEXT("Element-only damage is unchanged"), Resolved, 16.0f);
    FKalmalaResistanceProfile ArmoredPlayer;
    ArmoredPlayer.Armor = 100.0f;
    TestTrue(TEXT("General armor does not absorb element-only damage"), ArmoredPlayer.Resolve(PhysicalHit, Resolved));
    TestEqual(TEXT("Fire, water, holy and poison bypass general armor"), Resolved, 16.0f);
    FKalmalaDamageChannels PoisonOnly;
    PoisonOnly.Poison = 1.0f;
    TestTrue(TEXT("Poison still resolves through channel resistance"), ArmoredPlayer.Resolve(PoisonOnly, Resolved));
    TestEqual(TEXT("Poison channel ignores physical armor"), Resolved, 1.0f);
    TestFalse(TEXT("Negative player armor fails closed"), PlayerDefenses.Resolve(PhysicalHit, Resolved, -1));
    TestFalse(TEXT("Unbounded player armor fails closed"), PlayerDefenses.Resolve(PhysicalHit, Resolved, 6001));
    FKalmalaDamageChannels Invalid = Combined;
    Invalid.Poison = -1.0f;
    TestFalse(TEXT("Negative damage is invalid"), Invalid.IsValid());
    Invalid.Poison = 100001.0f;
    TestFalse(TEXT("Unbounded damage is invalid"), Invalid.IsValid());

    Invalid.Fire = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("Nonfinite damage is invalid"), Invalid.IsValid());
    FKalmalaDamageChannels WildlifeHit;
    WildlifeHit.Crush = 25.0f;
    TestTrue(TEXT("Existing wildlife pressure remains within the accepted hit bound"),
        AKalmalaCharacter::IsWildlifeDamageWithinBounds(WildlifeHit));
    WildlifeHit = {};
    WildlifeHit.Crush = AKalmalaCharacter::MaximumWildlifeHitDamage;
    TestTrue(TEXT("The approved 300-damage wildlife profile is accepted"),
        AKalmalaCharacter::IsWildlifeDamageWithinBounds(WildlifeHit));
    WildlifeHit.Crush += 0.01f;
    TestFalse(TEXT("Wildlife hits above the approved maximum fail closed"),
        AKalmalaCharacter::IsWildlifeDamageWithinBounds(WildlifeHit));
    WildlifeHit = {};
    TestFalse(TEXT("Empty wildlife hits remain rejected"), AKalmalaCharacter::IsWildlifeDamageWithinBounds(WildlifeHit));
    WildlifeHit.Crush = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("Nonfinite wildlife hits remain rejected"), AKalmalaCharacter::IsWildlifeDamageWithinBounds(WildlifeHit));
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
    TestEqual(TEXT("Bronze Axe basic attack includes the authored profile and style bonuses"), Axe.Slash, 60.0f);
    TestEqual(TEXT("Bronze Axe upgrade changes the slash bonus"),
        FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(TEXT("BronzeAxe"), 2).Slash, 55.0f);
    TestEqual(TEXT("Undefined attack styles cannot authorize damage"),
        FKalmalaToolLifecycleContract::GetAttackDamageBonuses(TEXT("BronzeAxe"), 1, TEXT("Unknown")).Total(), 0.0f);
    const FKalmalaDamageChannels Spear = FKalmalaToolLifecycleContract::GetWeaponDamageBonuses(TEXT("StoneSpear"), 1);
    TestEqual(TEXT("Stone Spear uses its authored stab bonus"), Spear.Stab, 15.0f);
    TestEqual(TEXT("Melee spears do not use ranged pierce"), Spear.Pierce, 0.0f);
    struct FMaceDamageExpectation
    {
        FName ItemId;
        float Crush;
        float Total;
    };
    for (const FMaceDamageExpectation& Mace : TArray<FMaceDamageExpectation>{
        {TEXT("StoneMace"), 25.0f, 35.0f},
        {TEXT("BronzeMace"), 35.0f, 55.0f},
        {TEXT("IronMace"), 55.0f, 80.0f}})
    {
        const FKalmalaToolDefinition* Definition = FKalmalaToolLifecycleContract::FindDefinition(Mace.ItemId);
        if (TestNotNull(TEXT("Mace identity resolves to a server weapon definition"), Definition))
        {
            const FKalmalaDamageChannels Bonuses = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(Mace.ItemId, 1, TEXT("Basic"));
            TestEqual(TEXT("Mace Basic attack resolves authored crush damage"), Bonuses.Crush, Mace.Crush);
            TestEqual(TEXT("Mace Basic attack resolves all authored damage channels"), Bonuses.Total(), Mace.Total);
            TestEqual(TEXT("Maces cannot perform material gathering"), Definition->Action, EKalmalaToolAction::None);
            TestTrue(TEXT("Appended mace kind is allowlisted"), FKalmalaToolLifecycleContract::IsKnownTool(Definition->Kind));
        }
    }
    const UKalmalaToolCatalogue* Catalogue = UKalmalaToolCatalogue::Get();
    TestTrue(TEXT("Knife tiers share one attack list"),
        &Catalogue->GetAttacks(TEXT("StoneKnife")) == &Catalogue->GetAttacks(TEXT("BronzeKnife"))
        && &Catalogue->GetAttacks(TEXT("StoneKnife")) == &Catalogue->GetAttacks(TEXT("IronKnife")));
    TestTrue(TEXT("Axes have their own family list"),
        &Catalogue->GetAttacks(TEXT("StoneAxe")) != &Catalogue->GetAttacks(TEXT("StoneKnife")));
    UKalmalaToolCatalogue* InvalidCatalogue = NewObject<UKalmalaToolCatalogue>();
    InvalidCatalogue->Profiles = Catalogue->Profiles;
    InvalidCatalogue->Recipes = Catalogue->Recipes;
    InvalidCatalogue->UpgradeCosts = Catalogue->UpgradeCosts;
    InvalidCatalogue->LevelUpRewards = Catalogue->LevelUpRewards;
    InvalidCatalogue->AttackLists = Catalogue->AttackLists;
    if (!InvalidCatalogue->AttackLists.IsEmpty())
    {
        const FKalmalaWeaponAttackListData Original = InvalidCatalogue->AttackLists[0];
        InvalidCatalogue->AttackLists[0].ItemIds.Add(TEXT("UnknownTool"));
        TestFalse(TEXT("Unknown shared attack item IDs are rejected"), InvalidCatalogue->IsValidCatalogue());
        InvalidCatalogue->AttackLists[0] = Original;
        FKalmalaWeaponAttackListData Overlap = Original;
        Overlap.AttackListId = TEXT("Overlap");
        InvalidCatalogue->AttackLists.Add(Overlap);
        TestFalse(TEXT("Overlapping attack lists are rejected"), InvalidCatalogue->IsValidCatalogue());
        InvalidCatalogue->AttackLists.RemoveAt(InvalidCatalogue->AttackLists.Num() - 1);
        InvalidCatalogue->AttackLists[0].Attacks.Reset();
        TestFalse(TEXT("Empty family attack lists are rejected"), InvalidCatalogue->IsValidCatalogue());
        InvalidCatalogue->AttackLists[0] = Original;
        InvalidCatalogue->AttackLists[0].Attacks.Add(Original.Attacks[0]);
        TestFalse(TEXT("Duplicate attack styles are rejected"), InvalidCatalogue->IsValidCatalogue());
    }
    TestTrue(TEXT("The migrated mixed-channel catalogue remains valid"), Catalogue->IsValidCatalogue());
    return true;
}
#endif
