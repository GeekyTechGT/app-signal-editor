#!/usr/bin/env python3
"""
project_manager.py — interactive build manager shared by all desktop/CLI projects.

Run it with uv, which provisions .venv from pyproject.toml/uv.lock on demand:
    uv run python scripts/project_manager.py

Supports three execution modes:
  native  — cmake runs directly in the current process
  wsl     — cmake runs inside WSL (wsl.exe on Windows, native on Linux)
  docker  — cmake runs inside Docker (docker run …)

This file is project-agnostic: everything project-specific is read from
project.json, and the two menu entries that vary the most between projects are
delegated to optional per-project scripts:

  [6] Run Tests     -> scripts/test.py      (override: scripts.test in project.json)
  [9] Run Examples  -> scripts/examples.py  (override: scripts.examples in project.json)

Both are invoked as ``python <script> --preset <preset>``. When the script does
not exist, the menu entry reports that nothing is available and does nothing
else, so a project without tests or examples needs no edits here. Reusable
machinery (preset catalogue, executors, JUnit parsing, HTML/text reports) stays
in this module so those scripts can import it instead of duplicating it.
"""

from __future__ import annotations

import json
import os
import platform
import re
import shutil
import stat
import subprocess
import sys
import textwrap
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Optional

# ---------------------------------------------------------------------------
# Bootstrap check
# ---------------------------------------------------------------------------
try:
    from rich.console import Console
    from rich.panel import Panel
    from rich.table import Table
    from rich.prompt import Prompt, Confirm
    from rich.rule import Rule
    from rich import box
except ImportError:
    print(
        "\n[ERROR] 'rich' is not installed.\n"
        "This project's Python tooling is managed with uv. Run it via:\n"
        "  uv run python scripts/project_manager.py\n"
        "which creates and syncs .venv from pyproject.toml/uv.lock on demand.\n"
        "Install uv: https://docs.astral.sh/uv/getting-started/installation/\n"
    )
    sys.exit(1)

# ---------------------------------------------------------------------------
# Globals
# ---------------------------------------------------------------------------
PROJECT_ROOT = Path(__file__).resolve().parent.parent
PROJECT_JSON = PROJECT_ROOT / "project.json"
CONSOLE = Console()
_HOST = platform.system()   # "Windows" or "Linux"

# CMake/CTest fallback paths on Windows
_WIN_CMAKE = [
    "C:/eng_apps/cmake/cmake-4.2.0/bin/cmake.exe",
    "C:/Program Files/CMake/bin/cmake.exe",
]
_WIN_CTEST = [
    "C:/eng_apps/cmake/cmake-4.2.0/bin/ctest.exe",
    "C:/Program Files/CMake/bin/ctest.exe",
]

# ---------------------------------------------------------------------------
# Preset catalogue
#
# The default catalogue below suits a project whose CMakePresets.json exposes
# the usual six. A project with a different set (an extra MSVC preset, no Docker
# variants, ...) overrides it wholesale through a "presets" list in
# project.json, so this file never needs a per-project edit.
# ---------------------------------------------------------------------------
DEFAULT_PRESETS: list[dict] = [
    {
        "name":           "windows-mingw64-debug",
        "display":        "Windows Qt6 GCC/MinGW64  [Debug]",
        "executor":       "native",
        "arch":           "64-bit",
        "build_type":     "Debug",
        "host":           "Windows",
        "shared_variant": "windows-mingw64-debug",
    },
    {
        "name":           "windows-mingw64-release",
        "display":        "Windows Qt6 GCC/MinGW64  [Release]",
        "executor":       "native",
        "arch":           "64-bit",
        "build_type":     "Release",
        "host":           "Windows",
        "shared_variant": "windows-mingw64-release",
    },
    {
        "name":           "linux-gcc-debug",
        "display":        "Linux/WSL Qt6 GCC  [Debug]",
        "executor":       "wsl",
        "arch":           "native",
        "build_type":     "Debug",
        "host":           "any",
        "shared_variant": "linux-gcc-debug",
    },
    {
        "name":           "linux-gcc-release",
        "display":        "Linux/WSL Qt6 GCC  [Release]",
        "executor":       "wsl",
        "arch":           "native",
        "build_type":     "Release",
        "host":           "any",
        "shared_variant": "linux-gcc-release",
    },
    {
        "name":           "docker-gcc-debug",
        "display":        "Docker Qt6 GCC  [Debug]",
        "executor":       "docker",
        "arch":           "native",
        "build_type":     "Debug",
        "host":           "any",
        "shared_variant": "linux-gcc-debug",
    },
    {
        "name":           "docker-gcc-release",
        "display":        "Docker Qt6 GCC  [Release]",
        "executor":       "docker",
        "arch":           "native",
        "build_type":     "Release",
        "host":           "any",
        "shared_variant": "linux-gcc-release",
    },
]

# Resolved from project.json at startup; see load_presets().
PRESETS: list[dict] = list(DEFAULT_PRESETS)


def load_presets(meta: dict) -> list[dict]:
    """Preset catalogue for this project: project.json "presets" when present,
    the default catalogue otherwise. Missing per-preset fields fall back to
    sensible values so a project only has to spell out what differs."""
    configured = meta.get("presets")
    if not configured:
        return list(DEFAULT_PRESETS)

    resolved: list[dict] = []
    for entry in configured:
        preset = {
            "display":        entry.get("display", entry["name"]),
            "executor":       entry.get("executor", "native"),
            "arch":           entry.get("arch", "native"),
            "build_type":     entry.get("build_type", "Debug"),
            "host":           entry.get("host", "any"),
            "shared_variant": entry.get("shared_variant", entry["name"]),
        }
        preset.update(entry)
        resolved.append(preset)
    return resolved


def cmake_prefix(meta: dict) -> str:
    """Prefix of the project's CMake cache variables (<PREFIX>_BUILD_GUI, ...).

    Read from project.json "cmake_prefix"; falls back to the project name
    upper-cased. Getting this wrong is silent — CMake ignores unknown -D
    variables — so the project declares it explicitly.
    """
    configured = meta.get("cmake_prefix")
    if configured:
        return configured
    return re.sub(r"[^A-Za-z0-9]+", "_", meta.get("name", "")).upper()


def preset_is_available(p: dict, meta: dict | None = None) -> bool:
    if p["host"] == "Windows" and _HOST != "Windows":
        return False
    if p["executor"] == "wsl" and _HOST == "Windows":
        return shutil.which("wsl") is not None or shutil.which("wsl.exe") is not None
    if p["executor"] == "docker":
        if meta is not None and not meta.get("docker_image_available", True):
            return False
        return shutil.which("docker") is not None
    return True


def executor_label(p: dict) -> str:
    if p["executor"] == "native":
        return "[cyan]native[/]"
    if p["executor"] == "wsl":
        return "[cyan]native[/]" if _HOST == "Linux" else "[yellow]WSL[/]"
    if p["executor"] == "docker":
        return "[magenta]Docker[/]"
    return p["executor"]


# ---------------------------------------------------------------------------
# Tool discovery
# ---------------------------------------------------------------------------

def _find_exe(name: str, extra: list[str] | None = None) -> Optional[str]:
    found = shutil.which(name)
    if found:
        return found
    for c in (extra or []):
        if Path(c).is_file():
            return c
    return None


