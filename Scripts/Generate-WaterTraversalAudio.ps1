[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$cueDefinitions = @(
    @{ Name = 'GeneratedOceanEntryCue'; Kind = 'Entry'; Duration = 0.24 },
    @{ Name = 'GeneratedOceanExitCue'; Kind = 'Exit'; Duration = 0.26 }
)

foreach ($cue in $cueDefinitions) {
    $cue.Path = Join-Path $outputDirectory ($cue.Name + '.wav')
    if ((Test-Path -LiteralPath $cue.Path) -and -not $Force) {
        throw "Refusing to replace an existing audio source without -Force: $($cue.Path)"
    }
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class KalmalaWaterTraversalWaveGenerator
{
    private const int SampleRate = 22050;

    public static void WriteCue(string path, string kind, double duration)
    {
        int frames = (int)(SampleRate * duration);
        var samples = new double[frames];
        double peak = 0.0;
        uint randomState = kind == "Entry" ? 0x57415445u : 0x45584954u;
        for (int frame = 0; frame < frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            double noise = NextNoise(ref randomState);
            double value;
            if (kind == "Entry")
            {
                double lowSweep = Math.Sin(2.0 * Math.PI * (118.0 * time - 34.0 * time * time / duration));
                double brightRipple = Math.Sin(2.0 * Math.PI * (410.0 * time + 190.0 * time * time / duration));
                double envelope = (1.0 - Math.Exp(-time * 170.0)) * Math.Exp(-time * 14.0);
                value = envelope * (0.48 * lowSweep + 0.18 * brightRipple + 0.25 * noise * Math.Exp(-time * 4.0));
            }
            else
            {
                double highSweep = Math.Sin(2.0 * Math.PI * (760.0 * time - 330.0 * time * time / duration));
                double softBody = Math.Sin(2.0 * Math.PI * (206.0 * time + 42.0 * time * time / duration));
                double envelope = (1.0 - Math.Exp(-time * 95.0)) * Math.Exp(-time * 12.0);
                value = envelope * (0.28 * highSweep + 0.36 * softBody + 0.16 * noise * Math.Exp(-time * 5.0));
            }
            samples[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }

        WriteWave(path, samples, peak > 0.0 ? 0.24 / peak : 0.0);
    }

    private static double NextNoise(ref uint state)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return ((state & 0xffff) / 32767.5) - 1.0;
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
                double fadeIn = Math.Min(1.0, frame / (SampleRate * 0.006));
                double fadeOut = Math.Min(1.0, (samples.Length - 1 - frame) / (SampleRate * 0.045));
                double clamped = Math.Max(-1.0, Math.Min(1.0, samples[frame] * gain * fadeIn * fadeOut));
                writer.Write((short)Math.Round(clamped * 32767.0));
            }
        }
    }
}
'@

foreach ($cue in $cueDefinitions) {
    [KalmalaWaterTraversalWaveGenerator]::WriteCue($cue.Path, $cue.Kind, [double]$cue.Duration)
    Write-Output "Generated original $($cue.Duration)-second mono water traversal cue: $($cue.Path)"
}
