#include "ExportWorldMapsCommandlet.h"

#include "KalmalaGenerationPreview.h"
#include "KalmalaMasterMap.h"
#include "KalmalaRegionalGeneration.h"
#include "ImageUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/ScopeExit.h"

namespace
{
    struct FRequest
    {
        FKalmalaWorldGenerationConfig World{418};
        FKalmalaGenerationPreviewSettings Tuning;
        int32 Size = 512;
    };

    bool ParseRequest(const FString& Text, FRequest& R)
    {
        TArray<FString> Lines;
        Text.ParseIntoArrayLines(Lines);
        TSet<FString> Seen;
        for (FString Line : Lines)
        {
            Line.TrimStartAndEndInline();
            if (Line.IsEmpty() || Line.StartsWith(TEXT("#"))) continue;
            FString Key, Value;
            if (!Line.Split(TEXT("="), &Key, &Value)) return false;
            Key.TrimStartAndEndInline(); Value.TrimStartAndEndInline();
            if (Seen.Contains(Key)) return false;
            Seen.Add(Key);
            if (Key == TEXT("WorldSeed")) { if (!LexTryParseString(R.World.WorldSeed, *Value) || Value.StartsWith(TEXT("-"))) return false; }
            else if (Key == TEXT("Size")) { if (!LexTryParseString(R.Size, *Value)) return false; }
            else if (Key == TEXT("MasterSeed")) { if (!LexTryParseString(R.Tuning.MasterSeed, *Value) || Value.StartsWith(TEXT("-"))) return false; }
            else
            {
                double Number;
                if (!LexTryParseString(Number, *Value) || !FMath::IsFinite(Number)) return false;
                if (Key == TEXT("LandThreshold")) R.Tuning.LandThreshold = Number;
                else if (Key == TEXT("MasterWavelengthKm")) R.Tuning.MasterWavelength = Number * 100000;
                else if (Key == TEXT("BiomeScaleKm")) R.Tuning.BiomeScale = Number * 100000;
                else if (Key == TEXT("WarpStrengthKm")) R.Tuning.WarpStrength = Number * 100000;
                else if (Key == TEXT("StarterRadiusKm")) R.Tuning.StarterRadius = Number * 100000;
                else if (Key == TEXT("ElderwoodMinimumKm")) R.Tuning.ElderwoodMinimum = Number * 100000;
                else if (Key == TEXT("LakesMinimumKm")) R.Tuning.LakesMinimum = Number * 100000;
                else if (Key == TEXT("LakesMaximumKm")) R.Tuning.LakesMaximum = Number * 100000;
                else if (Key == TEXT("MireMaximumKm")) R.Tuning.MireMaximum = Number * 100000;
                else if (Key == TEXT("WetlandHumidityMinimum")) R.Tuning.WetlandHumidityMinimum = Number;
                else if (Key == TEXT("WetlandHumidityFull")) R.Tuning.WetlandHumidityFull = Number;
                else if (Key == TEXT("WetlandElevationFull")) R.Tuning.WetlandElevationFull = Number;
                else if (Key == TEXT("WetlandElevationMaximum")) R.Tuning.WetlandElevationMaximum = Number;
                else if (Key == TEXT("WetlandTemperatureMinimum")) R.Tuning.WetlandTemperatureMinimum = Number;
                else if (Key == TEXT("WetlandTemperatureFull")) R.Tuning.WetlandTemperatureFull = Number;
                else if (Key == TEXT("MireMinimumKm")) R.Tuning.MireMinimum = Number * 100000;
                else if (Key == TEXT("TundraMinimumKm")) R.Tuning.TundraMinimum = Number * 100000;
                else if (Key == TEXT("MeadowsMaximumKm")) R.Tuning.MeadowsMaximum = Number * 100000;
                else if (Key == TEXT("EligibilityBlendKm")) R.Tuning.EligibilityBlend = Number * 100000;
                else return false;
            }
        }
        const auto& T = R.Tuning;
        auto UnitRange = [](double Min, double Max) { return Min >= 0 && Min < Max && Max <= 1; };
        auto DistanceRange = [&](double Min, double Max) { return Min >= T.StarterRadius && Min < Max && Max <= FKalmalaWorldBounds::Radius; };
        return R.Size >= 64 && R.Size <= 2048
            && T.MasterWavelength >= 10000 && T.MasterWavelength <= 3200000
            && T.BiomeScale >= 10000 && T.BiomeScale <= 1600000
            && FMath::Abs(T.LandThreshold) <= .5 && T.WarpStrength >= 0 && T.WarpStrength <= 200000
            && T.StarterRadius >= 0 && T.StarterRadius <= T.ElderwoodMinimum
            && T.ElderwoodMinimum <= T.TundraMinimum && T.TundraMinimum <= FKalmalaWorldBounds::Radius
            && DistanceRange(T.LakesMinimum, T.LakesMaximum) && DistanceRange(T.MireMinimum, T.MireMaximum)
            && UnitRange(T.WetlandHumidityMinimum, T.WetlandHumidityFull)
            && UnitRange(T.WetlandElevationFull, T.WetlandElevationMaximum)
            && UnitRange(T.WetlandTemperatureMinimum, T.WetlandTemperatureFull)
            && T.MeadowsMaximum > T.StarterRadius && T.MeadowsMaximum <= FKalmalaWorldBounds::Radius
            && T.EligibilityBlend >= 100 && T.EligibilityBlend <= T.MeadowsMaximum;
    }

