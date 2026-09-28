#if WITH_DEV_AUTOMATION_TESTS

#include "KalmalaConstructionActor.h"
#include "KalmalaConstructionSaveGame.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaCharacter.h"
#include "KalmalaGameMode.h"
#include "KalmalaInventoryComponent.h"
#include "KalmalaItemCatalogue.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaSkillProgressionComponent.h"
#include "KalmalaStorageSaveGame.h"
#include "KalmalaWorldGenerationGameState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaM9RaisedStorageTest,
    "Kalmala.Gameplay.M9.RaisedStorage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaM9RaisedStorageTest::RunTest(const FString& Parameters)
{
    const auto* Items = GetDefault<UKalmalaItemCatalogue>();
    const auto* Recipes = GetDefault<UKalmalaRecipeCatalogue>();
    TestTrue(TEXT("Raised storage item and recipe catalogues validate"), Items->IsValidCatalogue() && Recipes->IsValidCatalogue());
    const auto* Kit = Items->FindItem(TEXT("RaisedStorageKit"));
    if (TestNotNull(TEXT("Raised chest kit has a canonical item definition"), Kit))
        TestEqual(TEXT("Raised chest stack remains bounded"), Kit->MaxStack, 5);

    const FKalmalaRecipe* Recipe = Recipes->Find(TEXT("RaisedStorage"));
    if (TestNotNull(TEXT("Raised chest has a canonical recipe"), Recipe))
    {
        TestEqual(TEXT("Workbench is selected by the server for raised chest assembly"), Recipe->RequiredStationKit, FName(TEXT("WorkbenchKit")));
        TestEqual(TEXT("One kit is produced per request"), Recipe->Output, FName(TEXT("RaisedStorageKit")));
        TestEqual(TEXT("Raised chest assembly is single-batch"), Recipe->MaxBatch, 1);
        TestEqual(TEXT("Construction does not award a skill level"), int32(Recipe->ExperienceAward), 0);
        const auto HasIngredient = [Recipe](const FName ItemId, const int32 Quantity)
        {
            const FKalmalaInventoryStack* Ingredient = Recipe->Ingredients.FindByPredicate(
                [ItemId](const FKalmalaInventoryStack& Stack) { return Stack.ItemId == ItemId; });
            return Ingredient && Ingredient->Quantity == Quantity;
        };
        TestEqual(TEXT("Recipe contains only the three approved material stacks"), Recipe->Ingredients.Num(), 3);
        TestTrue(TEXT("Recipe costs three Densewood"), HasIngredient(TEXT("Densewood"), 3));
        TestTrue(TEXT("Recipe costs two construction supplies"), HasIngredient(TEXT("ConstructionSupply"), 2));
        TestTrue(TEXT("Recipe costs two Fibre"), HasIngredient(TEXT("Fibre"), 2));
    }

    TestTrue(TEXT("Raised chest uses the existing placement path"), FKalmalaPlacementPreview::IsSupportedKit(TEXT("RaisedStorageKit")));
    TestTrue(TEXT("Raised chest remains session-only before M9 migration"), FKalmalaPlacementPreview::IsSessionOnlyKit(TEXT("RaisedStorageKit")));
    TestTrue(TEXT("Both chest kits share the storage interaction path"), AKalmalaConstructionActor::IsStorageKit(TEXT("StorageKit"))
        && AKalmalaConstructionActor::IsStorageKit(TEXT("RaisedStorageKit")));
    TestEqual(TEXT("Existing construction schema remains version one"), UKalmalaConstructionSaveGame::CurrentSchemaVersion, 1);
    FKalmalaConstructionSaveRecord SaveRecord;
    SaveRecord.ConstructionId = TEXT("raised-session-only");
    SaveRecord.KitId = TEXT("RaisedStorageKit");
    SaveRecord.Transform = FTransform(FVector(100, 200, 300));
    TestFalse(TEXT("Schema one rejects the new raised chest kit"), UKalmalaConstructionSaveGame::IsValidRecord(SaveRecord));

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Raised storage test world created"), World)) return false;
    World->SetGameState(World->SpawnActor<AKalmalaWorldGenerationGameState>());
    AKalmalaGameMode* Mode = World->SpawnActor<AKalmalaGameMode>();
    AKalmalaConstructionActor* Chest = World->SpawnActor<AKalmalaConstructionActor>();
    AKalmalaCharacter* Pawn = World->SpawnActor<AKalmalaCharacter>();
    APlayerController* Controller = World->SpawnActor<APlayerController>();
    if (!Mode || !Chest || !Pawn || !Controller)
    {
        AddError(TEXT("Raised storage world fixture spawn failed"));
        World->DestroyWorld(false);
        return false;
    }
    Chest->SetActorLocation(FVector(1200.0f, 800.0f, 0.0f));
    Controller->Possess(Pawn);
    UKalmalaCraftingComponent* Crafting = Pawn->FindComponentByClass<UKalmalaCraftingComponent>();
    UKalmalaInventoryComponent* Inventory = Pawn->GetInventoryComponent();
    if (!Crafting || !Inventory)
    {
        AddError(TEXT("Raised storage pawn lacks crafting or inventory components"));
        World->DestroyWorld(false);
        return false;
    }
    TestTrue(TEXT("Fixture receives the exact raised chest materials"), Inventory->TryGrantFromServer(TEXT("Densewood"), 3)
        && Inventory->TryGrantFromServer(TEXT("ConstructionSupply"), 2) && Inventory->TryGrantFromServer(TEXT("Fibre"), 2));
    FString Reason;
    TestFalse(TEXT("Raised chest crafting rejects a missing Workbench"), Crafting->CraftFromServer(TEXT("RaisedStorage"), 1, Reason));
    TestEqual(TEXT("Missing station preserves Densewood"), Inventory->GetQuantity(TEXT("Densewood")), 3);
    AKalmalaConstructionActor* Workbench = World->SpawnActor<AKalmalaConstructionActor>();
    if (!TestNotNull(TEXT("Raised chest Workbench fixture spawned"), Workbench))
    {
        World->DestroyWorld(false);
        return false;
    }
    Workbench->SetActorLocation(Pawn->GetActorLocation() + FVector(400.0f, 0.0f, 0.0f));
    Workbench->InitializeFromServer(TEXT("WorkbenchKit"), TEXT("raised-chest-workbench"));
    TestFalse(TEXT("Raised chest crafting rejects a distant Workbench"), Crafting->CraftFromServer(TEXT("RaisedStorage"), 1, Reason));
    Workbench->SetActorLocation(Pawn->GetActorLocation() + FVector(0.0f, 120.0f, 0.0f));
    TestTrue(TEXT("Raised chest Workbench passes server visibility and access"), Workbench->CanUse(Pawn));
    TestFalse(TEXT("Raised chest recipe rejects a batch above one"), Crafting->CraftFromServer(TEXT("RaisedStorage"), 2, Reason));
    TestEqual(TEXT("Malformed batch preserves all construction cost"), Inventory->GetQuantity(TEXT("Fibre")), 2);
    TestTrue(TEXT("Visible Workbench accepts the paid raised chest recipe"), Crafting->CraftFromServer(TEXT("RaisedStorage"), 1, Reason));
    TestEqual(TEXT("Accepted raised chest spends its Densewood"), Inventory->GetQuantity(TEXT("Densewood")), 0);
    TestEqual(TEXT("Accepted raised chest spends its construction supplies"), Inventory->GetQuantity(TEXT("ConstructionSupply")), 0);
    TestEqual(TEXT("Accepted raised chest spends its Fibre"), Inventory->GetQuantity(TEXT("Fibre")), 0);
    TestEqual(TEXT("Accepted raised chest yields one kit"), Inventory->GetQuantity(TEXT("RaisedStorageKit")), 1);

    Chest->InitializeFromServer(TEXT("RaisedStorageKit"), TEXT("raised-session-only"));
    const float HealthBeforeRain = Chest->GetHealth();
    Chest->AdvanceRainWearFromServer(1000.0f, 1.0f);
    TestEqual(TEXT("Raised chest is exempt from server rain wear"), Chest->GetHealth(), HealthBeforeRain);
    TestTrue(TEXT("Raised chest is not registered until accepted placement"), Mode->CanRegisterSessionStorage());
    TArray<FKalmalaInventoryStack> Contents;
    TestFalse(TEXT("Unregistered raised actor cannot open storage"), Mode->ReadStorage(Chest, Contents));
    TestTrue(TEXT("Accepted server placement registers bounded session storage"), Mode->RegisterSessionStorage(Chest));
    TestTrue(TEXT("New session chest opens empty"), Mode->ReadStorage(Chest, Contents));
    TestTrue(TEXT("New session chest starts without contents"), Contents.IsEmpty());
    TestTrue(TEXT("Existing bounded item stacks can be stored in the session chest"),
        Mode->PersistStorage(Chest, {{TEXT("Wood"), 3}, {TEXT("Stone"), 2}}));
    TestTrue(TEXT("Session chest contents can be read by its server owner"), Mode->ReadStorage(Chest, Contents));
    TestEqual(TEXT("Session chest retains the bounded stored stacks"), Contents.Num(), 2);
    if (Contents.Num() == 2)
    {
        TestEqual(TEXT("Session chest keeps the exact first item count"), Contents[0].Quantity, 3);
        TestEqual(TEXT("Session chest keeps the exact second item count"), Contents[1].Quantity, 2);
    }
    AKalmalaConstructionActor* Impostor = World->SpawnActor<AKalmalaConstructionActor>();
    if (TestNotNull(TEXT("Forged raised chest actor spawned"), Impostor))
    {
        Impostor->InitializeFromServer(TEXT("RaisedStorageKit"), TEXT("raised-session-only"));
        Contents.Reset();
        TestFalse(TEXT("A different actor cannot forge the registered chest identity"), Mode->ReadStorage(Impostor, Contents));
        TestTrue(TEXT("Forged actor rejection leaves the read output empty"), Contents.IsEmpty());
    }

    AKalmalaGameMode* FreshMode = World->SpawnActor<AKalmalaGameMode>();
    TestFalse(TEXT("A fresh server session has no restored M9 chest record"), FreshMode && FreshMode->ReadStorage(Chest, Contents));
    TestEqual(TEXT("Existing storage contents retain schema version one"), UKalmalaStorageSaveGame::CurrentSchemaVersion, 1);
    TestFalse(TEXT("Storage saves continue to reject unknown item identities"),
        UKalmalaStorageSaveGame::IsValidStacks({{TEXT("Forged"), 1}}));

    World->DestroyWorld(false);
    return true;
}

#endif
