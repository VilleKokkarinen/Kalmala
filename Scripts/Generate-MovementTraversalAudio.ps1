[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$cueDefinitions = @(
    @{ Name = 'MovementFootfallCue'; Kind = 'Footfall'; Duration = 0.12 },
    @{ Name = 'MovementJumpCue'; Kind = 'Jump'; Duration = 0.18 },
    @{ Name = 'MovementLandingCue'; Kind = 'Landing'; Duration = 0.16 }
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

public static class KalmalaMovementWaveGenerator
{
    private const int SampleRate = 22050;

    public static void WriteCue(string path, string kind, double duration)
    {
        int frames = (int)(SampleRate * duration);
        var samples = new double[frames];
        double peak = 0.0;
        uint randomState = 0x4b414c4d;
        for (int frame = 0; frame < frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            double noise = NextNoise(ref randomState);
            double value;
            if (kind == "Footfall")
            {
                double body = Math.Sin(2.0 * Math.PI * 78.0 * time) + 0.24 * Math.Sin(2.0 * Math.PI * 142.0 * time);
                value = (0.72 * body + 0.16 * noise) * Math.Exp(-time * 32.0);
            }
            else if (kind == "Jump")
            {
                double phase = 2.0 * Math.PI * (300.0 * time + 650.0 * time * time / duration);
                double envelope = (1.0 - Math.Exp(-time * 75.0)) * Math.Exp(-time * 17.0);
                value = envelope * (0.76 * Math.Sin(phase) + 0.10 * Math.Sin(phase * 2.0 + 0.2) + 0.08 * noise);
            }
            else
            {
                double first = Math.Sin(2.0 * Math.PI * 68.0 * time) * Math.Exp(-time * 31.0);
                double age = time - 0.045;
                double second = age >= 0.0 ? 0.30 * Math.Sin(2.0 * Math.PI * 93.0 * age) * Math.Exp(-age * 40.0) : 0.0;
                value = 0.68 * first + second + 0.14 * noise * Math.Exp(-time * 66.0);
            }
            samples[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }

        WriteWave(path, samples, peak > 0.0 ? 0.22 / peak : 0.0);
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
                double fadeOut = Math.Min(1.0, (samples.Length - 1 - frame) / (SampleRate * 0.035));
                double clamped = Math.Max(-1.0, Math.Min(1.0, samples[frame] * gain * fadeIn * fadeOut));
                writer.Write((short)Math.Round(clamped * 32767.0));
            }
        }
    }
}
'@

foreach ($cue in $cueDefinitions) {
    [KalmalaMovementWaveGenerator]::WriteCue($cue.Path, $cue.Kind, [double]$cue.Duration)
    Write-Output "Generated original $($cue.Duration)-second mono movement cue: $($cue.Path)"
}
