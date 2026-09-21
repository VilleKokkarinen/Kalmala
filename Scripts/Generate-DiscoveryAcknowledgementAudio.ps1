[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$cuePath = Join-Path $outputDirectory 'DiscoveryAcknowledgedCue.wav'
if ((Test-Path -LiteralPath $cuePath) -and -not $Force) {
    throw "Refusing to replace an existing audio source without -Force: $cuePath"
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class KalmalaDiscoveryAcknowledgementWaveGenerator
{
    private const int SampleRate = 22050;
    private const int Frames = SampleRate / 2;

    public static void WriteCue(string path)
    {
        double[] frequencies = { 392.00, 493.88, 587.33 };
        double[] starts = { 0.00, 0.105, 0.210 };
        var samples = new double[Frames];
        double peak = 0.0;
        for (int frame = 0; frame < Frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            double value = 0.0;
            for (int note = 0; note < frequencies.Length; ++note)
            {
                double age = time - starts[note];
                if (age < 0.0 || age > 0.34) continue;
                double envelope = (1.0 - Math.Exp(-age * 95.0)) * Math.Exp(-age * 8.0);
                double phase = 2.0 * Math.PI * frequencies[note] * age;
                value += envelope * (Math.Sin(phase) + 0.10 * Math.Sin(phase * 2.0 + 0.15));
            }
            samples[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }
        WriteWave(path, samples, peak > 0.0 ? 0.17 / peak : 0.0);
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
                double fadeOut = Math.Min(1.0, (samples.Length - 1 - frame) / (SampleRate * 0.04));
                double clamped = Math.Max(-1.0, Math.Min(1.0, samples[frame] * gain * fadeIn * fadeOut));
                writer.Write((short)Math.Round(clamped * 32767.0));
            }
        }
    }
}
'@

[KalmalaDiscoveryAcknowledgementWaveGenerator]::WriteCue($cuePath)
Write-Output "Generated original 0.5-second mono discovery acknowledgement cue: $cuePath"
