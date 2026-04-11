from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
SCRIPT_SOURCE = REPO_ROOT / "scripts" / "customize_project.py"


def _build_fixture(tmp_path: Path) -> tuple[Path, Path]:
    project_root = tmp_path / "fixture_project"
    (project_root / "scripts").mkdir(parents=True)
    (project_root / "include" / "myprj").mkdir(parents=True)
    (project_root / "src" / "my_module").mkdir(parents=True)
    (project_root / "docs").mkdir(parents=True)

    shutil.copy2(SCRIPT_SOURCE, project_root / "scripts" / "customize_project.py")

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
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )

    (project_root / "README.md").write_text(
        "# Signal Editor\nnamespace myprj\nmodule my_module\nmacro SIGNAL_EDITOR\n",
        encoding="utf-8",
    )
    (project_root / "include" / "myprj" / "version.h.in").write_text(
        "#define SIGNAL_EDITOR_NAME \"SignalEditor\"\n",
        encoding="utf-8",
    )
    (project_root / "src" / "my_module" / "main.cpp").write_text(
        "namespace myprj { }\nint main() { return 0; }\n",
        encoding="utf-8",
    )

    return project_root, project_root / "scripts" / "customize_project.py"


def _run(script: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(script), *args],
        text=True,
        capture_output=True,
        check=False,
    )


def test_dry_run_generates_report_without_mutating_files(tmp_path: Path) -> None:
    project_root, script = _build_fixture(tmp_path)
    report = project_root / ".scaffold-backups" / "dry_run_report.json"
    readme_before = (project_root / "README.md").read_text(encoding="utf-8")

    result = _run(script, "--dry-run", "--report-json", str(report))

    assert result.returncode == 0, result.stderr
    assert "Dry-run complete" in result.stdout
    assert report.exists()
    payload = json.loads(report.read_text(encoding="utf-8"))
    assert payload["status"] == "planned"
    assert any(item["from"] == "Signal Editor" and item["to"] == "AcmeControl" for item in payload["replacements"])
    assert (project_root / "README.md").read_text(encoding="utf-8") == readme_before
    assert (project_root / "include" / "myprj").exists()
    assert (project_root / "src" / "my_module").exists()


def test_apply_and_rollback_restore_fixture(tmp_path: Path) -> None:
    project_root, script = _build_fixture(tmp_path)
    report = project_root / ".scaffold-backups" / "apply_report.json"

    apply_result = _run(script, "--apply", "--report-json", str(report))
    assert apply_result.returncode == 0, apply_result.stdout + apply_result.stderr
    assert "Project customization applied successfully." in apply_result.stdout

    renamed_include = project_root / "include" / "acmectl"
    renamed_src = project_root / "src" / "control_core"
    assert renamed_include.exists()
    assert renamed_src.exists()
    assert not (project_root / "include" / "myprj").exists()
    assert not (project_root / "src" / "my_module").exists()

    readme_after = (project_root / "README.md").read_text(encoding="utf-8")
    assert "AcmeControl" in readme_after
    assert "acmectl" in readme_after
    assert "control_core" in readme_after
    assert "SIGNAL_EDITOR" not in readme_after
    assert "myprj" not in readme_after
    assert "my_module" not in readme_after

    manifest = project_root / ".scaffold-backups" / "latest_manifest.json"
    assert manifest.exists()

    rollback_result = _run(script, "--rollback", str(manifest), "--report-json", str(report))
    assert rollback_result.returncode == 0, rollback_result.stdout + rollback_result.stderr
    assert "Rollback completed." in rollback_result.stdout
    assert (project_root / "include" / "myprj").exists()
    assert (project_root / "src" / "my_module").exists()
    assert not renamed_include.exists()
    assert not renamed_src.exists()

    readme_restored = (project_root / "README.md").read_text(encoding="utf-8")
    assert "Signal Editor" in readme_restored
    assert "myprj" in readme_restored
    assert "my_module" in readme_restored