def find_cmake(meta: dict | None = None) -> Optional[str]:
    configured = (meta or {}).get("tools", {}).get("cmake", "")
    if configured and Path(configured).is_file():
        return configured
    return _find_exe("cmake", _WIN_CMAKE if _HOST == "Windows" else [])


def find_ctest(meta: dict | None = None) -> Optional[str]:
    configured = (meta or {}).get("tools", {}).get("ctest", "")
    if configured and Path(configured).is_file():
        return configured
    return _find_exe("ctest", _WIN_CTEST if _HOST == "Windows" else [])


def find_inno_compiler(meta: dict) -> Optional[str]:
    """Locate ISCC.exe (Inno Setup Compiler): project.json tools.inno_setup_compiler,
    then PATH, then the default winget install locations."""
    configured = meta.get("tools", {}).get("inno_setup_compiler", "")
    if configured and Path(configured).is_file():
        return configured

    found = shutil.which("iscc") or shutil.which("ISCC")
    if found:
        return found

    for candidate in (
        Path.home() / "AppData/Local/Programs/Inno Setup 6/ISCC.exe",
        Path("C:/Program Files (x86)/Inno Setup 6/ISCC.exe"),
        Path("C:/Program Files/Inno Setup 6/ISCC.exe"),
    ):
        if candidate.is_file():
            return str(candidate)

    return None


# ---------------------------------------------------------------------------
# Path helpers
# ---------------------------------------------------------------------------

def _win_to_wsl_path(p: Path) -> str:
    parts = p.parts
    drive = parts[0].rstrip(":\\").lower()
    rest  = "/".join(parts[1:])
    return f"/mnt/{drive}/{rest}"


def _win_to_docker_volume(p: Path) -> str:
    return str(p).replace("\\", "/")


# ---------------------------------------------------------------------------
# Command builders
# ---------------------------------------------------------------------------

def _build_native_cmd(base_cmd: list[str]) -> list[str]:
    return base_cmd


def _build_wsl_cmd(base_cmd: list[str]) -> list[str]:
    if _HOST == "Linux":
        return base_cmd
    wsl_path   = _win_to_wsl_path(PROJECT_ROOT)
    shell_cmd  = " ".join(f'"{a}"' if " " in a else a for a in base_cmd)
    return ["wsl.exe", "bash", "-lc", f"cd '{wsl_path}' && {shell_cmd}"]


def _build_docker_cmd(base_cmd: list[str], meta: dict) -> list[str]:
    image  = meta.get("docker_image_name", "signal-viewer-builder")
    volume = _win_to_docker_volume(PROJECT_ROOT) if _HOST == "Windows" else str(PROJECT_ROOT)
    return ["docker", "run", "--rm", "-v", f"{volume}:/workspace",
            "-w", "/workspace", image] + base_cmd


def build_cmd(preset: dict, base_cmd: list[str], meta: dict) -> list[str]:
    ex = preset["executor"]
    if ex == "native":
        return _build_native_cmd(base_cmd)
    if ex == "wsl":
        return _build_wsl_cmd(base_cmd)
    if ex == "docker":
        return _build_docker_cmd(base_cmd, meta)
    return base_cmd


# ---------------------------------------------------------------------------
# Project metadata
# ---------------------------------------------------------------------------

def load_metadata() -> dict:
    global PRESETS

    if not PROJECT_JSON.exists():
        CONSOLE.print(f"[bold red]ERROR[/] project.json not found at {PROJECT_JSON}")
        sys.exit(1)
    with open(PROJECT_JSON, encoding="utf-8") as fh:
        meta = json.load(fh)

    # Resolve the catalogue here rather than in main(): scripts/test.py and
    # scripts/examples.py enter through this function too, and must see the same
    # presets the menu offered them.
    PRESETS = load_presets(meta)
    return meta


def save_metadata(meta: dict) -> None:
    with open(PROJECT_JSON, "w", encoding="utf-8", newline="\n") as fh:
        json.dump(meta, fh, indent=4)
        fh.write("\n")


# ---------------------------------------------------------------------------
# Dependency validation
# ---------------------------------------------------------------------------

def _validate_dependencies(preset: dict, meta: dict) -> bool:
    """
    Verify that every declared dependency is present.

    Two modes, controlled by ``use_external_deps`` in project.json:

    * ``false`` (default): dependencies are pre-built binaries in
      ``shared_lib_base/<name>/<version>/<shared_variant>/``
    * ``true``: dependencies are vendored inside
      ``external/<name>/`` relative to the project root

    Returns True if all dependencies are satisfied; prints a verbose error and
    returns False otherwise.
    """
    dependencies     = meta.get("dependencies", [])
    use_external     = meta.get("use_external_deps", False)

    if not dependencies:
        return True

    all_ok = True

    if use_external:
        # ── external/ mode ────────────────────────────────────────────────────
        for dep in dependencies:
            dep_name = dep["name"]
            dep_path = PROJECT_ROOT / "external" / dep_name
            if not dep_path.exists():
                CONSOLE.print()
                CONSOLE.print(Panel(
                    f"[bold red]DEPENDENCY ERROR  (use_external_deps = true)[/]\n\n"
                    f"Dependency   : [yellow]{dep_name}[/]\n"
                    f"Expected at  : [dim]{dep_path}[/]\n\n"
                    f"[bold]This path does not exist![/]\n\n"
                    f"[bold cyan]How to fix:[/]\n"
                    f"  Copy or link the pre-built [yellow]{dep_name}[/] tree to:\n"
                    f"    [dim]{dep_path}[/]\n"
                    f"  Required layout: include/ lib/ bin/\n\n"
                    f"  Or set [yellow]use_external_deps[/] to [yellow]false[/] in project.json\n"
                    f"  to use the shared_lib folder instead.",
                    title="[bold red]Cannot configure — external dependency missing[/]",
                    border_style="red",
                    expand=False,
                ))
                CONSOLE.print()
                all_ok = False
    else:
        # ── shared_lib mode ───────────────────────────────────────────────────
        shared_lib_base = Path(meta.get("shared_lib_base", ""))
        shared_variant  = preset.get("shared_variant", preset["name"])

        for dep in dependencies:
            dep_name    = dep["name"]
            dep_version = dep["version"]
            dep_path    = shared_lib_base / dep_name / dep_version / shared_variant

            if not dep_path.exists():
                dep_base  = shared_lib_base / dep_name / dep_version
                available = sorted(d.name for d in dep_base.iterdir() if d.is_dir()) \
                    if dep_base.exists() else []

                CONSOLE.print()
                CONSOLE.print(Panel(
                    f"[bold red]DEPENDENCY ERROR[/]\n\n"
                    f"Dependency   : [yellow]{dep_name} {dep_version}[/]\n"
                    f"Required for : [yellow]{preset['name']}[/]  "
                    f"(shared variant: [yellow]{shared_variant}[/])\n\n"
                    f"Expected path:\n  [dim]{dep_path}[/]\n\n"
                    f"[bold]This path does not exist![/]\n" +
                    (f"\nAvailable variants for {dep_name} {dep_version}:\n" +
                     "\n".join(f"  [green]✓[/] {v}" for v in available)
                     if available else
                     f"\n[red]No variants deployed at all for {dep_name} {dep_version}.[/]") +
                    f"\n\n[bold cyan]How to fix:[/]\n"
                    f"  a) Build & deploy [yellow]{dep_name}[/] for variant "
                    f"'[yellow]{shared_variant}[/]'\n"
                    f"     using that library's project_manager.py\n"
                    f"  b) Switch to a preset whose shared_variant matches an available deployment\n"
                    f"  c) Set [yellow]use_external_deps[/] to [yellow]true[/] in project.json\n"
                    f"     and place the library under external/{dep_name}/",
                    title="[bold red]Cannot configure — dependency missing[/]",
                    border_style="red",
                    expand=False,
                ))
                CONSOLE.print()
                all_ok = False

    return all_ok


