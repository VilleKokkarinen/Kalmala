#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "KalmalaGameMode.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKalmalaBiomeBaselineArgumentTest,
    "Kalmala.Gameplay.World.BiomeBaselineCaptureArguments",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKalmalaBiomeBaselineArgumentTest::RunTest(const FString& Parameters)
{
    const auto MakeCommandLine = [](const FString& X, const FString& Y, const FString& ViewZ,
        const FString& TemporalFrames, const FString& TemporalInterval, const FString& GpuFrames)
    {
        return FString::Printf(TEXT("-KalmalaBiomeBaselineXcm=%s -KalmalaBiomeBaselineYcm=%s "
            "-KalmalaBiomeBaselineViewZcm=%s -KalmalaBiomeBaselineTemporalFrames=%s "
            "-KalmalaBiomeBaselineTemporalIntervalSeconds=%s -KalmalaBiomeBaselineGpuFrames=%s -csvGpuStats"),
            *X, *Y, *ViewZ, *TemporalFrames, *TemporalInterval, *GpuFrames);
    };
    const auto Parse = [](const FString& CommandLine, float& X, float& Y, float& ViewZ,
        int32& TemporalFrames, float& TemporalInterval, int32& GpuFrames)
    {
        return AKalmalaGameMode::TryParseBiomeBaselineNumericArguments(*CommandLine,
            X, Y, ViewZ, TemporalFrames, TemporalInterval, GpuFrames);
    };

    float X = 0.0f;
    float Y = 0.0f;
    float ViewZ = 0.0f;
    int32 TemporalFrames = 0;
    float TemporalInterval = 0.0f;
    int32 GpuFrames = -1;

    TestTrue(TEXT("Signed and scientific coordinates with supported options are accepted"),
        Parse(MakeCommandLine(TEXT("-1.25e3"), TEXT("+2.5E2"), TEXT("-.5e+2"),
            TEXT("8"), TEXT("0.5"), TEXT("120")), X, Y, ViewZ, TemporalFrames, TemporalInterval, GpuFrames));
    TestTrue(TEXT("X coordinate retains signed scientific value"), FMath::IsNearlyEqual(X, -1250.0f));
    TestTrue(TEXT("Y coordinate retains signed scientific value"), FMath::IsNearlyEqual(Y, 250.0f));
    TestTrue(TEXT("View height retains signed scientific value"), FMath::IsNearlyEqual(ViewZ, -50.0f));
    TestEqual(TEXT("Temporal frame setting is preserved"), TemporalFrames, 8);
    TestTrue(TEXT("Temporal interval is preserved"), FMath::IsNearlyEqual(TemporalInterval, 0.5f));
    TestEqual(TEXT("GPU frame setting is preserved"), GpuFrames, 120);

    TestTrue(TEXT("Lowest supported frame settings are accepted"),
        Parse(MakeCommandLine(TEXT("1e2"), TEXT("-2E2"), TEXT("3.0e2"),
            TEXT("1"), TEXT("0.1"), TEXT("0")), X, Y, ViewZ, TemporalFrames, TemporalInterval, GpuFrames));
    TestEqual(TEXT("Minimum temporal frame boundary is preserved"), TemporalFrames, 1);
    TestTrue(TEXT("Minimum interval boundary is preserved"), FMath::IsNearlyEqual(TemporalInterval, 0.1f));
    TestEqual(TEXT("Minimum GPU frame boundary is preserved"), GpuFrames, 0);
    TestTrue(TEXT("Maximum frame settings are accepted"),
        Parse(MakeCommandLine(TEXT("1"), TEXT("2"), TEXT("3"), TEXT("12"), TEXT("2.0"), TEXT("1200")),
            X, Y, ViewZ, TemporalFrames, TemporalInterval, GpuFrames));
    TestEqual(TEXT("Maximum temporal frame boundary is preserved"), TemporalFrames, 12);
    TestTrue(TEXT("Maximum interval boundary is preserved"), FMath::IsNearlyEqual(TemporalInterval, 2.0f));
    TestEqual(TEXT("Maximum GPU frame boundary is preserved"), GpuFrames, 1200);

    const FString DefaultsCommandLine = TEXT("-KalmalaBiomeBaselineXcm=-1 -KalmalaBiomeBaselineYcm=2 -KalmalaBiomeBaselineViewZcm=3");
    TestTrue(TEXT("Omitted optional settings retain their defaults"),
        Parse(DefaultsCommandLine, X, Y, ViewZ, TemporalFrames, TemporalInterval, GpuFrames));
    TestEqual(TEXT("Default temporal frame count"), TemporalFrames, 1);
    TestTrue(TEXT("Default temporal interval"), FMath::IsNearlyEqual(TemporalInterval, 0.5f));
    TestEqual(TEXT("Default GPU frame count"), GpuFrames, 0);
    const FString MissingGpuValueCommandLine = TEXT("-KalmalaBiomeBaselineXcm=1 -KalmalaBiomeBaselineYcm=2 "
        "-KalmalaBiomeBaselineViewZcm=3 -KalmalaBiomeBaselineGpuFrames");
    TestFalse(TEXT("An explicit GPU option without a numeric value is rejected"),
        Parse(MissingGpuValueCommandLine, X, Y, ViewZ, TemporalFrames, TemporalInterval, GpuFrames));

    struct FInvalidTokenCase
    {
        const TCHAR* Argument;
        const TCHAR* Value;
    };
    const FInvalidTokenCase InvalidCases[] = {
        { TEXT("Xcm"), TEXT("north") }, { TEXT("Xcm"), TEXT("100junk") },
        { TEXT("Ycm"), TEXT("north") }, { TEXT("Ycm"), TEXT("100junk") },
        { TEXT("ViewZcm"), TEXT("north") }, { TEXT("ViewZcm"), TEXT("100junk") },
        { TEXT("TemporalFrames"), TEXT("many") }, { TEXT("TemporalFrames"), TEXT("2junk") },
        { TEXT("TemporalIntervalSeconds"), TEXT("often") }, { TEXT("TemporalIntervalSeconds"), TEXT("0.5junk") },
        { TEXT("GpuFrames"), TEXT("many") }, { TEXT("GpuFrames"), TEXT("60junk") },
    };
    for (const FInvalidTokenCase& InvalidCase : InvalidCases)
    {
        FString XToken = TEXT("10");
        FString YToken = TEXT("20");
        FString ViewZToken = TEXT("300");
        FString TemporalFramesToken = TEXT("1");
        FString TemporalIntervalToken = TEXT("0.5");
        FString GpuFramesToken = TEXT("60");
        const FString Argument = InvalidCase.Argument;
        if (Argument == TEXT("Xcm")) XToken = InvalidCase.Value;
        else if (Argument == TEXT("Ycm")) YToken = InvalidCase.Value;
        else if (Argument == TEXT("ViewZcm")) ViewZToken = InvalidCase.Value;
        else if (Argument == TEXT("TemporalFrames")) TemporalFramesToken = InvalidCase.Value;
        else if (Argument == TEXT("TemporalIntervalSeconds")) TemporalIntervalToken = InvalidCase.Value;
        else if (Argument == TEXT("GpuFrames")) GpuFramesToken = InvalidCase.Value;

        X = 111.0f;
        Y = 222.0f;
        ViewZ = 333.0f;
        TemporalFrames = 4;
        TemporalInterval = 0.75f;
        GpuFrames = 30;
        const FString Label = FString::Printf(TEXT("Reject malformed %s token '%s'"), *Argument, InvalidCase.Value);
        TestFalse(Label, Parse(MakeCommandLine(XToken, YToken, ViewZToken,
            TemporalFramesToken, TemporalIntervalToken, GpuFramesToken), X, Y, ViewZ,
            TemporalFrames, TemporalInterval, GpuFrames));
        TestEqual(Label + TEXT(" without partially changing X"), X, 111.0f);
        TestEqual(Label + TEXT(" without partially changing GPU frames"), GpuFrames, 30);
    }

    const FInvalidTokenCase OutOfRangeCases[] = {
        { TEXT("TemporalFrames"), TEXT("0") }, { TEXT("TemporalFrames"), TEXT("13") },
        { TEXT("TemporalIntervalSeconds"), TEXT("0.09") }, { TEXT("TemporalIntervalSeconds"), TEXT("2.01") },
        { TEXT("GpuFrames"), TEXT("-1") }, { TEXT("GpuFrames"), TEXT("1201") },
    };
    for (const FInvalidTokenCase& InvalidCase : OutOfRangeCases)
    {
        FString TemporalFramesToken = TEXT("1");
        FString TemporalIntervalToken = TEXT("0.5");
        FString GpuFramesToken = TEXT("0");
        const FString Argument = InvalidCase.Argument;
        if (Argument == TEXT("TemporalFrames")) TemporalFramesToken = InvalidCase.Value;
        else if (Argument == TEXT("TemporalIntervalSeconds")) TemporalIntervalToken = InvalidCase.Value;
        else GpuFramesToken = InvalidCase.Value;

        TestFalse(FString::Printf(TEXT("Reject out-of-range %s token '%s'"), *Argument, InvalidCase.Value),
            Parse(MakeCommandLine(TEXT("10"), TEXT("20"), TEXT("300"), TemporalFramesToken,
                TemporalIntervalToken, GpuFramesToken), X, Y, ViewZ, TemporalFrames, TemporalInterval, GpuFrames));
    }

    return true;
}

#endif
