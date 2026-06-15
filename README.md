# NoCompass End Dialog SKSE

NoCompass End Dialog SKSE is a lightweight SKSE plugin for Skyrim that fades the vanilla HUD, including the compass, while the Dialogue Menu is open and restores it when dialogue ends.

The mod is built for dialogue camera, cinematic letterbox, and minimal-HUD setups where compass or HUD elements can overlap conversations. It contains no ESP/ESL/ESM plugin, no Papyrus scripts, no MCM, and no SWF files.

## Requirements

- Skyrim Script Extender (SKSE) for your Skyrim runtime
- Address Library for SKSE Plugins, or VR Address Library for SKSEVR
- Skyrim SE 1.5.97, Skyrim AE/Steam 1.6.x, Skyrim GOG 1.6.x, or Skyrim VR where supported by CommonLibSSE-NG

## Installation

Download the latest release archive and install it with Mod Organizer 2, Vortex, or any mod manager that supports standard Skyrim `Data` archives. Enable the mod and launch the game through SKSE.

For manual installation, copy the archive contents into your Skyrim folder so the files land here:

```text
Data/SKSE/Plugins/DialogueHUDFade.dll
Data/SKSE/Plugins/DialogueHUDFade.ini
```

The INI file is optional. If it is removed, the plugin uses safe defaults.

## Configuration

Edit `Data/SKSE/Plugins/DialogueHUDFade.ini` to change fade duration, visible/hidden alpha, easing, debug logging, and HUD target paths.

## Stack

- C++23
- SKSE
- CommonLibSSE-NG
- Address Library
- CMake
- vcpkg

## Build

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset release
cmake --build --preset release --target package_mod
```

The mod-manager-ready package is staged in `dist` and copied to `Output` by `Build.ps1`.

## License

Source available, all rights reserved. See `LICENSE`.
