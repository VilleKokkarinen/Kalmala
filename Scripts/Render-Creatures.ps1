[CmdletBinding()]
param(
    [string[]]$Creature = @(),
    [string]$Output = '',
    [ValidateRange(320,4096)][int]$Width = 1280,
    [ValidateRange(240,2160)][int]$Height = 960,
    [string]$Cache = (Join-Path $env:TEMP 'kmcreature'),
    [string]$SeedCache = '',
    [string]$EditorCmd = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',
    [switch]$BuildModules,
    [switch]$List,
    [ValidateRange(10,300)][int]$TimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'

function Get-RelativePath([string]$BasePath, [string]$TargetPath) {
    $baseFullPath = [IO.Path]::GetFullPath($BasePath)
    $targetFullPath = [IO.Path]::GetFullPath($TargetPath)
    $rootPath = [IO.Path]::GetPathRoot($baseFullPath)
    $trimChars = [char[]]@([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar)
    if ($baseFullPath.Length -gt $rootPath.Length) { $baseFullPath = $baseFullPath.TrimEnd($trimChars) }
    if ($targetFullPath.Equals($baseFullPath, [StringComparison]::OrdinalIgnoreCase)) { return '.' }
    $prefix = if ($baseFullPath.EndsWith([string][IO.Path]::DirectorySeparatorChar)) { $baseFullPath } else { $baseFullPath + [IO.Path]::DirectorySeparatorChar }
    if (!$targetFullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Target path '$TargetPath' is not under base path '$BasePath'."
    }
    return $targetFullPath.Substring($prefix.Length)
}

function Get-Sha256Hex([byte[]]$Bytes) {
    $sha256 = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha256.ComputeHash($Bytes)).Replace('-', '') }
    finally { $sha256.Dispose() }
}

function Assert-ShortPath([string]$Path) {
    if ($Path.Length -ge 260) { throw "MAX_PATH exceeded: $Path" }
}

$creatureIds = @(
    'Mireling','Boar','Deer','Rat','Badger','MoonBear','DeathAdder','Viper','GiantSnake',
    'MosquitoSwarm','PeatHusk','Wolf','Lynx','PolarBear','HornedGoat','ThunderSentinel','ReefMaw'
)
if ($List) { $creatureIds; return }
if ($Creature.Count -eq 0) { $Creature = $creatureIds }
$unknownCreatures = @($Creature | Where-Object { $_ -notin $creatureIds })
if ($unknownCreatures.Count -gt 0) { throw "Unknown creature(s) '$($unknownCreatures -join ', ')'. Use -List." }
$canonicalCreatures = foreach ($requestedCreature in $Creature) {
    $creatureIds | Where-Object { $_ -ieq $requestedCreature } | Select-Object -First 1
}
$Creature = @($canonicalCreatures | Select-Object -Unique)

