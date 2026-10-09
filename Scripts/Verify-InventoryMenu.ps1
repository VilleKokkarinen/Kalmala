param([int]$Port = 17991, [int]$Width = 1280, [int]$Height = 720,
    [int]$TextScale = 100, [int]$Contrast = 0, [int]$InterfaceScale = 100,
    [switch]$ReducedMotion)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot
$project = Join-Path $projectRoot 'Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
# Use a shallow evidence root so shader/config/temp paths remain below MAX_PATH.
$output = Join-Path $env:TEMP ('im-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
if (($output + '\Client\Intermediate\ShaderAutogen\PCD3D_SM5\AutogenShaderHeaders.ush').Length -ge 260) {
    throw "Evidence root is too long: $output"
}
New-Item -ItemType Directory -Path $output | Out-Null
New-Item -ItemType Directory -Path (Join-Path $output 'HShader'), (Join-Path $output 'CShader') | Out-Null
$hostLog = Join-Path $output 'host.log'
$clientLog = Join-Path $output 'client.log'
$Food = 'HearthBroth' # The only current catalogue item supported by the existing meal-use contract.
$common = "-game -nosound -unattended -nosplash -DDC-ForceMemoryCache -forcelogflush -windowed -RenderOffscreen -ForceRes -ResX=$Width -ResY=$Height -KalmalaInventoryTest -KalmalaInventoryMenuReview -KalmalaUIDeveloperTextScale=$TextScale -KalmalaUIDeveloperContrast=$Contrast -KalmalaUIDeveloperInterfaceScale=$InterfaceScale"
if ($ReducedMotion) { $common += ' -KalmalaInventoryMenuReducedMotion' }
$hostPeer = $null
$clientPeer = $null
try {
    $hostPeer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype?listen -port=$Port -WorldSeed=418 $common -ShaderWorkingDir=`"$output\HShader`" -UserDir=`"$output\Host`" -abslog=`"$hostLog`" -KalmalaInventoryMenuCapture=`"$output\host`""
    $deadline = (Get-Date).AddSeconds(90)
    do {
        if ($hostPeer.HasExited) { throw 'Inventory menu host exited during startup.' }
        if ((Test-Path $hostLog) -and (Select-String -LiteralPath $hostLog -Pattern 'GameNetDriver.*listening on port' -Quiet)) { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Inventory menu host startup timed out.' }
    $clientPeer = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" 127.0.0.1:$Port -WorldSeed=999 $common -ShaderWorkingDir=`"$output\CShader`" -UserDir=`"$output\Client`" -abslog=`"$clientLog`" -KalmalaInventoryMenuCapture=`"$output\client`""
    $deadline = (Get-Date).AddSeconds(180)
    do {
        if ($hostPeer.HasExited -or $clientPeer.HasExited) { throw 'Inventory menu peer exited before review completed.' }
        $hostText = if (Test-Path $hostLog) { [string](Get-Content -LiteralPath $hostLog -Raw) } else { '' }
        $clientText = if (Test-Path $clientLog) { [string](Get-Content -LiteralPath $clientLog -Raw) } else { '' }
        if (($hostText + $clientText) -match 'Fatal error:|Assertion failed:|Ensure condition failed:|Inventory menu (server|review).*Passed=0|Inventory menu review complete: Passed=0') {
            throw 'Inventory menu review reported a failure.'
        }
        if ($hostText -match "Inventory menu review complete: Passed=1 Host=1 Food=$Food" -and
            $clientText -match "Inventory menu review complete: Passed=1 Host=0 Food=$Food") { break }
        Start-Sleep -Milliseconds 500
    } while ((Get-Date) -lt $deadline)
    if ((Get-Date) -ge $deadline) { throw 'Inventory menu review timed out.' }
    $stages = @('filled', 'details', 'hotbar-assigned', 'hotbar-removed', 'food-ready', 'food-accepted', 'food-repeat', 'live', 'materials-empty', 'hud')
    foreach ($peer in @(@{ Name = 'host'; Text = $hostText; Host = 1 }, @{ Name = 'client'; Text = $clientText; Host = 0 })) {
        foreach ($stage in $stages) {
            if ($peer.Text -notmatch "Inventory menu review: Stage=$stage Passed=1 Host=$($peer.Host) Private=1") {
                throw "$($peer.Name) did not pass $stage."
            }
            $capture = Join-Path $output "$($peer.Name)-$stage.png"
            $captureDeadline = (Get-Date).AddSeconds(10)
            while (!(Test-Path $capture) -and (Get-Date) -lt $captureDeadline) { Start-Sleep -Milliseconds 250 }
            if (!(Test-Path $capture) -or (Get-Item -LiteralPath $capture).Length -le 32) { throw "Missing or empty capture: $capture" }
        }
    }
    if ($clientText -notmatch 'Client received world-generation identity: Seed=418' -or
        $clientText -notmatch 'Inventory remote: Empty=1' -or $clientText -match 'Inventory remote: Empty=0') {
        throw 'Inventory menu owner privacy/world identity failed.'
    }
    Write-Output "PASS: Inventory menu host/client shared forty-cell grids, hotbar assignment/removal, $Food use/repeat, live refresh, materials depleted without moving tools, and input restoration at ${Width}x${Height}/$TextScale%/contrast$Contrast."
}
finally {
    foreach ($peer in @($clientPeer, $hostPeer)) { if ($null -ne $peer -and !$peer.HasExited) { Stop-Process -Id $peer.Id } }
    Write-Output "Inventory menu evidence: $output"
}
