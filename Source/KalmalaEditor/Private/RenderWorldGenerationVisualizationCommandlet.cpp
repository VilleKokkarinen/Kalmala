#include "RenderWorldGenerationVisualizationCommandlet.h"

#include "HAL/FileManager.h"
#include "KalmalaBiomeClassifier.h"
#include "KalmalaForestLayout.h"
#include "KalmalaGenerationPreview.h"
#include "KalmalaRegionalGeneration.h"
#include "KalmalaTerrainHeightSampler.h"
#include "KalmalaTerrainPatchLayout.h"
#include "KalmalaWaterSurfaceMesh.h"
#include "KalmalaWorldFieldSampler.h"
#include "KalmalaWorldPlayerStartResolver.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"

namespace KalmalaWorldGenerationVisualization
{
    constexpr int32 DefaultImageSize = 256;
    constexpr float DefaultWorldExtent = 20000.0f;

    struct FVisualizationPixel
    {
        uint8 R;
        uint8 G;
        uint8 B;
    };

    FVisualizationPixel GetBiomeColor(const EKalmalaBiome Biome)
    {
        switch (Biome)
        {
        case EKalmalaBiome::Ocean: return { 23, 88, 160 };
        case EKalmalaBiome::ShimmeringLakes: return { 61, 177, 190 };
        case EKalmalaBiome::Elderwood: return { 30, 100, 47 };
        case EKalmalaBiome::MossyMire: return { 113,79,55 };
        case EKalmalaBiome::FreezingTundra: return { 153, 153, 151 };
        case EKalmalaBiome::ThunderMountains: return { 240, 240, 240 };
        case EKalmalaBiome::Meadows: return { 131, 174, 76 };
        default: return { 255, 0, 255 };
        }
    }

