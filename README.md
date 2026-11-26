# AstroToo Application and Fixed SDK

Use this project to modify the AstroToo application, link it against the prebuilt platform SDK, and produce a complete flash image. The original AstroToo project serves as a read-only input during SDK export. The build in this repository uses its own scripts and configuration and can run independently of the original project.

Developers and AI tools should first read the [development guide](DEVELOPMENT.md) and [AI contributor rules](AGENTS.md), both in Chinese. Use the development guide and the current SDK for the current project layout and rules. The dated export and validation records below document earlier versions.

> **Keep the factory-test code and the path into flash mode intact. Changes can prevent the device from entering flash mode and leave it without a working recovery path.** See Section 2 of the development guide for the protected code and entry points.

## Cloud feature availability

OSS cloud photo/video transfer is disabled because its API keys cannot be publicly redistributed. **When the device and app are not on the same LAN, the device cannot receive photos or videos through OSS.** Local transfers on the same LAN remain available.

Google Calendar and Outlook Calendar remain available with their existing runtime authorization flow.

## Development Environment and Build

### 1. Set up Linux

Use x86_64 Linux and Python 3.8 or later. On an x86_64 Windows PC, run `wsl --install -d Ubuntu` from an administrator PowerShell session. Restart if prompted and open Ubuntu in WSL 2. Run `wsl -l -v` from PowerShell to check the WSL version. Run the remaining commands in the Linux terminal and keep the checkout on the Linux filesystem, for example in `~/AstroTooDevice`.

