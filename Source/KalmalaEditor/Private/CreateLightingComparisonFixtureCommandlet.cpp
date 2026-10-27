#include "CreateLightingComparisonFixtureCommandlet.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PlayerStart.h"
#include "Engine/SkyAtmosphere.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Factories/WorldFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "Engine/ToonProfile.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    constexpr TCHAR TestRoot[] = TEXT("/Game/Kalmala/Tests/Lighting");
    constexpr TCHAR TestMap[] = TEXT("/Game/Kalmala/Tests/Lighting/L_LightingComparison");

    struct FSurfaceSample
    {
        const TCHAR* Name;
        FLinearColor Color;
        float Roughness;
        const TCHAR* MeshPath;
        FVector Scale;
    };

    const FSurfaceSample Samples[] =
    {
        { TEXT("Ground"), FLinearColor(0.22f, 0.34f, 0.17f), 0.94f, TEXT("/Engine/BasicShapes/Plane.Plane"), FVector(1.35f, 1.0f, 1.0f) },
        { TEXT("Rock"), FLinearColor(0.24f, 0.27f, 0.23f), 0.96f, TEXT("/Engine/BasicShapes/Sphere.Sphere"), FVector(0.58f, 0.72f, 0.48f) },
        { TEXT("Wood"), FLinearColor(0.34f, 0.26f, 0.17f), 0.90f, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), FVector(0.25f, 0.25f, 0.82f) },
        { TEXT("Canopy"), FLinearColor(0.16f, 0.30f, 0.11f), 0.91f, TEXT("/Engine/BasicShapes/Sphere.Sphere"), FVector(0.72f, 0.72f, 0.62f) },
        { TEXT("Creature"), FLinearColor(0.25f, 0.20f, 0.17f), 0.88f, TEXT("/Engine/BasicShapes/Sphere.Sphere"), FVector(0.42f, 0.32f, 0.58f) },
        { TEXT("Construction"), FLinearColor(0.32f, 0.28f, 0.20f), 0.93f, TEXT("/Engine/BasicShapes/Cube.Cube"), FVector(0.58f, 0.58f, 0.58f) },
    };

    bool SaveAsset(UObject* Object, const FString& PackageName, const bool bMap = false)
    {
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename,
            bMap ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension())) return false;
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(Object->GetOutermost(), Object, *Filename, Args);
    }

    UToonProfile* MakeProfile()
    {
        const FString PackageName = FString::Printf(TEXT("%s/TP_L03SoftThreeBand"), TestRoot);
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension())) return nullptr;
        if (IFileManager::Get().FileExists(*Filename))
            return LoadObject<UToonProfile>(nullptr, TEXT("/Game/Kalmala/Tests/Lighting/TP_L03SoftThreeBand.TP_L03SoftThreeBand"));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        UPackage* Package = CreatePackage(*PackageName);
        UToonProfile* Profile = NewObject<UToonProfile>(Package, TEXT("TP_L03SoftThreeBand"), RF_Public | RF_Standalone);
        if (Profile == nullptr) return nullptr;

        const float Times[] = { 0.0f, 0.31f, 0.39f, 0.61f, 0.69f, 1.0f };
        const float Values[] = { 0.32f, 0.32f, 0.58f, 0.58f, 0.92f, 0.92f };
        for (int32 Channel = 0; Channel < 3; ++Channel)
        {
            FRichCurve& Curve = Profile->Settings.DiffuseRamp.ColorCurves[Channel];
            Curve.Reset();
            for (int32 Index = 0; Index < UE_ARRAY_COUNT(Times); ++Index)
            {
                const FKeyHandle Key = Curve.AddKey(Times[Index], Values[Index]);
                Curve.SetKeyInterpMode(Key, RCIM_CubicAuto);
            }
        }
        Profile->Settings.DiffuseIndirectScale = 1.0f;
        Profile->Settings.SpecularIndirectScale = 1.0f;
        Profile->Settings.bDiffuseRampIncludeShadow = false;
        Profile->PostEditChange();
        return SaveAsset(Profile, PackageName) ? Profile : nullptr;
    }

    UMaterial* MakeMaterial(const FSurfaceSample& Sample, const bool bBanded, UToonProfile* Profile)
    {
        const FString Variant = bBanded ? TEXT("Band") : TEXT("Smooth");
        const FString PackageName = FString::Printf(TEXT("%s/M_L03%s_%s"), TestRoot, *Variant, Sample.Name);
        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension())) return nullptr;
        if (IFileManager::Get().FileExists(*Filename))
        {
            const FString Path = FString::Printf(TEXT("%s.M_L03%s_%s"), *PackageName, *Variant, Sample.Name);
            return LoadObject<UMaterial>(nullptr, *Path);
        }
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        UPackage* Package = CreatePackage(*PackageName);
        const FName Name(*FPackageName::GetLongPackageAssetName(PackageName));
        UMaterial* Material = NewObject<UMaterial>(Package, Name, RF_Public | RF_Standalone);
        if (Material == nullptr || Material->GetEditorOnlyData() == nullptr) return nullptr;
        Material->SetMaterialDomain(MD_Surface);
        Material->SetBlendMode(BLEND_Opaque);

        UMaterialExpressionConstant3Vector* Color = NewObject<UMaterialExpressionConstant3Vector>(Material);
        Color->Constant = Sample.Color;
        Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Color);
        UMaterialExpressionConstant* Roughness = NewObject<UMaterialExpressionConstant>(Material);
        Roughness->R = Sample.Roughness;
        Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Roughness);

        UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
        if (bBanded)
        {
            if (Profile == nullptr) return nullptr;
            UMaterialExpressionSubstrateToonBSDF* Toon = NewObject<UMaterialExpressionSubstrateToonBSDF>(Material);
            Toon->ToonProfile = Profile;
            Toon->BaseColor.Connect(0, Color);
            Toon->Roughness.Connect(0, Roughness);
            Data->ExpressionCollection.Expressions.Add(Toon);
            Data->FrontMaterial.Connect(0, Toon);
        }
        else
        {
            Material->SetShadingModel(MSM_DefaultLit);
            Data->BaseColor.Connect(0, Color);
            Data->Roughness.Connect(0, Roughness);
        }
        Material->PostEditChange();
        return SaveAsset(Material, PackageName) ? Material : nullptr;
    }

    AStaticMeshActor* AddMesh(UWorld* World, UStaticMesh* Mesh, UMaterialInterface* Material,
        const FVector Location, const FVector Scale, const TCHAR* Name)
    {
        FActorSpawnParameters Spawn;
        Spawn.Name = MakeUniqueObjectName(World->PersistentLevel, AStaticMeshActor::StaticClass(), FName(Name));
        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location,
            FRotator::ZeroRotator, Spawn);
        if (Actor == nullptr || Actor->GetStaticMeshComponent() == nullptr) return nullptr;
        Actor->SetActorScale3D(Scale);
        Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        Actor->GetStaticMeshComponent()->SetMaterial(0, Material);
        Actor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Actor->Tags.Add(TEXT("KalmalaL03ComparisonFixture"));
        return Actor;
    }

    bool AddLightingRig(UWorld* World)
    {
        ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),
            FVector(0.0, 0.0, 2000.0), FRotator(-40.0, -35.0, 0.0));
        ASkyLight* Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        ASkyAtmosphere* Atmosphere = World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        if (Sun == nullptr || Sky == nullptr || Atmosphere == nullptr) return false;
        Sun->Tags.Add(TEXT("KalmalaEnvironmentSun"));
        Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sun->GetLightComponent()->SetIntensity(10.0f);
        Sun->GetLightComponent()->SetAtmosphereSunLight(true);
        Sky->Tags.Add(TEXT("KalmalaEnvironmentSkyLight"));
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->SetIntensity(1.0f);
        Sky->GetLightComponent()->SetSourceType(SLS_CapturedScene);
        Sky->GetLightComponent()->bRealTimeCapture = true;
        return true;
    }
}

