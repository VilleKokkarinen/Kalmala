[CmdletBinding()]
param(
    [string]$ProjectPath = '',
    [int]$Port = 19731,
    [switch]$Rendered,
    [int]$Width = 1280,
    [int]$Height = 720,
    [int]$TextScale = 100,
    [int]$Contrast = 0
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectPath)) { $ProjectPath = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject' }
$project = (Resolve-Path -LiteralPath $ProjectPath).Path
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) { throw "Unreal Editor was not found: $editor" }
$output = Join-Path $env:TEMP ('KIP-' + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $output, (Join-Path $output 'Host'), (Join-Path $output 'Client') -Force | Out-Null
$serverLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$common = "-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -KalmalaInteractionPromptReview -KalmalaUIDeveloperTextScale=$TextScale -KalmalaUIDeveloperContrast=$Contrast -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height -ExecCmds=`"t.MaxFPS 60`""
$serverCapture = Join-Path $output 'host.png'
$clientCapture = Join-Path $output 'client.png'
$hostShaderDir = Join-Path $output 'Host\ShaderWorkingDir'
$clientShaderDir = Join-Path $output 'Client\ShaderWorkingDir'
New-Item -ItemType Directory -Path $hostShaderDir, $clientShaderDir -Force | Out-Null
$server = $null
$client = $null
try {
    $server = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$hostShaderDir`" -KalmalaInteractionPromptCapture=`"$serverCapture`" -abslog=`"$serverLog`" -UserDir=`"$output\Host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($server.HasExited) { throw 'Listen server exited during startup.' }
        if ((Test-Path -LiteralPath $serverLog) -and (Select-String -LiteralPath $serverLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Listen server readiness timed out.' }

    $client = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$clientShaderDir`" -KalmalaInteractionPromptCapture=`"$clientCapture`" -abslog=`"$clientLog`" -UserDir=`"$output\Client`""
    $deadline = (Get-Date).AddSeconds(90)
    $stages = @('available', 'unavailable', 'modal', 'no-target', 'repair-all')
    do {
        if ($server.HasExited -or $client.HasExited) { throw 'A peer exited before prompt review finished.' }
        $serverText = if (Test-Path -LiteralPath $serverLog) { Get-Content -LiteralPath $serverLog -Raw } else { '' }
        $clientText = if (Test-Path -LiteralPath $clientLog) { Get-Content -LiteralPath $clientLog -Raw } else { '' }
        if (($serverText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Interaction prompt review: Stage=(available|unavailable|repair-all) Displayed=0|Interaction prompt review: Stage=(modal|no-target) Displayed=1|Grinding Stone interaction prompt: (RepairAll|NoBinding)=0') {
            throw 'Interaction prompt review failed; inspect retained peer logs.'
        }
        $ready = $clientText.Contains('Client received world-generation identity: Seed=418')
        foreach ($stage in $stages) {
            $expectedDisplay = if ($stage -in @('available','unavailable','repair-all')) { 1 } else { 0 }
            $ready = $ready -and $serverText.Contains("Interaction prompt review: Stage=$stage Displayed=$expectedDisplay")
            $ready = $ready -and $clientText.Contains("Interaction prompt review: Stage=$stage Displayed=$expectedDisplay")
            $ready = $ready -and (Test-Path -LiteralPath (Join-Path $output "host-$stage.png"))
            $ready = $ready -and (Test-Path -LiteralPath (Join-Path $output "client-$stage.png"))
        }
        $ready = $ready -and $serverText.Contains('Grinding Stone interaction prompt: RepairAll=1 NoBinding=1') `
            -and $clientText.Contains('Grinding Stone interaction prompt: RepairAll=1 NoBinding=1')
        if ($ready) { break }
        Start-Sleep -Milliseconds 250
    } while ((Get-Date) -lt $deadline)
    if (!$ready) { throw 'Host/client prompt review timed out or a required capture is missing.' }
    Write-Output "PASS: local host/client interaction prompt render at ${Width}x${Height}, text scale ${TextScale}%, contrast $Contrast; available, unavailable, modal, no-target and Grinding Stone Repair All states."
}
finally {
    foreach ($peer in @($client, $server)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Scenario logs and captures: $output"
}
