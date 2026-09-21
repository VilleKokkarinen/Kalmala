[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$cuePath = Join-Path $outputDirectory 'SupportAcceptedCue.wav'
if ((Test-Path -LiteralPath $cuePath) -and -not $Force) {
    throw "Refusing to replace an existing audio source without -Force: $cuePath"
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class KalmalaSupportFeedbackWaveGenerator
{
    private const int SampleRate = 22050;

    public static void WriteAcceptedCue(string path)
    {
        const int frames = SampleRate * 4 / 5;
        double[] frequencies = { 523.25, 659.25, 783.99 };
        double[] starts = { 0.00, 0.16, 0.32 };
        var samples = new double[frames];
        double peak = 0.0;
        for (int frame = 0; frame < frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            double value = 0.0;
            for (int note = 0; note < frequencies.Length; ++note)
            {
                double age = time - starts[note];
                if (age < 0.0 || age > 0.48) continue;
                double envelope = (1.0 - Math.Exp(-age * 60.0)) * Math.Exp(-age * 8.5);
                double phase = 2.0 * Math.PI * frequencies[note] * age;
                value += envelope * (Math.Sin(phase) + 0.16 * Math.Sin(phase * 2.0 + 0.2));
            }
            samples[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }
        WriteWave(path, samples, peak > 0.0 ? 0.20 / peak : 0.0);
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

[KalmalaSupportFeedbackWaveGenerator]::WriteAcceptedCue($cuePath)
Write-Output "Generated original 0.8-second mono SupportAcceptedCue: $cuePath"
