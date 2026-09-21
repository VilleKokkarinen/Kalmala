param(
    [int]$Port = 18160,
    [string]$OutputDirectory = '',
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$output = if ($OutputDirectory) { $OutputDirectory } else { Join-Path $env:TEMP ('KalmalaM4VerticalSlice-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Path $output -Force | Out-Null

function Invoke-PeerScenario([string]$Name, [string]$Script, [int]$ScenarioPort) {
    $scenarioOutput = Join-Path $output $Name
    New-Item -ItemType Directory -Path $scenarioOutput -Force | Out-Null
    Write-Output "Running M4 $Name peer scenario on port $ScenarioPort."
    # The child is a PowerShell script, so failures propagate as terminating
    # errors. $LASTEXITCODE is only meaningful for native executables and may
    # be null or stale after a successful script invocation.
    & (Join-Path $PSScriptRoot $Script) -Port $ScenarioPort -OutputDirectory $scenarioOutput
}

function Invoke-SupportRegression {
    $editorCmd = Join-Path (Split-Path $Editor) 'UnrealEditor-Cmd.exe'
    $automationLog = Join-Path $output 'support-magic.log'
    $automation = Start-Process $editorCmd -WindowStyle Hidden -PassThru -ArgumentList "`"$(Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject')`" -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache -ExecCmds=`"Automation RunTests Kalmala.Gameplay.Discovery.PlayerScopedPersistence`" -TestExit=`"Automation Test Queue Empty`" -abslog=`"$automationLog`""
    try {
        $deadline = (Get-Date).AddSeconds(90)
        do {
            $text = if (Test-Path $automationLog) { Get-Content $automationLog -Raw } else { '' }
            if ($text -match 'Result=\{Success\}.*PlayerScopedPersistence') { return }
            if ($text -match 'No automation tests matched|PlayerScopedPersistence.*(Fail|Error)|Fatal error:|Assertion failed:|Ensure condition failed:') { throw 'M4 support-magic regression failed; inspect support-magic.log.' }
            if ($automation.HasExited) { throw 'M4 support-magic regression exited before completing.' }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        throw 'M4 support-magic regression timed out.'
    }
    finally { if (!$automation.HasExited) { Stop-Process -Id $automation.Id } }
}

try {
    Invoke-PeerScenario 'Mireling' 'Verify-MirelingPeer.ps1' $Port
    Invoke-PeerScenario 'Boar' 'Verify-BoarPeer.ps1' ($Port + 1)
    Invoke-PeerScenario 'Deer' 'Verify-DeerPeer.ps1' ($Port + 2)
    Invoke-SupportRegression
    Write-Output 'PASS: route-free M4 peer fixtures covered Mireling, boar, deer, owner-only rewards, defeat persistence, all four support-effect authority/non-damage gates, and matching-world learning persistence.'
}
finally { Write-Output "M4 vertical-slice evidence: $output" }
