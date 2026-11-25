#!/usr/bin/env python3
"""Export a fixed, relocatable SDK from an existing build. Source is read-only."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
from datetime import datetime, timezone

PROJECT = "f101s3_astrotoo"
APP = "projects/f101s3/astrotoo/src"
TC = "Xuantie-900-gcc-elf-newlib-x86_64-V3.3.0"
HEADERS = {".h", ".hh", ".hpp", ".hxx", ".inc", ".inl", ".def"}


def digest(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source", type=Path, required=True)
    ap.add_argument("--output", type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument("--sdk-only", action="store_true", help="export fixed SDK without writing application sources or source list")
    ap.add_argument("--freeze-app-source", action="append", default=[], metavar="PATH",
                    help="SDK-only: retain the compiled object of this application source in the SDK")
    args = ap.parse_args()
    if args.freeze_app_source and not args.sdk_only:
        ap.error("--freeze-app-source requires --sdk-only")
    source, out = args.source.resolve(), args.output.resolve()
    if source == out or source in out.parents or out in source.parents:
        ap.error("source and output must be separate sibling trees")
    if not (out / "tools/build_app.py").is_file():
        ap.error("output must contain the new project's tools/build_app.py")
    protected = ("sdk", "toolchain", "sdk.lock") if args.sdk_only else ("src", "sdk", "toolchain", "app_sources.txt", "sdk.lock")
    for name in protected:
        if (out / name).exists():
            ap.error(f"refusing to overwrite existing {name}; export to a fresh project")
    rtos = source / "lichee/rtos"
    app = rtos / APP
    build = rtos / "build" / PROJECT
    tc = rtos / "tools" / TC
    gcc, ar = tc / "bin/riscv64-unknown-elf-gcc", tc / "bin/riscv64-unknown-elf-ar"
    sdk = out / "sdk"
    observed = {}

    def observe(p):
        p = Path(os.path.abspath(p))
        if p not in observed:
            observed[p] = digest(p)
        return p

    def copy(p, dest):
        p = observe(p)
        before = observed[p]
        dest.parent.mkdir(parents=True, exist_ok=True)
        if dest.exists():
            if digest(dest) != before:
                raise RuntimeError(f"different files map to {dest}")
            return
        shutil.copy2(p, dest)
        if digest(dest) != before:
            raise RuntimeError(f"source changed while copying {p}")

    def write(p, value):
        if p.exists():
            raise RuntimeError(f"refusing to overwrite {p}")
        p.parent.mkdir(parents=True, exist_ok=True)
        if isinstance(value, (list, dict)):
            value = json.dumps(value, ensure_ascii=False, indent=2) + "\n"
        p.write_text(value, encoding="utf-8")

    def sdk_path(p):
        p = Path(os.path.abspath(p))
        return sdk / "sysroot" / p.relative_to(source / "lichee")

    def portable(p):
        p = Path(os.path.abspath(rtos / p))
        if p == app or app in p.parents:
            return "{APP}/" + p.relative_to(app).as_posix()
        if p == tc or tc in p.parents:
            return "{TOOLCHAIN}/" + p.relative_to(tc).as_posix()
        if (source / "lichee") not in p.parents:
            raise RuntimeError(f"nonportable external path: {p}")
        return "{SDK}/sysroot/" + p.relative_to(source / "lichee").as_posix()

    print("[export] reading exact link members and compile recipes", flush=True)
    roots = ["arch", "components", "drivers", "kernel", "projects"]
    groups = {}
    for group in roots:
        archive = build / group / "obj-in.o"
        observe(archive)
        result = subprocess.check_output([str(ar), "t", str(archive.relative_to(rtos))], cwd=rtos, text=True)
        groups[group] = [Path(os.path.abspath(rtos / x)) for x in result.splitlines() if x.strip()]
    all_members = [p for g in roots for p in groups[g]]
    app_prefix = build / APP
    app_objects = [p for p in all_members if app_prefix in p.parents]
    if not app_objects:
        raise RuntimeError("no application objects found")
    recipes, include_dirs, dependencies = {}, set(), set()
    object_sources = {}
    for obj in app_objects:
        observe(obj)
        cmdfile = obj.with_name("." + obj.name + ".cmd")
        text = observe(cmdfile).read_text(errors="replace")
        command = next(line.split(" := ", 1)[1] for line in reversed(text.splitlines())
                       if line.startswith("cmd_") and " := " in line)
        if command.endswith("; set -e"):
            command = command[:-len("; set -e")]
        command = command.replace("$(pound)", "#")
        argv = shlex.split(command)
        source_file = argv[-1]
        src = Path(os.path.abspath(rtos / source_file))
        if app not in src.parents or src.suffix not in {".c", ".cpp", ".cc", ".cxx", ".S", ".s"}:
            raise RuntimeError(f"unexpected source for {obj}: {source_file}")
        flags, i = [], 1
        while i < len(argv):
            a = argv[i]
            if a.startswith("-Wp,-MD,") or a in ("-c", source_file):
                i += 1
                continue
            if a == "-o":
                i += 2
                continue
            if a in ("-I", "-include", "-isystem", "-iquote"):
                val = argv[i + 1]
                flags += [a, portable(val)]
                p = Path(os.path.abspath(rtos / val))
                if a in ("-I", "-isystem", "-iquote"):
                    include_dirs.add(p)
                else:
                    dependencies.add(p)
                i += 2
                continue
            if a.startswith("-I") and len(a) > 2:
                include_dirs.add(Path(os.path.abspath(rtos / a[2:])))
                flags += ["-I", portable(a[2:])]
            else:
                if str(source) in a:
                    raise RuntimeError(f"unhandled source path in argument: {a}")
                flags.append(a)
            i += 1
        # Keep every translation unit's original defines and include order.
        rel = src.relative_to(app).as_posix()
        object_sources[obj] = rel
        recipes[rel] = {"compiler": "g++" if "g++" in argv[0] else "gcc", "flags": flags}
        for line in text.split("\ncmd_", 1)[0].splitlines():
            token = line.strip().rstrip("\\").strip()
            if token and not token.startswith("#") and not token.endswith(":") and " " not in token:
                p = Path(os.path.abspath(rtos / token))
                if p.is_file() and tc not in p.parents and app not in p.parents:
                    dependencies.add(p)

    frozen = set(args.freeze_app_source)
    if frozen - recipes.keys():
        raise RuntimeError("cannot freeze unknown application source: " + ", ".join(sorted(frozen - recipes.keys())))
    for rel in frozen:
        observe(app / rel)
        del recipes[rel]
    write(sdk / "config/frozen_application.json", sorted(frozen))

    if not args.sdk_only:
        print(f"[export] copying {len(recipes)} application translation units and source tree", flush=True)
        for p in sorted(app.rglob("*")):
            if p.is_file() and not any(x in {".git", ".svn", "__pycache__"} for x in p.parts):
                if p.suffix not in {".o", ".pyc", ".d"}:
                    copy(p, out / "src" / p.relative_to(app))
        write(out / "app_sources.txt", "# Active application sources; add new sources here. Fixed SDK configuration.\n" +
              "\n".join(recipes) + "\n")
        # Preserve the source tree's shared-workspace directory access permissions.
        for p in sorted(app.rglob("*")):
            dest = out / "src" / p.relative_to(app)
            if p.is_dir() and dest.is_dir():
                shutil.copystat(p, dest)
        shutil.copystat(app, out / "src")
        (out / "app_sources.txt").chmod((app.stat().st_mode & 0o666) | 0o600)
    # Actual dependency closure plus interface headers support new application includes.
    for inc in sorted(include_dirs):
        if app == inc or app in inc.parents or tc in inc.parents:
            continue
        sdk_path(inc).mkdir(parents=True, exist_ok=True)
        if inc.is_dir():
            for p in inc.rglob("*"):
                if p.is_file() and p.suffix.lower() in HEADERS:
                    dependencies.add(p)
    # The application includes LVGL's MIT QR encoder as C. Keep that small
    # implementation in the open application instead of the precompiled SDK.
    qr = rtos / "components/common/thirdparty/littlevgl-9/lvgl/src/libs/qrcode/qrcodegen.c"
    if qr in dependencies and args.sdk_only:
        # This implementation is already part of the editable application.
        # Updating it here would violate the SDK-only synchronization contract.
        dependencies.remove(qr)
    elif qr in dependencies:
        copy(qr, out / "src/thirdparty/qrcode/qrcodegen.c")
        copy(qr.with_suffix(".h"), out / "src/thirdparty/qrcode/qrcodegen.h")
        wrapper = out / "src/middle/divoom_qrencode.c"
        latest = wrapper.read_text(encoding="utf-8")
        old = '#include "src/libs/qrcode/qrcodegen.c"'
        if latest.count(old) != 1:
            raise RuntimeError("unexpected QR wrapper; refuse broad replacement")
        wrapper.write_text(latest.replace(old, '#include "../thirdparty/qrcode/qrcodegen.c"'), encoding="utf-8")
        qr_copy = out / "src/thirdparty/qrcode/qrcodegen.c"
        latest = qr_copy.read_text(encoding="utf-8")
        qr_copy.write_text(latest.replace('#include "../../misc/lv_assert.h"', '#include "src/misc/lv_assert.h"'), encoding="utf-8")
        qr_header = out / "src/thirdparty/qrcode/qrcodegen.h"
        latest = qr_header.read_text(encoding="utf-8")
        qr_header.write_text(latest.replace('#include "../../../lvgl.h"', '#include "lvgl.h"'), encoding="utf-8")
        dependencies.remove(qr)
    for p in sorted(dependencies):
        if app in p.parents:
            continue
        if not p.is_file():
            raise RuntimeError(f"missing SDK dependency {p}")
        if p.suffix.lower() in {".c", ".cpp", ".s"}:
            raise RuntimeError(f"application directly includes non-application source: {p}")
        copy(p, sdk_path(p))
    for name in (".config", ".config.mk"):
        copy(rtos / name, sdk / "config" / name.lstrip("."))
    copy(build / "projects/f101s3/astrotoo/freertos.lds", sdk / "linker/freertos.lds")
    write(sdk / "config/recipes.json", recipes)

    print("[export] making regular archives (no thin archive references)", flush=True)
    link_inputs = []
    app_block_seen = False
    for group in roots:
        members = groups[group]
        # Project-level libraries may follow the application in the same Kbuild
        # archive. Keep the original sequence instead of moving them before src/.
        blocks = []
        for member in members:
            is_app = app_prefix in member.parents and object_sources[member] not in frozen
            if not blocks or blocks[-1][0] != is_app:
                blocks.append((is_app, []))
            blocks[-1][1].append(member)
        for index, (is_app, fixed) in enumerate(blocks):
            if is_app:
                if app_block_seen and not frozen:
                    raise RuntimeError("application objects must form one contiguous link block")
                app_block_seen = True
                link_inputs.append({"application_sources": [object_sources[p] for p in fixed]}
                                   if frozen else "{APP_OBJECTS}")
                continue
            name = group if len(blocks) == 1 else f"{group}_fixed_{index}"
            dest = sdk / "lib" / (name + ".a")
            dest.parent.mkdir(parents=True, exist_ok=True)
            for p in fixed:
                observe(p)
            # q appends, preserving duplicate basenames and original member order.
            for offset in range(0, len(fixed), 100):
                subprocess.run([str(ar), "qcD", str(dest), *map(str, fixed[offset:offset + 100])], check=True)
            subprocess.run([str(ar), "s", str(dest)], check=True)
            link_inputs.append("{SDK}/lib/" + name + ".a")
    application_blocks = [item for item in link_inputs if isinstance(item, dict)]
    if application_blocks:
        application_blocks[-1]["include_extra"] = True
    mapfile = observe(build / "img/rt_system.map")
    vendor = []
    for line in mapfile.read_text(errors="replace").splitlines():
        if line.startswith("LOAD "):
            p = Path(os.path.abspath(rtos / line[5:].strip()))
            if p.suffix == ".a" and tc not in p.parents and p not in vendor:
                vendor.append(p)
    for idx, p in enumerate(vendor):
        copy(p, sdk / "lib/vendor" / f"{idx:02d}_{p.name}")
    config = (rtos / ".config").read_text()
    def enabled(k):
        return f"{k}=y" in config.splitlines()
    if any(enabled(k) for k in ("CONFIG_COMPONENTS_KALLSYMS", "CONFIG_XIP", "CONFIG_PSRAM", "CONFIG_HPSRAM", "CONFIG_KERNEL_COMPRESS")):
        raise RuntimeError("this exporter supports the current single-image configuration only")
    link_flags = ["-nostdlib", "-Wl,--gc-sections", "-mcmodel=medany", "-mcpu=c907fdv-rv32"]
    # The current AstroToo target wraps newlib's two stdio locks at link time.
    top_makefile = observe(rtos / "Makefile").read_text()
    if enabled("CONFIG_PROJECT_F101S3_ASTROTOO"):
        link_flags += re.findall(r"^LDFLAGS-y\s*\+=\s*(-Wl,--wrap=__retarget_lock_(?:acquire|release)_recursive)\s*$",
                                 top_makefile, re.M)
    if enabled("CONFIG_COMPONENTS_STACK_PROTECTOR"):
        link_flags += ["-nostartfiles", "-nodefaultlibs"]
    if enabled("CONFIG_NEWLIBC_NANOSPECS"):
        link_flags += ["--specs=nano.specs"]
    for k, sym in (("CONFIG_NEWLIBC_NANO_PRINTF_FLOAT", "_printf_float"), ("CONFIG_NEWLIBC_NANO_SCANF_FLOAT", "_scanf_float")):
        if enabled(k):
            link_flags += ["-u", sym]
    write(sdk / "config/link.json", {"inputs": link_inputs, "flags": link_flags,
          "vendor": ["{SDK}/lib/vendor/" + f"{idx:02d}_{p.name}" for idx, p in enumerate(vendor)],
          "crash_build_id": enabled("CONFIG_DIVOOM_CRASH_LOG"),
          "objcopy_flags": ["-g", "-S", "-O", "binary", "-R", ".note", "-R", ".comment", "-R", "COMMON",
                            "-R", ".ARM.attributes", "-R", ".en_frame", "-R", ".init", "-R", ".fini", "--gap-fill=0x00"]})

    print("[export] copying toolchain and fixed packing assets", flush=True)
    shutil.copytree(tc, out / "toolchain" / TC, symlinks=False)
    image = source / "out/f101s3/astrotoo/image"
    # Start from the exact verified pack staging directory, excluding stale RTOS products.
    excluded = {"freertos.fex", "freertos-gz.fex", "freertos-gz-update.fex", "freertos-lz4.fex",
                "rtos_pkg_nor.fex", "Vrtos_pkg_nor.fex", "rtos_riscv.fex", "rtos_pkg.fex", "melis_pkg_nor.fex"}
    for p in sorted(image.iterdir()):
        if p.is_file() and p.name not in excluded:
            copy(p, sdk / "pack/template" / p.name)
    for name in ("dragon", "dragonsecboot", "plgvector.dll", "ImageBuilder.dll", "IniParasPlgex.dll"):
        copy(source / "tools/tool" / name, sdk / "pack/bin" / name)
    # Derive limits from the actual fixed board configuration.
    ucfg = observe(source / "lichee/brandy-2.0/u-boot-2018/configs/sun252iw2p1_freertos_nor_defconfig").read_text()
    malloc = int(re.search(r"^CONFIG_SUNXI_MALLOC_LEN=(\S+)", ucfg, re.M)[1], 0)
    partitions = (image / "sys_partition_for_dragon.fex").read_text()
    banks = [int(re.search(r"^\s*size\s*=\s*(\d+)", part, re.M)[1]) * 512
             for part in partitions.split("[partition]")
             if re.search(r"^\s*name\s*=\s*boot[AB]\s*$", part, re.M)]
    write(sdk / "config/board.json", {"target": PROJECT, "storage": "480Mmmc",
          "image_name": "rtos_freertos_f101s3_uart0_480Mmmc.img",
          "load_address": 0x40010000, "read_buffer": 0x40800000,
          "uboot_heap_start": 0x40f61000 - malloc, "boot_partition_bytes": min(banks)})
    for rel in ("lichee/rtos/LICENSE", "lichee/brandy-2.0/u-boot-2018/Licenses/README",
                "lichee/brandy-2.0/u-boot-2018/Licenses/gpl-2.0.txt",
                "lichee/rtos/kernel/FreeRTOS-orig/Source/LICENSE.md",
                "lichee/rtos/components/common/thirdparty/network/mbedtls-2.16.0/LICENSE",
                "lichee/rtos/components/common/thirdparty/network/mbedtls-2.16.0/apache-2.0.txt"):
        p = source / rel
        if p.is_file():
            copy(p, out / "licenses/vendor" / rel)
    project_lib = rtos / "projects/f101s3/astrotoo/lib"
    if project_lib.exists():
        for p in sorted((project_lib / "licenses").rglob("*")):
            if p.is_file():
                copy(p, out / "licenses/vendor" / p.relative_to(source))
        notice = project_lib / "THIRD_PARTY_NOTICES.md"
        if notice.is_file():
            copy(notice, out / "licenses/vendor" / notice.relative_to(source))
    copy(build / "img/rt_system.bin", sdk / "reference/rt_system.bin")
    # Copying never changes the source. Detect concurrent edits before accepting the snapshot.
    changed = [str(p.relative_to(source)) for p, h in observed.items() if digest(p) != h]
    if changed:
        raise RuntimeError("source changed during export; snapshot rejected: " + ", ".join(changed[:10]))
    write(out / "verification/source_snapshot.json", {str(p.relative_to(source)): h for p, h in observed.items()})
    manifest = {"schema": 1, "sdk_id": PROJECT + "-" + datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ"),
                "toolchain": TC, "compiler_version": subprocess.check_output([str(gcc), "--version"], text=True).splitlines()[0],
                "application_units": len(recipes), "platform_members": len(all_members) - len(app_objects) + len(frozen),
                "reference_sha256": digest(build / "img/rt_system.bin"), "files": {}}
    for p in sorted(sdk.rglob("*")):
        if p.is_file():
            manifest["files"][p.relative_to(out).as_posix()] = digest(p)
    for p in sorted((out / "toolchain").rglob("*")):
        if p.is_file():
            manifest["files"][p.relative_to(out).as_posix()] = digest(p)
    write(sdk / "manifest.json", manifest)
    write(out / "sdk.lock", {"sdk_id": manifest["sdk_id"], "manifest_sha256": digest(sdk / "manifest.json")})
    print(f"[export] done: {len(recipes)} application units, {manifest['platform_members']} platform objects, "
          f"{len(manifest['files'])} fixed files", flush=True)


if __name__ == "__main__":
    main()
