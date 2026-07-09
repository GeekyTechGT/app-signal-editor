#!/usr/bin/env python3
"""
generate_guid.py — generate (and optionally persist) the application GUID
used as the Inno Setup AppId and as project.json's stable identity.

Usage:
    python scripts/generate_guid.py              # print a new uuid4 and exit
    python scripts/generate_guid.py --write       # populate project.json["guid"] (only if empty)
    python scripts/generate_guid.py --write --force  # overwrite an existing guid

The GUID must stay stable across releases: changing it makes Inno Setup treat
future installers as a different application, breaking upgrade/uninstall
detection for machines that already have the app installed.
"""

from __future__ import annotations

import argparse
import json
import sys
import uuid
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
PROJECT_JSON = PROJECT_ROOT / "project.json"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="Write the GUID into project.json")
    parser.add_argument("--force", action="store_true", help="Overwrite an existing guid (dangerous)")
    args = parser.parse_args()

    new_guid = str(uuid.uuid4())

    if not args.write:
        print(new_guid)
        return 0

    if not PROJECT_JSON.exists():
        print(f"[ERROR] {PROJECT_JSON} not found.", file=sys.stderr)
        return 1

    with open(PROJECT_JSON, encoding="utf-8") as fh:
        meta = json.load(fh)

    existing = meta.get("guid", "")
    if existing and not args.force:
        print(f"[ERROR] project.json already has a guid ({existing}). "
              "Use --force to overwrite (this breaks upgrade/uninstall detection "
              "for existing installs).", file=sys.stderr)
        return 1

    meta["guid"] = new_guid
    with open(PROJECT_JSON, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(meta, fh, indent=4)
        fh.write("\n")

    print(f"[OK] project.json guid set to {new_guid}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