# ---------------------------------------------------------------------------
# Subprocess helpers
# ---------------------------------------------------------------------------

def _make_env(preset_name: str | None = None) -> dict:
    env = os.environ.copy()
    if _HOST != "Windows" or preset_name is None:
        return env

    meta = {}
    if PROJECT_JSON.exists():
        with open(PROJECT_JSON, encoding="utf-8") as fh:
            meta = json.load(fh)

    qt_root    = meta.get("qt_root",    "C:/eng_apps/Qt")
    qt_version = meta.get("qt_version", "6.10.1")

    if "gcc" in preset_name or "mingw" in preset_name:
        qt_mingw_bin = f"{qt_root}/Tools/mingw1310_64/bin"
        qt_lib_bin   = f"{qt_root}/{qt_version}/mingw_64/bin"
        qt_ninja     = f"{qt_root}/Tools/Ninja"
        prepend      = [qt_mingw_bin, qt_lib_bin, qt_ninja]
    else:
        return env

    sep          = os.pathsep
    env["PATH"]  = sep.join(prepend) + sep + env.get("PATH", "")
    return env


def run(cmd: list[str], preset_name: str | None = None) -> int:
    CONSOLE.print(f"[dim]$ {' '.join(cmd)}[/]")
    return subprocess.run(cmd, cwd=PROJECT_ROOT, env=_make_env(preset_name)).returncode


def run_capture(cmd: list[str], preset_name: str | None = None) -> tuple[int, str]:
    CONSOLE.print(f"[dim]$ {' '.join(cmd)}[/]")
    r = subprocess.run(cmd, cwd=PROJECT_ROOT, capture_output=True,
                       text=True, encoding="utf-8", errors="replace",
                       env=_make_env(preset_name))
    return r.returncode, r.stdout + r.stderr


# ---------------------------------------------------------------------------
# HTML test report
# ---------------------------------------------------------------------------

def _parse_junit(xml_path: Path) -> tuple[int, int, list[dict]]:
    tests: list[dict] = []
    total = failures = 0
    try:
        root = ET.parse(xml_path).getroot()
        for tc in root.iter("testcase"):
            total += 1
            f_el = tc.find("failure")
            e_el = tc.find("error")
            s_el = tc.find("skipped")
            if f_el is not None or e_el is not None:
                failures += 1
                status     = "FAIL"
                detail_el  = f_el if f_el is not None else e_el
                msg        = detail_el.get("message", "") if detail_el is not None else ""
            elif s_el is not None:
                status = "SKIP"
                msg    = ""
            else:
                status = "PASS"
                msg    = ""
            tests.append({
                "name":      tc.get("name", "?"),
                "classname": tc.get("classname", ""),
                "time":      float(tc.get("time", "0")),
                "status":    status,
                "message":   msg,
            })
    except Exception as exc:
        CONSOLE.print(f"[yellow]Warning: could not parse JUnit XML: {exc}[/]")
    return total, failures, tests


def _generate_html(out: Path, raw: str, junit: Optional[Path],
                   preset_name: str, meta: dict, tail_characters: int = 0) -> None:
    total, failures, cases = 0, 0, []
    if junit and junit.exists():
        total, failures, cases = _parse_junit(junit)
    passed = total - failures
    ok = failures == 0
    sc = "#28a745" if ok else "#dc3545"

    if tail_characters and len(raw) > tail_characters:
        raw = f"…[truncated to last {tail_characters} characters]…\n" + raw[-tail_characters:]

    rows = ""
    for tc in cases:
        bg = {"PASS": "#d4edda", "FAIL": "#f8d7da", "SKIP": "#fff3cd"}.get(tc["status"], "#fff")
        rows += (
            f"<tr style='background:{bg}'>"
            f"<td>{tc['classname']}</td><td>{tc['name']}</td>"
            f"<td style='text-align:center;font-weight:bold'>{tc['status']}</td>"
            f"<td style='text-align:right'>{tc['time']:.3f}s</td>"
            f"<td style='font-size:.8em;color:#555'>{tc['message'][:120]}</td>"
            f"</tr>\n"
        )
    tbl = (
        "<table border='1' cellspacing='0' cellpadding='6' "
        "style='border-collapse:collapse;width:100%;font-family:monospace;font-size:.9em'>"
        "<thead style='background:#343a40;color:white'>"
        "<tr><th>Suite</th><th>Test</th><th>Status</th><th>Time</th><th>Message</th></tr>"
        f"</thead><tbody>{rows}</tbody></table>"
    ) if cases else "<p><em>No test detail (JUnit XML unavailable).</em></p>"

    esc = raw.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    lbl = "ALL PASSED" if ok else f"{failures} FAILED"

    html = textwrap.dedent(f"""<!DOCTYPE html>
    <html lang="en"><head><meta charset="UTF-8"/>
    <title>Test Report — {meta['name']} {meta['version']}</title>
    <style>
      body{{font-family:'Segoe UI',Arial,sans-serif;margin:24px;background:#f5f5f5}}
      h1{{color:#1a1a2e}}
      .summary{{background:{sc};color:white;padding:12px 20px;border-radius:6px;
                font-size:1.2em;margin-bottom:20px}}
      .badge{{display:inline-block;padding:4px 12px;border-radius:12px;
              font-weight:bold;font-size:.85em}}
      .pass{{background:#28a745;color:white}} .fail{{background:#dc3545;color:white}}
      pre{{background:#1e1e1e;color:#d4d4d4;padding:16px;border-radius:6px;
           overflow-x:auto;font-size:.85em}}
      details summary{{cursor:pointer;color:#007bff;font-weight:bold;margin-top:20px}}
    </style></head>
    <body>
    <h1>Test Report — {meta['name']} {meta['version']}</h1>
    <div class="summary">
      Preset: <strong>{preset_name}</strong> &nbsp;|&nbsp;
      Total: <strong>{total}</strong> &nbsp;|&nbsp;
      <span class="badge {'pass' if ok else 'fail'}">{lbl}</span>
      &nbsp;({passed} passed, {failures} failed)
    </div>
    {tbl}
    <details open><summary>Raw CTest output</summary><pre>{esc}</pre></details>
    </body></html>
    """)
    out.write_text(html, encoding="utf-8")


def _write_text_report(out: Path, raw: str, tail_characters: int = 0) -> None:
    if tail_characters and len(raw) > tail_characters:
        raw = f"...[truncated to last {tail_characters} characters]...\n" + raw[-tail_characters:]
    out.write_text(raw, encoding="utf-8")


