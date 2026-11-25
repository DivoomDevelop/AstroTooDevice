#!/usr/bin/env python3
"""Integration checks for frozen SDK + editable application builds (no flashing)."""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def sha(p):
    h = hashlib.sha256()
    with p.open("rb") as f:
        for b in iter(lambda: f.read(1024 * 1024), b""):
            h.update(b)
    return h.hexdigest()


def write_report(p, value):
    if p.exists():
        p.read_bytes()
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def build(label, *args, expected=0):
    log = ROOT / "verification" / (label + ".log")
    if log.exists():
        log.read_bytes()
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("wb") as f:
        result = subprocess.run([sys.executable, "-u", str(ROOT / "tools/build_app.py"), *args],
                                cwd=ROOT, stdout=f, stderr=subprocess.STDOUT)
    if result.returncode != expected:
        raise RuntimeError(f"{label}: exit {result.returncode}, expected {expected}\n" + log.read_text(errors="replace")[-6000:])
    print("[test] " + label + " OK", flush=True)
    return log.read_text(errors="replace")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source-root", type=Path, help="read-only check against export source snapshot")
    args = ap.parse_args()
    results = {}
    report_file = ROOT / "out/build-report.json"
    if not report_file.is_file():
        raise RuntimeError("run a complete build first")
    baseline = json.loads(report_file.read_text())
    write_report(ROOT / "verification/baseline-build-report.json", baseline)
    build("incremental-unchanged", "--no-pack")
    incremental = json.loads(report_file.read_text())
    assert incremental["compiled_units"] == 0, incremental
    results["unchanged_application_rebuilt_units"] = 0

    config = ROOT / "sdk/config/board.json"
    original = config.read_bytes()
    changed = original + b"\n"
    config.write_bytes(changed)
    try:
        log = build("reject-modified-sdk", "--verify-only", expected=1)
        assert "fixed SDK file missing or modified: sdk/config/board.json" in log
        results["modified_sdk_rejected"] = True
    finally:
        if config.read_bytes() != changed:
            raise RuntimeError("SDK changed externally during test; leaving external content intact")
        config.write_bytes(original)

    source_list = ROOT / "app_sources.txt"
    original_list = source_list.read_bytes()
    probe = ROOT / "src/app/astrotoo_open_build_probe.c"
    if probe.exists():
        raise RuntimeError("probe file already exists; refusing to overwrite")
    marker = b"ASTROTOO_OPEN_APPLICATION_SMOKE_20261013"
    probe_content = b'const char astrotoo_open_build_probe[] __attribute__((used, retain)) = "' + marker + b'";\n'
    changed_list = original_list.rstrip(b"\r\n") + b"\napp/astrotoo_open_build_probe.c\n"
    probe.write_bytes(probe_content)
    # Read again before changing a user-editable file.
    if source_list.read_bytes() != original_list:
        raise RuntimeError("source list changed externally")
    source_list.write_bytes(changed_list)
    try:
        build("new-application-source", "--no-pack")
        assert marker in (ROOT / "out/rt_system.bin").read_bytes()
        assert json.loads(report_file.read_text())["compiled_units"] == 1
        results["new_application_source_linked"] = True
    finally:
        if source_list.read_bytes() != changed_list or probe.read_bytes() != probe_content:
            raise RuntimeError("application changed externally during test; preserving external edits")
        source_list.write_bytes(original_list)
        probe.unlink()

    build("final-clean-application", "--jobs", "8")
    binary = (ROOT / "out/rt_system.bin").read_bytes()
    assert marker not in binary
    package_path = ROOT / "out/rtos_pkg_nor.fex"
    package = package_path.read_bytes()
    assert struct.unpack_from("<I", package, 16)[0] == 0x89119800
    stored_sum = struct.unpack_from("<I", package, 20)[0]
    items, valid_len = struct.unpack_from("<II", package, 32)
    assert 0 < items < 16 and valid_len <= len(package) and valid_len % 4 == 0
    checksum = (sum(struct.unpack_from("<" + "I" * (valid_len // 4), package)) - stored_sum + 0x5F0A6C39) & 0xffffffff
    assert checksum == stored_sum, (hex(checksum), hex(stored_sum))
    contents = {}
    for i in range(items):
        base = 64 + 368 * i
        name = package[base:base + 64].split(b"\0")[0].decode("ascii")
        offset, length = struct.unpack_from("<II", package, base + 64)
        assert offset + length <= valid_len
        contents[name] = package[offset:offset + length]
    assert contents["rtos"] == binary
    assert contents["melis-config"] == (ROOT / "sdk/pack/template/sys_config.bin").read_bytes()
    results["rtos_package_checksum_and_payload"] = True

    board = json.loads((ROOT / "sdk/config/board.json").read_text())
    full = ROOT / "out" / board["image_name"]
    with full.open("rb") as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as mm:
        assert mm[:8] == b"IMAGEWTY"
        assert mm.find(package) >= 0, "full flash image does not contain freshly linked RTOS package"
        assert mm.find((ROOT / "sdk/pack/template/boot_package.fex").read_bytes()) >= 0
    results["full_image_contains_new_rtos_and_fixed_bootloader"] = True

    archives = list((ROOT / "sdk/lib").rglob("*.a"))
    for p in archives:
        with p.open("rb") as f:
            assert f.read(8) == b"!<arch>\n", str(p)
    results["standalone_regular_archives"] = len(archives)
    sdk_sources = [str(p.relative_to(ROOT)) for p in (ROOT / "sdk/sysroot").rglob("*")
                   if p.is_file() and p.suffix in (".c", ".cpp", ".cc", ".S", ".s")]
    assert not sdk_sources, sdk_sources
    results["platform_implementation_sources_in_sdk"] = 0
    depfiles = list((ROOT / "build/app").rglob("*.d"))
    if args.source_root:
        old = str(args.source_root.resolve()) + "/"
        assert all(old not in p.read_text() for p in depfiles), "application depends on original source tree"
        snapshot = json.loads((ROOT / "verification/source_snapshot.json").read_text())
        changed = [rel for rel, h in snapshot.items() if sha(args.source_root / rel) != h]
        if changed:
            raise RuntimeError("original source changed since export: " + ", ".join(changed[:10]))
        results["original_input_files_unchanged"] = len(snapshot)
        results["original_tree_compile_dependencies"] = 0
    results["hardware_flash_test"] = "not performed"
    results["final_build"] = json.loads(report_file.read_text())
    write_report(ROOT / "verification/integration-results.json", results)
    print(json.dumps(results, ensure_ascii=False, indent=2), flush=True)


if __name__ == "__main__":
    main()
