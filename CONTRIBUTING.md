# Contributing to Signal Editor

Signal Editor is maintained as a production-grade engineering repository. Contributions are expected to improve the product without weakening architectural boundaries, file-format guarantees, documentation quality, or user workflow clarity.

This guide describes the working expectations for contributors and reviewers.

## Recommended Reading Order

Before making material changes, read the repository in this order:

1. `README.md`
2. `GOVERNANCE.md`
3. `docs/product/vision.md`
4. `docs/product/prd.md`
5. `docs/specs/srs.md`
6. `docs/architecture/architecture_overview.md`
7. `SECURITY.md`
8. `CHANGELOG.md`

## Definition of Done

A change is only complete when every affected artifact has been updated.

That usually means:

- source code
- automated tests for changed behavior
- product documentation when user-visible behavior changed
- architecture documentation when structure or boundaries changed
- specifications when requirements or semantics changed
- changelog entries when the change matters to users or maintainers

## Architectural Expectations

Keep the established boundaries intact.

- `core/domain/` owns invariants and editing rules
- `core/usecases/` orchestrates workflows and failure handling
- `ports/` define abstractions consumed by the core
- `adapters/` implement framework and infrastructure behavior
- `apps/` remain thin composition roots

Do not move Qt types, filesystem concerns, or persistence-specific behavior into the domain layer.

## Product and UX Expectations

Signal Editor is not a generic desktop shell. It is a specialized waveform-editing tool for engineering workflows.

UI changes should therefore improve one or more of the following:

- discoverability
- edit speed
- state clarity
- error prevention
- format transparency
- workflow consistency between plot and table editing

Avoid visual churn that changes the interface without improving task execution.

## Documentation Expectations

Documentation is part of the deliverable.

Update these documents when relevant:

- `README.md` for setup, repository structure, format support, and overall workflow
- `GOVERNANCE.md` when repository operating rules or decision boundaries change
- `docs/product/vision.md` when target users or strategic goals change
- `docs/product/prd.md` when product capabilities or priorities change
- `docs/specs/srs.md` when behavior or non-functional expectations change
- `docs/architecture/*` when the design, runtime flow, or boundaries change
- `CHANGELOG.md` for release-facing visibility

## Development Workflow

### 1. Start from current repository state

Understand the existing implementation before editing it. Do not assume that documentation or code from older iterations still reflects the current system.

### 2. Keep changes coherent

A branch or commit should ideally cover one bounded concern:

- one feature
- one bug fix
- one refactor
- one documentation alignment effort

### 3. Avoid unrelated edits

Do not mix unrelated cleanup, generated churn, or unrelated deletions into the same change unless those edits are required to make the primary change safe.

### 4. Preserve explicit semantics

Signal Editor has behavior that matters beyond what the UI visibly shows, including:

- interpolation rules
- enumerated mapping persistence
- multi-format load/save behavior
- undo behavior
- active workspace semantics

Be careful not to change these implicitly.

## Coding Standards

- Prefer explicit, readable code over compact cleverness.
- Keep error handling actionable for both UI users and developers.
- Use Doxygen comments on public module boundaries where they improve discoverability.
- Remove dead code and stale comments rather than layering new behavior around them.
- Preserve deterministic behavior in parsing, editing, and export flows.

## Testing Expectations

Run the most relevant checks for the area you touched.

Typical expectations include:

- domain tests for invariants, interpolation, and enumerated behavior
- service tests for workflow-level editing behavior
- repository tests for round-trip persistence and format parsing
- manual GUI validation when workspace or interaction behavior changed

## Commit Guidance

Use concise, professional English commit messages that communicate intent.

Good examples:

- `Refine workspace navigation and shared interpolation controls`
- `Add enumerated signal fixtures for multi-format persistence coverage`
- `Document Signal Editor governance and delivery expectations`

## Review Checklist

Before opening a change for review, verify:

- the code respects architectural boundaries
- tests cover the modified behavior where practical
- documentation matches the implementation
- no stale scaffold language remains in active docs
- no unrelated repository churn was introduced
- user-visible changes are reflected in the changelog when appropriate
