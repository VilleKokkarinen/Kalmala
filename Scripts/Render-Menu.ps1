[CmdletBinding()]
param(
    [string]$Menu = 'inventory-crafting',
    [string]$Output = '',
    [ValidateRange(320,4096)][int]$Width = 1920,
    [ValidateRange(240,2160)][int]$Height = 1080,
    [ValidateRange(100,150)][int]$TextScale = 100,
    [ValidateRange(0,2)][int]$Contrast = 0,
    [string]$WidgetClass = '',
    [string]$Cache = (Join-Path $env:TEMP 'kmmenu'),
    [string]$SeedCache = '',
    [string]$EditorCmd = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',
    [switch]$BuildModules,
    [switch]$Empty,
    [switch]$List,
    [ValidateRange(10,300)][int]$TimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'
$menuIds = @('inventory','crafting','inventory-crafting','build','settings.home','settings.options',
    'settings.audio','settings.controls','settings.controller','settings.video','settings.accessibility','settings.status')
if ($List) { $menuIds; return }
if (!$WidgetClass -and $Menu -notin $menuIds) { throw "Unknown menu '$Menu'. Use -List." }
$sourceRoot = [IO.Path]::GetFullPath((Split-Path $PSScriptRoot))
$Cache = [IO.Path]::GetFullPath($Cache)
if ($Cache.StartsWith($sourceRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Use a cache outside the source repository.' }
function Assert-ShortPath([string]$Path) { if ($Path.Length -ge 260) { throw "MAX_PATH exceeded: $Path" } }
Assert-ShortPath $Cache
Assert-ShortPath (Join-Path $Cache 'Intermediate/Build/Win64/x64/UnrealEditor/Development/KalmalaUI/KalmalaInventoryMenuSelectionTest.cpp.obj.response')
if (!$Output) { $Output = Join-Path $Cache ('png/' + $Menu + '-' + [guid]::NewGuid().ToString('N').Substring(0,8) + '.png') }
$Output = [IO.Path]::GetFullPath($Output)
Assert-ShortPath $Output
if ([IO.Path]::GetExtension($Output) -ne '.png' -or (Test-Path -LiteralPath $Output)) { throw 'Output must be a new .png file.' }
if (!(Test-Path -LiteralPath $EditorCmd)) { throw "Editor executable missing: $EditorCmd" }
if ($SeedCache) {
    $SeedCache = (Resolve-Path -LiteralPath $SeedCache).Path
    if ($SeedCache -eq $Cache) { throw 'Seed and destination cache must differ.' }
    if (Test-Path -LiteralPath (Join-Path $Cache 'Kalmala.uproject')) { throw 'SeedCache is only for a new cache.' }
    foreach ($seedFile in Get-ChildItem -LiteralPath $SeedCache -File -Recurse) {
        $destination = Join-Path $Cache ([IO.Path]::GetRelativePath($SeedCache, $seedFile.FullName))
        Assert-ShortPath $destination
    }
    New-Item -ItemType Directory -Force -Path $Cache | Out-Null
    & robocopy $SeedCache $Cache /E /XD .git .vs Saved DerivedDataCache run ddc png docs Scripts wt /NFL /NDL /NJH /NJS /NP | Out-Null
    if ($LASTEXITCODE -ge 8) { throw 'Editor cache seeding failed.' }
}
New-Item -ItemType Directory -Force -Path $Cache, (Split-Path $Output) | Out-Null
# Never mirror generated files back into source. Keep source/asset edits fresh even without compiling.
foreach ($directory in @('Source','Config','Content')) {
    foreach ($file in Get-ChildItem -LiteralPath (Join-Path $sourceRoot $directory) -File -Recurse) {
        $destination = Join-Path $Cache ([IO.Path]::GetRelativePath($sourceRoot, $file.FullName))
        Assert-ShortPath $destination
        if (!(Test-Path -LiteralPath $destination) -or (Get-Item -LiteralPath $destination).LastWriteTimeUtc -ne $file.LastWriteTimeUtc) {
            New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
            Copy-Item -LiteralPath $file.FullName -Destination $destination
        }
    }
}
Copy-Item -LiteralPath (Join-Path $sourceRoot 'Kalmala.uproject') -Destination (Join-Path $Cache 'Kalmala.uproject')
$project = Join-Path $Cache 'Kalmala.uproject'
$nativeSources = foreach ($module in @('KalmalaUI','KalmalaEditor','KalmalaGameplay')) {
    Get-ChildItem -LiteralPath (Join-Path $sourceRoot "Source/$module") -Recurse -File |
        Sort-Object FullName | ForEach-Object { ([IO.Path]::GetRelativePath($sourceRoot,$_.FullName)) + ':' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
}
$sourceDigest = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($nativeSources -join "`n"))))
$stamp = Join-Path $Cache 'menu-source.sha256'
if ($BuildModules) {
    $engine = Split-Path (Split-Path (Split-Path $EditorCmd))
    & (Join-Path $engine 'Build/BatchFiles/Build.bat') KalmalaEditor Win64 Development "-Project=$project" `
        -Module=KalmalaUI -Module=KalmalaEditor -Module=KalmalaGameplay -WaitMutex -NoHotReload -MaxParallelActions=4
    if ($LASTEXITCODE -ne 0) { throw 'Menu module compilation failed.' }
    [IO.File]::WriteAllText($stamp, $sourceDigest)
}
if (!(Test-Path -LiteralPath $stamp) -or [IO.File]::ReadAllText($stamp) -ne $sourceDigest) {
    throw 'Native menu source differs from the compiled cache. Run once with -BuildModules; unchanged renders need no build.'
}
if (!(Test-Path -LiteralPath (Join-Path $Cache 'Binaries/Win64/UnrealEditor-KalmalaEditor.dll'))) {
    throw 'A compiled editor cache is required once. Seed with -SeedCache <compiled-project> and use -BuildModules.'
}
$session = Join-Path $Cache ('run/' + [guid]::NewGuid().ToString('N').Substring(0,8))
Assert-ShortPath (Join-Path $session 'user/Saved/Config/WindowsEditor/GameUserSettings.ini')
New-Item -ItemType Directory -Force -Path $session | Out-Null
$log = Join-Path $session 'render.log'
# Arguments are passed to an executable, not interpreted by another shell.
$arguments = @("`"$project`"", '-run=RenderMenu', "-Menu=$Menu", "-Output=`"$Output`"", "-Width=$Width", "-Height=$Height",
    "-TextScale=$TextScale", "-Contrast=$Contrast", '-AllowCommandletRendering','-RenderOffscreen','-sm6','-unattended',
    '-nop4','-nosplash','-nosound',
    "-UserDir=`"$session/user`"", "-abslog=`"$log`"")
if ($Empty) { $arguments += '-Empty' }
if ($WidgetClass) { $arguments += "-WidgetClass=`"$WidgetClass`"" }
$timer = [Diagnostics.Stopwatch]::StartNew()
$process = Start-Process -FilePath $EditorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (!$process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill($true); throw "Render timed out. Log: $log" }
if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $Output)) { throw "Menu render failed ($($process.ExitCode)). Log: $log" }
$pngBytes = [IO.File]::ReadAllBytes($Output)
if ($pngBytes.Length -lt 24 -or [BitConverter]::ToString($pngBytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A') { throw 'Renderer did not produce a PNG.' }
[pscustomobject]@{ Menu=$Menu; PNG=$Output; Width=$Width; Height=$Height; Seconds=[math]::Round($timer.Elapsed.TotalSeconds,2); Log=$log }