    bool WritePpm(const FString& Filename, const int32 ImageSize, const TArray<FVisualizationPixel>& Pixels)
    {
        TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Filename));
        if (!Writer)
        {
            return false;
        }

        const FTCHARToUTF8 Header(*FString::Printf(TEXT("P6\n%d %d\n255\n"), ImageSize, ImageSize));
        Writer->Serialize(const_cast<ANSICHAR*>(Header.Get()), Header.Length());
        Writer->Serialize(const_cast<FVisualizationPixel*>(Pixels.GetData()), Pixels.Num() * sizeof(FVisualizationPixel));
        return !Writer->IsError();
    }

    struct FCollisionTriangleSample
    {
        float Height = 0.0f;
        float SlopeDegrees = 0.0f;
        float GradePercent = 0.0f;
        int32 CellX = INDEX_NONE;
        int32 CellY = INDEX_NONE;
        uint8 TriangleIndex = 0;
    };

    bool TrySampleCollisionTriangle(const FKalmalaWorldGenerationConfig& Config, const FVector2D Position,
        const FVector2D PatchCenter, FCollisionTriangleSample& OutSample)
    {
        constexpr int32 CellsPerSide = FKalmalaWaterSurfaceMesh::CellsPerSide;
        constexpr float SurfaceSize = FKalmalaTerrainPatchLayout::PatchSize;
        constexpr float HalfSurfaceSize = SurfaceSize * 0.5f;
        constexpr float CellSize = SurfaceSize / CellsPerSide;
        static_assert(CellsPerSide > 0 && CellSize > 0.0f, "Collision triangle sampling requires a positive grid.");

        const float GridX = (Position.X - PatchCenter.X + HalfSurfaceSize) / CellSize;
        const float GridY = (Position.Y - PatchCenter.Y + HalfSurfaceSize) / CellSize;
        if (!FMath::IsFinite(GridX) || !FMath::IsFinite(GridY))
        {
            return false;
        }

        const int32 CellX = FMath::Clamp(FMath::FloorToInt(GridX), 0, CellsPerSide - 1);
        const int32 CellY = FMath::Clamp(FMath::FloorToInt(GridY), 0, CellsPerSide - 1);
        const float U = FMath::Clamp(GridX - CellX, 0.0f, 1.0f);
        const float V = FMath::Clamp(GridY - CellY, 0.0f, 1.0f);
        const float X0 = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(CellX) / CellsPerSide);
        const float X1 = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(CellX + 1) / CellsPerSide);
        const float Y0 = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(CellY) / CellsPerSide);
        const float Y1 = FMath::Lerp(-HalfSurfaceSize, HalfSurfaceSize, static_cast<float>(CellY + 1) / CellsPerSide);
        const FVector2D BottomLeftPosition = PatchCenter + FVector2D(X0, Y0);
        const FVector2D BottomRightPosition = PatchCenter + FVector2D(X1, Y0);
        const FVector2D TopLeftPosition = PatchCenter + FVector2D(X0, Y1);
        const FVector2D TopRightPosition = PatchCenter + FVector2D(X1, Y1);
        const float BottomLeft = FKalmalaTerrainHeightSampler::SampleHeight(Config, BottomLeftPosition);
        const float BottomRight = FKalmalaTerrainHeightSampler::SampleHeight(Config, BottomRightPosition);
        const float TopLeft = FKalmalaTerrainHeightSampler::SampleHeight(Config, TopLeftPosition);
        const float TopRight = FKalmalaTerrainHeightSampler::SampleHeight(Config, TopRightPosition);
        const float CellWidth = X1 - X0;
        const float CellHeight = Y1 - Y0;
        if (!FMath::IsFinite(BottomLeft) || !FMath::IsFinite(BottomRight)
            || !FMath::IsFinite(TopLeft) || !FMath::IsFinite(TopRight)
            || CellWidth <= 0.0f || CellHeight <= 0.0f)
        {
            return false;
        }

        const bool bFirstTriangle = U + V <= 1.0f;
        float HeightGradientX = 0.0f;
        float HeightGradientY = 0.0f;
        if (bFirstTriangle)
        {
            OutSample.Height = BottomLeft * (1.0f - U - V) + BottomRight * U + TopLeft * V;
            HeightGradientX = (BottomRight - BottomLeft) / CellWidth;
            HeightGradientY = (TopLeft - BottomLeft) / CellHeight;
        }
        else
        {
            OutSample.Height = BottomRight * (1.0f - V) + TopLeft * (1.0f - U) + TopRight * (U + V - 1.0f);
            HeightGradientX = (TopRight - TopLeft) / CellWidth;
            HeightGradientY = (TopRight - BottomRight) / CellHeight;
        }

        const float GradientMagnitude = FMath::Sqrt(HeightGradientX * HeightGradientX + HeightGradientY * HeightGradientY);
        OutSample.SlopeDegrees = FMath::RadiansToDegrees(FMath::Atan(GradientMagnitude));
        OutSample.GradePercent = GradientMagnitude * 100.0f;
        OutSample.CellX = CellX;
        OutSample.CellY = CellY;
        OutSample.TriangleIndex = bFirstTriangle ? 0 : 1;
        return FMath::IsFinite(OutSample.Height) && FMath::IsFinite(OutSample.SlopeDegrees)
            && FMath::IsFinite(OutSample.GradePercent);
    }

    bool ExportNumericSamples(const FString& InputPath, const FString& OutputPath)
    {
        TArray<FString> Lines;
        if (!FFileHelper::LoadFileToStringArray(Lines, *InputPath) || Lines.Num() < 2)
        {
            UE_LOG(LogTemp, Error, TEXT("Could not load numeric sample rows from %s."), *InputPath);
            return false;
        }

        Lines[0].TrimStartAndEndInline();
        if (!Lines[0].Equals(TEXT("Seed,Site,Xcm,Ycm"), ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Error, TEXT("Sample input must start with Seed,Site,Xcm,Ycm."));
            return false;
        }

        FString Csv = TEXT("Seed,Site,Xcm,Ycm,BiomeIndex,FinalHeightCm,MeadowsWeight,ShimmeringLakesWeight,ElderwoodWeight,MossyMireWeight,FreezingTundraWeight,ThunderMountainsWeight,OceanWeight,WaterLevelCm,HasWater,CollisionTriangleHeightCm,CollisionTriangleSlopeDegrees,CollisionTriangleGradePercent,PatchOriginXcm,PatchOriginYcm,PatchX,PatchY,PatchCenterXcm,PatchCenterYcm,PatchCellX,PatchCellY,PatchTriangle,ForestDescriptorCount\n");
        int32 SampleCount = 0;
        for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
        {
            FString Line = Lines[LineIndex];
            Line.TrimStartAndEndInline();
            if (Line.IsEmpty())
            {
                continue;
            }

            TArray<FString> Columns;
            Line.ParseIntoArray(Columns, TEXT(","), false);
            if (Columns.Num() != 4)
            {
                UE_LOG(LogTemp, Error, TEXT("Sample input row %d must have Seed,Site,Xcm,Ycm."), LineIndex + 1);
                return false;
            }
            for (FString& Column : Columns)
            {
                Column.TrimStartAndEndInline();
            }

            uint64 Seed = 0;
            float X = 0.0f;
            float Y = 0.0f;
            if (Columns[1].IsEmpty() || Columns[0].StartsWith(TEXT("-"))
                || !LexTryParseString(Seed, *Columns[0])
                || !LexTryParseString(X, *Columns[2])
                || !LexTryParseString(Y, *Columns[3])
                || !FMath::IsFinite(X) || !FMath::IsFinite(Y))
            {
                UE_LOG(LogTemp, Error, TEXT("Sample input row %d has an invalid seed, site, or coordinate."), LineIndex + 1);
                return false;
            }

            FKalmalaWorldGenerationConfig Config;
            Config.WorldSeed = Seed;
            const FVector StartLocation = FKalmalaWorldPlayerStartResolver::ResolveStartTransform(Config).GetLocation();
            const FVector2D PatchOrigin(StartLocation.X, StartLocation.Y);
            const FVector2D Position(X, Y);
            const FIntPoint PatchCoordinate = FKalmalaTerrainPatchLayout::GetPatchCoordinate(PatchOrigin, Position);
            const FVector2D PatchCenter = FKalmalaTerrainPatchLayout::GetPatchCenter(PatchOrigin, PatchCoordinate.X, PatchCoordinate.Y);
            FCollisionTriangleSample TriangleSample;
            if (!TrySampleCollisionTriangle(Config, Position, PatchCenter, TriangleSample))
            {
                UE_LOG(LogTemp, Error, TEXT("Sample input row %d could not resolve its collision triangle."), LineIndex + 1);
                return false;
            }
            const int32 ForestDescriptorCount = FKalmalaForestLayout::Generate(Config, PatchCenter).Num();
            const FKalmalaWorldFieldSample Fields = FKalmalaWorldFieldSampler::Sample(Config, Position);
            const FKalmalaRegionalSample Region = FKalmalaRegionalGeneration::Sample(Fields);
            if (!FMath::IsFinite(Region.Height) || !FMath::IsFinite(Region.WaterLevel))
            {
                UE_LOG(LogTemp, Error, TEXT("Sample input row %d produced a non-finite height or water level."), LineIndex + 1);
                return false;
            }
            for (float Weight : Region.Weights)
            {
                if (!FMath::IsFinite(Weight))
                {
                    UE_LOG(LogTemp, Error, TEXT("Sample input row %d produced a non-finite biome weight."), LineIndex + 1);
                    return false;
                }
            }

            Csv += FString::Printf(TEXT("%llu,%s,%.1f,%.1f,%u,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%.9g,%.9g,%.9g,%.3f,%.3f,%d,%d,%.3f,%.3f,%d,%d,%u,%d\n"),
                static_cast<unsigned long long>(Seed), *Columns[1], X, Y, static_cast<unsigned int>(Region.Biome), Region.Height,
                Region.Weights[0], Region.Weights[1], Region.Weights[2], Region.Weights[3],
                Region.Weights[4], Region.Weights[5], Region.Weights[6], Region.WaterLevel, Region.bHasWater ? 1 : 0,
                TriangleSample.Height, TriangleSample.SlopeDegrees, TriangleSample.GradePercent,
                PatchOrigin.X, PatchOrigin.Y, PatchCoordinate.X, PatchCoordinate.Y, PatchCenter.X, PatchCenter.Y,
                TriangleSample.CellX, TriangleSample.CellY, static_cast<unsigned int>(TriangleSample.TriangleIndex), ForestDescriptorCount);
            ++SampleCount;
        }

        const FString OutputDirectory = FPaths::GetPath(OutputPath);
        if (SampleCount == 0
            || (!OutputDirectory.IsEmpty() && !IFileManager::Get().MakeDirectory(*OutputDirectory, true))
            || !FFileHelper::SaveStringToFile(Csv, *OutputPath))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not write numeric samples to %s."), *OutputPath);
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("Exported %d numeric world-generation samples to %s."), SampleCount, *OutputPath);
        return true;
    }
}

