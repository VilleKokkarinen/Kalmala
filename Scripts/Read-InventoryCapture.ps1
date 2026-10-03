function Read-KalmalaInventoryCaptureLogs {
    param([string]$ServerLog, [string]$ClientLog, [string]$ScrollState)
    $captureDeadline = (Get-Date).AddSeconds(10)
    do {
        $serverSnapshot = Get-Content -LiteralPath $ServerLog -Raw
        $clientSnapshot = Get-Content -LiteralPath $ClientLog -Raw
        $captureReady = $true
        foreach ($snapshot in @($serverSnapshot, $clientSnapshot)) {
            if ($snapshot -match 'Fatal error:|Assertion failed:|Ensure condition failed:') {
                throw 'Inventory peer failed while capturing; inspect retained logs.'
            }
            $captureReady = $captureReady `
                -and $snapshot -match 'Inventory grid fixture: State=Empty PackSlots=16 Filled=0 Empty=16 CarriedTools=0' `
                -and $snapshot -match "Inventory grid fixture: State=Filled PackSlots=16 Filled=1 Empty=15 CarriedTools=\d+ Scrollable=$ScrollState"
        }
        if ($captureReady) {
            return @{ Server = $serverSnapshot; Client = $clientSnapshot }
        }
        Start-Sleep -Milliseconds 100
    } while ((Get-Date) -lt $captureDeadline)
    throw 'Inventory capture logs did not confirm both empty and filled views on both peers.'
}
