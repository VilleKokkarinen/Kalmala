[CmdletBinding()]
param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$outputPath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\FireBed.wav'
$outputDirectory = Split-Path $outputPath
if ((Test-Path -LiteralPath $outputPath) -and -not $Force) {
    throw "Refusing to replace the existing audio source without -Force: $outputPath"
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Text;

public static class KalmalaFireWaveGenerator
{
    public static void Write(string path)
    {
        const int sampleRate = 22050;
        const int sourceSeconds = 9;
        const int crossfadeSeconds = 1;
        const int sourceFrames = sampleRate * sourceSeconds;
        const int crossfadeFrames = sampleRate * crossfadeSeconds;
        const int outputFrames = sourceFrames - crossfadeFrames;

        var random = new Random(927451);
        var source = new double[sourceFrames];
        double slow = 0.0;
        double mid = 0.0;
        double fast = 0.0;
        double crackle = 0.0;
        double peak = 0.0;
        for (int frame = 0; frame < sourceFrames; ++frame)
        {
            double white = random.NextDouble() * 2.0 - 1.0;
            slow += 0.008 * (white - slow);
            mid += 0.075 * (white - mid);
            fast += 0.38 * (white - fast);
            if (random.NextDouble() < 0.00007)
            {
                crackle += (0.25 + random.NextDouble() * 0.55) * (random.NextDouble() < 0.5 ? -1.0 : 1.0);
            }
            crackle *= 0.9972;
            double time = (double)frame / sampleRate;
            double warmth = 0.84 + 0.11 * Math.Sin(2.0 * Math.PI * 0.17 * time + 0.8)
                + 0.05 * Math.Sin(2.0 * Math.PI * 0.29 * time + 1.4);
            double value = (0.62 * slow + 0.52 * (mid - slow) + 0.055 * (white - fast) + 0.14 * crackle) * warmth;
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

        // Give the loop boundary a shared, low-energy half-second on each side.
        // This makes the first and final PCM windows agree without a hard cut.
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

[KalmalaFireWaveGenerator]::Write($outputPath)
Write-Output "Generated original, loop-seamed fire ambience: $outputPath"