$sourceRoot = [IO.Path]::GetFullPath((Split-Path $PSScriptRoot))
$Cache = [IO.Path]::GetFullPath($Cache)
if ($Cache.StartsWith($sourceRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Use a cache outside the source repository.'
}
Assert-ShortPath $Cache
Assert-ShortPath (Join-Path $Cache 'Intermediate/Build/Win64/x64/UnrealEditor/Development/KalmalaUI/KalmalaInventoryMenuSelectionTest.cpp.obj.response')

$singleCreature = $Creature.Count -eq 1
if ($Output -and $singleCreature) {
    $Output = [IO.Path]::GetFullPath($Output)
    Assert-ShortPath $Output
    if ([IO.Path]::GetExtension($Output) -ne '.png' -or (Test-Path -LiteralPath $Output)) {
        throw 'Output must be a new .png file.'
    }
}
else {
    if ($Output) {
        $outputDirectory = [IO.Path]::GetFullPath($Output)
        if ([IO.Path]::GetExtension($outputDirectory) -eq '.png' -or (Test-Path -LiteralPath $outputDirectory -PathType Leaf)) {
            throw 'For multiple creatures, Output must be a directory.'
        }
    }
    else { $outputDirectory = Join-Path $Cache 'png' }
    Assert-ShortPath $outputDirectory
}

$renderTargets = @(
    foreach ($creatureId in $Creature) {
        if ($singleCreature -and $Output) { $outputPath = $Output }
        else { $outputPath = Join-Path $outputDirectory ($creatureId + '-' + [guid]::NewGuid().ToString('N').Substring(0,8) + '.png') }
        $outputPath = [IO.Path]::GetFullPath($outputPath)
        Assert-ShortPath $outputPath
        if ([IO.Path]::GetExtension($outputPath) -ne '.png' -or (Test-Path -LiteralPath $outputPath)) {
            throw "Output must be a new .png file: $outputPath"
        }
        [pscustomobject]@{ Creature=$creatureId; PNG=$outputPath }
    }
)

if (!(Test-Path -LiteralPath $EditorCmd)) { throw "Editor executable missing: $EditorCmd" }
if ($SeedCache) {
    $SeedCache = (Resolve-Path -LiteralPath $SeedCache).Path
    if ($SeedCache -eq $Cache) { throw 'Seed and destination cache must differ.' }
    if (Test-Path -LiteralPath (Join-Path $Cache 'Kalmala.uproject')) { throw 'SeedCache is only for a new cache.' }
    foreach ($seedFile in Get-ChildItem -LiteralPath $SeedCache -File -Recurse) {
        $destination = Join-Path $Cache (Get-RelativePath $SeedCache $seedFile.FullName)
        Assert-ShortPath $destination
    }
    New-Item -ItemType Directory -Force -Path $Cache | Out-Null
    & robocopy $SeedCache $Cache /E /XD .git .vs Saved DerivedDataCache run ddc png docs Scripts wt /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw 'Editor cache seeding failed.' }
}
New-Item -ItemType Directory -Force -Path $Cache | Out-Null
# Mirror only authored inputs; generated Unreal files stay in the separate cache.
foreach ($directory in @('Source','Config','Content')) {
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $sourceRoot $directory) -File -Recurse) {
        $destination = Join-Path $Cache (Get-RelativePath $sourceRoot $file.FullName)
        Assert-ShortPath $destination
        if (!(Test-Path -LiteralPath $destination) -or (Get-Item -LiteralPath $destination).LastWriteTimeUtc -ne $file.LastWriteTimeUtc) {
            New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $destination
        }
    }
}
Copy-Item -LiteralPath (Join-Path $sourceRoot 'Kalmala.uproject') -Destination (Join-Path $Cache 'Kalmala.uproject')
$project = Join-Path $Cache 'Kalmala.uproject'
$nativeSources = Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'Source') -Recurse -File |
    Sort-Object FullName | ForEach-Object { (Get-RelativePath $sourceRoot $_.FullName) + ':' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
$nativeSources += 'Kalmala.uproject:' + (Get-FileHash -LiteralPath $project -Algorithm SHA256).Hash
$sourceDigest = Get-Sha256Hex ([Text.Encoding]::UTF8.GetBytes(($nativeSources -join "`n")))
$stamp = Join-Path $Cache 'creature-source.sha256'
if ($BuildModules) {
    $engine = Split-Path (Split-Path (Split-Path $EditorCmd))
    & (Join-Path $engine 'Build/BatchFiles/Build.bat') KalmalaEditor Win64 Development "-Project=$project" `
        -WaitMutex -NoHotReload -MaxParallelActions=4 -gather
    if ($LASTEXITCODE -ne 0) { throw 'Creature preview editor target compilation failed.' }
    [IO.File]::WriteAllText($stamp, $sourceDigest)
}
if (!(Test-Path -LiteralPath $stamp) -or [IO.File]::ReadAllText($stamp) -ne $sourceDigest) {
    throw 'Native source differs from the compiled cache. Run once with -BuildModules; unchanged renders need no build.'
}
if (!(Test-Path -LiteralPath (Join-Path $Cache 'Binaries/Win64/UnrealEditor-KalmalaEditor.dll'))) {
    throw 'A compiled editor cache is required once. Seed with -SeedCache <compiled-project> and use -BuildModules.'
}

if (!($singleCreature -and $Output)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    $renderedCreatureNames = @($creatureIds | ForEach-Object { [regex]::Escape($_) }) -join '|'
    $renderedPNGPattern = '^(?:' + $renderedCreatureNames + ')-[0-9a-f]{8}\.png$'
    $staleRenderedPNGs = @(Get-ChildItem -LiteralPath $outputDirectory -File |
        Where-Object { $_.Name -match $renderedPNGPattern })
    foreach ($staleRenderedPNG in $staleRenderedPNGs) { Remove-Item -LiteralPath $staleRenderedPNG.FullName }
}

$session = Join-Path $Cache ('run/' + [guid]::NewGuid().ToString('N').Substring(0,8))
Assert-ShortPath (Join-Path $session 'user/Saved/Config/WindowsEditor/GameUserSettings.ini')
New-Item -ItemType Directory -Force -Path $session | Out-Null
foreach ($target in $renderTargets) {
    New-Item -ItemType Directory -Force -Path (Split-Path -Path $target.PNG -Parent) | Out-Null
}
$log = Join-Path $session 'render.log'
$arguments = @("`"$project`"", '-run=RenderCreatures', "-Width=$Width", "-Height=$Height",
    '-AllowCommandletRendering','-RenderOffscreen','-sm6','-unattended','-nop4','-nosplash','-nosound',
    "-UserDir=`"$session/user`"", "-abslog=`"$log`"")
if ($singleCreature) {
    $arguments += "-Creature=$($renderTargets[0].Creature)"
    $arguments += "-Output=`"$($renderTargets[0].PNG)`""
}
else {
    $creatureList = (@($renderTargets | ForEach-Object { $_.Creature }) -join '|')
    $outputList = (@($renderTargets | ForEach-Object { $_.PNG }) -join '|')
    $arguments += "-Creatures=`"$creatureList`""
    $arguments += "-Outputs=`"$outputList`""
}
$timer = [Diagnostics.Stopwatch]::StartNew()
$process = Start-Process -FilePath $EditorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
$lastRenderProgress = [Diagnostics.Stopwatch]::StartNew()
$completedRenderCount = 0
while (!$process.WaitForExit(1000)) {
    $newCompletedRenderCount = @($renderTargets | Where-Object { Test-Path -LiteralPath $_.PNG }).Count
    if ($newCompletedRenderCount -gt $completedRenderCount) {
        $completedRenderCount = $newCompletedRenderCount
        $lastRenderProgress.Restart()
    }
    if ($lastRenderProgress.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
        $process.Kill()
        throw "Creature render timed out after $TimeoutSeconds seconds without a completed image. Log: $log"
    }
}
if ($process.ExitCode -ne 0) { throw "Creature render set failed ($($process.ExitCode)). Log: $log" }
$renderLogText = Get-Content -LiteralPath $log -Raw
foreach ($target in $renderTargets) {
    $creatureId = $target.Creature
    $outputPath = $target.PNG
    if (!(Test-Path -LiteralPath $outputPath)) { throw "Creature render '$creatureId' did not produce an image. Log: $log" }
    $pngBytes = [IO.File]::ReadAllBytes($outputPath)
    if ($pngBytes.Length -lt 24 -or [BitConverter]::ToString($pngBytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A') {
        throw "Renderer did not produce a PNG for '$creatureId'."
    }
    $durationPattern = 'CreatureRender: Creature=' + [regex]::Escape($creatureId) + ' Size=\d+x\d+ Vertices=\d+ Saved=1 Seconds=([0-9.]+)'
    $durationMatch = [regex]::Match($renderLogText, $durationPattern)
    $seconds = if ($durationMatch.Success) {
        [double]::Parse($durationMatch.Groups[1].Value, [Globalization.CultureInfo]::InvariantCulture)
    }
    else { $timer.Elapsed.TotalSeconds }
    [pscustomobject]@{ Creature=$creatureId; PNG=$outputPath; Width=$Width; Height=$Height; Seconds=[math]::Round($seconds,2); BatchSeconds=[math]::Round($timer.Elapsed.TotalSeconds,2); Log=$log }
}