    bool WritePng(const FString& Path, int32 Size, const TArray<FColor>& Pixels)
    {
        TArray64<uint8> Compressed;
        FImageUtils::PNGCompressImageArray(Size, Size, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Compressed);
        const FString Temp = Path + TEXT(".tmp");
        return !Compressed.IsEmpty() && FFileHelper::SaveArrayToFile(Compressed, *Temp)
            && IFileManager::Get().Move(*Path, *Temp, true, true);
    }

    bool Render(const FRequest& R, const FString& Text, const FString& Output, bool bVerify)
    {
        if (!FKalmalaGenerationPreview::Set(R.Tuning)) return false;
        ON_SCOPE_EXIT { FKalmalaGenerationPreview::Reset(); };
        const double Start = FPlatformTime::Seconds();
        const FColor Land(131,174,76), Water(23,88,160), Exterior(15,19,27);
        const FColor Palette[] = {
            Land,
            FColor(61,177,190),
            FColor(30,100,47),
            FColor(113,79,55),
            FColor(224,206,164),
            FColor(203,202,204),
            Water
        };
        TArray<FColor> Master, Crop, Biomes;
        Master.SetNumUninitialized(R.Size * R.Size);
        Crop.SetNumUninitialized(R.Size * R.Size);
        Biomes.SetNumUninitialized(R.Size * R.Size);
        const auto Transform = FKalmalaMasterMap::Crop(R.World);
        for (int32 Y = 0; Y < R.Size; ++Y) for (int32 X = 0; X < R.Size; ++X)
        {
            const int32 I = Y * R.Size + X;
            const FVector2D UV(double(X) / (R.Size - 1) - .5, double(Y) / (R.Size - 1) - .5);
            Master[I] = FKalmalaMasterMap::SampleMaster(UV * (FKalmalaMasterMap::HalfExtent * 2)) > 0 ? Land : Water;
            const FVector2D P = UV * (FKalmalaWorldBounds::Radius * 2);
            Crop[I] = Biomes[I] = Exterior;
            if (!FKalmalaWorldBounds::Contains(R.World, P)) continue;
            const auto Fields = FKalmalaWorldFieldSampler::Sample(R.World, P);
            const bool bLand = FKalmalaMasterMap::SampleMaster(FKalmalaMasterMap::ToMasterPosition(Transform, P)) > 0;
            Crop[I] = bLand ? Land : Water;
            // Ocean requires no region/basin evaluation. Land uses the exact
            // shared classifier; only hydrology (which cannot select a biome) is omitted.
            Biomes[I] = bLand ? Palette[FKalmalaRegionalGeneration::SampleBiome(Fields)] : Water;
            if (bVerify && R.Tuning.LakesMinimum == R.Tuning.MireMinimum
                && R.Tuning.LakesMaximum == R.Tuning.MireMaximum)
            {
                const auto Full = FKalmalaRegionalGeneration::Sample(Fields);
                if (Full.Weights[1] != Full.Weights[3])
                {
                    UE_LOG(LogTemp, Error, TEXT("Shared wetland weights differ at pixel %d,%d"), X, Y);
                    return false;
                }
            }
            if (bVerify && Biomes[I] != Palette[FKalmalaRegionalGeneration::Sample(Fields).Biome])
            {
                UE_LOG(LogTemp, Error, TEXT("Fast/full biome mismatch at pixel %d,%d"), X, Y);
                return false;
            }
        }
        const double SampleSeconds = FPlatformTime::Seconds() - Start;
        if (!WritePng(Output / TEXT("MasterLandWater.png"), R.Size, Master)
            || !WritePng(Output / TEXT("LandWaterCrop.png"), R.Size, Crop)
            || !WritePng(Output / TEXT("Biomes.png"), R.Size, Biomes)) return false;
        FString Summary = FString::Printf(TEXT("Preview only; overrides do not change gameplay or saves.\nWorldSeed=%llu Size=%d\nCropX=%.3f CropY=%.3f RotationRadians=%.9f\nSamplingSeconds=%.3f TotalExportSeconds=%.3f\nPalette: Meadows=83AE4C Lakes=3DB1BE Elderwood=1E642F Mire=4C7137 Tundra=D5ECEE Mountains=686270 Ocean=1758A0\n\nRequested parameters:\n%s"),
            R.World.WorldSeed, R.Size, Transform.Center.X, Transform.Center.Y, Transform.Rotation,
            SampleSeconds, FPlatformTime::Seconds() - Start, *Text);
        const auto& T = R.Tuning;
        Summary += FString::Printf(TEXT("\nEffective tuning (cm): MasterSeed=%llu MasterWavelength=%.3f LandThreshold=%.6f BiomeScale=%.3f WarpStrength=%.3f\nStarterRadius=%.3f ElderwoodMinimum=%.3f LakesMinimum=%.3f LakesMaximum=%.3f MireMinimum=%.3f MireMaximum=%.3f TundraMinimum=%.3f MeadowsMaximum=%.3f EligibilityBlend=%.3f\n"),
            T.MasterSeed, T.MasterWavelength, T.LandThreshold, T.BiomeScale, T.WarpStrength,
            T.StarterRadius, T.ElderwoodMinimum, T.LakesMinimum, T.LakesMaximum, T.MireMinimum, T.MireMaximum, T.TundraMinimum, T.MeadowsMaximum, T.EligibilityBlend);
        Summary += FString::Printf(TEXT("WetlandHumidityMinimum=%.6f WetlandHumidityFull=%.6f WetlandElevationFull=%.6f WetlandElevationMaximum=%.6f WetlandTemperatureMinimum=%.6f WetlandTemperatureFull=%.6f\n"),
            T.WetlandHumidityMinimum, T.WetlandHumidityFull, T.WetlandElevationFull, T.WetlandElevationMaximum, T.WetlandTemperatureMinimum, T.WetlandTemperatureFull);
        if (!FFileHelper::SaveStringToFile(Summary, *(Output / TEXT("LastRender.txt")))) return false;
        UE_LOG(LogTemp, Display, TEXT("World PNG export complete: Seed=%llu Size=%d Sampling=%.3fs Total=%.3fs Output=%s"),
            R.World.WorldSeed, R.Size, SampleSeconds, FPlatformTime::Seconds() - Start, *Output);
        return true;
    }
}

