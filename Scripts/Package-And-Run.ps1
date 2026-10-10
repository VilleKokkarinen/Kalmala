[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Dev", "Prod")]
    [string]$Mode,

    [string]$UnrealRoot = "C:\Program Files\Epic Games\UE_5.8",

    [string]$OutputRoot = (Join-Path $env:USERPROFILE "KalmalaBuilds"),

    [string]$Map = "/Game/Kalmala/Maps/Prototype/L_Prototype",

    [ValidateRange(640, 7680)]
    [int]$Width = 1280,

    [ValidateRange(480, 4320)]
    [int]$Height = 720
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ShortPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($fullPath.Length -ge 260) {
        throw "Path is $($fullPath.Length) characters; keep it below 260: $fullPath"
    }
    return $fullPath
}

function Assert-MirrorPathsFit {
    param(
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$MirrorRoot,
        [Parameter(Mandatory = $true)][string[]]$ExcludedDirectoryNames
    )

    $pending = New-Object 'System.Collections.Generic.Stack[string]'
    $pending.Push($SourceRoot)

    while ($pending.Count -gt 0) {
        $directory = $pending.Pop()
        foreach ($entry in [System.IO.Directory]::EnumerateFileSystemEntries($directory)) {
            $name = [System.IO.Path]::GetFileName($entry)
            if ($ExcludedDirectoryNames -contains $name) {
                continue
            }

            $relativePath = $entry.Substring($SourceRoot.Length).TrimStart('\')
            $null = Assert-ShortPath (Join-Path $MirrorRoot $relativePath)

            $attributes = [System.IO.File]::GetAttributes($entry)
            if (($attributes -band [System.IO.FileAttributes]::Directory) -and
                -not ($attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
                $pending.Push($entry)
            }
        }
    }
}

function Get-GameProcess {
    param([Parameter(Mandatory = $true)][string]$ExpectedPath)

    foreach ($candidate in (Get-Process -Name "Kalmala" -ErrorAction SilentlyContinue)) {
        try {
            if ([System.StringComparer]::OrdinalIgnoreCase.Equals(
                    [System.IO.Path]::GetFullPath($candidate.Path), $ExpectedPath)) {
                return $candidate
            }
        }
        catch {
            # Ignore a process whose executable path is not available to this user.
        }
    }
    return $null
}

function ConvertTo-ProcessArgumentString {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    return (($Arguments | ForEach-Object {
        if ($_ -match '\s') { '"' + $_ + '"' } else { $_ }
    }) -join ' ')
}

$projectRoot = Assert-ShortPath (Split-Path -Parent $PSScriptRoot)
$projectFile = Join-Path $projectRoot "Kalmala.uproject"
$unrealRoot = Assert-ShortPath $UnrealRoot
$runUat = Join-Path $unrealRoot "Engine\Build\BatchFiles\RunUAT.bat"
$outputRoot = Assert-ShortPath $OutputRoot

if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
    throw "Project file not found: $projectFile"
}
if (-not (Test-Path -LiteralPath $runUat -PathType Leaf)) {
    throw "RunUAT.bat not found under UnrealRoot: $runUat"
}

$projectPrefix = $projectRoot.TrimEnd('\') + '\'
if ([System.StringComparer]::OrdinalIgnoreCase.Equals($outputRoot, $projectRoot) -or
    $outputRoot.StartsWith($projectPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "OutputRoot must be outside the repository so package files never enter the source tree: $outputRoot"
}

if ($Mode -eq "Dev") {
    $clientConfig = "Development"
}
else {
    $clientConfig = "Shipping"
}

$runId = "{0}-{1}-{2}" -f $Mode, (Get-Date -Format "yyyyMMdd-HHmmss"), ([Guid]::NewGuid().ToString("N").Substring(0, 6))
$runRoot = Join-Path $outputRoot $runId
$mirrorRoot = Join-Path $runRoot "Source"
$archiveRoot = Join-Path $runRoot "Archive"
$packageRoot = Join-Path $archiveRoot "Windows"
$smokeRoot = Join-Path $runRoot "Smoke"
$userDir = Join-Path $smokeRoot "User"
$logRoot = Join-Path $smokeRoot "Logs"
$logPath = Join-Path $logRoot "Kalmala.log"
$wrapperPath = Join-Path $packageRoot "Kalmala.exe"
$gamePath = Join-Path $packageRoot "Kalmala\Binaries\Win64\Kalmala.exe"

foreach ($path in @($runRoot, $mirrorRoot, $archiveRoot, $packageRoot, $userDir, $logRoot, $logPath, $wrapperPath, $gamePath)) {
    $null = Assert-ShortPath $path
}
if (Test-Path -LiteralPath $runRoot) {
    throw "This unique run directory already exists; choose another OutputRoot: $runRoot"
}

$localAppData = $env:LOCALAPPDATA
if ([string]::IsNullOrWhiteSpace($localAppData)) {
    throw "LOCALAPPDATA is not set; UnrealBuildTool needs a writable user profile directory."
}
$ubtDirectory = Assert-ShortPath (Join-Path $localAppData "UnrealBuildTool")
$robocopy = (Get-Command "robocopy.exe" -ErrorAction Stop).Source

if (-not $PSCmdlet.ShouldProcess($runRoot, "Mirror, package Win64 $clientConfig, and launch Kalmala")) {
    return
}

$excludedDirectories = @(".git", ".vs", "Binaries", "Intermediate", "Saved", "DerivedDataCache")
Assert-MirrorPathsFit -SourceRoot $projectRoot -MirrorRoot $mirrorRoot -ExcludedDirectoryNames $excludedDirectories

if (Test-Path -LiteralPath $outputRoot -PathType Leaf) {
    throw "OutputRoot exists as a file: $outputRoot"
}
$null = New-Item -ItemType Directory -Path $outputRoot -Force
$null = New-Item -ItemType Directory -Path $runRoot

$copyArguments = @(
    $projectRoot,
    $mirrorRoot,
    "/E",
    "/XJ",
    "/R:1",
    "/W:1",
    "/NFL",
    "/NDL",
    "/XD"
) + $excludedDirectories
& $robocopy @copyArguments
$copyExitCode = $LASTEXITCODE
if ($copyExitCode -gt 7) {
    throw "Robocopy failed with exit code $copyExitCode. The partial mirror is at $mirrorRoot"
}

if (-not (Test-Path -LiteralPath $ubtDirectory -PathType Container)) {
    $null = New-Item -ItemType Directory -Path $ubtDirectory -Force
}
$uatArguments = @(
    "BuildCookRun",
    "-project=$(Join-Path $mirrorRoot 'Kalmala.uproject')",
    "-noP4",
    "-platform=Win64",
    "-clientconfig=$clientConfig",
    "-build",
    "-cook",
    "-stage",
    "-package",
    "-archive",
    "-pak",
    "-iostore",
    "-archivedirectory=$archiveRoot",
    "-unattended",
    "-utf8output"
)
& $runUat @uatArguments
$uatExitCode = $LASTEXITCODE
if ($uatExitCode -ne 0) {
    throw "BuildCookRun failed with exit code $uatExitCode. Review its output and the run directory: $runRoot"
}

if (-not (Test-Path -LiteralPath $wrapperPath -PathType Leaf)) {
    throw "The package completed but its launcher is missing: $wrapperPath"
}
if (-not (Test-Path -LiteralPath $gamePath -PathType Leaf)) {
    throw "The package completed but its game executable is missing: $gamePath"
}

$null = New-Item -ItemType Directory -Path $userDir
$null = New-Item -ItemType Directory -Path $logRoot
$launchArguments = @(
    $Map,
    "-game",
    "-windowed",
    "-ResX=$Width",
    "-ResY=$Height",
    "-UserDir=$userDir",
    "-abslog=$logPath"
)
$argumentString = ConvertTo-ProcessArgumentString $launchArguments
$launcher = Start-Process -FilePath $wrapperPath -ArgumentList $argumentString -WorkingDirectory $packageRoot -PassThru
Write-Host "Started packaged launcher PID $($launcher.Id). Waiting for the game process..."

$startupDeadline = [DateTime]::UtcNow.AddSeconds(120)
$gameProcess = $null
$developmentLogReady = $false
while ([DateTime]::UtcNow -lt $startupDeadline) {
    $gameProcess = Get-GameProcess -ExpectedPath $gamePath
    if ($Mode -eq "Dev" -and (Test-Path -LiteralPath $logPath -PathType Leaf)) {
        $logContents = Get-Content -LiteralPath $logPath -Raw -ErrorAction SilentlyContinue
        $developmentLogReady = $logContents -match "Game Engine Initialized" -and
            $logContents -match "Load map complete /Game/Kalmala/Maps/Prototype/L_Prototype"
    }

    if ($gameProcess -and ($Mode -eq "Prod" -or $developmentLogReady)) {
        break
    }
    Start-Sleep -Seconds 2
}

if (-not $gameProcess) {
    throw "Packaged game process did not appear at the expected path within 120 seconds: $gamePath"
}
if ($Mode -eq "Dev" -and -not $developmentLogReady) {
    $tail = if (Test-Path -LiteralPath $logPath) { Get-Content -LiteralPath $logPath -Tail 30 -ErrorAction SilentlyContinue } else { @("No startup log was created.") }
    $tail | ForEach-Object { Write-Host $_ }
    throw "Development startup log did not confirm engine initialization and prototype map load within 120 seconds."
}

for ($second = 0; $second -lt 20; $second++) {
    $gameProcess = Get-GameProcess -ExpectedPath $gamePath
    if (-not $gameProcess) {
        throw "The packaged game exited during its 20-second startup check. See $logPath"
    }
    Start-Sleep -Seconds 1
}

$gameProcess = Get-GameProcess -ExpectedPath $gamePath
Write-Host "Kalmala $clientConfig is running (PID $($gameProcess.Id))."
Write-Host "Package: $packageRoot"
Write-Host "Fresh user data: $userDir"
Write-Host "Startup log: $logPath"
