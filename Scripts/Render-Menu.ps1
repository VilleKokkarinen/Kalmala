[CmdletBinding()]
param(
    [string[]]$Menu = @(),
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
$menuIds = @('inventory','crafting','inventory-crafting','build','settings.home','settings.options',
    'settings.audio','settings.controls','settings.controller','settings.video','settings.accessibility')
$retiredMenuOutputIds = @('settings.status')
if ($List) { $menuIds; return }
if ($Menu.Count -eq 0) {
    # Keep the custom-widget invocation's previous single-context behavior.
    $Menu = if ($WidgetClass) { @('inventory-crafting') } else { @($menuIds | Where-Object { $_ -ne 'settings.options' }) }
}
if (!$WidgetClass) {
    $unknownMenus = @($Menu | Where-Object { $_ -notin $menuIds })
    if ($unknownMenus.Count -gt 0) { throw "Unknown menu(s) '$($unknownMenus -join ', ')'. Use -List." }

    # Render every Options selection together when the grouped target is requested.
    $expandedMenuIds = [System.Collections.Generic.List[string]]::new()
    foreach ($requestedMenuId in $Menu) {
        $requestedTargets = if ($requestedMenuId -eq 'settings.options') {
            @('settings.options','settings.controller','settings.video','settings.audio','settings.accessibility')
        }
        else { @($requestedMenuId) }
        foreach ($requestedTarget in $requestedTargets) {
            if (!$expandedMenuIds.Contains($requestedTarget)) { $expandedMenuIds.Add($requestedTarget) }
        }
    }
    $Menu = @($expandedMenuIds.ToArray())
}
$sourceRoot = [IO.Path]::GetFullPath((Split-Path $PSScriptRoot))
$Cache = [IO.Path]::GetFullPath($Cache)
if ($Cache.StartsWith($sourceRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Use a cache outside the source repository.' }
function Assert-ShortPath([string]$Path) { if ($Path.Length -ge 260) { throw "MAX_PATH exceeded: $Path" } }
Assert-ShortPath $Cache
Assert-ShortPath (Join-Path $Cache 'Intermediate/Build/Win64/x64/UnrealEditor/Development/KalmalaUI/KalmalaInventoryMenuSelectionTest.cpp.obj.response')
$singleMenu = $Menu.Count -eq 1
if ($Output -and $singleMenu) {
    $Output = [IO.Path]::GetFullPath($Output)
    Assert-ShortPath $Output
    if ([IO.Path]::GetExtension($Output) -ne '.png' -or (Test-Path -LiteralPath $Output)) { throw 'Output must be a new .png file.' }
}
else {
    if ($Output) {
        $outputDirectory = [IO.Path]::GetFullPath($Output)
        if ([IO.Path]::GetExtension($outputDirectory) -eq '.png' -or (Test-Path -LiteralPath $outputDirectory -PathType Leaf)) {
            throw 'For multiple menus, Output must be a directory.'
        }
    }
    else { $outputDirectory = Join-Path $Cache 'png' }
    Assert-ShortPath $outputDirectory
}
$renderTargets = @(
    foreach ($menuId in $Menu) {
        if ($singleMenu -and $Output) { $outputPath = $Output }
        else { $outputPath = Join-Path $outputDirectory ($menuId + '-' + [guid]::NewGuid().ToString('N').Substring(0,8) + '.png') }
        $outputPath = [IO.Path]::GetFullPath($outputPath)
        Assert-ShortPath $outputPath
        if ([IO.Path]::GetExtension($outputPath) -ne '.png' -or (Test-Path -LiteralPath $outputPath)) {
            throw "Output must be a new .png file: $outputPath"
        }
        [pscustomobject]@{ Menu=$menuId; PNG=$outputPath }
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
# Never mirror generated files back into source. Keep source/asset edits fresh even without compiling.
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
$nativeSources = foreach ($module in @('KalmalaUI','KalmalaEditor','KalmalaGameplay')) {
    Get-ChildItem -LiteralPath (Join-Path $sourceRoot "Source/$module") -Recurse -File |
        Sort-Object FullName | ForEach-Object { (Get-RelativePath $sourceRoot $_.FullName) + ':' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
}
$sourceDigest = Get-Sha256Hex ([Text.Encoding]::UTF8.GetBytes(($nativeSources -join "`n")))
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
if (!($singleMenu -and $Output)) {
    New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    $renderedMenuNames = (@($menuIds) + @($retiredMenuOutputIds) |
        ForEach-Object { [regex]::Escape($_) }) -join '|'
    $renderedPNGPattern = '^(?:' + $renderedMenuNames + ')-[0-9a-f]{8}\.png$'
    $staleRenderedPNGs = @(Get-ChildItem -LiteralPath $outputDirectory -File |
        Where-Object { $_.Name -match $renderedPNGPattern })
    foreach ($staleRenderedPNG in $staleRenderedPNGs) {
        Remove-Item -LiteralPath $staleRenderedPNG.FullName
    }
}
foreach ($target in $renderTargets) {
    $menuId = $target.Menu
    $outputPath = $target.PNG
    $session = Join-Path $Cache ('run/' + [guid]::NewGuid().ToString('N').Substring(0,8))
    Assert-ShortPath (Join-Path $session 'user/Saved/Config/WindowsEditor/GameUserSettings.ini')
    New-Item -ItemType Directory -Force -Path $session, (Split-Path -Path $outputPath -Parent) | Out-Null
    $log = Join-Path $session 'render.log'
    # Arguments are passed to an executable, not interpreted by another shell.
    $arguments = @("`"$project`"", '-run=RenderMenu', "-Menu=$menuId", "-Output=`"$outputPath`"", "-Width=$Width", "-Height=$Height",
        "-TextScale=$TextScale", "-Contrast=$Contrast", '-AllowCommandletRendering','-RenderOffscreen','-sm6','-unattended',
        '-nop4','-nosplash','-nosound',
        "-UserDir=`"$session/user`"", "-abslog=`"$log`"")
    if ($Empty) { $arguments += '-Empty' }
    if ($WidgetClass) { $arguments += "-WidgetClass=`"$WidgetClass`"" }
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $process = Start-Process -FilePath $EditorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) { $process.Kill(); throw "Render timed out. Log: $log" }
    if ($process.ExitCode -ne 0 -or !(Test-Path -LiteralPath $outputPath)) { throw "Menu render '$menuId' failed ($($process.ExitCode)). Log: $log" }
    $pngBytes = [IO.File]::ReadAllBytes($outputPath)
    if ($pngBytes.Length -lt 24 -or [BitConverter]::ToString($pngBytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A') { throw "Renderer did not produce a PNG for '$menuId'." }
    [pscustomobject]@{ Menu=$menuId; PNG=$outputPath; Width=$Width; Height=$Height; Seconds=[math]::Round($timer.Elapsed.TotalSeconds,2); Log=$log }
}
