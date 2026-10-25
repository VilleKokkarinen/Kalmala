[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ProjectPath,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [string]$OutputDirectory = '',
    [ValidateRange(1, 20)][int]$TimeoutMinutes = 8
)

. (Join-Path (Split-Path $PSScriptRoot -Parent) 'TestPaths.ps1')
$repoPaths = Get-KalmalaPaths -StartPath $PSScriptRoot
$ErrorActionPreference = 'Stop'

$project = (Resolve-Path -LiteralPath $ProjectPath).Path
if ([IO.Path]::GetExtension($project) -ne '.uproject') { throw '-ProjectPath must name a compiled Kalmala.uproject.' }
$projectRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $project)).Path
$repoRoot = (Resolve-Path -LiteralPath $repoPaths.ProjectRoot).Path.TrimEnd('\')
$repoPrefix = $repoRoot + '\'
if (($project.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase)) -or
    ($projectRoot.Equals($repoRoot, [StringComparison]::OrdinalIgnoreCase))) {
    throw 'Use a compiled project mirror outside the source checkout so this run keeps generated files and SaveGames isolated.'
}

$editorCmd = Join-Path (Split-Path -Parent $Editor) 'UnrealEditor-Cmd.exe'
if (!(Test-Path -LiteralPath $project -PathType Leaf)) { throw "Project file not found: $project" }
if (!(Test-Path -LiteralPath $editorCmd -PathType Leaf)) { throw "UnrealEditor-Cmd not found: $editorCmd" }

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $env:TEMP ('KalmalaFreshA10-' + [guid]::NewGuid().ToString('N').Substring(0, 10))
} elseif (Test-Path -LiteralPath $OutputDirectory) {
    throw '-OutputDirectory must name a new directory.'
}
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (($output.Equals($repoRoot, [StringComparison]::OrdinalIgnoreCase)) -or
    ($output.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase))) {
    throw '-OutputDirectory must be outside the source checkout.'
}
$userDir = Join-Path $output 'User'
$log = Join-Path $output 'automation.log'
$mirrorSave = Join-Path $projectRoot 'Saved\SaveGames\KalmalaConstruction_18446744073709551615.sav'
$mirrorPopulationSave = Join-Path $projectRoot 'Saved\SaveGames\KalmalaPopulationDeltas_18446744073709551615.sav'
$mirrorPlayerSave = Join-Path $projectRoot 'Saved\SaveGames\KalmalaPlayerDiscoveries_18446744073709551615_FFFFFFFF.sav'
foreach ($path in @($project, $editorCmd, $output, $userDir, $log, $mirrorSave,
        $mirrorPopulationSave, $mirrorPlayerSave)) {
    if ($path.Length -ge 260) { throw "Path exceeds the 260-character Windows limit: $path" }
}
New-Item -ItemType Directory -Path $output | Out-Null

$seedHex = [guid]::NewGuid().ToString('N').Substring(0, 15)
$worldSeed = [Convert]::ToUInt64($seedHex, 16)
$testName = 'Kalmala.Gameplay.Progression.FreshStartBronzeAndIron'
$execCommands = "Automation RunTests $testName; Quit"
$arguments = "`"$project`" -unattended -nop4 -nosplash -nullrhi -nosound -DDC-ForceMemoryCache -forcelogflush -UserDir=`"$userDir`" -WorldSeed=$worldSeed -abslog=`"$log`" -ExecCmds=`"$execCommands`" -TestExit=`"Automation Test Queue Empty`""
$process = $null
try {
    Write-Output 'Running one no-debug fresh-start source-to-pick automation in the compiled mirror.'
    Write-Output "World seed: $worldSeed"
    $process = Start-Process $editorCmd -WindowStyle Hidden -PassThru -ArgumentList $arguments
    $deadline = (Get-Date).AddMinutes($TimeoutMinutes)
    while (!$process.WaitForExit(1000)) {
        if ((Get-Date) -ge $deadline) { throw "Fresh-start progression automation timed out; inspect $log." }
        if (Test-Path -LiteralPath $log) {
            $partial = Get-Content -LiteralPath $log -Raw
            if ($partial -match 'No automation tests matched|Result=\{Fail\}|Fatal error:|Assertion failed:|Ensure condition failed:') {
                throw "Fresh-start progression automation reported a failure; inspect $log."
            }
        }
    }
    if ($process.ExitCode -ne 0) { throw "Automation exited with code $($process.ExitCode); inspect $log." }
    if (!(Test-Path -LiteralPath $log -PathType Leaf)) { throw "Automation produced no log at $log." }
    $text = Get-Content -LiteralPath $log -Raw
    if ($text -match 'No automation tests matched|Result=\{Fail\}|Fatal error:|Assertion failed:|Ensure condition failed:') {
        throw "Fresh-start progression automation reported a failure; inspect $log."
    }
    $success = 'Result=\{Success\} Name=\{[^}]+\} Path=\{' + [regex]::Escape($testName) + '\}'
    if ($text -notmatch $success) { throw "Automation did not record success for $testName; inspect $log." }
    Write-Output 'PASS: starter-only inventory progressed through real source, Kiln, Smelter and Forge server transactions to Bronze and Iron Picks.'
    Write-Output "Evidence log: $log"
}
finally {
    if ($null -ne $process -and !$process.HasExited) { Stop-Process -Id $process.Id }
}
