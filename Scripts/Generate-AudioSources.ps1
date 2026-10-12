[CmdletBinding()]
param(
    [ValidateSet('WildernessWind', 'WaterAmbience', 'FireAmbience', 'BiomeAmbience', 'WeatherExposureAudio', 'SupportFeedbackAudio', 'CombatResultAudio', 'InteractionFeedbackAudio', 'DiscoveryAcknowledgementAudio', 'SupportEffectAudio', 'MovementTraversalAudio', 'WaterTraversalAudio')]
    [string[]]$Generator = @(),
    [switch]$All,
    [switch]$Force,
    [switch]$ListGenerators
)

$ErrorActionPreference = 'Stop'
$scriptProjectRoot = Split-Path -Parent $PSScriptRoot
$sourceDirectory = Join-Path $scriptProjectRoot 'Content\Kalmala\Audio\Source'
$outputsByGenerator = [ordered]@{
    WildernessWind = @('WindBed.wav')
    WaterAmbience = @('WaterBed.wav')
    FireAmbience = @('FireBed.wav')
    BiomeAmbience = @('BiomeBed.wav')
    WeatherExposureAudio = @('RainBed.wav', 'WetStatusCue.wav')
    SupportFeedbackAudio = @('SupportAcceptedCue.wav')
    CombatResultAudio = @('CombatResultCue.wav')
    InteractionFeedbackAudio = @('InteractionAcceptedCue.wav', 'InteractionRejectedCue.wav')
    DiscoveryAcknowledgementAudio = @('DiscoveryAcknowledgedCue.wav')
    SupportEffectAudio = @('SupportMendingCue.wav', 'SupportHearthShieldCue.wav', 'SupportBearsVigorCue.wav', 'SupportDeerCallCue.wav', 'SupportHearthShieldExpiryCue.wav', 'SupportBearsVigorExpiryCue.wav')
    MovementTraversalAudio = @('MovementFootfallCue.wav', 'MovementJumpCue.wav', 'MovementLandingCue.wav')
    WaterTraversalAudio = @('GeneratedOceanEntryCue.wav', 'GeneratedOceanExitCue.wav')
}
function Invoke-BiomeAmbience {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-CombatResultAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-DiscoveryAcknowledgementAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-FireAmbience {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-InteractionFeedbackAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-MovementTraversalAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-SupportEffectAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-SupportFeedbackAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-WaterAmbience {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-WaterTraversalAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
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

}

function Invoke-WeatherExposureAudio {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
$outputDirectory = Join-Path $projectRoot 'Content\Kalmala\Audio\Source'
$rainPath = Join-Path $outputDirectory 'RainBed.wav'
$wetPath = Join-Path $outputDirectory 'WetStatusCue.wav'
foreach ($path in @($rainPath, $wetPath)) {
    if ((Test-Path -LiteralPath $path) -and -not $Force) {
        throw "Refusing to replace an existing audio source without -Force: $path"
    }
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;

public static class KalmalaWeatherExposureWaveGenerator
{
    private const int SampleRate = 22050;

    public static void WriteRain(string path)
    {
        const int sourceFrames = SampleRate * 9;
        const int crossfadeFrames = SampleRate;
        const int outputFrames = sourceFrames - crossfadeFrames;
        var random = new Random(731905);
        var source = new double[sourceFrames];
        double low = 0.0;
        double mid = 0.0;
        double peak = 0.0;
        for (int frame = 0; frame < sourceFrames; ++frame)
        {
            double white = random.NextDouble() * 2.0 - 1.0;
            low += 0.018 * (white - low);
            mid += 0.14 * (white - mid);
            double time = (double)frame / SampleRate;
            double swell = 0.88 + 0.08 * Math.Sin(2.0 * Math.PI * 0.19 * time)
                + 0.04 * Math.Sin(2.0 * Math.PI * 0.41 * time + 0.7);
            double value = (0.63 * (white - mid) + 0.22 * (mid - low) + 0.15 * (white - low)) * swell;
            source[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }

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

        double gain = peak > 0.0 ? 0.28 / peak : 0.0;
        WriteWave(path, loop, gain);
    }

    public static void WriteWetStatusCue(string path)
    {
        const int frames = SampleRate * 4 / 5;
        var samples = new double[frames];
        double peak = 0.0;
        for (int frame = 0; frame < frames; ++frame)
        {
            double time = (double)frame / SampleRate;
            double progress = time / 0.8;
            double envelope = (1.0 - Math.Exp(-time * 180.0)) * Math.Exp(-time * 5.2);
            double fundamental = 360.0 - 145.0 * Math.Min(1.0, progress);
            double phase = 2.0 * Math.PI * (fundamental * time + 35.0 * time * time);
            double tone = Math.Sin(phase) + 0.24 * Math.Sin(2.13 * phase + 0.4);
            double tick = Math.Sin(2.0 * Math.PI * 1120.0 * time) * Math.Exp(-time * 30.0);
            double value = envelope * (0.72 * tone + 0.18 * tick);
            samples[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }
        double gain = peak > 0.0 ? 0.22 / peak : 0.0;
        WriteWave(path, samples, gain);
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

[KalmalaWeatherExposureWaveGenerator]::WriteRain($rainPath)
[KalmalaWeatherExposureWaveGenerator]::WriteWetStatusCue($wetPath)
Write-Output "Generated original 8-second mono RainBed: $rainPath"
Write-Output "Generated original 0.8-second mono WetStatusCue: $wetPath"

}

function Invoke-WildernessWind {
    [CmdletBinding()]
    param([switch]$Force)

$ErrorActionPreference = 'Stop'
$projectRoot = $scriptProjectRoot
$outputPath = Join-Path $projectRoot 'Content\Kalmala\Audio\Source\WindBed.wav'
$outputDirectory = Split-Path $outputPath
if ((Test-Path -LiteralPath $outputPath) -and -not $Force) {
    throw "Refusing to replace the existing audio source without -Force: $outputPath"
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Text;

public static class KalmalaWindWaveGenerator
{
    public static void Write(string path)
    {
        const int sampleRate = 22050;
        const int sourceSeconds = 9;
        const int crossfadeSeconds = 1;
        const int sourceFrames = sampleRate * sourceSeconds;
        const int crossfadeFrames = sampleRate * crossfadeSeconds;
        const int outputFrames = sourceFrames - crossfadeFrames;

        var random = new Random(731904);
        var source = new double[sourceFrames];
        double low = 0.0;
        double middle = 0.0;
        double peak = 0.0;
        for (int frame = 0; frame < sourceFrames; ++frame)
        {
            double white = random.NextDouble() * 2.0 - 1.0;
            low += 0.014 * (white - low);
            middle += 0.16 * (white - middle);
            double time = (double)frame / sampleRate;
            double gust = 0.66
                + 0.19 * Math.Sin(2.0 * Math.PI * 0.17 * time)
                + 0.13 * Math.Sin(2.0 * Math.PI * 0.071 * time + 1.4);
            double value = (1.35 * low + 0.72 * (middle - low)) * gust;
            source[frame] = value;
            peak = Math.Max(peak, Math.Abs(value));
        }

        const int bytesPerSample = 2;
        int dataBytes = outputFrames * bytesPerSample;
        double gain = peak > 0.0 ? 0.48 / peak : 0.0;
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

[KalmalaWindWaveGenerator]::Write($outputPath)
Write-Output "Generated original, loop-seamed wind ambience: $outputPath"

}

if ($ListGenerators) {
    foreach ($name in $outputsByGenerator.Keys) {
        [pscustomobject]@{ Generator = $name; Outputs = $outputsByGenerator[$name] -join ', ' }
    }
    return
}
if ($All -and $Generator.Count -gt 0) { throw 'Choose either -All or -Generator, not both.' }
if (-not $All -and $Generator.Count -eq 0) { throw 'Specify one or more -Generator values, or use -All. Run with -ListGenerators to see the choices.' }

$selectedGenerators = if ($All) { @($outputsByGenerator.Keys) } else { @($Generator | Select-Object -Unique) }
$outputPaths = @($selectedGenerators | ForEach-Object { $outputsByGenerator[$_] } | ForEach-Object { Join-Path $sourceDirectory $_ })
$existingPaths = @($outputPaths | Where-Object { Test-Path -LiteralPath $_ })
if ($existingPaths.Count -gt 0 -and -not $Force) {
    throw "Refusing to replace existing audio sources without -Force:`n$($existingPaths -join "`n")"
}
New-Item -ItemType Directory -Path $sourceDirectory -Force | Out-Null

foreach ($name in $selectedGenerators) {
    Write-Output "Generating audio sources: $name"
    & "Invoke-$name" -Force:$Force
}
Write-Output "Completed $($selectedGenerators.Count) audio source generator(s). Unreal asset imports are a separate step."