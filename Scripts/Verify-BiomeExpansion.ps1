param(
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editorCmd = Join-Path (Split-Path $Editor) 'UnrealEditor-Cmd.exe'
$output = Join-Path $env:TEMP ('KalmalaBiomeExpansion-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
$automationLog = Join-Path $output 'automation.log'
$automation = $null
try {
    $automation = Start-Process $editorCmd -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -ExecCmds=`"Automation RunTests Kalmala.World.BiomeExpansion.IntegratedScenario`" -TestExit=`"Automation Test Queue Empty`" -abslog=`"$automationLog`""
    $deadline = (Get-Date).AddSeconds(60)
    do {
        $automationText = if (Test-Path $automationLog) { Get-Content $automationLog -Raw } else { '' }
        if ($automationText -match 'Result=\{Success\}.*IntegratedScenario') { break }
        if ($automationText -match 'No automation tests matched|IntegratedScenario.*(Fail|Error)') { throw 'Integrated biome automation failed; inspect its log.' }
        if ($automation.HasExited) { throw 'Integrated biome automation exited before completing.' }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Integrated biome automation timed out.' }
    $automationText = Get-Content $automationLog -Raw
    if ($automationText -match 'Fatal error:|Assertion failed:') { throw 'Integrated biome automation reported a fatal error.' }
    & (Join-Path $PSScriptRoot 'Verify-CampChoices.ps1') -Editor $Editor
    if ($LASTEXITCODE -ne 0) { throw "Two-player camp scenario exited with code $LASTEXITCODE." }
    Write-Output 'PASS: all biome deterministic seam/discovery/shelter checks and two-player replicated camp recovery passed.'
}
finally {
    if ($null -ne $automation -and !$automation.HasExited) { Stop-Process -Id $automation.Id }
    Write-Output "Biome verification log: $output"
}
