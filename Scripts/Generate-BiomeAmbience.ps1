[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputPath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\BiomeBed.wav'
$outputDirectory = Split-Path $outputPath
if ((Test-Path -LiteralPath $outputPath) -and -not $Force) {
    throw "Refusing to replace the existing audio source without -Force: $outputPath"
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Text;

public static class KalmalaBiomeWaveGenerator
{
    public static void Write(string path)
    {
        const int sampleRate = 22050;
        const int sourceSeconds = 9;
        const int crossfadeSeconds = 1;
        const int sourceFrames = sampleRate * sourceSeconds;
        const int crossfadeFrames = sampleRate * crossfadeSeconds;
        const int outputFrames = sourceFrames - crossfadeFrames;

        var random = new Random(684217);
        var source = new double[sourceFrames];
        double slow = 0.0;
        double middle = 0.0;
        double quick = 0.0;
        double peak = 0.0;
        for (int frame = 0; frame < sourceFrames; ++frame)
        {
            double white = random.NextDouble() * 2.0 - 1.0;
            slow += 0.009 * (white - slow);
            middle += 0.065 * (white - middle);
            quick += 0.30 * (white - quick);
            double time = (double)frame / sampleRate;
            double swell = 0.78
                + 0.12 * Math.Sin(2.0 * Math.PI * 0.13 * time + 0.2)
                + 0.08 * Math.Sin(2.0 * Math.PI * 0.27 * time + 1.0);
            double air = 0.76 * slow + 0.50 * (middle - slow) + 0.12 * (white - quick);
            double value = air * swell;
            source[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }

        const int bytesPerSample = 2;
        int dataBytes = outputFrames * bytesPerSample;
        double gain = peak > 0.0 ? 0.34 / peak : 0.0;
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

        const int seamFrames = sampleRate / 2;
        double seamLevel = 0.0;
        for (int frame = 0; frame < seamFrames; ++frame)
        {
            seamLevel += loop[frame] + loop[outputFrames - 1 - frame];
        }
        seamLevel /= 2.0 * seamFrames;
        for (int frame = 0; frame < seamFrames; ++frame)
        {
            double alpha = (double)frame / (seamFrames - 1);
            double fade = alpha * alpha * (3.0 - 2.0 * alpha);
            double sharedEdge = seamLevel * (1.0 - fade);
            loop[frame] = loop[frame] * fade + sharedEdge;
            int tailFrame = outputFrames - 1 - frame;
            loop[tailFrame] = loop[tailFrame] * fade + sharedEdge;
        }

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

            for (int frame = 0; frame < outputFrames; ++frame)
            {
                short pcm = (short)Math.Round(Math.Max(-1.0, Math.Min(1.0, loop[frame] * gain)) * short.MaxValue);
                writer.Write(pcm);
            }
        }
    }
}
'@

[KalmalaBiomeWaveGenerator]::Write($outputPath)
Write-Output "Generated original, loop-seamed biome ambience: $outputPath"
