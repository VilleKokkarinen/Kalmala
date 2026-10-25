#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaToolLifecycleContract.h"
#include "KalmalaToolProgressionContract.h"
#include "KalmalaPlayerModelComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaCombatComponent.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaPlayerDiscoverySaveGame.h"
#include "KalmalaResistanceComponent.h"
#include "KalmalaWildlifeSpawn.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "OnlineSubsystemTypes.h"
#include "Misc/ScopeExit.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStormcutSwordTest, "Kalmala.Gameplay.Combat.StormcutSword",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaStormcutSwordTest::RunTest(const FString& Parameters)
{
    const auto* Definition = FKalmalaToolLifecycleContract::FindDefinition(TEXT("StormcutSword"));
    if (!TestNotNull(TEXT("Sword has a runtime lifecycle"), Definition)) return false;
    TestEqual(TEXT("Sword has its own supported kind"), Definition->Kind, EKalmalaToolKind::StormcutSword);
    TestEqual(TEXT("Sword is combat only"), Definition->Action, EKalmalaToolAction::None);
    TestFalse(TEXT("Sword uses one hand"), FKalmalaToolLifecycleContract::IsTwoHanded(TEXT("StormcutSword")));
    TestEqual(TEXT("Held silhouette is a sword"), UKalmalaPlayerModelComponent::GetHeldToolVisualForItem(TEXT("StormcutSword")), EKalmalaHeldToolVisual::Sword);
    const auto Damage = FKalmalaToolLifecycleContract::GetAttackDamageBonuses(TEXT("StormcutSword"), 1, TEXT("Basic"));
    TestEqual(TEXT("Basic has seventy Slash"), Damage.Slash, 70.f);
    TestEqual(TEXT("No extra elemental/style damage"), Damage.Total(), 70.f);
    TestEqual(TEXT("Starting condition"), FKalmalaToolLifecycleContract::GetMaxDurability(TEXT("StormcutSword"), 1), 220);
    const TArray<FKalmalaInventoryStack> Materials = {{TEXT("Iron"),25}, {TEXT("Densewood"),3}, {TEXT("DeerHide"),2}, {TEXT("StormSapphire"),1}};
    TArray<FKalmalaToolState> Crafted, Upgraded;
    TArray<FKalmalaInventoryStack> AfterCraft, AfterUpgrade;
    FString Reason;
    const auto StartingTools = FKalmalaToolLifecycleContract::BuildInitialCarriedTools();
    TestFalse(TEXT("Client cannot craft a sword"), FKalmalaToolProgressionContract::BuildServerUpgrade(
        false, nullptr, TEXT("ForgeKit"), 1, TEXT("StormcutSword"), StartingTools, Materials, Crafted, AfterCraft, Reason));
    if (!TestTrue(TEXT("Paid Forge acquisition succeeds"), FKalmalaToolProgressionContract::BuildServerUpgrade(
        true, nullptr, TEXT("ForgeKit"), 1, TEXT("StormcutSword"), StartingTools, Materials, Crafted, AfterCraft, Reason))) return false;
    const auto* Sword = Crafted.FindByPredicate([](const auto& Tool) { return Tool.ItemId == TEXT("StormcutSword"); });
    if (!TestNotNull(TEXT("Craft creates a carried sword"), Sword)) return false;
    const FGuid Guid = Sword->InstanceGuid;
    TestTrue(TEXT("Paid Forge upgrade succeeds"), FKalmalaToolProgressionContract::BuildServerUpgrade(
        true, nullptr, TEXT("ForgeKit"), 2, TEXT("StormcutSword"), Crafted, AfterCraft, Upgraded, AfterUpgrade, Reason));
    const auto* LevelTwo = Upgraded.FindByPredicate([](const auto& Tool) { return Tool.ItemId == TEXT("StormcutSword"); });
    if (!TestNotNull(TEXT("Upgrade retains sword"), LevelTwo)) return false;
    TestTrue(TEXT("Upgrade retains physical identity and current condition"), LevelTwo->InstanceGuid == Guid && LevelTwo->ToolLevel == 2 && LevelTwo->Durability == 220);
    TestEqual(TEXT("Upgrade raises maximum condition by thirty"), FKalmalaToolLifecycleContract::GetMaxDurability(TEXT("StormcutSword"), 2), 250);
    TestTrue(TEXT("Exact acquisition and upgrade materials consumed"), AfterUpgrade.IsEmpty());
    TestEqual(TEXT("Upgrade adds four Slash"), FKalmalaToolLifecycleContract::GetAttackDamageBonuses(TEXT("StormcutSword"), 2, TEXT("Basic")).Slash, 74.f);
    auto Worn = *LevelTwo; Worn.Durability = 0;
    FKalmalaToolState Repaired;
    TestTrue(TEXT("Normal free station repair supports sword"), FKalmalaToolLifecycleContract::BuildServerFreeRepair(true, true, Worn, Repaired));
    TestEqual(TEXT("Repair restores level maximum"), Repaired.Durability, 250);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaStormcutAcceptedHitWearTest,
    "Kalmala.Gameplay.Combat.StormcutAcceptedHitWear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaStormcutAcceptedHitWearTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Stormcut hit fixture world exists"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!World->SetGameMode(FURL())) return false;
    World->InitializeActorsForPlay(FURL());

    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    if (!TestNotNull(TEXT("Stormcut owner pawn spawns"), Pawn)) return false;
    Pawn->DispatchBeginPlay();
    AKalmalaGameMode* Mode = World->GetAuthGameMode<AKalmalaGameMode>();
    APlayerState* PlayerState = World->SpawnActor<APlayerState>();
    if (!TestNotNull(TEXT("Persistence fixture has an authenticated player state"), PlayerState)
        || !TestNotNull(TEXT("Persistence fixture has an authoritative game mode"), Mode)) return false;
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        FString::Printf(TEXT("stormcut-hit-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
        FName(TEXT("KalmalaStormcutHitTest")));
    const FUniqueNetIdRef BaseNetId = NetId;
    PlayerState->SetUniqueId(FUniqueNetIdRepl(BaseNetId));
    Pawn->SetPlayerState(PlayerState);

    UKalmalaInventoryComponent* Inventory = Pawn->GetInventoryComponent();
    UKalmalaCombatComponent* Combat = Pawn->GetCombatComponent();
    if (!TestNotNull(TEXT("Owner has the server inventory"), Inventory)
        || !TestNotNull(TEXT("Owner has the server combat component"), Combat)) return false;
    TArray<FKalmalaToolState>& Tools = const_cast<TArray<FKalmalaToolState>&>(Pawn->GetCarriedToolInventory());
    FKalmalaToolState* CarriedSword = Tools.FindByPredicate([](const FKalmalaToolState& Entry)
    {
        return Entry.ItemId == TEXT("StormcutSword");
    });
    if (CarriedSword == nullptr)
    {
        CarriedSword = &Tools.AddDefaulted_GetRef();
        CarriedSword->ItemId = TEXT("StormcutSword");
    }
    if (!CarriedSword->InstanceGuid.IsValid()) CarriedSword->InstanceGuid = FGuid::NewGuid();
    CarriedSword->ToolLevel = 1;
    CarriedSword->Durability = FKalmalaToolLifecycleContract::GetMaxDurability(
        CarriedSword->ItemId, CarriedSword->ToolLevel);
    const FKalmalaToolState Sword = *CarriedSword;
    FKalmalaToolState* OtherMeleeTool = Tools.FindByPredicate([](const FKalmalaToolState& Entry)
    {
        return Entry.ItemId == TEXT("StoneKnife");
    });
    if (OtherMeleeTool == nullptr)
    {
        OtherMeleeTool = &Tools.AddDefaulted_GetRef();
        OtherMeleeTool->ItemId = TEXT("StoneKnife");
        OtherMeleeTool->InstanceGuid = FGuid::NewGuid();
    }
    if (!OtherMeleeTool->InstanceGuid.IsValid()) OtherMeleeTool->InstanceGuid = FGuid::NewGuid();
    OtherMeleeTool->ToolLevel = 1;
    OtherMeleeTool->Durability = FKalmalaToolLifecycleContract::GetMaxDurability(
        OtherMeleeTool->ItemId, OtherMeleeTool->ToolLevel);
    const FGuid OtherMeleeToolGuid = OtherMeleeTool->InstanceGuid;
    const int32 OtherMeleeToolCondition = OtherMeleeTool->Durability;
    Inventory->SynchronizeGridFromServer();
    FString Reason;
    if (!TestTrue(TEXT("Server equips the exact Stormcut instance"),
        Inventory->SetMainHandFromServer(Sword.InstanceGuid, Reason))) return false;

    const auto Advance = [World, Combat](const float Seconds)
    {
        World->Tick(LEVELTICK_TimeOnly, Seconds);
        Combat->TickComponent(Seconds, LEVELTICK_All, nullptr);
    };
    Combat->ServerRequestAttack(1);
    TestEqual(TEXT("A target-free swing enters windup"), Combat->GetActionPhase(), EKalmalaCombatActionPhase::Windup);
    Advance(0.20f);
    TestEqual(TEXT("A target-free miss does not wear Stormcut"), Pawn->GetToolDurability(Sword.InstanceGuid), 220);
    TestEqual(TEXT("A target-free miss grants no hit feedback"), Combat->GetFeedback(), EKalmalaCombatFeedback::None);
    Advance(0.20f); Advance(0.20f);

    FKalmalaWorldPopulationSpawn Spawn;
    Spawn.Kind = EKalmalaWorldPopulationKind::Wildlife;
    Spawn.Location = FVector(140, 0, 55);
    AKalmalaWildlifeSpawn* Target = World->SpawnActor<AKalmalaWildlifeSpawn>(Spawn.Location, FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("A nonlethal wildlife target spawns"), Target)) return false;
    Target->InitializeServer(Spawn, EKalmalaWildlifeArchetype::MoonBear);
    UKalmalaResistanceComponent* Resistance = Target->FindComponentByClass<UKalmalaResistanceComponent>();
    if (!TestNotNull(TEXT("Wildlife target exposes its resistance profile"), Resistance)) return false;
    Resistance->Profile.Armor = 0.0f;
    const float InitialHealth = Target->GetHealth();
    FKalmalaDamageChannels Slash;
    Slash.Slash = 70.0f;

    TestFalse(TEXT("A forged active sword GUID rejects the hit"),
        Target->ApplyStormcutCombatDamageFromServer(Slash, Pawn, FGuid::NewGuid()));
    TestEqual(TEXT("Wrong GUID leaves target health unchanged"), Target->GetHealth(), InitialHealth);
    TestEqual(TEXT("Wrong GUID spends no condition"), Pawn->GetToolDurability(Sword.InstanceGuid), 220);

    Pawn->SetRole(ROLE_SimulatedProxy);
    TestFalse(TEXT("A non-authoritative attacker cannot apply a Stormcut hit"),
        Target->ApplyStormcutCombatDamageFromServer(Slash, Pawn, Sword.InstanceGuid));
    TestEqual(TEXT("Non-authority rejection leaves target health unchanged"), Target->GetHealth(), InitialHealth);
    TestEqual(TEXT("Non-authority rejection spends no condition"), Pawn->GetToolDurability(Sword.InstanceGuid), 220);
    Pawn->SetRole(ROLE_Authority);

    Target->SetRole(ROLE_SimulatedProxy);
    TestFalse(TEXT("A non-authoritative target cannot accept a Stormcut hit"),
        Target->ApplyStormcutCombatDamageFromServer(Slash, Pawn, Sword.InstanceGuid));
    TestEqual(TEXT("Target authority rejection leaves condition unchanged"), Pawn->GetToolDurability(Sword.InstanceGuid), 220);
    Target->SetRole(ROLE_Authority);

    FKalmalaToolState* MutableSword = Tools.FindByPredicate([&Sword](const FKalmalaToolState& Entry)
    {
        return Entry.InstanceGuid == Sword.InstanceGuid;
    });
    if (!TestNotNull(TEXT("Exact carried Stormcut record remains available"), MutableSword)) return false;
    MutableSword->Durability = 0;
    TestFalse(TEXT("A depleted Stormcut cannot apply a hit"),
        Target->ApplyStormcutCombatDamageFromServer(Slash, Pawn, Sword.InstanceGuid));
    TestEqual(TEXT("Depleted Stormcut remains depleted after rejection"), Pawn->GetToolDurability(Sword.InstanceGuid), 0);
    TestEqual(TEXT("Depleted rejection leaves target health unchanged"), Target->GetHealth(), InitialHealth);
    Combat->ServerRequestAttack(2);
    TestEqual(TEXT("Combat cannot start with the depleted sword"), Combat->GetActionPhase(), EKalmalaCombatActionPhase::Idle);
    TestEqual(TEXT("Depleted combat request does not create a swing"), Combat->GetActionSerial(), uint32(1));
    MutableSword->Durability = 220;

    Mode->PlayerStateSaveGateForTests = []() { return false; };
    Combat->ServerRequestAttack(2);
    Advance(0.20f);
    TestEqual(TEXT("Failed player persistence rejects the hit before target damage"), Target->GetHealth(), InitialHealth);
    TestEqual(TEXT("Failed player persistence rolls Stormcut condition back"), Pawn->GetToolDurability(Sword.InstanceGuid), 220);
    TestEqual(TEXT("Failed player persistence reports unavailable, not a hit"), Combat->GetFeedback(), EKalmalaCombatFeedback::Unavailable);
    Mode->PlayerStateSaveGateForTests = TFunction<bool()>();
    Advance(0.20f); Advance(0.20f);

    Combat->ServerRequestAttack(3);
    Advance(0.20f);
    TestEqual(TEXT("One accepted Stormcut hit spends exactly one condition"), Pawn->GetToolDurability(Sword.InstanceGuid), 219);
    TestEqual(TEXT("Accepted Stormcut hit uses authored seventy Slash"), Target->GetHealth(), InitialHealth - 70.0f);
    TestEqual(TEXT("Accepted Stormcut hit publishes hit feedback"), Combat->GetFeedback(), EKalmalaCombatFeedback::Hit);

    FString Identity;
    UKalmalaPlayerDiscoverySaveGameV2* Saved = Mode->GetPlayerDiscoverySave(Pawn, Identity);
    if (!TestNotNull(TEXT("Accepted Stormcut condition has a player save record"), Saved)) return false;
    const FKalmalaPlayerToolSaveRecord* SavedSword = Saved->GetToolRecords().FindByPredicate(
        [&Sword](const FKalmalaPlayerToolSaveRecord& Record) { return Record.InstanceGuid == Sword.InstanceGuid; });
    TestNotNull(TEXT("Persisted tools include the exact Stormcut instance"), SavedSword);
    if (SavedSword) TestEqual(TEXT("Persisted Stormcut condition matches the accepted hit"), SavedSword->Condition, 219);

    Advance(0.20f); Advance(0.20f);
    if (!TestTrue(TEXT("A different carried melee weapon can be selected"),
        Inventory->SetMainHandFromServer(OtherMeleeToolGuid, Reason))) return false;
    Combat->ServerRequestAttack(4);
    TestEqual(TEXT("Other melee weapon starts a normal accepted-action swing"),
        Combat->GetActionPhase(), EKalmalaCombatActionPhase::Windup);
    Advance(0.20f);
    TestEqual(TEXT("Other melee weapon still applies its normal hit"),
        Combat->GetFeedback(), EKalmalaCombatFeedback::Hit);
    TestEqual(TEXT("The Stormcut-specific contract does not add wear to other melee weapons"),
        Pawn->GetToolDurability(OtherMeleeToolGuid), OtherMeleeToolCondition);
    return true;
}
#endif
