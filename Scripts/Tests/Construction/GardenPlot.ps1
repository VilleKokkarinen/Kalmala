[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Project,
    [int]$Port = 17879,
    [switch]$Rendered,
    [ValidateRange(640, 2560)][int]$Width = 1280,
    [ValidateRange(480, 1440)][int]$Height = 720
)

$ErrorActionPreference = 'Stop'
$projectPath = [IO.Path]::GetFullPath($Project)
if (!(Test-Path -LiteralPath $projectPath -PathType Leaf) -or [IO.Path]::GetFileName($projectPath) -ne 'Kalmala.uproject') {
    throw 'Project must point to an existing isolated Kalmala.uproject.'
}
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$repoPrefix = $repoRoot.TrimEnd([IO.Path]::DirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if ($projectPath.StartsWith($repoPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Use a compiled project mirror outside the repository so the runner leaves Binaries, Intermediate, and Saved untouched.'
}

$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path -LiteralPath $editor -PathType Leaf)) { throw "UnrealEditor.exe not found: $editor" }
$output = Join-Path $env:TEMP ('KGP-' + [guid]::NewGuid().ToString('N'))
$serverLog = Join-Path $output 'server.log'
$clientLog = Join-Path $output 'client.log'
$hostUserDir = Join-Path $output 'Host'
$clientUserDir = Join-Path $output 'Client'
$hostShaderDir = Join-Path $hostUserDir 'Shader'
$clientShaderDir = Join-Path $clientUserDir 'Shader'
$hostCapture = Join-Path $output 'host.png'
$clientCapture = Join-Path $output 'client.png'

function Read-SharedText([string]$Path) {
    $stream = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    $reader = [IO.StreamReader]::new($stream)
    try { return $reader.ReadToEnd() }
    finally { $reader.Dispose() }
}

foreach ($path in @($output, $serverLog, $clientLog, $hostCapture, $clientCapture)) {
    if ($path.Length -ge 260) { throw "MAX_PATH exceeded: $path" }
}
New-Item -ItemType Directory -Path $output, $hostUserDir, $clientUserDir -Force | Out-Null

$common = "-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaGardenPlotTest"
if ($Rendered) {
    New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir -Force | Out-Null
    $common += " -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height"
    $hostOptions = "-ShaderWorkingDir=`"$hostShaderDir`" -KalmalaGardenPlotCapture=`"$hostCapture`""
    $clientOptions = "-ShaderWorkingDir=`"$clientShaderDir`" -KalmalaGardenPlotCapture=`"$clientCapture`""
}
else {
    $common += ' -nullrhi'
    $hostOptions = ''
    $clientOptions = ''
}

$server = $null
$client = $null
try {
    $serverArgs = "`"$projectPath`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common $hostOptions -abslog=`"$serverLog`" -UserDir=`"$hostUserDir`""
    $server = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList $serverArgs
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Garden Plot listen server exited during startup.' }
        if ((Test-Path -LiteralPath $serverLog) -and (Select-String -LiteralPath $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Garden Plot listen server readiness timed out.' }

    $clientArgs = "`"$projectPath`" 127.0.0.1:$Port -WorldSeed=418 $common $clientOptions -abslog=`"$clientLog`" -UserDir=`"$clientUserDir`""
    $client = Start-Process -FilePath $editor -WindowStyle Hidden -PassThru -ArgumentList $clientArgs
    $deadline = (Get-Date).AddSeconds($(if ($Rendered) { 300 } else { 150 }))
    $ready = $false
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A Garden Plot peer exited before verification.' }
        $serverText = if (Test-Path -LiteralPath $serverLog) { Read-SharedText $serverLog } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Read-SharedText $clientLog } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Garden Plot (?:server|UI): .*Passed=0') {
            throw 'Garden Plot verification failed; inspect the retained host/client logs.'
        }
        $serverPasses = [regex]::Matches($serverText, 'Garden Plot server: Owner=\d+ Passed=1').Count -eq 2
        $hostUiPass = $serverText -match 'Garden Plot UI: Owner=\d+ View=Growing Passed=1'
        $clientUiPass = $clientText -match 'Garden Plot UI: Owner=\d+ View=Growing Passed=1'
        $ready = $serverPasses -and $hostUiPass -and $clientUiPass
        if ($ready -and $Rendered) { $ready = (Test-Path -LiteralPath $hostCapture) -and (Test-Path -LiteralPath $clientCapture) }
        if (!$ready) { Start-Sleep -Milliseconds 500 }
    } while (!$ready -and (Get-Date) -lt $deadline)
    if (!$ready) { throw 'Garden Plot host/client or rendered acceptance timed out.' }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418') { throw 'Garden Plot client world identity mismatch.' }

    if ($Rendered) {
        foreach ($capture in @($hostCapture, $clientCapture)) {
            $bytes = [IO.File]::ReadAllBytes($capture)
            if ($bytes.Length -lt 1024 -or [BitConverter]::ToString($bytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A') {
                throw "Rendered Garden Plot capture is missing, blank, or invalid: $capture"
            }
        }
    }

    Write-Output 'PASS: direct-material Garden Plot placement, host/client Carrot planting, material and Trowel payment, replicated crop state, seven Cultivation actions, and rendered host/client view.'
    Write-Output "Evidence: $output"
}
finally {
    foreach ($peer in @($client, $server)) {
        if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id }
    }
}
