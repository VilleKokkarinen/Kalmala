$ErrorActionPreference='Stop'
$root='C:/Users/Ville/AppData/Local/Temp/km13end'
$uat='C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat'
foreach ($config in @('Shipping')) {
 $archive=if($config -eq 'Development'){'C:/Users/Ville/AppData/Local/Temp/km13dev2'}else{'C:/Users/Ville/AppData/Local/Temp/km13ship2'}
 "START package-$config" | Tee-Object -FilePath "$root/package-results.txt" -Append
 & $uat BuildCookRun "-project=$root/Kalmala.uproject" -noP4 -platform=Win64 "-clientconfig=$config" -build -cook -stage -package -archive -pak -iostore "-archivedirectory=$archive" -unattended -utf8output '-UbtArgs=-MaxParallelActions=4' *> "$root/package-$config.log"
 if ($LASTEXITCODE -ne 0) { throw "Package $config exit $LASTEXITCODE" }
 "PASS package-$config" | Tee-Object -FilePath "$root/package-results.txt" -Append
 $exe=if($config -eq 'Development'){"$archive/Windows/Kalmala/Binaries/Win64/Kalmala.exe"}else{"$archive/Windows/Kalmala/Binaries/Win64/Kalmala-Win64-Shipping.exe"}
 $profile="$archive/smoke"
 New-Item -ItemType Directory $profile -Force | Out-Null
 $arguments="/Game/Kalmala/Maps/Prototype/L_Prototype -game -windowed -RenderOffscreen -ForceRes -ResX=1280 -ResY=720 -unattended -nosplash -nosound -DDC-ForceMemoryCache -forcelogflush -UserDir=$profile -abslog=$profile/game.log"
 if($config -eq 'Development'){ $arguments+=" -KalmalaHotbarCapture=$profile/icons" }
 $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
 try {
  $deadline=(Get-Date).AddSeconds(150)
  do {
   if($process.HasExited){throw "Packaged $config exited early"}
   $ready=if($config -eq 'Development'){(Test-Path "$profile/icons-icons.png") -and (Test-Path "$profile/icons-DeerCall.png")}else{((Get-Date) -gt $deadline.AddSeconds(-120))}
   if($ready){break}; Start-Sleep -Milliseconds 500
  }while((Get-Date) -lt $deadline)
  if(!$ready){throw "Packaged $config smoke timed out"}
  if($config -eq 'Development'){
   $log=Get-Content "$profile/game.log" -Raw
   if(!$log.Contains('Catalogue icon gallery: Canonical=52 ReadOnly=1')){throw 'Packaged icon gallery did not load'}
   if($log -match 'Fatal error:|Assertion failed:|Ensure condition failed:'){throw 'Packaged smoke diagnostic failure'}
  }
  "PASS smoke-$config" | Tee-Object -FilePath "$root/package-results.txt" -Append
 }finally{if(!$process.HasExited){Stop-Process -Id $process.Id}}
}
