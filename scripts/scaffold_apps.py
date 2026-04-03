#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_METADATA_PATH = ROOT / "project.metadata.json"
DEFAULT_REPORT_PATH = ROOT / ".scaffold-backups" / "last_scaffold_apps_report.json"
TEXT_SUFFIXES = {
    ".bat", ".cmake", ".cpp", ".h", ".hpp", ".in", ".json", ".md", ".py", ".schema", ".txt", ".yml", ".yaml"
}
TEXT_FILENAMES = {"CMakeLists.txt"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Create additional app/module scaffolds from project metadata.")
    parser.add_argument("--metadata", type=Path, default=DEFAULT_METADATA_PATH, help="Path to project metadata JSON")
    parser.add_argument("--report-json", type=Path, default=DEFAULT_REPORT_PATH, help="Write a JSON report to this path")
    return parser.parse_args()


def write_report(path: Path | None, payload: dict[str, Any]) -> None:
    if path is None:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def load_metadata(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"[ERROR] Metadata file not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"[ERROR] Invalid JSON in metadata file {path}: {exc}") from exc

    if not str(data.get("primary_module", "")).strip():
        raise SystemExit("[ERROR] Missing required metadata key: primary_module")
    return data


def validate_module_name(module: str, field_name: str) -> None:
    if not re.fullmatch(r"[a-z][a-z0-9_]*", module):
        raise SystemExit(f"[ERROR] {field_name} must be snake_case, e.g. my_module: {module}")


def normalize_apps(metadata: dict[str, Any]) -> list[dict[str, Any]]:
    raw_apps = metadata.get("apps", [])
    if raw_apps in (None, ""):
        return []
    if not isinstance(raw_apps, list):
        raise SystemExit("[ERROR] metadata field apps must be an array of objects.")

    normalized: list[dict[str, Any]] = []
    seen_modules: set[str] = set()
    for index, entry in enumerate(raw_apps, start=1):
        if isinstance(entry, str):
            module = entry.strip()
            cli_enabled = True
            gui_enabled = True
        elif isinstance(entry, dict):
            module = str(entry.get("module", "")).strip()
            cli_enabled = bool(entry.get("cli", True))
            gui_enabled = bool(entry.get("gui", True))
        else:
            raise SystemExit(f"[ERROR] apps[{index}] must be either a module name string or an object.")

        validate_module_name(module, f"apps[{index}].module")
        if module in seen_modules:
            raise SystemExit(f"[ERROR] Duplicate apps entry for module: {module}")
        if not cli_enabled and not gui_enabled:
            raise SystemExit(f"[ERROR] apps[{index}] must enable at least one delivery: {module}")
        normalized.append({"module": module, "cli": cli_enabled, "gui": gui_enabled})
        seen_modules.add(module)
    return normalized


def module_to_display_name(module: str) -> str:
    return "".join(part.capitalize() for part in module.split("_"))


def is_text_candidate(path: Path) -> bool:
    return path.name in TEXT_FILENAMES or path.suffix.lower() in TEXT_SUFFIXES


def replace_module_tokens(content: str, source_module: str, target_module: str) -> str:
    return content.replace(module_to_display_name(source_module), module_to_display_name(target_module)).replace(source_module, target_module)


def copy_tree_with_replacements(source: Path, target: Path, source_module: str, target_module: str) -> list[str]:
    created: list[str] = []
    for path in source.rglob("*"):
        rel = path.relative_to(source)
        target_rel = Path(*[replace_module_tokens(part, source_module, target_module) for part in rel.parts])
        target_path = target / target_rel
        if path.is_dir():
            target_path.mkdir(parents=True, exist_ok=True)
            continue

        target_path.parent.mkdir(parents=True, exist_ok=True)
        if is_text_candidate(path):
            updated = replace_module_tokens(path.read_text(encoding="utf-8"), source_module, target_module)
            target_path.write_text(updated, encoding="utf-8")
        else:
            shutil.copy2(path, target_path)
        created.append(str(target_path.relative_to(ROOT)))
    return created


def ensure_module_catalog(apps: list[dict[str, Any]]) -> None:
    catalog_path = ROOT / "deploy" / "config" / "module_catalog.json"
    if not catalog_path.exists():
        return

    modules: list[dict[str, Any]] = []
    for app in apps:
        module = app["module"]
        entry: dict[str, Any] = {
            "id": module,
            "display_name": module_to_display_name(module),
            "version": "0.1.0",
            "description": f"TODO: describe this module ({module})",
        }
        if app["cli"]:
            entry["executables"] = {
                "windows": [f"{module}.exe"],
                "linux": [module],
            }
        if app["gui"]:
            entry["gui_executables"] = {
                "windows": [f"{module}-gui.exe"]
            }
        modules.append(entry)

    payload = {
        "$schema": "../schemas/module_catalog.schema.json",
        "version": "1.0",
        "modules": modules,
    }
    catalog_path.write_text(json.dumps(payload, indent=4) + "\n", encoding="utf-8")


def scaffold_additional_apps(metadata: dict[str, Any]) -> dict[str, Any]:
    apps = normalize_apps(metadata)
    if not apps:
        return {"status": "noop", "message": "no_apps_requested", "created": [], "skipped": []}

    primary_module = str(metadata["primary_module"]).strip()
    validate_module_name(primary_module, "primary_module")

    source_module = "my_module" if (ROOT / "src" / "my_module").exists() else primary_module
    include_root_name = "myprj"
    namespace_prefix = str(metadata.get("namespace_prefix", "")).strip()
    if namespace_prefix and (ROOT / "include" / namespace_prefix).exists():
        include_root_name = namespace_prefix
    elif (ROOT / "include" / "myprj").exists():
        include_root_name = "myprj"

    source_roots = {
        "src": ROOT / "src" / source_module,
        "apps": ROOT / "apps" / source_module,
        "tests": ROOT / "tests" / "03.unit_test" / source_module,
        "spec": ROOT / "docs" / "specs" / f"{source_module}_srs.md",
        "version_header": ROOT / "include" / include_root_name / f"{source_module}_version.h.in",
    }

    missing_sources = [str(path.relative_to(ROOT)) for path in source_roots.values() if not path.exists()]
    if missing_sources:
        raise SystemExit(f"[ERROR] Module scaffold template is incomplete. Missing: {', '.join(missing_sources)}")

    created: list[str] = []
    skipped: list[dict[str, str]] = []

    for app in apps:
        module = app["module"]
        if module == primary_module and source_module == "my_module":
            skipped.append({"module": module, "reason": "provided by template rename during customization"})
            continue
        if module == source_module:
            skipped.append({"module": module, "reason": "already present in scaffold"})
            continue

        target_paths = [
            ROOT / "src" / module,
            ROOT / "apps" / module,
            ROOT / "tests" / "03.unit_test" / module,
            ROOT / "docs" / "specs" / f"{module}_srs.md",
            ROOT / "include" / include_root_name / f"{module}_version.h.in",
        ]
        already_existing = [str(path.relative_to(ROOT)) for path in target_paths if path.exists()]
        if already_existing:
            skipped.append({"module": module, "reason": "target already exists"})
            continue

        created.extend(copy_tree_with_replacements(source_roots["src"], ROOT / "src" / module, source_module, module))
        created.extend(copy_tree_with_replacements(source_roots["apps"], ROOT / "apps" / module, source_module, module))
        created.extend(copy_tree_with_replacements(source_roots["tests"], ROOT / "tests" / "03.unit_test" / module, source_module, module))

        spec_target = ROOT / "docs" / "specs" / f"{module}_srs.md"
        spec_target.parent.mkdir(parents=True, exist_ok=True)
        spec_target.write_text(
            replace_module_tokens(source_roots["spec"].read_text(encoding="utf-8"), source_module, module),
            encoding="utf-8",
        )
        created.append(str(spec_target.relative_to(ROOT)))

        version_target = ROOT / "include" / include_root_name / f"{module}_version.h.in"
        version_target.parent.mkdir(parents=True, exist_ok=True)
        version_target.write_text(
            replace_module_tokens(source_roots["version_header"].read_text(encoding="utf-8"), source_module, module),
            encoding="utf-8",
        )
        created.append(str(version_target.relative_to(ROOT)))

        if not app["cli"]:
            shutil.rmtree(ROOT / "apps" / module / "cli")
        if not app["gui"]:
            shutil.rmtree(ROOT / "apps" / module / "gui")

    ensure_module_catalog(apps)
    return {"status": "ok", "created": created, "skipped": skipped, "apps": apps}


def main() -> int:
    args = parse_args()
    metadata = load_metadata(args.metadata)
    result = scaffold_additional_apps(metadata)
    write_report(args.report_json, result)

    if result["status"] == "noop":
        print("[INFO] No additional apps requested in project metadata.")
        return 0

    for item in result["skipped"]:
        print(f"[INFO] Skipped module {item['module']}: {item['reason']}")
    if result["created"]:
        print("[OK] Additional app scaffolds created.")
    else:
        print("[INFO] No new app scaffolds were created.")
    print(f"[INFO] JSON report written to: {args.report_json}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
