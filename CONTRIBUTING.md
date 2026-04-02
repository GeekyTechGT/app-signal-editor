# Contributing to MyProject

This scaffold contains template placeholders such as `MyProject`, `myprj`, `MYPRJ`, and `my_module`. Replace them consistently before treating the repository as a real product codebase.

## Purpose of This Document

This repository is private. Contribution is therefore controlled, review-driven, and intentionally explicit. The goal of this document is to make internal onboarding easier, reduce avoidable review cycles, and ensure that code, tests, documentation, and compliance material evolve together.

If you are new to the project, read the following in order:

1. `README.md`
2. `docs/product/vision.md`
3. `docs/architecture/architecture_overview.md`
4. `docs/guidelines/`
5. `SECURITY.md`
6. `CHANGELOG.md`

## What a Good Contribution Looks Like

A contribution is considered complete when it includes all applicable parts of the change:

- source code
- tests
- documentation
- configuration or schema updates
- compliance updates when licensing or redistribution implications change
- changelog entry when the change is user-visible, operationally relevant, or onboarding-relevant

## Recommended Onboarding Path

For new contributors, the fastest way to become productive is:

1. Identify the target module under `src/` and `apps/`.
2. Read the module-specific docs in `apps/<module>/docs/`.
3. Inspect the corresponding API in `include/myprj/`.
4. Review unit tests in `tests/03.unit_test/<module>/`.
5. Review end-to-end tests in `tests/04.e2e_test/<module>/`.
6. Build the relevant preset and run the affected tests before editing.

## Branching and Change Scope

Keep branches focused. A branch should ideally address one coherent concern:

- one feature
- one bug fix
- one refactor with a tightly bounded surface
- one documentation or compliance update

## Development Workflow

### 1. Build before you edit

Confirm that the repository builds on your machine before introducing changes.

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure
```

On Windows:

```batch
project_manager.bat
```

### 2. Make the smallest coherent change

Preserve existing architecture:

- domain rules stay in `core/`
- interfaces stay in `ports/`
- infrastructure belongs in `adapters/`
- application wiring belongs in `apps/`

### 3. Run the relevant tests

At minimum, run the unit tests for the touched module and e2e tests for the affected executable.

### 4. Update documentation

- `README.md` when module inventory, setup, or structure changes
- `CHANGELOG.md` when the change is notable
- module docs under `apps/<module>/docs/` when user-facing behavior changes

## Coding Expectations

- prefer clear, explicit code over clever compression
- keep domain logic deterministic
- surface failures through explicit error handling
- preserve separation between use cases, ports, and adapters
- avoid leaking UI concerns into domain code

## Build and Preset Guidance

Documented presets:

- `windows-mingw64-debug`
- `windows-mingw64-release`
- `linux-gcc-debug`
- `linux-gcc-release`

Use Windows MinGW presets when touching GUI applications. Use Linux GCC presets for fast CLI or core-library iteration.

## Testing Expectations

- `tests/01.data/` — reusable input data
- `tests/02.config/` — reusable config fixtures
- `tests/03.unit_test/` — module-focused unit coverage
- `tests/04.e2e_test/` — executable and workflow-level verification
- `tests/05.pipeline_test/` — pipeline scenarios

## Security and Sensitive Data

Read `SECURITY.md` before contributing. Never commit secrets, certificates, private keys, customer credentials, or proprietary datasets.

## Commit Guidance

Prefer commit messages that explain intent, not just file movement.

Good commit themes:

- `add my_module e2e coverage`
- `refactor my_module config validation`
- `document test layout and build presets`

A good merge request description states:

- what changed
- why it changed
- which modules are affected
- which tests were run
- whether docs were updated
