#include "CreateWorldMaterialsCommandlet.h"

#include "HAL/FileManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace KalmalaWorldMaterials
{
    static const FLinearColor TerrainGrassColor(0.22f, 0.34f, 0.17f);
    static const FLinearColor TerrainSoilColor(0.34f, 0.26f, 0.17f);
    static const FLinearColor TerrainRockColor(0.24f, 0.27f, 0.23f);
    static const FLinearColor TerrainSnowColor(0.78f, 0.84f, 0.87f);

    template <typename TExpression>
    static TExpression* AddFunctionExpression(UMaterialFunction* Function, const int32 X, const int32 Y)
    {
        TExpression* Expression = NewObject<TExpression>(Function);
        Expression->MaterialExpressionEditorX = X;
        Expression->MaterialExpressionEditorY = Y;
        Function->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
        return Expression;
    }

    template <typename TExpression>
    static TExpression* AddMaterialExpression(UMaterial* Material, const int32 X, const int32 Y)
    {
        TExpression* Expression = NewObject<TExpression>(Material);
        Expression->MaterialExpressionEditorX = X;
        Expression->MaterialExpressionEditorY = Y;
        Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
        return Expression;
    }

    static UMaterialExpressionFunctionInput* AddFunctionInput(UMaterialFunction* Function, const FName Name,
        const EFunctionInputType Type, const FVector4f PreviewValue, const int32 SortPriority, const TCHAR* Description)
    {
        UMaterialExpressionFunctionInput* Input = AddFunctionExpression<UMaterialExpressionFunctionInput>(
            Function, -900, SortPriority * 170);
        Input->InputName = Name;
        Input->Description = Description;
        Input->InputType = Type;
        Input->PreviewValue = PreviewValue;
        Input->bUsePreviewValueAsDefault = true;
        Input->SortPriority = SortPriority;
        Input->ConditionallyGenerateId(true);
        Input->ValidateName();
        return Input;
    }

    static UMaterialExpressionFunctionOutput* AddFunctionOutput(UMaterialFunction* Function, const FName Name,
        UMaterialExpression* Source, const int32 SortPriority, const TCHAR* Description)
    {
        UMaterialExpressionFunctionOutput* Output = AddFunctionExpression<UMaterialExpressionFunctionOutput>(
            Function, 1000, SortPriority * 220);
        Output->OutputName = Name;
        Output->Description = Description;
        Output->A.Connect(0, Source);
        Output->SortPriority = SortPriority;
        Output->ConditionallyGenerateId(true);
        Output->ValidateName();
        return Output;
    }

    static UMaterialFunction* CreateSurfaceResponseFunction(const FString& PackageName)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not resolve surface response function package %s."), *PackageName);
            return nullptr;
        }

        const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
        UMaterialFunction* Function = nullptr;
        UPackage* Package = nullptr;
        if (IFileManager::Get().FileExists(*Filename))
        {
            Function = LoadObject<UMaterialFunction>(nullptr,
                *FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName));
            if (Function == nullptr || Function->GetEditorOnlyData() == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("Could not load existing surface response function %s."), *PackageName);
                return nullptr;
            }
            UE_LOG(LogTemp, Display, TEXT("Surface response function already exists at %s."), *PackageName);
            return Function;
        }
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        Package = CreatePackage(*PackageName);
        Function = NewObject<UMaterialFunction>(Package, FName(*AssetName), RF_Public | RF_Standalone);
        if (Function == nullptr || Function->GetEditorOnlyData() == nullptr || Package == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not prepare surface response function %s."), *PackageName);
            return nullptr;
        }

        FMaterialExpressionCollection& Expressions = Function->GetEditorOnlyData()->ExpressionCollection;
        Expressions.Expressions.Reset();
        Function->Description = TEXT("Shared opaque-surface response. Inputs use linear color, unit tangent normals, and absolute world position in centimeters.");
        Function->UserExposedCaption = TEXT("Kalmala Surface Response");
        Function->bExposeToLibrary = true;

        UMaterialExpressionFunctionInput* BaseColor = AddFunctionInput(Function, TEXT("BaseColor"),
            FunctionInput_Vector3, FVector4f(1.0f, 1.0f, 1.0f, 1.0f), 0,
            TEXT("Linear surface color before the shared tint and broad variation."));
        UMaterialExpressionFunctionInput* BaseTint = AddFunctionInput(Function, TEXT("BaseTint"),
            FunctionInput_Vector3, FVector4f(1.0f, 1.0f, 1.0f, 1.0f), 1,
            TEXT("Neutral-white multiplier for the surface base color."));
        UMaterialExpressionFunctionInput* Roughness = AddFunctionInput(Function, TEXT("Roughness"),
            FunctionInput_Scalar, FVector4f(0.92f, 0.0f, 0.0f, 0.0f), 2,
            TEXT("Opaque-surface roughness, from zero to one."));
        UMaterialExpressionFunctionInput* Normal = AddFunctionInput(Function, TEXT("Normal"),
            FunctionInput_Vector3, FVector4f(0.0f, 0.0f, 1.0f, 1.0f), 3,
            TEXT("Tangent-space normal input; use a flat normal when no normal map is assigned."));
        UMaterialExpressionFunctionInput* NormalStrength = AddFunctionInput(Function, TEXT("NormalStrength"),
            FunctionInput_Scalar, FVector4f(1.0f, 0.0f, 0.0f, 0.0f), 4,
            TEXT("Blend from a flat tangent normal at zero to the input normal at one."));
        UMaterialExpressionFunctionInput* ColorVariation = AddFunctionInput(Function, TEXT("ColorVariation"),
            FunctionInput_Scalar, FVector4f(0.05f, 0.0f, 0.0f, 0.0f), 5,
            TEXT("Subtle symmetric amplitude for broad world-space color variation."));
        UMaterialExpressionFunctionInput* MappingScale = AddFunctionInput(Function, TEXT("MappingScale"),
            FunctionInput_Scalar, FVector4f(0.001f, 0.0f, 0.0f, 0.0f), 6,
            TEXT("World-position multiplier in inverse centimeters; 0.001 gives broad, roughly ten-meter variation."));
        UMaterialExpressionFunctionInput* WorldPosition = AddFunctionInput(Function, TEXT("WorldPosition"),
            FunctionInput_Vector3, FVector4f(0.0f, 0.0f, 0.0f, 1.0f), 7,
            TEXT("Absolute world position in centimeters, shared across mesh and patch boundaries."));

        UMaterialExpressionMultiply* ScaledWorldPosition = AddFunctionExpression<UMaterialExpressionMultiply>(Function, -520, 1190);
        ScaledWorldPosition->A.Connect(0, WorldPosition);
        ScaledWorldPosition->B.Connect(0, MappingScale);
        UMaterialExpressionNoise* BroadNoise = AddFunctionExpression<UMaterialExpressionNoise>(Function, -280, 1190);
        BroadNoise->Position.Connect(0, ScaledWorldPosition);
        BroadNoise->Scale = 1.0f;
        BroadNoise->Quality = 1;
        BroadNoise->NoiseFunction = NOISEFUNCTION_GradientTex3D;
        BroadNoise->bTurbulence = false;
        BroadNoise->Levels = 1;
        BroadNoise->OutputMin = 0.0f;
        BroadNoise->OutputMax = 1.0f;

        UMaterialExpressionConstant* Half = AddFunctionExpression<UMaterialExpressionConstant>(Function, -260, 1450);
        Half->R = 0.5f;
        UMaterialExpressionSubtract* CenteredNoise = AddFunctionExpression<UMaterialExpressionSubtract>(Function, -40, 1190);
        CenteredNoise->A.Connect(0, BroadNoise);
        CenteredNoise->B.Connect(0, Half);
        UMaterialExpressionMultiply* VariationAmount = AddFunctionExpression<UMaterialExpressionMultiply>(Function, 190, 1190);
        VariationAmount->A.Connect(0, CenteredNoise);
        VariationAmount->B.Connect(0, ColorVariation);
        UMaterialExpressionConstant* One = AddFunctionExpression<UMaterialExpressionConstant>(Function, -30, 1460);
        One->R = 1.0f;
        UMaterialExpressionAdd* VariationMultiplier = AddFunctionExpression<UMaterialExpressionAdd>(Function, 410, 1190);
        VariationMultiplier->A.Connect(0, One);
        VariationMultiplier->B.Connect(0, VariationAmount);
        UMaterialExpressionMultiply* TintedColor = AddFunctionExpression<UMaterialExpressionMultiply>(Function, 180, 470);
        TintedColor->A.Connect(0, BaseColor);
        TintedColor->B.Connect(0, BaseTint);
        UMaterialExpressionMultiply* VariedColor = AddFunctionExpression<UMaterialExpressionMultiply>(Function, 650, 470);
        VariedColor->A.Connect(0, TintedColor);
        VariedColor->B.Connect(0, VariationMultiplier);

        UMaterialExpressionConstant3Vector* FlatNormal = AddFunctionExpression<UMaterialExpressionConstant3Vector>(Function, -20, 1780);
        FlatNormal->Constant = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
        UMaterialExpressionLinearInterpolate* BlendedNormal = AddFunctionExpression<UMaterialExpressionLinearInterpolate>(Function, 420, 1760);
        BlendedNormal->A.Connect(0, FlatNormal);
        BlendedNormal->B.Connect(0, Normal);
        BlendedNormal->Alpha.Connect(0, NormalStrength);
        UMaterialExpressionNormalize* UnitNormal = AddFunctionExpression<UMaterialExpressionNormalize>(Function, 700, 1760);
        UnitNormal->VectorInput.Connect(0, BlendedNormal);

        AddFunctionOutput(Function, TEXT("BaseColor"), VariedColor, 0,
            TEXT("Tinted linear color with subtle, world-anchored broad variation."));
        AddFunctionOutput(Function, TEXT("Roughness"), Roughness, 1,
            TEXT("Pass-through roughness control."));
        AddFunctionOutput(Function, TEXT("Normal"), UnitNormal, 2,
            TEXT("Normalized tangent normal blended by NormalStrength."));

        Function->UpdateInputOutputTypes();
        Function->PostEditChange();
        Package->MarkPackageDirty();

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(Package, Function, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not save surface response function %s."), *PackageName);
            return nullptr;
        }

        UE_LOG(LogTemp, Display, TEXT("Generated shared Kalmala surface response function %s with eight inputs and three outputs."), *PackageName);
        return Function;
    }

    static bool ConnectFunctionInput(UMaterialExpressionMaterialFunctionCall* FunctionCall, const FName InputName,
        UMaterialExpression* Source)
    {
        for (FFunctionExpressionInput& Input : FunctionCall->FunctionInputs)
        {
            if (Input.ExpressionInput != nullptr && Input.ExpressionInput->InputName == InputName)
            {
                Input.Input.Connect(0, Source);
                return true;
            }
        }
        return false;
    }

    static int32 FindFunctionOutputIndex(const UMaterialExpressionMaterialFunctionCall* FunctionCall, const FName OutputName)
    {
        for (int32 Index = 0; Index < FunctionCall->FunctionOutputs.Num(); ++Index)
        {
            const UMaterialExpressionFunctionOutput* Output = FunctionCall->FunctionOutputs[Index].ExpressionOutput;
            if (Output != nullptr && Output->OutputName == OutputName)
            {
                return Index;
            }
        }
        return INDEX_NONE;
    }

    static UMaterialParameterCollection* CreateWeatherMaterialCollection(const FString& PackageName)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not resolve weather material collection package %s."), *PackageName);
            return nullptr;
        }

        const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
        UMaterialParameterCollection* Collection = nullptr;
        UPackage* Package = nullptr;
        if (IFileManager::Get().FileExists(*Filename))
        {
            Collection = LoadObject<UMaterialParameterCollection>(nullptr,
                *FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName));
            if (Collection == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("Could not load weather material collection %s."), *PackageName);
                return nullptr;
            }
            Package = Collection->GetPackage();
            Collection->Modify();
        }
        else
        {
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
            Package = CreatePackage(*PackageName);
            Collection = Package != nullptr
                ? NewObject<UMaterialParameterCollection>(Package, FName(*AssetName), RF_Public | RF_Standalone)
                : nullptr;
        }

        if (Collection == nullptr || Package == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not prepare weather material collection %s."), *PackageName);
            return nullptr;
        }

        const FName PrecipitationParameterName(TEXT("PrecipitationIntensity"));
        bool bHasPrecipitationParameter = false;
        for (FCollectionScalarParameter& Parameter : Collection->ScalarParameters)
        {
            if (Parameter.ParameterName == PrecipitationParameterName)
            {
                Parameter.DefaultValue = 0.0f;
                bHasPrecipitationParameter = true;
                break;
            }
        }
        if (!bHasPrecipitationParameter)
        {
            FCollectionScalarParameter& Parameter = Collection->ScalarParameters.AddDefaulted_GetRef();
            Parameter.ParameterName = PrecipitationParameterName;
            Parameter.DefaultValue = 0.0f;
        }

        Collection->PostEditChange();
        Package->MarkPackageDirty();
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(Package, Collection, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not save weather material collection %s."), *PackageName);
            return nullptr;
        }

        UE_LOG(LogTemp, Display, TEXT("Generated or updated local weather material collection %s."), *PackageName);
        return Collection;
    }

    static bool CreateSurfaceBaseMaterial(const FString& PackageName, UMaterialFunction* SurfaceResponse,
        UMaterialParameterCollection* WeatherCollection)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not resolve surface base material package %s."), *PackageName);
            return false;
        }

        const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
        UMaterial* Material = nullptr;
        UPackage* Package = nullptr;
        if (IFileManager::Get().FileExists(*Filename))
        {
            Material = LoadObject<UMaterial>(nullptr, *FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName));
            if (Material == nullptr || Material->GetEditorOnlyData() == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("Could not load existing surface base material %s."), *PackageName);
                return false;
            }
            Package = Material->GetPackage();
            Material->Modify();
        }
        else
        {
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
            Package = CreatePackage(*PackageName);
            Material = Package != nullptr
                ? NewObject<UMaterial>(Package, FName(*AssetName), RF_Public | RF_Standalone)
                : nullptr;
        }
        if (Material == nullptr || Material->GetEditorOnlyData() == nullptr || Package == nullptr
            || SurfaceResponse == nullptr || WeatherCollection == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not prepare surface base material %s."), *PackageName);
            return false;
        }

        FMaterialExpressionCollection& Expressions = Material->GetEditorOnlyData()->ExpressionCollection;
        Expressions.Expressions.Reset();
        UMaterialExpressionVertexColor* VertexColor = AddMaterialExpression<UMaterialExpressionVertexColor>(Material, -900, -180);
        UMaterialExpressionWorldPosition* WorldPosition = AddMaterialExpression<UMaterialExpressionWorldPosition>(Material, -900, 600);
        UMaterialExpressionConstant3Vector* FlatNormal = AddMaterialExpression<UMaterialExpressionConstant3Vector>(Material, -900, 160);
        FlatNormal->Constant = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
        UMaterialExpressionVectorParameter* BaseTint = AddMaterialExpression<UMaterialExpressionVectorParameter>(Material, -900, -460);
        BaseTint->ParameterName = TEXT("BaseTint");
        BaseTint->Group = TEXT("Kalmala Surface");
        BaseTint->SortPriority = 0;
        BaseTint->DefaultValue = FLinearColor::White;
        BaseTint->Desc = TEXT("Neutral-white multiplier for biome, rock, and wood base color.");

        const auto CreateScalarParameter = [Material](const FName Name, const float DefaultValue,
            const float SliderMin, const float SliderMax, const int32 SortPriority, const int32 Y, const TCHAR* Description)
        {
            UMaterialExpressionScalarParameter* Parameter = AddMaterialExpression<UMaterialExpressionScalarParameter>(Material, -900, Y);
            Parameter->ParameterName = Name;
            Parameter->Group = TEXT("Kalmala Surface");
            Parameter->SortPriority = SortPriority;
            Parameter->DefaultValue = DefaultValue;
            Parameter->SliderMin = SliderMin;
            Parameter->SliderMax = SliderMax;
            Parameter->Desc = Description;
            return Parameter;
        };
        UMaterialExpressionScalarParameter* Roughness = CreateScalarParameter(TEXT("Roughness"), 0.92f, 0.0f, 1.0f, 1, -320,
            TEXT("Surface microfacet roughness."));
        UMaterialExpressionScalarParameter* NormalStrength = CreateScalarParameter(TEXT("NormalStrength"), 1.0f, 0.0f, 1.0f, 2, 0,
            TEXT("Blend normal detail from flat at zero to full strength at one."));
        UMaterialExpressionScalarParameter* ColorVariation = CreateScalarParameter(TEXT("ColorVariation"), 0.05f, 0.0f, 0.12f, 3, 320,
            TEXT("Subtle symmetric amplitude for broad world-space color variation."));
        UMaterialExpressionScalarParameter* MappingScale = CreateScalarParameter(TEXT("MappingScale"), 0.001f, 0.0001f, 0.01f, 4, 460,
            TEXT("Absolute-world-position multiplier in inverse centimeters; smaller values produce broader variation."));
        UMaterialExpressionScalarParameter* WeatherWetResponse = CreateScalarParameter(TEXT("WeatherWetResponse"), 0.0f, 0.0f, 1.0f, 5, 780,
            TEXT("Opt in selected surfaces to the local replicated precipitation response."));

        UMaterialExpressionMaterialFunctionCall* FunctionCall = AddMaterialExpression<UMaterialExpressionMaterialFunctionCall>(Material, -380, 40);
        if (!FunctionCall->SetMaterialFunction(SurfaceResponse))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not bind the surface response function to %s."), *PackageName);
            return false;
        }

        const bool bInputsConnected =
            ConnectFunctionInput(FunctionCall, TEXT("BaseColor"), VertexColor) &&
            ConnectFunctionInput(FunctionCall, TEXT("BaseTint"), BaseTint) &&
            ConnectFunctionInput(FunctionCall, TEXT("Roughness"), Roughness) &&
            ConnectFunctionInput(FunctionCall, TEXT("Normal"), FlatNormal) &&
            ConnectFunctionInput(FunctionCall, TEXT("NormalStrength"), NormalStrength) &&
            ConnectFunctionInput(FunctionCall, TEXT("ColorVariation"), ColorVariation) &&
            ConnectFunctionInput(FunctionCall, TEXT("MappingScale"), MappingScale) &&
            ConnectFunctionInput(FunctionCall, TEXT("WorldPosition"), WorldPosition);
        const int32 BaseColorOutput = FindFunctionOutputIndex(FunctionCall, TEXT("BaseColor"));
        const int32 RoughnessOutput = FindFunctionOutputIndex(FunctionCall, TEXT("Roughness"));
        const int32 NormalOutput = FindFunctionOutputIndex(FunctionCall, TEXT("Normal"));
        if (!bInputsConnected || BaseColorOutput == INDEX_NONE || RoughnessOutput == INDEX_NONE || NormalOutput == INDEX_NONE)
        {
            UE_LOG(LogTemp, Error, TEXT("Surface response interface did not expose its expected inputs and outputs."));
            return false;
        }

        UMaterialExpressionCollectionParameter* PrecipitationIntensity =
            AddMaterialExpression<UMaterialExpressionCollectionParameter>(Material, -380, 980);
        PrecipitationIntensity->Collection = WeatherCollection;
        PrecipitationIntensity->ParameterName = TEXT("PrecipitationIntensity");
        PrecipitationIntensity->ParameterId = WeatherCollection->GetParameterId(PrecipitationIntensity->ParameterName);

        UMaterialExpressionMultiply* WetResponseStrength = AddMaterialExpression<UMaterialExpressionMultiply>(Material, -120, 900);
        WetResponseStrength->A.Connect(0, PrecipitationIntensity);
        WetResponseStrength->B.Connect(0, WeatherWetResponse);
        UMaterialExpressionSaturate* WetAmount = AddMaterialExpression<UMaterialExpressionSaturate>(Material, 100, 900);
        WetAmount->Input.Connect(0, WetResponseStrength);

        UMaterialExpressionConstant* One = AddMaterialExpression<UMaterialExpressionConstant>(Material, 80, 1120);
        One->R = 1.0f;
        UMaterialExpressionConstant* ColorDarkening = AddMaterialExpression<UMaterialExpressionConstant>(Material, 80, 1240);
        ColorDarkening->R = 0.08f;
        UMaterialExpressionMultiply* DarkeningAmount = AddMaterialExpression<UMaterialExpressionMultiply>(Material, 320, 1060);
        DarkeningAmount->A.Connect(0, WetAmount);
        DarkeningAmount->B.Connect(0, ColorDarkening);
        UMaterialExpressionSubtract* WetColorMultiplier = AddMaterialExpression<UMaterialExpressionSubtract>(Material, 540, 1060);
        WetColorMultiplier->A.Connect(0, One);
        WetColorMultiplier->B.Connect(0, DarkeningAmount);
        UMaterialExpressionMultiply* WetBaseColor = AddMaterialExpression<UMaterialExpressionMultiply>(Material, 800, 720);
        WetBaseColor->A.Connect(BaseColorOutput, FunctionCall);
        WetBaseColor->B.Connect(0, WetColorMultiplier);

        UMaterialExpressionConstant* RoughnessChange = AddMaterialExpression<UMaterialExpressionConstant>(Material, 320, 1240);
        RoughnessChange->R = 0.12f;
        UMaterialExpressionMultiply* WetRoughnessChange = AddMaterialExpression<UMaterialExpressionMultiply>(Material, 540, 1240);
        WetRoughnessChange->A.Connect(0, WetAmount);
        WetRoughnessChange->B.Connect(0, RoughnessChange);
        UMaterialExpressionSubtract* WetRoughness = AddMaterialExpression<UMaterialExpressionSubtract>(Material, 800, 1080);
        WetRoughness->A.Connect(RoughnessOutput, FunctionCall);
        WetRoughness->B.Connect(0, WetRoughnessChange);

        UMaterialEditorOnlyData* EditorOnlyData = Material->GetEditorOnlyData();
        EditorOnlyData->BaseColor.Connect(0, WetBaseColor);
        EditorOnlyData->Roughness.Connect(0, WetRoughness);
        EditorOnlyData->Normal.Connect(NormalOutput, FunctionCall);
        Material->PostEditChange();
        Package->MarkPackageDirty();

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(Package, Material, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not save surface base material %s."), *PackageName);
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("Generated Default Lit Kalmala surface base material %s with six artist controls."), *PackageName);
        return true;
    }

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

    static bool CreateMaterialInstance(const FString& PackageName, const FString& ParentPath,
        const FLinearColor BaseTint, const float Roughness, const float WeatherWetResponse = 0.0f)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not resolve material instance package %s."), *PackageName);
            return false;
        }

        const FString AssetName = FPackageName::GetLongPackageAssetName(PackageName);
        UMaterialInstanceConstant* Instance = nullptr;
        UPackage* Package = nullptr;
        if (IFileManager::Get().FileExists(*Filename))
        {
            Instance = LoadObject<UMaterialInstanceConstant>(nullptr,
                *FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName));
            if (Instance == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("Could not load existing material instance %s."), *PackageName);
                return false;
            }
            if (WeatherWetResponse <= 0.0f)
            {
                UE_LOG(LogTemp, Display, TEXT("Non-weather material instance remains unchanged at %s."), *PackageName);
                return true;
            }
            Package = Instance->GetPackage();
            Instance->Modify();
        }

        UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, *ParentPath);
        if (Parent == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not load material instance parent %s for %s."), *ParentPath, *PackageName);
            return false;
        }

        if (Instance == nullptr)
        {
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
            Package = CreatePackage(*PackageName);
            if (Package == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("Could not create material instance package %s."), *PackageName);
                return false;
            }
            Instance = NewObject<UMaterialInstanceConstant>(Package, FName(*AssetName), RF_Public | RF_Standalone);
        }
        if (Instance == nullptr || Package == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not create or update material instance %s."), *PackageName);
            return false;
        }

        Instance->SetParentEditorOnly(Parent);
        Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(FName(TEXT("BaseTint"))), BaseTint);
        Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName(TEXT("Roughness"))), Roughness);
        if (WeatherWetResponse > 0.0f)
        {
            Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName(TEXT("WeatherWetResponse"))), WeatherWetResponse);
        }
        Instance->PostEditChange();
        Package->MarkPackageDirty();

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        if (!UPackage::SavePackage(Package, Instance, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not save material instance %s."), *PackageName);
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("Created project-owned material instance %s from %s."), *PackageName, *ParentPath);
        return true;
    }

    static bool CreateTerrainGroundBlendMaterial(const FString& PackageName, const float Roughness,
        UMaterialFunction* SurfaceResponse)
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
        if (Material == nullptr || Material->GetEditorOnlyData() == nullptr || Package == nullptr || SurfaceResponse == nullptr)
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
            if (bAlpha)
            {
                // UE 5.8's Vertex Color output 0 is RGB (float3); alpha is
                // exposed on output 4. Select that scalar pin, then its R
                // component, instead of asking an RGB value for channel A.
                Mask->Input.Connect(4, VertexColor);
                Mask->R = true;
            }
            else
            {
                Mask->Input.Connect(0, VertexColor);
                Mask->R = bRed;
                Mask->G = bGreen;
                Mask->B = bBlue;
            }
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
        const auto CreateSurfaceScalarParameter = [Material, EditorOnlyData](const FName Name,
            const float DefaultValue, const float SliderMin, const float SliderMax, const int32 SortPriority,
            const int32 Y, const TCHAR* Description)
        {
            UMaterialExpressionScalarParameter* Parameter = NewObject<UMaterialExpressionScalarParameter>(Material);
            Parameter->ParameterName = Name;
            Parameter->Group = TEXT("Kalmala Surface");
            Parameter->SortPriority = SortPriority;
            Parameter->DefaultValue = DefaultValue;
            Parameter->SliderMin = SliderMin;
            Parameter->SliderMax = SliderMax;
            Parameter->Desc = Description;
            Parameter->MaterialExpressionEditorX = -900;
            Parameter->MaterialExpressionEditorY = Y;
            EditorOnlyData->ExpressionCollection.Expressions.Add(Parameter);
            return Parameter;
        };
        UMaterialExpressionVectorParameter* BaseTint = NewObject<UMaterialExpressionVectorParameter>(Material);
        BaseTint->ParameterName = TEXT("BaseTint");
        BaseTint->Group = TEXT("Kalmala Surface");
        BaseTint->SortPriority = 0;
        BaseTint->DefaultValue = FLinearColor::White;
        BaseTint->Desc = TEXT("Neutral-white multiplier for the vertex-weighted ground color.");
        BaseTint->MaterialExpressionEditorX = -900;
        BaseTint->MaterialExpressionEditorY = -460;
        EditorOnlyData->ExpressionCollection.Expressions.Add(BaseTint);
        UMaterialExpressionScalarParameter* RoughnessParameter = CreateSurfaceScalarParameter(TEXT("Roughness"),
            Roughness, 0.0f, 1.0f, 1, -320, TEXT("Terrain surface microfacet roughness."));
        UMaterialExpressionScalarParameter* NormalStrength = CreateSurfaceScalarParameter(TEXT("NormalStrength"),
            1.0f, 0.0f, 1.0f, 2, 0, TEXT("Blend from a flat tangent normal to the input normal."));
        UMaterialExpressionScalarParameter* ColorVariation = CreateSurfaceScalarParameter(TEXT("ColorVariation"),
            0.05f, 0.0f, 0.12f, 3, 320, TEXT("Subtle broad absolute-world color variation."));
        UMaterialExpressionScalarParameter* MappingScale = CreateSurfaceScalarParameter(TEXT("MappingScale"),
            0.001f, 0.0001f, 0.01f, 4, 460, TEXT("Absolute-world-position multiplier in inverse centimeters."));
        UMaterialExpressionConstant3Vector* FlatNormal = NewObject<UMaterialExpressionConstant3Vector>(Material);
        FlatNormal->Constant = FLinearColor(0.0f, 0.0f, 1.0f, 1.0f);
        FlatNormal->MaterialExpressionEditorX = -900;
        FlatNormal->MaterialExpressionEditorY = 160;
        EditorOnlyData->ExpressionCollection.Expressions.Add(FlatNormal);
        UMaterialExpressionWorldPosition* WorldPosition = NewObject<UMaterialExpressionWorldPosition>(Material);
        WorldPosition->MaterialExpressionEditorX = -900;
        WorldPosition->MaterialExpressionEditorY = 600;
        EditorOnlyData->ExpressionCollection.Expressions.Add(WorldPosition);

        UMaterialExpressionMaterialFunctionCall* FunctionCall = NewObject<UMaterialExpressionMaterialFunctionCall>(Material);
        FunctionCall->MaterialExpressionEditorX = -380;
        FunctionCall->MaterialExpressionEditorY = 40;
        EditorOnlyData->ExpressionCollection.Expressions.Add(FunctionCall);
        if (!FunctionCall->SetMaterialFunction(SurfaceResponse))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not bind the shared surface response to terrain material %s."), *PackageName);
            return false;
        }
        const bool bInputsConnected =
            ConnectFunctionInput(FunctionCall, TEXT("BaseColor"), GroundAndSnowColor) &&
            ConnectFunctionInput(FunctionCall, TEXT("BaseTint"), BaseTint) &&
            ConnectFunctionInput(FunctionCall, TEXT("Roughness"), RoughnessParameter) &&
            ConnectFunctionInput(FunctionCall, TEXT("Normal"), FlatNormal) &&
            ConnectFunctionInput(FunctionCall, TEXT("NormalStrength"), NormalStrength) &&
            ConnectFunctionInput(FunctionCall, TEXT("ColorVariation"), ColorVariation) &&
            ConnectFunctionInput(FunctionCall, TEXT("MappingScale"), MappingScale) &&
            ConnectFunctionInput(FunctionCall, TEXT("WorldPosition"), WorldPosition);
        const int32 BaseColorOutput = FindFunctionOutputIndex(FunctionCall, TEXT("BaseColor"));
        const int32 RoughnessOutput = FindFunctionOutputIndex(FunctionCall, TEXT("Roughness"));
        const int32 NormalOutput = FindFunctionOutputIndex(FunctionCall, TEXT("Normal"));
        if (!bInputsConnected || BaseColorOutput == INDEX_NONE || RoughnessOutput == INDEX_NONE || NormalOutput == INDEX_NONE)
        {
            UE_LOG(LogTemp, Error, TEXT("Terrain material %s could not connect the shared surface response interface."), *PackageName);
            return false;
        }

        EditorOnlyData->BaseColor.Connect(BaseColorOutput, FunctionCall);
        EditorOnlyData->Roughness.Connect(RoughnessOutput, FunctionCall);
        EditorOnlyData->Normal.Connect(NormalOutput, FunctionCall);
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

    UMaterialParameterCollection* WeatherCollection = CreateWeatherMaterialCollection(
        TEXT("/Game/Kalmala/World/Materials/MPC_KalmalaWeather"));
    UMaterialFunction* SurfaceResponse = CreateSurfaceResponseFunction(
        TEXT("/Game/Kalmala/World/Materials/Functions/MF_KalmalaSurfaceResponse"));
    const bool bSurfaceBaseCreated = SurfaceResponse != nullptr && WeatherCollection != nullptr
        && CreateSurfaceBaseMaterial(TEXT("/Game/Kalmala/World/Materials/M_KalmalaSurfaceBase"), SurfaceResponse, WeatherCollection);
    const bool bSoilCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedSoil"), TerrainSoilColor, 0.96f);
    const bool bTerrainCreated = CreateTerrainGroundBlendMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrain"), 0.92f, SurfaceResponse);
    const bool bTerrainInstanceCreated = bTerrainCreated && CreateMaterialInstance(
        TEXT("/Game/Kalmala/World/Materials/MI_GeneratedTerrain"),
        TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrain.M_GeneratedTerrain"),
        FLinearColor::White, 0.92f);
    const bool bWaterCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedWater"), FLinearColor(0.05f, 0.24f, 0.38f), 0.28f);
    const bool bLakeShoreInstanceCreated = CreateMaterialInstance(
        TEXT("/Game/Kalmala/World/Materials/MI_GeneratedLakeShore"),
        TEXT("/Game/Kalmala/World/Materials/M_KalmalaSurfaceBase.M_KalmalaSurfaceBase"),
        FLinearColor(0.30f / 0.48f, 0.46f / 0.70f, 0.35f / 0.67f), 0.78f);
    const bool bRockCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedRock"), TerrainRockColor, 0.95f);
    // Tint the original Meadow rock vertex-color mean back to the previous flat TerrainRockColor.
    const bool bRockInstanceCreated = CreateMaterialInstance(
        TEXT("/Game/Kalmala/World/Materials/MI_GeneratedRock"),
        TEXT("/Game/Kalmala/World/Materials/M_KalmalaSurfaceBase.M_KalmalaSurfaceBase"),
        FLinearColor(0.9032258f, 0.9137931f, 0.9044944f), 0.95f, 1.0f);
    const bool bBarkCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedBark"), FLinearColor(0.20f, 0.13f, 0.09f), 0.88f);
    const bool bBarkInstanceCreated = CreateMaterialInstance(
        TEXT("/Game/Kalmala/World/Materials/MI_GeneratedBark"),
        TEXT("/Game/Kalmala/World/Materials/M_KalmalaSurfaceBase.M_KalmalaSurfaceBase"),
        FLinearColor::White, 0.88f, 1.0f);
    const bool bWoodInstanceCreated = CreateMaterialInstance(
        TEXT("/Game/Kalmala/World/Materials/MI_GeneratedWood"),
        TEXT("/Game/Kalmala/World/Materials/M_KalmalaSurfaceBase.M_KalmalaSurfaceBase"),
        FLinearColor(0.20f, 0.13f, 0.09f), 0.88f, 1.0f);
    const bool bCanopyCreated = CreateMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedCanopy"), FLinearColor(0.12f, 0.30f, 0.14f), 0.82f);
    const bool bBiomeDebugTerrainCreated = CreateVertexColorMaterial(TEXT("/Game/Kalmala/World/Materials/M_GeneratedTerrainBiomeDebug"));
    return bSurfaceBaseCreated && bTerrainCreated && bTerrainInstanceCreated && bLakeShoreInstanceCreated
        && bSoilCreated && bWaterCreated && bRockCreated && bRockInstanceCreated
        && bBarkCreated && bBarkInstanceCreated && bWoodInstanceCreated && bCanopyCreated
        && bBiomeDebugTerrainCreated ? 0 : 1;
}