UExportWorldMapsCommandlet::UExportWorldMapsCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UExportWorldMapsCommandlet::Main(const FString& Params)
{
    FString Input, Output, StopFile;
    const bool bWatch = FParse::Param(*Params, TEXT("Watch"));
    int32 MaxExports = 0;
    FParse::Value(*Params, TEXT("MaxExports="), MaxExports);
    FParse::Value(*Params, TEXT("StopFile="), StopFile);
    if (!FParse::Value(*Params, TEXT("Parameters="), Input) || !FParse::Value(*Params, TEXT("Output="), Output)) return 1;
    IFileManager::Get().MakeDirectory(*Output, true);
    FString LastText, PendingText;
    bool bFirst = true;
    int32 Exports = 0;
    do
    {
        if (IsEngineExitRequested() || (!StopFile.IsEmpty() && IFileManager::Get().FileExists(*StopFile))) break;
        FString Text;
        if (FFileHelper::LoadFileToString(Text, *Input))
        {
            if ((bFirst || Text != LastText) && (!bWatch || Text == PendingText))
            {
                FRequest Request;
                if (ParseRequest(Text, Request))
                {
                    if (!Render(Request, Text, Output, FParse::Param(*Params, TEXT("Verify")))) return 1;
                    ++Exports;
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Invalid world-map parameters in %s; keeping previous PNGs. Check names, finite values, ranges and ordered biome minima."), *Input);
                    if (!bWatch) return 1;
                }
                LastText = Text; bFirst = false;
            }
            PendingText = Text;
        }
        else if (!bWatch) return 1;
        if (!bWatch || (MaxExports > 0 && Exports >= MaxExports)) break;
        FPlatformProcess::Sleep(.25f);
    } while (true);
    return Exports > 0 ? 0 : 1;
}
