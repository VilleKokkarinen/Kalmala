$ErrorActionPreference='Stop'
$root='C:/Users/Ville/AppData/Local/Temp/km13dev2/smoke2'
New-Item -ItemType Directory $root -Force | Out-Null
$exe='C:/Users/Ville/AppData/Local/Temp/km13dev2/Windows/Kalmala/Binaries/Win64/Kalmala.exe'
$args="/Game/Kalmala/Maps/Prototype/L_Prototype -game -windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720 -unattended -nosplash -nosound -DDC-ForceMemoryCache -forcelogflush -UserDir=$root -abslog=$root/game.log -KalmalaHotbarCapture=$root/icons -LogCmds=`"LogTemp All`""
$p=Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru
try {
 $deadline=(Get-Date).AddSeconds(150)
 do {
  if($p.HasExited){throw 'Development package exited early'}
  $ready=$true
  foreach($phase in @('populated','icons','Mending','BearsVigor','DeerCall')){ $ready=$ready -and (Test-Path "$root/icons-$phase.png") }
  if($ready){break}; Start-Sleep -Milliseconds 500
 }while((Get-Date) -lt $deadline)
 if(!$ready){throw 'Development image captures timed out'}
 $log=Get-Content "$root/game.log" -Raw
 if(!$log.Contains('Game Engine Initialized.') -or !$log.Contains('Load map complete /Game/Kalmala/Maps/Prototype/L_Prototype')){throw 'Development startup failed'}
 if($log -match 'Fatal error:|Assertion failed:|Ensure condition failed:'){throw 'Development runtime error'}
 'PASS smoke-Development (fresh raster captures, engine initialization and prototype load)' | Tee-Object C:/Users/Ville/AppData/Local/Temp/km13end/package-results.txt -Append
}finally{if(!$p.HasExited){Stop-Process -Id $p.Id}}
