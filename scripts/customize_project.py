#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Iterable

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_METADATA_PATH = ROOT / "project.metadata.json"
LEGACY_METADATA_PATH = ROOT / "scaffold.metadata.json"
BACKUP_ROOT = ROOT / ".scaffold-backups"
LATEST_MANIFEST = BACKUP_ROOT / "latest_manifest.json"
DEFAULT_REPORT = BACKUP_ROOT / "last_customize_report.json"
DEFAULT_PLACEHOLDERS = {
    "SignalEditor": "project_name",
    "Signal Editor": "project_name",
    "myprj": "namespace_prefix",
    "SIGNAL_EDITOR": "macro_prefix",
    "my_module": "primary_module",
}
REQUIRED_KEYS = (
    "project_name",
    "project_slug",
    "namespace_prefix",
    "macro_prefix",
    "primary_module",
    "description",
    "vendor",
)
TEXT_SUFFIXES = {
    ".bat", ".cmake", ".cpp", ".h", ".hpp", ".in", ".json", ".md", ".py", ".schema", ".txt", ".yml", ".yaml"
}
TEXT_FILENAMES = {"CMakeLists.txt"}
SKIP_DIR_NAMES = {
    ".git", ".idea", ".vs", ".scaffold-backups", "build", "cmake-build-debug", "cmake-build-release", "out", "pyvenv", "__pycache__"
}
SKIP_TOP_LEVEL_PREFIXES = ("cmake-build-",)
SKIP_FILES = {
    "project.metadata.json",
    "scaffold.metadata.json",
    "customize_project.py",
    "customize_project.bat",
}


@dataclass(frozen=True)
class Change:
    kind: str
    path: Path
    target: Path | None = None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Customize the scaffold using metadata from project.metadata.json")
    parser.add_argument("--metadata", type=Path, default=DEFAULT_METADATA_PATH, help="Path to project metadata JSON")
    parser.add_argument("--apply", action="store_true", help="Apply changes. Without this flag the script performs a dry run.")
    parser.add_argument("--dry-run", action="store_true", help="Force dry-run mode even if --apply is provided.")
    parser.add_argument("--rollback", type=Path, help="Rollback a previous customization using the given manifest path.")
    parser.add_argument("--report-json", type=Path, default=DEFAULT_REPORT, help="Write a JSON report to this path.")
    return parser.parse_args()


def write_report(path: Path | None, payload: dict) -> None:
    if path is None:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def resolve_metadata_path(path: Path) -> Path:
    if path.exists():
        return path
    if path == DEFAULT_METADATA_PATH and LEGACY_METADATA_PATH.exists():
        print(f"[WARN] Falling back to legacy metadata file: {LEGACY_METADATA_PATH}")
        return LEGACY_METADATA_PATH
    return path


def load_metadata(path: Path) -> dict[str, Any]:
    path = resolve_metadata_path(path)
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"[ERROR] Metadata file not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"[ERROR] Invalid JSON in metadata file {path}: {exc}") from exc

    missing = [key for key in REQUIRED_KEYS if not str(data.get(key, "")).strip()]
    if missing:
        raise SystemExit(f"[ERROR] Missing required metadata keys: {', '.join(missing)}")

    return data


def validate_metadata(metadata: dict[str, Any]) -> None:
    values = {key: str(metadata[key]).strip() for key in REQUIRED_KEYS}
    for key, value in values.items():
        if any(ch in value for ch in ('/', '\\')) and key in {"project_name", "project_slug", "namespace_prefix", "macro_prefix", "primary_module"}:
            raise SystemExit(f"[ERROR] Metadata field {key} must not contain path separators: {value}")
    if values["macro_prefix"] != values["macro_prefix"].upper():
        raise SystemExit("[ERROR] macro_prefix must be uppercase, e.g. SIGNAL_EDITOR")
    if not values["namespace_prefix"].islower():
        raise SystemExit("[ERROR] namespace_prefix must be lowercase, e.g. myprj")
    if not values["primary_module"].islower():
        raise SystemExit("[ERROR] primary_module must be lowercase, e.g. my_module")
    if not re.fullmatch(r"[a-z0-9]+(?:[a-z0-9_-]*[a-z0-9])?", values["project_slug"]):
        raise SystemExit("[ERROR] project_slug must be lowercase and filesystem-friendly, e.g. bin-tools or bin_tools")


def should_skip_dir(path: Path) -> bool:
    return any(part in SKIP_DIR_NAMES for part in path.parts) or any(path.parts and path.parts[0].startswith(prefix) for prefix in SKIP_TOP_LEVEL_PREFIXES)


def is_text_candidate(path: Path) -> bool:
    return path.name in TEXT_FILENAMES or path.suffix.lower() in TEXT_SUFFIXES


