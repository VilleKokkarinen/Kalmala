[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$acceptedPath = Join-Path $outputDirectory 'InteractionAcceptedCue.wav'
$rejectedPath = Join-Path $outputDirectory 'InteractionRejectedCue.wav'
foreach ($path in @($acceptedPath, $rejectedPath)) {
    if ((Test-Path -LiteralPath $path) -and -not $Force) {
        throw "Refusing to replace an existing audio source without -Force: $path"
    }
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class KalmalaInteractionFeedbackWaveGenerator
{
    private const int SampleRate = 22050;
    private const int Frames = SampleRate * 2 / 5;

    public static void WriteAccepted(string path)
    {
        var samples = new double[Frames];
        double peak = 0.0;
        uint noiseState = 0xa341316c;
        for (int frame = 0; frame < Frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            noiseState ^= noiseState << 13;
            noiseState ^= noiseState >> 17;
            noiseState ^= noiseState << 5;
            double noise = ((noiseState & 0xffff) / 32767.5) - 1.0;
            double value = 0.0;
            AddWoodNote(ref value, time, 0.000, 196.00);
            AddWoodNote(ref value, time, 0.085, 246.94);
            double click = noise * Math.Exp(-time * 180.0) * 0.10;
            samples[frame] = value + click;
            peak = Math.Max(peak, Math.Abs(samples[frame]));
        }
        WriteWave(path, samples, peak > 0.0 ? 0.22 / peak : 0.0);
    }

    public static void WriteRejected(string path)
    {
        var samples = new double[Frames];
        double peak = 0.0;
        uint noiseState = 0xc8013ea4;
        for (int frame = 0; frame < Frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            noiseState ^= noiseState << 13;
            noiseState ^= noiseState >> 17;
            noiseState ^= noiseState << 5;
            double noise = ((noiseState & 0xffff) / 32767.5) - 1.0;
            double value = 0.0;
            AddMutedNote(ref value, time, 0.000, 174.61);
            AddMutedNote(ref value, time, 0.105, 146.83);
            double click = noise * Math.Exp(-time * 220.0) * 0.07;
            samples[frame] = value + click;
            peak = Math.Max(peak, Math.Abs(samples[frame]));
        }
        WriteWave(path, samples, peak > 0.0 ? 0.16 / peak : 0.0);
    }

    private static void AddWoodNote(ref double value, double time, double start, double frequency)
    {
        double age = time - start;
        if (age < 0.0 || age > 0.22) return;
        double envelope = (1.0 - Math.Exp(-age * 240.0)) * Math.Exp(-age * 22.0);
        double phase = 2.0 * Math.PI * frequency * age;
        value += envelope * (Math.Sin(phase) + 0.28 * Math.Sin(phase * 2.71 + 0.3));
    }

    private static void AddMutedNote(ref double value, double time, double start, double frequency)
    {
        double age = time - start;
        if (age < 0.0 || age > 0.18) return;
        double envelope = (1.0 - Math.Exp(-age * 180.0)) * Math.Exp(-age * 28.0);
        double phase = 2.0 * Math.PI * frequency * age;
        value += envelope * (Math.Sin(phase) + 0.18 * Math.Sin(phase * 1.53 + 0.1));
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
            for (int frame = 0; frame < samples.Length; ++frame)
            {
                double fadeIn = Math.Min(1.0, frame / (SampleRate * 0.01));
                double fadeOut = Math.Min(1.0, (samples.Length - 1 - frame) / (SampleRate * 0.02));
                double clamped = Math.Max(-1.0, Math.Min(1.0, samples[frame] * gain * fadeIn * fadeOut));
                writer.Write((short)Math.Round(clamped * 32767.0));
            }
        }
    }
}
'@

[KalmalaInteractionFeedbackWaveGenerator]::WriteAccepted($acceptedPath)
[KalmalaInteractionFeedbackWaveGenerator]::WriteRejected($rejectedPath)
Write-Output "Generated original 0.4-second mono interaction result cues: $acceptedPath and $rejectedPath"
