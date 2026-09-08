param([int]$Width = 1280, [int]$Height = 720)

$ErrorActionPreference = 'Stop'
$project = Join-Path $PSScriptRoot '..\Kalmala.uproject'
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$output = Join-Path ([System.IO.Path]::GetTempPath()) ('KalmalaWorldMap-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $output | Out-Null
foreach ($resolution in @(@(1024, 768), @(1280, 720), @(2560, 1080))) {
    $log = Join-Path $output ("$($resolution[0])x$($resolution[1]).log")
    $shot = Join-Path $output ("$($resolution[0])x$($resolution[1]).png")
    $process = Start-Process $editor -WindowStyle Hidden -PassThru -ArgumentList "`"$project`" /Game/Kalmala/Maps/Prototype/L_Prototype -game -windowed -RenderOffscreen -ForceRes -ResX=$($resolution[0]) -ResY=$($resolution[1]) -nosound -unattended -nosplash -DDC-ForceMemoryCache -KalmalaWorldMapVerification -KalmalaWorldMapScreenshot=`"$shot`" -abslog=`"$log`" -UserDir=`"$output\$($resolution[0])x$($resolution[1])`""
    try {
        $deadline = (Get-Date).AddSeconds(60)
        do {
            if ($process.HasExited) { throw "World-map process exited at $($resolution[0])x$($resolution[1])." }
            $text = if (Test-Path $log) { Get-Content $log -Raw } else { '' }
            if ($text -match 'Fatal error:|Assertion failed:') { throw "World-map verification failed at $($resolution[0])x$($resolution[1])." }
            if ($text -match 'World map verification: Open=1 Input=1 ZoomMin=1 ZoomMax=1 Pan=1 Recenter=1.' -and (Test-Path $shot)) { break }
            Start-Sleep -Milliseconds 500
        } while ((Get-Date) -lt $deadline)
        if ((Get-Date) -ge $deadline) { throw "World-map verification timed out at $($resolution[0])x$($resolution[1])." }
        Write-Output "PASS: $($resolution[0])x$($resolution[1]) world-map input and screenshot."
    } finally { if (!$process.HasExited) { Stop-Process -Id $process.Id } }
}
Write-Output "Scenario logs and screenshots: $output"
