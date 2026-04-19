from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPT_SOURCE = REPO_ROOT / "scripts" / "scaffold_apps.py"


def _build_fixture(tmp_path: Path) -> tuple[Path, Path]:
    project_root = tmp_path / "fixture_project"
    (project_root / "scripts").mkdir(parents=True)
    (project_root / "include" / "signal_editor").mkdir(parents=True)
    (project_root / "src" / "my_module").mkdir(parents=True)
    (project_root / "apps" / "my_module" / "cli").mkdir(parents=True)
    (project_root / "apps" / "my_module" / "gui").mkdir(parents=True)
    (project_root / "tests" / "03.unit_test" / "my_module").mkdir(parents=True)
    (project_root / "docs" / "specs").mkdir(parents=True)

    shutil.copy2(SCRIPT_SOURCE, project_root / "scripts" / "scaffold_apps.py")

    (project_root / "project.metadata.json").write_text(
        json.dumps(
            {
                "project_name": "AcmeControl",
                "project_slug": "acme_control",
                "namespace_prefix": "acmectl",
                "macro_prefix": "ACMECTL",
                "primary_module": "control_core",
                "description": "Acme control platform",
                "vendor": "Acme",
                "apps": [
                    {"module": "control_core", "cli": True, "gui": True},
                    {"module": "hex_tool", "cli": True, "gui": False},
                ],
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    (project_root / "src" / "my_module" / "CMakeLists.txt").write_text(
        "add_library(myprj_my_module_core STATIC api/my_module_api.cpp)\n",
        encoding="utf-8",
    )
    (project_root / "apps" / "my_module" / "cli" / "CMakeLists.txt").write_text(
        "add_executable(myprj_my_module_cli main.cpp)\n",
        encoding="utf-8",
    )
    (project_root / "apps" / "my_module" / "cli" / "main.cpp").write_text(
        "int main() { return 0; }\n",
        encoding="utf-8",
    )
    (project_root / "apps" / "my_module" / "gui" / "CMakeLists.txt").write_text(
        "qt_add_executable(myprj_my_module_gui main.cpp)\n",
        encoding="utf-8",
    )
    (project_root / "apps" / "my_module" / "gui" / "main.cpp").write_text(
        "int main() { return 0; }\n",
        encoding="utf-8",
    )
    (project_root / "tests" / "03.unit_test" / "my_module" / "CMakeLists.txt").write_text(
        "add_executable(test_my_module test.cpp)\n",
        encoding="utf-8",
    )
    (project_root / "docs" / "specs" / "my_module_srs.md").write_text(
        "# MyModule\nmodule my_module\n",
        encoding="utf-8",
    )
    (project_root / "include" / "signal_editor" / "my_module_version.h.in").write_text(
        '#define MY_MODULE_VERSION "0.1.0"\n',
        encoding="utf-8",
    )

    return project_root, project_root / "scripts" / "scaffold_apps.py"


def _run(script: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(script), *args],
        text=True,
        capture_output=True,
        check=False,
    )


def test_scaffold_apps_creates_additional_modules(tmp_path: Path) -> None:
    project_root, script = _build_fixture(tmp_path)
    report = project_root / ".scaffold-backups" / "scaffold_apps_report.json"

    result = _run(script, "--metadata", str(project_root / "project.metadata.json"), "--report-json", str(report))

    assert result.returncode == 0, result.stdout + result.stderr
    assert "Additional app scaffolds created." in result.stdout
    assert report.exists()

    payload = json.loads(report.read_text(encoding="utf-8"))
    assert payload["status"] == "ok"
    assert any(item["module"] == "control_core" for item in payload["skipped"])
    assert (project_root / "src" / "hex_tool" / "CMakeLists.txt").exists()
    assert (project_root / "apps" / "hex_tool" / "cli" / "CMakeLists.txt").exists()
    assert not (project_root / "apps" / "hex_tool" / "gui").exists()
    assert (project_root / "tests" / "03.unit_test" / "hex_tool" / "CMakeLists.txt").exists()
    assert (project_root / "docs" / "specs" / "hex_tool_srs.md").exists()
    assert (project_root / "include" / "signal_editor" / "hex_tool_version.h.in").exists()


def test_scaffold_apps_accepts_string_entries_as_shorthand(tmp_path: Path) -> None:
    project_root, script = _build_fixture(tmp_path)
    metadata_path = project_root / "project.metadata.json"
    payload = json.loads(metadata_path.read_text(encoding="utf-8"))
    payload["apps"] = ["control_core", "hex_tool"]
    metadata_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    report = project_root / ".scaffold-backups" / "scaffold_apps_report.json"
    result = _run(script, "--metadata", str(metadata_path), "--report-json", str(report))

    assert result.returncode == 0, result.stdout + result.stderr
    payload = json.loads(report.read_text(encoding="utf-8"))
    assert payload["status"] == "ok"
    assert any(item["module"] == "control_core" for item in payload["skipped"])
    created_app = next(item for item in payload["apps"] if item["module"] == "hex_tool")
    assert created_app == {"module": "hex_tool", "cli": True, "gui": True}
    assert (project_root / "apps" / "hex_tool" / "cli" / "CMakeLists.txt").exists()
    assert (project_root / "apps" / "hex_tool" / "gui" / "CMakeLists.txt").exists()
