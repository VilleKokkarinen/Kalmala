[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputPath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\WaterBed.wav'
$outputDirectory = Split-Path $outputPath
if ((Test-Path -LiteralPath $outputPath) -and -not $Force) {
    throw "Refusing to replace the existing audio source without -Force: $outputPath"
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Text;

public static class KalmalaWaterWaveGenerator
{
    public static void Write(string path)
    {
        const int sampleRate = 22050;
        const int sourceSeconds = 9;
        const int crossfadeSeconds = 1;
        const int sourceFrames = sampleRate * sourceSeconds;
        const int crossfadeFrames = sampleRate * crossfadeSeconds;
        const int outputFrames = sourceFrames - crossfadeFrames;

        var random = new Random(418773);
        var source = new double[sourceFrames];
        double slow = 0.0;
        double mid = 0.0;
        double fast = 0.0;
        double peak = 0.0;
        for (int frame = 0; frame < sourceFrames; ++frame)
        {
            double white = random.NextDouble() * 2.0 - 1.0;
            slow += 0.012 * (white - slow);
            mid += 0.065 * (white - mid);
            fast += 0.34 * (white - fast);
            double time = (double)frame / sampleRate;
            double swell = 0.72
                + 0.16 * Math.Sin(2.0 * Math.PI * 0.19 * time + 0.3)
                + 0.09 * Math.Sin(2.0 * Math.PI * 0.31 * time + 1.1);
            double ripple = 0.88 + 0.12 * Math.Sin(2.0 * Math.PI * 1.17 * time + 0.6);
            double value = (1.05 * slow + 0.82 * (mid - slow) + 0.22 * (white - fast)) * swell * ripple;
            source[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }

        const int bytesPerSample = 2;
        int dataBytes = outputFrames * bytesPerSample;
        double gain = peak > 0.0 ? 0.42 / peak : 0.0;
        using (var stream = new FileStream(path, FileMode.Create, FileAccess.Write, FileShare.None))
        using (var writer = new BinaryWriter(stream))
        {
            writer.Write(Encoding.ASCII.GetBytes("RIFF"));
            writer.Write(36 + dataBytes);
            writer.Write(Encoding.ASCII.GetBytes("WAVE"));
            writer.Write(Encoding.ASCII.GetBytes("fmt "));
            writer.Write(16);
            writer.Write((short)1);
            writer.Write((short)1);
            writer.Write(sampleRate);
            writer.Write(sampleRate * bytesPerSample);
            writer.Write((short)bytesPerSample);
            writer.Write((short)16);
            writer.Write(Encoding.ASCII.GetBytes("data"));
            writer.Write(dataBytes);

            int straightFrames = outputFrames - crossfadeFrames;
            for (int frame = 0; frame < outputFrames; ++frame)
            {
                double sample;
                if (frame < straightFrames)
                {
                    sample = source[frame + crossfadeFrames];
                }
                else
                {
                    int blendFrame = frame - straightFrames;
                    double alpha = (double)blendFrame / (crossfadeFrames - 1);
                    double tail = source[sourceFrames - crossfadeFrames + blendFrame];
                    double head = source[blendFrame];
                    sample = tail * (1.0 - alpha) + head * alpha;
                }

                short pcm = (short)Math.Round(Math.Max(-1.0, Math.Min(1.0, sample * gain)) * short.MaxValue);
                writer.Write(pcm);
            }
        }
    }
}
'@

[KalmalaWaterWaveGenerator]::Write($outputPath)
Write-Output "Generated original, loop-seamed water ambience: $outputPath"
