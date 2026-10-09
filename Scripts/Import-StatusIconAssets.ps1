param(
    [string]$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$manifestPath = Join-Path $repoRoot 'docs\status-icon-manifest.csv'
$importerPath = Join-Path $repoRoot 'Scripts\Import-StatusIconAssets.py'
$lookupPath = Join-Path $repoRoot 'Source\KalmalaUI\Private\KalmalaStatusIconLibrary.cpp'
$sourceDir = Join-Path $repoRoot 'Content\Kalmala\UI\Source\Icons\Status'
$destinationDir = Join-Path $repoRoot 'Content\Kalmala\UI\Icons\Status'

foreach ($path in @($EditorCmd, $manifestPath, $importerPath, $lookupPath)) {
    if ($path.Length -ge 260) {
        throw "MAX_PATH violation: $path"
    }
}
if (!(Test-Path -LiteralPath $EditorCmd -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe was not found: $EditorCmd"
}
if (!(Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Status icon manifest was not found: $manifestPath"
}
if (!(Test-Path -LiteralPath $importerPath -PathType Leaf)) {
    throw "Status icon importer was not found: $importerPath"
}
if (!(Test-Path -LiteralPath $lookupPath -PathType Leaf)) {
    throw "Status icon lookup was not found: $lookupPath"
}

$lookupSource = [IO.File]::ReadAllText($lookupPath)
if (!$lookupSource.Contains('/Game/Kalmala/UI/Icons/Status/%s.%s')) {
    throw 'The status lookup path does not match the importer destination.'
}
$rows = @(Import-Csv -LiteralPath $manifestPath)
if ($rows.Count -ne 9) {
    throw "The pinned status manifest must contain nine identities; found $($rows.Count)."
}
$expectedBatchCounts = @{ '01' = 4; '02' = 4; '03' = 1 }
foreach ($batch in $expectedBatchCounts.Keys) {
    $batchCount = @($rows | Where-Object { $_.batch.Trim() -eq $batch }).Count
    if ($batchCount -ne $expectedBatchCounts[$batch]) {
        throw "Pinned generation batch $batch must contain $($expectedBatchCounts[$batch]) entries; found $batchCount."
    }
}
$entryIds = @($rows | ForEach-Object { $_.entry_id.Trim() })
$iconIds = @($rows | ForEach-Object { $_.icon_id.Trim() })
if (@($entryIds | Select-Object -Unique).Count -ne 9 -or @($iconIds | Select-Object -Unique).Count -ne 9) {
    throw 'Status entry and image IDs must each be unique.'
}

foreach ($row in $rows) {
    $entryId = $row.entry_id.Trim()
    $iconId = $row.icon_id.Trim()
    foreach ($path in @(
        (Join-Path $repoRoot "Content\Kalmala\UI\Source\IconOriginals\Status\$iconId.png"),
        (Join-Path $sourceDir "$iconId.png"),
        (Join-Path $destinationDir "$iconId.uasset")
    )) {
        if ($path.Length -ge 260) {
            throw "MAX_PATH violation: $path"
        }
    }
    $mappingNeedle = 'TEXT("' + $entryId + '"), TEXT("' + $iconId + '")'
    if (!$lookupSource.Contains($mappingNeedle)) {
        throw "Runtime status map is missing $entryId -> $iconId."
    }
    & (Join-Path $repoRoot 'Scripts\Validate-StatusIcon.ps1') -Id $iconId
}

if (Test-Path -LiteralPath $destinationDir) {
    $existingPackages = @(Get-ChildItem -LiteralPath $destinationDir -Filter '*.uasset' -File)
    if ($existingPackages.Count -gt 0) {
        throw "Status texture packages already exist; inspect before reimporting: $destinationDir"
    }
}

$scratchRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$scratchPath = Join-Path $scratchRoot ("KSI-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
$uprojectPath = Join-Path $scratchPath 'KalmalaStatusIcons.uproject'
$userDir = Join-Path $scratchPath 'User'
$logPath = Join-Path $scratchPath 'Import.log'
$scratchAssets = Join-Path $scratchPath 'Content\Kalmala\UI\Icons\Status'
foreach ($path in @($scratchPath, $uprojectPath, $userDir, $logPath, $scratchAssets)) {
    if ($path.Length -ge 260) {
        throw "MAX_PATH violation: $path"
    }
}
if (Test-Path -LiteralPath $scratchPath) {
    throw "Temporary project path already exists: $scratchPath"
}

New-Item -ItemType Directory -Path $scratchPath | Out-Null
$scratchProject = @{
    FileVersion = 3
    EngineAssociation = '5.8'
    Category = ''
    Description = 'Temporary content-only status icon importer'
    Plugins = @(
        @{ Name = 'PythonScriptPlugin'; Enabled = $true }
        @{ Name = 'EditorScriptingUtilities'; Enabled = $true }
    )
} | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText($uprojectPath, $scratchProject, [Text.UTF8Encoding]::new($false))

$oldRepoRoot = $env:KALMALA_STATUS_ICON_REPO_ROOT
$success = $false
try {
    $env:KALMALA_STATUS_ICON_REPO_ROOT = $repoRoot
    $arguments = @(
        $uprojectPath,
        '-unattended', '-nop4', '-nosplash', '-nullrhi',
        '-DDC-ForceMemoryCache',
        "-UserDir=$userDir", "-abslog=$logPath",
        '-run=pythonscript', '-EnablePlugin=PythonScriptPlugin',
        '-EnablePlugin=EditorScriptingUtilities', "-script=$importerPath"
    )
    $process = Start-Process -FilePath $EditorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (!$process.WaitForExit(120000)) {
        Stop-Process -Id $process.Id -Force
        $process.Close()
        throw "The Unreal status icon import exceeded 120 seconds (PID $($process.Id))."
    }
    $exitCode = $process.ExitCode
    $process.Close()
    if ($exitCode -ne 0) {
        throw "Unreal status icon import exited with $exitCode. See $logPath"
    }
    if (!(Test-Path -LiteralPath $logPath -PathType Leaf)) {
        throw "Unreal did not create its import log. Scratch project: $scratchPath"
    }

    $log = Get-Content -LiteralPath $logPath -Raw
    if ($log -match 'LogPython: Error|KALMALA_STATUS_ICON_IMPORT_FAILED') {
        throw "The Unreal importer logged an error. See $logPath"
    }
    if ($log -notmatch 'KALMALA_STATUS_ICON_IMPORT_COMPLETE: count=9') {
        throw "The Unreal importer did not complete all nine status images. See $logPath"
    }
    foreach ($row in $rows) {
        $entryId = $row.entry_id.Trim()
        $iconId = $row.icon_id.Trim()
        $marker = "KALMALA_STATUS_ICON_IMPORTED: entry=$entryId icon_id=$iconId object=/Game/Kalmala/UI/Icons/Status/$iconId.$iconId size=64x64 alpha=1 group=UI"
        if ($log -notmatch [regex]::Escape($marker)) {
            throw "The importer did not verify $entryId -> $iconId. See $logPath"
        }
    }

    foreach ($iconId in $iconIds) {
        if (!(Test-Path -LiteralPath (Join-Path $scratchAssets "$iconId.uasset") -PathType Leaf)) {
            throw "Unreal did not save the expected package for $iconId. See $logPath"
        }
    }
    $unexpected = @(Get-ChildItem -LiteralPath $scratchAssets -File | Where-Object { $_.BaseName -notin $iconIds })
    if ($unexpected.Count -gt 0) {
        throw "Scratch import created unexpected package files: $($unexpected.Name -join ', ')"
    }

    New-Item -ItemType Directory -Path $destinationDir -Force | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath $scratchAssets -File) {
        $target = Join-Path $destinationDir $file.Name
        if ($target.Length -ge 260) {
            throw "MAX_PATH violation: $target"
        }
        Copy-Item -LiteralPath $file.FullName -Destination $target
    }

    & (Join-Path $repoRoot 'Scripts\Validate-StatusIconSet.ps1')
    Write-Output 'Imported and validated the complete bounded status icon set (nine textures).'
    $success = $true
}
catch {
    Write-Output "Import scratch retained for diagnosis: $scratchPath"
    throw
}
finally {
    $env:KALMALA_STATUS_ICON_REPO_ROOT = $oldRepoRoot
}

if ($success) {
    $resolvedScratch = [IO.Path]::GetFullPath($scratchPath)
    $resolvedTempRoot = [IO.Path]::GetFullPath($scratchRoot).TrimEnd('\') + '\'
    if (!$resolvedScratch.StartsWith($resolvedTempRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove scratch outside the temporary directory: $resolvedScratch"
    }
    Remove-Item -LiteralPath $resolvedScratch -Recurse -Force
}
