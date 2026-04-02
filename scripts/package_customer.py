#!/usr/bin/env python3
"""
package_customer.py
Assembles a customer-specific deployment package from build artifacts,
driven by a customer profile JSON and a module catalog JSON.

Usage:
    python package_customer.py
        --profile  <path/to/customer.json>
        --catalog  <path/to/module_catalog.json>
        --platform <windows|linux-ubuntu|linux-alpine>
        --deploy-root <path/to/build/artifacts>
        --configuration <debug|release>
        --output-dir <path/to/deploy/out>
        [--output-layout customer_platform_config]
        [--no-archive]
"""

import argparse
import json
import shutil
import sys
from pathlib import Path


def load_json(path: Path) -> dict:
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def build_output_dir(output_dir: Path, customer_id: str, platform: str, config: str, layout: str) -> Path:
    if layout == "customer_platform_config":
        return output_dir / customer_id / platform / config
    return output_dir / customer_id


def copy_module_artifacts(module_id: str, catalog: dict, deploy_root: Path, dest: Path, platform: str) -> list[str]:
    """Copy executables for a module from deploy_root to dest. Returns list of copied files."""
    copied = []
    module = next((m for m in catalog.get("modules", []) if m["id"] == module_id), None)
    if not module:
        print(f"[WARN] Module '{module_id}' not found in catalog — skipping.")
        return copied

    is_windows = "windows" in platform
    exe_key = "windows" if is_windows else "linux"
    executables = module.get("executables", {}).get(exe_key, [])

    for exe in executables:
        src = deploy_root / exe
        if src.exists():
            shutil.copy2(src, dest / exe)
            copied.append(exe)
        else:
            print(f"[WARN] Executable not found: {src}")

    return copied


def main() -> int:
    parser = argparse.ArgumentParser(description="Assemble a customer deployment package.")
    parser.add_argument("--profile",        required=True, help="Customer profile JSON")
    parser.add_argument("--catalog",        required=True, help="Module catalog JSON")
    parser.add_argument("--platform",       required=True, help="Target platform")
    parser.add_argument("--deploy-root",    required=True, help="Build artifacts root")
    parser.add_argument("--configuration",  default="release", help="debug or release")
    parser.add_argument("--output-dir",     required=True, help="Output base directory")
    parser.add_argument("--output-layout",  default="customer_platform_config")
    parser.add_argument("--no-archive",     action="store_true", help="Skip tar/zip creation")
    args = parser.parse_args()

    profile_path = Path(args.profile)
    catalog_path = Path(args.catalog)
    deploy_root  = Path(args.deploy_root)
    output_dir   = Path(args.output_dir)

    profile = load_json(profile_path)
    catalog = load_json(catalog_path)

    customer_id = profile.get("customer_id", profile_path.stem)
    modules     = profile.get("modules", [])

    dest = build_output_dir(output_dir, customer_id, args.platform, args.configuration, args.output_layout)
    dest.mkdir(parents=True, exist_ok=True)

    print(f"[INFO] Customer: {customer_id}")
    print(f"[INFO] Platform: {args.platform} / {args.configuration}")
    print(f"[INFO] Output:   {dest}")

    total_copied = []
    for module_id in modules:
        copied = copy_module_artifacts(module_id, catalog, deploy_root, dest, args.platform)
        total_copied.extend(copied)

    if not total_copied:
        print("[WARN] No files were copied. Check deploy-root and catalog.")

    print(f"[OK] Packaged {len(total_copied)} file(s) for {customer_id}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