def _dir_size_bytes(path: Path) -> int:
    return sum(f.stat().st_size for f in path.rglob("*") if f.is_file())


# ---------------------------------------------------------------------------
# WSL / Docker helpers
# ---------------------------------------------------------------------------

def _check_wsl() -> bool:
    if _HOST == "Linux":
        return True
    wsl = shutil.which("wsl") or shutil.which("wsl.exe")
    if not wsl:
        CONSOLE.print("[red]WSL not found. Install WSL: wsl --install[/]")
        return False
    return True


def _check_docker(meta: dict) -> bool:
    if not meta.get("docker_image_available", True):
        CONSOLE.print("[red]docker_image_available is false in project.json.[/]")
        return False
    docker = shutil.which("docker")
    if not docker:
        CONSOLE.print("[red]docker not found in PATH. Is Docker Desktop running?[/]")
        return False
    image = meta.get("docker_image_name", "signal-viewer-builder")
    r = subprocess.run(["docker", "image", "inspect", image], capture_output=True, text=True)
    if r.returncode != 0:
        CONSOLE.print(
            f"[bold red]Docker image '[yellow]{image}[/]' not found.[/]\n"
            f"Build it first:\n"
            f"  [bold]docker build -t {image} .[/bold]  (from project root)"
        )
        return False
    return True


def _check_executor(preset: dict, meta: dict) -> bool:
    ex = preset["executor"]
    if ex == "native":
        if preset["host"] == "Windows" and _HOST != "Windows":
            CONSOLE.print("[red]Windows presets can only run on Windows.[/]")
            return False
        return True
    if ex == "wsl":
        return _check_wsl()
    if ex == "docker":
        return _check_docker(meta)
    return True


# ---------------------------------------------------------------------------
# Qt helpers
# ---------------------------------------------------------------------------

def _find_windeployqt(meta: dict) -> Optional[Path]:
    qt_root    = meta.get("qt_root",    "C:/eng_apps/Qt")
    qt_version = meta.get("qt_version", "6.10.1")
    qt_bin     = Path(f"{qt_root}/{qt_version}/mingw_64/bin")

    for candidate in ["windeployqt6.exe", "windeployqt.exe"]:
        p = qt_bin / candidate
        if p.exists():
            return p

    for name in ("windeployqt6", "windeployqt"):
        found = shutil.which(name)
        if found:
            return Path(found)

    return None


def _has_executed_tests(output: str, junit_xml: Optional[Path]) -> bool:
    if junit_xml and junit_xml.exists():
        total, _, _ = _parse_junit(junit_xml)
        if total > 0:
            return True
    return "no tests were found" not in output.lower()


# ---------------------------------------------------------------------------
# Filesystem helpers
# ---------------------------------------------------------------------------

def _rmtree_force(path: Path) -> None:
    """Delete a directory tree, unlocking read-only files first (required on Windows for git repos)."""
    def _handle_ro(func, fpath, _exc):
        os.chmod(fpath, stat.S_IWRITE)
        func(fpath)

    if sys.version_info >= (3, 12):
        shutil.rmtree(path, onexc=_handle_ro)
    else:
        shutil.rmtree(path, onerror=_handle_ro)


# ---------------------------------------------------------------------------
# Stale cache detection
# ---------------------------------------------------------------------------

def _clear_stale_cache(preset_name: str) -> None:
    """
    If the build directory was generated from a different source directory
    (e.g. after the project was renamed or moved), delete the entire build
    directory so CMake can start fresh.  Deleting only the top-level
    CMakeCache.txt is not enough: FetchContent sub-builds also have their own
    stale CMakeCache.txt files that trigger the same error.
    """
    build_dir = PROJECT_ROOT / "build" / preset_name
    cache     = build_dir / "CMakeCache.txt"
    if not cache.exists():
        return

    source_dir: Optional[str] = None
    for line in cache.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
            source_dir = line.split("=", 1)[1].strip()
            break

    if source_dir is None:
        return

    cached_root  = Path(source_dir).resolve()
    current_root = PROJECT_ROOT.resolve()

    if cached_root != current_root:
        CONSOLE.print(
            f"\n[bold yellow]⚠  Stale build directory detected[/]\n"
            f"  Cached source : [dim]{cached_root}[/]\n"
            f"  Current source: [dim]{current_root}[/]\n"
            f"  The build directory was created from a different path.\n"
            f"  Deleting entire build directory to allow a clean reconfigure...",
        )
        _rmtree_force(build_dir)
        CONSOLE.print("[bold green]✓  Stale build directory removed — CMake will reconfigure from scratch.[/]\n")


# ---------------------------------------------------------------------------
# Core actions
# ---------------------------------------------------------------------------

def action_select_preset(state: dict, meta: dict) -> None:
    CONSOLE.print()
    table = Table(title="All Presets", box=box.ROUNDED, header_style="bold cyan")
    table.add_column("#",              justify="right",  style="bold yellow")
    table.add_column("Preset Name",    style="white")
    table.add_column("Description",    style="dim")
    table.add_column("Arch",           justify="center")
    table.add_column("Type",           justify="center")
    table.add_column("Runner",         justify="center")
    table.add_column("Deps Variant",   style="dim")
    table.add_column("Ready",          justify="center")

    available_indices: list[int] = []
    for idx, p in enumerate(PRESETS, start=1):
        avail  = preset_is_available(p, meta)
        marker = "[bold green]→[/]" if p["name"] == state["preset"] else "  "
        ready  = "[green]✓[/]" if avail else "[red]✗[/]"
        if avail:
            available_indices.append(idx)
        table.add_row(
            f"{marker}{idx}", p["name"], p["display"],
            p["arch"], p["build_type"], executor_label(p),
            p.get("shared_variant", p["name"]), ready,
        )
    CONSOLE.print(table)

    if not available_indices:
        CONSOLE.print("[red]No presets available on this host.[/]")
        return

    CONSOLE.print("[dim]✓ = ready  |  ✗ = unavailable (WSL/Docker not found or Windows-only)[/]")
    choice = Prompt.ask(
        "Select preset number",
        choices=[str(i) for i in range(1, len(PRESETS) + 1)],
        default=str(available_indices[0]),
    )
    chosen = PRESETS[int(choice) - 1]
    if not preset_is_available(chosen, meta):
        CONSOLE.print(f"[bold yellow]⚠ Preset '{chosen['name']}' is marked as unavailable — "
                      "proceeding anyway.[/]")
    state["preset"] = chosen["name"]
    CONSOLE.print(f"[bold green]✓[/] Preset set to [bold]{state['preset']}[/]"
                  f" (executor: {chosen['executor']},"
                  f" deps-variant: {chosen.get('shared_variant', chosen['name'])})")


def _get_preset(name: str) -> dict:
    for p in PRESETS:
        if p["name"] == name:
            return p
    raise KeyError(name)


