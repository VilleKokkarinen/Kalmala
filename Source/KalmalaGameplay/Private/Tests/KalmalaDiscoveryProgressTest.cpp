#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaCharacter.h"
#include "KalmalaSupportMagicComponent.h"
#include "KalmalaGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaDiscoveryProgressTest, "Kalmala.Gameplay.Discovery.PlayerScopedPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaDiscoveryProgressTest::RunTest(const FString& Parameters)
{
    FKalmalaWorldGenerationConfig World; World.WorldSeed = 418;
    const FString Entitled(TEXT("NULL:host-player"));
    UKalmalaPlayerDiscoverySaveGame* Save = NewObject<UKalmalaPlayerDiscoverySaveGame>();
    Save->InitializeForPlayer(World, Entitled);
    const FString Scroll(TEXT("Scroll:1:mending:0,0:0"));
    TestTrue(TEXT("Matching entitled player accepts the identity-scoped save"), Save->Matches(World, Entitled));
    TestFalse(TEXT("A different player cannot match the entitled record"), Save->Matches(World, TEXT("NULL:other-player")));
    TestTrue(TEXT("First server-confirmed discovery commits"), Save->AddDiscovery(Scroll));
    TestFalse(TEXT("Duplicate discovery cannot reward twice"), Save->AddDiscovery(Scroll));
    const FString Effect = UKalmalaSupportMagicComponent::CanonicalId(UKalmalaSupportMagicComponent::FromScrollDefinition(TEXT("mending")));
    TestEqual(TEXT("Scroll definition maps only to an allowlisted canonical effect"), Effect, FString(TEXT("Effect:mending")));
    TestTrue(TEXT("Entitled player learns the effect once"), Save->AddLearnedEffect(Effect));
    TestFalse(TEXT("Unknown effect fails closed"), Save->AddLearnedEffect(TEXT("Effect:unknown")));
    const TArray<EKalmalaSupportEffect> Effects = { EKalmalaSupportEffect::Mending, EKalmalaSupportEffect::HearthShield, EKalmalaSupportEffect::BearsVigor, EKalmalaSupportEffect::DeerCall };
    for (const EKalmalaSupportEffect SupportEffect : Effects)
    {
        const FString Canonical = UKalmalaSupportMagicComponent::CanonicalId(SupportEffect);
        TestTrue(TEXT("Every support effect is allowlisted"), UKalmalaSupportMagicComponent::IsKnownEffect(SupportEffect) && !Canonical.IsEmpty());
        TestTrue(TEXT("Every support effect is non-damaging"), UKalmalaSupportMagicComponent::IsNonDamagingEffect(SupportEffect));
        Save->AddLearnedEffect(Canonical);
        TestTrue(TEXT("Every allowlisted effect persists as an entitled learned token"), Save->HasLearnedEffect(Canonical));
    }
    TestFalse(TEXT("Malformed support effect is rejected"), UKalmalaSupportMagicComponent::IsKnownEffect(static_cast<EKalmalaSupportEffect>(255)));
    TestFalse(TEXT("Malformed support effect cannot be treated as non-damaging gameplay"), UKalmalaSupportMagicComponent::IsNonDamagingEffect(static_cast<EKalmalaSupportEffect>(255)));
    TestTrue(TEXT("Activation requires authority, entitlement, sequence, cooldown, and stamina"), UKalmalaSupportMagicComponent::IsActivationAllowed(true, true, true, true, true));
    TestFalse(TEXT("Client-role activation is rejected"), UKalmalaSupportMagicComponent::IsActivationAllowed(false, true, true, true, true));
    TestFalse(TEXT("Replay or zero-sequence activation is rejected"), UKalmalaSupportMagicComponent::IsActivationAllowed(true, true, false, true, true));
    TestFalse(TEXT("Activation cannot bypass stamina"), UKalmalaSupportMagicComponent::IsActivationAllowed(true, true, true, true, false));
    TestTrue(TEXT("Hearth Shield needs the same server activation gates and rejects an active refresh"), UKalmalaSupportMagicComponent::IsHearthShieldActivationAllowed(true, false));
    TestFalse(TEXT("Hearth Shield cannot refresh while its server-owned protection is active"), UKalmalaSupportMagicComponent::IsHearthShieldActivationAllowed(true, true));
    TestEqual(TEXT("Hearth Shield absorbs only its bounded remaining strength"), UKalmalaSupportMagicComponent::CalculateHearthShieldAbsorption(true, true, 25.0f, 10.0f), 10.0f);
    TestEqual(TEXT("Expired or non-authoritative Hearth Shield absorbs no damage"), UKalmalaSupportMagicComponent::CalculateHearthShieldAbsorption(false, true, 25.0f, 40.0f), 0.0f);
    TestTrue(TEXT("Bear's Vigor needs the shared activation gates and rejects an active refresh"), UKalmalaSupportMagicComponent::IsBearsVigorActivationAllowed(true, false));
    TestFalse(TEXT("Bear's Vigor cannot refresh while its temporary modifiers are active"), UKalmalaSupportMagicComponent::IsBearsVigorActivationAllowed(true, true));
    TestEqual(TEXT("Bear's Vigor applies only its bounded server strength modifier"), UKalmalaSupportMagicComponent::CalculateBearsVigorDamage(true, true, 25.0f, 1.4f), 35.0f);
    TestEqual(TEXT("Inactive or malformed Bear's Vigor cannot increase combat damage"), UKalmalaSupportMagicComponent::CalculateBearsVigorDamage(true, false, 25.0f, 1.4f), 25.0f);
    TestTrue(TEXT("Deer Call requires the shared gates and an eligible server-selected deer"), UKalmalaSupportMagicComponent::IsDeerCallActivationAllowed(true, true));
    TestFalse(TEXT("Deer Call rejects an empty eligible set before payment"), UKalmalaSupportMagicComponent::IsDeerCallActivationAllowed(true, false));
    FString BossCandidate;
    for (uint64 CandidateSeed = 1; CandidateSeed < 128 && BossCandidate.IsEmpty(); ++CandidateSeed)
    {
        const FString Candidate = FString::Printf(TEXT("0/0/0/%llu"), CandidateSeed);
        if (AKalmalaGameMode::IsMirelingBossRewardId(Candidate)) BossCandidate = Candidate;
    }
    TestTrue(TEXT("Mireling boss designation is a bounded server-derived stable ID gate"), !BossCandidate.IsEmpty());
    TestFalse(TEXT("Malformed Mireling boss IDs are rejected"), AKalmalaGameMode::IsMirelingBossRewardId(TEXT("mireling-boss")));
    const FString BossScroll = AKalmalaGameMode::GetMirelingBossScrollId(World.WorldSeed);
    const FString BossDefinition = AKalmalaGameMode::GetMirelingBossScrollDefinition(World.WorldSeed);
    TestTrue(TEXT("Mireling boss scroll is one optional world-derived allowlisted entitlement"), BossScroll == FString::Printf(TEXT("Scroll:1:%s:mireling-boss"), *BossDefinition));
    TestTrue(TEXT("Mireling boss scroll does not encode a route or client location"), !BossScroll.Contains(TEXT("/")) && BossScroll.Contains(TEXT("mireling-boss")));
    TestTrue(TEXT("Mending accepts only a finite damaged living allied pawn in range on the server"), AKalmalaCharacter::IsMendingReceiveAllowed(true, true, true, true, true, true, 30.0f));
    TestFalse(TEXT("Mending rejects a non-allied target"), AKalmalaCharacter::IsMendingReceiveAllowed(true, false, true, true, true, true, 30.0f));
    TestFalse(TEXT("Mending rejects a dead, full-health, distant, or excessive target state"),
        AKalmalaCharacter::IsMendingReceiveAllowed(true, true, true, false, true, true, 30.0f)
        || AKalmalaCharacter::IsMendingReceiveAllowed(true, true, true, true, false, true, 30.0f)
        || AKalmalaCharacter::IsMendingReceiveAllowed(true, true, true, true, true, false, 30.0f)
        || AKalmalaCharacter::IsMendingReceiveAllowed(true, true, true, true, true, true, 31.0f));
    TArray<uint8> Bytes; TestTrue(TEXT("Player discovery serializes before acknowledgement"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Reloaded = Cast<UKalmalaPlayerDiscoverySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Reloaded player discovery is typed"), Reloaded))
    {
        bool bAllEffectsPersisted = true;
        for (const EKalmalaSupportEffect SupportEffect : Effects)
        {
            bAllEffectsPersisted = bAllEffectsPersisted && Reloaded->HasLearnedEffect(UKalmalaSupportMagicComponent::CanonicalId(SupportEffect));
        }
        TestTrue(TEXT("Matching reconnect retains exact discovery and every learned effect"), Reloaded->Matches(World, Entitled) && Reloaded->HasDiscovery(Scroll) && bAllEffectsPersisted);
        World.WorldSeed = 419;
        TestFalse(TEXT("A different immutable world cannot reuse discovery"), Reloaded->Matches(World, Entitled));
    }
    return true;
}
#endif