def replacement_pairs(metadata: dict[str, Any]) -> list[tuple[str, str]]:
    pairs = [(placeholder, str(metadata[key]).strip()) for placeholder, key in DEFAULT_PLACEHOLDERS.items()]
    return sorted(pairs, key=lambda item: len(item[0]), reverse=True)


def iter_candidate_files(root: Path) -> Iterable[Path]:
    for path in root.rglob('*'):
        if not path.is_file():
            continue
        rel = path.relative_to(root)
        if should_skip_dir(rel):
            continue
        if path.name in SKIP_FILES:
            continue
        if is_text_candidate(path):
            yield path


def replace_text(content: str, pairs: list[tuple[str, str]]) -> tuple[str, bool]:
    updated = content
    changed = False
    for old, new in pairs:
        if old in updated:
            updated = updated.replace(old, new)
            changed = True
    return updated, changed


def replace_path_parts(rel_path: Path, pairs: list[tuple[str, str]]) -> Path:
    replaced_parts: list[str] = []
    for part in rel_path.parts:
        new_part = part
        for old, new in pairs:
            if old in new_part:
                new_part = new_part.replace(old, new)
        replaced_parts.append(new_part)
    return Path(*replaced_parts)


def detect_default_metadata(metadata: dict[str, Any]) -> bool:
    return all(str(metadata[key]).strip() == placeholder for placeholder, key in DEFAULT_PLACEHOLDERS.items())


def collect_plan(metadata: dict[str, Any]) -> tuple[list[tuple[Path, str]], list[tuple[Path, Path]], list[Change]]:
    pairs = replacement_pairs(metadata)
    file_updates: list[tuple[Path, str]] = []
    renames: list[tuple[Path, Path]] = []
    change_log: list[Change] = []

    for path in iter_candidate_files(ROOT):
        rel = path.relative_to(ROOT)
        original = path.read_text(encoding="utf-8")
        updated, changed = replace_text(original, pairs)
        if changed:
            file_updates.append((path, updated))
            change_log.append(Change("update", rel))

        new_rel = replace_path_parts(rel, pairs)
        if new_rel != rel:
            renames.append((path, ROOT / new_rel))
            change_log.append(Change("rename", rel, new_rel))

    return file_updates, renames, change_log


def detect_remaining_placeholders() -> list[str]:
    remaining: list[str] = []
    placeholders = tuple(DEFAULT_PLACEHOLDERS.keys())
    for path in iter_candidate_files(ROOT):
        rel = path.relative_to(ROOT)
        text = path.read_text(encoding="utf-8")
        hits = [placeholder for placeholder in placeholders if placeholder in text]
        if hits:
            remaining.append(f"{rel}: {', '.join(hits)}")
    return remaining


def validate_plan_conflicts(renames: list[tuple[Path, Path]]) -> None:
    rename_targets = [target for _, target in renames]
    if len(rename_targets) != len(set(rename_targets)):
        raise SystemExit("[ERROR] Rename collision detected in planned path changes.")
    for source, target in renames:
        if target.exists() and target != source:
            raise SystemExit(f"[ERROR] Rename target already exists: {target.relative_to(ROOT)}")


def create_backup(file_updates: list[tuple[Path, str]], renames: list[tuple[Path, Path]], metadata_path: Path) -> Path:
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    backup_dir = BACKUP_ROOT / timestamp
    files_dir = backup_dir / "files"
    files_dir.mkdir(parents=True, exist_ok=False)

    backup_map: dict[Path, Path] = {}
    for path, _ in file_updates:
        backup_map[path] = files_dir / path.relative_to(ROOT)
    for source, _ in renames:
        backup_map.setdefault(source, files_dir / source.relative_to(ROOT))

    for source, target in backup_map.items():
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)

    manifest = {
        "created_at_utc": timestamp,
        "root": str(ROOT),
        "metadata": str(metadata_path),
        "backed_up_files": [str(path.relative_to(ROOT)) for path in sorted(backup_map)],
        "updates": [str(path.relative_to(ROOT)) for path, _ in file_updates],
        "renames": [
            {
                "from": str(source.relative_to(ROOT)),
                "to": str(target.relative_to(ROOT)),
            }
            for source, target in renames
        ],
    }
    manifest_path = backup_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    LATEST_MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    LATEST_MANIFEST.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest_path


