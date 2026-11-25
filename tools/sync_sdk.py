#!/usr/bin/env python3
"""Refresh all fixed SDK assets from a built AstroToo tree, preserving src/."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from operation_lock import operation_lock

ROOT = Path(__file__).resolve().parents[1]
FIXED = ("sdk", "toolchain", "sdk.lock", "licenses/vendor", "verification/source_snapshot.json")
GENERATED = ("build", "out")
APPLICATION = ("src", "app_sources.txt")


def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def snapshot(path):
    if path.is_symlink():
        raise RuntimeError(f"symlink is not supported for synchronization: {path}")
    if not path.exists():
        return None
    if path.is_file():
        return {".": digest(path)}
    result = {"./": "directory"}
    for item in sorted(path.rglob("*")):
        if item.is_symlink():
            raise RuntimeError(f"symlink is not supported for synchronization: {item}")
        key = item.relative_to(path).as_posix()
        if item.is_dir():
            result[key + "/"] = "directory"
        elif item.is_file():
            result[key] = digest(item)
        else:
            raise RuntimeError(f"unexpected file type: {item}")
    return result


def assert_same(path, expected):
    if snapshot(path) != expected:
        raise RuntimeError(f"concurrent modification detected; retained without overwriting: {path}")


def write_json(path, value):
    # Reports are unique per run; never overwrite a user's report.
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8") as handle:
        json.dump(value, handle, ensure_ascii=False, indent=2)
        handle.write("\n")


def run(command, log):
    print("[sync] " + log.stem, flush=True)
    with log.open("x", encoding="utf-8") as handle:
        result = subprocess.run(command, stdout=handle, stderr=subprocess.STDOUT)
    if result.returncode:
        tail = log.read_text(errors="replace").splitlines()[-30:]
        raise RuntimeError(f"{log.stem} failed; log: {log}\n" + "\n".join(tail))


def move_new(source, destination):
    if destination.exists() or destination.is_symlink():
        raise RuntimeError(f"refusing to replace unexpected destination: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    source.rename(destination)


def install(root, candidate, backup, baseline, expected, validate, fixed=FIXED):
    """Replace only fixed assets; keep old SDK and generated outputs for rollback."""
    moved_old, installed, prepared_generated = [], [], []
    try:
        for rel in fixed + GENERATED:
            target = root / rel
            assert_same(target, baseline[rel])
            if target.exists():
                move_new(target, backup / rel)
                moved_old.append(rel)
            if rel in fixed:
                assert_same(candidate / rel, expected[rel])
                move_new(candidate / rel, target)
                installed.append(rel)
            else:
                prepared_generated.append(rel)
        validate()
        for rel in installed:
            assert_same(root / rel, expected[rel])
    except BaseException as failure:
        # Do not hide external edits made after installing the candidate SDK.
        conflicts = [rel for rel in installed if snapshot(root / rel) != expected[rel]]
        if conflicts:
            raise RuntimeError("rollback stopped to preserve concurrent SDK changes: " +
                               ", ".join(conflicts) + f"; original assets are in {backup}") from failure
        for rel in reversed(installed):
            assert_same(root / rel, expected[rel])
            move_new(root / rel, backup.parent / "rejected" / rel)
        # These directories were produced by the attempted build; preserve them too.
        for rel in prepared_generated:
            if (root / rel).exists():
                snapshot(root / rel)
                move_new(root / rel, backup.parent / "rejected" / rel)
        for rel in reversed(moved_old):
            move_new(backup / rel, root / rel)
        raise RuntimeError(f"update failed; previous SDK restored, diagnostics retained in {backup.parent}") from failure


def aligned_source_list(text, old_recipes, new_recipes):
    active = [line.strip() for line in text.splitlines() if line.strip() and not line.lstrip().startswith("#")]
    removed = [name for name in active if name in old_recipes and name not in new_recipes]
    added = [name for name in new_recipes if name not in active]
    lines = [line for line in text.splitlines() if line.strip() not in removed]
    return "\n".join(lines + added) + "\n", added, removed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=ROOT.with_name("AstroToo"),
                        help="already-built original project; default: sibling AstroToo")
    parser.add_argument("--jobs", "-j", type=int, default=min(os.cpu_count() or 2, 8))
    parser.add_argument("--check-only", action="store_true", help="export and build candidate without installing it")
    parser.add_argument("--align-source-list", action="store_true",
                        help="also align app_sources.txt to new SDK recipes; never modify src/")
    parser.add_argument("--freeze-app-source", action="append", default=[], metavar="PATH",
                        help="explicitly include a required compiled application module in the SDK instead of copying its source")
    args = parser.parse_args()
    if sys.platform != "linux":
        parser.error("run under Linux x86_64 or WSL2 Ubuntu")
    if args.jobs < 1:
        parser.error("jobs must be positive")
    source = args.source.resolve()
    if source == ROOT or source in ROOT.parents or ROOT in source.parents:
        parser.error("source and destination must be separate trees")
    if not (source / "lichee/rtos/build/f101s3_astrotoo/img/rt_system.bin").is_file():
        parser.error("source has no completed RTOS build; build and pack the original project first")
    for rel in APPLICATION + ("sdk", "toolchain", "sdk.lock"):
        if not (ROOT / rel).exists():
            parser.error(f"missing project input: {rel}")
    run_dir = ROOT.parent / (ROOT.name + "_maintenance") / "sdk-sync" / datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    run_dir.mkdir(parents=True, exist_ok=False)
    candidate, backup = run_dir / "candidate", run_dir / "backup"
    report = {"source": str(source), "destination": str(ROOT), "run_directory": str(run_dir),
              "application_synchronized": False, "hardware_flash_test": "not performed"}
    fixed = FIXED + (("app_sources.txt",) if args.align_source_list else ())
    try:
        print("[sync] snapshot current application and fixed assets", flush=True)
        app_before = {rel: snapshot(ROOT / rel) for rel in APPLICATION}
        baseline = {rel: snapshot(ROOT / rel) for rel in fixed + GENERATED}
        helpers = {rel: snapshot(ROOT / "tools" / rel)
                   for rel in ("build_app.py", "export_sdk.py", "operation_lock.py", "sync_sdk.py")}
        candidate.joinpath("tools").mkdir(parents=True)
        for name in ("build_app.py", "operation_lock.py"):
            shutil.copy2(ROOT / "tools" / name, candidate / "tools" / name)
            assert_same(candidate / "tools" / name, helpers[name])
        run([sys.executable, str(ROOT / "tools/build_app.py"), "--verify-only"], run_dir / "verify-current.log")
        previous_frozen = ROOT / "sdk/config/frozen_application.json"
        frozen = set(json.loads(previous_frozen.read_text()) if previous_frozen.is_file() else [])
        frozen.update(args.freeze_app_source)
        report["frozen_application_sources"] = sorted(frozen)
        export_command = [sys.executable, str(ROOT / "tools/export_sdk.py"), "--sdk-only", "--source", str(source),
                          "--output", str(candidate)]
        for name in sorted(frozen):
            export_command += ["--freeze-app-source", name]
        run(export_command, run_dir / "export-sdk.log")
        if (candidate / "src").exists() or (candidate / "app_sources.txt").exists():
            raise RuntimeError("SDK-only export unexpectedly wrote application sources")
        # Validate using a copy of the destination application, not the source project's app.
        shutil.copytree(ROOT / "src", candidate / "src", symlinks=False)
        shutil.copy2(ROOT / "app_sources.txt", candidate / "app_sources.txt")
        for rel in APPLICATION:
            assert_same(candidate / rel, app_before[rel])
            assert_same(ROOT / rel, app_before[rel])
        if args.align_source_list:
            source_list = candidate / "app_sources.txt"
            text = source_list.read_text(encoding="utf-8")
            old_recipes = json.loads((ROOT / "sdk/config/recipes.json").read_text())
            new_recipes = json.loads((candidate / "sdk/config/recipes.json").read_text())
            updated, added, removed = aligned_source_list(text, old_recipes, new_recipes)
            assert_same(source_list, app_before["app_sources.txt"])
            source_list.write_text(updated, encoding="utf-8")
            report["source_list_changes"] = {"added": added, "removed": removed}
            print(f"[sync] candidate source list: {len(added)} added, {len(removed)} removed; src/ retained", flush=True)
        run([sys.executable, str(candidate / "tools/build_app.py"), "--jobs", str(args.jobs)],
            run_dir / "validate-candidate.log")
        expected = {rel: snapshot(candidate / rel) for rel in fixed}
        for rel, value in helpers.items():
            assert_same(ROOT / "tools" / rel, value)
        for rel in APPLICATION:
            assert_same(ROOT / rel, app_before[rel])
        for rel in fixed + GENERATED:
            assert_same(ROOT / rel, baseline[rel])
        report["candidate_build"] = json.loads((candidate / "out/build-report.json").read_text())
        report["candidate_sdk_id"] = report["candidate_build"]["sdk_id"]
        report["candidate_validated"] = True
        if args.check_only:
            report["status"] = "checked_without_installing"
        else:
            print("[sync] install validated SDK; preserve previous SDK and outputs in backup", flush=True)

            def validate_installed():
                run([sys.executable, str(ROOT / "tools/build_app.py"), "--jobs", str(args.jobs)],
                    run_dir / "build-installed.log")
                for rel in APPLICATION:
                    value = expected[rel] if rel in expected else app_before[rel]
                    assert_same(ROOT / rel, value)

            install(ROOT, candidate, backup, baseline, expected, validate_installed, fixed=fixed)
            report["status"] = "installed"
            report["backup"] = str(backup)
            report["final_build"] = json.loads((ROOT / "out/build-report.json").read_text())
        report["application_files_unchanged"] = True
        report["source_list_unchanged"] = (args.check_only or not args.align_source_list or
                                         expected["app_sources.txt"] == app_before["app_sources.txt"])
        write_json(run_dir / "result.json", report)
        print(f"[sync] {report['status']}: {report['candidate_sdk_id']}; src/ unchanged", flush=True)
        print(f"[sync] report: {run_dir / 'result.json'}", flush=True)
    except BaseException as error:
        report["status"] = "failed"
        report["error"] = str(error)
        write_json(run_dir / "failure.json", report)
        raise


if __name__ == "__main__":
    try:
        with operation_lock(ROOT):
            main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
