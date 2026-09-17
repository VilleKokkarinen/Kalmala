#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaSupportMagicComponent.h"
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
    TestTrue(TEXT("Activation requires authority, entitlement, sequence, cooldown, and stamina"), UKalmalaSupportMagicComponent::IsActivationAllowed(true, true, true, true, true));
    TestFalse(TEXT("Activation cannot bypass stamina"), UKalmalaSupportMagicComponent::IsActivationAllowed(true, true, true, true, false));
    TArray<uint8> Bytes; TestTrue(TEXT("Player discovery serializes before acknowledgement"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Reloaded = Cast<UKalmalaPlayerDiscoverySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Reloaded player discovery is typed"), Reloaded))
    {
        TestTrue(TEXT("Matching reconnect retains exact discovery and learned effect"), Reloaded->Matches(World, Entitled) && Reloaded->HasDiscovery(Scroll) && Reloaded->HasLearnedEffect(Effect));
        World.WorldSeed = 419;
        TestFalse(TEXT("A different immutable world cannot reuse discovery"), Reloaded->Matches(World, Entitled));
    }
    return true;
}
#endif
