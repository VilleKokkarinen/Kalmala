param(
    [ValidateRange(1024, 65530)]
    [int]$PortBase = 23820,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [ValidateRange(1, 20)]
    [int]$AutomationTimeoutMinutes = 8
)

$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$editorCmd = Join-Path (Split-Path $Editor) 'UnrealEditor-Cmd.exe'
$output = Join-Path $env:TEMP ('KalmalaM7AutomatedAcceptance-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output -Force | Out-Null

$automationTests = @(
    'Kalmala.World.M7.BiomeContentContract',
    'Kalmala.Gameplay.Progression.SkillContract',
    'Kalmala.Gameplay.Progression.ReplicationContract',
    'Kalmala.Gameplay.Tools.LifecycleContract',
    'Kalmala.Gameplay.Crafting.Transactions',
    'Kalmala.Gameplay.Crafting.NetworkContract',
    'Kalmala.Gameplay.Food.CampfireProcessing',
    'Kalmala.Gameplay.Status.SteadyMeal',
    'Kalmala.Gameplay.Exposure.RecoverableTravelPenalty',
    'Kalmala.Gameplay.Exposure.WeatherHazardResponse',
    'Kalmala.Gameplay.Discovery.PlayerScopedPersistence',
    'Kalmala.UI.SurvivalStatus.LocalPresentation',
    'Kalmala.UI.Inventory.PreparedFoodDetails'
)

function Assert-PortAvailable([int]$Port) {
    $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Any, $Port)
    try {
        $listener.Start()
    }
    catch {
        throw "Port $Port is already in use; choose an unused -PortBase."
    }
    finally {
        try { $listener.Stop() } catch { }
    }
}

function Invoke-M7Automation {
    $automationLog = Join-Path $output 'automation.log'
    $automationUser = Join-Path $output 'AutomationUser'
    $testList = $automationTests -join '+'
    $execCommands = "Automation RunTests $testList; Quit"
    $arguments = "`"$project`" -unattended -nop4 -nosplash -nullrhi -nosound -DDC-ForceMemoryCache -forcelogflush -UserDir=`"$automationUser`" -abslog=`"$automationLog`" -ExecCmds=`"$execCommands`" -TestExit=`"Automation Test Queue Empty`""
    $process = $null
    try {
        Write-Output "Running $($automationTests.Count) M7 automation tests headlessly."
        $process = Start-Process $editorCmd -WindowStyle Hidden -PassThru -ArgumentList $arguments
        $deadline = (Get-Date).AddMinutes($AutomationTimeoutMinutes)
        while (!$process.WaitForExit(1000)) {
            if ((Get-Date) -ge $deadline) { throw "M7 editor automation timed out; inspect $automationLog." }
            if (Test-Path -LiteralPath $automationLog) {
                $partialLog = Get-Content -LiteralPath $automationLog -Raw
                if ($partialLog -match 'No automation tests matched|Result=\{Fail\}|Fatal error:|Assertion failed:|Ensure condition failed:') {
                    throw "M7 editor automation reported a failure; inspect $automationLog."
                }
            }
        }
        if ($process.ExitCode -ne 0) { throw "M7 editor automation exited with code $($process.ExitCode); inspect $automationLog." }
        if (!(Test-Path -LiteralPath $automationLog)) { throw "M7 editor automation produced no log at $automationLog." }

        $automationText = Get-Content -LiteralPath $automationLog -Raw
        if ($automationText -match 'No automation tests matched|Result=\{Fail\}|Fatal error:|Assertion failed:|Ensure condition failed:') {
            throw "M7 editor automation reported a failure; inspect $automationLog."
        }
        foreach ($testName in $automationTests) {
            $successPattern = 'Result=\{Success\} Name=\{[^}]+\} Path=\{' + [regex]::Escape($testName) + '\}'
            if ($automationText -notmatch $successPattern) { throw "M7 editor automation did not report success for $testName; inspect $automationLog." }
        }
        Write-Output "PASS: all M7 automation results recorded in $automationLog."
    }
    finally {
        if ($null -ne $process -and !$process.HasExited) { Stop-Process -Id $process.Id }
    }
}

function Invoke-M7Scenario([string]$Name, [string]$ScriptName, [int]$Port, [switch]$Rendered, [switch]$HasOutputDirectory) {
    $scenarioScript = Join-Path $PSScriptRoot $ScriptName
    if (!(Test-Path -LiteralPath $scenarioScript)) { throw "Missing M7 scenario script: $scenarioScript" }
    Assert-PortAvailable $Port

    $scenarioOutput = Join-Path $output $Name
    New-Item -ItemType Directory -Path $scenarioOutput -Force | Out-Null
    $scenarioLog = Join-Path $scenarioOutput 'result.txt'
    $arguments = @{ Port = $Port }
    if ($Rendered) { $arguments['Rendered'] = $true }
    if ($HasOutputDirectory) { $arguments['OutputDirectory'] = $scenarioOutput }

    Write-Output "Running M7 scenario '$Name' on port $Port."
    $result = @(& $scenarioScript @arguments)
    $resultText = $result | ForEach-Object { [string]$_ }
    $resultText | Set-Content -LiteralPath $scenarioLog -Encoding utf8
    if (($resultText -join "`n") -notmatch '(?m)^PASS:') { throw "M7 scenario '$Name' did not return a PASS marker; inspect $scenarioLog." }
    $resultText | ForEach-Object { Write-Output $_ }
}

try {
    if (!(Test-Path -LiteralPath $project)) { throw "Project file not found: $project" }
    if (!(Test-Path -LiteralPath $editorCmd)) { throw "UnrealEditor-Cmd not found: $editorCmd" }
    for ($offset = 0; $offset -lt 5; ++$offset) { Assert-PortAvailable ($PortBase + $offset) }

    Invoke-M7Automation
    Invoke-M7Scenario 'gathering-and-tool-lifecycle' 'Verify-InventoryReconnect.ps1' $PortBase
    Invoke-M7Scenario 'rendered-crafting-and-stations' 'Verify-Crafting.ps1' ($PortBase + 1) -Rendered
    Invoke-M7Scenario 'weather-and-camp-recovery' 'Verify-CampChoices.ps1' ($PortBase + 2)
    Invoke-M7Scenario 'optional-boar-encounter' 'Verify-BoarPeer.ps1' ($PortBase + 3) -HasOutputDirectory
    Invoke-M7Scenario 'optional-discovery' 'Verify-DiscoveryPeer.ps1' ($PortBase + 4) -HasOutputDirectory

    Write-Output 'PASS: M7 automated progression/recovery acceptance completed without native computer use.'
}
finally {
    Write-Output "M7 automated acceptance evidence: $output"
}
