$ErrorActionPreference='Stop'
$root='C:/Users/Ville/AppData/Local/Temp/km13end'
Set-Location $root
function Check([string]$name,[scriptblock]$action) {
  "START $name $(Get-Date -Format o)" | Tee-Object -FilePath "$root/results.txt" -Append
  try { & $action *> "$root/$name.out"; "PASS $name" | Tee-Object -FilePath "$root/results.txt" -Append }
  catch { "FAIL $name $_" | Tee-Object -FilePath "$root/results.txt" -Append; throw }
}
Check 'automation' {
 & 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' "$root/Kalmala.uproject" -unattended -nop4 -nosplash -nullrhi -DDC-ForceMemoryCache "-UserDir=$root/test-user" "-abslog=$root/tests.log" '-ExecCmds=Automation RunTests Kalmala; Quit' '-TestExit=Automation Test Queue Empty'
 if ($LASTEXITCODE -ne 0) { throw "Automation exit $LASTEXITCODE" }
}
Check 'craft-standard' { & Scripts/Verify-Crafting.ps1 -Port 19800 -Rendered }
Check 'craft-compact' { & Scripts/Verify-Crafting.ps1 -Port 19801 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1 }
Check 'inventory-standard' { & Scripts/Verify-InventoryMenu.ps1 -Port 19802 }
Check 'inventory-compact' { & Scripts/Verify-InventoryMenu.ps1 -Port 19803 -Width 1024 -Height 768 -TextScale 150 -Contrast 1 -ReducedMotion }
Check 'inventory-reconnect' { & Scripts/Verify-InventoryReconnect.ps1 -Port 19804 }
Check 'settings-standard' { & Scripts/Verify-SettingsAccessibility.ps1 -Port 19805 }
Check 'settings-compact' { & Scripts/Verify-SettingsAccessibility.ps1 -Port 19806 -Width 1024 -Height 768 }
Check 'status-standard' { & Scripts/Verify-StatusHotbar.ps1 -Port 19807 }
Check 'status-compact' { & Scripts/Verify-StatusHotbar.ps1 -Port 19808 -Width 1024 -Height 768 -TextScale 150 -Contrast 1 }
Check 'status-wide' { & Scripts/Verify-StatusHotbar.ps1 -Port 19809 -Width 2560 -Height 1080 }
Check 'minimap-standard' { & Scripts/Verify-Minimap.ps1 -Port 19810 -Rendered -Width 1920 -Height 1080 }
Check 'minimap-compact' { & Scripts/Verify-Minimap.ps1 -Port 19811 -Rendered -Width 1024 -Height 768 }
Check 'minimap-wide' { & Scripts/Verify-Minimap.ps1 -Port 19812 -Rendered -Width 3440 -Height 1440 }
Check 'prompt-standard' { & Scripts/Verify-InteractionPrompt.ps1 -Port 19813 -Rendered }
Check 'prompt-compact' { & Scripts/Verify-InteractionPrompt.ps1 -Port 19814 -Rendered -Width 1024 -Height 768 -TextScale 150 -Contrast 1 }
'COMPLETE' | Tee-Object -FilePath "$root/results.txt" -Append