def action_configure(state: dict, cmake: str, meta: dict) -> bool:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Configure — {state['preset']} [{preset['executor']}]"))
    if not _check_executor(preset, meta):
        return False

    # Remove stale cache before anything else (catches project renames/moves)
    _clear_stale_cache(state["preset"])

    # ── Dependency check before invoking cmake ────────────────────────────────
    CONSOLE.print("[dim]Checking dependencies...[/]")
    if not _validate_dependencies(preset, meta):
        return False
    CONSOLE.print("[bold green]✓ All dependencies verified[/]")

    # Pass the shared-lib variant and external-deps flag as cmake cache variables,
    # under this project's own prefix (see cmake_prefix).
    shared_variant  = preset.get("shared_variant", preset["name"])
    use_external    = meta.get("use_external_deps", False)
    prefix          = cmake_prefix(meta)
    extra_args      = [
        f"-D{prefix}_DEPS_VARIANT={shared_variant}",
        f"-D{prefix}_USE_EXTERNAL_DEPS={'ON' if use_external else 'OFF'}",
        f"-D{prefix}_BUILD_GUI=ON",
    ]

    base = [cmake, "--preset", state["preset"]] + extra_args
    if preset["executor"] in ("wsl", "docker"):
        base = ["cmake", "--preset", state["preset"]] + extra_args
    cmd  = build_cmd(preset, base, meta)
    code = run(cmd, preset_name=state["preset"] if preset["executor"] == "native" else None)
    if code == 0:
        CONSOLE.print("[bold green]✓ Configure succeeded[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Configure failed (exit {code})[/]")
    return code == 0


def action_build(state: dict, cmake: str, meta: dict) -> bool:
    preset = _get_preset(state["preset"])
    CONSOLE.print(Rule(f"[bold cyan]Build — {state['preset']} [{preset['executor']}]"))
    if not _check_executor(preset, meta):
        return False
    base = [cmake, "--build", "--preset", state["preset"]]
    if preset["executor"] in ("wsl", "docker"):
        base = ["cmake", "--build", "--preset", state["preset"]]
    cmd  = build_cmd(preset, base, meta)
    code = run(cmd, preset_name=state["preset"] if preset["executor"] == "native" else None)
    if code == 0:
        CONSOLE.print("[bold green]✓ Build succeeded[/]")
    else:
        CONSOLE.print(f"[bold red]✗ Build failed (exit {code})[/]")
    return code == 0


def action_clean(state: dict) -> None:
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    if not build_dir.exists():
        CONSOLE.print(f"[yellow]Build directory not found: {build_dir}[/]")
        return
    if Confirm.ask(f"Delete [bold]{build_dir}[/]?", default=False):
        _rmtree_force(build_dir)
        CONSOLE.print(f"[bold green]✓[/] Deleted {build_dir}")
    else:
        CONSOLE.print("[dim]Cancelled.[/]")


def action_clean_all() -> None:
    build_root = PROJECT_ROOT / "build"
    if not build_root.exists() or not any(build_root.iterdir()):
        CONSOLE.print("[yellow]Build directory is already empty.[/]")
        return
    subdirs = [d for d in build_root.iterdir() if d.is_dir()]
    for d in subdirs:
        CONSOLE.print(f"  [dim]{d}[/]")
    if Confirm.ask("Delete ALL build directories?", default=False):
        _rmtree_force(build_root)
        CONSOLE.print("[bold green]✓[/] All build directories deleted.")
    else:
        CONSOLE.print("[dim]Cancelled.[/]")


# ---------------------------------------------------------------------------
# Per-project scripts (tests / examples)
# ---------------------------------------------------------------------------

# Menu entries delegated to an optional per-project script, keyed by the
# project.json "scripts" field that overrides the default path.
PROJECT_SCRIPTS: dict[str, dict[str, str]] = {
    "test": {
        "default": "scripts/test.py",
        "title":   "Tests",
        "missing": "No runnable tests",
    },
    "examples": {
        "default": "scripts/examples.py",
        "title":   "Examples",
        "missing": "No runnable examples",
    },
}


def _project_script_path(meta: dict, key: str) -> Path:
    """Absolute path of a per-project script, whether or not it exists."""
    configured = meta.get("scripts", {}).get(key) or PROJECT_SCRIPTS[key]["default"]
    path = Path(configured)
    return path if path.is_absolute() else PROJECT_ROOT / path


def action_run_project_script(state: dict, meta: dict, key: str) -> None:
    """Run a per-project script (tests or examples) for the selected preset.

    The script is invoked as ``python <script> --preset <preset>`` from the
    project root and owns everything project-specific; this module only locates
    it and reports the outcome. A missing script is a normal state, not an
    error: the project simply has nothing of that kind to run.
    """
    spec   = PROJECT_SCRIPTS[key]
    script = _project_script_path(meta, key)

    CONSOLE.print(Rule(f"[bold cyan]{spec['title']} — {state['preset']}"))

    if not script.is_file():
        CONSOLE.print(f"[bold yellow]{spec['missing']}[/]")
        CONSOLE.print(
            f"[dim]Expected script: {script}\n"
            f"Create it, or point \"scripts.{key}\" in project.json at another path.[/]"
        )
        return

    code = run([sys.executable, str(script), "--preset", state["preset"]])
    if code != 0:
        CONSOLE.print(f"[bold red]✗ {script.name} failed (exit {code})[/]")


_MINGW_WINPTHREAD = [
    "C:/eng_apps/msys64/mingw64/bin/libwinpthread-1.dll",
    "C:/eng_apps/msys64/mingw32/bin/libwinpthread-1.dll",
]

_MINGW_LIBZIP_RUNTIME_DLLS = [
    "libzip.dll",
    "zlib1.dll",
    "liblzma-5.dll",
    "libbz2-1.dll",
]


def _candidate_mingw_bin_dirs(preset_name: str) -> list[Path]:
    candidates: list[Path] = []
    if "mingw64" in preset_name:
        candidates.append(Path("C:/eng_apps/msys64/mingw64/bin"))
    if "mingw32" in preset_name:
        candidates.append(Path("C:/eng_apps/msys64/mingw32/bin"))
    candidates.extend([
        Path("C:/eng_apps/msys64/mingw64/bin"),
        Path("C:/eng_apps/msys64/mingw32/bin"),
    ])

    unique_candidates: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate).lower()
        if key in seen:
            continue
        seen.add(key)
        unique_candidates.append(candidate)
    return unique_candidates


def _copy_mingw_runtime(preset_name: str, bin_dir: Path) -> None:
    if _HOST != "Windows":
        return
    src: Optional[Path] = None
    for c in _MINGW_WINPTHREAD:
        p = Path(c)
        if not p.exists():
            continue
        if "mingw64" in preset_name and "mingw64" in str(p):
            src = p
            break
        if "mingw32" in preset_name and "mingw32" in str(p):
            src = p
            break
    if src is None:
        for c in _MINGW_WINPTHREAD:
            p = Path(c)
            if p.exists():
                src = p
                break
    if src is None:
        CONSOLE.print("[yellow]  libwinpthread-1.dll not found — add it manually if needed.[/]")
        return
    dst = bin_dir / "libwinpthread-1.dll"
    shutil.copy2(src, dst)
    CONSOLE.print(f"  [dim]Bundled:[/] {dst.name} ← {src}")


