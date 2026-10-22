#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCombatComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaToolCatalogue.h"
#include "KalmalaArrowProjectile.h"
#include "KalmalaWildlifeSpawn.h"
#include "KalmalaGameMode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "OnlineSubsystemTypes.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBowDrawTest, "Kalmala.Gameplay.Combat.BowDrawAndArrows",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FKalmalaBowDrawTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Half draw follows the bow attribute"), UKalmalaCombatComponent::CalculateBowDrawProgress(0.75f, 1.5f), 0.5f);
    TestEqual(TEXT("A faster bow reaches full draw sooner"), UKalmalaCombatComponent::CalculateBowDrawProgress(0.75f, 0.75f), 1.0f);
    TestEqual(TEXT("Future server skill multiplier can reduce draw time"), UKalmalaCombatComponent::CalculateBowDrawProgress(0.75f, 1.5f, 2), 1.0f);
    TestEqual(TEXT("Overholding clamps at full draw"), UKalmalaCombatComponent::CalculateBowDrawProgress(30, 1.5f), 1.0f);
    TestEqual(TEXT("Invalid draw duration fails closed"), UKalmalaCombatComponent::CalculateBowDrawProgress(1, 0), 0.0f);
    TestEqual(TEXT("Nonfinite draw is rejected"), UKalmalaCombatComponent::CalculateBowDrawProgress(std::numeric_limits<float>::quiet_NaN(), 1.5f), 0.0f);
    TestTrue(TEXT("Crosshair circle continuously shrinks with draw"), UKalmalaCombatComponent::GetBowCrosshairRadius(0)
        > UKalmalaCombatComponent::GetBowCrosshairRadius(0.5f) && UKalmalaCombatComponent::GetBowCrosshairRadius(0.5f)
        > UKalmalaCombatComponent::GetBowCrosshairRadius(1));
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!World) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AKalmalaGameMode::StaticClass();
    if (!World->SetGameMode(FURL())) return false;
    World->InitializeActorsForPlay(FURL());
    auto* Pawn = World->SpawnActor<AKalmalaCharacter>();
    Pawn->DispatchBeginPlay();
    auto* PlayerState = World->SpawnActor<APlayerState>();
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(
        FString::Printf(TEXT("bow-acceptance-%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)),
        FName(TEXT("KalmalaBowTest")));
    const FUniqueNetIdRef BaseNetId = NetId;
    PlayerState->SetUniqueId(FUniqueNetIdRepl(BaseNetId));
    Pawn->SetPlayerState(PlayerState);
    auto* Inventory = Pawn->GetInventoryComponent();
    auto* Combat = Pawn->GetCombatComponent();
    Combat->ServerRequestAttack(1);
    TestEqual(TEXT("Held hammer cannot attack"), Combat->GetActionSerial(), uint32(0));
    auto& Tools = const_cast<TArray<FKalmalaToolState>&>(Pawn->GetCarriedToolInventory());
    FKalmalaToolState Bow;
    Bow.ItemId = TEXT("LightBow"); Bow.InstanceGuid = FGuid::NewGuid(); Bow.Durability = 140; Bow.ToolLevel = 1;
    Tools.Add(Bow); Inventory->SynchronizeGridFromServer();
    FString Reason;
    TestTrue(TEXT("Bow equips"), Inventory->SetMainHandFromServer(Bow.InstanceGuid, Reason));
    TestFalse(TEXT("No arrows prevents drawing"), Combat->CanDrawBow());
    TestTrue(TEXT("Two real arrow stacks granted"), Inventory->TryGrantFromServer(TEXT("WoodenArrow"), 200));
    const auto Before = Inventory->GetStacks();
    if (!TestEqual(TEXT("Two stack fixture exists"), Before.Num(), 2)) return false;
    const FGuid Preferred = Before[1].InstanceGuid;
    Combat->ServerPreferArrowStack(Before[1].ItemGuids[0]);
    TestEqual(TEXT("Inventory unit selection resolves the exact arrow stack"), Combat->GetPreferredArrowStack(), Preferred);
    TestEqual(TEXT("Selected arrow type is available for HUD display"), Combat->GetArrowItemIdForDisplay(), FName(TEXT("WoodenArrow")));
    Combat->ServerPreferArrowStack(FGuid::NewGuid());
    TestEqual(TEXT("Forged stack request does not change preference"), Combat->GetPreferredArrowStack(), Preferred);
    TestEqual(TEXT("Light Bow draw duration loads from catalogue"), Combat->GetBowDrawDuration(), 2.0f);
    Combat->ServerRequestAttack(1);
    TestEqual(TEXT("Bow has no melee attack"), Combat->GetActionSerial(), uint32(0));
    Combat->ServerBeginBowDraw(2);
    TestTrue(TEXT("Press starts a target-free bow draw"), Combat->IsBowDrawing());
    for (int32 Step=0; Step<4; ++Step) World->Tick(LEVELTICK_TimeOnly, 0.25f);
    TestEqual(TEXT("Server elapsed time controls half draw"), Combat->GetBowDrawProgress(), 0.5f);
    Combat->ServerReleaseBow(3, false);
    TestFalse(TEXT("Release ends drawing"), Combat->IsBowDrawing());
    TestEqual(TEXT("One arrow consumed"), Inventory->GetQuantity(TEXT("WoodenArrow")), 199);
    TestEqual(TEXT("Preferred stack used before the first stack"), Inventory->GetStacks()[0].Quantity, 100);
    TestEqual(TEXT("Preferred stack loses exactly one unit"), Inventory->GetStacks()[1].Quantity, 99);
    TestEqual(TEXT("One Light Bow condition consumed"), Pawn->GetToolDurability(TEXT("LightBow")), 139);
    int32 ArrowCount=0;
    for (TActorIterator<AKalmalaArrowProjectile> Arrow(World); Arrow; ++Arrow)
    { ++ArrowCount; TestEqual(TEXT("Partial release creates a weaker real arrow"), Arrow->GetShotDamage(), 25.0f); Arrow->Destroy(); }
    TestEqual(TEXT("Release spawns exactly one projectile"), ArrowCount, 1);
    Combat->ServerReleaseBow(4, false);
    TestEqual(TEXT("Duplicate release consumes nothing"), Inventory->GetQuantity(TEXT("WoodenArrow")), 199);
    const auto Recover = [&]() { for(int32 Step=0;Step<2;++Step) {World->Tick(LEVELTICK_TimeOnly,0.2f);Combat->TickComponent(0.2f,LEVELTICK_All,nullptr);} };
    Recover();
    Combat->ServerBeginBowDraw(5); Combat->ServerReleaseBow(6, true);
    TestEqual(TEXT("Cancel does not consume ammo"), Inventory->GetQuantity(TEXT("WoodenArrow")), 199);
    TestEqual(TEXT("Cancel does not wear the Light Bow"), Pawn->GetToolDurability(TEXT("LightBow")), 139);
    Combat->ServerPreferArrowStack(FGuid());
    TestFalse(TEXT("Automatic arrow selection can be restored"), Combat->GetPreferredArrowStack().IsValid());
    Combat->ServerBeginBowDraw(7);
    for (int32 Step=0; Step<8; ++Step) World->Tick(LEVELTICK_TimeOnly,0.25f);
    Combat->ServerReleaseBow(8,false);
    TestEqual(TEXT("Automatic selection consumes first available stack"), Inventory->GetStacks()[0].Quantity, 99);
    auto* Wildlife=World->SpawnActor<AKalmalaWildlifeSpawn>(FVector(600,20,25), FRotator::ZeroRotator);
    for(TActorIterator<AKalmalaArrowProjectile> Arrow(World); Arrow; ++Arrow)
    {
        TestEqual(TEXT("Full draw reaches configured maximum arrow damage"), Arrow->GetShotDamage(), 40.0f);
        auto* Movement=Arrow->FindComponentByClass<UProjectileMovementComponent>();
        for(int32 Step=0;Step<20 && IsValid(*Arrow);++Step)
        {
            Movement->TickComponent(0.02f,LEVELTICK_All,nullptr);
            Arrow->Tick(0.02f);
        }
        if (IsValid(*Arrow)) Arrow->Destroy();
    }
    TestEqual(TEXT("Real swept projectile damages wildlife beyond melee reach"), Wildlife->GetHealth(), 60.0f);
    TestEqual(TEXT("Wooden arrows do not apply fang poison"), Wildlife->GetFangArrowPoisonRemainingSeconds(), 0.0f);
    Recover();
    Combat->ServerBeginBowDraw(9);
    Combat->ServerReleaseBow(10,false);
    for(TActorIterator<AKalmalaArrowProjectile> Arrow(World);Arrow;++Arrow)
    {TestEqual(TEXT("Immediate release still fires a low-power arrow"),Arrow->GetShotDamage(),10.0f);Arrow->Destroy();}
    Recover();
    TestTrue(TEXT("Stone arrows can be granted from their catalogue definition"), Inventory->TryGrantFromServer(TEXT("StoneArrow"), 1));
    const auto StoneStack = Inventory->GetStacks().FindByPredicate(
        [](const auto& Stack) { return Stack.ItemId == TEXT("StoneArrow") && Stack.Quantity > 0; });
    if (!TestNotNull(TEXT("Stone arrow stack exists"), StoneStack) || StoneStack->ItemGuids.IsEmpty()) return false;
    const FGuid StoneRowGuid = StoneStack->InstanceGuid;
    const FGuid StoneUnitGuid = StoneStack->ItemGuids[0];
    TestTrue(TEXT("Two Bronze Arrows can be granted from their catalogue definition"), Inventory->TryGrantFromServer(TEXT("BronzeArrow"), 2));
    const auto BronzeStack = Inventory->GetStacks().FindByPredicate(
        [](const auto& Stack) { return Stack.ItemId == TEXT("BronzeArrow") && Stack.Quantity > 0; });
    if (!TestNotNull(TEXT("Bronze arrow stack exists"), BronzeStack) || BronzeStack->ItemGuids.IsEmpty()) return false;
    const FGuid BronzeRowGuid = BronzeStack->InstanceGuid;
    const FGuid BronzeUnitGuid = BronzeStack->ItemGuids[0];
    Combat->ServerPreferArrowStack(BronzeUnitGuid);
    TestEqual(TEXT("Bronze arrow unit selection resolves its exact stack"), Combat->GetPreferredArrowStack(), BronzeRowGuid);
    TestEqual(TEXT("Preferred Bronze Arrow overrides automatic inventory order"), Combat->GetArrowItemIdForDisplay(), FName(TEXT("BronzeArrow")));
    Recover();
    Combat->ServerBeginBowDraw(11);
    for (int32 Step = 0; Step < 8; ++Step) World->Tick(LEVELTICK_TimeOnly, 0.25f);
    Combat->ServerReleaseBow(12, false);
    TestEqual(TEXT("Committed preferred Bronze Arrow consumes one unit"), Inventory->GetQuantity(TEXT("BronzeArrow")), 1);
    bool bSawPreferredBronzeArrowShot = false;
    for (TActorIterator<AKalmalaArrowProjectile> Arrow(World); Arrow; ++Arrow)
    {
        bSawPreferredBronzeArrowShot = true;
        TestEqual(TEXT("Bronze Arrow uses its authored 50 Pierce attack"), Arrow->GetShotDamage(), 50.0f);
        Arrow->Destroy();
    }
    TestTrue(TEXT("Preferred Bronze Arrow release creates a projectile"), bSawPreferredBronzeArrowShot);
    TestEqual(TEXT("A remaining preferred stack stays selected"), Combat->GetPreferredArrowStack(), BronzeRowGuid);
    Combat->ServerPreferArrowStack(FGuid());
    const auto& GridBeforeMove = Inventory->GetGridInstanceGuids();
    const int32 BronzeSlot = GridBeforeMove.IndexOfByKey(BronzeRowGuid);
    const int32 FirstWoodenSlot = GridBeforeMove.IndexOfByKey(Before[0].InstanceGuid);
    TestTrue(TEXT("Bronze Arrow moves ahead of other types in inventory order"), BronzeSlot != INDEX_NONE
        && FirstWoodenSlot != INDEX_NONE
        && Inventory->MoveSlotByInstanceFromServer(BronzeSlot, FirstWoodenSlot,
            BronzeRowGuid, GridBeforeMove[FirstWoodenSlot]));
    TestEqual(TEXT("Automatic arrow preview follows top-left inventory order"), Combat->GetArrowItemIdForDisplay(), FName(TEXT("BronzeArrow")));
    Recover();
    Combat->ServerBeginBowDraw(13);
    for (int32 Step = 0; Step < 8; ++Step) World->Tick(LEVELTICK_TimeOnly, 0.25f);
    Combat->ServerReleaseBow(14, false);
    TestEqual(TEXT("Automatic selection consumes the first arrow in grid order"), Inventory->GetQuantity(TEXT("BronzeArrow")), 0);
    bool bSawAutomaticBronzeArrowShot = false;
    for (TActorIterator<AKalmalaArrowProjectile> Arrow(World); Arrow; ++Arrow)
    {
        bSawAutomaticBronzeArrowShot = true;
        TestEqual(TEXT("Automatic Bronze Arrow keeps its authored 50 Pierce attack"), Arrow->GetShotDamage(), 50.0f);
        Arrow->Destroy();
    }
    TestTrue(TEXT("Automatic inventory-order release creates a Bronze projectile"), bSawAutomaticBronzeArrowShot);
    TestFalse(TEXT("Exhausted preferred Bronze Arrow stack clears"), Combat->GetPreferredArrowStack().IsValid());
    Combat->ServerPreferArrowStack(StoneUnitGuid);
    TestEqual(TEXT("Stone arrow unit selection resolves its exact stack"), Combat->GetPreferredArrowStack(), StoneRowGuid);
    TestEqual(TEXT("Selected Stone Arrow overrides the first usable grid stack"), Combat->GetArrowItemIdForDisplay(), FName(TEXT("StoneArrow")));
    Recover();
    Combat->ServerBeginBowDraw(15);
    for (int32 Step = 0; Step < 8; ++Step) World->Tick(LEVELTICK_TimeOnly, 0.25f);
    Combat->ServerReleaseBow(16, false);
    TestEqual(TEXT("Committed preferred Stone Arrow consumes one unit"), Inventory->GetQuantity(TEXT("StoneArrow")), 0);
    bool bSawStoneArrowShot = false;
    for (TActorIterator<AKalmalaArrowProjectile> Arrow(World); Arrow; ++Arrow)
    {
        bSawStoneArrowShot = true;
        TestEqual(TEXT("Stone Arrow uses its authored 25 Pierce attack"), Arrow->GetShotDamage(), 25.0f);
        Arrow->Destroy();
    }
    TestTrue(TEXT("Preferred Stone Arrow release creates a projectile"), bSawStoneArrowShot);
    TestFalse(TEXT("Exhausted preferred Stone Arrow stack clears"), Combat->GetPreferredArrowStack().IsValid());
    Recover();

    TestTrue(TEXT("Fang Arrows can be granted from their catalogue definition"),
        Inventory->TryGrantFromServer(TEXT("FangArrows"), 2));
    const auto FangStack = Inventory->GetStacks().FindByPredicate(
        [](const auto& Stack) { return Stack.ItemId == TEXT("FangArrows") && Stack.Quantity > 0; });
    if (!TestNotNull(TEXT("Fang Arrow stack exists"), FangStack) || FangStack->ItemGuids.IsEmpty()) return false;
    Combat->ServerPreferArrowStack(FangStack->ItemGuids[0]);
    TestEqual(TEXT("Fang Arrows can be selected for a bow shot"), Combat->GetArrowItemIdForDisplay(),
        FName(TEXT("FangArrows")));
    float FangDamage = 0.0f;
    const bool bResolvedFangDamage = AKalmalaArrowProjectile::ResolveShotDamage(TEXT("FangArrows"), 0.0f, FangDamage);
    TestTrue(TEXT("Fang Arrow damage resolves at zero draw"), bResolvedFangDamage);
    TestEqual(TEXT("Fang Arrows use fixed 50 Pierce at any draw"), FangDamage, 50.0f);
    FKalmalaWorldPopulationSpawn FangSpawn;
    FangSpawn.SpawnSeed = 703;
    FangSpawn.Location = FVector(400.0f, 300.0f, 25.0f);
    auto* FangTarget = World->SpawnActor<AKalmalaWildlifeSpawn>(FangSpawn.Location, FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Fang Arrow wildlife target exists"), FangTarget)) return false;
    FangTarget->InitializeServer(FangSpawn, EKalmalaWildlifeArchetype::MoonBear);
    Combat->ServerBeginBowDraw(17);
    for (int32 Step = 0; Step < 8; ++Step) World->Tick(LEVELTICK_TimeOnly, 0.25f);
    Combat->ServerReleaseBow(18, false);
    TestEqual(TEXT("Bow release consumes one Fang Arrow"), Inventory->GetQuantity(TEXT("FangArrows")), 1);
    bool bSawFangArrowShot = false;
    for (TActorIterator<AKalmalaArrowProjectile> Arrow(World); Arrow; ++Arrow)
    {
        bSawFangArrowShot = true;
        TestEqual(TEXT("Fang Arrow projectile retains its selected item ID"), Arrow->GetShotItemId(), FName(TEXT("FangArrows")));
        TestEqual(TEXT("Fang Arrow projectile deals 50 Pierce"), Arrow->GetShotDamage(), 50.0f);
        Arrow->SetActorLocation(FangTarget->GetActorLocation());
        Arrow->Tick(0.0f);
    }
    TestTrue(TEXT("Selected Fang Arrow creates a server projectile"), bSawFangArrowShot);
    TestEqual(TEXT("Accepted Fang Arrow hit applies five seconds of weak poison"),
        FangTarget->GetFangArrowPoisonRemainingSeconds(), AKalmalaWildlifeSpawn::FangArrowWeakPoisonDurationSeconds);
    FangTarget->Tick(0.5f);
    TestEqual(TEXT("Fang poison retains the half-second time to its first tick"),
        FangTarget->GetFangArrowPoisonRemainingSeconds(), 4.5f);
    auto* RefreshArrow = World->SpawnActor<AKalmalaArrowProjectile>(Pawn->GetActorLocation(), FRotator::ZeroRotator);
    if (!TestNotNull(TEXT("Fang Arrow refresh projectile exists"), RefreshArrow)) return false;
    RefreshArrow->SetOwner(Pawn);
    TestTrue(TEXT("Server initializes a Fang Arrow refresh projectile"),
        RefreshArrow->InitializeShot(FVector::ForwardVector, 1.0f, TEXT("FangArrows")));
    RefreshArrow->SetActorLocation(FangTarget->GetActorLocation());
    RefreshArrow->Tick(0.0f);
    TestEqual(TEXT("Fang Arrow refresh restores duration without stacking"),
        FangTarget->GetFangArrowPoisonRemainingSeconds(), AKalmalaWildlifeSpawn::FangArrowWeakPoisonDurationSeconds);
    const float HealthAtRefresh = FangTarget->GetHealth();
    FangTarget->Tick(0.5f);
    TestEqual(TEXT("Refresh preserves the shared poison tick clock"), FangTarget->GetHealth(), HealthAtRefresh - 1.0f);
    for (int32 Tick = 0; Tick < 4; ++Tick) FangTarget->Tick(1.0f);
    const float HealthAfterFiveTicks = FangTarget->GetHealth();
    TestEqual(TEXT("Fang poison applies exactly five nonstacking one-damage ticks"),
        HealthAtRefresh - HealthAfterFiveTicks, 5.0f);
    FangTarget->Tick(1.0f);
    TestEqual(TEXT("Fang poison expires without a sixth tick"), FangTarget->GetHealth(), HealthAfterFiveTicks);
    TestEqual(TEXT("Fang poison timer clears after expiry"), FangTarget->GetFangArrowPoisonRemainingSeconds(), 0.0f);
    Combat->ServerPreferArrowStack(FGuid());
    Recover();

    FKalmalaToolState HornBow;
    HornBow.ItemId = TEXT("HornBow"); HornBow.InstanceGuid = FGuid::NewGuid(); HornBow.Durability = 140; HornBow.ToolLevel = 1;
    Tools.Add(HornBow); Inventory->SynchronizeGridFromServer();
    TestTrue(TEXT("Horn Bow equips through the shared two-handed server path"), Inventory->SetMainHandFromServer(HornBow.InstanceGuid, Reason));
    TestTrue(TEXT("Equipped Horn Bow can draw a server-available arrow"), Combat->CanDrawBow());
    TestEqual(TEXT("Horn Bow uses its authored three-second draw"), Combat->GetBowDrawDuration(), 3.0f);
    float HornWoodenArrowPreview = 0.0f;
    TestTrue(TEXT("Horn Bow resolves the selected Wooden Arrow preview"),
        Combat->ResolveCurrentBowShotDamage(TEXT("WoodenArrow"), 0.5f, HornWoodenArrowPreview));
    TestEqual(TEXT("Half-drawn Wooden Arrow preview includes the 100 Pierce bow bonus once"),
        HornWoodenArrowPreview, 125.0f);
    float HornStoneArrowPreview = 0.0f;
    TestTrue(TEXT("Horn Bow resolves fixed Stone Arrow damage"),
        AKalmalaArrowProjectile::ResolveShotDamage(TEXT("StoneArrow"), 0.0f, TEXT("HornBow"), 1, HornStoneArrowPreview));
    TestEqual(TEXT("Horn Bow adds its bonus to fixed Stone Arrow Pierce"), HornStoneArrowPreview, 125.0f);
    float HornFangArrowPreview = 0.0f;
    TestTrue(TEXT("Horn Bow resolves Fang Arrow damage"),
        AKalmalaArrowProjectile::ResolveShotDamage(TEXT("FangArrows"), 1.0f, TEXT("HornBow"), 1, HornFangArrowPreview));
    TestEqual(TEXT("Horn Bow adds its bonus to fixed Fang Arrow Pierce"), HornFangArrowPreview, 150.0f);
    const int32 WoodenBeforeHornShot = Inventory->GetQuantity(TEXT("WoodenArrow"));
    Combat->ServerBeginBowDraw(19);
    for (int32 Step = 0; Step < 6; ++Step) World->Tick(LEVELTICK_TimeOnly, 0.5f);
    TestEqual(TEXT("Three seconds fills the Horn Bow draw"), Combat->GetBowDrawProgress(), 1.0f);
    Combat->ServerReleaseBow(20, false);
    TestEqual(TEXT("Horn Bow commits one Wooden Arrow"), Inventory->GetQuantity(TEXT("WoodenArrow")), WoodenBeforeHornShot - 1);
    TestEqual(TEXT("Horn Bow consumes its own condition"), Pawn->GetToolDurability(TEXT("HornBow")), 139);
    bool bSawHornBowShot = false;
    for (TActorIterator<AKalmalaArrowProjectile> Arrow(World); Arrow; ++Arrow)
    {
        bSawHornBowShot = true;
        TestEqual(TEXT("Horn Bow shot retains the selected Wooden Arrow"), Arrow->GetShotItemId(), FName(TEXT("WoodenArrow")));
        TestEqual(TEXT("Full draw arrow curve plus Horn Bow bonus produces 140 Pierce"), Arrow->GetShotDamage(), 140.0f);
        Arrow->SetActorLocation(FangTarget->GetActorLocation());
        Arrow->Tick(0.0f);
    }
    TestTrue(TEXT("Horn Bow creates a server projectile"), bSawHornBowShot);
    TestTrue(TEXT("Horn Bow bonus reaches the accepted server impact"), FangTarget->GetHealth() < HealthAfterFiveTicks);
    Recover();
    const int32 AmmoBeforeFailure=Inventory->GetQuantity(TEXT("WoodenArrow"));
    const int32 ConditionBeforeFailure=Pawn->GetToolDurability(TEXT("HornBow"));
    Pawn->SetPlayerState(nullptr);
    Combat->ServerBeginBowDraw(21); Combat->ServerReleaseBow(22,false);
    TestEqual(TEXT("Persistence failure rolls back ammunition"),Inventory->GetQuantity(TEXT("WoodenArrow")),AmmoBeforeFailure);
    TestEqual(TEXT("Persistence failure rolls back Horn Bow condition"),Pawn->GetToolDurability(TEXT("HornBow")),ConditionBeforeFailure);
    Pawn->SetPlayerState(PlayerState);
    Combat->ServerBeginBowDraw(23);
    const auto Hammer=Tools.FindByPredicate([](const auto& Tool){return Tool.ItemId==TEXT("ConstructionHammer");});
    Inventory->SetMainHandFromServer(Hammer->InstanceGuid,Reason);
    Combat->TickComponent(0.01f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Swapping away cancels drawing"),Combat->IsBowDrawing());
    TestEqual(TEXT("Swap cancellation consumes no arrows"),Inventory->GetQuantity(TEXT("WoodenArrow")),AmmoBeforeFailure);
    return true;
}
#endif
