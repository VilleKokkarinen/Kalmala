function Get-KalmalaPaths {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$StartPath)

    $current = (Resolve-Path -LiteralPath $StartPath).Path
    while (-not (Test-Path -LiteralPath (Join-Path $current 'Kalmala.uproject'))) {
        $parent = Split-Path -Parent $current
        if ([string]::IsNullOrWhiteSpace($parent) -or $parent -eq $current) {
            throw "Could not locate Kalmala.uproject above '$StartPath'."
        }
        $current = $parent
    }

    [pscustomobject]@{
        ProjectRoot = $current
        ScriptsRoot = Join-Path $current 'Scripts'
        TestsRoot = Join-Path $current 'Scripts/Tests'
    }
}