UCreateLightingComparisonFixtureCommandlet::UCreateLightingComparisonFixtureCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UCreateLightingComparisonFixtureCommandlet::Main(const FString& Params)
{
    FString MapFilename;
    if (!FPackageName::TryConvertLongPackageNameToFilename(TestMap, MapFilename, FPackageName::GetMapPackageExtension())) return 1;
    if (IFileManager::Get().FileExists(*MapFilename))
    {
        UE_LOG(LogTemp, Error, TEXT("L03 fixture map already exists; refusing to overwrite it."));
        return 1;
    }
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(MapFilename), true);

    UToonProfile* Profile = MakeProfile();
    if (Profile == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("L03 fixture could not create its test-only three-band Toon Profile."));
        return 1;
    }
    UMaterial* Smooth[UE_ARRAY_COUNT(Samples)] = {};
    UMaterial* Banded[UE_ARRAY_COUNT(Samples)] = {};
    UStaticMesh* Meshes[UE_ARRAY_COUNT(Samples)] = {};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Samples); ++Index)
    {
        Smooth[Index] = MakeMaterial(Samples[Index], false, Profile);
        Banded[Index] = MakeMaterial(Samples[Index], true, Profile);
        Meshes[Index] = LoadObject<UStaticMesh>(nullptr, Samples[Index].MeshPath);
        if (Smooth[Index] == nullptr || Banded[Index] == nullptr || Meshes[Index] == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("L03 fixture dependency failed for %s."), Samples[Index].Name);
            return 1;
        }
    }

    UPackage* Package = CreatePackage(TestMap);
    UWorldFactory* Factory = NewObject<UWorldFactory>();
    UWorld* World = Cast<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Package, TEXT("L_LightingComparison"),
        RF_Public | RF_Standalone, nullptr, GWarn));
    if (World == nullptr || !AddLightingRig(World))
    {
        UE_LOG(LogTemp, Error, TEXT("L03 fixture could not create its isolated world and lighting rig."));
        return 1;
    }

    struct FAnchor { const TCHAR* Site; double X; double Y; double Z; };
    static const FAnchor Anchors[] =
    {
        { TEXT("MeadowsDaylight"), -781.6, -781.6, 528.94458 },
        { TEXT("ForestShade"), 207132.4, -325940.4, 15.1806488 },
        { TEXT("OpenWater"), 970004.9, -243087.4, 0.0 },
        { TEXT("OvercastRain"), -970004.9, -243087.4, 768.734985 },
    };
    for (const FAnchor& Anchor : Anchors)
    {
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Samples); ++Index)
        {
            const FSurfaceSample& Sample = Samples[Index];
            const double RowY = Anchor.Y + Index * 230.0;
            const double HalfHeight = Index == 0 ? 1.0 : Sample.Scale.Z * 50.0;
            const FVector SmoothLocation(Anchor.X - 315.0, RowY, Anchor.Z + HalfHeight);
            const FVector BandedLocation(Anchor.X + 315.0, RowY, Anchor.Z + HalfHeight);
            if (AddMesh(World, Meshes[Index], Smooth[Index], SmoothLocation, Sample.Scale,
                    *FString::Printf(TEXT("L03Smooth_%s_%s"), Anchor.Site, Sample.Name)) == nullptr
                || AddMesh(World, Meshes[Index], Banded[Index], BandedLocation, Sample.Scale,
                    *FString::Printf(TEXT("L03Band_%s_%s"), Anchor.Site, Sample.Name)) == nullptr)
            {
                UE_LOG(LogTemp, Error, TEXT("L03 fixture could not place %s at %s."), Sample.Name, Anchor.Site);
                return 1;
            }
        }
    }
    if (World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator) == nullptr
        || !SaveAsset(World, TestMap, true))
    {
        UE_LOG(LogTemp, Error, TEXT("L03 fixture map could not be saved."));
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("Created isolated L03 fixture: paired Default Lit and soft three-band materials on six surface proxies at four L01 anchor placements."));
    return 0;
}