def _copy_mingw_libzip_runtime(preset_name: str, bin_dir: Path) -> set[str]:
    if _HOST != "Windows":
        return set()

    copied: set[str] = set()
    search_dirs = [d for d in _candidate_mingw_bin_dirs(preset_name) if d.exists()]
    if not search_dirs:
        CONSOLE.print("[yellow]  MinGW bin directory not found — libzip runtime was not bundled.[/]")
        return copied

    missing: list[str] = []
    for dll_name in _MINGW_LIBZIP_RUNTIME_DLLS:
        src: Optional[Path] = None
        for directory in search_dirs:
            candidate = directory / dll_name
            if candidate.exists():
                src = candidate
                break
        if src is None:
            missing.append(dll_name)
            continue

        shutil.copy2(src, bin_dir / dll_name)
        copied.add(dll_name)
        CONSOLE.print(f"  [dim]Bundled:[/] {dll_name} ← {src}")

    if missing:
        CONSOLE.print(
            "[yellow]  Missing optional libzip runtime DLL(s): "
            + ", ".join(missing)
            + "[/]"
        )

    return copied


def _resolve_relative_dir(meta: dict, key: str, default: str) -> Path:
    """Resolve a project.json path field (e.g. deploy_folder/setup_folder)
    relative to the project root; absolute values are used as-is."""
    configured = meta.get(key) or default
    root = Path(configured)
    if not root.is_absolute():
        root = PROJECT_ROOT / root
    return root


def _resolve_app_deploy_dir(state: dict, meta: dict) -> Path:
    return _resolve_relative_dir(meta, "deploy_folder", "deploy/app") / state["preset"]


def _resolve_setup_dir(meta: dict) -> Path:
    return _resolve_relative_dir(meta, "setup_folder", "deploy/setup")


