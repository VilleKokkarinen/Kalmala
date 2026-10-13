#include "KalmalaGameCatalogueLoader.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolCatalogue.h"
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
        if (CatalogueItemId == TEXT("CampFire")) return TEXT("CampfireKit");
        if (CatalogueItemId == TEXT("Workbench")) return TEXT("WorkbenchKit");
        if (CatalogueItemId == TEXT("Forge")) return TEXT("ForgeKit");
        if (CatalogueItemId == TEXT("WorkbenchToolRack")) return TEXT("WorkbenchToolRackKit");
        if (CatalogueItemId == TEXT("ForgeAnvil")) return TEXT("ForgeAnvilKit");
        if (CatalogueItemId == TEXT("GrindingStone")) return TEXT("GrindingStoneKit");
        if (CatalogueItemId == TEXT("Storage")) return TEXT("StorageKit");
        if (CatalogueItemId == TEXT("CookingRack")) return TEXT("CookingRackKit");
        if (CatalogueItemId == TEXT("Cauldron")) return TEXT("CauldronKit");
        if (CatalogueItemId == TEXT("FryingPan")) return TEXT("FryingPanKit");
        if (CatalogueItemId == TEXT("SmokeFrame")) return TEXT("SmokeFrameKit");
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

    bool NormalizeRequiredTool(const TSharedPtr<FJsonObject>& RecipeObject)
    {
        if (!RecipeObject.IsValid()) return false;
        if (!RecipeObject->HasField(TEXT("RequiredTool")))
        {
            RecipeObject->SetStringField(TEXT("RequiredTool"), TEXT(""));
            return true;
        }
        FString CatalogueId;
        if (!RecipeObject->TryGetStringField(TEXT("RequiredTool"), CatalogueId) || CatalogueId.IsEmpty()) return false;
        RecipeObject->SetStringField(TEXT("RequiredTool"), ToRuntimeItemId(FName(*CatalogueId)).ToString());
        return true;
    }

    bool NormalizeRecipeOutput(const TSharedPtr<FJsonObject>& RecipeObject)
    {
        FString CatalogueId;
        if (!RecipeObject.IsValid() || !RecipeObject->TryGetStringField(TEXT("Output"), CatalogueId))
        {
            return false;
        }

        const FName RuntimeId = ToRuntimeItemId(FName(*CatalogueId));
        if (UKalmalaRecipeCatalogue::IsDirectMaterialBuildable(RuntimeId))
        {
            RecipeObject->SetStringField(TEXT("Output"), TEXT(""));
            RecipeObject->SetStringField(TEXT("BuildableOutput"), RuntimeId.ToString());
        }
        else
        {
            RecipeObject->SetStringField(TEXT("Output"), RuntimeId.ToString());
            RecipeObject->SetStringField(TEXT("BuildableOutput"), TEXT(""));
        }
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
            const auto Item = Value->AsObject();
            // Keep weight optional in schema-4 content while retaining strict
            // conversion and validation for every authored field.
            if (!Item->HasField(TEXT("WeightKg"))) Item->SetNumberField(TEXT("WeightKg"), 1.0);
            double Weight = 0.0;
            if (!Item->TryGetNumberField(TEXT("WeightKg"), Weight)
                || !FMath::IsFinite(Weight) || Weight < 0.0 || Weight > 300.0) return false;
        }

        for (const TSharedPtr<FJsonValue>& Value : RecipeValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object)
            {
                return false;
            }
            const TSharedPtr<FJsonObject> RecipeObject = Value->AsObject();
            if (!NormalizeRecipeOutput(RecipeObject)
                || !NormalizeStationArray(RecipeObject)
                || !NormalizeRequiredTool(RecipeObject))
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

    bool IsValidCategoryPath(const FString& RelativePath, const TCHAR* ExpectedDirectory, FString& OutNormalizedPath)
    {
        OutNormalizedPath = RelativePath.Replace(TEXT("\\"), TEXT("/"));
        if (OutNormalizedPath.IsEmpty() || OutNormalizedPath.StartsWith(TEXT("/"))
            || OutNormalizedPath.EndsWith(TEXT("/")) || OutNormalizedPath.Contains(TEXT(":")))
        {
            return false;
        }

        TArray<FString> PathParts;
        OutNormalizedPath.ParseIntoArray(PathParts, TEXT("/"), false);
        if (PathParts.Num() != 2 || PathParts[0] != ExpectedDirectory
            || PathParts[1].IsEmpty() || !PathParts[1].EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
        {
            return false;
        }

        for (const FString& Part : PathParts)
        {
            if (Part == TEXT(".") || Part == TEXT("..")) return false;
            for (const TCHAR Character : Part)
            {
                const bool bAllowed = (Character >= TEXT('a') && Character <= TEXT('z'))
                    || (Character >= TEXT('A') && Character <= TEXT('Z'))
                    || (Character >= TEXT('0') && Character <= TEXT('9'))
                    || Character == TEXT('-') || Character == TEXT('_') || Character == TEXT('.');
                if (!bAllowed) return false;
            }
        }
        return true;
    }

    bool LoadCategoryFiles(const TSharedPtr<FJsonObject>& Manifest, const TCHAR* ManifestField,
        const TCHAR* ArrayField, const TCHAR* ExpectedDirectory, int32 MaxEntries,
        int32& InOutJsonCharacters, TArray<TSharedPtr<FJsonValue>>& OutValues)
    {
        const TArray<TSharedPtr<FJsonValue>>* FileValues = nullptr;
        if (!Manifest.IsValid() || !Manifest->TryGetArrayField(ManifestField, FileValues)
            || FileValues == nullptr || FileValues->IsEmpty() || FileValues->Num() > 16)
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest has an invalid file list: %s"), ManifestField);
            return false;
        }

        OutValues.Reset();
        for (const TSharedPtr<FJsonValue>& FileValue : *FileValues)
        {
            if (!FileValue.IsValid() || FileValue->Type != EJson::String)
            {
                UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest contains a non-string file path: %s"), ManifestField);
                return false;
            }

            FString RelativePath;
            if (!IsValidCategoryPath(FileValue->AsString(), ExpectedDirectory, RelativePath))
            {
                UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest contains an unsafe category path: %s"),
                    *FileValue->AsString());
                return false;
            }

            FString CategoryPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/GameCatalogues"));
            CategoryPath = FPaths::Combine(CategoryPath, RelativePath);
            FString CategoryText;
            if (!FFileHelper::LoadFileToString(CategoryText, *CategoryPath) || CategoryText.IsEmpty()
                || CategoryText.Len() > 262144 - InOutJsonCharacters)
            {
                UE_LOG(LogTemp, Error, TEXT("Could not read bounded gameplay catalogue category at %s"), *CategoryPath);
                return false;
            }
            InOutJsonCharacters += CategoryText.Len();

            const TSharedRef<TJsonReader<>> CategoryReader = TJsonReaderFactory<>::Create(CategoryText);
            TSharedPtr<FJsonObject> CategoryRoot;
            const TArray<TSharedPtr<FJsonValue>>* CategoryValues = nullptr;
            if (!FJsonSerializer::Deserialize(CategoryReader, CategoryRoot) || !CategoryRoot.IsValid()
                || !CategoryRoot->TryGetArrayField(ArrayField, CategoryValues) || CategoryValues == nullptr
                || CategoryValues->IsEmpty() || OutValues.Num() + CategoryValues->Num() > MaxEntries)
            {
                UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue category is malformed or exceeds its bounds: %s"),
                    *CategoryPath);
                return false;
            }
            OutValues.Append(*CategoryValues);
        }
        return true;
    }

    bool OrderCategoryValues(const TSharedPtr<FJsonObject>& Manifest, const TCHAR* OrderField,
        const TCHAR* IdField, TArray<TSharedPtr<FJsonValue>>& InOutValues)
    {
        const TArray<TSharedPtr<FJsonValue>>* OrderedIds = nullptr;
        if (!Manifest.IsValid() || !Manifest->TryGetArrayField(OrderField, OrderedIds)
            || OrderedIds == nullptr || OrderedIds->Num() != InOutValues.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest has an invalid ordering list: %s"), OrderField);
            return false;
        }

        TMap<FName, TSharedPtr<FJsonValue>> ValuesById;
        for (const TSharedPtr<FJsonValue>& Value : InOutValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object) return false;
            FString IdString;
            if (!Value->AsObject()->TryGetStringField(IdField, IdString) || IdString.IsEmpty()) return false;
            const FName Id(*IdString);
            if (Id.IsNone() || ValuesById.Contains(Id)) return false;
            ValuesById.Add(Id, Value);
        }

        TArray<TSharedPtr<FJsonValue>> OrderedValues;
        OrderedValues.Reserve(OrderedIds->Num());
        for (const TSharedPtr<FJsonValue>& OrderedIdValue : *OrderedIds)
        {
            if (!OrderedIdValue.IsValid() || OrderedIdValue->Type != EJson::String
                || OrderedIdValue->AsString().IsEmpty()) return false;
            const FName Id(*OrderedIdValue->AsString());
            TSharedPtr<FJsonValue>* Value = ValuesById.Find(Id);
            if (Value == nullptr) return false;
            OrderedValues.Add(*Value);
            ValuesById.Remove(Id);
        }

        if (!ValuesById.IsEmpty()) return false;
        InOutValues = MoveTemp(OrderedValues);
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
        if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) || SchemaVersion != 6.0)
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest has an unsupported schema: %s"), *JsonPath);
            return false;
        }

        int32 TotalJsonCharacters = JsonText.Len();
        TArray<TSharedPtr<FJsonValue>> ItemValues;
        TArray<TSharedPtr<FJsonValue>> RecipeValues;
        TArray<TSharedPtr<FJsonValue>> ToolProfileValues;
        TArray<TSharedPtr<FJsonValue>> ToolRecipeValues;
        TArray<TSharedPtr<FJsonValue>> ToolUpgradeValues;
        if (!LoadCategoryFiles(Root, TEXT("itemFiles"), TEXT("items"), TEXT("items"),
                UKalmalaItemCatalogue::MaxDefinitions, TotalJsonCharacters, ItemValues)
            || !LoadCategoryFiles(Root, TEXT("recipeFiles"), TEXT("recipes"), TEXT("recipes"),
                32, TotalJsonCharacters, RecipeValues)
            || !OrderCategoryValues(Root, TEXT("recipeOrder"), TEXT("RecipeId"), RecipeValues)
            || !LoadCategoryFiles(Root, TEXT("toolProfileFiles"), TEXT("toolProfiles"), TEXT("items"),
                UKalmalaToolCatalogue::MaxProfiles, TotalJsonCharacters, ToolProfileValues)
            || !LoadCategoryFiles(Root, TEXT("toolFiles"), TEXT("toolProfiles"), TEXT("tools"),
                UKalmalaToolCatalogue::MaxProfiles, TotalJsonCharacters, ToolProfileValues)
            || !LoadCategoryFiles(Root, TEXT("toolRecipeFiles"), TEXT("toolRecipes"), TEXT("recipes"),
                UKalmalaToolCatalogue::MaxRecipes, TotalJsonCharacters, ToolRecipeValues)
            || !LoadCategoryFiles(Root, TEXT("toolUpgradeFiles"), TEXT("toolUpgradeCosts"), TEXT("recipes"),
                UKalmalaToolCatalogue::MaxRecipes, TotalJsonCharacters, ToolUpgradeValues)
            || !OrderCategoryValues(Root, TEXT("toolProfileOrder"), TEXT("ToolId"), ToolProfileValues)
            || !OrderCategoryValues(Root, TEXT("toolRecipeOrder"), TEXT("RecipeId"), ToolRecipeValues))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest could not load its category files: %s"), *JsonPath);
            return false;
        }

        if (ItemValues.IsEmpty() || RecipeValues.IsEmpty() || ToolProfileValues.IsEmpty() || ToolRecipeValues.IsEmpty()
            || !NormalizeCatalogueItemReferences(ItemValues, RecipeValues))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue category data has invalid clean item or station references: %s"), *JsonPath);
            return false;
        }

        TArray<FKalmalaItemDefinition> ParsedItems;
        TArray<FKalmalaRecipe> ParsedRecipes;
        TArray<FKalmalaToolProfileData> ParsedToolProfiles;
        TArray<FKalmalaToolRecipeData> ParsedToolRecipes;
        TArray<FKalmalaToolRecipeData> ParsedUpgradeCosts;
        if (!FJsonObjectConverter::JsonArrayToUStruct<FKalmalaItemDefinition>(ItemValues, &ParsedItems, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaRecipe>(RecipeValues, &ParsedRecipes, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaToolProfileData>(
                ToolProfileValues, &ParsedToolProfiles, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaToolRecipeData>(
                ToolRecipeValues, &ParsedToolRecipes, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaToolRecipeData>(
                ToolUpgradeValues, &ParsedUpgradeCosts, 0, 0, true))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue data does not match its item, recipe, and tool definitions: %s"), *JsonPath);
            return false;
        }

        UKalmalaItemCatalogue* ItemCatalogue = GetMutableDefault<UKalmalaItemCatalogue>();
        UKalmalaRecipeCatalogue* RecipeCatalogue = GetMutableDefault<UKalmalaRecipeCatalogue>();
        UKalmalaToolCatalogue* ToolCatalogue = GetMutableDefault<UKalmalaToolCatalogue>();
        ItemCatalogue->Items = MoveTemp(ParsedItems);
        RecipeCatalogue->Recipes = MoveTemp(ParsedRecipes);
        ToolCatalogue->Profiles = MoveTemp(ParsedToolProfiles);
        ToolCatalogue->Recipes = MoveTemp(ParsedToolRecipes);
        ToolCatalogue->UpgradeCosts = MoveTemp(ParsedUpgradeCosts);
        if (!ItemCatalogue->IsValidCatalogue() || !RecipeCatalogue->IsValidCatalogue()
            || !ToolCatalogue->IsValidCatalogue(ItemCatalogue))
        {
            ItemCatalogue->Items.Reset();
            RecipeCatalogue->Recipes.Reset();
            ToolCatalogue->Profiles.Reset();
            ToolCatalogue->Recipes.Reset();
            ToolCatalogue->UpgradeCosts.Reset();
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue JSON failed item, recipe, or tool validation: %s"), *JsonPath);
            return false;
        }

        UE_LOG(LogTemp, Log, TEXT("Loaded %d items, %d recipes, %d tool profiles, and %d tool recipes from %s"),
            ItemCatalogue->Items.Num(), RecipeCatalogue->Recipes.Num(), ToolCatalogue->Profiles.Num(),
            ToolCatalogue->Recipes.Num(), *JsonPath);
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
