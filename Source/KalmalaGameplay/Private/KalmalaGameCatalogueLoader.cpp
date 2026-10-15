#include "KalmalaGameCatalogueLoader.h"

#include "KalmalaItemCatalogue.h"
#include "KalmalaRecipeCatalogue.h"
#include "KalmalaToolCatalogue.h"
#include "Containers/Set.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    // Item authoring has one identity; legacy runtime fields are derived for shared equipment contracts.
    bool NormalizeEquipmentProfiles(const TArray<TSharedPtr<FJsonValue>>& Values)
    {
        for (const TSharedPtr<FJsonValue>& Value : Values)
        {
            const TSharedPtr<FJsonObject> Object = Value->AsObject();
            FString ItemId;
            if (!Object.IsValid() || !Object->TryGetStringField(TEXT("ItemId"), ItemId)) return false;
            for (const TCHAR* Retired : { TEXT("ToolId"), TEXT("ToolSet"), TEXT("ToolKind"),
                TEXT("Action"), TEXT("RequiredSkill"), TEXT("ItemCategory") })
            {
                if (Object->HasField(Retired)) return false;
            }
            FString Set;
            FString Action = TEXT("None");
            if (ItemId == TEXT("ConstructionHammer")) Set = TEXT("Utility");
            else
            {
                FString Family;
                if (ItemId.StartsWith(TEXT("Stone"))) { Set = TEXT("Starter"); Family = ItemId.Mid(5); }
                else if (ItemId.StartsWith(TEXT("Bronze"))) { Set = TEXT("Crafted"); Family = ItemId.Mid(6); }
                else if (ItemId.StartsWith(TEXT("Iron"))) { Set = TEXT("Crafted"); Family = ItemId.Mid(4); }
                else return false;
                if (Family == TEXT("Axe")) Action = TEXT("Woodcutting");
                else if (Family == TEXT("Pick")) Action = TEXT("Mining");
                else if (Family == TEXT("Knife")) Action = TEXT("Gathering");
                else if (Family != TEXT("Mace") && Family != TEXT("Spear")) return false;
            }
            Object->RemoveField(TEXT("ItemId"));
            Object->SetStringField(TEXT("ToolId"), ItemId);
            Object->SetStringField(TEXT("ToolKind"), ItemId);
            Object->SetStringField(TEXT("ToolSet"), Set);
            Object->SetStringField(TEXT("Action"), Action);
            Object->SetStringField(TEXT("RequiredSkill"), Action);
            if (!Object->HasField(TEXT("MinimumSkillLevel"))) Object->SetNumberField(TEXT("MinimumSkillLevel"), 1.0);
        }
        return true;
    }

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

    bool IsValidCategoryPath(const FString& RelativePath, const TCHAR* ExpectedDirectory,
        FString& OutNormalizedPath, const TCHAR* AllowedSubdirectory,
        const TCHAR* AdditionalAllowedSubdirectory)
    {
        OutNormalizedPath = RelativePath.Replace(TEXT("\\"), TEXT("/"));
        if (OutNormalizedPath.IsEmpty() || OutNormalizedPath.StartsWith(TEXT("/"))
            || OutNormalizedPath.EndsWith(TEXT("/")) || OutNormalizedPath.Contains(TEXT(":")))
        {
            return false;
        }

        TArray<FString> PathParts;
        OutNormalizedPath.ParseIntoArray(PathParts, TEXT("/"), false);
        const bool bIsAllowedSubdirectory = PathParts.Num() == 3
            && ((AllowedSubdirectory != nullptr && PathParts[1] == AllowedSubdirectory)
                || (AdditionalAllowedSubdirectory != nullptr && PathParts[1] == AdditionalAllowedSubdirectory));
        const bool bHasExpectedDepth = PathParts.Num() == 2 || bIsAllowedSubdirectory;
        if (!bHasExpectedDepth || PathParts[0] != ExpectedDirectory || PathParts.Last().IsEmpty()
            || !PathParts.Last().EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
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
        int32& InOutJsonCharacters, TArray<TSharedPtr<FJsonValue>>& OutValues,
        const TCHAR* AllowedSubdirectory = nullptr, const TCHAR* AdditionalAllowedSubdirectory = nullptr,
        const bool bAllowEmptyFiles = false)
    {
        const TArray<TSharedPtr<FJsonValue>>* FileValues = nullptr;
        if (!Manifest.IsValid() || !Manifest->TryGetArrayField(ManifestField, FileValues)
            || FileValues == nullptr || FileValues->IsEmpty() || FileValues->Num() > 16)
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest has an invalid file list: %s"), ManifestField);
            return false;
        }

        // Append so each category can be assembled from multiple files.
        for (const TSharedPtr<FJsonValue>& FileValue : *FileValues)
        {
            if (!FileValue.IsValid() || FileValue->Type != EJson::String)
            {
                UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest contains a non-string file path: %s"), ManifestField);
                return false;
            }

            FString RelativePath;
            if (!IsValidCategoryPath(FileValue->AsString(), ExpectedDirectory, RelativePath,
                    AllowedSubdirectory, AdditionalAllowedSubdirectory))
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
                || (!bAllowEmptyFiles && CategoryValues->IsEmpty())
                || OutValues.Num() + CategoryValues->Num() > MaxEntries)
            {
                UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue category is malformed or exceeds its bounds: %s"),
                    *CategoryPath);
                return false;
            }
            OutValues.Append(*CategoryValues);
        }
        return true;
    }

    bool SortCategoryValues(TArray<TSharedPtr<FJsonValue>>& InOutValues, const TCHAR* IdField)
    {
        TArray<TPair<FString, TSharedPtr<FJsonValue>>> ValuesWithIds;
        ValuesWithIds.Reserve(InOutValues.Num());
        TSet<FName> SeenIds;
        for (const TSharedPtr<FJsonValue>& Value : InOutValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object) return false;
            FString IdString;
            if (!Value->AsObject()->TryGetStringField(IdField, IdString) || IdString.IsEmpty()) return false;
            const FName Id(*IdString);
            if (Id.IsNone() || SeenIds.Contains(Id)) return false;
            SeenIds.Add(Id);
            ValuesWithIds.Emplace(IdString, Value);
        }

        ValuesWithIds.Sort([](const TPair<FString, TSharedPtr<FJsonValue>>& A,
            const TPair<FString, TSharedPtr<FJsonValue>>& B)
        {
            return A.Key.Compare(B.Key, ESearchCase::IgnoreCase) < 0;
        });

        InOutValues.Reset(ValuesWithIds.Num());
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : ValuesWithIds)
        {
            InOutValues.Add(Entry.Value);
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
        if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) || SchemaVersion != 13.0)
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest has an unsupported schema: %s"), *JsonPath);
            return false;
        }

        int32 TotalJsonCharacters = JsonText.Len();
        TArray<TSharedPtr<FJsonValue>> ItemValues;
        TArray<TSharedPtr<FJsonValue>> RecipeValues;
        TArray<TSharedPtr<FJsonValue>> StationRecipeValues;
        TArray<TSharedPtr<FJsonValue>> ToolProfileValues;
        TArray<TSharedPtr<FJsonValue>> ToolRecipeValues;
        TArray<TSharedPtr<FJsonValue>> ToolUpgradeValues;
        TArray<TSharedPtr<FJsonValue>> LevelUpRewardValues;
        TArray<TSharedPtr<FJsonValue>> AttackListValues;
        if (!LoadCategoryFiles(Root, TEXT("itemFiles"), TEXT("items"), TEXT("items"),
                UKalmalaItemCatalogue::MaxDefinitions, TotalJsonCharacters, ItemValues,
                TEXT("buildItems"), TEXT("materials"))
            || !LoadCategoryFiles(Root, TEXT("recipeFiles"), TEXT("recipes"), TEXT("recipes"),
                32 + UKalmalaToolCatalogue::MaxRecipes, TotalJsonCharacters, StationRecipeValues,
                TEXT("stations"), nullptr, true)
            || !LoadCategoryFiles(Root, TEXT("itemProfileFiles"), TEXT("items"), TEXT("items"),
                UKalmalaToolCatalogue::MaxProfiles, TotalJsonCharacters, ToolProfileValues, TEXT("equipment"))
            || !LoadCategoryFiles(Root, TEXT("itemUpgradeFiles"), TEXT("upgrades"), TEXT("item-level-up"),
                UKalmalaToolCatalogue::MaxRecipes, TotalJsonCharacters, ToolUpgradeValues,
                TEXT("costs"), nullptr, true)
            || !LoadCategoryFiles(Root, TEXT("levelUpRewardFiles"), TEXT("levelUpRewards"), TEXT("item-level-up"),
                UKalmalaToolCatalogue::MaxProfiles, TotalJsonCharacters, LevelUpRewardValues, TEXT("rewards"))
            || !LoadCategoryFiles(Root, TEXT("attackFiles"), TEXT("attackLists"), TEXT("attacks"),
                UKalmalaToolCatalogue::MaxProfiles, TotalJsonCharacters, AttackListValues)
            || !SortCategoryValues(ItemValues, TEXT("ItemId"))
            || !SortCategoryValues(ToolProfileValues, TEXT("ItemId"))
            || !SortCategoryValues(ToolUpgradeValues, TEXT("ToolId")))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue manifest could not load its category files: %s"), *JsonPath);
            return false;
        }

        for (const TSharedPtr<FJsonValue>& Value : StationRecipeValues)
        {
            if (!Value.IsValid() || Value->Type != EJson::Object)
            {
                return false;
            }

            if (Value->AsObject()->HasField(TEXT("ToolId")))
            {
                ToolRecipeValues.Add(Value);
            }
            else
            {
                RecipeValues.Add(Value);
            }
        }

        if (RecipeValues.Num() > 32 || ToolRecipeValues.Num() > UKalmalaToolCatalogue::MaxRecipes
            || !SortCategoryValues(RecipeValues, TEXT("RecipeId"))
            || !SortCategoryValues(ToolRecipeValues, TEXT("RecipeId")))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue station recipes exceed their bounds or contain duplicate IDs: %s"), *JsonPath);
            return false;
        }

        if (ItemValues.IsEmpty() || RecipeValues.IsEmpty() || ToolProfileValues.IsEmpty() || ToolRecipeValues.IsEmpty()
            || !NormalizeCatalogueItemReferences(ItemValues, RecipeValues)
            || !NormalizeEquipmentProfiles(ToolProfileValues))
        {
            UE_LOG(LogTemp, Error, TEXT("Gameplay catalogue category data has invalid clean item or station references: %s"), *JsonPath);
            return false;
        }

        // Damage channels are optional in item JSON; retain strict conversion for other fields.
        for (const TSharedPtr<FJsonValue>& ProfileValue : ToolProfileValues)
        {
            const TSharedPtr<FJsonObject> ProfileObject = ProfileValue->AsObject();
            const TSharedPtr<FJsonObject>* Bonuses = nullptr;
            if (!ProfileObject.IsValid()
                || !ProfileObject->TryGetObjectField(TEXT("DamageBonuses"), Bonuses))
            {
                return false;
            }
            FString Action;
            ProfileObject->TryGetStringField(TEXT("Action"), Action);
            if (Action != TEXT("Woodcutting") && Action != TEXT("Mining")
                && !ProfileObject->HasField(TEXT("MaterialTier")))
            {
                ProfileObject->SetNumberField(TEXT("MaterialTier"), 0.0);
            }
            for (const TCHAR* Channel : { TEXT("Slash"), TEXT("Stab"), TEXT("Crush"), TEXT("Pierce"),
                TEXT("Fire"), TEXT("Water"), TEXT("Holy"), TEXT("Poison") })
            {
                if (!(*Bonuses)->HasField(Channel))
                {
                    (*Bonuses)->SetNumberField(Channel, 0.0);
                }
            }
        }

        // Shared rewards use the same sparse channel authoring as item base stats.
        for (const TSharedPtr<FJsonValue>& RewardValue : LevelUpRewardValues)
        {
            const TSharedPtr<FJsonObject> GroupObject = RewardValue->AsObject();
            const TSharedPtr<FJsonObject>* RewardObject = nullptr;
            const TSharedPtr<FJsonObject>* Bonuses = nullptr;
            if (!GroupObject.IsValid()
                || !GroupObject->TryGetObjectField(TEXT("LevelUpReward"), RewardObject)
                || !(*RewardObject)->TryGetObjectField(TEXT("DamageBonuses"), Bonuses))
            {
                return false;
            }
            if (!(*RewardObject)->HasField(TEXT("MaxDurability")))
            {
                (*RewardObject)->SetNumberField(TEXT("MaxDurability"), 0.0);
            }
            for (const TCHAR* Channel : { TEXT("Slash"), TEXT("Stab"), TEXT("Crush"), TEXT("Pierce"),
                TEXT("Fire"), TEXT("Water"), TEXT("Holy"), TEXT("Poison") })
            {
                if (!(*Bonuses)->HasField(Channel))
                {
                    (*Bonuses)->SetNumberField(Channel, 0.0);
                }
            }
        }

        // Upgrade-cost records have no independent station-level requirement.
        for (const TSharedPtr<FJsonValue>& Value : ToolUpgradeValues)
        {
            const TSharedPtr<FJsonObject> Object = Value->AsObject();
            if (!Object.IsValid()) return false;
            if (!Object->HasField(TEXT("RequiredStationLevel")))
            {
                Object->SetNumberField(TEXT("RequiredStationLevel"), 0.0);
            }
        }

        // Zero skill requirements are implicit for acquisitions and upgrade-cost records.
        for (const TArray<TSharedPtr<FJsonValue>>* Values : { &ToolRecipeValues, &ToolUpgradeValues })
        {
            for (const TSharedPtr<FJsonValue>& Value : *Values)
            {
                const TSharedPtr<FJsonObject> Object = Value->AsObject();
                if (!Object.IsValid()) return false;
                if (!Object->HasField(TEXT("RequiredSkill"))) Object->SetStringField(TEXT("RequiredSkill"), TEXT("None"));
                if (!Object->HasField(TEXT("RequiredSkillLevel"))) Object->SetNumberField(TEXT("RequiredSkillLevel"), 0.0);
            }
        }

        TArray<FKalmalaItemDefinition> ParsedItems;
        TArray<FKalmalaRecipe> ParsedRecipes;
        TArray<FKalmalaToolProfileData> ParsedToolProfiles;
        TArray<FKalmalaToolRecipeData> ParsedToolRecipes;
        TArray<FKalmalaToolRecipeData> ParsedUpgradeCosts;
        TArray<FKalmalaLevelUpRewardGroupData> ParsedLevelUpRewards;
        TArray<FKalmalaWeaponAttackListData> ParsedAttackLists;
        if (!FJsonObjectConverter::JsonArrayToUStruct<FKalmalaItemDefinition>(ItemValues, &ParsedItems, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaRecipe>(RecipeValues, &ParsedRecipes, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaToolProfileData>(
                ToolProfileValues, &ParsedToolProfiles, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaToolRecipeData>(
                ToolRecipeValues, &ParsedToolRecipes, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaToolRecipeData>(
                ToolUpgradeValues, &ParsedUpgradeCosts, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaLevelUpRewardGroupData>(
                LevelUpRewardValues, &ParsedLevelUpRewards, 0, 0, true)
            || !FJsonObjectConverter::JsonArrayToUStruct<FKalmalaWeaponAttackListData>(
                AttackListValues, &ParsedAttackLists, 0, 0, true))
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
        ToolCatalogue->LevelUpRewards = MoveTemp(ParsedLevelUpRewards);
        ToolCatalogue->AttackLists = MoveTemp(ParsedAttackLists);
        if (!ItemCatalogue->IsValidCatalogue() || !RecipeCatalogue->IsValidCatalogue()
            || !ToolCatalogue->IsValidCatalogue(ItemCatalogue))
        {
            ItemCatalogue->Items.Reset();
            RecipeCatalogue->Recipes.Reset();
            ToolCatalogue->Profiles.Reset();
            ToolCatalogue->Recipes.Reset();
            ToolCatalogue->UpgradeCosts.Reset();
            ToolCatalogue->LevelUpRewards.Reset();
            ToolCatalogue->AttackLists.Reset();
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
