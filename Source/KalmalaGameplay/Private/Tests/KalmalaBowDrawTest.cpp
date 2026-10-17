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
    const FUniqueNetIdStringRef NetId = FUniqueNetIdString::Create(TEXT("bow-acceptance"), FName(TEXT("KalmalaBowTest")));
    const FUniqueNetIdRef BaseNetId = NetId;
    PlayerState->SetUniqueId(FUniqueNetIdRepl(BaseNetId));
    Pawn->SetPlayerState(PlayerState);
    auto* Inventory = Pawn->GetInventoryComponent();
    auto* Combat = Pawn->GetCombatComponent();
    Combat->ServerRequestAttack(1);
    TestEqual(TEXT("Held hammer cannot attack"), Combat->GetActionSerial(), uint32(0));
    auto& Tools = const_cast<TArray<FKalmalaToolState>&>(Pawn->GetCarriedToolInventory());
    FKalmalaToolState Bow;
    Bow.ItemId = TEXT("Bow"); Bow.InstanceGuid = FGuid::NewGuid(); Bow.Durability = 100; Bow.ToolLevel = 1;
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
    Combat->ServerPreferArrowStack(FGuid::NewGuid());
    TestEqual(TEXT("Forged stack request does not change preference"), Combat->GetPreferredArrowStack(), Preferred);
    TestEqual(TEXT("Draw duration loads from catalogue"), Combat->GetBowDrawDuration(), 1.5f);
    Combat->ServerRequestAttack(1);
    TestEqual(TEXT("Bow has no melee attack"), Combat->GetActionSerial(), uint32(0));
    Combat->ServerBeginBowDraw(2);
    TestTrue(TEXT("Press starts a target-free bow draw"), Combat->IsBowDrawing());
    for (int32 Step=0; Step<3; ++Step) World->Tick(LEVELTICK_TimeOnly, 0.25f);
    TestEqual(TEXT("Server elapsed time controls half draw"), Combat->GetBowDrawProgress(), 0.5f);
    Combat->ServerReleaseBow(3, false);
    TestFalse(TEXT("Release ends drawing"), Combat->IsBowDrawing());
    TestEqual(TEXT("One arrow consumed"), Inventory->GetQuantity(TEXT("WoodenArrow")), 199);
    TestEqual(TEXT("Preferred stack used before the first stack"), Inventory->GetStacks()[0].Quantity, 100);
    TestEqual(TEXT("Preferred stack loses exactly one unit"), Inventory->GetStacks()[1].Quantity, 99);
    TestEqual(TEXT("One bow condition consumed"), Pawn->GetToolDurability(TEXT("Bow")), 99);
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
    TestEqual(TEXT("Cancel does not wear the bow"), Pawn->GetToolDurability(TEXT("Bow")), 99);
    Combat->ServerPreferArrowStack(FGuid());
    TestFalse(TEXT("Automatic arrow selection can be restored"), Combat->GetPreferredArrowStack().IsValid());
    Combat->ServerBeginBowDraw(7);
    for (int32 Step=0; Step<6; ++Step) World->Tick(LEVELTICK_TimeOnly,0.25f);
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
    Recover();
    Combat->ServerBeginBowDraw(9);
    Combat->ServerReleaseBow(10,false);
    for(TActorIterator<AKalmalaArrowProjectile> Arrow(World);Arrow;++Arrow)
    {TestEqual(TEXT("Immediate release still fires a low-power arrow"),Arrow->GetShotDamage(),10.0f);Arrow->Destroy();}
    Recover();
    const int32 AmmoBeforeFailure=Inventory->GetQuantity(TEXT("WoodenArrow"));
    const int32 ConditionBeforeFailure=Pawn->GetToolDurability(TEXT("Bow"));
    Pawn->SetPlayerState(nullptr);
    Combat->ServerBeginBowDraw(11); Combat->ServerReleaseBow(12,false);
    TestEqual(TEXT("Persistence failure rolls back ammunition"),Inventory->GetQuantity(TEXT("WoodenArrow")),AmmoBeforeFailure);
    TestEqual(TEXT("Persistence failure rolls back bow condition"),Pawn->GetToolDurability(TEXT("Bow")),ConditionBeforeFailure);
    Pawn->SetPlayerState(PlayerState);
    Combat->ServerBeginBowDraw(13);
    const auto Hammer=Tools.FindByPredicate([](const auto& Tool){return Tool.ItemId==TEXT("ConstructionHammer");});
    Inventory->SetMainHandFromServer(Hammer->InstanceGuid,Reason);
    Combat->TickComponent(0.01f,LEVELTICK_All,nullptr);
    TestFalse(TEXT("Swapping away cancels drawing"),Combat->IsBowDrawing());
    TestEqual(TEXT("Swap cancellation consumes no arrows"),Inventory->GetQuantity(TEXT("WoodenArrow")),AmmoBeforeFailure);
    return true;
}
#endif
