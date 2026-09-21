[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$rainPath = Join-Path $outputDirectory 'RainBed.wav'
$wetPath = Join-Path $outputDirectory 'WetStatusCue.wav'
foreach ($path in @($rainPath, $wetPath)) {
    if ((Test-Path -LiteralPath $path) -and -not $Force) {
        throw "Refusing to replace an existing audio source without -Force: $path"
    }
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class KalmalaWeatherExposureWaveGenerator
{
    private const int SampleRate = 22050;

    public static void WriteRain(string path)
    {
        const int sourceFrames = SampleRate * 9;
        const int crossfadeFrames = SampleRate;
        const int outputFrames = sourceFrames - crossfadeFrames;
        var random = new Random(731905);
        var source = new double[sourceFrames];
        double low = 0.0;
        double mid = 0.0;
        double peak = 0.0;
        for (int frame = 0; frame < sourceFrames; ++frame)
        {
            double white = random.NextDouble() * 2.0 - 1.0;
            low += 0.018 * (white - low);
            mid += 0.14 * (white - mid);
            double time = (double)frame / SampleRate;
            double swell = 0.88 + 0.08 * Math.Sin(2.0 * Math.PI * 0.19 * time)
                + 0.04 * Math.Sin(2.0 * Math.PI * 0.41 * time + 0.7);
            double value = (0.63 * (white - mid) + 0.22 * (mid - low) + 0.15 * (white - low)) * swell;
            source[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }

        var loop = new double[outputFrames];
        int straightFrames = outputFrames - crossfadeFrames;
        for (int frame = 0; frame < outputFrames; ++frame)
        {
            if (frame < straightFrames)
            {
                loop[frame] = source[frame + crossfadeFrames];
            }
            else
            {
                int blendFrame = frame - straightFrames;
                double alpha = (double)blendFrame / (crossfadeFrames - 1);
                double tail = source[sourceFrames - crossfadeFrames + blendFrame];
                double head = source[blendFrame];
                loop[frame] = tail * (1.0 - alpha) + head * alpha;
            }
        }

        double gain = peak > 0.0 ? 0.28 / peak : 0.0;
        WriteWave(path, loop, gain);
    }

    public static void WriteWetStatusCue(string path)
    {
        const int frames = SampleRate * 4 / 5;
        var samples = new double[frames];
        double peak = 0.0;
        for (int frame = 0; frame < frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            double progress = time / 0.8;
            double envelope = (1.0 - Math.Exp(-time * 180.0)) * Math.Exp(-time * 5.2);
            double fundamental = 360.0 - 145.0 * Math.Min(1.0, progress);
            double phase = 2.0 * Math.PI * (fundamental * time + 35.0 * time * time);
            double tone = Math.Sin(phase) + 0.24 * Math.Sin(2.13 * phase + 0.4);
            double tick = Math.Sin(2.0 * Math.PI * 1120.0 * time) * Math.Exp(-time * 30.0);
            double value = envelope * (0.72 * tone + 0.18 * tick);
            samples[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }
        double gain = peak > 0.0 ? 0.22 / peak : 0.0;
        WriteWave(path, samples, gain);
    }

    private static void WriteWave(string path, double[] samples, double gain)
    {
        const short channels = 1;
        const short bitsPerSample = 16;
        int dataBytes = samples.Length * channels * bitsPerSample / 8;
        using (var stream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
        using (var writer = new BinaryWriter(stream))
        {
            writer.Write(new char[] { 'R', 'I', 'F', 'F' });
            writer.Write(36 + dataBytes);
            writer.Write(new char[] { 'W', 'A', 'V', 'E' });
            writer.Write(new char[] { 'f', 'm', 't', ' ' });
            writer.Write(16);
            writer.Write((short)1);
            writer.Write(channels);
            writer.Write(SampleRate);
            writer.Write(SampleRate * channels * bitsPerSample / 8);
            writer.Write((short)(channels * bitsPerSample / 8));
            writer.Write(bitsPerSample);
            writer.Write(new char[] { 'd', 'a', 't', 'a' });
            writer.Write(dataBytes);
            foreach (double sample in samples)
            {
                double clamped = Math.Max(-1.0, Math.Min(1.0, sample * gain));
                writer.Write((short)Math.Round(clamped * 32767.0));
            }
        }
    }
}
'@

[KalmalaWeatherExposureWaveGenerator]::WriteRain($rainPath)
[KalmalaWeatherExposureWaveGenerator]::WriteWetStatusCue($wetPath)
Write-Output "Generated original 8-second mono RainBed: $rainPath"
Write-Output "Generated original 0.8-second mono WetStatusCue: $wetPath"
