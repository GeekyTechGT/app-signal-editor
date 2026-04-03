# Template Customization Checklist

Use this checklist immediately after cloning the scaffold into a real project. The preferred workflow is: edit `project.metadata.json`, run `Initialize Project`, then run `Customize Project`.

## Mandatory Renames

Replace these placeholders everywhere they appear:

- `MyProject`: human-readable project name
- `myprj`: namespace and path prefix
- `MYPRJ`: macro and CMake option prefix
- `my_module`: example functional module name

Typical locations:

- `CMakeLists.txt`
- `CMakePresets.json`
- `include/`
- `src/`
- `apps/`
- `tests/`
- `docs/`
- `deploy/config/`
- `README.md`
- `CONTRIBUTING.md`

## Mandatory Content Updates

Replace placeholder content before first production use:

- project overview and description
- module descriptions
- product vision and PRD placeholders
- architecture documentation that still references the example module only
- deployment catalog entries
- license selection

## Recommended First Actions

1. Edit `project.metadata.json` with the real project values.
2. Run `project_manager.bat` and choose `Initialize Project`.
3. Run `project_manager.bat` and choose `Customize Project`.
4. Review `CMakePresets.json` paths and adapt them to the actual toolchain layout.
5. Review `.githooks/` and keep only the hook policy your team actually wants.
6. Update `deploy/config/module_catalog.json` with real module metadata.
7. Replace placeholder docs in `docs/product/`, `docs/specs/`, and `apps/<module>/docs/`.

## Rollback And Reports

The customization flow now stores backups and reports under `.scaffold-backups/`.

- inspect `last_customize_report.json` after each dry-run or apply
- use `Customize Project` -> rollback to restore the last backed up state
- do not delete `.scaffold-backups/` until the customization result is verified

## Quality Bar Before First Commit

Do not treat the scaffold as finished until all of the following are true:

- no business-facing README text still contains placeholder names
- the primary module builds on the intended host
- tests run from a host-appropriate preset
- git hooks are enabled and understood by the team
- the product docs describe the real problem, not template filler
- the install/export names are aligned with the final project naming
