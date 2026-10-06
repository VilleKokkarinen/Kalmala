#if WITH_DEV_AUTOMATION_TESTS
#include "KalmalaCharacter.h"
#include "KalmalaCraftingComponent.h"
#include "KalmalaCraftingSubsystem.h"
#include "KalmalaPlacementPreview.h"
#include "KalmalaRecipeCatalogue.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaRecipeActivityReceiptTest,
    "Kalmala.UI.Crafting.ActivityReceiptReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaRecipeActivityReceiptTest::RunTest(const FString& Parameters)
{
    const UKalmalaRecipeCatalogue* Catalogue = UKalmalaRecipeCatalogue::Get();
    const TArray<FKalmalaRecipe> OriginalRecipes = Catalogue->Recipes;
    ON_SCOPE_EXIT
    {
        GetMutableDefault<UKalmalaRecipeCatalogue>()->Recipes = OriginalRecipes;
    };

    FKalmalaRecipe SyntheticCraftRecipe = Catalogue->Recipes.Last();
    SyntheticCraftRecipe.RecipeId = TEXT("ActivityReceiptTestCraft");
    SyntheticCraftRecipe.DisplayName = TEXT("Receipt test crafted item");
    SyntheticCraftRecipe.Output = TEXT("Wood");
    SyntheticCraftRecipe.ExperienceSkill = EKalmalaSkill::None;
    SyntheticCraftRecipe.ExperienceAward = 0;
    SyntheticCraftRecipe.RequiredStation.Reset();
    SyntheticCraftRecipe.RequiredTool = NAME_None;
    GetMutableDefault<UKalmalaRecipeCatalogue>()->Recipes.Add(SyntheticCraftRecipe);
    if (!TestTrue(TEXT("Synthetic other-craft receipt fixture satisfies active catalogue bounds"), Catalogue->IsValidCatalogue()))
        return false;

    FName BuildRecipeId = NAME_None;
    FName CookRecipeId = NAME_None;
    for (const FKalmalaRecipe& Recipe : Catalogue->Recipes)
    {
        if (FKalmalaPlacementPreview::IsSupportedKit(Recipe.Output))
            BuildRecipeId = BuildRecipeId.IsNone() ? Recipe.RecipeId : BuildRecipeId;
        else if (Recipe.ExperienceSkill == EKalmalaSkill::Cooking)
            CookRecipeId = CookRecipeId.IsNone() ? Recipe.RecipeId : CookRecipeId;
    }
    const FName CraftRecipeId = SyntheticCraftRecipe.RecipeId;
    if (!TestTrue(TEXT("Activity test resolves build, cooking, and fixture-crafted IDs"),
        !BuildRecipeId.IsNone() && !CookRecipeId.IsNone() && Catalogue->Find(CraftRecipeId) != nullptr)) return false;

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient activity receipt world created"), World)) return false;

    AKalmalaCharacter* ServerPawn = World->SpawnActor<AKalmalaCharacter>();
    ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    UKalmalaCraftingSubsystem* LocalState = LocalPlayer ? NewObject<UKalmalaCraftingSubsystem>(LocalPlayer) : nullptr;
    UKalmalaCraftingComponent* Crafting = ServerPawn
        ? ServerPawn->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    if (!TestNotNull(TEXT("Receipt local-player fixture created"), LocalPlayer)
        || !TestNotNull(TEXT("Receipt subsystem belongs to a local player"), LocalState)
        || !TestNotNull(TEXT("Server-owned crafting component spawned"), Crafting))
    {
        World->DestroyWorld(false);
        return false;
    }

    TestTrue(TEXT("Fixture uses an authoritative actor"), ServerPawn->HasAuthority());
    Crafting->PublishResultForTest(TEXT("Rejected"), false, CraftRecipeId, EKalmalaCraftingActionKind::CraftedItem);
    Crafting->PublishResultForTest(TEXT("Missing accepted identity"), true, NAME_None, EKalmalaCraftingActionKind::CraftedItem);
    TestEqual(TEXT("Rejected and identity-free outcomes publish no receipts"), Crafting->GetAcceptedCraftingActionReceipts().Num(), 0);

    Crafting->PublishResultForTest(TEXT("Accepted before first observation"), true, BuildRecipeId, EKalmalaCraftingActionKind::BuiltPiece);
    LocalState->ObserveRecipeActivityForTest(Crafting);
    TestEqual(TEXT("First observation baselines the existing queue without counting it"),
        LocalState->GetRecipeActivityCount(EKalmalaCraftingActionKind::BuiltPiece, BuildRecipeId), 0u);

    Crafting->PublishResultForTest(TEXT("Accepted build"), true, BuildRecipeId, EKalmalaCraftingActionKind::BuiltPiece);
    LocalState->ObserveRecipeActivityForTest(Crafting);
    TestEqual(TEXT("A new owner receipt increments the matching bucket once"),
        LocalState->GetRecipeActivityCount(EKalmalaCraftingActionKind::BuiltPiece, BuildRecipeId), 1u);
    TestEqual(TEXT("The accepted build becomes Recent"),
        LocalState->GetRecentRecipeActivity(EKalmalaCraftingActionKind::BuiltPiece), BuildRecipeId);
    LocalState->ObserveRecipeActivityForTest(Crafting);
    LocalState->ObserveRecipeActivityForTest(Crafting);
    TestEqual(TEXT("Replayed snapshots do not increment counts again"),
        LocalState->GetRecipeActivityCount(EKalmalaCraftingActionKind::BuiltPiece, BuildRecipeId), 1u);

    Crafting->PublishResultForTest(TEXT("Wrong bucket"), true, CookRecipeId, EKalmalaCraftingActionKind::CraftedItem);
    LocalState->ObserveRecipeActivityForTest(Crafting);
    TestEqual(TEXT("Cooking cannot enter other-crafting counts"),
        LocalState->GetRecipeActivityCount(EKalmalaCraftingActionKind::CraftedItem, CookRecipeId), 0u);
    TestTrue(TEXT("Wrong-bucket cooking does not replace crafted Recent"),
        LocalState->GetRecentRecipeActivity(EKalmalaCraftingActionKind::CraftedItem).IsNone());

    Crafting->PublishResultForTest(TEXT("Accepted cook"), true, CookRecipeId, EKalmalaCraftingActionKind::CookedRecipe);
    Crafting->PublishResultForTest(TEXT("Accepted craft"), true, CraftRecipeId, EKalmalaCraftingActionKind::CraftedItem);
    LocalState->ObserveRecipeActivityForTest(Crafting);
    TestEqual(TEXT("Cooking is counted only in the cooking bucket"),
        LocalState->GetRecipeActivityCount(EKalmalaCraftingActionKind::CookedRecipe, CookRecipeId), 1u);
    TestEqual(TEXT("Non-cooking recipes are counted in the crafted bucket"),
        LocalState->GetRecipeActivityCount(EKalmalaCraftingActionKind::CraftedItem, CraftRecipeId), 1u);
    TestEqual(TEXT("Recent state stays separate by action kind"),
        LocalState->GetRecentRecipeActivity(EKalmalaCraftingActionKind::CookedRecipe), CookRecipeId);
    TestEqual(TEXT("Crafted Recent is the latest accepted non-cooking recipe"),
        LocalState->GetRecentRecipeActivity(EKalmalaCraftingActionKind::CraftedItem), CraftRecipeId);

    AKalmalaCharacter* ClientPawn = World->SpawnActor<AKalmalaCharacter>();
    UKalmalaCraftingComponent* ClientCrafting = ClientPawn
        ? ClientPawn->FindComponentByClass<UKalmalaCraftingComponent>() : nullptr;
    if (TestNotNull(TEXT("Client-role crafting component spawned"), ClientCrafting))
    {
        ClientPawn->SetRole(ROLE_SimulatedProxy);
        ClientCrafting->PublishResultForTest(TEXT("Client-forged acceptance"), true, BuildRecipeId,
            EKalmalaCraftingActionKind::BuiltPiece);
        TestEqual(TEXT("A non-authority owner cannot publish accepted receipts"),
            ClientCrafting->GetAcceptedCraftingActionReceipts().Num(), 0);
    }

    World->DestroyWorld(false);
    return true;
}
#endif
