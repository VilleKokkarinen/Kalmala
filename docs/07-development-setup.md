# Development setup

## Baseline

- **Engine:** Installed Unreal Engine 5.8.2 build at `C:\Program Files\Epic Games\UE_5.8`.
- **Project:** `E:\dev\Kalmala\Kalmala.uproject`.
- **IDE:** Visual Studio 2026 with the Game development with C++ workload, MSVC tools, and a Windows SDK.

## First build

Open PowerShell and run:

```powershell
& 'C:\Program Files\Epic Games\Launcher\Engine\Binaries\Win64\UnrealVersionSelector.exe' /projectfiles 'E:\dev\Kalmala\Kalmala.uproject'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaEditor Win64 Development -Project='E:\dev\Kalmala\Kalmala.uproject' -WaitMutex
```

Open the project in the editor with:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' 'E:\dev\Kalmala\Kalmala.uproject'
```

The first editor launch must create the prototype map at `/Game/Kalmala/Maps/Prototype/L_Prototype`. Do not set it as the default map until it exists.

## Dedicated-server build

The Epic Games Launcher engine distribution does not include dedicated-server support. The `KalmalaServer` target remains in the project, but building it requires a UE 5.8 source build or another UE 5.8 distribution with server support. Do not attempt the command below with the installed Launcher engine.

With a server-capable engine, build the target with:

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' KalmalaServer Win64 Development -Project='E:\dev\Kalmala\Kalmala.uproject' -WaitMutex
```

## Source-control rules

- Commit `Config/`, `Source/`, `.uproject`, and `.uasset`/`.umap` content assets.
- Never commit generated `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/` folders.
- Unreal assets are marked as binary in `.gitattributes`; resolve asset conflicts in the editor, not through text merging.
