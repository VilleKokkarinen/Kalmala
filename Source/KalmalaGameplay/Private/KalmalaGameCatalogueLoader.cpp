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

    FName ToRuntimeItemId(const FName CatalogueItemId)
    {
        if (CatalogueItemId == TEXT("HearthRing")) return TEXT("CampfireKit");
        if (CatalogueItemId == TEXT("Workbench")) return TEXT("WorkbenchKit");
        if (CatalogueItemId == TEXT("Forge")) return TEXT("ForgeKit");
        if (CatalogueItemId == TEXT("WorkbenchToolRack")) return TEXT("WorkbenchToolRackKit");
        if (CatalogueItemId == TEXT("ForgeAnvil")) return TEXT("ForgeAnvilKit");
        if (CatalogueItemId == TEXT("GrindingStone")) return TEXT("GrindingStoneKit");
        if (CatalogueItemId == TEXT("Storage")) return TEXT("StorageKit");
        if (CatalogueItemId == TEXT("CookingRack")) return TEXT("CookingRackKit");
        if (CatalogueItemId == TEXT("Cauldron")) return TEXT("CauldronKit");
        if (CatalogueItemId == TEXT("SmokeFrame")) return TEXT("SmokeFrameKit");
        if (CatalogueItemId == TEXT("DryingLine")) return TEXT("DryingLineKit");
        if (CatalogueItemId == TEXT("Floor")) return TEXT("FloorKit");
        if (CatalogueItemId == TEXT("Wall")) return TEXT("WallKit");
        if (CatalogueItemId == TEXT("Roof")) return TEXT("RoofKit");
        return CatalogueItemId;
    }

    bool NormalizeItemReference(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName)
    {
        FString CatalogueId;
        if (!Object.IsValid() || !Object->TryGetStringField(FieldName, CatalogueId))
        {
            return false;
        }

        if (!CatalogueId.IsEmpty())
        {
            Object->SetStringField(FieldName, ToRuntimeItemId(FName(*CatalogueId)).ToString());
        }
        return true;
    }

    bool NormalizeStationArray(const TSharedPtr<FJsonObject>& RecipeObject)
    {
        const TArray<TSharedPtr<FJsonValue>>* StationValues = nullptr;
        if (!RecipeObject.IsValid() || !RecipeObject->TryGetArrayField(TEXT("RequiredStation"), StationValues)
            || StationValues == nullptr || StationValues->Num() > 4)
        {
            return false;
        }
        TArray<TSharedPtr<FJsonValue>> NormalizedStations;
        NormalizedStations.Reserve(StationValues->Num());
        for (const TSharedPtr<FJsonValue>& Value : *StationValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::String) return false;
            const FString CatalogueId = Value->AsString();
            if (CatalogueId.IsEmpty()) return false;
            NormalizedStations.Add(MakeShared<FJsonValueString>(
                ToRuntimeItemId(FName(*CatalogueId)).ToString()));
        }
        RecipeObject->SetArrayField(TEXT("RequiredStation"), MoveTemp(NormalizedStations));
        return true;
    }

    bool NormalizeCatalogueItemReferences(const TArray<TSharedPtr<FJsonValue>>& ItemValues,
        const TArray<TSharedPtr<FJsonValue>>& RecipeValues)
    {
        for (const TSharedPtr<FJsonValue>& Value : ItemValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object
                || !NormalizeItemReference(Value->AsObject(), TEXT("ItemId")))
            {
                return false;
            }
        }

        for (const TSharedPtr<FJsonValue>& Value : RecipeValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object)
            {
                return false;
            }
            const TSharedPtr<FJsonObject> RecipeObject = Value->AsObject();
            if (!NormalizeItemReference(RecipeObject, TEXT("Output"))
                || !NormalizeStationArray(RecipeObject))
            {
                return false;
            }

            const TArray<TSharedPtr<FJsonValue>>* IngredientValues = nullptr;
            if (!RecipeObject->TryGetArrayField(TEXT("Ingredients"), IngredientValues) || IngredientValues == nullptr)
            {
                return false;
            }
            for (const TSharedPtr<FJsonValue>& IngredientValue : *IngredientValues)
            {
                if (!IngredientValue.IsValid() || IngredientValue->Type != EJson::Object
                    || !NormalizeItemReference(IngredientValue->AsObject(), TEXT("ItemId")))
                {
                    return false;
                }
            }
        }
        return true;
    }

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
        if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) || SchemaVersion != 4.0
            || !Root->TryGetArrayField(TEXT("items"), ItemValues)
            || !Root->TryGetArrayField(TEXT("recipes"), RecipeValues)
            || ItemValues == nullptr || RecipeValues == nullptr
            || ItemValues->IsEmpty() || ItemValues->Num() > UKalmalaItemCatalogue::MaxDefinitions
            || RecipeValues->IsEmpty() || RecipeValues->Num() > 32)
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue JSON has an unsupported schema or invalid catalogue bounds: %s"), *JsonPath);
            return false;
        }

        if (!NormalizeCatalogueItemReferences(*ItemValues, *RecipeValues))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue JSON has invalid clean item or station references: %s"), *JsonPath);
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
