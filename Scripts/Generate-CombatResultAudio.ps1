[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$cuePath = Join-Path $outputDirectory 'CombatResultCue.wav'
if ((Test-Path -LiteralPath $cuePath) -and -not $Force) {
    throw "Refusing to replace an existing audio source without -Force: $cuePath"
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class KalmalaCombatResultWaveGenerator
{
    private const int SampleRate = 22050;

    public static void WriteCue(string path)
    {
        const int frames = SampleRate * 2 / 5;
        var samples = new double[frames];
        double peak = 0.0;
        uint noiseState = 0x6d2b79f5;
        for (int frame = 0; frame < frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            noiseState ^= noiseState << 13;
            noiseState ^= noiseState >> 17;
            noiseState ^= noiseState << 5;
            double noise = ((noiseState & 0xffff) / 32767.5) - 1.0;
            double bodyEnvelope = Math.Exp(-time * 17.0);
            double body = bodyEnvelope * (Math.Sin(2.0 * Math.PI * 146.8 * time)
                + 0.22 * Math.Sin(2.0 * Math.PI * 293.7 * time + 0.12));
            double ringAge = time - 0.035;
            double ring = ringAge >= 0.0 && ringAge <= 0.24
                ? Math.Exp(-ringAge * 19.0) * Math.Sin(2.0 * Math.PI * 880.0 * ringAge)
                : 0.0;
            double transient = noise * Math.Exp(-time * 95.0);
            samples[frame] = body * 0.62 + ring * 0.19 + transient * 0.035;
            peak = Math.Max(peak, Math.Abs(samples[frame]));
        }
        WriteWave(path, samples, peak > 0.0 ? 0.24 / peak : 0.0);
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

[KalmalaCombatResultWaveGenerator]::WriteCue($cuePath)
Write-Output "Generated original 0.4-second mono CombatResultCue: $cuePath"
