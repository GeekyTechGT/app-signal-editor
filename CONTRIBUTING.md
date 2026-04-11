# Contributing to Signal Editor

This repository is treated as a production-grade engineering codebase. Contributions are expected to preserve architectural boundaries, keep the editing workflow stable, and update the surrounding documentation when behavior changes.

## Read This First

Recommended onboarding order:

1. `README.md`
2. `docs/product/vision.md`
3. `docs/product/prd.md`
4. `docs/specs/srs.md`
5. `docs/architecture/architecture_overview.md`
6. `SECURITY.md`
7. `CHANGELOG.md`

## Definition of Done

A contribution is complete when it includes every applicable part of the change:

- source code
- tests
- updated product or architecture documentation when scope changed
- updated requirements when expected behavior changed
- changelog entry when the change is user-visible or workflow-relevant

## Architectural Expectations

Keep the existing module boundaries intact:

- `core/domain/` owns invariants and pure editing logic
- `core/usecases/` orchestrates workflows and failure handling
- `ports/` define abstractions consumed by the core
- `adapters/` handle frameworks, UI, and persistence details
- `apps/` stay thin and only wire dependencies together

Do not leak Qt types or filesystem concerns into the domain layer.

## Development Workflow

### 1. Start from a clean understanding

Before editing:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure
```

If the change touches the GUI, also validate the corresponding Windows MinGW preset.

### 2. Make the smallest coherent change

Keep branches focused. A branch should ideally address one concern:

- one feature
- one bug fix
- one refactor with bounded impact
- one documentation/requirements update

### 3. Preserve UX quality

Signal Editor is an interactive desktop tool. UI changes must improve clarity, responsiveness, or discoverability. Avoid decorative churn that does not improve actual usage.

### 4. Update docs with intent

Update these documents when relevant:

- `README.md` for setup, structure, or workflow changes
- `docs/product/` for user-facing scope or goals
- `docs/specs/srs.md` for behavioral requirements
- `docs/architecture/` for structural or dependency changes
- `CHANGELOG.md` for notable changes

## Coding Standards

- Prefer explicit code over compact cleverness.
- Preserve deterministic behavior in editing and export flows.
- Keep public APIs documented with Doxygen when they are part of module boundaries.
- Remove dead code and stale comments rather than layering new behavior on top of them.
- Keep error reporting actionable for both UI and test diagnostics.

## Testing Expectations

At minimum, run tests that cover every touched behavior. For changes in this repository that usually means:

- domain tests for waveform invariants and interpolation
- service tests for editing workflows and error handling
- CSV repository tests for round-trip behavior and metadata handling

## Commit Guidance

Use concise, professional English commit messages that describe intent.

Good examples:

- `Refine Signal Editor workspace UX and plot interactions`
- `Document Signal Editor architecture and product scope`
- `Tighten CSV repository metadata round-trip coverage`

## Pull Request Checklist

- [ ] Code matches the intended architecture
- [ ] Tests were run and relevant results are recorded
- [ ] Documentation was updated where applicable
- [ ] Dead code and stale comments were removed
- [ ] The change leaves the repository easier to understand than before
