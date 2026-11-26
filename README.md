# AstroToo Firmware

Build and customize the AstroToo application using the included prebuilt SDK and toolchain.

Before editing the firmware, read the [development guide](DEVELOPMENT.md) and [contributor rules](AGENTS.md).

> **Do not modify the factory-test code or the flash-entry flow. Changes may prevent the device from entering flash mode and recovering.**

## Requirements

Use x86_64 Linux or WSL 2 Ubuntu with Python 3.8 or later. Keep the checkout on the Linux filesystem when using WSL.

Install the build dependencies on Ubuntu:

```bash
sudo apt update
sudo apt install -y git git-lfs python3 make libc6-i386 lib32stdc++6
```

## Get the Source

```bash
git lfs install
git clone https://github.com/DivoomDevelop/AstroTooDevice.git
cd AstroTooDevice
git lfs pull
```

Git LFS is required to download the SDK resource images. The cross-compilation toolchain is included in the repository.

## Build Firmware

Run one of the following commands from the repository root in Linux or WSL 2.

### Release build

```bash
bash gerate_firmware.sh --jobs 8
```

Builds and packages firmware with application error logs.

### Debug build

```bash
bash debug_gerate_firmware.sh --jobs 8
```

Builds and packages firmware with application state and debug logs.

Both commands verify the SDK and toolchain before compiling, linking, and packaging. Outputs are written to `out/`; each build replaces the previous outputs at the same paths.

### Rebuild

To recompile all application source files:

```bash
bash gerate_firmware.sh --rebuild --jobs 8
# Or build debug firmware:
bash debug_gerate_firmware.sh --rebuild --jobs 8
```

### Verify or clean

```bash
bash build.sh --verify-only  # Check SDK and toolchain files
bash build.sh --clean        # Remove generated build/ and out/ directories
```

After cleaning, run the release or debug build command again.

## Project Layout

| Path | Purpose |
|---|---|
| `src/` | Editable application source |
| `app_sources.txt` | Application source files to compile, relative to `src/` |
| `sdk/` | Prebuilt libraries, public headers, board configuration, and resources |
| `toolchain/` | Cross-compilation toolchain |
| `build/` | Generated objects and build logs |
| `out/` | Generated firmware and symbols |

Add new C/C++ source files to `app_sources.txt`. Keep shared interfaces compatible with the prebuilt SDK. Do not change the fixed board configuration, partitions, bootloader, or linker scripts.

## Build Outputs

| File | Purpose |
|---|---|
| `out/rtos_freertos_f101s3_uart0_480Mmmc.img` | Complete flash image |
| `out/rt_system_debug.zip` | Matching symbol archive for debugging |
| `out/rt_system.elf`, `rt_system.bin`, `rt_system.map`, `rt_system.syms` | Firmware and debugging files |
| `out/rtos_pkg_nor.fex`, `rtos_pkg_nor.zip` | RTOS partition package |
| `out/build-report.json` | Build details and artifact hashes |

## Flash the Device

1. Build release or debug firmware.
2. On Windows, download and extract [PhoenixSuit](tools/phoenSuit.zip).
3. Open PhoenixSuit and select `out/rtos_freertos_f101s3_uart0_480Mmmc.img`.
4. Put the device into flash mode using the existing procedure, then flash the image.

**Flashing a complete image may overwrite user data.** Use partition packages only through the device's supported upgrade procedure. Check startup, display, networking, and flash-mode recovery on a development device before deploying modified firmware.

## Feature Limitations

- OSS cloud photo/video transfer is disabled because its API keys cannot be publicly redistributed. **When the device and app are not on the same LAN, cloud delivery of photos and videos is unavailable.**
- Network-based lyrics retrieval, including synchronized lyrics and fallback online sources, is unavailable.

## Licenses

See [licenses/](licenses/) for third-party notices and license information.