def action_deploy_app(state: dict, cmake: str, meta: dict) -> None:
    """Deploy the GUI executable with windeployqt to deploy/app/<preset>/."""
    CONSOLE.print(Rule(f"[bold cyan]Deploy App — {state['preset']}"))

    preset    = _get_preset(state["preset"])
    build_dir = PROJECT_ROOT / "build" / state["preset"]
    bin_dir   = build_dir / "bin"

    if not bin_dir.exists():
        CONSOLE.print(f"[red]Bin directory not found: {bin_dir}[/]")
        CONSOLE.print("[dim]Configure + Build first.[/]")
        return

    if _HOST != "Windows":
        CONSOLE.print("[yellow]windeployqt is Windows-only. Skipping Qt DLL bundling.[/]")
        return

    windeployqt = _find_windeployqt(meta)
    if windeployqt is None:
        qt_root    = meta.get("qt_root",    "C:/eng_apps/Qt")
        qt_version = meta.get("qt_version", "6.10.1")
        CONSOLE.print(
            f"[red]windeployqt not found.[/]\n"
            f"Expected at: {qt_root}/{qt_version}/mingw_64/bin/windeployqt6.exe\n"
            f"Check qt_root and qt_version in project.json."
        )
        return

    exe_name = meta.get("app_executable", "signal-viewer.exe")
    exe = bin_dir / exe_name
    if not exe.exists():
        CONSOLE.print(f"[red]Executable not found: {exe}[/]")
        return

    out_dir = _resolve_app_deploy_dir(state, meta)
    if not Confirm.ask(f"Deploy to [bold]{out_dir}[/]?", default=True):
        CONSOLE.print("[dim]Cancelled.[/]")
        return

    if out_dir.exists():
        _rmtree_force(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    # Copy exe
    shutil.copy2(exe, out_dir / exe.name)

    # Copy runtime DLLs produced by this project build itself.
    # This covers shared libraries such as libsignal_viewer_core.dll that the
    # executable depends on but that are not part of shared_lib/ or Qt.
    project_runtime_dlls: set[str] = set()
    for dll in sorted(bin_dir.glob("*.dll")):
        shutil.copy2(dll, out_dir / dll.name)
        project_runtime_dlls.add(dll.name)
        CONSOLE.print(f"  [dim]Project runtime:[/] {dll.name}")

    # Copy declared dependency DLLs from shared_lib
    shared_lib_base = Path(meta.get("shared_lib_base", ""))
    shared_variant  = preset.get("shared_variant", preset["name"])
    copied_dependency_dlls: set[str] = set()
    for dep in meta.get("dependencies", []):
        dll = (shared_lib_base / dep["name"] / dep["version"] /
               shared_variant / "bin" / f"{dep['name']}.dll")
        if dll.exists():
            shutil.copy2(dll, out_dir / dll.name)
            copied_dependency_dlls.add(dll.name)
            CONSOLE.print(f"  [dim]Dependency:[/] {dll.name}")
        else:
            CONSOLE.print(f"  [yellow]Missing dependency DLL:[/] {dll}")

    # windeployqt
    cmd  = [
        str(windeployqt),
        "--dir", str(out_dir),
        "--compiler-runtime",
        str(out_dir / exe.name),
    ]
    code = run(cmd, preset_name=state["preset"])
    if code == 0:
        CONSOLE.print("[green]✓ Qt DLLs bundled[/]")
    else:
        CONSOLE.print(f"[red]✗ windeployqt failed (exit {code})[/]")
        return

    # Copy translations
    out_trans = out_dir / "translations"
    out_trans.mkdir(exist_ok=True)
    qm_candidates = []
    bin_translations = build_dir / "bin" / "translations"
    if bin_translations.exists():
        qm_candidates.extend(sorted(bin_translations.glob("*.qm")))
    else:
        qm_candidates.extend(sorted(build_dir.glob("apps/signal_viewer/gui/*.qm")))
    for qm in qm_candidates:
        shutil.copy2(qm, out_trans / qm.name)
        CONSOLE.print(f"  [dim]Translation:[/] {qm.name}")

    _copy_mingw_runtime(state["preset"], out_dir)
    bundled_libzip_runtime = _copy_mingw_libzip_runtime(state["preset"], out_dir)

    deployed_files = {p.name for p in out_dir.iterdir() if p.is_file()}
    missing_dependencies = sorted(name for name in copied_dependency_dlls if name not in deployed_files)
    if missing_dependencies:
        CONSOLE.print(
            f"[red]✗ Deploy incomplete — missing dependency DLL(s): {', '.join(missing_dependencies)}[/]"
        )
        return

    missing_project_runtime = sorted(name for name in project_runtime_dlls if name not in deployed_files)
    if missing_project_runtime:
        CONSOLE.print(
            f"[red]✗ Deploy incomplete — missing project runtime DLL(s): "
            f"{', '.join(missing_project_runtime)}[/]"
        )
        return

    missing_libzip_runtime = sorted(name for name in bundled_libzip_runtime if name not in deployed_files)
    if missing_libzip_runtime:
        CONSOLE.print(
            f"[red]✗ Deploy incomplete — missing libzip runtime DLL(s): "
            f"{', '.join(missing_libzip_runtime)}[/]"
        )
        return

    CONSOLE.print(f"\n[bold green]✓ App deployed to:[/] {out_dir}")


def action_docker_build_image(meta: dict) -> None:
    CONSOLE.print(Rule("[bold cyan]Build Docker image"))
    if not meta.get("docker_image_available", True):
        CONSOLE.print("[red]docker_image_available is false in project.json — skipping.[/]")
        return
    image      = meta.get("docker_image_name", "signal-viewer-builder")
    dockerfile = PROJECT_ROOT / "Dockerfile.ubuntu"
    if not dockerfile.exists():
        dockerfile = PROJECT_ROOT / "Dockerfile.alpine"
    if not dockerfile.exists():
        CONSOLE.print("[red]Dockerfile not found at project root.[/]")
        return
    CONSOLE.print(f"Building image [bold]{image}[/] from {dockerfile} ...")
    code = run(["docker", "build", "-f", str(dockerfile), "-t", image, str(PROJECT_ROOT)])
    if code == 0:
        CONSOLE.print(f"[bold green]✓ Image '{image}' built.[/]")
    else:
        CONSOLE.print(f"[bold red]✗ docker build failed (exit {code})[/]")


def _is_git_worktree(path: Path) -> bool:
    """True when `path` is the root of a usable git checkout.

    A submodule's .git is a *file* pointing into the superproject, so existence
    alone is not enough — the link is checked by asking git itself, which also
    catches the broken case where .git/modules/<name> has gone missing.
    """
    if not (path / ".git").exists():
        return False
    r = subprocess.run(["git", "-C", str(path), "rev-parse", "--git-dir"],
                       capture_output=True, text=True)
    return r.returncode == 0


def _submodule_remote(path: Path) -> str:
    r = subprocess.run(["git", "-C", str(path), "remote", "get-url", "origin"],
                       capture_output=True, text=True)
    return r.stdout.strip() if r.returncode == 0 else ""


def _clone_dependency(git: str, sub: dict, target: Path) -> bool:
    """Clone a declared dependency into `target`, at its branch/tag when given."""
    url = sub.get("url", "")
    if not url:
        CONSOLE.print(f"  [red]✗ {sub.get('name', '?')}: no url declared in project.json[/]")
        return False

    target.parent.mkdir(parents=True, exist_ok=True)
    cmd = [git, "clone", url, str(target)]
    branch = sub.get("branch") or ""
    if branch:
        cmd[2:2] = ["--branch", branch]
    if run(cmd) != 0:
        return False

    tag = sub.get("tag") or ""
    if tag and run([git, "-C", str(target), "checkout", tag]) != 0:
        return False
    return True


def action_sync_submodules(meta: dict) -> None:
    """Bring every dependency declared in project.json "submodules" into shape.

    Works whether or not the project itself is a git repository, and repairs
    the states that `git submodule update` cannot: a plain directory sitting
    where a checkout should be, or a submodule whose gitlink no longer resolves.
    Such a directory is replaced by a fresh clone — it carries no history to
    lose, being either vendored files or a broken link.
    """
    CONSOLE.print(Rule("[bold cyan]Sync Dependencies"))
    submodules = meta.get("submodules", [])
    if not submodules:
        CONSOLE.print("[dim]project.json declares no submodules — nothing to sync.[/]")
        return

    git = shutil.which("git")
    if not git:
        CONSOLE.print("[red]git not found in PATH.[/]")
        return

    superproject_is_git = _is_git_worktree(PROJECT_ROOT)
    if not superproject_is_git:
        CONSOLE.print("[yellow]This project is not a git repository — dependencies are "
                      "cloned into external/ as standalone checkouts.[/]")

    failed: list[str] = []
    for sub in submodules:
        name   = sub.get("name", "?")
        url    = sub.get("url", "")
        target = PROJECT_ROOT / "external" / name
        CONSOLE.print(f"\n  [bold]{name}[/]  [dim]{url}[/]")

        if target.exists() and not _is_git_worktree(target):
            CONSOLE.print(f"  [yellow]{target.name} is not a git checkout — replacing it "
                          f"with a clone.[/]")
            _rmtree_force(target)

        if target.exists():
            remote = _submodule_remote(target)
            if url and remote and remote != url:
                CONSOLE.print(f"  [yellow]remote is {remote}, expected {url} — re-cloning.[/]")
                _rmtree_force(target)

        if not target.exists():
            if not _clone_dependency(git, sub, target):
                failed.append(name)
                continue
            CONSOLE.print(f"  [green]✓ cloned[/]")
        else:
            ref = sub.get("tag") or sub.get("branch") or ""
            run([git, "-C", str(target), "fetch", "--quiet", "origin"])
            if ref and run([git, "-C", str(target), "checkout", "--quiet", ref]) != 0:
                failed.append(name)
                continue
            CONSOLE.print(f"  [green]✓ present[/] "
                          f"[dim]{_submodule_remote(target) or 'no remote'}[/]")

    # Registered submodules still need their gitlink resolved.
    if superproject_is_git and (PROJECT_ROOT / ".gitmodules").exists():
        if run([git, "submodule", "update", "--init", "--recursive"]) != 0:
            failed.append("git submodule update --init --recursive")

    if failed:
        CONSOLE.print(f"\n[bold red]✗ Failed: {', '.join(failed)}[/]")
    else:
        CONSOLE.print("\n[bold green]✓ Dependencies synced.[/]")


def action_create_installer(state: dict, meta: dict) -> None:
    """Package the already-deployed app into a Windows installer via Inno Setup."""
    CONSOLE.print(Rule(f"[bold cyan]Create Installer — {state['preset']}"))

    if _HOST != "Windows":
        CONSOLE.print("[yellow]Inno Setup installers are Windows-only.[/]")
        return

    deploy_dir = _resolve_app_deploy_dir(state, meta)
    exe_name   = meta.get("app_executable", "signal-viewer.exe")
    if not (deploy_dir / exe_name).exists():
        CONSOLE.print(
            f"[bold red]Deployed app not found:[/] {deploy_dir / exe_name}\n"
            "[dim]Run option [7] Deploy app first.[/]"
        )
        return

    iscc = find_inno_compiler(meta)
    if not iscc:
        CONSOLE.print(
            "[bold red]ISCC.exe (Inno Setup Compiler) not found.[/]\n"
            "Set tools.inno_setup_compiler in project.json, or install it via:\n"
            "  winget install --id JRSoftware.InnoSetup -e -s winget -i"
        )
        return

    template_path = PROJECT_ROOT / "cmake" / "Templates" / "installer.iss.in"
    if not template_path.exists():
        CONSOLE.print(f"[bold red]Installer template not found:[/] {template_path}")
        return

    icon_path    = PROJECT_ROOT / "resources" / "img" / "app_icon.ico"
    license_path = PROJECT_ROOT / "LICENSE.md"
    if not meta.get("guid"):
        CONSOLE.print(
            "[bold red]project.json has no guid.[/] Run: "
            "python scripts/generate_guid.py --write"
        )
        return

    setup_dir = _resolve_setup_dir(meta)
    setup_dir.mkdir(parents=True, exist_ok=True)

    # Bump + persist the build number so the installer filename/version stay traceable.
    meta["build"] = int(meta.get("build", 0)) + 1
    save_metadata(meta)
    CONSOLE.print(f"[dim]Build number bumped to {meta['build']}[/]")

    output_basename = f"{meta['name']}-Setup-{meta['version']}-b{meta['build']}-{state['preset']}"

    tokens = {
        "@APP_NAME@":        meta["name"],
        "@APP_VERSION@":     meta["version"],
        "@APP_BUILD@":       str(meta["build"]),
        "@APP_GUID@":        meta["guid"].upper(),
        "@EXE_NAME@":        exe_name,
        "@DEPLOY_DIR@":      str(deploy_dir),
        "@OUTPUT_DIR@":      str(setup_dir),
        "@OUTPUT_BASENAME@": output_basename,
        "@ICON_PATH@":       str(icon_path),
        "@LICENSE_PATH@":    str(license_path),
    }

    script_text = template_path.read_text(encoding="utf-8")
    for token, value in tokens.items():
        script_text = script_text.replace(token, value)

    iss_path = setup_dir / f"{meta['name']}.iss"
    iss_path.write_text(script_text, encoding="utf-8")
    CONSOLE.print(f"[dim]Rendered:[/] {iss_path}")

    code = run([iscc, str(iss_path)])
    setup_exe = setup_dir / f"{output_basename}.exe"
    if code == 0 and setup_exe.exists():
        CONSOLE.print(f"\n[bold green]✓ Installer created:[/] {setup_exe}")
    else:
        CONSOLE.print(f"[bold red]✗ ISCC failed (exit {code})[/]")


def _print_tree(prefix: Path) -> None:
    table = Table(box=box.SIMPLE, header_style="bold magenta", show_header=True)
    table.add_column("Path",  style="white")
    table.add_column("Size",  justify="right", style="dim")
    for p in sorted(prefix.rglob("*")):
        if p.is_file():
            rel = p.relative_to(prefix)
            sz  = p.stat().st_size
            table.add_row(
                str(rel),
                f"{sz/1024:.1f} KB" if sz >= 1024 else f"{sz} B"
            )
    CONSOLE.print(table)


# ---------------------------------------------------------------------------
# Menu / header
# ---------------------------------------------------------------------------

def _executor_for_current(name: str) -> str:
    try:
        p = _get_preset(name)
        if p["executor"] == "native":
            return "native"
        if p["executor"] == "wsl":
            return "WSL" if _HOST == "Windows" else "native"
        return p["executor"]
    except KeyError:
        return "?"


def print_header(state: dict, meta: dict) -> None:
    try:
        preset = _get_preset(state["preset"])
        variant = preset.get("shared_variant", state["preset"])
    except KeyError:
        variant = "?"

    CONSOLE.print()
    CONSOLE.print(Panel(
        f"[bold white]{meta['name']}[/]  [dim]v{meta['version']}[/]\n"
        f"{meta.get('description', '')}\n\n"
        f"Preset       : [bold yellow]{state['preset']}[/]  "
        f"([italic]{_executor_for_current(state['preset'])}[/italic])\n"
        f"Deps variant : [dim]{variant}[/]\n"
        f"Build dir    : [dim]{PROJECT_ROOT / 'build' / state['preset']}[/]",
        title=f"[bold cyan]{meta['name']}  —  Project Manager[/]",
        border_style="cyan",
        expand=False,
    ))


def _script_menu_hint(meta: dict, key: str) -> str:
    """Menu suffix telling the user whether the per-project script is present."""
    script = _project_script_path(meta, key)
    try:
        shown = script.relative_to(PROJECT_ROOT).as_posix()
    except ValueError:
        shown = str(script)
    if script.is_file():
        return f"[dim]({shown})[/]"
    return f"[dim]({shown} — [/][yellow]not available[/][dim])[/]"


def print_menu(meta: dict) -> None:
    txt = (
        "[bold cyan]Preset[/]\n"
        "  [bold yellow][1][/] Select Preset\n\n"
        "[bold cyan]Build[/]\n"
        "  [bold yellow][2][/] Configure  (validates deps first)\n"
        "  [bold yellow][3][/] Build\n"
        "  [bold yellow][4][/] Clean current preset\n"
        "  [bold yellow][5][/] Clean ALL presets\n\n"
        "[bold cyan]Test[/]\n"
        f"  [bold yellow][6][/] Run Tests  {_script_menu_hint(meta, 'test')}\n\n"
        "[bold cyan]Deploy[/]\n"
        "  [bold yellow][7][/] Deploy app  (deploy/app/ + windeployqt)\n\n"
        "[bold cyan]Docker[/]\n"
        "  [bold yellow][8][/] Build Docker image\n\n"
        "[bold cyan]Examples[/]\n"
        f"  [bold yellow][9][/] Run Examples  {_script_menu_hint(meta, 'examples')}\n\n"
        "[bold cyan]Submodules[/]\n"
        "  [bold yellow][10][/] Sync Dependencies  (clone/repair external/)\n\n"
        "[bold cyan]Installer[/]\n"
        "  [bold yellow][11][/] Create Installer  (Inno Setup, requires Deploy app first)\n\n"
        "[bold cyan]Exit[/]\n"
        "  [bold yellow][0][/] Quit"
    )
    CONSOLE.print(Panel(txt, title="Menu", border_style="dim white", expand=False))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    meta  = load_metadata()
    cmake = find_cmake(meta)
    ctest = find_ctest(meta)

    if not cmake:
        CONSOLE.print(
            "[bold red]cmake not found.[/]\n"
            "Add CMake to PATH or install at C:/eng_apps/cmake/cmake-4.2.0/"
        )
        sys.exit(1)

    CONSOLE.print(f"[dim]cmake  : {cmake}[/]")
    CONSOLE.print(f"[dim]ctest  : {ctest or 'NOT FOUND'}[/]")
    CONSOLE.print(f"[dim]host   : {_HOST}[/]")
    docker_ok = shutil.which("docker") is not None
    CONSOLE.print(f"[dim]docker : {'available' if docker_ok else 'not found'}[/]")
    wsl_ok    = shutil.which("wsl") is not None or shutil.which("wsl.exe") is not None
    CONSOLE.print(f"[dim]wsl    : {'available' if wsl_ok or _HOST == 'Linux' else 'not found'}[/]")

    qt_root     = meta.get("qt_root",    "C:/eng_apps/Qt")
    qt_version  = meta.get("qt_version", "?")
    windeployqt = _find_windeployqt(meta)
    CONSOLE.print(f"[dim]Qt     : {qt_root}/{qt_version} "
                  f"({'windeployqt found' if windeployqt else 'windeployqt NOT found'})[/]")

    default_preset = "windows-mingw64-debug" if _HOST == "Windows" else "linux-gcc-debug"
    state: dict    = {"preset": default_preset}

    while True:
        print_header(state, meta)
        print_menu(meta)
        choice = Prompt.ask(
            "[bold]Choice",
            choices=["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"],
            default="0",
        )
        if choice == "0":
            CONSOLE.print("[bold]Bye![/]")
            break
        elif choice == "1":
            action_select_preset(state, meta)
        elif choice == "2":
            action_configure(state, cmake, meta)
        elif choice == "3":
            action_build(state, cmake, meta)
        elif choice == "4":
            action_clean(state)
        elif choice == "5":
            action_clean_all()
        elif choice == "6":
            action_run_project_script(state, meta, "test")
        elif choice == "7":
            action_deploy_app(state, cmake, meta)
        elif choice == "8":
            action_docker_build_image(meta)
        elif choice == "9":
            action_run_project_script(state, meta, "examples")
        elif choice == "10":
            action_sync_submodules(meta)
        elif choice == "11":
            action_create_installer(state, meta)

        CONSOLE.print()
        input("Press Enter to continue...")


if __name__ == "__main__":
    main()
