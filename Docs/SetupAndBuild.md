---
title: Echoes of the Broken Sun Setup and Build Guide
author: Angelis Pseftis
creator: Angelis Pseftis
status: Authoritative
created: 2026-08-28
updated: 2026-08-28
---

# Setup and Build Guide

This is the single authoritative setup and build guide. Edit it in place; do not create version-copy guides.

## Verified host state

The initial development host is a MacBook Pro (`MacBookPro18,1`) with an Apple M1 Pro, 10 CPU cores, 16 GPU cores, 16 GB unified memory, Metal 4, and macOS 26.6.2. Xcode 26.6 was installed, but the active developer directory pointed at Command Line Tools rather than full Xcode, and the separately delivered Metal Toolchain component was not installed. Epic Games Launcher 20.2.4 was installed. Unreal Engine 5.8.2 reached approximately 64% before the launcher was suspended because free space fell to a critical 11–14 GiB.

The official Epic installer endpoint was downloaded and the disk image verified. The retained file is `~/Downloads/EpicInstaller-20.1.4.dmg`, 114,676,675 bytes, SHA-256 `5c4f204ed623b01890f26cc99d4af657c3fbd6be1d04be7fed176ddbc94b1259`. Because the installed launcher is newer, do not install this backup over it unless repairing the launcher becomes necessary.

## Supported production baseline

The project pins Unreal Engine 5.8. Epic's version-specific macOS table lists Xcode 26.0 as the minimum and 26.1.1 as recommended, and explicitly marks 26.4 incompatible. The locally installed Xcode 26.6 is an unverified combination, not a confirmed failure. The controlled baseline is Xcode 26.1.1 until a clean build and full test run demonstrate another combination.

Epic recommends Apple Silicon M3 and 32 GB for UE 5.8. The inspected M1 Pro and 16 GB configuration meets Epic's lower hardware tier for supported rendering features, but it is below the recommendation. Nanite and Virtual Shadow Maps are disabled because Epic limits them to M2 or newer on Mac and still labels that support beta. Lumen software ray tracing may be evaluated; hardware ray tracing and MegaLights are not Mac targets.

Primary references:

- [Unreal Engine 5.8 release](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available)
- [Epic macOS development requirements](https://dev.epicgames.com/documentation/unreal-engine/macos-development-requirements-for-unreal-engine?lang=en-US)
- [Epic Apple Silicon and universal binaries guidance](https://dev.epicgames.com/documentation/unreal-engine/supporting-universal-binaries-for-macos-in-unreal-engine?lang=en-US)
- [Epic installation workflow](https://dev.epicgames.com/documentation/en-us/unreal-engine/install-unreal-engine)
- [Apple Xcode system requirements](https://developer.apple.com/xcode/system-requirements)
- [Apple Xcode resources](https://developer.apple.com/xcode/resources/)

## User installation gate

### 1. Create storage headroom

Free at least 60 GB before resuming the staged installer; 100 GB is the safer working target, or use a fast APFS external development volume. These thresholds are project engineering recommendations intended to avoid installation and packaging failures, not Epic official minimums.

The largest inspected local storage area was Docker's managed data, about 65 GB. Review images, containers, build cache, and volumes through Docker Desktop before removing anything. Do not delete `~/Library/Containers/com.docker.docker` manually; volumes may contain user data. Application Support and caches require application-specific review rather than wholesale deletion.

### 2. Install the pinned Xcode baseline

Use Apple's authenticated developer downloads to obtain Xcode 26.1.1. Keep Xcode 26.6 if other work needs it; the archived build can be renamed, for example `Xcode-26.1.1.app`, and installed alongside it. Complete Xcode's first-run setup and install its Metal Toolchain component.

For project-local commands, avoid silently changing every developer tool on the Mac:

```sh
export ECHOES_XCODE="/Applications/Xcode-26.1.1.app/Contents/Developer"
export DEVELOPER_DIR="$ECHOES_XCODE"
xcodebuild -version
xcrun metal -v
```

If Unreal Launcher or Editor does not honor that environment, select the same Xcode under Xcode Settings → Locations → Command Line Tools. A system-wide `xcode-select` change affects other projects and should be deliberate.

### 3. Complete Unreal Engine 5.8.2

After storage is available, resume Epic Games Launcher. The observed target is `/Users/Shared/Epic Games/UE_5.8`, and the launcher manifest reports version `5.8.2-56702186+++UE5+Release-5.8-Mac` with a 45,344,581,313-byte final installed size. The install included the Mac editor, templates, and engine source-view files. Do not add unused platform SDKs or debug symbols until needed.

The Launcher installation and sign-in are interactive and may require account verification, license acceptance, and macOS security approval. Those states cannot be inferred from the launcher merely existing.

## Native simulation test

The deterministic core is deliberately testable without Unreal:

```sh
./Scripts/test_sim.sh
```

The script compiles with Apple Clang, writes generated files only beneath `.build`, and runs the native suite. A passing suite validates only the named engine-independent behaviors; it does not validate Unreal integration, rendering, navigation, multiplayer transport, or packaging.

## Generate and build the Unreal project

After UE 5.8 is complete, define its location rather than embedding it in source:

```sh
export UE_ROOT="/Users/Shared/Epic Games/UE_5.8"
export ECHOES_ROOT="$PWD"

"$UE_ROOT/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" \
  -project="$ECHOES_ROOT/EchoesOfTheBrokenSun.uproject" -game

"$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" \
  EchoesOfTheBrokenSunEditor Mac Development \
  "$ECHOES_ROOT/EchoesOfTheBrokenSun.uproject" -waitmutex
```

Open the project only after the command-line build succeeds. Run the Unreal automation suite before treating editor launch as an accepted build. Packaging, signing, notarization, and clean-machine installation remain separate gates.

## Distribution boundary

A free Apple Account is enough for local Xcode development. Developer ID signing, notarization, and Mac App Store distribution require the appropriate Apple Developer Program credentials. No signing, notarization, App Store readiness, or compatibility beyond tested hosts may be claimed until observed and recorded.

