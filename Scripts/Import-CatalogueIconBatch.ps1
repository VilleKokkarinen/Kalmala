param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('A', 'B', 'C')]
    [string]$Batch,

    [string]$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$manifestPath = Join-Path $repoRoot 'docs\catalogue-icon-manifest.csv'
$importerPath = Join-Path $repoRoot 'Scripts\Import-CatalogueIconBatch.py'
$lookupPath = Join-Path $repoRoot 'Source\KalmalaUI\Private\KalmalaCatalogueIconLibrary.cpp'
$sourceDir = Join-Path $repoRoot 'Content\Kalmala\UI\Source\Icons'
$destinationDir = Join-Path $repoRoot 'Content\Kalmala\UI\Icons\Items'

if (!(Test-Path -LiteralPath $EditorCmd -PathType Leaf)) {
    throw "UnrealEditor-Cmd.exe was not found: $EditorCmd"
}
if (!(Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
    throw "Canonical icon manifest was not found: $manifestPath"
}
if (!(Test-Path -LiteralPath $importerPath -PathType Leaf)) {
    throw "Shared Python importer was not found: $importerPath"
}
if (!(Test-Path -LiteralPath $lookupPath -PathType Leaf)) {
    throw "Shared runtime texture lookup was not found: $lookupPath"
}
$lookupSource = [IO.File]::ReadAllText($lookupPath)
if (!$lookupSource.Contains('"/Game/Kalmala/UI/Icons/Items/%s.%s"')) {
    throw 'The runtime lookup path no longer matches the shared importer destination.'
}

$rows = @(Import-Csv -LiteralPath $manifestPath | Where-Object {
    $_.import_batch.Trim().ToUpperInvariant() -eq $Batch
})
if ($rows.Count -eq 0 -or $rows.Count -gt 16) {
    throw "Batch $Batch must contain between one and sixteen canonical identities; found $($rows.Count)."
}

$canonicalIds = @($rows | ForEach-Object { $_.canonical_id.Trim() })
if (@($canonicalIds | Select-Object -Unique).Count -ne $canonicalIds.Count) {
    throw "Batch $Batch contains duplicate canonical IDs."
}
foreach ($id in $canonicalIds) {
    foreach ($path in @(
        (Join-Path $sourceDir "$id.png"),
        (Join-Path $destinationDir "$id.uasset")
    )) {
        if ($path.Length -ge 260) {
            throw "MAX_PATH violation: $path"
        }
    }
    if (!(Test-Path -LiteralPath (Join-Path $sourceDir "$id.png") -PathType Leaf)) {
        throw "Prepared PNG is missing for $id."
    }
}

$scratchRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$scratchPath = Join-Path $scratchRoot ("KIC-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
$uprojectPath = Join-Path $scratchPath 'KalmalaIcons.uproject'
$userDir = Join-Path $scratchPath 'User'
$logPath = Join-Path $scratchPath 'Import.log'
$scratchAssets = Join-Path $scratchPath 'Content\Kalmala\UI\Icons\Items'

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
    Description = 'Temporary content-only catalogue icon importer'
    Plugins = @(
        @{ Name = 'PythonScriptPlugin'; Enabled = $true }
        @{ Name = 'EditorScriptingUtilities'; Enabled = $true }
    )
} | ConvertTo-Json -Depth 4
[IO.File]::WriteAllText($uprojectPath, $scratchProject, [Text.UTF8Encoding]::new($false))

$oldBatch = $env:KALMALA_ICON_IMPORT_BATCH
$oldRepoRoot = $env:KALMALA_ICON_REPO_ROOT
try {
    $env:KALMALA_ICON_IMPORT_BATCH = $Batch
    $env:KALMALA_ICON_REPO_ROOT = $repoRoot
    $arguments = @(
        $uprojectPath,
        '-unattended', '-nop4', '-nosplash', '-nullrhi',
        '-DDC-ForceMemoryCache',
        "-UserDir=$userDir", "-abslog=$logPath",
        '-run=pythonscript', '-EnablePlugin=PythonScriptPlugin',
        '-EnablePlugin=EditorScriptingUtilities', "-script=$importerPath"
    )
    $process = Start-Process -FilePath $EditorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (!$process.WaitForExit(60000)) {
        Stop-Process -Id $process.Id -Force
        $process.Close()
        throw "The Unreal icon import exceeded 60 seconds (PID $($process.Id))."
    }
    $exitCode = $process.ExitCode
    $process.Close()
    if ($exitCode -ne 0) {
        throw "Unreal icon import exited with $exitCode. See $logPath"
    }
    if (!(Test-Path -LiteralPath $logPath -PathType Leaf)) {
        throw "Unreal did not create its import log. Scratch project: $scratchPath"
    }

    $log = Get-Content -LiteralPath $logPath -Raw
    if ($log -match 'LogPython: Error|KALMALA_ICON_IMPORT_FAILED') {
        throw "The Unreal importer logged an error. See $logPath"
    }
    if ($log -notmatch "KALMALA_ICON_IMPORT_COMPLETE: batch=$Batch count=$($rows.Count)") {
        throw "The Unreal importer did not complete batch $Batch. See $logPath"
    }
    foreach ($id in $canonicalIds) {
        $marker = "KALMALA_ICON_IMPORTED: batch=$Batch canonical=$id object=/Game/Kalmala/UI/Icons/Items/$id.$id size=64x64 alpha=1 group=UI"
        if ($log -notmatch [regex]::Escape($marker)) {
            throw "Batch $Batch did not verify the expected object assignment for $id. See $logPath"
        }
    }

    foreach ($id in $canonicalIds) {
        $assetPath = Join-Path $scratchAssets "$id.uasset"
        if (!(Test-Path -LiteralPath $assetPath -PathType Leaf)) {
            throw "Unreal did not save the expected package for $id. See $logPath"
        }
    }
    $unexpected = @(Get-ChildItem -LiteralPath $scratchAssets -File | Where-Object { $_.BaseName -notin $canonicalIds })
    if ($unexpected.Count -gt 0) {
        throw "Scratch import created unexpected package files: $($unexpected.Name -join ', ')"
    }

    New-Item -ItemType Directory -Path $destinationDir -Force | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath $scratchAssets -File) {
        $target = Join-Path $destinationDir $file.Name
        if ($target.Length -ge 260) {
            throw "MAX_PATH violation: $target"
        }
        Copy-Item -LiteralPath $file.FullName -Destination $target -Force
    }

    foreach ($id in $canonicalIds) {
        & (Join-Path $repoRoot 'Scripts\Validate-CatalogueIcon.ps1') -Id $id
    }
    foreach ($row in $rows) {
        $aliasText = $row.catalogue_aliases.Trim()
        if ($aliasText -and $aliasText -ne 'none') {
            foreach ($alias in ($aliasText -split ';' | ForEach-Object { $_.Trim() } | Where-Object { $_ })) {
                if (Test-Path -LiteralPath (Join-Path $destinationDir "$alias.uasset")) {
                    throw "Alias $alias unexpectedly has a duplicate texture package."
                }
            }
        }
    }
    Write-Output "Imported and validated batch $Batch ($($canonicalIds.Count) canonical textures)."
    Write-Output "Canonical IDs: $($canonicalIds -join ', ')"
}
catch {
    Write-Output "Import scratch retained for diagnosis: $scratchPath"
    throw
}
finally {
    $env:KALMALA_ICON_IMPORT_BATCH = $oldBatch
    $env:KALMALA_ICON_REPO_ROOT = $oldRepoRoot
}

$resolvedScratch = [IO.Path]::GetFullPath($scratchPath)
$resolvedTempRoot = [IO.Path]::GetFullPath($scratchRoot).TrimEnd('\') + '\'
if (!$resolvedScratch.StartsWith($resolvedTempRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to remove scratch outside the temporary directory: $resolvedScratch"
}
Remove-Item -LiteralPath $resolvedScratch -Recurse -Force
