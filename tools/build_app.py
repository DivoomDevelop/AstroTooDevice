#!/usr/bin/env python3
"""Compile application sources, link the frozen SDK, and make a flash image."""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile
from operation_lock import operation_lock

ROOT = Path(__file__).resolve().parents[1]
SDK = ROOT / "sdk"
BUILD = ROOT / "build"
OUT = ROOT / "out"


def digest(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def load(p):
    return json.loads(p.read_text(encoding="utf-8"))


def safe_write(p, data):
    """Read current state just before replacing our own generated artifact."""
    if p.exists():
        if p.is_symlink() or not p.is_file():
            raise RuntimeError(f"unexpected output target {p}")
        p.read_bytes()
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(data, encoding="utf-8")


def verify():
    lock = load(ROOT / "sdk.lock")
    if digest(SDK / "manifest.json") != lock["manifest_sha256"]:
        raise RuntimeError("SDK manifest changed; install the matching original SDK")
    manifest = load(SDK / "manifest.json")
    if manifest["sdk_id"] != lock["sdk_id"]:
        raise RuntimeError("SDK version mismatch")
    for rel, expected in manifest["files"].items():
        p = ROOT / rel
        if not p.is_file() or digest(p) != expected:
            raise RuntimeError(f"fixed SDK file missing or modified: {rel}")
    print(f"[verify] {manifest['sdk_id']}: {len(manifest['files'])} fixed files OK", flush=True)
    return manifest


def run(cmd, log):
    log.parent.mkdir(parents=True, exist_ok=True)
    with open(log, "wb") as f:
        result = subprocess.run(cmd, cwd=ROOT, stdout=f, stderr=subprocess.STDOUT)
    if result.returncode:
        tail = log.read_text(errors="replace").splitlines()[-35:]
        raise RuntimeError(f"command failed ({result.returncode}); {log.relative_to(ROOT)}\n" + "\n".join(tail))


def compile_app(recipes, expand, tc, jobs, rebuild, log_level):
    sources = [s.strip() for s in (ROOT / "app_sources.txt").read_text().splitlines()
               if s.strip() and not s.lstrip().startswith("#")]
    if len(sources) != len(set(sources)):
        raise RuntimeError("duplicate sources in app_sources.txt")
    if not sources:
        raise RuntimeError("app_sources.txt is empty")

    def one(rel):
        src = (ROOT / "src" / rel).resolve()
        if (ROOT / "src").resolve() not in src.parents or not src.is_file():
            raise RuntimeError(f"invalid or missing application source {rel}")
        recipe = recipes.get(rel)
        if recipe is None:
            # New sources inherit the nearest active directory's recipe for their language.
            compiler = "g++" if src.suffix in {".cc", ".cpp", ".cxx"} else "gcc"
            choices = [(name, r) for name, r in recipes.items() if r["compiler"] == compiler
                       and Path(name).suffix.lower() == src.suffix.lower()]
            if not choices:
                raise RuntimeError(f"no matching compiler recipe for new source {rel}")
            def score(pair):
                a, b = Path(rel).parts, Path(pair[0]).parts
                return next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
            recipe = max(choices, key=score)[1]
        obj = BUILD / "app" / (rel + ".o")
        dep = obj.with_suffix(".d")
        state = obj.with_suffix(".json")
        flags = [expand(x) for x in recipe["flags"]]
        if log_level is not None:
            flags.append(f"-DDIVOOM_APP_LOG_LEVEL={log_level}")
        cmd = [str(tc / "bin" / ("riscv64-unknown-elf-" + recipe["compiler"])), *flags,
               "-MMD", "-MF", str(dep), "-MT", str(obj), "-c", str(src), "-o", str(obj)]
        signature = hashlib.sha256(json.dumps(cmd).encode()).hexdigest()
        uptodate = False
        if not rebuild and obj.is_file() and dep.is_file() and state.is_file() and load(state)["command"] == signature:
            text = dep.read_text().replace("\\\n", " ")
            deps = shlex.split(text.split(":", 1)[1])
            uptodate = all(Path(p).is_file() and Path(p).stat().st_mtime_ns <= obj.stat().st_mtime_ns for p in deps)
        if uptodate:
            return obj, False
        obj.parent.mkdir(parents=True, exist_ok=True)
        run(cmd, BUILD / "logs/compile" / (rel + ".log"))
        safe_write(state, json.dumps({"command": signature}) + "\n")
        return obj, True

    objects, compiled = {}, 0
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = {pool.submit(one, s): s for s in sources}
        for n, future in enumerate(as_completed(futures), 1):
            obj, changed = future.result()
            objects[futures[future]] = obj
            compiled += changed
            if n % 50 == 0 or n == len(sources):
                print(f"[compile] {n}/{len(sources)} ({compiled} rebuilt)", flush=True)
    return [objects[s] for s in sources], compiled


def application_link_inputs(items, source_names, objects, expand):
    inputs = []
    object_map = dict(zip(source_names, map(str, objects)))
    original_names = {name for item in items if isinstance(item, dict)
                      for name in item["application_sources"]}
    for item in items:
        if isinstance(item, dict):
            inputs += [object_map[name] for name in item["application_sources"] if name in object_map]
            if item.get("include_extra"):
                inputs += [object_map[name] for name in source_names if name not in original_names]
        else:
            inputs += list(map(str, objects)) if item == "{APP_OBJECTS}" else [expand(item)]
    return inputs


def pack(binary, board):
    stage = BUILD / "pack"
    stage.mkdir(parents=True, exist_ok=True)
    for p in (SDK / "pack/template").iterdir():
        if p.is_file():
            shutil.copy2(p, stage / p.name)
    shutil.copy2(binary, stage / "freertos.fex")
    package_cfg = stage / "package_rtos.cfg"
    if 'item=rtos,' not in package_cfg.read_text().replace(" ", ""):
        raise RuntimeError("unsupported fixed RTOS package format")
    # dragonsecboot writes boot_package.fex; restore the fixed U-Boot package afterwards.
    cmd = [str(SDK / "pack/bin/dragonsecboot"), "-pack", "package_rtos.cfg"]
    with open(BUILD / "logs/pack-rtos.log", "wb") as f:
        subprocess.run(cmd, cwd=stage, stdout=f, stderr=subprocess.STDOUT, check=True)
    package = stage / "boot_package.fex"
    if not package.is_file():
        raise RuntimeError("RTOS packer did not produce a package")
    shutil.copy2(package, stage / "rtos_pkg_nor.fex")
    if package.stat().st_size > board["boot_partition_bytes"]:
        raise RuntimeError("RTOS package exceeds fixed bootA/bootB partitions")
    shutil.copy2(SDK / "pack/template/boot_package.fex", package)
    # Full production burn image, using frozen partition/boot/resource inputs.
    image = stage / board["image_name"]
    if image.exists():
        image.stat()
        image.unlink()
    with open(BUILD / "logs/pack-image.log", "wb") as f:
        subprocess.run([str(SDK / "pack/bin/dragon"), "image.cfg", "sys_partition_for_dragon.fex"],
                       cwd=stage, stdout=f, stderr=subprocess.STDOUT, check=True)
    if not image.is_file() or image.stat().st_size == 0:
        raise RuntimeError("full-image packer produced no image; see build/logs/pack-image.log")
    for p in (image, stage / "rtos_pkg_nor.fex"):
        shutil.copy2(p, OUT / p.name)
    with zipfile.ZipFile(OUT / "rtos_pkg_nor.zip", "w", compression=zipfile.ZIP_DEFLATED) as z:
        z.write(stage / "rtos_pkg_nor.fex", "rtos_pkg_nor.fex")
    return [OUT / image.name, OUT / "rtos_pkg_nor.fex", OUT / "rtos_pkg_nor.zip"]


def pack_debug(elf, binary, image):
    archive = OUT / "rt_system_debug.zip"
    with tempfile.TemporaryDirectory(prefix=".debug-", dir=OUT) as directory:
        staged = Path(directory) / archive.name
        with zipfile.ZipFile(staged, "w", compression=zipfile.ZIP_DEFLATED) as z:
            for path in (binary, elf, elf.with_suffix(".map"), elf.with_suffix(".syms")):
                z.write(path, path.name)
            z.writestr("README_DEBUG.txt",
                       "RTOS debugging artifacts for " + image.name + "\n"
                       "Use the matching .img file in PhoenixSuit. Keep this archive for debugging.\n")
        staged.replace(archive)
    return archive


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--jobs", "-j", type=int, default=min(os.cpu_count() or 2, 8))
    ap.add_argument("--verify-only", action="store_true")
    ap.add_argument("--rebuild", action="store_true", help="recompile all application translation units")
    ap.add_argument("--no-pack", action="store_true", help="build ELF/BIN only")
    ap.add_argument("--pack-only", action="store_true", help="pack an existing out/rt_system.bin")
    ap.add_argument("--clean", action="store_true", help="remove only this project's generated build and out directories")
    modes = ap.add_mutually_exclusive_group()
    modes.add_argument("--debug", dest="log_level", action="store_const", const="DIVOOM_LOG_LEVEL_DEBUG",
                       help="enable application debug logs and package symbols")
    modes.add_argument("--release", dest="log_level", action="store_const", const="DIVOOM_LOG_LEVEL_ERROR",
                       help="disable application state and debug logs and package symbols")
    args = ap.parse_args()
    if args.log_level is not None and (args.clean or args.verify_only or args.no_pack or args.pack_only):
        ap.error("firmware modes require a full build and package")
    if args.clean:
        for p in (BUILD, OUT):
            if p.exists():
                if p.is_symlink() or p.resolve().parent != ROOT:
                    raise RuntimeError(f"unsafe clean target {p}")
                list(p.iterdir())
                shutil.rmtree(p)
        print("[clean] generated build/ and out/ removed")
        return
    if args.jobs < 1:
        ap.error("jobs must be positive")
    if os.name != "posix" or sys.platform != "linux":
        ap.error("use Linux x86_64 or WSL2 Ubuntu (bundled Linux toolchain)")
    started = time.time()
    manifest = verify()
    if args.verify_only:
        return
    tc = ROOT / "toolchain" / manifest["toolchain"]
    def expand(s):
        return s.replace("{APP}", str(ROOT / "src")).replace("{SDK}", str(SDK)).replace("{TOOLCHAIN}", str(tc))
    actual_version = subprocess.check_output([str(tc / "bin/riscv64-unknown-elf-gcc"), "--version"], text=True).splitlines()[0]
    if actual_version != manifest["compiler_version"]:
        raise RuntimeError("toolchain version mismatch")
    board = load(SDK / "config/board.json")
    OUT.mkdir(exist_ok=True)
    (BUILD / "logs").mkdir(parents=True, exist_ok=True)
    elf, binary = OUT / "rt_system.elf", OUT / "rt_system.bin"
    compiled = 0
    if not args.pack_only:
        objects, compiled = compile_app(load(SDK / "config/recipes.json"), expand, tc,
                                        args.jobs, args.rebuild, args.log_level)
        link = load(SDK / "config/link.json")
        source_names = [s.strip() for s in (ROOT / "app_sources.txt").read_text().splitlines()
                        if s.strip() and not s.lstrip().startswith("#")]
        inputs = application_link_inputs(link["inputs"], source_names, objects, expand)
        cmd = [str(tc / "bin/riscv64-unknown-elf-gcc"), "-mcmodel=medany", "-mcpu=c907fdv-rv32",
               "-o", str(elf), "-T", str(SDK / "linker/freertos.lds"), "-Wl,-Map=" + str(OUT / "rt_system.map"),
               "-Wl,--whole-archive", *inputs, "-Wl,--no-whole-archive", *map(expand, link["vendor"]),
               *link["flags"], "-lm", "-lc", "-lgcc", "-lstdc++", "-Wl,--print-memory-usage",
               "-Wl,--start-group", "-Wl,--end-group", "-lc", "-lm", "-lgcc"]
        if link["crash_build_id"]:
            cmd += ["-Wl,--build-id=uuid"]
        # GCC response files also support projects installed in a path containing spaces.
        response = BUILD / "link.rsp"
        safe_write(response, "\n".join('"' + x.replace("\\", "\\\\").replace('"', '\\"') + '"' for x in cmd[1:]) + "\n")
        run([cmd[0], "@" + str(response)], BUILD / "logs/link.log")
        run([str(tc / "bin/riscv64-unknown-elf-objcopy"), str(elf), *link["objcopy_flags"], str(binary)], BUILD / "logs/objcopy.log")
        with open(OUT / "rt_system.syms", "wb") as f:
            subprocess.run([str(tc / "bin/riscv64-unknown-elf-nm"), "-n", str(elf)], stdout=f, check=True)
        print(f"[link] {binary.stat().st_size} bytes", flush=True)
    if not binary.is_file():
        raise RuntimeError("out/rt_system.bin missing")
    limit = min(board["read_buffer"], board["uboot_heap_start"])
    if board["load_address"] + binary.stat().st_size >= limit:
        raise RuntimeError("RTOS binary exceeds fixed boot memory layout")
    artifacts = [binary]
    if not args.no_pack:
        artifacts += pack(binary, board)
    if args.log_level is not None:
        artifacts.append(pack_debug(elf, binary, OUT / board["image_name"]))
    report = {"sdk_id": manifest["sdk_id"], "compiled_units": compiled, "elapsed_seconds": round(time.time() - started, 2),
              "artifacts": {p.relative_to(ROOT).as_posix(): {"bytes": p.stat().st_size, "sha256": digest(p)} for p in artifacts},
              "hardware_flash_test": "not performed"}
    safe_write(OUT / "build-report.json", json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print("[done] " + ", ".join(p.relative_to(ROOT).as_posix() for p in artifacts), flush=True)


if __name__ == "__main__":
    try:
        with operation_lock(ROOT):
            main()
    except (RuntimeError, subprocess.CalledProcessError, OSError) as e:
        print(f"ERROR: {e}", file=sys.stderr)
        sys.exit(1)