def rollback_from_manifest(manifest_path: Path, report_path: Path | None) -> int:
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        print(f"[ERROR] Rollback manifest not found: {manifest_path}")
        return 1
    except json.JSONDecodeError as exc:
        print(f"[ERROR] Invalid rollback manifest {manifest_path}: {exc}")
        return 1

    if manifest_path.name == LATEST_MANIFEST.name and manifest.get("created_at_utc"):
        backup_root = manifest_path.parent / manifest["created_at_utc"] / "files"
    else:
        backup_root = manifest_path.parent / "files"
    renames = manifest.get("renames", [])
    backed_up = [Path(item) for item in manifest.get("backed_up_files", [])]

    print(f"[INFO] Rolling back customization from manifest: {manifest_path}")

    for entry in reversed(renames):
        source = ROOT / entry["from"]
        target = ROOT / entry["to"]
        if target.exists():
            source.parent.mkdir(parents=True, exist_ok=True)
            if source.exists() and source != target:
                if source.is_file():
                    source.unlink()
                else:
                    shutil.rmtree(source)
            target.rename(source)
            cleanup_parent = target.parent
            while cleanup_parent != ROOT and cleanup_parent.exists() and not any(cleanup_parent.iterdir()):
                cleanup_parent.rmdir()
                cleanup_parent = cleanup_parent.parent

    for rel in backed_up:
        backup_file = backup_root / rel
        target = ROOT / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(backup_file, target)

    report = {
        "mode": "rollback",
        "manifest": str(manifest_path),
        "restored_files": [str(path) for path in backed_up],
        "restored_renames": renames,
    }
    write_report(report_path, report)
    print("[OK] Rollback completed.")
    return 0


def main() -> int:
    args = parse_args()

    if args.rollback:
        return rollback_from_manifest(args.rollback, args.report_json)

    metadata = load_metadata(args.metadata)
    validate_metadata(metadata)

    if detect_default_metadata(metadata):
        report = {
            "mode": "noop",
            "reason": "metadata_contains_default_placeholders",
            "metadata": str(args.metadata),
        }
        write_report(args.report_json, report)
        print("[WARN] Metadata still contains default project placeholder values. Update project.metadata.json before applying customization.")
        print("[INFO] No changes applied.")
        return 0

    pairs = replacement_pairs(metadata)
    dry_run = (not args.apply) or args.dry_run
    file_updates, renames, change_log = collect_plan(metadata)

    print(f"[INFO] Metadata file: {args.metadata}")
    print(f"[INFO] Mode: {'dry-run' if dry_run else 'apply'}")
    print("[INFO] Replacement plan:")
    for old, new in pairs:
        print(f"  - {old} -> {new}")

    if not change_log:
        report = {
            "mode": "dry-run" if dry_run else "apply",
            "metadata": str(args.metadata),
            "changes": [],
            "message": "no_changes_detected",
        }
        write_report(args.report_json, report)
        print("[INFO] No project placeholder occurrences were found in eligible files.")
        return 0

    validate_plan_conflicts(renames)

    print("[INFO] Planned changes:")
    for change in change_log:
        if change.kind == "update":
            print(f"  - update {change.path}")
        else:
            print(f"  - rename {change.path} -> {change.target}")

    report = {
        "mode": "dry-run" if dry_run else "apply",
        "metadata": str(args.metadata),
        "replacements": [{"from": old, "to": new} for old, new in pairs],
        "updates": [str(path.relative_to(ROOT)) for path, _ in file_updates],
        "renames": [{"from": str(src.relative_to(ROOT)), "to": str(dst.relative_to(ROOT))} for src, dst in renames],
    }

    if dry_run:
        report["status"] = "planned"
        write_report(args.report_json, report)
        print("[INFO] Dry-run complete. Re-run with --apply to perform the customization.")
        print(f"[INFO] JSON report written to: {args.report_json}")
        return 0

    manifest_path = create_backup(file_updates, renames, args.metadata)
    report["backup_manifest"] = str(manifest_path)

    for path, updated in file_updates:
        path.write_text(updated, encoding="utf-8")

    for source, target in sorted(renames, key=lambda item: len(item[0].parts), reverse=True):
        target.parent.mkdir(parents=True, exist_ok=True)
        source.rename(target)
        parent = source.parent
        while parent != ROOT and not any(parent.iterdir()):
            parent.rmdir()
            parent = parent.parent

    remaining = detect_remaining_placeholders()
    report["remaining_placeholders"] = remaining
    report["status"] = "applied" if not remaining else "applied_with_remaining_placeholders"
    write_report(args.report_json, report)

    if remaining:
        print("[ERROR] Customization applied, but critical project placeholders still remain in managed files:")
        for item in remaining:
            print(f"  - {item}")
        print(f"[INFO] Review the report and use rollback if needed: {manifest_path}")
        return 1

    print("[OK] Project customization applied successfully.")
    print(f"[INFO] Backup manifest: {manifest_path}")
    print(f"[INFO] JSON report written to: {args.report_json}")
    print("[INFO] Review the modified files and run host-appropriate build/test presets next.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
