#include "KalmalaGameCatalogueLoader.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    enum class ECatalogueLoadState : uint8
    {
        NotStarted,
        Loading,
        Loaded,
        Failed
    };

    ECatalogueLoadState LoadState = ECatalogueLoadState::NotStarted;

    bool LoadCataloguesFromJson()
    {
        const FString JsonPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/GameCatalogues.json"));
        FString JsonText;
        if (!FFileHelper::LoadFileToString(JsonText, *JsonPath) || JsonText.IsEmpty() || JsonText.Len() > 262144)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not read bounded gameplay catalogue JSON at %s"), *JsonPath);
            return false;
        }

        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
        TSharedPtr<FJsonObject> Root;
        if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue JSON is malformed: %s"), *JsonPath);
            return false;
        }

        double SchemaVersion = 0.0;
        const TArray<TSharedPtr<FJsonValue>>* ItemValues = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* RecipeValues = nullptr;
        if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) || SchemaVersion != 1.0
            || !Root->TryGetArrayField(TEXT("items"), ItemValues)
            || !Root->TryGetArrayField(TEXT("recipes"), RecipeValues)
            || ItemValues == nullptr || RecipeValues == nullptr
            || ItemValues->IsEmpty() || ItemValues->Num() > UKalmalaItemCatalogue::MaxDefinitions
            || RecipeValues->IsEmpty() || RecipeValues->Num() > 32)
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue JSON has an unsupported schema or invalid catalogue bounds: %s"), *JsonPath);
            return false;
        }

        TArray<FKalmalaItemDefinition> ParsedItems;
        TArray<FKalmalaRecipe> ParsedRecipes;
        if (!FJsonObjectConverter::JsonArrayToUStruct<FKalmalaItemDefinition>(*ItemValues, &ParsedItems, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaRecipe>(*RecipeValues, &ParsedRecipes, 0, 0, true))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue JSON does not match the item and recipe definitions: %s"), *JsonPath);
            return false;
        }

        UKalmalaItemCatalogue* ItemCatalogue = GetMutableDefault<UKalmalaItemCatalogue>();
        UKalmalaRecipeCatalogue* RecipeCatalogue = GetMutableDefault<UKalmalaRecipeCatalogue>();
        ItemCatalogue->Items = MoveTemp(ParsedItems);
        RecipeCatalogue->Recipes = MoveTemp(ParsedRecipes);
        if (!ItemCatalogue->IsValidCatalogue() || !RecipeCatalogue->IsValidCatalogue())
        {
            ItemCatalogue->Items.Reset();
            RecipeCatalogue->Recipes.Reset();
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue JSON failed item or recipe validation: %s"), *JsonPath);
            return false;
        }

        UE_LOG(LogTemp, Log, TEXT("Loaded %d items and %d recipes from %s"),
            ItemCatalogue->Items.Num(), RecipeCatalogue->Recipes.Num(), *JsonPath);
        return true;
    }
}

void FKalmalaGameCatalogueLoader::EnsureLoaded()
{
    if (LoadState != ECatalogueLoadState::NotStarted)
    {
        return;
    }

    LoadState = ECatalogueLoadState::Loading;
    LoadState = LoadCataloguesFromJson() ? ECatalogueLoadState::Loaded : ECatalogueLoadState::Failed;
}