Install the dependencies on Ubuntu. The last two packages provide the 32-bit runtime libraries required by the Allwinner `dragon` packaging tool. For Windows setup details, see the [Microsoft WSL installation guide](https://learn.microsoft.com/windows/wsl/install).

```bash
sudo apt update
sudo apt install -y git git-lfs python3 make libc6-i386 lib32stdc++6
```

### 2. Get the source and fixed toolchain

```bash
git lfs install
cd ~
git clone https://github.com/DivoomDevelop/AstroTooDevice.git
cd AstroTooDevice
git lfs pull
```

Git LFS downloads two resource images used by the fixed SDK. The repository includes the x86_64 Linux build of the Xuantie V3.3.0 toolchain with GCC 14.3.0. The tools use the `riscv64-unknown-elf-` prefix. The build script supplies the board-specific flags that target the F101-S3's C907 RV32 core.

From the repository root, check that the cross-compiler runs. The build script uses the SDK manifest pinned by `sdk.lock` to locate the toolchain directly, so you can use your existing `PATH` and leave `CROSS_COMPILE` unset.

```bash
TC="$PWD/toolchain/Xuantie-900-gcc-elf-newlib-x86_64-V3.3.0/bin"
"$TC/riscv64-unknown-elf-gcc" --version
```

### 3. Verify and build

```bash
bash build.sh --verify-only     # Verify the fixed SDK and toolchain
bash build.sh --jobs 8          # Compile, link, validate, and package the application
```

After building, use the same toolchain to inspect the ELF architecture.

```bash
"$TC/riscv64-unknown-elf-readelf" -h out/rt_system.elf
```

Additional build commands are available.

```bash
bash build.sh --rebuild --jobs 8 # Recompile the application against the prebuilt SDK
bash build.sh --no-pack         # Generate the ELF and BIN only
bash build.sh --pack-only       # Package the existing out/rt_system.bin
bash build.sh --clean           # Remove this project's build/ and out/ directories only
```

You can also use `make`, `make verify`, `make rebuild`, `make pack`, and `make clean`.

## Repository Layout and Application Changes

| Path | Purpose |
|---|---|
| `src/` | Editable application source snapshot from `lichee/rtos/projects/f101s3/astrotoo/src` |
| `app_sources.txt` | Application compilation units for the current configuration. Add new source files here. |
| `sdk/lib/` | Standard static archives for the fixed platform and the original vendor libraries, all contained in this repository |
| `sdk/sysroot/` | Interface headers required by the application, with their original directory layout |
| `sdk/config/` | Fixed board configuration, exact compiler flags per compilation unit, and linker flags |
| `sdk/linker/` | Preprocessed linker scripts for the fixed SDK |
| `sdk/pack/` | Fixed bootloader, partitions, resource images, and packaging tools |
| `toolchain/` | Fixed cross-compilation toolchain |
| `sdk.lock`, `sdk/manifest.json` | SDK version and file integrity checks |
| `build/` | Application objects, dependency files, per-file compiler logs, and packaging staging area |
| `out/` | Generated firmware and build report |
| `verification/` | Local export and validation records. Application builds can run without these records. |

The build checks the SDK and toolchain file by file before compiling. Configuration changes require a new SDK from the maintainer. Use the repository's build script for application work and keep `defconfig`, `autoconf.h`, and the linker scripts intact. The original Makefiles under `src/` are retained as part of the snapshot. The standalone build has its own entry point and has no `menuconfig` or `mboot` target.

After editing an existing `.c`, `.cpp`, or `.h` file, run the build again. Dependency files identify the application units affected by header changes. The application links against the same precompiled platform objects. Structure layouts, compiler defines, and calling conventions shared with the platform must stay compatible with the SDK binaries.

The application QR code module directly includes the MIT-licensed LVGL QR encoder. The implementation and header are included in `src/thirdparty/qrcode/` so the standalone build can use local files. The copy adjusts only include paths.

Add each new source file to `app_sources.txt` on its own line, using a path relative to `src/`. Existing compilation units use their exact exported flags. New units inherit flags from an existing unit with the closest language and directory. Add only the third-party units you intend to compile, since some implementations are disabled in the current configuration.

## Build Outputs and Flashing

- `out/rt_system.elf`, `rt_system.bin`, `rt_system.map`, and `rt_system.syms` contain the linked RTOS image and debugging information.
- `out/rtos_freertos_f101s3_uart0_480Mmmc.img` is the complete production flash image for the fixed `f101s3_astrotoo` / `480Mmmc` target.
- `out/rtos_pkg_nor.fex` and `rtos_pkg_nor.zip` contain the RTOS partition package. Their filenames follow the original SDK naming convention for the `480Mmmc` target.
- `out/build-report.json` records the SDK ID, build duration, number of recompiled units, artifact sizes, and SHA-256 hashes.

### Package and flash with PhoenixSuit

From the repository root in Linux or WSL 2, build and package the firmware in either mode.

```bash
bash gerate_firmware.sh --jobs 8        # Release firmware with application error logs
bash debug_gerate_firmware.sh --jobs 8  # Firmware with application state and debug logs
```

Both scripts write the flash image to `out/rtos_freertos_f101s3_uart0_480Mmmc.img` and the matching symbol archive to `out/rt_system_debug.zip`. These paths are relative to the repository root. Each build replaces the previous outputs at those paths.

On Windows, download and extract the [PhoenixSuit flashing tool](tools/phoenSuit.zip). Open PhoenixSuit and select `out/rtos_freertos_f101s3_uart0_480Mmmc.img` as the firmware image. Put the device into flash mode using its existing procedure, then start flashing in PhoenixSuit.

Packaging runs in `build/pack`. It regenerates the RTOS package, restores the fixed U-Boot package, and invokes the Allwinner tool to generate the complete `.img`. It checks the RTOS load-memory limit and both the `bootA` and `bootB` partition capacities. The flash image from the original project stays intact.

The complete image includes the fixed resource partitions. Use the existing production flashing procedure and assume that flashing may overwrite user data. Use the RTOS partition package only through the device's supported upgrade procedure. The local build produces flashable artifacts. Flashing a device and publishing an OTA update are separate operations. Verify startup, display, networking, and recovery on a development device.

> Open-source policy as of 2026-11-25. The OSS implementation and credentials are removed from the distribution; compatibility download interfaces return failure. Google Calendar and Outlook Calendar remain available. Bailian and LyricFind remain disabled, and their credentials are excluded from the distribution. `Device/UploadLog` is disabled. New synchronization candidates and backups are stored outside the repository in `AstroToo_open_maintenance/`. The local `verification/private-archive-index.json` records earlier archived verification materials. The dated records below document the policy in effect at the time.

> Shared AES uses the SDK's existing private library implementation. The old key and IV copies have been removed from the public application sources. The encryption protocol, API, runtime behavior, factory tests, and flash recovery path retain their existing behavior.

## Internal Maintenance and Release

### Synchronize the SDK while keeping the application source

After building the platform and packaging the resources in the original project, run the SDK synchronization command from this repository's root.

```bash
bash sync_sdk.sh --jobs 8
# The default is a sibling AstroToo checkout. To specify the source explicitly, run
bash sync_sdk.sh --source /home/qifi/esp/allwinner/AstroToo --jobs 8
# Export and validate a candidate SDK without installing it
bash sync_sdk.sh --check-only --jobs 8
# Align the application source list when modules move into a library
bash sync_sdk.sh --align-source-list --jobs 8
# Migration only for older source trees without the newlib stdio lock patch
bash sync_sdk.sh --align-source-list --freeze-app-source newlib_stdio_lock.c --jobs 8
```

You can also run `make sync-sdk JOBS=8`. The command uses the same Linux or WSL environment and full-image packaging libraries as the regular build.

Synchronization copies `sdk/` (static archives, public headers, configuration, linker scripts, bootloader, fixed resource images, and packaging tools), `toolchain/`, `sdk.lock`, vendor licenses, and the input snapshot. It leaves `src/` intact and preserves `app_sources.txt` by default. It reads artifacts that already exist in the original project. Build any source changes there first and repackage the resources after changes under `board/.../data`.

The original project moved networking, Zstandard, WebP, SQLite, audio decoders, and minilzo into the `astrotoo_bundle` library. When migrating an application list that still includes those modules, run `--align-source-list` once. It drops units compiled as application sources in the old SDK that now come from a library and adds the application units from the new SDK. It preserves local units and comments. The synchronization report records the changes and the previous list is backed up with the old SDK for rollback. The old source files stay on disk. Editing them has no effect on the built library. If the new list requires a source file missing locally, validation fails and the maintainer must supply the file.

Use `--freeze-app-source PATH_RELATIVE_TO_SRC` to select a module that should use the object already built by the original project, such as the `newlib_stdio_lock.c` runtime patch used in an earlier migration. The exporter adds the object to a fixed archive at its original position relative to the application objects. It records the module in `sdk/config/frozen_application.json` for future syncs. If the original project stops providing the module, synchronization reports an error so the maintainer can review the interface. Only explicitly selected modules use this mechanism. SDK export also preserves the two newlib stdio linker wrapping flags.

The application source has since been updated to compile `newlib_stdio_lock.c` directly, so the freeze flag is unnecessary for the current tree. A standard SDK sync still preserves `src/`.

The sync first exports a candidate SDK under `verification/sdk-sync/<run-id>/candidate/` and performs a full build and package with a temporary copy of the current application. When the candidate passes, the sync backs up the previous SDK and `build/` and `out/`, installs the fixed files, and rebuilds and packages in the main checkout. This regenerates all application objects for the new SDK. If the application is incompatible with the candidate SDK, validation fails and the current SDK stays installed. If the build fails after installation, the sync restores the previous SDK and build outputs. If it detects concurrent SDK changes, it stops automatic rollback and preserves the changes and backup for manual review.

Synchronization compares the application source and source list before and after the operation and stops if it detects concurrent edits. Build, clean, and sync share one process lock and must run separately. The process releases the lock when it exits. Leave the lock file in place.

Each run writes logs and `result.json` (or `failure.json`) under `verification/sdk-sync/<run-id>/`. The run also keeps the previous version in `backup/` and retains the candidate checkout for troubleshooting. A run can take several gigabytes of disk space. Archive or remove its run directory once you have verified the installed version. The initial export record is `verification/integration-results.json`. For later versions, consult the corresponding synchronization report and `out/build-report.json`. Keep the SDK and the original project's build outputs stable while synchronization runs.

### Perform the initial full export

`tools/export_sdk.py` is an internal maintenance tool that reads artifacts from an already-built original project. It requires a fresh project scaffold with no existing `src/`, `sdk/`, or `toolchain/` trees.

```bash
python3 tools/export_sdk.py --source /path/to/AstroToo --output /path/to/new-project
```

The inputs must include all object files, their corresponding `.cmd` files, linker scripts, the toolchain, and the matching packaging files staged under `out/f101s3/astrotoo/image`. The exporter supports the verified single RTOS image layout without a separate XIP/PSRAM image, kernel compression, or kallsyms. It rejects unsupported configurations. The linker rules follow the existing C907 RV32 configuration.

The exporter checks for changes to the source and objects it reads during export and records a SHA-256 snapshot. Subsequent changes in the original project require another export or sync to reach this SDK.

## Initial Export Validation

The initial export compiled 679 application units from source, linked them with 1,247 prebuilt platform objects, and generated the complete 480Mmmc flash image. Integration checks confirmed the following results.

- An unchanged application rebuilt zero units. Adding one source file rebuilt only that unit and put its test marker in the binary. The test changes were then removed.
- Editing a fixed SDK configuration triggered an integrity check failure.
- The RTOS package checksums were valid and its payload exactly matched the linked BIN. The complete flash image contained that RTOS package and the fixed bootloader.
- The SDK contained standard static archives, and the application build used only local dependencies.
- All 7,838 input files from the original project retained their SHA-256 hashes. The checks stopped at image generation, with on-device validation still pending.

See `verification/integration-results.json` for the detailed local record. Run the integration check when the checkout is free of concurrent edits. The script temporarily changes one SDK configuration file and adds an application probe, then reverts its changes after the checks.

```bash
python3 tools/validate_project.py
```

## SDK Synchronization Validation

The initial library migration and a complete default `bash sync_sdk.sh --jobs 8` run were validated. SDK `f101s3_astrotoo-20261013T075737Z` compiled 538 application units, linked them against the updated prebuilt SDK, and packaged the result.

- The migration kept `src/` intact and removed 141 now-prebuilt units from the application source list. The subsequent default sync kept the list as it was.
- The SDK supplied the object for `newlib_stdio_lock.c`. Both linker wrapping flags and the corresponding symbols were verified.
- Ten tests passed for replacement, rollback, concurrent edit protection, source list alignment, and link order.
- The RTOS package checksum and BIN payload were verified, along with the bootloader and both resource partitions in the full image. All seven SDK archives were standard static archives.
- Application compile dependencies were local to this project. The original project continued to update its build outputs after the export and those differences were recorded. This SDK retained the exported snapshot. Further synchronization requires the original project to finish building and packaging first.

The local reports are `verification/sdk-sync/20261013T075627118113Z/result.json` and `post-checks.json` in the same directory. Each run kept the previous SDK and build outputs under `backup/`. The initial migration backup also kept the original application source list. On-device flashing was outside the scope of this validation.

## Application Source and SDK Update

This update aligned the application source with the original project's library split and used SDK `f101s3_astrotoo-20261013T081137Z`. Before the update, a comparison with the initial export snapshot confirmed that the local application tree had no independent edits. The original project served as a read-only input.

- The update added four files, changed 25 files, and removed 454 files whose modules had moved into the library. The application tree went from 1,718 to 1,268 files.
- The prebuilt bundle in `sdk/lib/projects_fixed_1.a` provided networking, WebP, Zstandard, SQLite, audio decoder, and minilzo implementations. The public headers were at `sdk/sysroot/rtos/projects/f101s3/astrotoo/lib/include/`.
- `newlib_stdio_lock.c` returned to source compilation. `sdk/config/frozen_application.json` was empty, and the application build list matched the original configuration at 539 units.
- The QR code module kept its local include paths for the standalone build.
- Both the candidate and installed checkouts completed a full compile, link, and flash-image build. On-device flashing remained pending.

The local update report is `verification/src-update/20261013T081036954367Z/result.json`. Its `backup/` contains the previous `src/`, application source list, SDK, and flash image.

Before a public release, follow the [release checklist](licenses/RELEASE_CHECKLIST.md) to confirm redistribution rights for the Allwinner SDK and the corresponding source obligations for GPL-covered binaries.

## Current open-source service restrictions

OSS is unavailable; its AccessKeys are excluded from source, SDK archives and firmware. Compatibility OSS download APIs return failure. Delivered photo tasks notify Device/DelPhoto for their original and preview objects and skip download; local albums are retained. Queue acceptance is not confirmation of server deletion.
Google Calendar and Outlook Calendar keep their original functionality. Factory-test and flash-entry code are unchanged.
