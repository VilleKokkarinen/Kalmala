param(
    [string]$Parameters = (Join-Path $PSScriptRoot 'WorldMapPreview.params'),
    [string]$Output = (Join-Path (Split-Path $PSScriptRoot) '.cache/WorldMaps'),
    [string]$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
    [switch]$Watch,
    [switch]$Build
)
$ErrorActionPreference = 'Stop'
$project = Join-Path (Split-Path $PSScriptRoot) 'Kalmala.uproject'
$Parameters = (Resolve-Path -LiteralPath $Parameters).Path
$Output = [IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force -Path $Output | Out-Null
if ($Build) {
    $engineRoot = Split-Path (Split-Path (Split-Path $EditorCmd))
    & (Join-Path $engineRoot 'Build/BatchFiles/Build.bat') KalmalaEditor Win64 Development "-Project=$project" -WaitMutex -NoHotReload -Force -MaxParallelActions=4
    if ($LASTEXITCODE -ne 0) { throw 'Editor build failed.' }
}
$session = Join-Path $env:TEMP ('KalmalaMapExport-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $session | Out-Null
$log = Join-Path $session 'export.log'
$stopFile = Join-Path $session 'stop'
$arguments = "`"$project`" -run=ExportWorldMaps -Parameters=`"$Parameters`" -Output=`"$Output`" -StopFile=`"$stopFile`" -unattended -nop4 -nosplash -nosound -nullrhi -NoZenAutoLaunch -DDC=NoZenLocalFallback -LocalDataCachePath=`"$session/DDC`" -UserDir=`"$session/User`" -abslog=`"$log`" -forcelogflush"
if ($Watch) { $arguments += ' -Watch' }
$process = $null
$timer = [Diagnostics.Stopwatch]::StartNew()
try {
    Write-Output "Parameters: $Parameters"
    Write-Output "PNG output: $Output"
    if ($Watch) { Write-Output 'Watching: save the parameter file to regenerate. Ctrl+C stops the exporter.' }
    $process = Start-Process $EditorCmd -WindowStyle Hidden -PassThru -ArgumentList $arguments
    $lastReport = ''
    while (!$process.WaitForExit(250)) {
        if (Test-Path -LiteralPath $log) {
            $report = Get-Content -LiteralPath $log -Tail 12 | Where-Object { $_ -match 'World PNG export complete:|Invalid world-map parameters' } | Select-Object -Last 1
            if ($report -and $report -ne $lastReport) { Write-Output $report; $lastReport = $report }
        }
        if (!$Watch -and $timer.Elapsed.TotalMinutes -gt 5) { throw "Map export timed out. Log: $log" }
    }
    if ($process.ExitCode -ne 0) { throw "Map export failed (exit $($process.ExitCode)). Log: $log" }
    $text = Get-Content -LiteralPath $log -Raw
    if ($text -notmatch 'World PNG export complete:') { throw "No completed export found. Log: $log" }
    Write-Output (Get-Content -LiteralPath (Join-Path $Output 'LastRender.txt') -TotalCount 5)
    Write-Output ('Finished in {0:N2}s including headless startup.' -f $timer.Elapsed.TotalSeconds)
}
finally {
    if ($null -ne $process -and !$process.HasExited) {
        New-Item -ItemType File -Path $stopFile -Force | Out-Null
        if (!$process.WaitForExit(5000)) { Stop-Process -Id $process.Id }
    }
    Write-Output "Export log: $log"
}
