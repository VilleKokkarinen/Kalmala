#include "CreateWorldMaterialsCommandlet.h"

#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace KalmalaWorldMaterials
{
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

    const bool bTerrainCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrain"), FLinearColor(0.22f, 0.34f, 0.17f), 0.92f);
    const bool bWaterCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedWater"), FLinearColor(0.05f, 0.24f, 0.38f), 0.28f);
    const bool bLakeShoreCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedLakeShore"), FLinearColor(0.30f, 0.46f, 0.35f), 0.78f);
    return bTerrainCreated && bWaterCreated && bLakeShoreCreated ? 0 : 1;
}
