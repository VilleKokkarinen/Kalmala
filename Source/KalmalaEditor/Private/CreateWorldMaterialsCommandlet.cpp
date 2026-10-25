#include "CreateWorldMaterialsCommandlet.h"

#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace KalmalaWorldMaterials
{
    static const FLinearColor TerrainGrassColor(0.22f, 0.34f, 0.17f);
    static const FLinearColor TerrainSoilColor(0.34f, 0.26f, 0.17f);
    static const FLinearColor TerrainRockColor(0.24f, 0.27f, 0.23f);
    static const FLinearColor TerrainSnowColor(0.78f, 0.84f, 0.87f);

    static bool CreateMaterial(const FString& PackageName, const FLinearColor Color, const float Roughness)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not resolve material package %s."), *PackageName);
            return false;
        }

        if (IFileManager::Get().FileExists(*Filename))
        {
            UE_LOG(LogTemp, Display, TEXT("World material already exists at %s."), *PackageName);
            return true;
        }

        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        UPackage* Package = CreatePackage(*PackageName);
        const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
        UMaterial* Material = NewObject<UMaterial>(Package, AssetName, RF_Public | RF_Standalone);
        if (Material == nullptr || Material->GetEditorOnlyData() == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not create world material %s."), *PackageName);
            return false;
        }

        UMaterialExpressionConstant3Vector* BaseColorExpression = NewObject<UMaterialExpressionConstant3Vector>(Material);
        UMaterialExpressionConstant* RoughnessExpression = NewObject<UMaterialExpressionConstant>(Material);
        BaseColorExpression->Constant = Color;
        RoughnessExpression->R = Roughness;

        UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
        EditorOnlyData->ExpressionCollection.Expressions.Add(BaseColorExpression);
        EditorOnlyData->ExpressionCollection.Expressions.Add(RoughnessExpression);
        EditorOnlyData->BaseColor.Expression = BaseColorExpression;
        EditorOnlyData->Roughness.Expression = RoughnessExpression;
        Material->PostEditChange();

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(Package, Material, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not save world material %s."), *PackageName);
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("Created project-owned world material %s."), *PackageName);
        return true;
    }

    static bool CreateTerrainGroundBlendMaterial(const FString& PackageName, const float Roughness)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not resolve material package %s."), *PackageName);
            return false;
        }

        const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
        UMaterial* Material = LoadObject<UMaterial>(nullptr, *FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName));
        UPackage* Package = Material != nullptr ? Material->GetOutermost() : nullptr;
        if (Material == nullptr)
        {
            if (IFileManager::Get().FileExists(*Filename))
            {
                UE_LOG(LogTemp, Error, TEXT("Could not load existing terrain material %s."), *PackageName);
                return false;
            }
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
            Package = CreatePackage(*PackageName);
            Material = NewObject<UMaterial>(Package, FName(*AssetName), RF_Public | RF_Standalone);
        }
        if (Material == nullptr || Material->GetEditorOnlyData() == nullptr || Package == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not prepare terrain blend material %s."), *PackageName);
            return false;
        }

        UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
        EditorOnlyData->ExpressionCollection.Expressions.Reset();

        UMaterialExpressionVertexColor* VertexColor = NewObject<UMaterialExpressionVertexColor>(Material);
        EditorOnlyData->ExpressionCollection.Expressions.Add(VertexColor);

        const auto CreateWeightChannel = [Material, EditorOnlyData, VertexColor](const bool bRed,
            const bool bGreen, const bool bBlue, const bool bAlpha)
        {
            UMaterialExpressionComponentMask* Mask = NewObject<UMaterialExpressionComponentMask>(Material);
            Mask->Input.Connect(0, VertexColor);
            Mask->R = bRed;
            Mask->G = bGreen;
            Mask->B = bBlue;
            Mask->A = bAlpha;
            EditorOnlyData->ExpressionCollection.Expressions.Add(Mask);
            return Mask;
        };
        const auto CreateColor = [Material, EditorOnlyData](const FLinearColor Color)
        {
            UMaterialExpressionConstant3Vector* Expression = NewObject<UMaterialExpressionConstant3Vector>(Material);
            Expression->Constant = Color;
            EditorOnlyData->ExpressionCollection.Expressions.Add(Expression);
            return Expression;
        };
        const auto CreateLayer = [Material, EditorOnlyData](UMaterialExpression* Weight,
            UMaterialExpression* Color)
        {
            UMaterialExpressionMultiply* Multiply = NewObject<UMaterialExpressionMultiply>(Material);
            Multiply->A.Connect(0, Weight);
            Multiply->B.Connect(0, Color);
            EditorOnlyData->ExpressionCollection.Expressions.Add(Multiply);
            return Multiply;
        };
        const auto CreateAdd = [Material, EditorOnlyData](UMaterialExpression* Left,
            UMaterialExpression* Right)
        {
            UMaterialExpressionAdd* Add = NewObject<UMaterialExpressionAdd>(Material);
            Add->A.Connect(0, Left);
            Add->B.Connect(0, Right);
            EditorOnlyData->ExpressionCollection.Expressions.Add(Add);
            return Add;
        };
        const auto CreateBlend = [Material, EditorOnlyData](UMaterialExpression* Base,
            UMaterialExpression* Overlay, UMaterialExpression* Weight)
        {
            UMaterialExpressionLinearInterpolate* Blend = NewObject<UMaterialExpressionLinearInterpolate>(Material);
            Blend->A.Connect(0, Base);
            Blend->B.Connect(0, Overlay);
            Blend->Alpha.Connect(0, Weight);
            EditorOnlyData->ExpressionCollection.Expressions.Add(Blend);
            return Blend;
        };

        UMaterialExpression* GrassWeight = CreateWeightChannel(true, false, false, false);
        UMaterialExpression* GrassColorExpression = CreateColor(TerrainGrassColor);
        UMaterialExpressionMultiply* GrassLayer = CreateLayer(GrassWeight, GrassColorExpression);
        UMaterialExpression* SoilWeight = CreateWeightChannel(false, true, false, false);
        UMaterialExpression* SoilColorExpression = CreateColor(TerrainSoilColor);
        UMaterialExpressionMultiply* SoilLayer = CreateLayer(SoilWeight, SoilColorExpression);
        UMaterialExpression* RockWeight = CreateWeightChannel(false, false, true, false);
        UMaterialExpression* RockColorExpression = CreateColor(TerrainRockColor);
        UMaterialExpressionMultiply* RockLayer = CreateLayer(RockWeight, RockColorExpression);
        UMaterialExpressionAdd* GrassAndSoil = CreateAdd(GrassLayer, SoilLayer);
        UMaterialExpressionAdd* GroundColor = CreateAdd(GrassAndSoil, RockLayer);
        UMaterialExpression* SnowWeight = CreateWeightChannel(false, false, false, true);
        UMaterialExpression* SnowColorExpression = CreateColor(TerrainSnowColor);
        UMaterialExpression* GroundAndSnowColor = CreateBlend(GroundColor, SnowColorExpression, SnowWeight);
        UMaterialExpressionConstant* RoughnessExpression = NewObject<UMaterialExpressionConstant>(Material);
        RoughnessExpression->R = Roughness;
        EditorOnlyData->ExpressionCollection.Expressions.Add(RoughnessExpression);

        EditorOnlyData->BaseColor.Expression = GroundAndSnowColor;
        EditorOnlyData->Roughness.Expression = RoughnessExpression;
        Material->PostEditChange();
        Package->MarkPackageDirty();

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(Package, Material, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not save terrain blend material %s."), *PackageName);
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("Generated grass/soil/rock/Tundra-snow terrain material %s."), *PackageName);
        return true;
    }

    static bool CreateVertexColorMaterial(const FString& PackageName)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not resolve material package %s."), *PackageName);
            return false;
        }

        if (IFileManager::Get().FileExists(*Filename))
        {
            UE_LOG(LogTemp, Display, TEXT("World material already exists at %s."), *PackageName);
            return true;
        }

        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        UPackage* Package = CreatePackage(*PackageName);
        const FName AssetName(*FPackageName::GetLongPackageAssetName(PackageName));
        UMaterial* Material = NewObject<UMaterial>(Package, AssetName, RF_Public | RF_Standalone);
        if (Material == nullptr || Material->GetEditorOnlyData() == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not create world material %s."), *PackageName);
            return false;
        }

        UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
        UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
        EditorOnlyData->ExpressionCollection.Expressions.Add(VertexColorExpression);
        EditorOnlyData->BaseColor.Expression = VertexColorExpression;
        EditorOnlyData->EmissiveColor.Expression = VertexColorExpression;
        Material->PostEditChange();

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(Package, Material, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not save world material %s."), *PackageName);
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("Created vertex-colour debug material %s."), *PackageName);
        return true;
    }
}

UCreateWorldMaterialsCommandlet::UCreateWorldMaterialsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCreateWorldMaterialsCommandlet::Main(const FString& Params)
{
    using namespace KalmalaWorldMaterials;

    const bool bSoilCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedSoil"), TerrainSoilColor, 0.96f);
    const bool bTerrainCreated = CreateTerrainGroundBlendMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrain"), 0.92f);
    const bool bWaterCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedWater"), FLinearColor(0.05f, 0.24f, 0.38f), 0.28f);
    const bool bLakeShoreCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedLakeShore"), FLinearColor(0.30f, 0.46f, 0.35f), 0.78f);
    const bool bRockCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock"), TerrainRockColor, 0.95f);
    const bool bBarkCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedBark"), FLinearColor(0.20f, 0.13f, 0.09f), 0.88f);
    const bool bCanopyCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedCanopy"), FLinearColor(0.12f, 0.30f, 0.14f), 0.82f);
    const bool bBiomeDebugTerrainCreated = CreateVertexColorMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrainBiomeDebug"));
    return bTerrainCreated && bSoilCreated && bWaterCreated && bLakeShoreCreated && bRockCreated && bBarkCreated && bCanopyCreated && bBiomeDebugTerrainCreated ? 0 : 1;
}
