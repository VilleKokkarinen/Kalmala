[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$cueDefinitions = @(
    @{ Name = 'SupportMendingCue'; Notes = @(523.25, 659.25, 783.99); Starts = @(0.0, 0.09, 0.18); Length = 0.24 },
    @{ Name = 'SupportHearthShieldCue'; Notes = @(349.23, 466.16); Starts = @(0.0, 0.12); Length = 0.30 },
    @{ Name = 'SupportBearsVigorCue'; Notes = @(293.66, 440.00, 587.33); Starts = @(0.0, 0.10, 0.20); Length = 0.25 },
    @{ Name = 'SupportDeerCallCue'; Notes = @(659.25, 880.00); Starts = @(0.0, 0.13); Length = 0.31 },
    @{ Name = 'SupportHearthShieldExpiryCue'; Notes = @(466.16, 349.23); Starts = @(0.0, 0.13); Length = 0.27 },
    @{ Name = 'SupportBearsVigorExpiryCue'; Notes = @(587.33, 440.00); Starts = @(0.0, 0.13); Length = 0.27 }
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

public static class KalmalaSupportEffectWaveGenerator
{
    private const int SampleRate = 22050;

    public static void WriteCue(string path, double[] frequencies, double[] starts, double noteLength)
    {
        const double duration = 0.42;
        int frames = (int)(SampleRate * duration);
        var samples = new double[frames];
        double peak = 0.0;
        for (int frame = 0; frame < frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            double value = 0.0;
            for (int note = 0; note < frequencies.Length; ++note)
            {
                double age = time - starts[note];
                if (age < 0.0 || age > noteLength) continue;
                double envelope = (1.0 - Math.Exp(-age * 75.0)) * Math.Exp(-age * 9.0);
                double phase = 2.0 * Math.PI * frequencies[note] * age;
                value += envelope * (Math.Sin(phase) + 0.08 * Math.Sin(phase * 2.0 + 0.12));
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

foreach ($cue in $cueDefinitions) {
    [KalmalaSupportEffectWaveGenerator]::WriteCue(
        $cue.Path,
        [double[]]$cue.Notes,
        [double[]]$cue.Starts,
        [double]$cue.Length)
    Write-Output "Generated original 0.42-second mono support cue: $($cue.Path)"
}