URenderWorldGenerationVisualizationCommandlet::URenderWorldGenerationVisualizationCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 URenderWorldGenerationVisualizationCommandlet::Main(const FString& Params)
{
    using namespace KalmalaWorldGenerationVisualization;

    FKalmalaWorldGenerationConfig Config;
    Config.WorldSeed = 10323456789ull;
    int32 ImageSize = DefaultImageSize;
    float WorldExtent = DefaultWorldExtent;

    FParse::Value(*Params, TEXT("Seed="), Config.WorldSeed);
    FParse::Value(*Params, TEXT("Size="), ImageSize);
    FParse::Value(*Params, TEXT("Extent="), WorldExtent);

    if (!Config.IsValid() || ImageSize < 16 || ImageSize > 2048 || WorldExtent <= 0.0f)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid visualization parameters. Size must be 16-2048, and Extent must be positive."));
        return 1;
    }

    FString HillAmplitudeText;
    FString RollingAmplitudeText;
    const bool bHasHillAmplitude = FParse::Value(*Params, TEXT("MeadowHillAmplitudeM="), HillAmplitudeText);
    const bool bHasRollingAmplitude = FParse::Value(*Params, TEXT("MeadowRollingAmplitudeM="), RollingAmplitudeText);
    if (bHasHillAmplitude || bHasRollingAmplitude)
    {
        FKalmalaGenerationPreviewSettings Trial = FKalmalaGenerationPreview::Get();
        double HillAmplitudeM = 0.0;
        double RollingAmplitudeM = 0.0;
        if (bHasHillAmplitude)
        {
            if (!LexTryParseString(HillAmplitudeM, *HillAmplitudeText) || !FMath::IsFinite(HillAmplitudeM)
                || HillAmplitudeM < 4.0 || HillAmplitudeM > 10.0)
            {
                UE_LOG(LogTemp, Error, TEXT("MeadowHillAmplitudeM must be finite and between 4 and 10 metres."));
                return 1;
            }

            Trial.MeadowHillAmplitude = HillAmplitudeM * 100.0;
        }

        if (bHasRollingAmplitude)
        {
            if (!LexTryParseString(RollingAmplitudeM, *RollingAmplitudeText) || !FMath::IsFinite(RollingAmplitudeM)
                || RollingAmplitudeM < 0.6 || RollingAmplitudeM > 1.8)
            {
                UE_LOG(LogTemp, Error, TEXT("MeadowRollingAmplitudeM must be finite and between 0.6 and 1.8 metres."));
                return 1;
            }

            Trial.MeadowRollingAmplitude = RollingAmplitudeM * 100.0;
        }

        if (!Trial.HasValidReliefControls() || !FKalmalaGenerationPreview::Set(Trial))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not install the commandlet-only Meadows terrain trial."));
            return 1;
        }
        if (bHasHillAmplitude)
        {
            UE_LOG(LogTemp, Display, TEXT("Meadow hill sample trial accepted: AmplitudeM=%.3f"), HillAmplitudeM);
        }
        if (bHasRollingAmplitude)
        {
            UE_LOG(LogTemp, Display, TEXT("Meadow rolling sample trial accepted: AmplitudeM=%.3f"), RollingAmplitudeM);
        }
    }
    ON_SCOPE_EXIT { if (bHasHillAmplitude || bHasRollingAmplitude) FKalmalaGenerationPreview::Reset(); };

    FString SampleInputPath;
    FString SampleOutputPath;
    if (FParse::Value(*Params, TEXT("SampleInput="), SampleInputPath))
    {
        if (!FParse::Value(*Params, TEXT("SampleOutput="), SampleOutputPath) || SampleOutputPath.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("SampleInput requires SampleOutput."));
            return 1;
        }
        return ExportNumericSamples(SampleInputPath, SampleOutputPath) ? 0 : 1;
    }

    FString OutputDirectory = FPaths::ProjectContentDir() / TEXT("Kalmala/Developer/WorldGeneration");
    FParse::Value(*Params, TEXT("Output="), OutputDirectory);
    IFileManager::Get().MakeDirectory(*OutputDirectory, true);

    TArray<FVisualizationPixel> ElevationPixels;
    TArray<FVisualizationPixel> HumidityPixels;
    TArray<FVisualizationPixel> TemperaturePixels;
    TArray<FVisualizationPixel> FloraPixels;
    TArray<FVisualizationPixel> BiomePixels;
    TArray<FVisualizationPixel> WeightPixels[7], OverlapPixels, HydrologyPixels, HeightPixels, BoundaryPixels;
    TArray<uint8> BiomeIds;
    TArray<FVisualizationPixel> MasterPixels, LandWaterPixels;
    ElevationPixels.Reserve(ImageSize * ImageSize);
    HumidityPixels.Reserve(ImageSize * ImageSize);
    TemperaturePixels.Reserve(ImageSize * ImageSize);
    FloraPixels.Reserve(ImageSize * ImageSize);
    BiomePixels.Reserve(ImageSize * ImageSize);

    const auto AddFieldPixel = [](TArray<FVisualizationPixel>& Pixels, const float Value)
    {
        const uint8 Shade = static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 255.0f));
        Pixels.Add({ Shade, Shade, Shade });
    };

    for (int32 Y = 0; Y < ImageSize; ++Y)
    {
        for (int32 X = 0; X < ImageSize; ++X)
        {
            const FVector2D NormalizedPosition(
                static_cast<float>(X) / static_cast<float>(ImageSize - 1),
                static_cast<float>(Y) / static_cast<float>(ImageSize - 1));
            const FVector2D WorldPosition = (NormalizedPosition - FVector2D(0.5f, 0.5f)) * WorldExtent;
            const FKalmalaWorldFieldSample Sample = FKalmalaWorldFieldSampler::Sample(Config, WorldPosition);

            {
                const FVector2D AtlasPosition = (NormalizedPosition - FVector2D(0.5)) * (FKalmalaMasterMap::HalfExtent * 2);
                const bool bLand = FKalmalaMasterMap::SampleMaster(AtlasPosition) > 0;
                const auto Crop = FKalmalaMasterMap::Crop(Config);
                const bool bCropEdge = FMath::Abs((AtlasPosition - Crop.Center).Size() - FKalmalaWorldBounds::Radius)
                    < FKalmalaMasterMap::HalfExtent * 2 / ImageSize;
                MasterPixels.Add(bCropEdge ? FVisualizationPixel{255, 230, 150} : bLand ? FVisualizationPixel{131, 174, 76} : FVisualizationPixel{23, 88, 160});
                LandWaterPixels.Add(!FKalmalaWorldBounds::Contains(Config, WorldPosition) ? FVisualizationPixel{15, 19, 27}
                    : FKalmalaMasterMap::Sample(Config, WorldPosition) > 0 ? FVisualizationPixel{131, 174, 76} : FVisualizationPixel{23, 88, 160});
            }

            AddFieldPixel(ElevationPixels, Sample.Elevation);
            AddFieldPixel(HumidityPixels, Sample.Humidity);
            AddFieldPixel(TemperaturePixels, Sample.Temperature);
            AddFieldPixel(FloraPixels, Sample.Flora);
            BiomePixels.Add(GetBiomeColor(FKalmalaBiomeClassifier::Classify(Sample)));

            {
                const auto Region = FKalmalaRegionalGeneration::Sample(Sample);
                for (int32 I = 0; I < 7; ++I) AddFieldPixel(WeightPixels[I], Region.Weights[I]);
                float Strongest = 0;
                for (float W : Region.Weights) Strongest = FMath::Max(Strongest, W);
                AddFieldPixel(OverlapPixels, 1 - Strongest);
                AddFieldPixel(HeightPixels, (Region.Height + 500) / 2500.0f);
                HydrologyPixels.Add({ uint8(Region.RiverWeight * 255), uint8(Region.StreamWeight * 255), uint8(Region.BasinWeight * 255) });
                BiomeIds.Add(Region.Biome);
                const int32 Index = BiomeIds.Num() - 1;
                const bool Edge = (X > 0 && BiomeIds[Index - 1] != Region.Biome)
                    || (Y > 0 && BiomeIds[Index - ImageSize] != Region.Biome);
                BoundaryPixels.Add(Edge ? FVisualizationPixel{255, 40, 40} : BiomePixels.Last());
                if (!FKalmalaWorldBounds::Contains(Config, WorldPosition))
                {
                    BiomePixels.Last() = BoundaryPixels.Last() = FVisualizationPixel{15, 19, 27};
                }
            }
        }
    }

    bool bWroteAllImages =
        WritePpm(OutputDirectory / TEXT("Elevation.ppm"), ImageSize, ElevationPixels) &&
        WritePpm(OutputDirectory / TEXT("Humidity.ppm"), ImageSize, HumidityPixels) &&
        WritePpm(OutputDirectory / TEXT("Temperature.ppm"), ImageSize, TemperaturePixels) &&
        WritePpm(OutputDirectory / TEXT("Flora.ppm"), ImageSize, FloraPixels) &&
        WritePpm(OutputDirectory / TEXT("BiomeClassification.ppm"), ImageSize, BiomePixels);

    {
        // Overlay the actual indexed splines, so subpixel-width streams remain legible.
        const int32 First = int32(FMath::FloorToInt(-WorldExtent * 0.5 / FKalmalaRegionalTuning::GridCell));
        const int32 Last = int32(FMath::FloorToInt(WorldExtent * 0.5 / FKalmalaRegionalTuning::GridCell));
        for (int32 Y = First; !FParse::Param(*Params, TEXT("SkipSplineOverlay")) && Y <= Last; ++Y) for (int32 X = First; X <= Last; ++X)
        for (const auto& Segment : FKalmalaRegionalGeneration::GetHydrology(Config, FIntPoint(X, Y)))
        {
            const FVector2D A = (FVector2D(Segment.A) / WorldExtent + FVector2D(0.5)) * (ImageSize - 1);
            const FVector2D B = (FVector2D(Segment.B) / WorldExtent + FVector2D(0.5)) * (ImageSize - 1);
            const int32 Steps = FMath::Max(1, int32(FMath::CeilToInt(FVector2D::Distance(A, B))));
            for (int32 I = 0; I <= Steps; ++I)
            {
                const auto P = FMath::Lerp(A, B, double(I) / Steps);
                const int32 PX = int32(FMath::RoundToInt(P.X)), PY = int32(FMath::RoundToInt(P.Y));
                if (PX >= 0 && PX < ImageSize && PY >= 0 && PY < ImageSize)
                {
                    auto& Pixel = HydrologyPixels[PY * ImageSize + PX];
                    if (Segment.bStream) Pixel.G = 255; else Pixel.R = 255;
                }
            }
        }
        for (int32 I = 0; I < 7; ++I) bWroteAllImages &= WritePpm(OutputDirectory / FString::Printf(TEXT("Weight%d.ppm"), I), ImageSize, WeightPixels[I]);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("Overlap.ppm"), ImageSize, OverlapPixels);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("Hydrology.ppm"), ImageSize, HydrologyPixels);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("ShapedHeight.ppm"), ImageSize, HeightPixels);
        bWroteAllImages &= WritePpm(OutputDirectory / TEXT("Boundaries.ppm"), ImageSize, BoundaryPixels);
        FString Tuning = FString::Printf(TEXT("Seed=%llu\nBiomeScale=%.0f RegionFrequency=%.9f ElevationFrequency=%.9f ClimateFrequency=%.9f\nWarpStrength=%.0f WarpFrequency=%.9f EdgeWaveStrength=%.3f RingOverlap=%.3f\nRiverSpacing=%.0f MergeDistance=%.0f RiverRange=%.0f SplineAmplitude=%.0f SplineWavelength=%.0f SplineStep=%.0f GridCell=%.0f\n"),
            Config.WorldSeed, FKalmalaRegionalTuning::BiomeScale, FKalmalaRegionalTuning::RegionFrequency,
            FKalmalaRegionalTuning::ElevationFrequency, FKalmalaRegionalTuning::ClimateFrequency,
            FKalmalaRegionalTuning::WarpStrength, FKalmalaRegionalTuning::WarpFrequency, FKalmalaRegionalTuning::EdgeWaveStrength, FKalmalaRegionalTuning::RingOverlap,
            FKalmalaRegionalTuning::RiverSpacing, FKalmalaRegionalTuning::MergeDistance, FKalmalaRegionalTuning::RiverRange, FKalmalaRegionalTuning::SplineAmplitude,
            FKalmalaRegionalTuning::SplineWavelength, FKalmalaRegionalTuning::SplineStep, FKalmalaRegionalTuning::GridCell);

        {
            const auto Crop = FKalmalaMasterMap::Crop(Config);
            Tuning += FString::Printf(TEXT("MasterSeed=%llu MasterHalfExtent=%.0f MasterWavelength=%.0f CropX=%.3f CropY=%.3f RotationRadians=%.9f WorldRadius=%.0f\nStarterRadius=35000 ElderwoodMin=75000 LakesMin=35000 LakesMax=300000 MireMin=300000 MireMax=1600000 TundraMin=400000 MeadowsMax=400000 EligibilityBlend=5000\n"),
                FKalmalaMasterMap::MasterSeed, FKalmalaMasterMap::HalfExtent, FKalmalaMasterMap::Wavelength,
                Crop.Center.X, Crop.Center.Y, Crop.Rotation, FKalmalaWorldBounds::Radius);
            bWroteAllImages &= WritePpm(OutputDirectory / TEXT("MasterLandWater.ppm"), ImageSize, MasterPixels);
            bWroteAllImages &= WritePpm(OutputDirectory / TEXT("LandWaterCrop.ppm"), ImageSize, LandWaterPixels);
        }
        bWroteAllImages &= FFileHelper::SaveStringToFile(Tuning, *(OutputDirectory / TEXT("Tuning.txt")));
    }

    if (!bWroteAllImages)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not write all world-generation visualization images to %s."), *OutputDirectory);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Rendered world-generation previews for seed %llu to %s."), Config.WorldSeed, *OutputDirectory);
    return 0;
}
